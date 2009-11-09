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

#include "kftptransferfile.h"
#include "widgets/systemtray.h"
#include "kftpsession.h"
#include "statistics.h"

#include "engine/thread.h"

#include "misc/config.h"

#include <kmessagebox.h>
#include <klocale.h>
#include <kio/renamedlg.h>
#include <kdiskfreespace.h>

#include <qtimer.h>
#include <qfileinfo.h>

using namespace KFTPEngine;
using namespace KFTPSession;

namespace KFTPQueue {

TransferFile::TransferFile(QObject *parent)
  : Transfer(parent, Transfer::File),
    m_updateTimer(0),
    m_dfTimer(0)
{
}

bool TransferFile::assignSessions(Session *source, Session *destination)
{
  if (!Transfer::assignSessions(source, destination))
    return false;
      
  // Connect signals
  if (m_srcConnection) {
    connect(m_srcConnection->getClient()->eventHandler(), SIGNAL(engineEvent(KFTPEngine::Event*)), this, SLOT(slotEngineEvent(KFTPEngine::Event*)));
    connect(m_srcConnection, SIGNAL(aborting()), this, SLOT(slotSessionAborting()));
    connect(m_srcConnection, SIGNAL(connectionLost(KFTPSession::Connection*)), this, SLOT(slotConnectionLost(KFTPSession::Connection*)));
  }
  
  if (m_dstConnection) {
    connect(m_dstConnection->getClient()->eventHandler(), SIGNAL(engineEvent(KFTPEngine::Event*)), this, SLOT(slotEngineEvent(KFTPEngine::Event*)));
    connect(m_dstConnection, SIGNAL(aborting()), this, SLOT(slotSessionAborting()));
    connect(m_dstConnection, SIGNAL(connectionLost(KFTPSession::Connection*)), this, SLOT(slotConnectionLost(KFTPSession::Connection*)));
  }
  
  return true;
}

void TransferFile::execute()
{
  // Failed transfers aren't allowed to be executed until they are readded to
  // the queue and the Failed status is changed to Stopped.
  if (getStatus() == Failed)
    return;
    
  // Assign sessions if they are missing
  if (!connectionsReady() && !assignSessions(m_srcSession, m_dstSession))
    return;
  
  if ((m_dstConnection && !m_dstConnection->isConnected()) || (m_srcConnection && !m_srcConnection->isConnected())) {
    m_status = Connecting;
    return;
  }
  
  // We are running now
  m_status = Running;
  
  m_completed = 0;
  m_resumed = 0;
  
  // Init timer to follow the update
  if (!m_updateTimer) {
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, SIGNAL(timeout()), this, SLOT(slotTimerUpdate()));
    m_updateTimer->start(1000);
  }
  
  // Should we check for free space ?
  if (KFTPCore::Config::diskCheckSpace() && !m_dfTimer) {
    m_dfTimer = new QTimer(this);
    connect(m_dfTimer, SIGNAL(timeout()), this, SLOT(slotTimerDiskFree()));
    m_dfTimer->start(KFTPCore::Config::diskCheckInterval() * 1000);
  }
  
  emit transferStart(m_id);
  
  switch(m_transferType) {
    case Download: {
      m_srcConnection->getClient()->get(m_sourceUrl, m_destUrl);
      break;
    }
    case Upload: {
      m_dstConnection->getClient()->put(m_sourceUrl, m_destUrl);
      break;
    }
    case FXP: {
      // Start the timer to extrapolate transfer rate
      m_elapsedTime.start();
      m_srcConnection->getClient()->siteToSite(m_dstConnection->getClient(), m_sourceUrl, m_destUrl);
      break;
    }
  }
}

void TransferFile::slotConnectionLost(KFTPSession::Connection *connection)
{
  if (!isRunning())
    return;

  if (m_status != Connecting) {
    // Semi-reset the current transfer
    addCompleted(-m_completed);
    
    m_resumed = 0;
    m_completed = 0;
    m_aborting = false;
    m_size = m_actualSize;
    
    setSpeed(0);
    
    // Wait for the connection to become available
    m_status = Connecting;
    connection->reconnect();
  } else {
    connection->reconnect();
  }
}

void TransferFile::slotEngineEvent(KFTPEngine::Event *event)
{
  if (!isRunning())
    return;
  
  switch (event->type()) {
    case Event::EventTransferComplete: {
      // ***************************************************************************
      // ************************ EventTransferComplete ****************************
      // ***************************************************************************
      // Calculate transfer rate for last transfer, and save to site's statistics
      if (getTransferType() == FXP) {
        if (m_elapsedTime.elapsed() > 10000) {
          double speed = (m_size - m_resumed) / (double) m_elapsedTime.elapsed();
          Statistics::self()->getSite(m_sourceUrl)->setLastFxpSpeed(speed * 1024);
        }
      }
      
      // Update the completed size if the transfer was faster than the update timer
      addCompleted(m_size - m_completed);
  
      m_updateTimer->stop();
      m_updateTimer->QObject::disconnect();
      
      if (m_openAfterTransfer && m_transferType == Download) {
        // Set status to stopped, so the view gets reloaded
        m_status = Stopped;
        
        Manager::self()->openAfterTransfer(this);
      } else {
        showTransCompleteBalloon();
      }
      
      m_deleteMe = true;
      addActualSize(-m_size);
      
      resetTransfer();
      emit transferComplete(m_id);
      
      KFTPQueue::Manager::self()->doEmitUpdate();
      break;
    }
    case Event::EventResumeOffset: {
      // ***************************************************************************
      // ************************** EventResumeOffset ******************************
      // ***************************************************************************
      m_resumed = event->getParameter(0).toULongLong();
      addCompleted(m_resumed);
      break;
    }
    case Event::EventError: {
      // ***************************************************************************
      // ****************************** EventError *********************************
      // ***************************************************************************
      ErrorCode error = static_cast<ErrorCode>(event->getParameter(0).toInt());
      
      switch (error) {
        case ConnectFailed: {
          FailedTransfer::fail(this, i18n("Connection to the server has failed."));
          break;
        }
        case LoginFailed: {
          FailedTransfer::fail(this, i18n("Login to the server has failed."));
          break;
        }
        case FileNotFound: {
          FailedTransfer::fail(this, i18n("Source file cannot be found."));
          break;
        }
        case PermissionDenied: {
          FailedTransfer::fail(this, i18n("Permission was denied."));
          break;
        }
        case FileOpenFailed: {
          FailedTransfer::fail(this, i18n("Unable to open local file for read or write operations."));
          break;
        }
        case OperationFailed: {
          FailedTransfer::fail(this, i18n("Transfer failed for some reason."));
          break;
        }
        default: break;
      }
      
      break;
    }
    case Event::EventFileExists: {
      // ***************************************************************************
      // *************************** EventFileExists *******************************
      // ***************************************************************************
      DirectoryListing list = event->getParameter(0).value<DirectoryListing>();
      FileExistsWakeupEvent *event = Manager::self()->fileExistsAction(this, list.list());
      
      if (event)
        remoteConnection()->getClient()->wakeup(event);
      break;
    }
    default: break;
  }
}

void TransferFile::wakeup(KFTPEngine::FileExistsWakeupEvent *event)
{
  if (event)
    remoteConnection()->getClient()->wakeup(event);
}

void TransferFile::slotTimerUpdate()
{
  // Update the current stats
  if (!m_srcSession && !m_dstSession) {
    m_updateTimer->stop();
    m_updateTimer->QObject::disconnect();
    return;
  }
  
  if (m_status == Running) {
    // Get speed from connection, or use FXP extrapolation.
    if (getTransferType() == FXP) {
      double lastFxpSpeed = Statistics::self()->getSite(m_sourceUrl)->lastFxpSpeed();
      
      if (lastFxpSpeed != 0.0) {
        setSpeed(lastFxpSpeed);
        
        if (m_completed < m_size)
          addCompleted(getSpeed());
      }
    } else {
      Socket *socket = remoteConnection()->getClient()->socket();
      
      if (socket->getTransferBytes() > m_completed - m_resumed)
        addCompleted(socket->getTransferBytes() - (m_completed - m_resumed));

      setSpeed(socket->getTransferSpeed());
    }
  }

  update();
}

void TransferFile::slotTimerDiskFree()
{
  // Check for disk usage
  if (KFTPCore::Config::diskCheckSpace()) {
    KDiskFreeSpace *df = KDiskFreeSpace::findUsageInfo((getDestUrl().path()));
    connect(df, SIGNAL(foundMountPoint(const QString&, unsigned long, unsigned long, unsigned long)), this, SLOT(slotDiskFree(const QString&, unsigned long, unsigned long, unsigned long)));
  }
}

void TransferFile::slotDiskFree(const QString &mountPoint, unsigned long, unsigned long, unsigned long kBAvail)
{
  if (KFTPCore::Config::diskCheckSpace()) {
    // Is there enough free space ?
    if (kBAvail < (unsigned long) KFTPCore::Config::diskMinFreeSpace()) {
      QString transAbortStr = i18n("Transfer of the following files <b>has been aborted</b> because there is not enough free space left on '%1':",mountPoint);
      transAbortStr += "<br><i>";
      transAbortStr += getSourceUrl().fileName();
      transAbortStr += "</i>";
      KFTPWidgets::SystemTray::self()->showMessage(i18n("Information"), transAbortStr);
      
      // Abort the transfer
      abort();
    }
  }
}

void TransferFile::resetTransfer()
{
  // Unlock the sessions (they should be unlocked automaticly when the transferComplete signal
  // is emitted, but when a transfer is a child transfer, the next transfer may need the session
  // sooner). Also sessions should be unlocked when transfer aborts.
  if (getStatus() != Waiting) {
    // Disconnect signals
    if (m_srcConnection) {
      m_srcConnection->getClient()->eventHandler()->QObject::disconnect(this, SLOT(slotEngineEvent(KFTPEngine::Event*)));
      m_srcConnection->QObject::disconnect(this, SLOT(slotSessionAborting()));
      m_srcConnection->QObject::disconnect(this, SLOT(slotConnectionLost(KFTPSession::Connection*)));
      
      m_srcConnection->release();
    }
    
    if (m_dstConnection) {
      m_dstConnection->getClient()->eventHandler()->QObject::disconnect(this, SLOT(slotEngineEvent(KFTPEngine::Event*)));
      m_dstConnection->QObject::disconnect(this, SLOT(slotSessionAborting()));
      m_dstConnection->QObject::disconnect(this, SLOT(slotConnectionLost(KFTPSession::Connection*)));
      
      m_dstConnection->release();
    }
  }
  
  Transfer::resetTransfer();
}

void TransferFile::slotSessionAborting()
{
  if (!m_aborting)
    abort();
}

void TransferFile::abort()
{
  if (!isRunning())
    return;
  
  Transfer::abort();
  
  if (getStatus() == Waiting) {
    if (m_srcSession)
      m_srcSession->QObject::disconnect(this, SLOT(slotConnectionAvailable()));
      
    if (m_dstSession)
      m_dstSession->QObject::disconnect(this, SLOT(slotConnectionAvailable()));
  }
  
  if (m_updateTimer) {
    m_updateTimer->stop();
    m_updateTimer->QObject::disconnect();
    
    delete m_updateTimer;
    m_updateTimer = 0L;
  }
  
  if (m_dfTimer) {
    m_dfTimer->stop();
    m_dfTimer->QObject::disconnect();
    
    delete m_dfTimer;
    m_dfTimer = 0L;
  }

  // Abort any transfers
  if (m_srcConnection)
    m_srcConnection->abort();
  
  if (m_dstConnection)
    m_dstConnection->abort();
  
  // Update everything
  resetTransfer();
  
  if (!hasParentTransfer())
    update();
  
  if (hasParentObject() && parentObject()->isAborting())
    disconnect(parent());
}

}


#include "kftptransferfile.moc"
