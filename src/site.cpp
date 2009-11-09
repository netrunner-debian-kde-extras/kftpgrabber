/*
 * This file is part of the KFTPGrabber project
 *
 * Copyright (C) 2003-2005 by the KFTPGrabber developers
 * Copyright (C) 2003-2005 Jernej Kos <kostko@jweb-network.net>
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
#include "site.h"
#include "queuegroup.h"
#include "kftpqueue.h"

namespace KFTPQueue {

Site::Site(QueueObject *parent, KUrl url)
 : QueueObject(parent, QueueObject::Site),
   m_group(new QueueGroup(this))
{
  url.setPath("/");
  m_siteUrl = url;
  
  // Connect to some group signals
  connect(m_group, SIGNAL(interrupted()), this, SLOT(slotGroupInterrupted()));
}

void Site::execute()
{
  m_completed = 0;
  m_resumed = 0;
  m_status = Running;
  
  // Reset and start the group
  m_group->reset();
  m_group->executeNextTransfer();
}

void Site::abort()
{
  // If not running, just return
  if (!isRunning())
    return;
  
  // Set the aborting flag
  m_aborting = true;
  emit siteAborted();
  
  // Signal abort to all child transfers
  foreach (QueueObject *i, m_children) {
    if (i->isRunning() && !i->isAborting())
      i->abort();
  }
  
  // Clear all the stuff
  m_status = Stopped;
  m_resumed = 0;
  m_completed = 0;
  m_aborting = false;
  m_size = m_actualSize;
  
  emit objectUpdated();
}

void Site::slotGroupInterrupted()
{
  if (!m_aborting)
    abort();
}

}
#include "site.moc"
