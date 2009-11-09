/*
 * This file is part of the KFTPGrabber project
 *
 * Copyright (C) 2003-2007 by the KFTPGrabber developers
 * Copyright (C) 2003-2007 Jernej Kos <kostko@jweb-network.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * is provided AS IS, WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, and
 * NON-INFRINGEMENT.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 *
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
#include "speedlimiter.h"
#include "misc/config.h"

#include <KGlobal>

using namespace KFTPCore;

namespace KFTPEngine {

static const int tickDelay = 250;
static int bucketSize = 1000 / tickDelay;

class SpeedLimiterPrivate
{
public:
    SpeedLimiter instance;
};

K_GLOBAL_STATIC(SpeedLimiterPrivate, speedLimiterPrivate)

SpeedLimiter *SpeedLimiter::self()
{
  return &speedLimiterPrivate->instance;
}

SpeedLimiter::SpeedLimiter()
  : m_timer(new QTimer(this))
{
  // Reset limits and token debts
  m_limits[0] = 0;
  m_limits[1] = 0;
  
  m_tokenDebt[0] = 0;
  m_tokenDebt[1] = 0;
  
  connect(m_timer, SIGNAL(timeout()), this, SLOT(synchronize()));
  connect(this, SIGNAL(activateTimer(int)), m_timer, SLOT(start(int)));
  
  // Subscribe to config updates and update the limits
  connect(Config::self(), SIGNAL(configChanged()), this, SLOT(updateLimits()));
  updateLimits();
}

SpeedLimiter::~SpeedLimiter()
{
}

void SpeedLimiter::updateLimits()
{
  setLimit(SpeedLimiter::Download, Config::downloadSpeedLimit() * 1024);
  setLimit(SpeedLimiter::Upload, Config::uploadSpeedLimit() * 1024);
}

void SpeedLimiter::setLimit(Type type, int limit)
{
  m_limits[type] = limit;
}

void SpeedLimiter::append(SpeedLimiterItem *item, Type type)
{
  m_objects[type].append(item);
  
  int limit = m_limits[type];
  if (limit > 0) {
    int tokens = limit * tickDelay / 1000;
    tokens /= m_objects[type].count();
    
    if (m_tokenDebt[type] > 0) {
      if (tokens >= m_tokenDebt[type]) {
        tokens -= m_tokenDebt[type];
        m_tokenDebt[type] = 0;
      } else {
        tokens = 0;
      }
    }
    
    item->m_availableBytes = tokens;
  } else {
    item->m_availableBytes = -1;
  }
  
  // Fire the timer if not running
  if (!m_timer->isActive())
    emit activateTimer(tickDelay);
}

void SpeedLimiter::remove(SpeedLimiterItem *item)
{
  remove(item, Download);
  remove(item, Upload);
}

void SpeedLimiter::remove(SpeedLimiterItem *item, Type type)
{
  if (m_objects[type].contains(item)) {
    int tokens = m_limits[type] * tickDelay / 1000;
    tokens /= m_objects[type].count();
    
    if (item->m_availableBytes < tokens)
      m_tokenDebt[type] += tokens - item->m_availableBytes;
    
    m_objects[type].removeAll(item);
  }
  
  item->m_availableBytes = -1;
}

void SpeedLimiter::synchronize()
{
  QList<SpeedLimiterItem> pendingWakeup;
  
  for (int i = 0; i < 2; i++) {
    m_tokenDebt[i] = 0;
    
    int limit = m_limits[i];
    if (!limit) {
      // There is no limit, reset all items
      foreach (SpeedLimiterItem *item, m_objects[i]) {
        item->m_availableBytes = -1;
      }
      
      continue;
    }
    
    // If there are no objects, just skip it
    if (m_objects[i].isEmpty())
      continue;
    
    int tokens = limit * tickDelay / 1000;
    if (!tokens)
      tokens = 1;
    
    int maxTokens = tokens * bucketSize;
    
    // Get amount of tokens for each object
    int tokensPerObject = tokens / m_objects[i].count();
    if (!tokensPerObject)
      tokensPerObject = 1;
    
    tokens = 0;
    
    QList<SpeedLimiterItem*> unsaturatedObjects;
    
    foreach (SpeedLimiterItem *item, m_objects[i]) {
      if (item->m_availableBytes == -1) {
        item->m_availableBytes = tokensPerObject;
        unsaturatedObjects.append(item);
      } else {
        item->m_availableBytes += tokensPerObject;
        
        if (item->m_availableBytes > maxTokens) {
          tokens += item->m_availableBytes - maxTokens;
          item->m_availableBytes = maxTokens;
        } else {
          unsaturatedObjects.append(item);
        }
      }
    }
    
    // Assign any left-overs to unsaturated sources
    while (tokens && !unsaturatedObjects.isEmpty()) {
      tokensPerObject = tokens / unsaturatedObjects.count();
      if (!tokensPerObject)
        break;
      
      tokens = 0;
      
      foreach (SpeedLimiterItem *item, unsaturatedObjects) {
        item->m_availableBytes += tokensPerObject;
        
        if (item->m_availableBytes > maxTokens) {
          tokens += item->m_availableBytes - maxTokens;
          item->m_availableBytes = maxTokens;
          unsaturatedObjects.removeAll(item);
        }
      }
    }
  }
  
  if (m_objects[0].isEmpty() && m_objects[1].isEmpty())
    m_timer->stop();
}

SpeedLimiterItem::SpeedLimiterItem()
  : m_availableBytes(-1)
{
}

void SpeedLimiterItem::updateUsage(int bytes)
{
  // Ignore if there are no limits
  if (m_availableBytes == -1)
    return;
  
  if (bytes > m_availableBytes)
    m_availableBytes = 0;
  else
    m_availableBytes -= bytes;
}

}

#include "speedlimiter.moc"
