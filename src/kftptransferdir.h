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

#ifndef KFTPQUEUEKFTPTRANSFERDIR_H
#define KFTPQUEUEKFTPTRANSFERDIR_H

#include "kftptransfer.h"

class DirectoryScanner;

namespace KFTPQueue {

/**
 * This class represents a queued directory transfer. It can have child transfers that
 * will be executed one by one, when this transfer is executed.
 *
 * @author Jernej Kos
 */
class TransferDir : public Transfer
{
Q_OBJECT
public:
    /**
     * Possible transfer execution modes (= actions that can be executed when
     * the execute() method gets called).
     *
     * The following modes are supported:
     *   Default - just executes the transfer
     *   Ignore - does nothing but releases all connections
     *   ScanOnly - starts a directory scan and switches to Ignore
     *   ScanWithExecute - starts a directory scan and switches to Default
     */
    enum ExecutionMode {
      Default,
      Ignore,
      ScanOnly,
      ScanWithExecute
    };
    
    /**
     * Class constructor.
     *
     * @param parent The parent object
     */
    TransferDir(QObject *parent);
    
    /**
     * @overload
     * Reimplemented from KFTPQueue::Transfer.
     */
    void execute();
    
    /**
     * @overload
     * Reimplemented from KFTPQueue::Transfer.
     */
    void abort();
    
    /**
     * Initiates a directory scan. This method will do nothing if there are
     * existing children or the scan has already been initiated.
     */
    void scan();
private:
    bool m_scanned;
    QueueGroup *m_group;
    DirectoryScanner *m_srcScanner;
    ExecutionMode m_executionMode;
private slots:
    void slotGroupDone();
    void slotGroupInterrupted();
    
    void slotDirScanDone();
};

}

#endif
