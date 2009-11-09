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

#include "commands.h"

namespace KFTPEngine {

namespace Commands {

Base::Base(Socket *socket, Type command)
  : m_command(command),
    m_socket(socket),
    m_wakeupEvent(0),
    m_processing(false),
    m_autoDestruct(false),
    m_clean(false)
{
}

void Base::setProcessing(bool value)
{
  if (value)
    m_processing++;
  else
    m_processing--;
}

void Base::autoDestruct(ResetCode code)
{
  m_autoDestruct = true;
  m_resetCode = code;
}

void Base::wakeup(WakeupEvent *event)
{
  // The default implementation just calls process()
  m_wakeupEvent = event;
  process();
  m_wakeupEvent = 0;
}

}

}

