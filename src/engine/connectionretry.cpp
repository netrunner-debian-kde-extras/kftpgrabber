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
#include "connectionretry.h"
#include "socket.h"
#include "thread.h"
#include "event.h"

#include <KLocale>

namespace KFTPEngine {

ConnectionRetry::ConnectionRetry(Socket *socket)
  : QObject(),
    m_socket(socket),
    m_delay(socket->getConfig<int>("retry_delay")),
    m_max(socket->getConfig<int>("max_retries")),
    m_iteration(0)
{
  m_timer = new QTimer(this);
  
  connect(m_timer, SIGNAL(timeout()), this, SLOT(slotShouldRetry()));
  connect(m_socket->thread()->eventHandler(), SIGNAL(engineEvent(KFTPEngine::Event*)), this, SLOT(slotEngineEvent(KFTPEngine::Event*)));
}

void ConnectionRetry::startRetry()
{
  if ((m_iteration++ >= m_max && m_max != 0) || m_delay < 1) {
    abortRetry();
    return;
  }
  
  m_socket->setCurrentCommand(Commands::CmdConnectRetry);
  m_socket->emitEvent(Event::EventMessage, i18np("Waiting 1 second before reconnect...", "Waiting %1 seconds before reconnect...",m_delay));
  m_socket->emitEvent(Event::EventState, i18n("Waiting..."));
  
  m_timer->setSingleShot(true);
  m_timer->start(1000 * m_delay);
}

void ConnectionRetry::slotShouldRetry()
{
  m_socket->setCurrentCommand(Commands::CmdNone);
  if (m_max > 0)
    m_socket->emitEvent(Event::EventMessage, i18n("Retrying connection (%1/%2)...",m_iteration,m_max));
  else
    m_socket->emitEvent(Event::EventMessage, i18n("Retrying connection...",m_iteration,m_max));
  
  // Reconnect
  Thread *thread = m_socket->thread();
  thread->connect(m_socket->getCurrentUrl());
}

void ConnectionRetry::abortRetry()
{
  m_timer->stop();
  
  // Disable retry so we avoid infinite loops
  m_socket->setConfig("retry", 0);
  
  m_socket->setCurrentCommand(Commands::CmdNone);
  m_socket->emitEvent(Event::EventMessage, i18n("Retry aborted."));
  m_socket->emitEvent(Event::EventState, i18n("Idle."));
  m_socket->emitEvent(Event::EventReady);
  m_socket->emitError(ConnectFailed);
  
  // This object should be automagicly removed
  QObject::deleteLater();
}

void ConnectionRetry::slotEngineEvent(KFTPEngine::Event *event)
{
  if (event->type() == Event::EventConnect) {
    m_socket->emitEvent(Event::EventRetrySuccess);
    
    // This object should be automagicly removed
    QObject::deleteLater();
  }
}

}

#include "connectionretry.moc"
