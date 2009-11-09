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

#include "sftpsocket.h"
#include "cache.h"
#include "misc/config.h"

#include <qdir.h>

#include <klocale.h>
#include <kstandarddirs.h>
#include <kio/job.h>
#include <kio/renamedlg.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <utime.h>

namespace KFTPEngine {

SftpSocket::SftpSocket(Thread *thread)
  : QTcpSocket(),
    Socket(thread, "sftp"),
    SpeedLimiterItem(),
    m_sshSession(0),
    m_sftpSession(0),
    m_login(false),
    m_transferHandle(0),
    m_transferBuffer(0),
    m_transferBufferSize(0)
{
  // Control socket signals
  connect(this, SIGNAL(connected()), this, SLOT(slotConnected()));
  connect(this, SIGNAL(disconnected()), this, SLOT(slotDisconnected()));
  connect(this, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(slotError()));
}

SftpSocket::~SftpSocket()
{
}

int addPermInt(int &x, int n, int add)
{
  if (x >= n) {
    x -= n;
    return add;
  } else {
    return 0;
  }
}

int SftpSocket::intToPosix(int permissions)
{
  int posix = 0;
  QString str = QString::number(permissions);
  
  int user = str.mid(0, 1).toInt();
  int group = str.mid(1, 1).toInt();
  int other = str.mid(2, 1).toInt();
  
  posix |= addPermInt(user, 4, S_IRUSR);
  posix |= addPermInt(user, 2, S_IWUSR);
  posix |= addPermInt(user, 1, S_IXUSR);
  
  posix |= addPermInt(group, 4, S_IRGRP);
  posix |= addPermInt(group, 2, S_IWGRP);
  posix |= addPermInt(group, 1, S_IXGRP);
  
  posix |= addPermInt(other, 4, S_IROTH);
  posix |= addPermInt(other, 2, S_IWOTH);
  posix |= addPermInt(other, 1, S_IXOTH);
  
  return posix;
}

// *******************************************************************************************
// ***************************************** CONNECT *****************************************
// *******************************************************************************************

class SftpCommandConnect : public Commands::Base {
public:
    enum State {
      None,
      ConnectComplete,
      LoginComplete
    };
    
    ENGINE_STANDARD_COMMAND_CONSTRUCTOR(SftpCommandConnect, SftpSocket, CmdConnect)
    
    LIBSSH2_SESSION *session;
    static KUrl url;
    
    void process()
    {
      url = socket()->getCurrentUrl();
      session = socket()->sshSession();
      
      switch (currentState) {
        case None: {
          session = libssh2_session_init();
          
          if (!session) {
            socket()->emitEvent(Event::EventMessage, i18n("Unable to establish SSH connection."));
            socket()->emitError(ConnectFailed, i18n("Unknown error."));
            socket()->protoAbort();
            return;
          }
          
          socket()->m_sshSession = session;
          
          // Set blocking mode for libssh2
          libssh2_session_set_blocking(session, 0);
          
          // Start handshake
          int rc;
          while ((rc = libssh2_session_startup(session, socket()->socketDescriptor())) == LIBSSH2_ERROR_EAGAIN) ;
          if (rc) {
            socket()->emitEvent(Event::EventMessage, i18n("Unable to establish SSH connection (error code %1.)", QString::number(rc)));
            socket()->emitError(ConnectFailed, i18n("SSH handshake has failed (error code %1.)", QString::number(rc)));
            socket()->protoAbort();
            return;
          }
          
          // Get fingerprint hash and request authentication
          if (!socket()->getConfig<bool>("auth.ignore_fingerprint")) {
            const char *fingerprint = libssh2_hostkey_hash(session, LIBSSH2_HOSTKEY_HASH_MD5);
            QByteArray qFingerprint = QByteArray(fingerprint, 16);
            
            if (!KFTPCore::Config::self()->certificateStore()->verifyFingerprint(url, qFingerprint)) {
              PeerVerifyWakeupEvent *response = static_cast<PeerVerifyWakeupEvent*>(socket()->emitEventAndWait(Event::EventPeerVerify, qFingerprint));
              
              if (!response || !response->peerOk) {
                socket()->emitEvent(Event::EventMessage, i18n("Peer verification has failed."));
                socket()->emitError(ConnectFailed, i18n("Identity of the peer cannot be established."));
                socket()->protoAbort();
                return;
              }
            }
          }
          
          currentState = ConnectComplete;
        }
        case ConnectComplete: {
          int rc;
          
          if (socket()->getConfig<bool>("auth.pubkey")) {
            // Public key authentication - try without password first
            QString password = socket()->getConfig<QString>("auth.privkey_password");
            
            for (;;) {
              while ((rc = libssh2_userauth_publickey_fromfile(session, (char*) url.user().toAscii().data(),
                                                                        (char*) socket()->getConfig<QString>("auth.pubkey_path").toAscii().data(),
                                                                        (char*) socket()->getConfig<QString>("auth.privkey_path").toAscii().data(),
                                                                        (char*) password.toAscii().data())) == LIBSSH2_ERROR_EAGAIN) ;
              
              if (rc && password.isEmpty()) {
                PubkeyWakeupEvent *response = static_cast<PubkeyWakeupEvent*>(socket()->emitEventAndWait(Event::EventPubkeyPassword));
                if (response)
                  password = response->password;
                
                if (password.isEmpty())
                  break;
                
                continue;
              }
              
              break;
            }
            
            if (rc) {
              socket()->emitEvent(Event::EventMessage, i18n("Public key authentication has failed."));
              socket()->emitError(LoginFailed, i18n("Unable to decrypt the public key or public key has been rejected by server."));
              
              socket()->protoAbort();
              return;
            }
            
            // Save privkey password to socket settings so it can be reused by secondary connections
            socket()->setConfig("auth.privkey_password", password);
            
            socket()->emitEvent(Event::EventMessage, i18n("Public key authentication succeeded."));
          } else if (url.hasPass()) {
            // First let's try simple password authentication
            while ((rc = libssh2_userauth_password(session,
                                                   (char*) url.user().toAscii().data(),
                                                   (char*) url.pass().toAscii().data())) == LIBSSH2_ERROR_EAGAIN) ;
            if (rc) {
              // Password authentication has failed, let's try keyboard-interactive
              while ((rc = libssh2_userauth_keyboard_interactive(session,
                                                                 (char*) url.user().toAscii().data(),
                                                                 &keyboardInteractiveCallback)) == LIBSSH2_ERROR_EAGAIN) ;
              if (rc) {
                socket()->emitEvent(Event::EventMessage, i18n("Authentication has failed."));
                socket()->emitError(LoginFailed, i18n("The specified login credentials were rejected by the server."));
                
                socket()->protoAbort();
                return;
              }
            }
            
            socket()->emitEvent(Event::EventMessage, i18n("Authentication succeeded."));
          } else {
            socket()->emitEvent(Event::EventMessage, i18n("No authentication methods available."));
            socket()->emitError(LoginFailed, i18n("No authentication methods available."));
            
            socket()->protoAbort();
            return;
          }
          
          currentState = LoginComplete;
        }
        case LoginComplete: {
          LIBSSH2_SFTP *sftpSession;
          
          do {
            sftpSession = libssh2_sftp_init(session);
            
            if (!sftpSession && libssh2_session_last_errno(session) != LIBSSH2_ERROR_EAGAIN) {
              socket()->emitEvent(Event::EventMessage, i18n("Unable to initialize SFTP channel."));
              socket()->emitError(LoginFailed, i18n("SFTP initialization has failed."));
              
              socket()->protoAbort();
            }
          } while (!sftpSession);
          
          socket()->m_sftpSession = sftpSession;
          
          // Get the current directory
          char cwd[1024];
          while (libssh2_sftp_realpath(sftpSession, "./", cwd, sizeof(cwd)) == LIBSSH2_ERROR_EAGAIN) ;
          socket()->setDefaultDirectory(socket()->remoteEncoding()->decode(cwd));
          socket()->setCurrentDirectory(socket()->remoteEncoding()->decode(cwd));
          
          socket()->emitEvent(Event::EventMessage, i18n("Connected."));
          socket()->emitEvent(Event::EventConnect);
          socket()->m_login = true;
          
          socket()->resetCommandClass();
          break;
        }
      }
    }
    
    static LIBSSH2_USERAUTH_KBDINT_RESPONSE_FUNC(keyboardInteractiveCallback)
    {
      Q_UNUSED(name)
      Q_UNUSED(name_len)
      Q_UNUSED(instruction)
      Q_UNUSED(instruction_len)
      Q_UNUSED(abstract)
      
      for (int i = 0; i < num_prompts; i++) {
        if (!prompts[i].echo) {
          responses[i].text = (char*) url.pass().toAscii().data();
          responses[i].length = url.pass().length();
        }
      }
    }
};

KUrl SftpCommandConnect::url = KUrl();

void SftpSocket::protoConnect(const KUrl &url)
{
  emitEvent(Event::EventState, i18n("Connecting..."));
  emitEvent(Event::EventMessage, i18n("Connecting to %1:%2...", url.host(), url.port()));
  
  if (!getConfig("encoding").isEmpty())
    changeEncoding(getConfig("encoding"));
  
  // Start the connect procedure
  setCurrentUrl(url);
  connectToHost(url.host(), url.port());
}

void SftpSocket::slotConnected()
{
  emitEvent(Event::EventState, i18n("Logging in..."));
  emitEvent(Event::EventMessage, i18n("Connected with server, stand by for authentication..."));
  activateCommandClass(SftpCommandConnect);
}

void SftpSocket::slotError()
{
  emitEvent(Event::EventMessage, i18n("Failed to connect (%1.)", errorString()));
  emitError(ConnectFailed, errorString());
  
  resetCommandClass(FailedSilently);
}

// *******************************************************************************************
// **************************************** DISCONNECT ***************************************
// *******************************************************************************************

void SftpSocket::protoDisconnect()
{
  if (!m_sshSession)
    return;
  
  Socket::protoDisconnect();
    
  if (m_sftpSession) {
    libssh2_sftp_shutdown(m_sftpSession);
    m_sftpSession = 0;
  }
  
  libssh2_session_disconnect(m_sshSession, "Disconnected.");
  libssh2_session_free(m_sshSession);
  m_sshSession = 0;
  QTcpSocket::disconnectFromHost();
  
  m_login = false;
}

void SftpSocket::protoAbort()
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

void SftpSocket::slotDisconnected()
{
  protoDisconnect();
}

// *******************************************************************************************
// ******************************************* LIST ******************************************
// *******************************************************************************************

class SftpCommandList : public Commands::Base {
public:
    enum State {
      None
    };
    
    ENGINE_STANDARD_COMMAND_CONSTRUCTOR(SftpCommandList, SftpSocket, CmdList)
    
    void process()
    {
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
      
      socket()->m_lastDirectoryListing = DirectoryListing(socket()->getCurrentDirectory());
      
      char directory[1024] = {0,};
      strcpy(directory, socket()->remoteEncoding()->encode(socket()->getCurrentDirectory()).data());
      
      LIBSSH2_SESSION *sshSession = socket()->sshSession();
      LIBSSH2_SFTP *sftpSession = socket()->sftpSession();
      LIBSSH2_SFTP_HANDLE *dir;
      do {
        dir = libssh2_sftp_opendir(sftpSession, directory);
        
        if (!dir && libssh2_session_last_errno(sshSession) != LIBSSH2_ERROR_EAGAIN) {
          if (socket()->errorReporting()) {
            socket()->emitError(ListFailed);
            socket()->resetCommandClass(Failed);
          } else {
            socket()->resetCommandClass();
          }
          
          return;
        }
      } while (!dir);
      
      // Read the specified directory
      for (;;) {
        int rc;
        char filename[512];
        LIBSSH2_SFTP_ATTRIBUTES attrs;
        DirectoryEntry entry;
        
        while ((rc = libssh2_sftp_readdir(dir, filename, sizeof(filename), &attrs)) == LIBSSH2_ERROR_EAGAIN) ;
        if (rc > 0) {
          entry.setFilename(filename);
          
          if (entry.filename() != "." && entry.filename() != "..") {
            entry.setFilename(socket()->remoteEncoding()->decode(filename));
            
            if (attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) {
              entry.setPermissions(attrs.permissions);
            }
            
            if (attrs.flags & LIBSSH2_SFTP_ATTR_UIDGID) {
              entry.setOwner(QString::number(attrs.uid));
              entry.setGroup(QString::number(attrs.gid));
            }
            
            if (attrs.flags & LIBSSH2_SFTP_ATTR_SIZE) {
              entry.setSize(attrs.filesize);
            }
            
            if (attrs.flags & LIBSSH2_SFTP_ATTR_ACMODTIME) {
              entry.setTime(attrs.mtime);
            }
            
            if (attrs.permissions & S_IFDIR)
              entry.setType('d');
            else
              entry.setType('f');
            
            socket()->m_lastDirectoryListing.addEntry(entry);
          }
        } else {
          break;
        }
      }
      
      while (libssh2_sftp_closedir(dir) == LIBSSH2_ERROR_EAGAIN) ;
      
      // Cache the directory listing
      Cache::self()->addDirectory(socket(), socket()->m_lastDirectoryListing);
      
      if (!socket()->isChained())
        socket()->emitEvent(Event::EventDirectoryListing, socket()->m_lastDirectoryListing);
      socket()->resetCommandClass();
    }
};

void SftpSocket::protoList(const KUrl &path)
{
  emitEvent(Event::EventState, i18n("Fetching directory listing..."));
  emitEvent(Event::EventMessage, i18n("Fetching directory listing..."));
  
  // Set the directory that should be listed
  setCurrentDirectory(path.path());
  
  activateCommandClass(SftpCommandList);
}

// *******************************************************************************************
// ******************************************* GET *******************************************
// *******************************************************************************************

class SftpCommandGet : public Commands::Base {
public:
    enum State {
      None,
      WaitStat,
      DestChecked,
      WaitTransfer
    };
    
    ENGINE_STANDARD_COMMAND_CONSTRUCTOR(SftpCommandGet, SftpSocket, CmdGet)
    
    KUrl sourceFile;
    KUrl destinationFile;
    filesize_t resumeOffset;
    time_t modificationTime;
    QTimer pollTimer;
    
    void cleanup()
    {
      socket()->getTransferFile()->close();
      socket()->disconnect(&pollTimer, SIGNAL(timeout()), socket(), SLOT(slotDataTryRead()));
      
      LIBSSH2_SFTP_HANDLE *rfile = socket()->m_transferHandle;
      if (rfile)
        while (libssh2_sftp_close(rfile) == LIBSSH2_ERROR_EAGAIN) ;
      
      free(socket()->m_transferBuffer);
      socket()->m_transferBuffer = 0;
      SpeedLimiter::self()->remove(socket());
    }
    
    void process()
    {
      switch (currentState) {
        case None: {
          // Stat source file
          modificationTime = 0;
          resumeOffset = 0;
          sourceFile.setPath(socket()->getConfig("params.get.source"));
          destinationFile.setPath(socket()->getConfig("params.get.destination"));
          
          currentState = WaitStat;
          socket()->protoStat(sourceFile);
          break;
        }
        case WaitStat: {
          socket()->emitEvent(Event::EventState, i18n("Transferring..."));
          
          if (socket()->getStatResponse().filename().isEmpty()) {
            markClean();
            
            socket()->emitError(FileNotFound);
            socket()->resetCommandClass(Failed);
            return;
          }
          
          modificationTime = socket()->getStatResponse().time();
          
          if (QDir::root().exists(destinationFile.path())) {
            DirectoryListing list;
            list.addEntry(socket()->getStatResponse());
            
            currentState = DestChecked;
            socket()->emitEvent(Event::EventFileExists, list);
            return;
          } else {
            KStandardDirs::makeDir(destinationFile.directory());
          }
        }
        case DestChecked: {
          if (isWakeup()) {
            // We have been waken up because a decision has been made
            FileExistsWakeupEvent *event = static_cast<FileExistsWakeupEvent*>(m_wakeupEvent);
            
            switch (event->action) {
              case FileExistsWakeupEvent::Rename: {
                // Change the destination filename, otherwise it is the same as overwrite
                destinationFile.setPath(event->newFileName);
              }
              case FileExistsWakeupEvent::Overwrite: {
                socket()->getTransferFile()->setFileName(destinationFile.path());
                socket()->getTransferFile()->open(QIODevice::WriteOnly | QIODevice::Truncate);
                break;
              }
              case FileExistsWakeupEvent::Resume: {
                socket()->getTransferFile()->setFileName(destinationFile.path());
                socket()->getTransferFile()->open(QIODevice::WriteOnly | QIODevice::Append);
                
                // Signal resume
                resumeOffset = socket()->getTransferFile()->size();
                socket()->emitEvent(Event::EventResumeOffset, resumeOffset);
                break;
              }
              case FileExistsWakeupEvent::Skip: {
                // Transfer should be aborted
                markClean();
                
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
          
          // Start the file transfer
          char path[1024] = {0,};
          strcpy(path, socket()->remoteEncoding()->encode(sourceFile.path()).data());
          
          LIBSSH2_SESSION *sshSession = socket()->sshSession();
          LIBSSH2_SFTP *sftpSession = socket()->sftpSession();
          LIBSSH2_SFTP_HANDLE *rfile;
          do {
            rfile = libssh2_sftp_open(sftpSession, path, LIBSSH2_FXF_READ, 0);
            
            if (!rfile && libssh2_session_last_errno(sshSession) != LIBSSH2_ERROR_EAGAIN) {
              markClean();
              
              socket()->getTransferFile()->close();
              socket()->resetCommandClass(Failed);
              return;
            }
          } while (!rfile);
          
          if (resumeOffset > 0)
            libssh2_sftp_seek(rfile, resumeOffset);
          
          // Initialize the transfer buffer
          socket()->m_transferBufferSize = 4096;
          socket()->m_transferBuffer = (char*) malloc(socket()->m_transferBufferSize);
          socket()->m_transferBytes = 0;
          socket()->m_transferHandle = rfile;
          socket()->m_speedLastTime = time(0);
          socket()->m_speedLastBytes = 0;
          SpeedLimiter::self()->append(socket(), SpeedLimiter::Download);
          
          // Connect to socket read notifications
          socket()->connect(&pollTimer, SIGNAL(timeout()), socket(), SLOT(slotDataTryRead()));
          pollTimer.start(0);
          
          currentState = WaitTransfer;
          break;
        }
        case WaitTransfer: {
          // Transfer has been completed
          markClean();
          
          socket()->getTransferFile()->close();
          socket()->disconnect(&pollTimer, SIGNAL(timeout()), socket(), SLOT(slotDataTryRead()));
          
          if (modificationTime != 0) {
            // Use the modification time we got from stating
            utimbuf tmp;
            tmp.actime = time(0);
            tmp.modtime = modificationTime;
            utime(destinationFile.path().toAscii(), &tmp);
          }
          
          LIBSSH2_SFTP_HANDLE *rfile = socket()->m_transferHandle;
          while (libssh2_sftp_close(rfile) == LIBSSH2_ERROR_EAGAIN) ;
          
          free(socket()->m_transferBuffer);
          socket()->m_transferBuffer = 0;
          SpeedLimiter::self()->remove(socket());
          
          socket()->emitEvent(Event::EventTransferComplete);
          socket()->emitEvent(Event::EventReloadNeeded);
          socket()->resetCommandClass();
          break;
        }
      }
    }
};

void SftpSocket::variableBufferUpdate(int size)
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

void SftpSocket::slotDataTryRead()
{
  if (!m_transferHandle)
    return;
  
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
  
  int readBytes = 0;
  do {
    readBytes = libssh2_sftp_read(m_transferHandle, m_transferBuffer, m_transferBufferSize);
  } while (readBytes == LIBSSH2_ERROR_EAGAIN);
  
  if (readBytes == 0) {
    // Transfer has been completed
    nextCommand();
    return;
  } else if (readBytes < 0) {
    // An error has ocurred while reading, transfer is aborted
    emitEvent(Event::EventMessage, i18n("Transfer has failed."));
    resetCommandClass(Failed);
    return;
  } else {
    updateUsage(readBytes);
    
    m_transferFile.write(m_transferBuffer, readBytes);
    m_transferBytes += readBytes;
  }
  
  if (updateVariableBuffer)
    variableBufferUpdate(readBytes);
}

void SftpSocket::protoGet(const KUrl &source, const KUrl &destination)
{
  emitEvent(Event::EventState, i18n("Transferring..."));
  emitEvent(Event::EventMessage, i18n("Downloading file '%1'...",source.fileName()));
  
  // Set the source and destination
  setConfig("params.get.source", source.path());
  setConfig("params.get.destination", destination.path());
  
  activateCommandClass(SftpCommandGet);
}

// *******************************************************************************************
// ******************************************* PUT *******************************************
// *******************************************************************************************

class SftpCommandPut : public Commands::Base {
public:
    enum State {
      None,
      WaitStat,
      DestChecked,
      WaitTransfer
    };
    
    ENGINE_STANDARD_COMMAND_CONSTRUCTOR(SftpCommandPut, SftpSocket, CmdPut)
    
    KUrl sourceFile;
    KUrl destinationFile;
    filesize_t resumeOffset;
    time_t modificationTime;
    QTimer pollTimer;
    
    void cleanup()
    {
      socket()->getTransferFile()->close();
      socket()->disconnect(&pollTimer, SIGNAL(timeout()), socket(), SLOT(slotDataTryWrite()));
      
      LIBSSH2_SFTP_HANDLE *rfile = socket()->m_transferHandle;
      if (rfile)
        while (libssh2_sftp_close(rfile) == LIBSSH2_ERROR_EAGAIN) ;
      
      free(socket()->m_transferBuffer);
      socket()->m_transferBuffer = 0;
      SpeedLimiter::self()->remove(socket());
    }
    
    void process()
    {
      switch (currentState) {
        case None: {
          // Stat source file
          resumeOffset = 0;
          modificationTime = 0;
          sourceFile.setPath(socket()->getConfig("params.get.source"));
          destinationFile.setPath(socket()->getConfig("params.get.destination"));
          
          if (!QDir::root().exists(sourceFile.path())) {
            markClean();
            
            socket()->emitError(FileNotFound);
            socket()->resetCommandClass(Failed);
            return;
          } else {
            modificationTime = QFileInfo(sourceFile.path()).lastModified().toTime_t();
          }
          
          currentState = WaitStat;
          socket()->protoStat(destinationFile);
          break;
        }
        case WaitStat: {
          socket()->emitEvent(Event::EventState, i18n("Transferring..."));
          
          if (!socket()->getStatResponse().filename().isEmpty()) {
            DirectoryListing list;
            list.addEntry(socket()->getStatResponse());
            
            currentState = DestChecked;
            socket()->emitEvent(Event::EventFileExists, list);
            return;
          } else {
            // Create destination directories
            socket()->setErrorReporting(false);
            
            QString destinationDir = destinationFile.directory();
            QString fullPath;
            
            for (register int i = 1; i <= destinationDir.count('/'); i++) {
              fullPath += "/" + destinationDir.section('/', i, i);
              
              // Create the directory
              socket()->protoMkdir(fullPath);
            }
          }
        }
        case DestChecked: {
          if (isWakeup()) {
            // We have been waken up because a decision has been made
            FileExistsWakeupEvent *event = static_cast<FileExistsWakeupEvent*>(m_wakeupEvent);
            
            switch (event->action) {
              case FileExistsWakeupEvent::Rename: {
                // Change the destination filename, otherwise it is the same as overwrite
                destinationFile.setPath(event->newFileName);
              }
              case FileExistsWakeupEvent::Overwrite: {
                socket()->getTransferFile()->setFileName(sourceFile.path());
                socket()->getTransferFile()->open(QIODevice::ReadOnly);
                break;
              }
              case FileExistsWakeupEvent::Resume: {
                resumeOffset = socket()->getStatResponse().size();
                
                socket()->getTransferFile()->setFileName(sourceFile.path());
                socket()->getTransferFile()->open(QIODevice::ReadOnly);
                socket()->getTransferFile()->seek(resumeOffset);
                
                // Signal resume
                socket()->emitEvent(Event::EventResumeOffset, resumeOffset);
                break;
              }
              case FileExistsWakeupEvent::Skip: {
                // Transfer should be aborted
                markClean();
                
                socket()->emitEvent(Event::EventTransferComplete);
                socket()->resetCommandClass();
                return;
              }
            }
          } else {
            // The file doesn't exist so we are free to overwrite
            socket()->getTransferFile()->setFileName(sourceFile.path());
            socket()->getTransferFile()->open(QIODevice::ReadOnly);
          }
          
          // Start the file transfer
          char path[1024] = {0,};
          strcpy(path, socket()->remoteEncoding()->encode(destinationFile.path()).data());
          
          int flags = resumeOffset > 0 ? LIBSSH2_FXF_WRITE | LIBSSH2_FXF_APPEND : 
                                         LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC;
          
          LIBSSH2_SESSION *sshSession = socket()->sshSession();
          LIBSSH2_SFTP *sftpSession = socket()->sftpSession();
          LIBSSH2_SFTP_HANDLE *rfile;
          do {
            rfile = libssh2_sftp_open(sftpSession, path, flags, LIBSSH2_SFTP_S_IRUSR | LIBSSH2_SFTP_S_IWUSR |
                                                                LIBSSH2_SFTP_S_IRGRP | LIBSSH2_SFTP_S_IROTH);
            
            if (!rfile && libssh2_session_last_errno(sshSession) != LIBSSH2_ERROR_EAGAIN) {
              markClean();
              
              socket()->getTransferFile()->close();
              socket()->resetCommandClass(Failed);
              return;
            }
          } while (!rfile);
          
          if (resumeOffset > 0)
            libssh2_sftp_seek(rfile, resumeOffset);
          
          // Initialize the transfer buffer
          socket()->m_transferBufferSize = 4096;
          socket()->m_transferBuffer = (char*) malloc(socket()->m_transferBufferSize);
          socket()->m_transferBytes = 0;
          socket()->m_transferHandle = rfile;
          socket()->m_speedLastTime = time(0);
          socket()->m_speedLastBytes = 0;
          SpeedLimiter::self()->append(socket(), SpeedLimiter::Upload);
          
          // Connect to socket read notifications
          socket()->connect(&pollTimer, SIGNAL(timeout()), socket(), SLOT(slotDataTryWrite()));
          pollTimer.start(0);
          
          currentState = WaitTransfer;
          break;
        }
        case WaitTransfer: {
          // Transfer has been completed
          markClean();
          
          socket()->getTransferFile()->close();
          socket()->disconnect(&pollTimer, SIGNAL(timeout()), socket(), SLOT(slotDataTryWrite()));
          
          LIBSSH2_SFTP_HANDLE *rfile = socket()->m_transferHandle;
          
          if (modificationTime != 0) {
            // Use the modification time we got from stating
            LIBSSH2_SFTP_ATTRIBUTES attrs;
            attrs.mtime = modificationTime;
            attrs.flags = LIBSSH2_SFTP_ATTR_ACMODTIME;
            
            while (libssh2_sftp_fsetstat(rfile, &attrs) == LIBSSH2_ERROR_EAGAIN) ;
          }
          
          while (libssh2_sftp_close(rfile) == LIBSSH2_ERROR_EAGAIN) ;
          
          free(socket()->m_transferBuffer);
          socket()->m_transferBuffer = 0;
          SpeedLimiter::self()->remove(socket());
          
          socket()->emitEvent(Event::EventTransferComplete);
          socket()->emitEvent(Event::EventReloadNeeded);
          socket()->resetCommandClass();
          break;
        }
      }
    }
};

void SftpSocket::slotDataTryWrite()
{
  if (!m_transferHandle)
    return;
  
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
    nextCommand();
    return;
  }
  
  qint64 tmpOffset = getTransferFile()->pos();
  qint64 readSize = getTransferFile()->read(m_transferBuffer, m_transferBufferSize);
  int writtenBytes = 0;
  
  do {
    writtenBytes = libssh2_sftp_write(m_transferHandle, m_transferBuffer, readSize);
  } while (writtenBytes == LIBSSH2_ERROR_EAGAIN);
  
  if (writtenBytes < 0) {
    // An error has ocurred while writing, transfer is aborted
    emitEvent(Event::EventMessage, i18n("Transfer has failed."));
    resetCommandClass(Failed);
    return;
  } else if (writtenBytes < readSize) {
    getTransferFile()->seek(tmpOffset + writtenBytes);
  }
  
  m_transferBytes += writtenBytes;
  updateUsage(writtenBytes);
  
  if (getTransferFile()->atEnd()) {
    // We have reached the end of file, so we should terminate the connection
    nextCommand();
    return;
  }
  
  if (updateVariableBuffer)
    variableBufferUpdate(writtenBytes);
}

void SftpSocket::protoPut(const KUrl &source, const KUrl &destination)
{
  emitEvent(Event::EventState, i18n("Transferring..."));
  emitEvent(Event::EventMessage, i18n("Uploading file '%1'...",source.fileName()));
  
  // Set the source and destination
  setConfig("params.get.source", source.path());
  setConfig("params.get.destination", destination.path());
  
  activateCommandClass(SftpCommandPut);
}

// *******************************************************************************************
// **************************************** REMOVE *******************************************
// *******************************************************************************************

void SftpSocket::protoRemove(const KUrl &path)
{
  emitEvent(Event::EventState, i18n("Removing..."));
  
  // Remove a file or directory
  int result = 0;
  
  if (getConfig<bool>("params.remove.directory"))
    while ((result = libssh2_sftp_rmdir(m_sftpSession, remoteEncoding()->encode(path.path()).data())) == LIBSSH2_ERROR_EAGAIN) ;
  else
    while ((result = libssh2_sftp_unlink(m_sftpSession, remoteEncoding()->encode(path.path()).data())) == LIBSSH2_ERROR_EAGAIN) ;
    
  if (result < 0) {
    resetCommandClass(Failed);
  } else {
    // Invalidate cached parent entry (if any)
    Cache::self()->invalidateEntry(this, path.directory());
    
    emitEvent(Event::EventReloadNeeded);
    resetCommandClass();
  }
}

// *******************************************************************************************
// **************************************** RENAME *******************************************
// *******************************************************************************************

void SftpSocket::protoRename(const KUrl &source, const KUrl &destination)
{
  emitEvent(Event::EventState, i18n("Renaming..."));
  
  int result = 0;
  while ((result = libssh2_sftp_rename(m_sftpSession,
                                       remoteEncoding()->encode(source.path()).data(),
                                       remoteEncoding()->encode(destination.path()).data()) == LIBSSH2_ERROR_EAGAIN)) ;
  
  if (result < 0) {
    resetCommandClass(Failed);
  } else {
    // Invalidate cached parent entry (if any)
    Cache::self()->invalidateEntry(this, source.directory());
    Cache::self()->invalidateEntry(this, destination.directory());
    
    emitEvent(Event::EventReloadNeeded);
    resetCommandClass();
  }
}

// *******************************************************************************************
// **************************************** CHMOD ********************************************
// *******************************************************************************************

void SftpSocket::protoChmodSingle(const KUrl &path, int mode)
{
  emitEvent(Event::EventState, i18n("Changing mode..."));
  
  LIBSSH2_SFTP_ATTRIBUTES attrs;
  attrs.permissions = intToPosix(mode);
  attrs.flags = LIBSSH2_SFTP_ATTR_PERMISSIONS;
  
  while (libssh2_sftp_setstat(m_sftpSession, remoteEncoding()->encode(path.path()).data(), &attrs) == LIBSSH2_ERROR_EAGAIN) ;
  
  // Invalidate cached parent entry (if any)
  Cache::self()->invalidateEntry(this, path.directory());
  
  emitEvent(Event::EventReloadNeeded);
  resetCommandClass();
}

// *******************************************************************************************
// **************************************** MKDIR ********************************************
// *******************************************************************************************

void SftpSocket::protoMkdir(const KUrl &path)
{
  int result = 0;
  while ((result = libssh2_sftp_mkdir(m_sftpSession, remoteEncoding()->encode(path.path()).data(), 0755)) == LIBSSH2_ERROR_EAGAIN) ;
  
  if (result < 0) {
    if (errorReporting(true))
      resetCommandClass(Failed);
  } else {
    // Invalidate cached parent entry (if any)
    Cache::self()->invalidateEntry(this, path.directory());
    
    if (errorReporting(true)) {
      emitEvent(Event::EventReloadNeeded);
      resetCommandClass();
    }
  }
}

}

#include "sftpsocket.moc"

