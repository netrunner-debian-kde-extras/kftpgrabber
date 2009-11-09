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

#ifndef KFTPENGINEFTPSOCKET_H
#define KFTPENGINEFTPSOCKET_H

#include <QSslSocket>
#include <QSslError>
#include <QTcpServer>
#include <QTimer>

#include <qpointer.h>
#include <qfile.h>

#include "speedlimiter.h"
#include "socket.h"

namespace KFTPEngine {

class FtpDirectoryParser;

/**
 * A simple wrapper that returns QSslSocket objects instead of
 * standard QTcpSockets.
 *
 * @author Jernej Kos
 */
class SslServer : public QTcpServer {
public:
    /**
     * Class constructor.
     */
    SslServer();
    
    /**
     * @overload
     * Reimplemented from QTcpServer.
     */
    QSslSocket *nextPendingConnection();
protected:
    /**
     * @overload
     * Reimplemented from QTcpServer.
     */
    void incomingConnection(int socketDescriptor);
private:
    QSslSocket *m_socket;
};


/**
 * @author Jernej Kos <kostko@jweb-network.net>
 */
class FtpSocket : public QSslSocket, public Socket, public SpeedLimiterItem
{
Q_OBJECT
friend class Commands::Base;
friend class FtpCommandConnect;
friend class FtpCommandNegotiateData;
friend class FtpCommandList;
public:
    FtpSocket(Thread *thread);
    ~FtpSocket();
    
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
    void protoRaw(const QString &raw);
    void protoSiteToSite(Socket *socket, const KUrl &source, const KUrl &destination);
    void protoKeepAlive();
    
    void changeWorkingDirectory(const QString &path, bool shouldCreate = false);
    
    int features() { return SF_FXP_TRANSFER | SF_RAW_COMMAND; }
    
    bool isConnected() { return m_login; }
    bool isEncrypted() { return isConnected() && getConfig<bool>("ssl", false); }
    
    bool isResponse(const QString &code);
    QString getResponse() { return m_response; }
    bool isMultiline() { return !m_multiLineCode.isEmpty(); }
    
    void sendCommand(const QString &command);
    void resetCommandClass(ResetCode code = Ok);
    
    void setupPassiveTransferSocket(const QString &host, int port);
    SocketAddress setupActiveTransferSocket();
    
    QFile *getTransferFile() { return &m_transferFile; }
    
    void checkTransferEnd();
    void checkTransferStart();
    void resetTransferStart() { m_transferStart = 0; }
protected:
    void parseLine(const QString &line);
    void variableBufferUpdate(int size);
    void closeDataTransferSocket();
    void initializeTransferSocket();
    void transferCompleted();
private:
    bool m_login;
    
    QString m_buffer;
    QString m_multiLineCode;
    QString m_response;
    
    QSslSocket *m_transferSocket;
    SslServer *m_serverSocket;
    FtpDirectoryParser *m_directoryParser;
    
    char m_controlBuffer[1024];
    
    QFile m_transferFile;
    char *m_transferBuffer;
    int m_transferBufferSize;
    int m_transferStart;
    int m_transferEnd;
    
    QTimer *m_keepaliveTimer;
protected slots:
    /**
     * This method checks for timeouts and acts if needed.
     */
    void timerUpdate();
    
    void slotDisconnected();
    void slotConnected();
    void slotControlTryRead();
    void slotError();
    void slotSslErrors(const QList<QSslError> &errors);
    void slotSslNegotiated();
    
    void slotDataAccept();
    void slotDataConnected();
    void slotDataSslNegotiated();
    void slotDataDisconnected();
    void slotDataError(QAbstractSocket::SocketError error);
    void slotDataTryRead();
    void slotDataTryWrite();
};

}

Q_DECLARE_METATYPE(QSslError)

#endif
