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
#include "queueview/model.h"
#include "queueobject.h"
#include "kftptransfer.h"
#include "kftpqueue.h"
#include "site.h"


#include <KIcon>
#include <KLocale>

using namespace KFTPQueue;

namespace KFTPWidgets {

namespace Queue {

Model::Model(QObject *parent)
  : QAbstractItemModel(parent)
{
  connect(Manager::self(), SIGNAL(objectAdded(KFTPQueue::QueueObject*)), this, SLOT(slotObjectAdded(KFTPQueue::QueueObject*)));
  connect(Manager::self(), SIGNAL(objectRemoved(KFTPQueue::QueueObject*)), this, SLOT(slotObjectRemoved(KFTPQueue::QueueObject*)));
  connect(Manager::self(), SIGNAL(objectChanged(KFTPQueue::QueueObject*)), this, SLOT(slotObjectChanged(KFTPQueue::QueueObject*)));
  
  connect(Manager::self(), SIGNAL(objectBeforeRemoval(KFTPQueue::QueueObject*)), this, SLOT(slotObjectBeforeRemoval(KFTPQueue::QueueObject*)));
  connect(Manager::self(), SIGNAL(objectAfterRemoval()), this, SLOT(slotObjectAfterRemoval()));
}

int Model::columnCount(const QModelIndex&) const
{
  return ColumnCount;
}

QVariant Model::headerData(int section, Qt::Orientation orientation, int role) const
{
  Q_UNUSED(orientation);
  
  switch (role) {
    case Qt::DisplayRole: {
      switch (section) {
        case Name: return i18n("Name");
        case Size: return i18n("Size");
        case Source: return i18n("Source");
        case Destination: return i18n("Destination");
        case Progress: return i18n("Progress");
        case Speed: return i18n("Speed");
        case ETA: return i18n("ETA");
      }
    }
  }
  
  return QVariant();
}

QVariant Model::data(const QModelIndex &index, int role) const
{
  if (index.isValid()) {
    QueueObject *object = static_cast<QueueObject*>(index.internalPointer());
    Transfer *transfer = static_cast<Transfer*>(object);
    Site *site = static_cast<Site*>(object);
    
    switch (role) {
      case Qt::DisplayRole: {
        if (index.column() == Name) {
          switch (object->getType()) {
            case QueueObject::Site: return site->getUrl().host();
            case QueueObject::Directory:
            case QueueObject::File: return transfer->getSourceUrl().fileName();
            default: break;
          }
        } else if (index.column() == Size) {
          return KIO::convertSize(object->getActualSize());
        } else if (object->isRunning()) {
          switch (index.column()) {
            case ETA: return KIO::convertSeconds(KIO::calculateRemainingSeconds(object->getSize(),
                                                                                object->getCompleted(),
                                                                                object->getSpeed()));
            case Speed: return QString("%1/s").arg(KIO::convertSize(object->getSpeed()));
          }
        }
        
        if (object->isTransfer()) {
          switch (index.column()) {
            case Source: return transfer->getSourceUrl().pathOrUrl();
            case Destination: return transfer->getDestUrl().pathOrUrl();
            case Progress: {
              if (object->isDir() && object->isLocked()) {
                return i18n("Scanning...");
              } else if (object->isConnecting()) {
                return i18n("Connecting...");
              } else if (object->isWaiting()) {
                return i18n("Waiting...");
              } else if (object->isRunning()) {
                return QString("%1 %%").arg(object->getProgress().first);
              }
              break;
            }
          }
        }
        break;
      }
      case Qt::DecorationRole: {
        if (index.column() == Name) {
          switch (object->getType()) {
            case QueueObject::Site: return KIcon("network-server");
            case QueueObject::Directory: return KIcon("folder");
            case QueueObject::File: return KIcon("txt"); // FIXME depending on the mimetype + cache!
            default: break;
          }
        }
        break;
      }
      case Qt::FontRole: {
        if (index.column() == Name && object->isSite()) {
          QFont font;
          font.setBold(true);
          return font;
        } else if (index.column() == Progress && object->isDir() && object->isLocked()) {
          QFont font;
          font.setBold(true);
          return font;
        } else if (index.column() == Progress && (object->isConnecting() || object->isWaiting())) {
          QFont font;
          font.setBold(true);
          return font;
        }
        break;
      }
      case Qt::ForegroundRole: {
        if (object->isDir() && object->isLocked()) {
          // A directory scan is in progress
          return QColor(Qt::darkGreen);
        } else if (object->isConnecting() || object->isWaiting()) {
          // Object is connecting or waiting for a free connection
          return QColor(Qt::darkBlue);
        }
        break;
      }
      case ObjectRole: return QVariant::fromValue(object);
      default: break;
    }
  }
  
  return QVariant();
}

QModelIndex Model::index(int row, int column, const QModelIndex &parent) const
{
  if (!hasIndex(row, column, parent))
    return QModelIndex();
  
  QueueObject *parentObject;
  
  if (parent.isValid())
    parentObject = static_cast<QueueObject*>(parent.internalPointer());
  else
    parentObject = Manager::self()->topLevelObject();
  
  QueueObject *childObject = parentObject->getChildAt(row);
  if (childObject)
    return createIndex(row, column, childObject);
  else
    return QModelIndex();
}

QModelIndex Model::parent(const QModelIndex &index) const
{
  if (!index.isValid())
    return QModelIndex();
  
  QueueObject *childObject = static_cast<QueueObject*>(index.internalPointer());
  QueueObject *parentObject = childObject->parentObject();
  
  if (parentObject && !parentObject->isToplevel())
    return createIndex(parentObject->index(), 0, parentObject);
  
  return QModelIndex();
}

int Model::rowCount(const QModelIndex &parent) const
{
  QueueObject *parentObject;
  if (parent.column() > 0)
    return 0;
  
  if (!parent.isValid())
    parentObject = Manager::self()->topLevelObject();
  else
    parentObject = static_cast<QueueObject*>(parent.internalPointer());
  
  return parentObject->childCount();
}

void Model::slotObjectAdded(QueueObject *object)
{
  QueueObject *parentObject = object->parentObject();
  QModelIndex parentIndex;
  
  if (parentObject->isToplevel())
    parentIndex = QModelIndex();
  else
    parentIndex = createIndex(parentObject->index(), 0, parentObject);
  
  const int rowNumber = object->index();
  beginInsertRows(parentIndex, rowNumber, rowNumber);
  endInsertRows();
}

void Model::slotObjectRemoved(QueueObject *object)
{
  slotObjectBeforeRemoval(object);
  slotObjectAfterRemoval();
}

void Model::slotObjectBeforeRemoval(KFTPQueue::QueueObject *object)
{
  QueueObject *parentObject = object->parentObject();
  QModelIndex parentIndex;
  
  if (parentObject->isToplevel())
    parentIndex = QModelIndex();
  else
    parentIndex = createIndex(parentObject->index(), 0, parentObject);
  
  const int rowNumber = object->index();
  beginRemoveRows(parentIndex, rowNumber, rowNumber);
}

void Model::slotObjectAfterRemoval()
{
  endRemoveRows();
}

void Model::slotObjectChanged(QueueObject *object)
{
  QModelIndex startIndex = createIndex(object->index(), 0, object);
  QModelIndex endIndex = createIndex(object->index(), ColumnCount - 1, object);
  emit dataChanged(startIndex, endIndex);
}

}

}

#include "model.moc"
