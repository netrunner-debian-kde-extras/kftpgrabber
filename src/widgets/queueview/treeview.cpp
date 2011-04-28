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
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
#include "queueview/treeview.h"
#include "queueview/model.h"
#include "queueview/delegate.h"
#include "kftpqueue.h"

#include <QEvent>
#include <QMenu>
#include <QContextMenuEvent>

#include <KLocale>
#include <KIcon>
#include <KInputDialog>
#include <KMessageBox>

using namespace KFTPQueue;

namespace KFTPWidgets {

namespace Queue {

TreeView::TreeView(QWidget* parent)
  : QTreeView(parent)
{
  setAcceptDrops(true);
  setDragEnabled(true);
  setUniformRowHeights(true);
  setSelectionMode(QAbstractItemView::SingleSelection);
  setEditTriggers(QAbstractItemView::NoEditTriggers);
  setSortingEnabled(true);
  setFrameStyle(QFrame::NoFrame);
  setDragDropMode(QAbstractItemView::InternalMove);
  setDropIndicatorShown(true);
  setAutoExpandDelay(300);
  setRootIsDecorated(true);
  
  Delegate *delegate = new Delegate(this);
  setItemDelegate(delegate);

  viewport()->setAttribute(Qt::WA_Hover);
}

void TreeView::setModel(QAbstractItemModel *model)
{
  QTreeView::setModel(model);
  
  // Ensure that sites are automaticly expanded
  connect(model, SIGNAL(rowsInserted(const QModelIndex&, int, int)), this, SLOT(expandSite(const QModelIndex&)));
}

bool TreeView::event(QEvent* event)
{
  if (event->type() == QEvent::Polish) {
    setColumnWidth(Model::Name, 150);
    setColumnWidth(Model::Size, 75);
    setColumnWidth(Model::Source, 250);
    setColumnWidth(Model::Destination, 250);
    setColumnWidth(Model::Progress, 140);
    setColumnWidth(Model::Speed, 70);
    setColumnWidth(Model::ETA, 80);
  }

  return QTreeView::event(event);
}

void TreeView::expandSite(const QModelIndex &parent)
{
  if (!parent.isValid())
    return;
  
  QueueObject *object = parent.data(Model::ObjectRole).value<QueueObject*>();
  
  if (object->isSite()) {
    // Expand the site
    if (!isExpanded(parent))
      setExpanded(parent, true);
  }
}

void TreeView::contextMenuEvent(QContextMenuEvent *event)
{
  m_currentIndex = indexAt(event->pos());
  if (!m_currentIndex.isValid())
    return;
  
  QueueObject *object = m_currentIndex.data(Model::ObjectRole).value<QueueObject*>();
  QueueObject *parent = object->parentObject();
  
  // Show a context menu
  QMenu menu(this);
  
  if (object->isLocked()) {
    menu.addAction(KIcon("process-stop"), i18n("&Abort Directory Scan"), this, SLOT(slotAbortTransfer()));
  } else if (object->isRunning()) {
    menu.addAction(KIcon("process-stop"), i18n("&Abort Transfer"), this, SLOT(slotAbortTransfer()));
  } else {
    menu.addAction(KIcon("launch"), i18n("&Start Transfer"), this, SLOT(slotStartTransfer()));
  }
  
  if (object->isTransfer()) {
    menu.addAction(KIcon("edit-delete"), i18n("&Remove"), this, SLOT(slotRemoveTransfer()));
  }
  
  menu.addAction(i18n("R&emove All"), this, SLOT(slotRemoveAll()));
  
  menu.addSeparator();
  
  if (parent->canMoveChildUp(object)) {
    menu.addAction(KIcon("go-up"), i18n("Move &Up"), this, SLOT(slotMoveUp()));
    menu.addAction(KIcon("go-top"), i18n("Move To &Top"), this, SLOT(slotMoveTop()));
  }
  
  if (parent->canMoveChildDown(object)) {
    menu.addAction(KIcon("go-down"), i18n("Move &Down"), this, SLOT(slotMoveDown()));
    menu.addAction(KIcon("go-bottom"), i18n("Move To &Bottom"), this, SLOT(slotMoveBottom()));
  }
  
  menu.exec(event->globalPos());
}

void TreeView::slotStartTransfer()
{
  // Reset a possible preconfigured default action
  Manager::self()->setDefaultFileExistsAction();
  
  QueueObject *object = m_currentIndex.data(Model::ObjectRole).value<QueueObject*>();
  object->execute();
}

void TreeView::slotAbortTransfer()
{
  QueueObject *object = m_currentIndex.data(Model::ObjectRole).value<QueueObject*>();
  object->abort();
}

void TreeView::slotRemoveTransfer()
{
  if (KMessageBox::warningYesNo(this, i18n("Are you sure you wish to remove this transfer?"), i18n("Remove Transfer")) == KMessageBox::No)
    return;
  
  QueueObject *object = m_currentIndex.data(Model::ObjectRole).value<QueueObject*>();
  Manager::self()->removeTransfer(static_cast<Transfer*>(object));
}

void TreeView::slotRemoveAll()
{
  if (KMessageBox::warningYesNo(this, i18n("Are you sure you wish to clear the transfer queue?"), i18n("Clear Queue")) == KMessageBox::No)
    return;
  
  Manager::self()->clearQueue();
}

void TreeView::slotMoveUp()
{
  QueueObject *object = m_currentIndex.data(Model::ObjectRole).value<QueueObject*>();
  QueueObject *parent = object->parentObject();
  parent->moveChildUp(object);
  setCurrentIndex(model()->index(object->index(), 0, m_currentIndex.parent()));
}

void TreeView::slotMoveDown()
{
  QueueObject *object = m_currentIndex.data(Model::ObjectRole).value<QueueObject*>();
  QueueObject *parent = object->parentObject();
  parent->moveChildDown(object);
  
  m_currentIndex = model()->index(object->index(), 0, m_currentIndex.parent());
  setCurrentIndex(m_currentIndex);
  scrollTo(m_currentIndex);
}

void TreeView::slotMoveTop()
{
  QueueObject *object = m_currentIndex.data(Model::ObjectRole).value<QueueObject*>();
  QueueObject *parent = object->parentObject();
  parent->moveChildTop(object);
  
  m_currentIndex = model()->index(object->index(), 0, m_currentIndex.parent());
  setCurrentIndex(m_currentIndex);
  scrollTo(m_currentIndex);
}

void TreeView::slotMoveBottom()
{
  QueueObject *object = m_currentIndex.data(Model::ObjectRole).value<QueueObject*>();
  QueueObject *parent = object->parentObject();
  parent->moveChildBottom(object);
  
  m_currentIndex = model()->index(object->index(), 0, m_currentIndex.parent());
  setCurrentIndex(m_currentIndex);
  scrollTo(m_currentIndex);
}

}

}

#include "treeview.moc"
