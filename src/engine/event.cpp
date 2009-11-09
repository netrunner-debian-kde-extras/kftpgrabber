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

#include "event.h"
#include "thread.h"

namespace KFTPEngine {

Event::Event(Type type, QList<QVariant> params)
  : QEvent((QEvent::Type) 65123),
    m_type(type),
    m_params(params)
{
}

Event::~Event()
{
}

EventHandler::EventHandler(Thread *thread)
  : QObject(),
    m_thread(thread)
{
}

void EventHandler::customEvent(QEvent *e)
{
  if (e->type() == 65123) {
    Event *ev = static_cast<Event*>(e);

    emit engineEvent(ev);
    
    switch (ev->type()) {
      case Event::EventConnect: emit connected(); break;
      case Event::EventDisconnect: emit disconnected(); break;
      case Event::EventResponse:
      case Event::EventMultiline: {
        emit gotResponse(ev->getParameter(0).toString());
        break;
      }
      case Event::EventRaw: emit gotRawResponse(ev->getParameter(0).toString()); break;
      default: break;
    }
  }
}

}

#include "event.moc"
