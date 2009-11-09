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
#include "threadview.h"

#include "listview.h"

#include <qlayout.h>
//Added by qt3to4:
#include <Q3VBoxLayout>
#include <Q3PtrList>
#include <klocale.h>

namespace KFTPWidgets {

ThreadViewItem::ThreadViewItem(KFTPSession::Session *session, Q3ListView *parent)
  : QObject(parent),
    Q3ListViewItem(parent),
    m_connection(0),
    m_session(session)
{
  refresh();
}

ThreadViewItem::ThreadViewItem(KFTPSession::Connection *conn, Q3ListViewItem *parent, int id)
  : QObject(),
    Q3ListViewItem(parent),
    m_id(id),
    m_connection(conn),
    m_session(0)
{
  connect(conn, SIGNAL(connectionRemoved()), this, SLOT(slotUpdateItemRequested()));
  connect(conn, SIGNAL(connectionLost(KFTPSession::Connection*)), this, SLOT(slotUpdateItemRequested()));
  connect(conn, SIGNAL(connectionEstablished()), this, SLOT(slotUpdateItemRequested()));
  
  // Connect the transfer signals if the transfer is already present
  KFTPQueue::Transfer *transfer = m_connection->getTransfer();
  if (transfer) {
    connect(transfer, SIGNAL(objectUpdated()), this, SLOT(slotUpdateItemRequested()));
  } else {
    connect(conn, SIGNAL(connectionAcquired()), this, SLOT(slotConnectionAcquired()));
  }
  
  refresh();
}

void ThreadViewItem::slotConnectionAcquired()
{
  if (!m_connection->getTransfer())
    return;
    
  connect(m_connection->getTransfer(), SIGNAL(objectUpdated()), this, SLOT(slotUpdateItemRequested()));
  refresh();
}

void ThreadViewItem::refresh()
{
/*
  if (m_session) {
    // Set the columns
    setText(0, i18n("Site session [%1]").arg(m_session->getClient()->socket()->getCurrentUrl().host()));
    setPixmap(0, loadSmallPixmap("ftp"));
  } else if (m_connection) {
    setText(0, i18n("Thread %1").arg(m_id));
    setPixmap(0, loadSmallPixmap("server"));
    setText(1, m_connection->isConnected() ? i18n("idle") : i18n("disconnected"));
    setText(2, "");
    
    KFTPQueue::Transfer *transfer = m_connection->getTransfer();
    if (transfer && transfer->isRunning()) {
      QString speed;
      filesize_t rawSpeed = transfer->getSpeed();
      
      speed.sprintf( "%lld KB/s", (rawSpeed / 1024) );
      
      if (rawSpeed > 1024*1024)
        speed.sprintf("%lld MB/s", (rawSpeed / 1024) / 1024);
      else if (rawSpeed == 0)
        speed = "";
    
      if (transfer->getStatus() == KFTPQueue::Transfer::Connecting) {
        setText(1, i18n("connecting"));
      } else {
        setText(1, i18n("transferring"));
      }
      
      if (transfer->getTransferType() == KFTPQueue::FXP && rawSpeed == 0) {
        KFTPSession::Connection *c = static_cast<KFTPQueue::TransferFile*>(transfer)->getOppositeConnection(m_connection);
        
        setText(2, i18n("FXP - [%1]").arg(c->getUrl().host()));
      } else {
        setText(2, speed);
      }
    }
  }
  */
}

void ThreadViewItem::paintCell(QPainter *p, const QColorGroup &cg, int column, int width, int alignment)
{
  Q3ListViewItem::paintCell(p, cg, column, width, alignment);
}

void ThreadViewItem::slotUpdateItemRequested()
{
  refresh();
}

ThreadView::ThreadView(QWidget *parent, const char *name)
 : QWidget(parent, name)
{
  Q3VBoxLayout *layout = new Q3VBoxLayout(this);

  // Create the list view
  m_threads = new KFTPWidgets::ListView(this);
  
  // Create the columns
  m_threads->addColumn(i18n("Name"), 400);
  m_threads->addColumn(i18n("Status"), 120);
  m_threads->addColumn(i18n("Speed"), 70);
  
  // Text when there are no threads
  m_threads->setEmptyListText(i18n("There are no threads currently running."));

  // Multi-select
  m_threads->setSelectionModeExt(K3ListView::FileManager);
  m_threads->setAllColumnsShowFocus(true);
  m_threads->setRootIsDecorated(true);

  layout->addWidget(m_threads);
  
  connect(KFTPSession::Manager::self(), SIGNAL(update()), this, SLOT(slotUpdateSessions()));
}

ThreadView::~ThreadView()
{
}

void ThreadView::slotUpdateSessions()
{
  /*
  KFTPSession::SessionList *list = KFTPSession::Manager::self()->getSessionList();
  KFTPSession::Session *i;
  
  m_threads->clear();
  
  for (i = list->first(); i; i = list->next()) {
    if (i->isRemote()) {
      ThreadViewItem *site = new ThreadViewItem(i, m_threads);
      
      Q3PtrList<KFTPSession::Connection> *c_list = i->getConnectionList();
      
      if (c_list->count() > 0) {
        KFTPSession::Connection *conn;
        int id = 0;
        
        for (conn = c_list->first(); conn; conn = c_list->next()) {
          new ThreadViewItem(conn, site, ++id);
        }
        
        site->setOpen(true);
      }
    }
  }
  */
}

}

#include "threadview.moc"

