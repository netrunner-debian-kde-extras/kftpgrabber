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
#include "zeroconf.h"

#include <KGlobal>

namespace KFTPCore {

class ZeroConfPrivate {
public:
    ZeroConf instance;
};

K_GLOBAL_STATIC(ZeroConfPrivate, zeroConfPrivate)

ZeroConf *ZeroConf::self()
{
  return &zeroConfPrivate->instance;
}

ZeroConf::ZeroConf()
 : QObject()
{
  m_browser = new DNSSD::ServiceBrowser("_ftp._tcp", true);
  
  connect(m_browser, SIGNAL(finished()), this, SLOT(slotServiceChanged()));
  m_browser->startBrowse();
}


ZeroConf::~ZeroConf()
{
}

QList<DNSSD::RemoteService::Ptr> ZeroConf::getServiceList() const
{
  return m_browser->services();
}

void ZeroConf::slotServiceChanged()
{
  emit servicesUpdated();
}

}

#include "zeroconf.moc"
