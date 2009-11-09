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

#include "browser/treeview.h"
#include "browser/view.h"
#include "browser/dirmodel.h"
#include "browser/dirlister.h"
#include "browser/dirsortfilterproxymodel.h"

#include <kfileitemdelegate.h>

#include <KDirModel>
#include <KDirLister>
#include <QContextMenuEvent>
#include <QDropEvent>
#include <QDragMoveEvent>
#include <QHeaderView>

namespace KFTPWidgets {

namespace Browser {

TreeView::TreeView(QWidget* parent, View *view)
  : QTreeView(parent),
    m_view(view),
    m_dragging(false)
{
  setAcceptDrops(true);
  setUniformRowHeights(true);
  setSelectionMode(QAbstractItemView::SingleSelection);
  setEditTriggers(QAbstractItemView::NoEditTriggers);
  setSortingEnabled(true);
  setFrameStyle(QFrame::NoFrame);
  setDragDropMode(QAbstractItemView::DragDrop);
  setDropIndicatorShown(false);
  setAutoExpandDelay(300);
  setRootIsDecorated(false);

  viewport()->setAttribute(Qt::WA_Hover);
  
  m_dirModel = new DirModel(view->dirLister(), this);
  m_dirModel->setDropsAllowed(DirModel::DropOnDirectory);
  m_dirModel->setTreeViewBehavior(true);
  
  m_proxyModel = new DirSortFilterProxyModel(this);
  m_proxyModel->setSourceModel(m_dirModel);
  
  setModel(m_proxyModel);

  KFileItemDelegate *delegate = new KFileItemDelegate(this);
  setItemDelegate(delegate);
  
  connect(this, SIGNAL(clicked(const QModelIndex&)), view, SLOT(openIndex(const QModelIndex&)));
  connect(view->dirLister(), SIGNAL(completed()), this, SLOT(selectCurrent()));
}

void TreeView::selectCurrent()
{
  QModelIndex index = m_dirModel->indexForUrl(m_view->locationNavigator()->url());
  
  if (index.isValid()) {
    QModelIndex proxyIndex = m_proxyModel->mapFromSource(index);
    
    if (!selectionModel()->isSelected(proxyIndex)) {
      setExpanded(proxyIndex, true);
      scrollTo(proxyIndex);
      
      selectionModel()->clearSelection();
      selectionModel()->setCurrentIndex(proxyIndex, QItemSelectionModel::Select);
    }
  }
}

void TreeView::contextMenuEvent(QContextMenuEvent *event)
{
  m_view->openContextMenu(selectedIndexes(), event->globalPos());
}

bool TreeView::event(QEvent* event)
{
  if (event->type() == QEvent::Polish) {
    // Hide all columns except of the 'Name' column
    hideColumn(DirModel::Size);
    hideColumn(DirModel::ModifiedTime);
    hideColumn(DirModel::Permissions);
    hideColumn(DirModel::Owner);
    hideColumn(DirModel::Group);
    header()->hide();
  }

  return QTreeView::event(event);
}

void TreeView::dragEnterEvent(QDragEnterEvent* event)
{
  if (event->mimeData()->hasUrls()) {
    event->acceptProposedAction();
  }
  
  QTreeView::dragEnterEvent(event);
  m_dragging = true;
}

void TreeView::dragLeaveEvent(QDragLeaveEvent* event)
{
  QTreeView::dragLeaveEvent(event);

  // TODO: remove this code when the issue #160611 is solved in Qt 4.4
  m_dragging = false;
  setDirtyRegion(m_dropRect);
}

void TreeView::dragMoveEvent(QDragMoveEvent* event)
{
  QTreeView::dragMoveEvent(event);

  // TODO: remove this code when the issue #160611 is solved in Qt 4.4
  const QModelIndex index = indexAt(event->pos());
  setDirtyRegion(m_dropRect);
  m_dropRect = visualRect(index);
  setDirtyRegion(m_dropRect);
}

void TreeView::dropEvent(QDropEvent* event)
{
  const KUrl::List urls = KUrl::List::fromMimeData(event->mimeData());
  
  if (urls.isEmpty()) {
    QTreeView::dropEvent(event);
  } else {
    event->acceptProposedAction();
    const QModelIndex index = indexAt(event->pos());
    
    if (index.isValid()) {
      emit urlsDropped(urls, index);
    }
  }
  
  m_dragging = false;
}

}

}

#include "treeview.moc"
