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

#include "kftptransfer.h"
#include "kftptransferdir.h"
#include "widgets/systemtray.h"
#include "kftpqueue.h"
#include "kftpsession.h"

#include "misc/config.h"

#include <klocale.h>

using namespace KFTPSession;

namespace KFTPQueue {

FailedTransfer::FailedTransfer(QObject *parent, TransferFile *transfer, const QString &error)
  : QObject(parent),
    m_transfer(transfer),
    m_error(error)
{
 
  // Reparent the transfer, we can't remove it from the QueueObject's children list yet, because
  // that will cause an iterator reset for directories, so we'll do that later in the fail method.
  transfer->parentObject()->addSize(-transfer->m_actualSize);
  KFTPQueue::Manager::self()->failedTransfers()->append(this);
 
  // Check if the transfer's site should be removed as well
  QueueObject *site = 0;
  if (transfer->parentObject()->getType() == QueueObject::Site && transfer->parentObject()->getChildrenList().count() == 1) {
    site = transfer->parentObject();
    emit KFTPQueue::Manager::self()->objectRemoved(site);
  }
 
  // Complete reparenting and mark transfer as failed
  transfer->setParent(this);
  transfer->m_status = Transfer::Failed;
  delete site;
}

FailedTransfer::~ FailedTransfer()
{
}

TransferFile *FailedTransfer::restore()
{
  // Emit failed transfer removal
  emit KFTPQueue::Manager::self()->failedTransferBeforeRemoval(this);
  
  // Add the transfer back to the queue
  m_transfer->setParent(0);
  KFTPQueue::Manager::self()->insertTransfer(m_transfer);
  
  // Change the transfer's status, so it can be started
  m_transfer->m_status = Transfer::Stopped;
  
  // This object is now useless, so it shall be removed
  KFTPQueue::Manager::self()->failedTransfers()->removeAll(this);
  emit KFTPQueue::Manager::self()->failedTransferAfterRemoval();
  deleteLater();
  
  return m_transfer;
}

void FailedTransfer::fail(TransferFile *transfer, const QString &error)
{
  // Should the transfer be retried
  if (KFTPCore::Config::failedAutoRetry() && transfer->m_retryCount < KFTPCore::Config::failedAutoRetryCount()) {
    // Semi-reset the current transfer
    transfer->addCompleted(-transfer->m_completed);
    
    transfer->m_retryCount++;
    transfer->m_resumed = 0;
    transfer->m_completed = 0;
    transfer->m_aborting = false;
    transfer->m_size = transfer->m_actualSize;
    
    transfer->setSpeed(0);
    
    // Restart the transfer
    transfer->delayedExecute();
  } else {
    // Abort the transfer
    transfer->blockSignals(true);
    transfer->abort();
    transfer->blockSignals(false);
    
    QPointer<QueueObject> transferParent(transfer->parentObject());
    emit KFTPQueue::Manager::self()->objectBeforeRemoval(transfer);
    
    // Create a new failed transfer (will automaticly reparent the transfer)
    FailedTransfer *ft = new FailedTransfer(KFTPQueue::Manager::self(), transfer, error);
    
    // Notify others about transfer's "completion"
    emit transfer->transferComplete(transfer->m_id);
    
    // Now that the iterators have been incremented we can remove the child without
    // doing any harm to the queue processing.
    if (transferParent)
      transferParent->delChildObject(transfer);
    
    // Fake transfer removal and signal new failed transfer
    emit KFTPQueue::Manager::self()->objectAfterRemoval();
    emit KFTPQueue::Manager::self()->failedTransferAdded(ft);
  }
}

Transfer::Transfer(QObject *parent, Type type)
 : QueueObject(parent, type),
   m_deleteMe(false),
   m_openAfterTransfer(false),
   m_srcSession(0),
   m_dstSession(0),
   m_srcConnection(0),
   m_dstConnection(0),
   m_retryCount(0)
{
}


Transfer::~Transfer()
{
}

Connection *Transfer::getOppositeConnection(Connection *conn)
{
  return m_srcConnection == conn ? m_dstConnection : m_srcConnection;
}

Connection *Transfer::remoteConnection()
{
  return m_srcConnection ? m_srcConnection : m_dstConnection;
}

Connection *Transfer::initializeSession(Session *session)
{
  if (!session->isFreeConnection()) {
    // We should wait for a connection to become available
    connect(session, SIGNAL(freeConnectionAvailable()), this, SLOT(slotConnectionAvailable()));
    
    if (m_status != Waiting) {
      m_status = Waiting;
      emit objectUpdated();
    }
    
    return 0;
  }
  
  Connection *connection = session->assignConnection();
  connection->acquire(this);
  
  connect(connection->getClient()->eventHandler(), SIGNAL(connected()), this, SLOT(slotConnectionConnected()));
  
  return connection;
}

void Transfer::deinitializeConnections()
{
  if (m_srcConnection) {
    m_srcConnection->getClient()->eventHandler()->disconnect(this);
    m_srcConnection->release();
  }
    
  if (m_dstConnection) {
    m_dstConnection->getClient()->eventHandler()->disconnect(this);
    m_dstConnection->release();
  }
}

bool Transfer::assignSessions(Session *source, Session *destination)
{
  bool result = true;
  
  // We need a source session
  if (!m_sourceUrl.isLocalFile() && !m_srcConnection) {
    if (!source)
      m_srcSession = KFTPSession::Manager::self()->spawnRemoteSession(IgnoreSide, m_sourceUrl, 0);
    else
      m_srcSession = source;
    
    if (!(m_srcConnection = initializeSession(m_srcSession)))
      result = false;
  }
  
  // We need a destination session
  if (!m_destUrl.isLocalFile() && !m_dstConnection) {
    if (!destination)
      m_dstSession = KFTPSession::Manager::self()->spawnRemoteSession(m_srcSession ? oppositeSide(m_srcSession->getSide()) : IgnoreSide, m_destUrl, 0);
    else
      m_dstSession = destination;
    
    if (!(m_dstConnection = initializeSession(m_dstSession)))
      result = false;
  }
  
  return result;
}

bool Transfer::connectionsReady()
{
  return (m_sourceUrl.isLocalFile() || m_srcConnection) && (m_destUrl.isLocalFile() || m_dstConnection); 
}

void Transfer::slotConnectionAvailable()
{
  if (getStatus() != Waiting || (m_srcSession && !m_srcSession->isFreeConnection()) || (m_dstSession && !m_dstSession->isFreeConnection()))
    return;
  
  if (m_srcSession)
    m_srcSession->QObject::disconnect(this, SLOT(slotConnectionAvailable()));
  
  if (m_dstSession)
    m_dstSession->QObject::disconnect(this, SLOT(slotConnectionAvailable()));
  
  // Connection has become available, grab it now
  execute();
}

void Transfer::slotConnectionConnected()
{
  if ((m_srcConnection && !m_srcConnection->isConnected()) || (m_dstConnection && !m_dstConnection->isConnected()))
    return;
  
  // Everything is ready for immediate execution
  delayedExecute();
}

void Transfer::faceDestruction(bool abortSession)
{
  // This method is called before the object is deleted by the queue
  // manager.
  if (hasParentObject()) {
    parentObject()->addSize(-m_actualSize);
    parentObject()->delChildObject(this);
  }
    
  // Abort any dir scans that might be in progress
  if (abortSession) {
    KFTPSession::Session *session = KFTPSession::Manager::self()->find(this);
    
    if (session)
      session->abort();
  }
}

Transfer *Transfer::parentTransfer()
{
  if (!hasParentTransfer())
    return 0L;
  
  return static_cast<Transfer*>(parent());
}

void Transfer::resetTransfer()
{
  // Disconnect signals
  if (getStatus() != Waiting) {
    if (m_srcConnection)
      m_srcConnection->getClient()->eventHandler()->QObject::disconnect(this, SLOT(slotConnectionConnected()));
    
    if (m_dstConnection)
      m_dstConnection->getClient()->eventHandler()->QObject::disconnect(this, SLOT(slotConnectionConnected()));
  }
  
  // Reset connections & session pointers
  m_srcConnection = 0L;
  m_dstConnection = 0L;
  
  m_srcSession = 0L;
  m_dstSession = 0L;
  
  m_status = Stopped;
  m_resumed = 0;
  m_completed = 0;
  m_aborting = false;
  m_size = m_actualSize;
  m_retryCount = 0;
  
  // Set the transfer speed to zero
  setSpeed(0);
}

bool Transfer::canMove()
{
  return !isRunning();
}

void Transfer::update()
{
  KFTPQueue::Manager::self()->doEmitUpdate();
  statisticsUpdated();
}

void Transfer::abort()
{
  // Unlock if the transfer was locked
  unlock();
  
  // Set the aborting flag
  m_aborting = true;
  
  emit transferAbort(m_id);
}

void Transfer::showTransCompleteBalloon()
{
  // Show a balloon :P
  if (KFTPCore::Config::showBalloons() && !KFTPQueue::Manager::self()->isProcessing()) {
    if (!KFTPCore::Config::showBalloonWhenQueueEmpty() || (KFTPQueue::Manager::self()->topLevelObject()->getChildrenList().count() == 1 && !hasParentTransfer())) {
      QString transCompleteStr = i18n("Transfer of the following files is complete:");
      transCompleteStr += "<br><i>";
      transCompleteStr += getSourceUrl().fileName();
      transCompleteStr += "</i>";
      KFTPWidgets::SystemTray::self()->showMessage(i18n("Information"), transCompleteStr);
    }
  }
}

void Transfer::lock()
{
  // Transfers can only be locked if they are not currently running
  if (m_status == Stopped) {
    m_status = Locked;
    statisticsUpdated();
  }
}

void Transfer::unlock()
{
  if (isLocked()) {
    m_status = Stopped;
    statisticsUpdated();
  }
}

}
#include "kftptransfer.moc"
