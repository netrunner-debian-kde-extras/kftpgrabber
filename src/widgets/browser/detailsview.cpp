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
#include "browser/detailsview.h"
#include "browser/view.h"

#include <QHeaderView>
#include <QEvent>
#include <QDropEvent>
#include <QScrollBar>

#include <kfileitemdelegate.h>

namespace KFTPWidgets {

namespace Browser {

DetailsView::DetailsView(QWidget *parent, View *view)
  : QTreeView(parent),
    m_view(view)
{
  setAcceptDrops(true);
  setRootIsDecorated(false);
  setSortingEnabled(true);
  setUniformRowHeights(true);
  setSelectionBehavior(SelectItems);
  setDragDropMode(QAbstractItemView::DragDrop);
  setDropIndicatorShown(false);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  
  viewport()->setAttribute(Qt::WA_Hover);
  
  m_viewOptions = QTreeView::viewOptions();
  
  KFileItemDelegate *delegate = new KFileItemDelegate(this);
  setItemDelegate(delegate);
  
  connect(this, SIGNAL(doubleClicked(const QModelIndex&)), view, SLOT(openIndex(const QModelIndex&)));
}

DetailsView::~DetailsView()
{
}

bool DetailsView::event(QEvent* event)
{
  if (event->type() == QEvent::Polish) {
    // Assure that by respecting the available width that:
    // - the 'Name' column is stretched as large as possible
    // - the remaining columns are as small as possible
    QHeaderView *headerView = header();
    headerView->setStretchLastSection(false);
    headerView->setResizeMode(QHeaderView::ResizeToContents);
    //headerView->setResizeMode(0, QHeaderView::Stretch);

    // hide columns if this is indicated by the settings
    /*
    const DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
    Q_ASSERT(settings != 0);
    if (!settings->showDate()) {
        hideColumn(KDirModel::ModifiedTime);
    }

    if (!settings->showPermissions()) {
        hideColumn(KDirModel::Permissions);
    }

    if (!settings->showOwner()) {
        hideColumn(KDirModel::Owner);
    }

    if (!settings->showGroup()) {
        hideColumn(KDirModel::Group);
    }

    if (!settings->showType()) {
        hideColumn(KDirModel::Type);
    }
    */
  }

  return QTreeView::event(event);
}

void DetailsView::contextMenuEvent(QContextMenuEvent* event)
{
  m_view->openContextMenu(selectedIndexes(), event->globalPos());
}

void DetailsView::mouseReleaseEvent(QMouseEvent* event)
{
  QTreeView::mouseReleaseEvent(event);
  // TODO
  //m_controller->triggerActivation();
}

void DetailsView::dragEnterEvent(QDragEnterEvent* event)
{
  if (event->mimeData()->hasUrls()) {
    event->acceptProposedAction();
  }
}

void DetailsView::dropEvent(QDropEvent* event)
{
  const KUrl::List urls = KUrl::List::fromMimeData(event->mimeData());
  
  if (!urls.isEmpty()) {
    event->acceptProposedAction();
    // TODO
  }
  
  QTreeView::dropEvent(event);
}

void DetailsView::scrollContentsBy(int dx, int dy)
{
  QTreeView::scrollContentsBy(dx, dy);
  
  const int x = horizontalScrollBar()->value();
  const int y = verticalScrollBar()->value();
  
  emit contentsMoved(x, y);
}

}

}

#include "detailsview.moc"
