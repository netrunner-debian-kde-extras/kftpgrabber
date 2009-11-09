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

#include "ftpsocket.h"
#include "thread.h"
#include "ftpdirectoryparser.h"
#include "cache.h"
#include "speedlimiter.h"
#include "otpgenerator.h"

#include "misc/config.h"

#include <qdir.h>

#include <QByteArray>
#include <QSslCipher>
#include <QSslKey>
#include <QHostInfo>

#include <KLocale>
#include <KStandardDirs>
#include <ksocketfactory.h>

#include <utime.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>

Q_DECLARE_METATYPE(QSslCertificate)
Q_DECLARE_METATYPE(QSslKey)

namespace KFTPEngine {

SslServer::SslServer()
  : QTcpServer()
{
}

QSslSocket *SslServer::nextPendingConnection()
{
  return m_socket;
}

void SslServer::incomingConnection(int socketDescriptor)
{
  m_socket = new QSslSocket();
  m_socket->setSocketDescriptor(socketDescriptor);
}

FtpSocket::FtpSocket(Thread *thread)
 : QSslSocket(),
   Socket(thread, "ftp"),
   SpeedLimiterItem(),
   m_login(false),
   m_transferSocket(0),
   m_serverSocket(0),
   m_directoryParser(0)
{
  m_keepaliveTimer = new QTimer(this);
  connect(m_keepaliveTimer, SIGNAL(timeout()), this, SLOT(timerUpdate()));
  m_keepaliveTimer->start(1000);
  
  // Control socket signals
  connect(this, SIGNAL(readyRead()), this, SLOT(slotControlTryRead()));
  connect(this, SIGNAL(connected()), this, SLOT(slotConnected()));
  connect(this, SIGNAL(disconnected()), this, SLOT(slotDisconnected()));
  connect(this, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(slotError()));
  connect(this, SIGNAL(sslErrors(const QList<QSslError>&)), this, SLOT(slotSslErrors(const QList<QSslError>&)));
  connect(this, SIGNAL(encrypted()), this, SLOT(slotSslNegotiated()));
}

FtpSocket::~FtpSocket()
{
  protoDisconnect();
}

void FtpSocket::timerUpdate()
{
  timeoutCheck();
  keepaliveCheck();
}

void FtpSocket::slotDisconnected()
{
  protoDisconnect();
}

void FtpSocket::slotControlTryRead()
{
  QString tmpStr;
  qint64 size = read(m_controlBuffer, sizeof(m_controlBuffer) - 1);
  
  if (size == 0)
    return;
  
  for (int i = 0; i < size; i++)
    if (m_controlBuffer[i] == 0)
      m_controlBuffer[i] = '!';
  
  memset(m_controlBuffer + size, 0, sizeof(m_controlBuffer) - size);
  m_buffer.append(m_controlBuffer);
  
  // Parse any lines we might have
  int pos;
  while ((pos = m_buffer.indexOf('\n')) > -1) {
    QString line = m_buffer.mid(0, pos);
    line = m_remoteEncoding->decode(line.toAscii());
    parseLine(line);
    
    // Remove what we just parsed
    m_buffer.remove(0, pos + 1);
  }

  if (bytesAvailable())
    slotControlTryRead();
}

void FtpSocket::parseLine(const QString &line)
{
  // Is this the end of multiline response ?
  if (!m_multiLineCode.isEmpty() && line.left(4) == m_multiLineCode) {
    m_multiLineCode = "";
    emitEvent(Event::EventResponse, line);
  } else if (line.length() >= 4 && line[3] == '-' && m_multiLineCode.isEmpty()) {
    m_multiLineCode = line.left(3) + " ";
    emitEvent(Event::EventMultiline, line);
  } else if (!m_multiLineCode.isEmpty()) {
    emitEvent(Event::EventMultiline, line);
  } else {
    // Normal response
    emitEvent(Event::EventResponse, line);
  }
  
  timeoutWait(false);
  
  // Parse our response
  m_response = line;
  nextCommand();
}

bool FtpSocket::isResponse(const QString &code)
{
  QString ref;
  
  if (isMultiline())
    ref = m_multiLineCode;
  else
    ref = m_response;
    
  return ref.left(code.length()) == code;
}

void FtpSocket::sendCommand(const QString &command)
{
  emitEvent(Event::EventCommand, command);
  QByteArray buffer = m_remoteEncoding->encode(command) + "\r\n";
  
  write(buffer.data(), buffer.length());  
  timeoutWait(true);
}

void FtpSocket::resetCommandClass(ResetCode code)
{
  timeoutWait(false);
  
  if (m_transferSocket && code != Ok) {
    // Invalidate the socket
    closeDataTransferSocket();
    
    // Close the file that failed transfer
    if (getTransferFile()->isOpen()) {
      getTransferFile()->close();
      
      if (getCurrentCommand() == Commands::CmdGet && getTransferFile()->size() == 0)
        getTransferFile()->remove();
    }
  }
  
  if (m_serverSocket && code != Ok) {
    m_serverSocket->deleteLater();
    m_serverSocket = 0;
  }
  
  Socket::resetCommandClass(code);
}

// *******************************************************************************************
// ***************************************** CONNECT *****************************************
// *******************************************************************************************

class FtpCommandConnect : public Commands::Base {
public:
    enum State {
      None,
      SentAuthTls,
      WaitEncryption,
      SentUser,
      SentPass,
      SentPbsz,
      SentProt,
      DoingSyst,
      DoingFeat,
      SentPwd
    };
    
    ENGINE_STANDARD_COMMAND_CONSTRUCTOR(FtpCommandConnect, FtpSocket, CmdNone)
    
    void process()
    {
      switch (currentState) {
        case None: {
          if (!socket()->isMultiline()) {
            if (socket()->isResponse("2")) {
              // Negotiate a SSL connection if configured
              if (socket()->getConfig<bool>("ssl.use_tls")) {
                currentState = SentAuthTls;
                socket()->sendCommand("AUTH TLS");
              } else {
                // Send username
                currentState = SentUser;
                socket()->sendCommand("USER " + socket()->getCurrentUrl().user());
              }
            } else {
              socket()->emitEvent(Event::EventMessage, i18n("Connection has failed."));
              
              socket()->protoAbort();
              socket()->emitError(ConnectFailed, i18n("Server said: %1", socket()->getResponse()));
            }
          }
          break;
        }
        case SentAuthTls: {
          if (socket()->isResponse("2")) {
            socket()->startClientEncryption();
            currentState = WaitEncryption;
          } else {
            socket()->emitEvent(Event::EventMessage, i18n("SSL negotiation request failed. Login aborted."));
            socket()->resetCommandClass(Failed);
            
            socket()->protoAbort();
          }
          break;
        }
        case WaitEncryption: {
          // Encryption has been successfully negotiated, send the username
          currentState = SentUser;
          socket()->sendCommand("USER " + socket()->getCurrentUrl().user());
          break;
        }
        case SentUser: {
          if (socket()->isResponse("331")) {
            // Send password
            if (socket()->isResponse("331 Response to otp-")) {
              // OTP: 331 Response to otp-md5 41 or4828 ext required for foo.
              QString tmp = socket()->getResponse();
              tmp = tmp.section(' ', 3, 5);
              
              OtpGenerator otp(tmp, socket()->getCurrentUrl().pass());
              currentState = SentPass;
              socket()->sendCommand("PASS " + otp.response());
            } else {
              socket()->sendCommand("PASS " + socket()->getCurrentUrl().pass());
              currentState = SentPass;
            }
          } else if (socket()->isResponse("230")) {
            // Some servers imediately send the 230 response for anonymous accounts
            if (!socket()->isMultiline()) {
              if (socket()->getConfig<bool>("ssl")) {
                currentState = SentPbsz;
                socket()->sendCommand("PBSZ 0");
              } else {
                // Do SYST
                socket()->sendCommand("SYST");
                currentState = DoingSyst;
              }
            }
          } else {
            socket()->emitEvent(Event::EventMessage, i18n("Login has failed."));
            
            socket()->protoAbort();
            socket()->emitError(LoginFailed, i18n("The specified login credentials were rejected by the server."));
          }
          break;
        }
        case SentPass: {
          if (socket()->isResponse("230")) {
            if (!socket()->isMultiline()) {
              if (socket()->getConfig<bool>("ssl")) {
                currentState = SentPbsz;
                socket()->sendCommand("PBSZ 0");
              } else {
                // Do SYST
                socket()->sendCommand("SYST");
                currentState = DoingSyst;
              }
            }
          } else {
            socket()->emitEvent(Event::EventMessage, i18n("Login has failed."));
            
            socket()->protoAbort();
            socket()->emitError(LoginFailed, i18n("The specified login credentials were rejected by the server."));
          }
          break;
        }
        case SentPbsz: {
          currentState = SentProt;
          QString prot = "PROT ";
          
          if (socket()->getConfig<int>("ssl.prot_mode") == 0) 
            prot.append('P');
          else
            prot.append('C');

          socket()->sendCommand(prot);
          break;
        }
        case SentProt: {
          if (socket()->isResponse("5")) {
            // Fallback to unencrypted data channel
            socket()->setConfig("ssl.prot_mode", 2);
          }
          
          currentState = DoingSyst;
          socket()->sendCommand("SYST");
          break;
        }
        case DoingSyst: {
          socket()->sendCommand("FEAT");
          currentState = DoingFeat;
          break;
        }
        case DoingFeat: {
          if (socket()->isMultiline()) {
            parseFeat();
          } else {
            socket()->sendCommand("PWD");
            currentState = SentPwd;
          }
          break;
        }
        case SentPwd: {
          // Parse the current working directory
          if (socket()->isResponse("2")) {
            // 257 "/home/default/path"
            QString tmp = socket()->getResponse();
            int first = tmp.indexOf('"') + 1;
            tmp = tmp.mid(first, tmp.lastIndexOf('"') - first);
            
            socket()->setDefaultDirectory(tmp);
            socket()->setCurrentDirectory(tmp);
          }
          
          // Enable transmission of keepalive events
          socket()->keepaliveStart();
          
          currentState = None;
          socket()->emitEvent(Event::EventMessage, i18n("Connected."));
          socket()->emitEvent(Event::EventConnect);
          socket()->m_login = true;
          socket()->resetCommandClass();
          break;
        }
      }
    }
    
    void parseFeat()
    {
      QString feat = socket()->getResponse().trimmed().toUpper();
      
      if (feat.left(3).toInt() > 0 && feat[3] == '-')
        feat.remove(0, 4);
      
      if (feat.left(4) == "MDTM") {
        // Server has MDTM (MoDification TiMe) support
        socket()->setConfig("feat.mdtm", true);
      } else if (feat.left(4) == "PRET") {
        // Server is a distributed ftp server and requires PRET for transfers
        socket()->setConfig("feat.pret", true);
      } else if (feat.left(4) == "MLSD") {
        // Server supports machine-friendly directory listings
        socket()->setConfig("feat.mlsd", true);
      } else if (feat.left(4) == "REST") {
        // Server supports resume operations
        socket()->setConfig("feat.rest", true);
      } else if (feat.left(4) == "SSCN") {
        // Server supports SSCN for secure site-to-site transfers
        socket()->setConfig("feat.sscn", true);
        socket()->setConfig("feat.cpsv", false);
      } else if (feat.left(4) == "CPSV" && !socket()->getConfig<bool>("feat.sscn")) {
        // Server supports CPSV for secure site-to-site transfers
        socket()->setConfig("feat.cpsv", true);
      }
    }
};

void FtpSocket::protoConnect(const KUrl &url)
{
  emitEvent(Event::EventState, i18n("Connecting..."));
  emitEvent(Event::EventMessage, i18n("Connecting to %1:%2...",url.host(),url.port()));
  
  if (!getConfig("encoding").isEmpty())
    changeEncoding(getConfig("encoding"));
  
  // Start the connect procedure
  setCurrentUrl(url);
  setProxy(KSocketFactory::proxyForConnection(url.protocol(), url.host()));
  connectToHost(url.host(), url.port());
}

void FtpSocket::slotConnected()
{
  timeoutWait(true);
  
  // Set acceptable SSL protocols and set certificate store
  QSslSocket::setProtocol(QSsl::AnyProtocol);
  QSslSocket::setCaCertificates(KFTPCore::Config::self()->certificateStore()->trustedCertificates());
  
  // Configure peer certificate and private key (if set)
  if (!getConfig<QSslCertificate>("ssl.certificate").isNull()) {
    QSslSocket::setLocalCertificate(getConfig<QSslCertificate>("ssl.certificate"));
    QSslSocket::setPrivateKey(getConfig<QSslKey>("ssl.private_key"));
  }

  emitEvent(Event::EventState, i18n("Logging in..."));
  emitEvent(Event::EventMessage, i18n("Connected with server, waiting for welcome message..."));
  setupCommandClass(FtpCommandConnect);
  
  if (getConfig<bool>("ssl.use_implicit"))
    startClientEncryption();
}

void FtpSocket::slotError()
{
  emitEvent(Event::EventMessage, i18n("Failed to connect (%1.)", errorString()));
  emitError(ConnectFailed, errorString());
  
  resetCommandClass(FailedSilently);
}

void FtpSocket::slotSslNegotiated()
{
  QSslCipher cipher = sessionCipher();
              
  emitEvent(Event::EventMessage, i18n("SSL negotiation successful. Connection is secured with %1 bit cipher %2.", cipher.usedBits(), cipher.name()));
  setConfig("ssl", true);
  
  // Proceed with the next command
  if (!getConfig<bool>("ssl.use_implicit"))
    nextCommandAsync();
}

void FtpSocket::slotSslErrors(const QList<QSslError> &errors)
{
  if (getConfig<bool>("ssl.ignore_errors")) {
    ignoreSslErrors();
    return;
  }
  
  // Request immediate action from the user
  QVariantList elist;
  foreach (QSslError error, errors) {
    elist << QVariant::fromValue(QSslError(error.error(), peerCertificate()));
  }
  
  PeerVerifyWakeupEvent *response = static_cast<PeerVerifyWakeupEvent*>(emitEventAndWait(Event::EventPeerVerify, elist));
  if (response && response->peerOk) {
    // Certificate was deemed acceptable, so ignore errors and continue
    ignoreSslErrors();
  } else {
    // Negotiation has failed, disconnect will ocurr
    emitEvent(Event::EventMessage, i18n("SSL negotiation failed. Connection aborted."));
    resetCommandClass(Failed);
    protoAbort();
  }
}

// *******************************************************************************************
// **************************************** DISCONNECT ***************************************
// *******************************************************************************************

void FtpSocket::protoDisconnect()
{
  Socket::protoDisconnect();
  
  // Terminate the connection
  m_login = false;
  blockSignals(true);
  disconnectFromHost();
  blockSignals(false);
}

void FtpSocket::protoAbort()
{
  Socket::protoAbort();
  
  if (getCurrentCommand() != Commands::CmdNone) {
    // Abort current command
    if (getCurrentCommand() == Commands::CmdConnect)
      protoDisconnect();
      
    if (m_cmdData)
      resetCommandClass(UserAbort);
    
    emitEvent(Event::EventMessage, i18n("Aborted."));
  }
}

// *******************************************************************************************
// ********************************* NEGOTIATE DATA CONNECTION *******************************
// *******************************************************************************************

class FtpCommandNegotiateData : public Commands::Base {
public:
    enum State {
      None,
      SentSscnOff,
      SentType,
      SentProt,
      SentPret,
      NegotiateActive,
      NegotiatePasv,
      NegotiateEpsv,
      HaveConnection,
      SentRest,
      SentDataCmd,
      WaitTransfer
    };
    
    ENGINE_STANDARD_COMMAND_CONSTRUCTOR(FtpCommandNegotiateData, FtpSocket, CmdNone)
    
    void process()
    {
      switch (currentState) {
        case None: {
          if (socket()->getConfig<bool>("sscn.activated")) {
            // First disable SSCN
            currentState = SentSscnOff;
            socket()->sendCommand("SSCN OFF");
            return;
          }
        }
        case SentSscnOff: {
          if (currentState == SentSscnOff)
            socket()->setConfig("sscn.activated", false);
          
          // Change type
          currentState = SentType;
          socket()->resetTransferStart();
          
          QString type = "TYPE ";
          type.append(socket()->getConfig<char>("params.data_type", 'I'));
          socket()->sendCommand(type);
          break;
        }
        case SentType: {
          if (socket()->getConfig<bool>("ssl") && socket()->getConfig<int>("ssl.prot_mode") == 1) {
            currentState = SentProt;
            
            if (socket()->getPreviousCommand() == Commands::CmdList)
              socket()->sendCommand("PROT P");
            else
              socket()->sendCommand("PROT C"); 
          } else if (socket()->getConfig<bool>("feat.pret")) {
            currentState = SentPret;
            socket()->sendCommand("PRET " + socket()->getConfig("params.data_command"));
          } else {
            negotiateDataConnection();
          }
          break;
        }
        case SentProt: {
          if (socket()->getConfig<bool>("feat.pret")) {
            currentState = SentPret;
            socket()->sendCommand("PRET " + socket()->getConfig("params.data_command"));
          } else {
            negotiateDataConnection();
          }
          break;
        }
        case SentPret: {
          // PRET failed because of filesystem problems, abort right away!
          if (socket()->isResponse("530")) {
            socket()->emitError(PermissionDenied);
            socket()->resetCommandClass(Failed);
            return;
          } else if (socket()->isResponse("550")) {
            socket()->emitError(FileNotFound);
            socket()->resetCommandClass(Failed);
            return;
          } else if (socket()->isResponse("5")) {
            // PRET is not supported, disable for future use
            socket()->setConfig("feat.pret", false);
          }
          
          negotiateDataConnection();
          break;
        }
        case NegotiateActive: negotiateActive(); break;
        case NegotiateEpsv: negotiateEpsv(); break;
        case NegotiatePasv: negotiatePasv(); break;
        case HaveConnection: {
          // We have the connection
          if (socket()->getConfig<bool>("params.data_rest_do")) {
            currentState = SentRest;
            socket()->sendCommand("REST " + QString::number(socket()->getConfig<filesize_t>("params.data_rest")));
          } else {
            currentState = SentDataCmd;
            socket()->sendCommand(socket()->getConfig("params.data_command"));
          }
          break;
        }
        case SentRest: {
          if (!socket()->isResponse("2") && !socket()->isResponse("3")) {
            socket()->setConfig("feat.rest", false);
            socket()->getTransferFile()->close();
            
            bool ok;
            
            if (socket()->getPreviousCommand() == Commands::CmdGet)
              ok = socket()->getTransferFile()->open(QIODevice::WriteOnly | QIODevice::Truncate);
            else
              ok = socket()->getTransferFile()->open(QIODevice::ReadOnly);
            
            // Check if there was a problem opening the file
            if (!ok) {
              socket()->emitError(FileOpenFailed);
              socket()->resetCommandClass(Failed);
              return;
            }
          }
          
          // We have sent REST, now send the data command
          currentState = SentDataCmd;
          socket()->sendCommand(socket()->getConfig("params.data_command"));
          break;
        }
        case SentDataCmd: {
          if (!socket()->isResponse("1")) {
            // Some problems while executing the data command
            socket()->resetCommandClass(Failed);
            return;
          }
          
          if (!socket()->isMultiline()) {
            socket()->checkTransferStart();
            currentState = WaitTransfer;
          }
          break;
        }
        case WaitTransfer: {
          if (!socket()->isResponse("2")) {
            // Transfer has failed
            socket()->resetCommandClass(Failed);
            return;
          }
          
          if (!socket()->isMultiline()) {
            // Transfer has been completed
            socket()->checkTransferEnd();
          }
          break;
        }
      }
    }
    
    void negotiateDataConnection()
    {
      if (socket()->getConfig<bool>("feat.epsv")) {
        negotiateEpsv();
      } else if (socket()->getConfig<bool>("feat.pasv")) {
        negotiatePasv();
      } else {
        negotiateActive();
      }
    }
    
    void negotiateEpsv()
    {
      if (currentState == NegotiateEpsv) {
        if (!socket()->isResponse("2")) {
          // Negotiation failed
          socket()->setConfig("feat.epsv", false);
          
          // Try the next thing
          negotiateDataConnection();
          return;
        }
        
        // 229 Entering Extended Passive Mode (|||55016|)
        QString response = socket()->getResponse();
        int leftPos = response.lastIndexOf("(|||");
        int rightPos = response.lastIndexOf("|)");
        int port = response.mid(leftPos + 4, rightPos - leftPos - 4).toInt();
      
        if (!port) {
          // Unable to parse, try the next thing
          socket()->setConfig("feat.epsv", false);
          negotiateDataConnection();
          return;
        }
        
        // We have the address, let's setup the transfer socket and then
        // we are done.
        currentState = HaveConnection;
        socket()->setupPassiveTransferSocket(QString::null, port);
      } else {
        // Just send the EPSV command
        currentState = NegotiateEpsv;
        socket()->sendCommand("EPSV");
      }
    }
    
    void negotiatePasv()
    {
      if (currentState == NegotiatePasv) {
        if (!socket()->isResponse("2")) {
          // Negotiation failed
          socket()->setConfig("feat.pasv", false);
          
          // Try the next thing
          negotiateDataConnection();
          return;
        }
        
        // Ok PASV command successfull - let's parse the result
        int ip[6];
        const char *begin = strchr(socket()->getResponse().toAscii(), '(');
      
        // Some stinky servers don't respect RFC and do it on their own
        if (!begin)
          begin = strchr(socket()->getResponse().toAscii(), '=');
      
        if (!begin || (sscanf(begin, "(%d,%d,%d,%d,%d,%d)",&ip[0], &ip[1], &ip[2], &ip[3], &ip[4], &ip[5]) != 6 &&
                       sscanf(begin, "=%d,%d,%d,%d,%d,%d",&ip[0], &ip[1], &ip[2], &ip[3], &ip[4], &ip[5]) != 6)) {
          // Unable to parse, try the next thing
          socket()->setConfig("feat.pasv", false);
          negotiateDataConnection();
          return;
        }
      
        // Convert to string
        QString host;
        int port;
      
        host.sprintf("%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
        port = ip[4] << 8 | ip[5];
        
        // If the reported IP address is from a private IP range, this might be because the
        // remote server is not properly configured. So we just use the server's real IP instead
        // of the one we got (if the host is really local, then this should work as well).
        if (!socket()->getConfig<bool>("feat.pret")) {
          if (host.startsWith("192.168.") || host.startsWith("10.") || host.startsWith("172.16."))
            host = socket()->peerAddress().toString();
        }
        
        // We have the address, let's setup the transfer socket and then
        // we are done.
        currentState = HaveConnection;
        socket()->setupPassiveTransferSocket(host, port);
      } else {
        // Just send the PASV command
        currentState = NegotiatePasv;
        socket()->sendCommand("PASV");
      }
    }
    
    void negotiateActive()
    {
      if (currentState == NegotiateActive) {
        if (!socket()->isResponse("2")) {
          if (socket()->getConfig<bool>("feat.eprt")) {
            socket()->setConfig("feat.eprt", false);
          } else {
            // Negotiation failed, reset since active is the last fallback
            socket()->resetCommandClass(Failed);
            return;
          }
        } else {
          currentState = HaveConnection;
          socket()->nextCommandAsync();
          return;
        }
      }
      
      // Setup the socket and set the apropriate port command
      currentState = NegotiateActive;
      
      SocketAddress address = socket()->setupActiveTransferSocket();
      if (!address.ip.isNull()) {
        if (socket()->getConfig<bool>("feat.eprt")) {
          int ianaFamily = address.ip.protocol() == QSslSocket::IPv4Protocol ? 1 : 2;
          
          socket()->sendCommand(QString("EPRT |%1|%2|%3").arg(ianaFamily).arg(address.ip.toString()).arg(address.port));
        } else if (address.ip.protocol() == QSslSocket::IPv4Protocol) {
          QString format = address.ip.toString().replace(".", ",");
          
          format.append(",");
          format.append(QString::number((unsigned char) (address.port & 0xff00) >> 8));
          format.append(",");
          format.append(QString::number((unsigned char) (address.port & 0xff)));
          
          socket()->sendCommand("PORT " + format);
        } else {
          socket()->emitEvent(Event::EventMessage, i18n("Incompatible address family for PORT, but EPRT not supported, aborting."));
          socket()->resetCommandClass(Failed);
        }
      }
    }
};

void FtpSocket::initializeTransferSocket()
{
  connect(m_transferSocket, SIGNAL(connected()), this, SLOT(slotDataConnected()));
  connect(m_transferSocket, SIGNAL(readyRead()), this, SLOT(slotDataTryRead()));
  connect(m_transferSocket, SIGNAL(disconnected()), this, SLOT(slotDataDisconnected()));
  connect(m_transferSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(slotDataTryWrite()));
  connect(m_transferSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(slotDataError(QAbstractSocket::SocketError)));
  
  // Set SSL parameters
  m_transferSocket->setProtocol(QSslSocket::protocol());
  
  m_transferEnd = 0;
  m_transferBytes = 0;
  m_transferBufferSize = 4096;
  m_transferBuffer = (char*) malloc(m_transferBufferSize);
  
  m_speedLastTime = time(0);
  m_speedLastBytes = 0;
  
  // Setup the speed limiter
  switch (getPreviousCommand()) {
    case Commands::CmdGet: SpeedLimiter::self()->append(this, SpeedLimiter::Download); break;
    case Commands::CmdPut: SpeedLimiter::self()->append(this, SpeedLimiter::Upload); break;
    default: break;
  }
}

void FtpSocket::setupPassiveTransferSocket(const QString &host, int port)
{
  // Use the host from control connection if empty
  QString realHost = host;
  if (host.isEmpty() || getConfig<bool>("pasv.use_site_ip"))
    realHost = peerAddress().toString();
    
  // Let's connect
  emitEvent(Event::EventMessage, i18n("Establishing data connection with %1:%2...",realHost,port));
    
  if (!m_transferSocket)
    m_transferSocket = new QSslSocket();
    
  initializeTransferSocket();
  m_transferSocket->connectToHost(realHost, port);
}

SocketAddress FtpSocket::setupActiveTransferSocket()
{
  if (!m_serverSocket) {
    m_serverSocket = new SslServer();
    m_serverSocket->setProxy(KSocketFactory::proxyForListening("ftp"));
  }
  
  connect(m_serverSocket, SIGNAL(newConnection()), this, SLOT(slotDataAccept()));
  
  if (KFTPCore::Config::activeForcePort()) {
    // Bind only to ports in a specified portrange
    bool found = false;
    unsigned int max = KFTPCore::Config::activeMaxPort();
    unsigned int min = KFTPCore::Config::activeMinPort();

    for (unsigned int port = min + rand() % (max - min + 1); port <= max; port++) {
      found = m_serverSocket->listen(QHostAddress::Any, port);
      
      if (found)
        break;
        
      m_serverSocket->close();
    }
    
    if (!found) {
      emitEvent(Event::EventMessage, i18n("Unable to establish a listening socket."));
      resetCommandClass(Failed);
      return SocketAddress();
    }
  } else {
    if (!m_serverSocket->listen()) {
      emitEvent(Event::EventMessage, i18n("Unable to establish a listening socket."));
      resetCommandClass(Failed);
      return SocketAddress();
    }
  }
  
  QHostAddress serverAddr = m_serverSocket->serverAddress();
  QHostAddress controlAddr = localAddress();
  SocketAddress request;
  
  if (KFTPCore::Config::portForceIp() && !getConfig<bool>("active.no_force_ip")) {
    QString remoteIp = peerAddress().toString();
    
    if (KFTPCore::Config::ignoreExternalIpForLan() &&
        (remoteIp.startsWith("192.168.") || remoteIp.startsWith("10.") || remoteIp.startsWith("172.16."))) {
      request.ip = controlAddr;
    } else {
      // Force a specified IP/hostname to be used in PORT
      QHostInfo resolver = QHostInfo::fromName(KFTPCore::Config::portIp());
  
      if (!resolver.addresses().isEmpty()) {
        request.ip = resolver.addresses().first();
      } else {
        request.ip = controlAddr;
      }
    }
  } else {
    request.ip = controlAddr;
  }
  
  // Set the proper port
  request.port = m_serverSocket->serverPort();
  
  emitEvent(Event::EventMessage, i18n("Waiting for data connection on port %1...",request.port));
  
  return request;
}

void FtpSocket::slotDataAccept()
{
  m_transferSocket = m_serverSocket->nextPendingConnection();
  initializeTransferSocket();
  
  // Socket has been accepted so the server is not needed anymore
  m_serverSocket->deleteLater();
  m_serverSocket = 0;
  
  emitEvent(Event::EventMessage, i18n("Data connection established."));
  checkTransferStart();
}

void FtpSocket::closeDataTransferSocket()
{
  // Free the buffer and invalidate the socket
  if (!m_transferBuffer)
    return;
  
  free(m_transferBuffer);
  m_transferBuffer = 0;
  
  m_transferSocket->close();
  m_transferSocket->deleteLater();
  m_transferSocket = 0;
  m_transferBytes = 0;
  
  SpeedLimiter::self()->remove(this);
}

void FtpSocket::transferCompleted()
{
  // Transfer has been completed, cleanup
  closeDataTransferSocket();
  checkTransferEnd();
}

void FtpSocket::checkTransferStart()
{
  if (++m_transferStart >= 2) {
    // Setup SSL data connection
    if (getConfig<bool>("ssl") && (getConfig<int>("ssl.prot_mode") == 0 ||
      (getConfig<int>("ssl.prot_mode") == 1 && getToplevelCommand() == Commands::CmdList))) {
      // Connect to notification events and proceed with SSL negotiation
      connect(m_transferSocket, SIGNAL(encrypted()), this, SLOT(slotDataSslNegotiated()));
      
      m_transferSocket->ignoreSslErrors();
      m_transferSocket->startClientEncryption();
      return;
    }
    
    if (getToplevelCommand() == Commands::CmdPut)
      slotDataTryWrite();
  }
}

void FtpSocket::slotDataSslNegotiated()
{
  disconnect(m_transferSocket, SIGNAL(encrypted()), this, SLOT(slotDataSslNegotiated()));
  emitEvent(Event::EventMessage, i18n("Data channel secured with %1 bit SSL.", m_transferSocket->sessionCipher().usedBits()));
  
  if (getToplevelCommand() == Commands::CmdPut)
    slotDataTryWrite();
}

void FtpSocket::slotDataError(QAbstractSocket::SocketError error)
{
  if (error == QAbstractSocket::RemoteHostClosedError)
    return;
  
  emitEvent(Event::EventMessage, i18n("Failed to establish a data connection (%1.)", errorString()));
  resetCommandClass(Failed);
}

void FtpSocket::checkTransferEnd()
{
  if (++m_transferEnd >= 2) {
    emitEvent(Event::EventMessage, i18n("Transfer completed."));
    resetCommandClass();
  }
}

void FtpSocket::slotDataConnected()
{
  emitEvent(Event::EventMessage, i18n("Data connection established."));
  
  checkTransferStart();
  nextCommand();
}

void FtpSocket::slotDataDisconnected()
{
  emitEvent(Event::EventMessage, i18n("Data connection closed."));
  
  transferCompleted();
}

void FtpSocket::variableBufferUpdate(int size)
{
  if (size > m_transferBufferSize - 64) {
    if (m_transferBufferSize + 512 <= 32768) {
      m_transferBufferSize += 512;
      m_transferBuffer = (char*) realloc(m_transferBuffer, m_transferBufferSize);
    }
  } else if (size < m_transferBufferSize - 65) {
    if (m_transferBufferSize - 512 >= 4096) {
      m_transferBufferSize -= 512;
      m_transferBuffer = (char*) realloc(m_transferBuffer, m_transferBufferSize);
    }
  }
}

void FtpSocket::slotDataTryWrite()
{
  bool updateVariableBuffer = true;
  
  // Enforce speed limits
  if (allowedBytes() > -1) {
    m_transferBufferSize = allowedBytes();
    
    if (m_transferBufferSize > 32768)
      m_transferBufferSize = 32768;
    else if (m_transferBufferSize == 0)
      return;
      
    m_transferBuffer = (char*) realloc(m_transferBuffer, m_transferBufferSize);
    updateVariableBuffer = false;
  } else if (m_transferBufferSize == 0) {
    m_transferBufferSize = 4096;
    m_transferBuffer = (char*) realloc(m_transferBuffer, m_transferBufferSize);
  }
  
  if (!getTransferFile()->isOpen())
    return;
  
  // If there is nothing to upload, just close the connection right away
  if (getTransferFile()->size() == 0) {
    transferCompleted();
    return;
  }
  
  qint64 tmpOffset = getTransferFile()->pos();
  qint64 readSize = getTransferFile()->read(m_transferBuffer, m_transferBufferSize);
  qint64 size = m_transferSocket->write(m_transferBuffer, readSize);
  
  if (size < 0) {
    getTransferFile()->seek(tmpOffset);
    return;
  } else if (size < readSize) {
    getTransferFile()->seek(tmpOffset + size);
  }
    
  m_transferBytes += size;
  updateUsage(size);
  timeoutPing();
  
  if (getTransferFile()->atEnd()) {
    // We have reached the end of file, so we should terminate the connection
    transferCompleted();
    return;
  }
  
  if (updateVariableBuffer)
    variableBufferUpdate(size);
}

void FtpSocket::slotDataTryRead()
{
  bool updateVariableBuffer = true;
  
  // Enforce speed limits
  if (allowedBytes() > -1) {
    m_transferBufferSize = allowedBytes();
    
    if (m_transferBufferSize > 32768)
      m_transferBufferSize = 32768;
    else if (m_transferBufferSize == 0)
      return;
    
    m_transferBuffer = (char*) realloc(m_transferBuffer, m_transferBufferSize);
    updateVariableBuffer = false;
  } else if (m_transferBufferSize == 0) {
    m_transferBufferSize = 4096;
    m_transferBuffer = (char*) realloc(m_transferBuffer, m_transferBufferSize);
  }
  
  qint64 size = m_transferSocket->read(m_transferBuffer, m_transferBufferSize);
  
  if (size <= 0) {
    transferCompleted();
    return;
  }
  
  updateUsage(size);
  timeoutPing();

  switch (getPreviousCommand()) {
    case Commands::CmdList: {
      // Feed the data to the directory listing parser
      if (m_directoryParser)
        m_directoryParser->addData(m_transferBuffer, size);
      break;
    }
    case Commands::CmdGet: {
      // Write to file
      getTransferFile()->write(m_transferBuffer, size);
      m_transferBytes += size;
      break;
    }
    default: {
      qDebug("WARNING: slotDataReadActivity called for an invalid command (%d)!", getPreviousCommand());
      return;
    }
  }
  
  if (updateVariableBuffer)
    variableBufferUpdate(size);
}

// *******************************************************************************************
// ******************************************* LIST ******************************************
// *******************************************************************************************

class FtpCommandList : public Commands::Base {
public:
    enum State {
      None,
      SentCwd,
      SentStat,
      WaitList
    };
    
    ENGINE_STANDARD_COMMAND_CONSTRUCTOR(FtpCommandList, FtpSocket, CmdList)
    
    QString path;
    
    void process()
    {
      switch (currentState) {
        case None: {
          path = socket()->getConfig("params.list.path");
          
          if (socket()->isChained())
            socket()->m_lastDirectoryListing = DirectoryListing();
          
          // Change working directory
          currentState = SentCwd;
          socket()->changeWorkingDirectory(path);
          break;
        }
        case SentCwd: {
          if (!socket()->returnValue<bool>()) {
            // Change working directory has failed and we have to be silent (=error reporting is off)
            socket()->resetCommandClass();
            return;
          }
          
          // Check the directory listing cache
          DirectoryListing cached = Cache::self()->findCached(socket(), socket()->getCurrentDirectory());
          if (cached.isValid()) {
            socket()->emitEvent(Event::EventMessage, i18n("Using cached directory listing."));
            
            if (socket()->isChained()) {
              // We don't emit an event, because this list has been called from another
              // command. Just save the listing.
              socket()->m_lastDirectoryListing = cached;
            } else
              socket()->emitEvent(Event::EventDirectoryListing, cached);
              
            socket()->resetCommandClass();
            return;
          }
          
          socket()->m_directoryParser = new FtpDirectoryParser(socket());
          
          // Support for faster stat directory listings over the control connection
          if (socket()->getConfig<bool>("feat.stat")) {
            currentState = SentStat;
            socket()->sendCommand("STAT .");
            return;
          }
          
          // First we have to initialize the data connection, another class will
          // do this for us, so we just add it to the command chain
          socket()->setConfig("params.data_rest_do", 0);
          socket()->setConfig("params.data_type", 'A');
          
          if (socket()->getConfig<bool>("feat.mlsd"))
            socket()->setConfig("params.data_command", "MLSD");
          else
            socket()->setConfig("params.data_command", "LIST -a");
          
          currentState = WaitList;
          chainCommandClass(FtpCommandNegotiateData);
          break;
        }
        case SentStat: {
          if (!socket()->isResponse("2")) {
            // The server doesn't support STAT, disable it and fallback
            socket()->setConfig("feat.stat", false);
            
            socket()->setConfig("params.data_rest_do", 0);
            socket()->setConfig("params.data_type", 'A');
            
            if (socket()->getConfig<bool>("feat.mlsd"))
              socket()->setConfig("params.data_command", "MLSD");
            else
              socket()->setConfig("params.data_command", "LIST -a");
            
            currentState = WaitList;
            chainCommandClass(FtpCommandNegotiateData);
            return;
          } else if (socket()->isMultiline()) {
            // Some servers put the response code into the multiline reply
            QString response = socket()->getResponse();
            if (response.left(3) == "211")
              response = response.mid(4);
              
            socket()->m_directoryParser->addDataLine(response);
            return;
          }
          
          // If we are done, just go on and emit the listing
        }
        case WaitList: {
          // List has been received
          if (socket()->isChained()) {
            // We don't emit an event, because this list has been called from another
            // command. Just save the listing.
            socket()->m_lastDirectoryListing = socket()->m_directoryParser->getListing();
          } else {
            socket()->emitEvent(Event::EventDirectoryListing, socket()->m_directoryParser->getListing());
          }
          
          // Cache the directory listing
          Cache::self()->addDirectory(socket(), socket()->m_directoryParser->getListing());
          
          delete socket()->m_directoryParser;
          socket()->m_directoryParser = 0;

          socket()->resetCommandClass();
          break;
        }
      }
    }
};

void FtpSocket::protoList(const KUrl &path)
{
  emitEvent(Event::EventState, i18n("Fetching directory listing..."));
  emitEvent(Event::EventMessage, i18n("Fetching directory listing..."));
  
  // Set the directory that should be listed
  setConfig("params.list.path", path.path());
  
  activateCommandClass(FtpCommandList);
}

// *******************************************************************************************
// ******************************************* GET *******************************************
// *******************************************************************************************

class FtpCommandGet : public Commands::Base {
public:
    enum State {
      None,
      SentCwd,
      SentMdtm,
      StatDone,
      DestChecked,
      WaitTransfer
    };
    
    ENGINE_STANDARD_COMMAND_CONSTRUCTOR(FtpCommandGet, FtpSocket, CmdGet)
    
    KUrl sourceFile;
    KUrl destinationFile;
    time_t modificationTime;
    
    void process()
    {
      switch (currentState) {
        case None: {
          modificationTime = 0;
          sourceFile.setPath(socket()->getConfig("params.get.source"));
          destinationFile.setPath(socket()->getConfig("params.get.destination"));
          
          // Attempt to CWD to the parent directory
          currentState = SentCwd;
          socket()->changeWorkingDirectory(sourceFile.directory());
          break;
        }
        case SentCwd: {
          // Send MDTM
          if (socket()->getConfig<bool>("feat.mdtm")) {
            currentState = SentMdtm;
            socket()->sendCommand("MDTM " + sourceFile.path());
            break;
          } else {
            // Don't break so we will get on to checking for file existance
          }
        }
        case SentMdtm: {
          if (currentState == SentMdtm) {
            if (socket()->isResponse("550")) {
              // The file probably doesn't exist, just ignore it
            } else if (!socket()->isResponse("213")) {
              socket()->setConfig("feat.mdtm", false);
            } else {
              // Parse MDTM response
              struct tm dt = {0,0,0,0,0,0,0,0,0,0,0};
              QString tmp(socket()->getResponse());
        
              tmp.remove(0, 4);
              dt.tm_year = tmp.left(4).toInt() - 1900;
              dt.tm_mon = tmp.mid(4, 2).toInt() - 1;
              dt.tm_mday = tmp.mid(6, 2).toInt();
              dt.tm_hour = tmp.mid(8, 2).toInt();
              dt.tm_min = tmp.mid(10, 2).toInt();
              dt.tm_sec = tmp.mid(12, 2).toInt();
              modificationTime = mktime(&dt);
            }
          }

          // Check if the local file exists and stat the remote file if so
          if (QDir::root().exists(destinationFile.path())) {
            socket()->protoStat(sourceFile);
            currentState = StatDone;
            return;
          } else {
            KStandardDirs::makeDir(destinationFile.directory());
            
            // Don't break so we will get on to initiating the data connection
          }
        }
        case StatDone: {
          if (currentState == StatDone) {
            DirectoryListing list;
            list.addEntry(socket()->getStatResponse());
            
            currentState = DestChecked;
            socket()->emitEvent(Event::EventFileExists, list);
            return;
          }
        }
        case DestChecked: {
          socket()->setConfig("params.data_rest_do", 0);
          
          if (isWakeup()) {
            // We have been waken up because a decision has been made
            FileExistsWakeupEvent *event = static_cast<FileExistsWakeupEvent*>(m_wakeupEvent);
            
            if (!socket()->getConfig<bool>("feat.rest") && event->action == FileExistsWakeupEvent::Resume)
              event->action = FileExistsWakeupEvent::Overwrite;
            
            switch (event->action) {
              case FileExistsWakeupEvent::Rename: {
                // Change the destination filename, otherwise it is the same as overwrite
                destinationFile.setPath(event->newFileName);
              }
              case FileExistsWakeupEvent::Overwrite: {
                socket()->getTransferFile()->setFileName(destinationFile.path());
                socket()->getTransferFile()->open(QIODevice::WriteOnly | QIODevice::Truncate);
                
                if (socket()->getConfig<bool>("feat.rest")) {
                  socket()->setConfig("params.data_rest_do", true);
                  socket()->setConfig("params.data_rest", 0);
                }
                break;
              }
              case FileExistsWakeupEvent::Resume: {
                socket()->getTransferFile()->setFileName(destinationFile.path());
                socket()->getTransferFile()->open(QIODevice::WriteOnly | QIODevice::Append);
                
                // Signal resume
                socket()->emitEvent(Event::EventResumeOffset, socket()->getTransferFile()->size());
                
                socket()->setConfig("params.data_rest_do", true);
                socket()->setConfig("params.data_rest", (filesize_t) socket()->getTransferFile()->size());
                break;
              }
              case FileExistsWakeupEvent::Skip: {
                // Transfer should be aborted
                socket()->emitEvent(Event::EventTransferComplete);
                socket()->resetCommandClass();
                return;
              }
            }
          } else {
            // The file doesn't exist so we are free to overwrite
            socket()->getTransferFile()->setFileName(destinationFile.path());
            socket()->getTransferFile()->open(QIODevice::WriteOnly | QIODevice::Truncate);
          }
          
          // Check if there was a problem opening the file
          if (!socket()->getTransferFile()->isOpen()) {
            socket()->emitError(FileOpenFailed);
            socket()->resetCommandClass(Failed);
            return;
          }
          
          // First we have to initialize the data connection, another class will
          // do this for us, so we just add it to the command chain
          socket()->setConfig("params.data_type", KFTPCore::Config::self()->ftpMode(sourceFile.path()));
          socket()->setConfig("params.data_command", "RETR " + sourceFile.fileName());
          
          currentState = WaitTransfer;
          chainCommandClass(FtpCommandNegotiateData);
          break;
        }
        case WaitTransfer: {
          // Transfer has been completed
          socket()->getTransferFile()->close();
          
          if (modificationTime != 0) {
            // Use the modification time we got from MDTM
            utimbuf tmp;
            tmp.actime = time(0);
            tmp.modtime = modificationTime;
            utime(destinationFile.path().toAscii(), &tmp);
          }
          
          socket()->emitEvent(Event::EventTransferComplete);
          socket()->emitEvent(Event::EventReloadNeeded);
          socket()->resetCommandClass();
          break;
        }
      }
    }
};

void FtpSocket::protoGet(const KUrl &source, const KUrl &destination)
{
  emitEvent(Event::EventState, i18n("Transferring..."));
  emitEvent(Event::EventMessage, i18n("Downloading file '%1'...",source.fileName()));
  
  // Set the source and destination
  setConfig("params.get.source", source.path());
  setConfig("params.get.destination", destination.path());
  
  activateCommandClass(FtpCommandGet);
}

// *******************************************************************************************
// ******************************************* CWD *******************************************
// *******************************************************************************************

class FtpCommandCwd : public Commands::Base {
public:
    enum State {
      None,
      SentCwd,
      SentPwd,
      SentMkd,
      SentCwdEnd
    };
    
    ENGINE_STANDARD_COMMAND_CONSTRUCTOR(FtpCommandCwd, FtpSocket, CmdNone)
    
    QString targetDirectory;
    QString currentPathPart;
    QString cached;
    int currentPart;
    int numParts;
    bool shouldCreate;
    
    void process()
    {
      switch (currentState) {
        case None: {
          socket()->setReturnValue(true);
          targetDirectory = socket()->getConfig("params.cwd.path");
          
          // If we are already there, no need to CWD
          if (socket()->getCurrentDirectory() == targetDirectory) {
            socket()->resetCommandClass();
            return;
          }
          
          cached = Cache::self()->findCachedPath(socket(), targetDirectory);
          if (!cached.isEmpty()) {
            if (socket()->getCurrentDirectory() == cached) {
              // We are already there
              socket()->resetCommandClass();
              return;
            }
          }
          
          // First check the toplevel directory and if it exists we are done
          currentState = SentCwd;
          currentPart = 0;
          numParts = targetDirectory.count('/');
          shouldCreate = socket()->getConfig<bool>("params.cwd.create");
          
          socket()->sendCommand("CWD " + targetDirectory);
          break;
        }
        case SentCwd: {
          if (socket()->isMultiline())
            return;
          
          if (socket()->isResponse("250") && currentPart == 0) {
            if (!cached.isEmpty()) {
              socket()->setCurrentDirectory(cached);
              socket()->resetCommandClass();
            } else {
              // Directory exists, check where we are
              currentState = SentPwd;
              socket()->sendCommand("PWD");
            }
          } else {
            // Changing the working directory has failed
            if (shouldCreate) {
              currentPathPart = targetDirectory.section('/', 0, ++currentPart);
              currentState = SentMkd;
              socket()->sendCommand("MKD " + currentPathPart);
            } else if (socket()->errorReporting()) {
              socket()->emitError(socket()->getPreviousCommand() == Commands::CmdList ? ListFailed : FileNotFound);
              socket()->resetCommandClass(Failed);
            } else {
              socket()->setReturnValue(false);
              socket()->resetCommandClass();
            }
          }
          break;
        }
        case SentPwd: {
          // Parse the current working directory
          if (socket()->isResponse("2")) {
            QString tmp = socket()->getResponse();
            int first = tmp.indexOf('"') + 1;
            tmp = tmp.mid(first, tmp.lastIndexOf('"') - first);
            
            // Set the current directory and cache it
            socket()->setCurrentDirectory(tmp);
            Cache::self()->addPath(socket(), tmp);
            
            socket()->resetCommandClass();
          } else if (socket()->errorReporting()) {
            socket()->emitError(socket()->getPreviousCommand() == Commands::CmdList ? ListFailed : FileNotFound);
            socket()->resetCommandClass(Failed);
          } else {
            socket()->setReturnValue(false);
            socket()->resetCommandClass();
          }
          break;
        }
        case SentMkd: {
          // Invalidate parent cache
          if (socket()->isResponse("2")) {
            Cache::self()->invalidateEntry(socket(), KUrl(currentPathPart).directory());
          }
            
          if (currentPart == numParts) {
            // We are done, since all directories have been created
            currentState = SentCwdEnd;
            socket()->sendCommand("CWD " + targetDirectory);
          } else {
            currentPathPart = targetDirectory.section('/', 0, ++currentPart);
            currentState = SentMkd;
            socket()->sendCommand("MKD " + currentPathPart);
          }
          break;
        }
        case SentCwdEnd: {
          if (socket()->isMultiline())
            return;
          
          // See where we are and set current working directory
          currentState = SentPwd;
          socket()->sendCommand("PWD");
          break;
        }
      }
    }
};

void FtpSocket::changeWorkingDirectory(const QString &path, bool shouldCreate)
{
  // Set the path to cwd to
  setConfig("params.cwd.path", path);
  setConfig("params.cwd.create", shouldCreate);
  
  activateCommandClass(FtpCommandCwd);
}

// *******************************************************************************************
// ******************************************* PUT *******************************************
// *******************************************************************************************

class FtpCommandPut : public Commands::Base {
public:
    enum State {
      None,
      WaitCwd,
      SentSize,
      StatDone,
      DestChecked,
      WaitTransfer
    };
    
    ENGINE_STANDARD_COMMAND_CONSTRUCTOR(FtpCommandPut, FtpSocket, CmdPut)
    
    KUrl sourceFile;
    KUrl destinationFile;
    
    bool fetchedSize;
    filesize_t destinationSize;
    
    void cleanup()
    {
      // Unclean upload termination, be sure to erase the cached stat infos
      Cache::self()->invalidateEntry(socket(), destinationFile.directory());
    }
    
    void process()
    {
      switch (currentState) {
        case None: {
          sourceFile.setPath(socket()->getConfig("params.get.source"));
          destinationFile.setPath(socket()->getConfig("params.get.destination"));
          fetchedSize = false;
          
          // Check if the local file exists
          if (!QDir::root().exists(sourceFile.path())) {
            socket()->emitError(FileNotFound);
            socket()->resetCommandClass(Failed);
            return;
          }
          
          // Change to the current working directory, creating any directories that are
          // still missing
          currentState = WaitCwd;
          socket()->changeWorkingDirectory(destinationFile.directory(), true);
          break;
        }
        case WaitCwd: {
          // Check if the remote file exists
          if (socket()->getConfig<bool>("feat.size")) {
            currentState = SentSize;
            socket()->sendCommand("SIZE " + destinationFile.path());
          } else {
            // SIZE is not available, try stat directly
            currentState = StatDone;
            socket()->protoStat(destinationFile);
          }
          break;
        }
        case SentSize: {
          if (socket()->isResponse("213")) {
            destinationSize = socket()->getResponse().mid(4).toULongLong();
            fetchedSize = true;
            
            // File exists, we have to stat to get more data
            currentState = StatDone;
            socket()->protoStat(destinationFile);
          } else if (socket()->isResponse("500") || socket()->getResponse().contains("Operation not permitted")) {
            // Yes, some servers don't support the SIZE command :/
            socket()->setConfig("feat.size", false);
            
            currentState = StatDone;
            socket()->protoStat(destinationFile);
          } else {
            currentState = DestChecked;
            process();
          }
          break;
        }
        case StatDone: {
          if (!socket()->getStatResponse().filename().isEmpty()) {
            if (fetchedSize) {
              if (socket()->getStatResponse().size() != destinationSize) {
                // It would seem that the size has changed, cached data is invalid
                Cache::self()->invalidateEntry(socket(), destinationFile.directory());
                
                currentState = StatDone;
                socket()->protoStat(destinationFile);
                return;
              }
            }
            
            // Remote file exists, emit a request for action
            DirectoryListing list;
            list.addEntry(socket()->getStatResponse());
            
            currentState = DestChecked;
            socket()->emitEvent(Event::EventFileExists, list);
            return;
          }
          
          // Don't break here
        }
        case DestChecked: {
          socket()->setConfig("params.data_rest_do", 0);
          
          if (isWakeup()) {
            // We have been waken up because a decision has been made
            FileExistsWakeupEvent *event = static_cast<FileExistsWakeupEvent*>(m_wakeupEvent);
            
            if (!socket()->getConfig<bool>("feat.rest") && event->action == FileExistsWakeupEvent::Resume)
              event->action = FileExistsWakeupEvent::Overwrite;
            
            switch (event->action) {
              case FileExistsWakeupEvent::Rename: {
                // Change the destination filename, otherwise it is the same as overwrite
                destinationFile.setPath(event->newFileName);
              }
              case FileExistsWakeupEvent::Overwrite: {
                socket()->getTransferFile()->setFileName(sourceFile.path());
                socket()->getTransferFile()->open(QIODevice::ReadOnly);
                
                if (socket()->getConfig<bool>("feat.rest")) {
                  socket()->setConfig("params.data_rest_do", true);
                  socket()->setConfig("params.data_rest", 0);
                }
                break;
              }
              case FileExistsWakeupEvent::Resume: {
                socket()->getTransferFile()->setFileName(sourceFile.path());
                socket()->getTransferFile()->open(QIODevice::ReadOnly);
                socket()->getTransferFile()->seek(socket()->getStatResponse().size());
                
                // Signal resume
                socket()->emitEvent(Event::EventResumeOffset, socket()->getStatResponse().size());
                
                socket()->setConfig("params.data_rest_do", true);
                socket()->setConfig("params.data_rest", (filesize_t) socket()->getStatResponse().size());
                break;
              }
              case FileExistsWakeupEvent::Skip: {
                // Transfer should be aborted
                markClean();
                
                socket()->resetCommandClass(UserAbort);
                socket()->emitEvent(Event::EventTransferComplete);
                return;
              }
            }
          } else {
            // The file doesn't exist so we are free to overwrite
            socket()->getTransferFile()->setFileName(sourceFile.path());
            socket()->getTransferFile()->open(QIODevice::ReadOnly);
          }
          
          // Check if there was a problem opening the file
          if (!socket()->getTransferFile()->isOpen()) {
            socket()->emitError(FileOpenFailed);
            socket()->resetCommandClass(Failed);
            return;
          }
          
          // First we have to initialize the data connection, another class will
          // do this for us, so we just add it to the command chain
          socket()->setConfig("params.data_type", KFTPCore::Config::self()->ftpMode(destinationFile.path()));
          socket()->setConfig("params.data_command", "STOR " + destinationFile.fileName());
          
          currentState = WaitTransfer;
          chainCommandClass(FtpCommandNegotiateData);
          break;
        }
        case WaitTransfer: {
          // Transfer has been completed
          Cache::self()->updateDirectoryEntry(socket(), destinationFile, socket()->getTransferFile()->size());
          socket()->getTransferFile()->close();
          markClean();
          
          socket()->emitEvent(Event::EventTransferComplete);
          socket()->emitEvent(Event::EventReloadNeeded);
          socket()->resetCommandClass();
          break;
        }
      }
    }
};

void FtpSocket::protoPut(const KUrl &source, const KUrl &destination)
{
  emitEvent(Event::EventState, i18n("Transferring..."));
  emitEvent(Event::EventMessage, i18n("Uploading file '%1'...",source.fileName()));
  
  // Set the source and destination
  setConfig("params.get.source", source.path());
  setConfig("params.get.destination", destination.path());
  
  activateCommandClass(FtpCommandPut);
}

// *******************************************************************************************
// **************************************** REMOVE *******************************************
// *******************************************************************************************

class FtpCommandRemove : public Commands::Base {
public:
    enum State {
      None,
      SentCwd,
      SentRemove
    };
    
    ENGINE_STANDARD_COMMAND_CONSTRUCTOR(FtpCommandRemove, FtpSocket, CmdNone)
    
    QString destinationPath;
    QString parentDirectory;
    
    void process()
    {
      switch (currentState) {
        case None: {
          destinationPath = socket()->getConfig("params.remove.path");
          parentDirectory = socket()->getConfig("params.remove.parent");
          
          currentState = SentRemove;
          
          if (socket()->getConfig<bool>("params.remove.directory")) {
            if (socket()->getCurrentDirectory() != parentDirectory) {
              // We should change working directory to parent directory before removing
              currentState = SentCwd;
              socket()->sendCommand("CWD " + parentDirectory);
            } else {
              socket()->sendCommand("RMD " + destinationPath);
            }
          } else {
            socket()->sendCommand("DELE " + destinationPath);
          }
          break;
        }
        case SentCwd: {
          if (socket()->isMultiline())
            return;
          
          if (socket()->isResponse("2")) {
            // CWD was successful
            socket()->setCurrentDirectory(parentDirectory);
          }
          
          currentState = SentRemove;
          socket()->sendCommand("RMD " + destinationPath);
          break;
        }
        case SentRemove: {
          if (socket()->isMultiline())
            return;
          
          if (!socket()->isResponse("2")) {
            socket()->resetCommandClass(Failed);
          } else {
            // Invalidate cached parent entry (if any)
            Cache::self()->invalidateEntry(socket(), parentDirectory);
            Cache::self()->invalidatePath(socket(), destinationPath);
            
            if (!socket()->isChained())
              socket()->emitEvent(Event::EventReloadNeeded);
            socket()->resetCommandClass();
          }
          break;
        }
      }
    }
};

void FtpSocket::protoRemove(const KUrl &path)
{
  emitEvent(Event::EventState, i18n("Removing..."));
  
  // Set the file to remove
  setConfig("params.remove.parent", path.directory());
  setConfig("params.remove.path", path.path());
  
  activateCommandClass(FtpCommandRemove);
}

// *******************************************************************************************
// **************************************** RENAME *******************************************
// *******************************************************************************************

class FtpCommandRename : public Commands::Base {
public:
    enum State {
      None,
      SentRnfr,
      SentRnto
    };
    
    ENGINE_STANDARD_COMMAND_CONSTRUCTOR(FtpCommandRename, FtpSocket, CmdRename)
    
    QString sourcePath;
    QString destinationPath;
    
    void process()
    {
      switch (currentState) {
        case None: {
          sourcePath = socket()->getConfig("params.rename.source");
          destinationPath = socket()->getConfig("params.rename.destination");
          
          currentState = SentRnfr;
          socket()->sendCommand("RNFR " + sourcePath);
          break;
        }
        case SentRnfr: {
          if (socket()->isResponse("3")) {
            currentState = SentRnto;
            socket()->sendCommand("RNTO " + destinationPath);
          } else
            socket()->resetCommandClass(Failed);
          break;
        }
        case SentRnto: {
          if (socket()->isResponse("2")) {
            // Invalidate cached parent entry (if any)
            Cache::self()->invalidateEntry(socket(), KUrl(sourcePath).directory());
            Cache::self()->invalidateEntry(socket(), KUrl(destinationPath).directory());
            
            Cache::self()->invalidatePath(socket(), sourcePath);
            Cache::self()->invalidatePath(socket(), destinationPath);
            
            socket()->emitEvent(Event::EventReloadNeeded);
            socket()->resetCommandClass();
          } else
            socket()->resetCommandClass(Failed);
          break;
        }
      }
    }
};

void FtpSocket::protoRename(const KUrl &source, const KUrl &destination)
{
  emitEvent(Event::EventState, i18n("Renaming..."));
  
  // Set rename options
  setConfig("params.rename.source", source.path());
  setConfig("params.rename.destination", destination.path());
  
  activateCommandClass(FtpCommandRename);
}

// *******************************************************************************************
// **************************************** CHMOD ********************************************
// *******************************************************************************************

class FtpCommandChmod : public Commands::Base {
public:
    enum State {
      None,
      SentChmod
    };
    
    ENGINE_STANDARD_COMMAND_CONSTRUCTOR(FtpCommandChmod, FtpSocket, CmdChmod)
    
    void process()
    {
      switch (currentState) {
        case None: {
          currentState = SentChmod;
          
          QString chmod;
          chmod.sprintf("SITE CHMOD %.3d %s", socket()->getConfig<int>("params.chmod.mode", 0644),
                                              socket()->getConfig("params.chmod.path").toAscii().data());
          socket()->sendCommand(chmod);
          break;
        }
        case SentChmod: {
          if (!socket()->isResponse("2"))
            socket()->resetCommandClass(Failed);
          else {
            // Invalidate cached parent entry (if any)
            Cache::self()->invalidateEntry(socket(), KUrl(socket()->getConfig("params.chmod.path")).directory());
            
            socket()->emitEvent(Event::EventReloadNeeded);
            socket()->resetCommandClass();
          }
          break;
        }
      }
    }
};

void FtpSocket::protoChmodSingle(const KUrl &path, int mode)
{
  emitEvent(Event::EventState, i18n("Changing mode..."));
  
  // Set chmod options
  setConfig("params.chmod.path", path.path());
  setConfig("params.chmod.mode", mode);
  
  activateCommandClass(FtpCommandChmod);
}

// *******************************************************************************************
// **************************************** MKDIR ********************************************
// *******************************************************************************************

class FtpCommandMkdir : public Commands::Base {
public:
    enum State {
      None,
      SentMkdir
    };
    
    ENGINE_STANDARD_COMMAND_CONSTRUCTOR(FtpCommandMkdir, FtpSocket, CmdMkdir)
    
    void process()
    {
      switch (currentState) {
        case None: {
          currentState = SentMkdir;
          socket()->changeWorkingDirectory(socket()->getConfig("params.mkdir.path"), true);
          break;
        }
        case SentMkdir: {
          // Invalidate cached parent entry (if any)
          Cache::self()->invalidateEntry(socket(), KUrl(socket()->getCurrentDirectory()).directory());
          
          socket()->emitEvent(Event::EventReloadNeeded);
          socket()->resetCommandClass();
          break;
        }
      }
    }
};

void FtpSocket::protoMkdir(const KUrl &path)
{
  emitEvent(Event::EventState, i18n("Making directory..."));
  
  setConfig("params.mkdir.path", path.path());
  activateCommandClass(FtpCommandMkdir);
}

// *******************************************************************************************
// ******************************************* RAW *******************************************
// *******************************************************************************************

class FtpCommandRaw : public Commands::Base {
public:
    enum State {
      None,
      SentRaw
    };
    
    ENGINE_STANDARD_COMMAND_CONSTRUCTOR(FtpCommandRaw, FtpSocket, CmdRaw)
    
    QString response;
    
    void process()
    {
      switch (currentState) {
        case None: {
          currentState = SentRaw;
          socket()->sendCommand(socket()->getConfig("params.raw.command"));
          break;
        }
        case SentRaw: {
          response.append(socket()->getResponse());
          
          if (!socket()->isMultiline()) {
            socket()->emitEvent(Event::EventRaw, response);
            socket()->resetCommandClass();
          }
          break;
        }
      }
    }
};

void FtpSocket::protoRaw(const QString &raw)
{
  setConfig("params.raw.command", raw);
  activateCommandClass(FtpCommandRaw);
}

// *******************************************************************************************
// ******************************************* FXP *******************************************
// *******************************************************************************************

class FtpCommandFxp : public Commands::Base {
public:
    enum State {
      None,
      
      // Source socket
      SourceSentCwd,
      SourceSentStat,
      SourceDestVerified,
      SourceSentType,
      SourceSentSscn,
      SourceSentProt,
      SourceWaitType,
      SourceSentPret,
      SourceSentPasv,
      SourceDoRest,
      SourceSentRest,
      SourceDoRetr,
      SourceSentRetr,
      SourceWaitTransfer,
      SourceResetProt,
      
      // Destination socket
      DestSentStat,
      DestWaitCwd,
      DestDoType,
      DestSentType,
      DestSentSscn,
      DestSentProt,
      DestDoPort,
      DestSentPort,
      DestSentRest,
      DestDoStor,
      DestSentStor,
      DestWaitTransfer,
      DestResetProt
    };
    
    enum ProtectionMode {
      ProtClear = 0,
      ProtPrivate = 1,
      ProtSSCN = 2
    };
    
    enum TransferMode {
      TransferPASV = 0,
      TransferCPSV = 1
    };
    
    ENGINE_STANDARD_COMMAND_CONSTRUCTOR(FtpCommandFxp, FtpSocket, CmdFxp)
    
    FtpSocket *companion;
    
    KUrl sourceFile;
    KUrl destinationFile;
    filesize_t resumeOffset;
    
    void cleanup()
    {
      // We have been interrupted, so we have to abort the companion as well
      if (!socket()->getConfig<bool>("params.fxp.abort")) {
        companion->setConfig("params.fxp.abort", true);
        companion->protoAbort();
      }
      
      // Unclean upload termination, be sure to erase the cached stat infos
      if (!socket()->getConfig<bool>("params.fxp.keep_cache"))
        Cache::self()->invalidateEntry(socket(), destinationFile.directory());
    }
    
    void process()
    {
      switch (currentState) {
        case None: {
          sourceFile.setPath(socket()->getConfig("params.fxp.source"));
          destinationFile.setPath(socket()->getConfig("params.fxp.destination"));
          socket()->setConfig("params.fxp.keep_cache", false);
          
          // Who are we ? Where shall we begin ?
          if (socket()->getConfig<bool>("params.fxp.companion")) {
            // We are the companion, so we should check the destination
            socket()->setConfig("params.fxp.companion", false);
            
            currentState = DestSentStat;
            socket()->protoStat(destinationFile);
            return;
          } else {
            socket()->setConfig("params.transfer.mode", TransferPASV);
            
            if (socket()->getCurrentDirectory() != sourceFile.directory()) {
              // Attempt to CWD to the parent directory
              currentState = SourceSentCwd;
              socket()->sendCommand("CWD " + sourceFile.directory());
              return;
            }
          }
        }
        
        // ***************************************************************************
        // ***************************** Source socket *******************************
        // ***************************************************************************
        case SourceSentCwd: {
          if (currentState == SourceSentCwd) {
            if (!socket()->isResponse("250")) {
              socket()->emitError(FileNotFound);
              socket()->resetCommandClass(Failed);
              return;
            }
            
            if (socket()->isMultiline())
              return;
            else
              socket()->setCurrentDirectory(sourceFile.directory());
          }
          
          // We are the source socket, let's stat
          currentState = SourceSentStat;
          socket()->protoStat(sourceFile);
          break;
        }
        case SourceSentStat: {
          if (socket()->getStatResponse().filename().isEmpty()) {
            socket()->emitError(FileNotFound);
            socket()->resetCommandClass(Failed);
          } else {
            // File exists, invoke the companion
            companion->setConfig("params.fxp.companion", true);
            companion->Socket::thread()->siteToSite(socket()->Socket::thread(), sourceFile, destinationFile);
            currentState = SourceDestVerified;
          }
          break;
        }
        case SourceDestVerified: {
          if (isWakeup()) {
            // We have been waken up because a decision has been made
            FileExistsWakeupEvent *event = static_cast<FileExistsWakeupEvent*>(m_wakeupEvent);
            
            if (!socket()->getConfig<bool>("feat.rest") && event->action == FileExistsWakeupEvent::Resume)
              event->action = FileExistsWakeupEvent::Overwrite;
            
            switch (event->action) {
              case FileExistsWakeupEvent::Rename: {
                // Change the destination filename, otherwise it is the same as overwrite
                destinationFile.setPath(event->newFileName);
              }
              case FileExistsWakeupEvent::Overwrite: {
                companion->setConfig("params.fxp.rest", 0);
                resumeOffset = 0;
                break;
              }
              case FileExistsWakeupEvent::Resume: {
                companion->setConfig("params.fxp.rest", companion->getStatResponse().size());
                resumeOffset = companion->getStatResponse().size();
                break;
              }
              case FileExistsWakeupEvent::Skip: {
                // Transfer should be aborted
                companion->setConfig("params.fxp.keep_cache", true);
                socket()->setConfig("params.fxp.keep_cache", true);
                
                socket()->resetCommandClass(UserAbort);
                socket()->emitEvent(Event::EventTransferComplete);
                return;
              }
            }
          } else {
            companion->setConfig("params.fxp.rest", 0);
            resumeOffset = 0;
          }
          
          // Change type
          currentState = SourceSentType;
          
          QString type = "TYPE ";
          type.append(KFTPCore::Config::self()->ftpMode(sourceFile.path()));
          socket()->sendCommand(type);
          break;
        }
        case SourceSentType: {
          if (socket()->getConfig<bool>("ssl") && socket()->getConfig<int>("ssl.prot_mode") != 2 && !socket()->getConfig<bool>("sscn.activated")) {
            if (socket()->getConfig<int>("ssl.prot_mode") == 0) {
              if (socket()->getConfig<bool>("feat.sscn")) {
                // We support SSCN
                currentState = SourceSentSscn;
                socket()->sendCommand("SSCN ON");
                companion->setConfig("params.ssl.mode", ProtPrivate);
              } else if (companion->getConfig<bool>("feat.sscn")) {
                // Companion supports SSCN
                currentState = SourceWaitType;
                companion->setConfig("params.ssl.mode", ProtSSCN);
                companion->nextCommandAsync();
              } else if (socket()->getConfig<bool>("feat.cpsv")) {
                // We support CPSV
                currentState = SourceWaitType;
                socket()->setConfig("params.transfer.mode", TransferCPSV);
                companion->setConfig("params.ssl.mode", ProtPrivate);
                companion->nextCommandAsync();
              } else {
                // Neither support SSCN, can't do SSL transfer
                socket()->emitEvent(Event::EventMessage, i18n("Neither server supports SSCN/CPSV but SSL data connection requested, aborting transfer."));
                socket()->resetCommandClass(Failed);
                return;
              }
            } else {
              currentState = SourceSentProt;
              socket()->sendCommand("PROT C");
              companion->setConfig("params.ssl.mode", ProtClear);
            }
          } else {
            currentState = SourceWaitType;
            companion->nextCommandAsync();
          }
          break;
        }
        case SourceSentSscn: {
          if (!socket()->isResponse("2")) {
            socket()->resetCommandClass(Failed);
          } else {
            socket()->setConfig("sscn.activated", true);
            socket()->setConfig("params.fxp.changed_prot", 0);
            
            currentState = SourceWaitType;
            companion->nextCommandAsync();
          }
          break;
        }
        case SourceSentProt: {
          if (!socket()->isResponse("2")) {
            socket()->resetCommandClass(Failed);
          } else {
            socket()->setConfig("params.fxp.changed_prot", 1);
            
            currentState = SourceWaitType;
            companion->nextCommandAsync();
          }
          break;
        }
        case SourceWaitType: {
          // We are ready to invoke file transfer, do PASV
          if (socket()->getConfig<bool>("feat.pret")) {
            currentState = SourceSentPret;
            socket()->sendCommand("PRET RETR " + sourceFile.fileName());
          } else {
            currentState = SourceSentPasv;
            
            switch (socket()->getConfig<int>("params.transfer.mode")) {
              case TransferPASV: socket()->sendCommand("PASV"); break;
              case TransferCPSV: socket()->sendCommand("CPSV"); break;
            }
          }
          break;
        }
        case SourceSentPret: {
          if (!socket()->isResponse("2")) {
            if (socket()->isResponse("550")) {
              socket()->emitError(PermissionDenied);
              socket()->resetCommandClass(Failed);
              return;
            } else if (socket()->isResponse("530")) {
              socket()->emitError(FileNotFound);
              socket()->resetCommandClass(Failed);
            }
            
            socket()->setConfig("feat.pret", false);
          }
          
          currentState = SourceSentPasv;
            
          switch (socket()->getConfig<int>("params.transfer.mode")) {
            case TransferPASV: socket()->sendCommand("PASV"); break;
            case TransferCPSV: socket()->sendCommand("CPSV"); break;
          }
          break;
        }
        case SourceSentPasv: {
          // Parse the PASV response and get it to the companion to issue PORT
          if (!socket()->isResponse("2")) {
            socket()->resetCommandClass(Failed);
          } else {
            QString tmp = socket()->getResponse();
            int pos = tmp.indexOf('(') + 1;
            tmp = tmp.mid(pos, tmp.indexOf(')') - pos);
            
            currentState = SourceDoRest;
            companion->setConfig("params.fxp.ip", tmp);
            companion->nextCommandAsync();
          }
          break;
        }
        case SourceDoRest: {
          currentState = SourceSentRest;
          socket()->sendCommand("REST " + QString::number(resumeOffset));
          break;
        }
        case SourceSentRest: {
          if (!socket()->isResponse("2") && !socket()->isResponse("3")) {
            socket()->setConfig("feat.rest", false);
            companion->setConfig("params.fxp.rest", 0);
          } else {
            // Signal resume
            socket()->emitEvent(Event::EventResumeOffset, resumeOffset);
          }
          
          currentState = SourceDoRetr;
          companion->nextCommandAsync();
          break;
        }
        case SourceDoRetr: {
          currentState = SourceSentRetr;
          socket()->sendCommand("RETR " + sourceFile.fileName());
          break;
        }
        case SourceSentRetr: {
          if (!socket()->isResponse("1")) {
            socket()->resetCommandClass(Failed);
          } else {
            currentState = SourceWaitTransfer;
          }
          break;
        }
        case SourceWaitTransfer: {
          if (!socket()->isMultiline()) {
            // Transfer has been completed
            if (socket()->getConfig<int>("params.fxp.changed_prot")) {
              currentState = SourceResetProt;
              
              QString prot = "PROT ";
              
              if (socket()->getConfig<int>("ssl.prot_mode") == 0) 
                prot.append('P');
              else
                prot.append('C');
              
              socket()->sendCommand(prot);
            } else {
              markClean();
              
              socket()->emitEvent(Event::EventMessage, i18n("Transfer completed."));
              socket()->emitEvent(Event::EventTransferComplete);
              socket()->resetCommandClass();
            }
          }
          break;
        }
        case SourceResetProt: {
          markClean();
          
          socket()->emitEvent(Event::EventMessage, i18n("Transfer completed."));
          socket()->emitEvent(Event::EventTransferComplete);
          socket()->resetCommandClass();
          break;
        }
        
        // ***************************************************************************
        // *************************** Destination socket ****************************
        // ***************************************************************************
        case DestSentStat: {
          if (socket()->getStatResponse().filename().isEmpty()) {
            // Change the working directory
            currentState = DestWaitCwd;
            socket()->changeWorkingDirectory(destinationFile.directory(), true);
          } else {
            // The file already exists, request action
            DirectoryListing list;
            list.addEntry(companion->getStatResponse());
            list.addEntry(socket()->getStatResponse());
            
            currentState = DestDoType;
            socket()->emitEvent(Event::EventFileExists, list);
          }
          break;
        }
        case DestWaitCwd: {
          // Directory has been changed/created, call back the companion
          currentState = DestDoType;
          companion->nextCommandAsync();
          break;
        }
        case DestDoType: {
          currentState = DestSentType;
            
          QString type = "TYPE ";
          type.append(KFTPCore::Config::self()->ftpMode(sourceFile.path()));
          socket()->sendCommand(type);
          break;
        }
        case DestSentType: {
          if (socket()->getConfig<bool>("ssl")) {
            // Check what the source socket has instructed us to do
            switch (socket()->getConfig<int>("params.ssl.mode")) {
              case ProtClear: {
                // We should use cleartext data channel
                if (socket()->getConfig<int>("ssl.prot_mode") != 2) {
                  currentState = DestSentProt;
                  socket()->sendCommand("PROT C");
                } else {
                  currentState = DestDoPort;
                  companion->nextCommandAsync();
                }
                break;
              }
              case ProtPrivate: {
                // We should use private data channel
                if (socket()->getConfig<int>("ssl.prot_mode") != 0) {
                  currentState = DestSentProt;
                  socket()->sendCommand("PROT P");
                } else {
                  currentState = DestDoPort;
                  companion->nextCommandAsync();
                }
                break;
              }
              case ProtSSCN: {
                // We should initialize SSCN mode
                if (!socket()->getConfig<bool>("sscn.activated")) {
                  currentState = DestSentSscn;
                  socket()->sendCommand("SSCN ON");
                } else {
                  currentState = DestDoPort;
                  companion->nextCommandAsync();
                }
                break;
              }
            }
          } else {
            currentState = DestDoPort;
            companion->nextCommandAsync();
          }
          break;
        }
        case DestSentSscn: {
          if (!socket()->isResponse("2")) {
            socket()->resetCommandClass(Failed);
          } else {
            socket()->setConfig("sscn.activated", true);
            socket()->setConfig("params.fxp.changed_prot", 0);
            
            currentState = DestDoPort;
            companion->nextCommandAsync();
          }
          break;
        }
        case DestSentProt: {
          if (!socket()->isResponse("2")) {
            socket()->resetCommandClass(Failed);
          } else {
            socket()->setConfig("params.fxp.changed_prot", 1);
            
            currentState = DestDoPort;
            companion->nextCommandAsync();
          }
          break;
        }
        case DestDoPort: {
          currentState = DestSentPort;
          socket()->sendCommand("PORT " + socket()->getConfig("params.fxp.ip"));
          break;
        }
        case DestSentPort: {
          if (!socket()->isResponse("2")) {
            socket()->resetCommandClass(Failed);
          } else {
            currentState = DestSentRest;
            socket()->sendCommand("REST " + socket()->getConfig("params.fxp.rest"));
          }
          break;
        }
        case DestSentRest: {
          // We are ready for file transfer
          currentState = DestDoStor;
          companion->nextCommandAsync();
          break;
        }
        case DestDoStor: {
          currentState = DestSentStor;
          socket()->sendCommand("STOR " + destinationFile.fileName());
          break;
        }
        case DestSentStor: {
          if (!socket()->isResponse("1")) {
            socket()->resetCommandClass(Failed);
          } else {
            currentState = DestWaitTransfer;
            companion->nextCommandAsync();
          }
          break;
        }
        case DestWaitTransfer: {
          if (!socket()->isMultiline()) {
            // Transfer has been completed
            if (socket()->getConfig<bool>("params.fxp.changed_prot")) {
              currentState = DestResetProt;
              
              QString prot = "PROT ";
              
              if (socket()->getConfig<int>("ssl.prot_mode") == 0) 
                prot.append('P');
              else
                prot.append('C');
              
              socket()->sendCommand(prot);
            } else {
              markClean();
              
              socket()->emitEvent(Event::EventMessage, i18n("Transfer completed."));
              socket()->emitEvent(Event::EventReloadNeeded);
              socket()->resetCommandClass();
            }
          }
          break;
        }
        case DestResetProt: {
          markClean();
          
          socket()->emitEvent(Event::EventMessage, i18n("Transfer completed."));
          socket()->emitEvent(Event::EventReloadNeeded);
          socket()->resetCommandClass();
          break;
        }
      }
    }
};

void FtpSocket::protoSiteToSite(Socket *socket, const KUrl &source, const KUrl &destination)
{
  emitEvent(Event::EventState, i18n("Transferring..."));
  emitEvent(Event::EventMessage, i18n("Transferring file '%1'...",source.fileName()));
  
  // Set the source and destination
  setConfig("params.fxp.abort", 0);
  setConfig("params.fxp.source", source.path());
  setConfig("params.fxp.destination", destination.path());
  
  FtpCommandFxp *fxp = new FtpCommandFxp(this);
  fxp->companion = static_cast<FtpSocket*>(socket);
  m_cmdData = fxp;
  m_cmdData->process();
}

// *******************************************************************************************
// ******************************************* NOOP ******************************************
// *******************************************************************************************

class FtpCommandKeepAlive : public Commands::Base {
public:
    enum State {
      None,
      SentNoop
    };
    
    ENGINE_STANDARD_COMMAND_CONSTRUCTOR(FtpCommandKeepAlive, FtpSocket, CmdKeepAlive)
    
    void process()
    {
      switch (currentState) {
        case None: {
          currentState = SentNoop;
          socket()->sendCommand("NOOP");
          break;
        }
        case SentNoop: {
          socket()->resetCommandClass();
          break;
        }
      }
    }
};

void FtpSocket::protoKeepAlive()
{
  emitEvent(Event::EventState, i18n("Transmitting keep-alive..."));
  setCurrentCommand(Commands::CmdKeepAlive);
  activateCommandClass(FtpCommandKeepAlive);
}

}

#include "ftpsocket.moc"

