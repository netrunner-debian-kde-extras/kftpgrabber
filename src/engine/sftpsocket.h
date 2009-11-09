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

#ifndef KFTPENGINESFTPSOCKET_H
#define KFTPENGINESFTPSOCKET_H

#include <libssh2.h>
#include <libssh2_publickey.h>
#include <libssh2_sftp.h>

#include <QTcpSocket>
#include <QFile>

#include "socket.h"
#include "speedlimiter.h"

namespace KFTPEngine {

/**
 * @author Jernej Kos <kostko@jweb-network.net>
 */
class SftpSocket : public QTcpSocket, public Socket, public SpeedLimiterItem {
Q_OBJECT
friend class SftpCommandConnect;
friend class SftpCommandList;
friend class SftpCommandGet;
friend class SftpCommandPut;
public:
    SftpSocket(Thread *thread);
    ~SftpSocket();
    
    void protoConnect(const KUrl &url);
    void protoDisconnect();
    void protoAbort();
    void protoGet(const KUrl &source, const KUrl &destination);
    void protoPut(const KUrl &source, const KUrl &destination);
    void protoRemove(const KUrl &path);
    void protoRename(const KUrl &source, const KUrl &destination);
    void protoChmodSingle(const KUrl &path, int mode);
    void protoMkdir(const KUrl &path);
    void protoList(const KUrl &path);
    
    int features() { return 0; }
    
    bool isConnected() { return m_login; }
    bool isEncrypted() { return true; }
    
    LIBSSH2_SESSION *sshSession() { return m_sshSession; }
    LIBSSH2_SFTP *sftpSession() { return m_sftpSession; }
    
    QFile *getTransferFile() { return &m_transferFile; }
protected:
    QString posixToString(int permissions);
    int intToPosix(int permissions);
    void variableBufferUpdate(int size);
private:
    LIBSSH2_SESSION *m_sshSession;
    LIBSSH2_SFTP *m_sftpSession;
    
    bool m_login;
    
    LIBSSH2_SFTP_HANDLE *m_transferHandle;
    QFile m_transferFile;
    char *m_transferBuffer;
    int m_transferBufferSize;
private slots:
    void slotDisconnected();
    void slotConnected();
    void slotError();
    
    void slotDataTryRead();
    void slotDataTryWrite();
};

}

#endif
