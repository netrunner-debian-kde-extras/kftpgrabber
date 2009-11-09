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
#include "socket.h"
#include "thread.h"
#include "connectionretry.h"
#include "speedlimiter.h"
#include "cache.h"

#include "misc/config.h"

#include <KLocale>

namespace KFTPEngine {

Socket::Socket(Thread *thread, const QString &protocol)
 : m_remoteEncoding(new KRemoteEncoding()),
   m_cmdData(0),
   m_settings(thread->settings()),
   m_thread(thread),
   m_transferBytes(0),
   m_speedLastTime(0),
   m_speedLastBytes(0),
   m_protocol(protocol),
   m_currentCommand(Commands::CmdNone),
   m_errorReporting(true)
{
}

Socket::~Socket()
{
  delete m_remoteEncoding;
  delete m_connectionRetry;
}

void Socket::emitError(ErrorCode code, const QString &param1)
{
  // Intercept connect and login errors and pass them on to the ConnectionRetry class (if enabled)
  if (getConfig<bool>("retry", false) && (code == ConnectFailed || code == LoginFailed)) {
    if (!m_connectionRetry)
      m_connectionRetry = new ConnectionRetry(this);
      
    m_connectionRetry->startRetry();
    return;
  }
  
  QList<QVariant> params;
  params.append(QVariant(code));
  params.append(QVariant(param1));
  
  // Dispatch the event via socket thread
  m_thread->emitEvent(Event::EventError, params);
}

void Socket::emitEvent(Event::Type type, const QString &param1, const QString &param2)
{
  QList<QVariant> params;
  params.append(QVariant(param1));
  params.append(QVariant(param2));
  
  // Dispatch the event via socket thread
  m_thread->emitEvent(type, params);
}

void Socket::emitEvent(Event::Type type, DirectoryListing param1)
{
  emitEvent(type, QVariant::fromValue(param1));
}

void Socket::emitEvent(Event::Type type, DirectoryTree *param1)
{
  emitEvent(type, QVariant::fromValue(param1));
}

void Socket::emitEvent(Event::Type type, const QVariant &param1)
{
  QList<QVariant> params;
  params.append(param1);
  
  // Dispatch the event via socket thread
  m_thread->emitEvent(type, params);
}

WakeupEvent *Socket::emitEventAndWait(Event::Type type, const QVariant &param1)
{
  emitEvent(type, param1);
  return m_thread->waitForWakeup();
}

void Socket::changeEncoding(const QString &encoding)
{
  // Alter encoding and change socket config
  m_remoteEncoding->setEncoding(encoding.toAscii());
  setConfig("encoding", encoding);
}

void Socket::protoDisconnect()
{
  resetCommandClass(UserAbort);
  
  emitEvent(Event::EventMessage, i18n("Disconnected."));
  emitEvent(Event::EventDisconnect);
}

void Socket::timeoutWait(bool start)
{
  if (start) {
    m_timeoutCounter.start();
  } else {
    m_timeoutCounter = QTime();
  }
}
    
void Socket::timeoutPing()
{
  m_timeoutCounter.restart();
}
    
void Socket::timeoutCheck()
{
  if (!isConnected())
    return;
  
  if (!m_timeoutCounter.isNull()) {
    Commands::Type command = getCurrentCommand();
    int timeout = 0;
    
    // Ignore timeouts for FXP transfers, since there is no way to do pings
    if (command == Commands::CmdFxp)
      return;
    
    if (command == Commands::CmdGet || command == Commands::CmdPut)
      timeout = KFTPCore::Config::dataTimeout();
    else
      timeout = KFTPCore::Config::controlTimeout();
    
    if (timeout > 0 && m_timeoutCounter.elapsed() > (timeout * 1000)) {
      timeoutWait(false);
      
      // We have a timeout, let's abort
      emitEvent(Event::EventMessage, i18n("Connection timed out."));
      protoDisconnect();
    }
  }
}

void Socket::keepaliveStart()
{
  m_keepaliveCounter.start();
}

void Socket::keepaliveCheck()
{
  // Ignore keepalive if the socket is busy
  if (isBusy() || !isConnected()) {
    m_keepaliveCounter.restart();
    return;
  }
  
  if (getConfig<bool>("keepalive.enabled", false) && 
      m_keepaliveCounter.elapsed() > getConfig<int>("keepalive.timeout", 0) * 1000) {
    protoKeepAlive();
    
    // Reset the counter
    m_keepaliveCounter.restart();
  }
}

bool Socket::errorReporting(bool ignoreChaining) const
{
  return ignoreChaining ? m_errorReporting : m_errorReporting || !isChained();
}

Commands::Type Socket::getCurrentCommand()
{
  if (m_commandChain.count() > 0) {
    QStack<Commands::Base*>::iterator chainEnd = m_commandChain.end();
    for (QStack<Commands::Base*>::iterator i = m_commandChain.begin(); i != chainEnd; i++) {
      if ((*i)->command() != Commands::CmdNone)
        return (*i)->command();
    }
  }
  
  return m_currentCommand;
}

Commands::Type Socket::getToplevelCommand()
{
  return m_currentCommand;
}

Commands::Type Socket::getPreviousCommand()
{
  if (!isChained())
    return Commands::CmdNone;
  
  if (m_commandChain.count() > 1) {
    Commands::Base *tmp = m_commandChain.pop();
    Commands::Base *previous = m_commandChain.top();
    m_commandChain.push(tmp);
    
    return previous->command();
  } else {
    return m_currentCommand;
  }
}

void Socket::resetCommandClass(ResetCode code)
{
  if (m_commandChain.count() > 0) {
    Commands::Base *current = m_commandChain.top();
    
    if (current->isProcessing()) {
      current->autoDestruct(code);
      return;
    } else {
      if (!current->isClean())
        current->cleanup();
      
      delete m_commandChain.pop();
    }
    
    if (code == Ok) {
      nextCommandAsync();
    } else {
      // Command has completed with an error code. We should abort the
      // complete chain.
      resetCommandClass(code);
    }
  } else {
    if (m_cmdData) {
      if (m_cmdData->isProcessing()) {
        m_cmdData->autoDestruct(code);
        return;
      } else {
        if (!m_cmdData->isClean())
          m_cmdData->cleanup();
          
        delete m_cmdData;
        m_cmdData = 0;
      }
    }
    
    if (code == Failed)
      emitError(OperationFailed);
    
    // Reset current command and emit a ready event
    if (getCurrentCommand() != Commands::CmdConnectRetry) {
      setCurrentCommand(Commands::CmdNone);
      emitEvent(Event::EventReady);
      emitEvent(Event::EventState, i18n("Idle."));
    }
    
    setErrorReporting(true);
  }
}

void Socket::nextCommand()
{
  if (m_commandChain.count() > 0) {
    Commands::Base *current = m_commandChain.top();
    
    current->setProcessing(true);
    current->process();
    current->setProcessing(false);
    
    if (current->isDestructable())
      resetCommandClass(current->resetCode());
  } else if (m_cmdData) {
    m_cmdData->setProcessing(true);
    m_cmdData->process();
    m_cmdData->setProcessing(false);
    
    if (m_cmdData->isDestructable())
      resetCommandClass(m_cmdData->resetCode());
  }
}

void Socket::nextCommandAsync()
{
  m_thread->deferCommandExec();
}

void Socket::wakeup(WakeupEvent *event)
{
  if (m_commandChain.count() > 0) {
    Commands::Base *current = m_commandChain.top();
    
    if (current->isProcessing()) {
      qDebug("WARNING: Attempted to wakeup a processing socket!");
      return;
    }
    
    current->setProcessing(true);
    current->wakeup(event);
    current->setProcessing(false);
    
    if (current->isDestructable())
      resetCommandClass(current->resetCode());
  } else if (m_cmdData) {
    if (m_cmdData->isProcessing()) {
      qDebug("WARNING: Attempted to wakeup a processing socket!");
      return;
    }
    
    m_cmdData->setProcessing(true);
    m_cmdData->wakeup(event);
    m_cmdData->setProcessing(false);
    
    if (m_cmdData->isDestructable())
      resetCommandClass(m_cmdData->resetCode());
  }
}

filesize_t Socket::getTransferSpeed()
{
  time_t timeDelta = time(0) - m_speedLastTime;
  
  if (timeDelta == 0)
    return 0;
  
  if (m_speedLastBytes > m_transferBytes)
    m_speedLastBytes = 0;
  
  filesize_t speed = (m_transferBytes - m_speedLastBytes)/(time(0) - m_speedLastTime);
  
  m_speedLastBytes = m_transferBytes;
  m_speedLastTime = time(0);
  
  return speed;
}

void Socket::protoAbort()
{
  if (m_connectionRetry && !m_cmdData)
    m_connectionRetry->abortRetry();
}

// *******************************************************************************************
// ******************************************* STAT ******************************************
// *******************************************************************************************

class FtpCommandStat : public Commands::Base {
public:
    enum State {
      None,
      WaitList
    };
    
    ENGINE_STANDARD_COMMAND_CONSTRUCTOR(FtpCommandStat, Socket, CmdNone)
    
    KUrl path;
    
    void process()
    {
      switch (currentState) {
        case None: {
          // Issue a list of the parent directory
          currentState = WaitList;
          socket()->setErrorReporting(false);
          socket()->protoList(path.directory());
          break;
        }
        case WaitList: {
          // Now just extract what we need
          QList<DirectoryEntry> list = socket()->getLastDirectoryListing().list();
          QList<DirectoryEntry>::iterator listEnd = list.end();
          for (QList<DirectoryEntry>::iterator i = list.begin(); i != listEnd; i++) {
            if ((*i).filename() == path.fileName()) {
              socket()->m_lastStatResponse = *i;
              socket()->resetCommandClass();
              return;
            }
          }
          
          // We found no such file
          socket()->m_lastStatResponse = DirectoryEntry();
          socket()->resetCommandClass();
          break;
        }
      }
    }
};

void Socket::protoStat(const KUrl &path)
{
  // Lookup the cache first and don't even try to list if cached
  DirectoryListing cached = Cache::self()->findCached(this, path.directory());
  if (cached.isValid()) {
    QList<DirectoryEntry> list = cached.list();
    QList<DirectoryEntry>::iterator listEnd = list.end();
    for (QList<DirectoryEntry>::iterator i = list.begin(); i != listEnd; i++) {
      if ((*i).filename() == path.fileName()) {
        m_lastStatResponse = *i;
        nextCommandAsync();
        return;
      }
    }
    
    // Cached is valid but file can't be found
    m_lastStatResponse = DirectoryEntry();
    nextCommandAsync();
    return;
  }
  
  // Not cached, let's do a real listing
  FtpCommandStat *stat = new FtpCommandStat(this);
  stat->path = path;
  addToCommandChain(stat);
  nextCommand();
}

// *******************************************************************************************
// ****************************************** SCAN *******************************************
// *******************************************************************************************

class FtpCommandScan : public Commands::Base {
public:
    enum State {
      None,
      SentList,
      ProcessList,
      ScannedDir
    };
    
    ENGINE_STANDARD_COMMAND_CONSTRUCTOR(FtpCommandScan, Socket, CmdNone)
    
    QList<DirectoryEntry> currentList;
    QList<DirectoryEntry>::ConstIterator currentEntry;
    
    QString currentDirectory;
    DirectoryTree *currentTree;
    
    void cleanup()
    {
      // We didn't emit the tree, so we should free it
      if (!socket()->isChained())
        delete currentTree;
    }
    
    void process()
    {
      // NOTE: The missing breaks are mising for a purpuse! Do not dare to add them ;)
      switch (currentState) {
        case None: {
          // We would like to disable error reporting
          socket()->setErrorReporting(false);
          
          // Issue a directory listing on the given URL
          currentState = SentList;
          socket()->protoList(currentDirectory);
          break;
        }
        case SentList: {
          currentList = socket()->getLastDirectoryListing().list();
          qSort(currentList);
          
          currentEntry = currentList.constBegin();
          currentState = ProcessList;
          
          // Empty listing, we are done
          if (currentEntry == currentList.constEnd()) {
            if (socket()->isChained()) {
              socket()->resetCommandClass();
            } else {
              // We are the toplevel scan command
              markClean();
              
              socket()->emitEvent(Event::EventScanComplete, currentTree);
              socket()->emitEvent(Event::EventMessage, i18n("Scan complete."));
              socket()->resetCommandClass();
            }
            
            return;
          }
        }
        case ProcessList: {
          if ((*currentEntry).isDirectory()) {
            // A directory entry
            DirectoryTree *tree = currentTree->addDirectory(*currentEntry);
            currentState = ScannedDir;
            
            FtpCommandScan *scan = new FtpCommandScan(socket());
            scan->currentDirectory = currentDirectory + "/" + (*currentEntry).filename();
            scan->currentTree = tree;
            socket()->addToCommandChain(scan);
            socket()->nextCommandAsync();
            return;
          } else {
            // A file entry
            currentTree->addFile(*currentEntry);
          }
        }
        case ScannedDir: {
          currentState = ProcessList;
          
          if (++currentEntry == currentList.constEnd()) {
            // We are done
            if (socket()->isChained()) {
              socket()->resetCommandClass();
            } else {
              // We are the toplevel scan command
              markClean();
              
              socket()->emitEvent(Event::EventScanComplete, currentTree);
              socket()->emitEvent(Event::EventMessage, i18n("Scan complete."));
              socket()->resetCommandClass();
            }
          } else {
            socket()->nextCommandAsync();
          }
          break;
        }
      }
    }
};

void Socket::protoScan(const KUrl &path)
{
  emitEvent(Event::EventMessage, i18n("Starting recursive directory scan..."));
  
  // We have to create a new command class manually, since we need to set the
  // currentTree parameter
  FtpCommandScan *scan = new FtpCommandScan(this);
  scan->currentDirectory = path.path();
  scan->currentTree = new DirectoryTree(DirectoryEntry());
  m_cmdData = scan;
  m_cmdData->process();
}

// *******************************************************************************************
// ***************************************** DELETE ******************************************
// *******************************************************************************************

class FtpCommandDelete : public Commands::Base {
public:
    enum State {
      None,
      VerifyDir,
      SimpleRemove,
      SentList,
      ProcessList,
      DeletedDir,
      DeletedFile
    };
    
    ENGINE_STANDARD_COMMAND_CONSTRUCTOR(FtpCommandDelete, Socket, CmdDelete)
    
    QList<DirectoryEntry> currentList;
    QList<DirectoryEntry>::const_iterator currentEntry;
    
    KUrl destinationPath;
    
    void process()
    {
      switch (currentState) {
        case None: {
          // We have to determine if the destination is a file or a directory
          // TODO use cached information
          if (socket()->isChained()) {
            // We know that it is a directory
            currentState = SentList;
            socket()->protoList(destinationPath);
          } else {
            currentState = VerifyDir;
            socket()->protoStat(destinationPath);
          }
          break;
        }
        case VerifyDir: {
          DirectoryEntry entry = socket()->getStatResponse();
          
          if (entry.filename().isEmpty()) {
            // The file doesn't exist, abort
            socket()->resetCommandClass(Failed);
          } else {
            if (entry.isDirectory()) {
              // It is a directory, remove recursively
              currentState = SentList;
              socket()->protoList(destinationPath);
            } else {
              // A single file, a simple remove
              currentState = SimpleRemove;
              socket()->setConfig("params.remove.directory", 0);
              socket()->protoRemove(destinationPath);
            }
          }          
          break;
        }
        case SimpleRemove: {
          if (!socket()->isChained())
            socket()->emitEvent(Event::EventReloadNeeded);
          socket()->resetCommandClass();
          break;
        }
        case SentList: {
          currentList = socket()->getLastDirectoryListing().list();
          currentEntry = currentList.constBegin();
          currentState = ProcessList;
          
          // Empty listing, we are done
          if (currentEntry == currentList.constEnd()) {
            if (socket()->isChained())
              socket()->resetCommandClass();
            else {
              // We are the top level command class, remove the destination dir
              currentState = SimpleRemove;
              socket()->setConfig("params.remove.directory", 1);
              socket()->protoRemove(destinationPath);
            }
            
            return;
          }
        }
        case ProcessList: {
          KUrl childPath = destinationPath;
          childPath.addPath((*currentEntry).filename());
          
          if ((*currentEntry).isDirectory()) {
            // A directory, chain another delete command
            currentState = DeletedDir;
            
            // Chain manually, since we need to set some parameters
            FtpCommandDelete *del = new FtpCommandDelete(socket());
            del->destinationPath = childPath;
            socket()->addToCommandChain(del);
            socket()->nextCommand();
          } else {
            // A file entry - remove
            currentState = DeletedFile;
            socket()->setConfig("params.remove.directory", 0);
            socket()->protoRemove(childPath);
          }
          break;
        }
        case DeletedDir: {
          // We have to remove the empty directory
          KUrl childPath = destinationPath;
          childPath.addPath((*currentEntry).filename());
          
          currentState = DeletedFile;
          socket()->setConfig("params.remove.directory", 1);
          socket()->protoRemove(childPath);
          break;
        }
        case DeletedFile: {
          currentState = ProcessList;
          
          if (++currentEntry == currentList.constEnd()) {
            if (socket()->isChained())
              socket()->resetCommandClass();
            else {
              // We are the top level command class, remove the destination dir
              currentState = SimpleRemove;
              socket()->setConfig("params.remove.directory", 1);
              socket()->protoRemove(destinationPath);
            }
          } else
            socket()->nextCommand();
          break;
        }
      }
    }
};

void Socket::protoDelete(const KUrl &path)
{
  // We have to create a new command class manually to set some parameter
  FtpCommandDelete *del = new FtpCommandDelete(this);
  del->destinationPath = path;
  m_cmdData = del;
  m_cmdData->process();
}

// *******************************************************************************************
// ***************************************** CHMOD *******************************************
// *******************************************************************************************

class FtpCommandRecursiveChmod : public Commands::Base {
public:
    enum State {
      None,
      VerifyDir,
      SimpleChmod,
      SentList,
      ProcessList,
      ChmodedDir,
      ChmodedFile
    };
    
    ENGINE_STANDARD_COMMAND_CONSTRUCTOR(FtpCommandRecursiveChmod, Socket, CmdChmod)
    
    QList<DirectoryEntry> currentList;
    QList<DirectoryEntry>::const_iterator currentEntry;
    
    KUrl destinationPath;
    int mode;
    
    void process()
    {
      switch (currentState) {
        case None: {
          // We have to determine if the destination is a file or a directory
          if (socket()->isChained()) {
            // We know that it is a directory
            currentState = SentList;
            socket()->protoList(destinationPath);
          } else {
            currentState = VerifyDir;
            socket()->protoStat(destinationPath);
          }
          break;
        }
        case VerifyDir: {
          DirectoryEntry entry = socket()->getStatResponse();
          
          if (entry.filename().isEmpty()) {
            // The file doesn't exist, abort
            socket()->resetCommandClass(Failed);
          } else {
            if (entry.isDirectory()) {
              // It is a directory, chmod recursively
              currentState = SentList;
              socket()->protoList(destinationPath);
            } else {
              // A single file, a simple chmod
              currentState = SimpleChmod;
              socket()->protoChmodSingle(destinationPath, mode);
            }
          }          
          break;
        }
        case SimpleChmod: {
          socket()->resetCommandClass();
          break;
        }
        case SentList: {
          currentList = socket()->getLastDirectoryListing().list();
          currentEntry = currentList.constBegin();
          currentState = ProcessList;
          
          // Empty listing, we are done
          if (currentEntry == currentList.constEnd()) {
            if (socket()->isChained())
              socket()->resetCommandClass();
            else {
              // We are the top level command class, chmod the destination dir
              currentState = SimpleChmod;
              socket()->protoChmodSingle(destinationPath, mode);
            }
            
            return;
          }
        }
        case ProcessList: {
          KUrl childPath = destinationPath;
          childPath.addPath((*currentEntry).filename());
          
          if ((*currentEntry).isDirectory()) {
            // A directory, chain another recursive chmod command
            currentState = ChmodedDir;
            
            // Chain manually, since we need to set some parameters
            FtpCommandRecursiveChmod *cm = new FtpCommandRecursiveChmod(socket());
            cm->destinationPath = childPath;
            cm->mode = mode;
            socket()->addToCommandChain(cm);
            socket()->nextCommand();
          } else {
            // A file entry - remove
            currentState = ChmodedFile;
            socket()->protoChmodSingle(childPath, mode);
          }
          break;
        }
        case ChmodedDir: {
          // We have to chmod the directory
          KUrl childPath = destinationPath;
          childPath.addPath((*currentEntry).filename());
          
          currentState = ChmodedFile;
          socket()->protoChmodSingle(childPath, mode);
          break;
        }
        case ChmodedFile: {
          currentState = ProcessList;
          
          if (++currentEntry == currentList.constEnd()) {
            if (socket()->isChained())
              socket()->resetCommandClass();
            else {
              // We are the top level command class, chmod the destination dir
              currentState = SimpleChmod;
              socket()->protoChmodSingle(destinationPath, mode);
            }
          } else
            socket()->nextCommand();
          break;
        }
      }
    }
};

void Socket::protoChmod(const KUrl &path, int mode, bool recursive)
{
  if (recursive) {
    // We have to create a new command class manually to set some parameters
    FtpCommandRecursiveChmod *cm = new FtpCommandRecursiveChmod(this);
    cm->destinationPath = path;
    cm->mode = mode;
    m_cmdData = cm;
    m_cmdData->process();
  } else {
    // No recursive, just chmod a single file
    protoChmodSingle(path, mode);
  }
}

}
