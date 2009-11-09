/*
 * This file is part of the KFTPGrabber project
 *
 * Copyright (C) 2003-2004 by the KFTPGrabber developers
 * Copyright (C) 2003-2004 Jernej Kos <kostko@jweb-network.net>
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
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#include "zeroconflistview.h"
#include "kftpsession.h"
#include "misc/zeroconf.h"

#include <klocale.h>
#include <kiconloader.h>

#include <QList>

namespace KFTPWidgets {

ZeroConfListView::ZeroConfListView(QWidget *parent)
 : ListView(parent)
{
  // Set some stuff
  setMinimumWidth(150);
  setFullWidth(true);
  addColumn(i18n("Sites Near You"));
  setRootIsDecorated(true);
  setEmptyListText(i18n("No sites published."));
  setItemsRenameable(false);
  
  connect(KFTPCore::ZeroConf::self(), SIGNAL(servicesUpdated()), this, SLOT(slotSitesChanged()));
  connect(this, SIGNAL(executed(Q3ListViewItem*)), this, SLOT(slotSiteExecuted(Q3ListViewItem*)));
  
  slotSitesChanged();
}

void ZeroConfListView::slotSitesChanged()
{
  // Update the site list
  QList<DNSSD::RemoteService::Ptr> list = KFTPCore::ZeroConf::self()->getServiceList();
  clear();
  
  if (!list.isEmpty()) {
    QList<DNSSD::RemoteService::Ptr>::iterator end(list.end());
    
    for (QList<DNSSD::RemoteService::Ptr>::iterator i(list.begin()); i != end; ++i) {
      QString name = (*i)->serviceName();
      QString ip = (*i)->hostName();
      QString port = QString::number((*i)->port());
      
      K3ListViewItem *site = new K3ListViewItem(this, name, ip, port);
      site->setPixmap(0, SmallIcon("lan"));
    }
  }
}

void ZeroConfListView::slotSiteExecuted(Q3ListViewItem *item)
{
  // Connect to the site
  //KFTPAPI::getInstance()->mainWindow()->slotQuickConnect(item->text(0), item->text(1), item->text(2).toInt());
}

}

#include "zeroconflistview.moc"
