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

#include "kftptransferdir.h"
#include "kftpqueue.h"
#include "kftpsession.h"
#include "queuegroup.h"

#include <kstandarddirs.h>

using namespace KFTPEngine;
using namespace KFTPSession;

namespace KFTPQueue {

TransferDir::TransferDir(QObject *parent)
  : Transfer(parent, Transfer::Directory),
    m_scanned(false),
    m_group(new QueueGroup(this)),
    m_srcScanner(0),
    m_executionMode(Default)
{
  // Connect to some group signals
  connect(m_group, SIGNAL(interrupted()), this, SLOT(slotGroupInterrupted()));
  connect(m_group, SIGNAL(done()), this, SLOT(slotGroupDone()));
}

void TransferDir::execute()
{
  // Assign sessions if they are missing
  if (!connectionsReady() && !assignSessions(m_srcSession, m_dstSession))
    return;
  
  if ((m_dstConnection && !m_dstConnection->isConnected()) || (m_srcConnection && !m_srcConnection->isConnected())) {
    // If not yet connected, wait for a connection
    m_status = Connecting;
    return;
  }
  
  switch (m_executionMode) {
    case Ignore: {
      // We should just deinitialize connections and return
      deinitializeConnections();
      break;
    }
    case ScanOnly:
    case ScanWithExecute: {
      // We should initiate a scan
      if (m_srcSession) {
        m_srcSession->scanDirectory(this, m_srcConnection);
        m_scanned = true;
        
        connect(m_srcSession, SIGNAL(dirScanDone()), this, SLOT(slotDirScanDone()));
      } else {
        m_srcScanner = new DirectoryScanner(this);
        connect(m_srcScanner, SIGNAL(completed()), this, SLOT(slotDirScanDone()));
      }
      
      // Switch execution modes
      if (m_executionMode == ScanWithExecute)
        m_executionMode = Default;
      else
        m_executionMode = Ignore;
      break;
    }
    default: {
      // We should just execute the transfer
      if (!m_scanned && !hasParentTransfer() && m_children.count() == 0) {
        m_executionMode = ScanWithExecute;
        return execute();
      }
      
      m_status = Running;
  
      // If the directory is empty, create it anyway
      if (m_children.count() == 0) {
        if (m_destUrl.isLocalFile()) {
          KStandardDirs::makeDir(m_destUrl.path());
        } else {
          m_dstSession->getClient()->mkdir(m_destUrl);
        }
      }
      
      // We no longer need the connections, release them
      deinitializeConnections();
      
      // Reset and start the group
      m_group->reset();
      m_group->executeNextTransfer();
      break;
    }
  }
}

void TransferDir::scan()
{
  if (isLocked() || isRunning())
    return;
  
  m_executionMode = ScanOnly;
  execute();
}

void TransferDir::abort()
{
  if (isLocked()) {
    // The transfer is locked because a scan is in progress
    if (m_srcSession) {
      disconnect(m_srcSession, SIGNAL(dirScanDone()), this, SLOT(slotDirScanDone()));
      m_srcSession->abort();
    } else if (m_srcScanner) {
      disconnect(m_srcScanner, SIGNAL(completed()), this, SLOT(slotDirScanDone()));
      m_srcScanner->abort();
    }
  }
  
  // If not running, just return
  if (!isRunning())
    return;
  
  Transfer::abort();
  
  // Signal abort to all child transfers
  if (!m_deleteMe) {
    foreach (QueueObject *i, m_children) {
      if (i->isRunning() && !i->isAborting())
        i->abort();
    }
  }
  
  resetTransfer();
  update();
}

void TransferDir::slotGroupDone()
{
  // There are no more transfers, so we are finished
  showTransCompleteBalloon();
  m_deleteMe = true;
  resetTransfer();
  
  emit transferComplete(m_id);
  KFTPQueue::Manager::self()->doEmitUpdate();
}

void TransferDir::slotGroupInterrupted()
{
  if (!m_aborting)
    abort();
}

void TransferDir::slotDirScanDone()
{
  if (m_srcSession)
    disconnect(m_srcSession, SIGNAL(dirScanDone()), this, SLOT(slotDirScanDone()));
  else
    disconnect(m_srcScanner, SIGNAL(completed()), this, SLOT(slotDirScanDone()));
  
  // Reexecute the transfer
  delayedExecute();
}

}

#include "kftptransferdir.moc"
