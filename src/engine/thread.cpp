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
#include "thread.h"
#include "ftpsocket.h"
#include "sftpsocket.h"
#include "settings.h"

#include <QAbstractEventDispatcher>
#include <QCoreApplication>

namespace KFTPEngine {

CommandQueue::Event::Event(Commands::Type type)
  : QEvent((QEvent::Type) 65234),
    m_type(type)
{
}

CommandQueue::CommandQueue(Thread *thread)
  : QObject(),
    m_thread(thread)
{
}

void CommandQueue::customEvent(QEvent *event)
{
  Event *e = static_cast<Event*>(event);
  Socket *socket = m_thread->socket();
  
  if (!socket->isBusy()) {
    socket->setCurrentCommand(e->command());
    
    switch (e->command()) {
      case Commands::CmdConnect: {
        KUrl url = e->parameter(0).value<KUrl>();
        
        m_thread->setCurrentProtocol(url);
        m_thread->socket()->setCurrentCommand(e->command());
        m_thread->socket()->protoConnect(url);
        break;
      }
      case Commands::CmdDisconnect: {
        socket->protoDisconnect();
        break;
      }
      case Commands::CmdList: {
        socket->protoList(e->parameter(0).value<KUrl>());
        break;
      }
      case Commands::CmdScan: {
        socket->protoScan(e->parameter(0).value<KUrl>());
        break;
      }
      case Commands::CmdGet: {
        socket->protoGet(e->parameter(0).value<KUrl>(),
                         e->parameter(1).value<KUrl>());
        break;
      }
      case Commands::CmdPut: {
        socket->protoPut(e->parameter(0).value<KUrl>(),
                         e->parameter(1).value<KUrl>());
        break;
      }
      case Commands::CmdDelete: {
        socket->protoDelete(e->parameter(0).value<KUrl>());
        break;
      }
      case Commands::CmdRename: {
        socket->protoRename(e->parameter(0).value<KUrl>(),
                            e->parameter(1).value<KUrl>());
        break;
      }
      case Commands::CmdChmod: {
        socket->protoChmod(e->parameter(0).value<KUrl>(),
                           e->parameter(1).toULongLong(),
                           e->parameter(2).toBool());
        break;
      }
      case Commands::CmdMkdir: {
        socket->protoMkdir(e->parameter(0).value<KUrl>());
        break;
      }
      case Commands::CmdRaw: {
        socket->protoRaw(e->parameter(0).toString());
        break;
      }
      case Commands::CmdFxp: {
        socket->protoSiteToSite(e->parameter(0).value<Socket*>(),
                                e->parameter(1).value<KUrl>(),
                                e->parameter(2).value<KUrl>());
        break;
      }
      default: break;
    }
  }
  
  switch (e->command()) {
    case Commands::CmdWakeup: {
      socket->wakeup(e->parameter(0).value<WakeupEvent*>());
      break;
    }
    case Commands::CmdAbort: {
      socket->protoAbort();
      break;
    }
    default: break;
  }
}

Thread::Thread()
 : QThread(),
   m_eventHandler(new EventHandler(this)),
   m_socket(0),
   m_exit(false),
   m_commandsDeferred(0),
   m_immediateWakeup(false)
{
  m_startupSem.release(1);
  m_startupSem.acquire();
  
  // Auto start the thread
  start();
  
  m_startupSem.acquire();
}

Thread::~Thread()
{
  delete m_eventHandler;
}

void Thread::shutdown()
{
  m_commandsDeferred = 0;
  m_exit = true;
  QAbstractEventDispatcher::instance(this)->interrupt();

  // Start a timer to prevent the thread from hanging
  QTimer::singleShot(5000, this, SLOT(shutdownWithTerminate()));
}

void Thread::shutdownWithTerminate()
{
  terminate();
  deleteLater();
}

void Thread::run()
{
  m_commandQueue = new CommandQueue(this);
  m_settings = new Settings();
  setCurrentProtocol("ftp");
  m_startupSem.release();
  
  // Enter the event loop
  QEventLoop eventLoop;
  while (!m_exit) {
    if (m_commandsDeferred)
      eventLoop.processEvents(QEventLoop::ProcessEventsFlag(QEventLoop::DeferredDeletion));
    else
      eventLoop.processEvents(QEventLoop::WaitForMoreEvents | QEventLoop::ProcessEventsFlag(QEventLoop::DeferredDeletion));
    
    if (m_commandsDeferred) {
      m_socket->nextCommand();
      m_commandsDeferred--;
    }
  }
  
  // Cleanup before exiting
  delete m_settings;
  delete m_commandQueue;
  
  foreach (Socket *socket, m_sockets) {
    delete socket;
  }

  // Schedule our deletion
  deleteLater();
}

void Thread::deferCommandExec()
{
  m_commandsDeferred++;
  QAbstractEventDispatcher::instance(this)->interrupt();
}

void Thread::setCurrentProtocol(const QString &protocol)
{
  if (m_socket && m_socket->protocolName() == protocol)
    return;
  
  if (!m_sockets.contains(protocol)) {
    if (protocol == "ftp")
      m_socket = new FtpSocket(this);
    else if (protocol == "sftp")
      m_socket = new SftpSocket(this);
    else
      Q_ASSERT(false);
    
    m_sockets.insert(protocol, m_socket);
  } else {
    m_socket = m_sockets[protocol];
  }
}

void Thread::setCurrentProtocol(const KUrl &url)
{
  setCurrentProtocol(url.protocol());
}

void Thread::notifyCommandQueue(CommandQueue::Event *event, int priority)
{
  if (m_commandQueue)
    QCoreApplication::postEvent(m_commandQueue, event, priority);
}

void Thread::wakeup(WakeupEvent *e)
{
  if (m_immediateWakeup) {
    m_imWakeupEvent = e;
    m_immediateWakeup = false;
    m_imWakeupCond.wakeOne();
  } else {
    CommandQueue::Event *event = new CommandQueue::Event(Commands::CmdWakeup);
    event->addParameter(QVariant::fromValue(e));
    
    notifyCommandQueue(event);
  }
}

void Thread::abort()
{
  if (m_immediateWakeup) {
    m_imWakeupEvent = 0;
    m_immediateWakeup = false;
    m_imWakeupCond.wakeOne();
  }
  
  notifyCommandQueue(new CommandQueue::Event(Commands::CmdAbort), Qt::HighEventPriority * 100);
}

void Thread::emitEvent(Event::Type type, QList<QVariant> params)
{
  if (m_eventHandler)
    QCoreApplication::postEvent(m_eventHandler, new Event(type, params));
}

WakeupEvent *Thread::waitForWakeup()
{
  WakeupEvent *event;
  
  m_imWakeupMutex.lock();
  m_immediateWakeup = true;
  m_imWakeupCond.wait(&m_imWakeupMutex);
  
  event = m_imWakeupEvent;
  m_imWakeupEvent = 0;
  
  m_imWakeupMutex.unlock();
  
  return event;
}

void Thread::connect(const KUrl &url)
{
  CommandQueue::Event *event = new CommandQueue::Event(Commands::CmdConnect);
  event->addParameter(url);
  
  notifyCommandQueue(event);
}

void Thread::disconnect()
{
  notifyCommandQueue(new CommandQueue::Event(Commands::CmdDisconnect));
}

void Thread::list(const KUrl &url)
{
  CommandQueue::Event *event = new CommandQueue::Event(Commands::CmdList);
  event->addParameter(url);
  
  notifyCommandQueue(event);
}

void Thread::scan(const KUrl &url)
{
  CommandQueue::Event *event = new CommandQueue::Event(Commands::CmdScan);
  event->addParameter(url);
  
  notifyCommandQueue(event);
}

void Thread::get(const KUrl &source, const KUrl &destination)
{
  CommandQueue::Event *event = new CommandQueue::Event(Commands::CmdGet);
  event->addParameter(source);
  event->addParameter(destination);
  
  notifyCommandQueue(event);
}

void Thread::put(const KUrl &source, const KUrl &destination)
{
  CommandQueue::Event *event = new CommandQueue::Event(Commands::CmdPut);
  event->addParameter(source);
  event->addParameter(destination);
  
  notifyCommandQueue(event);
}

void Thread::remove(const KUrl &url)
{
  CommandQueue::Event *event = new CommandQueue::Event(Commands::CmdDelete);
  event->addParameter(url);
  
  notifyCommandQueue(event);
}

void Thread::rename(const KUrl &source, const KUrl &destination)
{
  CommandQueue::Event *event = new CommandQueue::Event(Commands::CmdRename);
  event->addParameter(source);
  event->addParameter(destination);
  
  notifyCommandQueue(event);
}

void Thread::chmod(const KUrl &url, int mode, bool recursive)
{
  CommandQueue::Event *event = new CommandQueue::Event(Commands::CmdChmod);
  event->addParameter(url);
  event->addParameter(mode);
  event->addParameter(recursive);
  
  notifyCommandQueue(event);
}

void Thread::mkdir(const KUrl &url)
{
  CommandQueue::Event *event = new CommandQueue::Event(Commands::CmdMkdir);
  event->addParameter(url);
  
  notifyCommandQueue(event);
}

void Thread::raw(const QString &raw)
{
  CommandQueue::Event *event = new CommandQueue::Event(Commands::CmdRaw);
  event->addParameter(raw);
  
  notifyCommandQueue(event);
}

void Thread::siteToSite(Thread *thread, const KUrl &source, const KUrl &destination)
{
  CommandQueue::Event *event = new CommandQueue::Event(Commands::CmdFxp);
  event->addParameter(QVariant::fromValue(thread->socket()));
  event->addParameter(source);
  event->addParameter(destination);
  
  notifyCommandQueue(event);
}

}
