/*
 * This file is part of the KFTPGrabber project
 *
 * Copyright (C) 2003-2004 by the KFTPGrabber developers
 * Copyright (C) 2003-2004 Jernej Kos <kostko@jweb-network.net>
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
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#include "kftpqueueprocessor.h"
#include "kftpqueue.h"
#include "kftpsession.h"
//Added by qt3to4:

KFTPQueueProcessor::KFTPQueueProcessor(QObject *parent)
 : QObject(parent)
{
  m_running = false;
}

bool KFTPQueueProcessor::isRunning()
{
  return m_running;
}

void KFTPQueueProcessor::processActiveSite()
{
  if (!m_activeSite)
    return;

  // Start the transfer
  m_activeSite->delayedExecute();
}

bool KFTPQueueProcessor::nextSite()
{
  // Select the first server (should be next) if there is none, we are done :)
  //KFTPQueueTransfers *transfers = FTPQueueManager->getQueueTransferList();
  QList<KFTPQueue::QueueObject*> sites = KFTPQueue::Manager::self()->topLevelObject()->getChildrenList();
  
  if (sites.count() > 0 && m_running) {
    if (m_activeSite && m_activeSite->getId() == sites.at(0)->getId()) {
      // Because this function can be called from slotSiteComplete and signal gets
      // emitted *before* site is removed we should use the second site on the list
      m_activeSite = static_cast<KFTPQueue::Site*>(sites.at(1));
    } else {
      m_activeSite = static_cast<KFTPQueue::Site*>(sites.at(0));
    }
  } else {
    m_activeSite = 0L;
  }
  
  if (m_activeSite) {
    // Connect the signals
    connect(m_activeSite, SIGNAL(destroyed(QObject*)), this, SLOT(slotSiteComplete()));
    connect(m_activeSite, SIGNAL(siteAborted()), this, SLOT(slotSiteAborted()));
    
    return true;
  } else {
    // We are done, so we emit the proper signal
    m_running = false;
    
    emit queueComplete();
    return false;
  }
  
  return false;
}

void KFTPQueueProcessor::startProcessing()
{
  m_running = true;
  
  // Select the site and process it
  if (nextSite())
    processActiveSite();
}

void KFTPQueueProcessor::stopProcessing()
{
  // Stop the queue processing
  m_running = false;
  
  // Abort current transfer
  if (m_activeSite) {
    // Disconnect signals
    m_activeSite->QObject::disconnect(this);
    m_activeSite->abort();
  }
  
  m_activeSite = 0L;
  emit queueAborted();
}

void KFTPQueueProcessor::slotSiteComplete()
{
  // Transfer is complete, move to the next
  if (nextSite())
    processActiveSite();
}

void KFTPQueueProcessor::slotSiteAborted()
{
  // Transfer has aborted, we should stop
  stopProcessing();
}

#include "kftpqueueprocessor.moc"

