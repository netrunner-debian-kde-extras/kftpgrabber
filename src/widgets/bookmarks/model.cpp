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
#include "bookmarks/model.h"
#include "kftpbookmarks.h"

#include <QMimeData>

using namespace KFTPBookmarks;

namespace KFTPWidgets {

namespace Bookmarks {

Model::Model(KFTPBookmarks::Manager *bookmarks, QObject *parent)
  : QAbstractItemModel(parent),
    m_bookmarks(bookmarks)
{
  if (!m_bookmarks)
    m_bookmarks = Manager::self();
  
  connect(m_bookmarks, SIGNAL(siteAdded(KFTPBookmarks::Site*)), this, SLOT(slotSiteAdded(KFTPBookmarks::Site*)));
  connect(m_bookmarks, SIGNAL(siteRemoved(KFTPBookmarks::Site*)), this, SLOT(slotSiteRemoved(KFTPBookmarks::Site*)));
  connect(m_bookmarks, SIGNAL(siteChanged(KFTPBookmarks::Site*)), this, SLOT(slotSiteChanged(KFTPBookmarks::Site*)));
}

int Model::columnCount(const QModelIndex &parent) const
{
  Q_UNUSED(parent)
  return 1;
}
    
QVariant Model::data(const QModelIndex &index, int role) const
{
  if (index.isValid()) {
    Site *site = static_cast<Site*>(index.internalPointer());
    
    switch (role) {
      case Qt::DisplayRole: return site->getAttribute("name");
      case Qt::DecorationRole: {
        switch (site->type()) {
          case ST_SITE: return KIcon("bookmarks");
          case ST_CATEGORY: return KIcon("folder-bookmark");
          default: break;
        }
        break;
      }
      case TypeRole: return site->isCategory() ? 1 : 0;
      case SiteRole: return QVariant::fromValue(site);
    }
  }
  
  return QVariant();
}

Qt::ItemFlags Model::flags(const QModelIndex &index) const
{
  if (!index.isValid())
    return Qt::ItemIsDropEnabled;
  
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

QModelIndex Model::index(int row, int column, const QModelIndex &parent) const
{
  if (!hasIndex(row, column, parent))
    return QModelIndex();
  
  Site *parentSite;
  
  if (parent.isValid())
    parentSite = static_cast<Site*>(parent.internalPointer());
  else
    parentSite = m_bookmarks->rootSite();
  
  Site *childSite = parentSite->child(row);
  if (childSite)
    return createIndex(row, column, childSite);
  else
    return QModelIndex();
}

QModelIndex Model::parent(const QModelIndex &index) const
{
  if (!index.isValid())
    return QModelIndex();
  
  Site *childSite = static_cast<Site*>(index.internalPointer());
  Site *parentSite = childSite->getParentSite();
  
  if (parentSite->isRoot())
    return QModelIndex();
  
  if (parentSite)
    return createIndex(parentSite->index(), 0, parentSite);
  
  return QModelIndex();
}

int Model::rowCount(const QModelIndex &parent) const
{
  Site *parentSite;
  if (parent.column() > 0)
    return 0;
  
  if (!parent.isValid())
    parentSite = m_bookmarks->rootSite();
  else
    parentSite = static_cast<Site*>(parent.internalPointer());
  
  return parentSite->childCount();
}

Qt::DropActions Model::supportedDropActions() const
{
  return Qt::CopyAction | Qt::MoveAction;
}

QStringList Model::mimeTypes() const
{
  QStringList types;
  types << "application/vnd.text.list";
  return types;
}

QMimeData *Model::mimeData(const QModelIndexList &indexes) const
{
  QMimeData *mimeData = new QMimeData();
  QByteArray encodedData;
  QDataStream stream(&encodedData, QIODevice::WriteOnly);
  
  foreach (QModelIndex index, indexes) {
    if (index.isValid()) {
      QString id = data(index, SiteRole).value<Site*>()->id();
      stream << id;
    }
  }
  
  mimeData->setData("application/vnd.text.list", encodedData);
  return mimeData;
}

bool Model::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int col,
                         const QModelIndex &parent)
{
  if (action == Qt::IgnoreAction)
    return true;
  
  if (!data->hasFormat("application/vnd.text.list"))
    return false;
  
  if (col > 0)
    return false;
  
  QByteArray encodedData = data->data("application/vnd.text.list");
  QDataStream stream(&encodedData, QIODevice::ReadOnly);
  QStringList siteIds;
  int rows = 0;
  
  while (!stream.atEnd()) {
    QString id;
    stream >> id;
    siteIds << id;
    rows++;
  }
  
  Site *parentSite;
  
  if (parent.isValid())
    parentSite = static_cast<Site*>(parent.internalPointer());
  else
    parentSite = m_bookmarks->rootSite();
  
  if (parentSite->isSite())
    parentSite = parentSite->getParentSite();
  
  foreach (QString id, siteIds) {
    Site *site = m_bookmarks->findSite(id);
    
    if (site)
      parentSite->reparentSite(site);
  }
  
  return true;
}

void Model::slotSiteAdded(Site *site)
{
  Site *parentSite = site->getParentSite();
  QModelIndex parentIndex;
  
  if (parentSite->isRoot())
    parentIndex = QModelIndex();
  else
    parentIndex = createIndex(parentSite->index(), 0, parentSite);
  
  const int rowNumber = site->index();
  beginInsertRows(parentIndex, rowNumber, rowNumber);
  endInsertRows();
}

void Model::slotSiteRemoved(Site *site)
{
  Site *parentSite = site->getParentSite();
  QModelIndex parentIndex;
  
  if (parentSite->isRoot())
    parentIndex = QModelIndex();
  else
    parentIndex = createIndex(parentSite->index(), 0, parentSite);
  
  const int rowNumber = site->index();
  beginRemoveRows(parentIndex, rowNumber, rowNumber);
  endRemoveRows();
}

void Model::slotSiteChanged(Site *site)
{
  QModelIndex itemIndex = createIndex(site->index(), 0, site);
  emit dataChanged(itemIndex, itemIndex);
}

}

}

#include "model.moc"
