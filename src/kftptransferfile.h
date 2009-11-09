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

#ifndef KFTPQUEUEKFTPTRANSFERFILE_H
#define KFTPQUEUEKFTPTRANSFERFILE_H

#include <qdatetime.h>

#include "kftptransfer.h"

namespace KFTPSession {
  class Connection;
}

namespace KFTPEngine {
  class Event;
  class FileExistsWakeupEvent;
}

namespace KFTPQueue {

/**
 * This class represents a queued file transfer.
 *
 * @author Jernej Kos
 */
class TransferFile : public Transfer
{
Q_OBJECT
friend class Manager;
public:
    /**
     * Class constructor.
     *
     * @param parent The parent object
     */
    TransferFile(QObject *parent);
    
    /**
     * Wakes this transfer up after the action for the file exists situation
     * has been decided. The event is simply relayed to the underlying socket.
     *
     * @param event Event instance or 0 if nothing should be delivered
     */
    void wakeup(KFTPEngine::FileExistsWakeupEvent *event);
    
    /**
     * @overload
     * Reimplemented from KFTPQueue::QueueObject.
     */
    void execute();
    
    /**
     * @overload
     * Reimplemented from KFTPQueue::Transfer.
     */
    void abort();
    
    /**
     * @overload
     * Reimplemented from KFTPQueue::Transfer.
     *
     * @param source The source session
     * @param destination The destination session
     * @return True if the sessions are ready for immediate use
     */
    bool assignSessions(KFTPSession::Session *source = 0, KFTPSession::Session *destination = 0);
private:
    /* Update timers */
    QTimer *m_updateTimer;
    QTimer *m_dfTimer;

    /* FXP */
    QTime m_elapsedTime;
    
    /**
     * @overload
     * Reimplemented from KFTPQueue::Transfer.
     */
    void resetTransfer();
private slots:
    void slotTimerUpdate();
    void slotTimerDiskFree();
    
    void slotDiskFree(const QString &mountPoint, unsigned long kBSize, unsigned long kBUsed, unsigned long kBAvail);
    
    void slotEngineEvent(KFTPEngine::Event *event);
    void slotSessionAborting();
    
    void slotConnectionLost(KFTPSession::Connection *connection);
};

}

#endif
