/*
 * This file is part of the KFTPGrabber project
 *
 * Copyright (C) 2003-2006 by the KFTPGrabber developers
 * Copyright (C) 2003-2006 Jernej Kos <kostko@jweb-network.net>
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
#include "queuegroup.h"
#include "queueobject.h"
#include "kftptransfer.h"
#include "kftpsession.h"

using namespace KFTPSession;

namespace KFTPQueue {

QueueGroup::QueueGroup(QueueObject *object)
  : QObject(object),
    m_object(object),
    m_childIterator(QListIterator<QueueObject*>(m_object->m_children)),
    m_directories(true)
{
}

void QueueGroup::reset()
{
  m_childIterator.toFront();
}

int QueueGroup::executeNextTransfer()
{
  // Check if there is actually something to execute
  if (!m_childIterator.hasNext()) {
    if (m_lastTransfer && m_lastTransfer->isRunning())
      return -1;
    
    if (m_object->m_children.count() <= 1)
      emit done();
    
    return -1;
  } else if (!m_directories && m_lastTransfer && m_lastTransfer->isDir()) {
    return 0;
  }
  
  Transfer *transfer = static_cast<Transfer*>(m_childIterator.next());
  
  // Check if we have enough connections available
  if (m_lastTransfer) {
    Session *sourceSession = m_lastTransfer->getSourceSession();
    Session *destinationSession = m_lastTransfer->getDestinationSession();
    
    if ((sourceSession && !sourceSession->isFreeConnection()) || (destinationSession && !destinationSession->isFreeConnection()))
      return 0;
        
    // Reserve the connections immediately
    transfer->assignSessions(sourceSession, destinationSession);
  }
  
  // Get the transfer instance and schedule it's execution
  transfer->QObject::disconnect(this);
  
  connect(transfer, SIGNAL(transferComplete(long)), this, SLOT(incrementAndExecute()));
  connect(transfer, SIGNAL(transferAbort(long)), this, SIGNAL(interrupted()));

  transfer->delayedExecute();
  
  // Prepare for the next transfer
  m_lastTransfer = transfer;
  
  return 1;
}

void QueueGroup::incrementAndExecute()
{
  if (QObject::sender()) {
    const Transfer *transfer = static_cast<const Transfer*>(QObject::sender());
    
    if (!transfer->isDir())
      m_directories = false;
  }
  
  int result = executeNextTransfer();
  
  switch (result) {
    case 0: m_childIterator.previous(); break;
    case 1: {
      m_directories = false;
      incrementAndExecute();
      break;
    }
    default: break;
  }
  
  if (result != -1 && !m_directories)
    m_directories = true;
}

}

#include "queuegroup.moc"
