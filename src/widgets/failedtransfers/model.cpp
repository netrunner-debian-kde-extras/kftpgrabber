/*
 * This file is part of the KFTPgrabber project
 *
 * Copyright (C) 2003-2009 by the KFTPgrabber developers
 * Copyright (C) 2003-2009 Jernej Kos <kostko@unimatrix-one.org>
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
#include "failedtransfers/model.h"
#include "kftptransfer.h"
#include "kftpqueue.h"

#include <KIcon>
#include <KLocale>

using namespace KFTPQueue;

namespace KFTPWidgets {

namespace FailedTransfers {

Model::Model(QObject *parent)
  : QAbstractTableModel(parent)
{
  connect(Manager::self(), SIGNAL(failedTransferAdded(KFTPQueue::FailedTransfer*)), this, SLOT(slotObjectAdded(KFTPQueue::FailedTransfer*)));
  connect(Manager::self(), SIGNAL(failedTransferBeforeRemoval(KFTPQueue::FailedTransfer*)), this, SLOT(slotObjectBeforeRemoval(KFTPQueue::FailedTransfer*)));
  connect(Manager::self(), SIGNAL(failedTransferAfterRemoval()), this, SLOT(slotObjectAfterRemoval()));
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
        case ErrorMessage: return i18n("Error Message");
        case Size: return i18n("Size");
        case Source: return i18n("Source");
        case Destination: return i18n("Destination");
      }
    }
  }
  
  return QVariant();
}

QVariant Model::data(const QModelIndex &index, int role) const
{
  if (index.isValid()) {
    FailedTransfer *ftransfer = static_cast<FailedTransfer*>(index.internalPointer());
    TransferFile *transfer = ftransfer->getTransfer();
    
    switch (role) {
      case Qt::DisplayRole: {
        switch (index.column()) {
          case Name: return transfer->getSourceUrl().fileName();
          case ErrorMessage: return ftransfer->getError();
          case Size: return KIO::convertSize(transfer->getActualSize());
          case Source: return transfer->getSourceUrl().pathOrUrl();
          case Destination: return transfer->getDestUrl().pathOrUrl();
        }
        break;
      }
      case Qt::DecorationRole: {
        if (index.column() == Name) {
          return KIcon("txt");
        }
        break;
      }
      case ObjectRole: return QVariant::fromValue(ftransfer);
      default: break;
    }
  }
  
  return QVariant();
}

QModelIndex Model::index(int row, int column, const QModelIndex &parent) const
{
  if (!hasIndex(row, column, parent))
    return QModelIndex();
  
  FailedTransfer *transfer = Manager::self()->failedTransfers()->at(row);
  if (transfer)
    return createIndex(row, column, transfer);
  else
    return QModelIndex();
}

int Model::rowCount(const QModelIndex &parent) const
{
  Q_UNUSED(parent);
  return Manager::self()->failedTransfers()->count();
}

void Model::slotObjectAdded(KFTPQueue::FailedTransfer *transfer)
{
  const int rowNumber = Manager::self()->failedTransfers()->lastIndexOf(transfer);
  beginInsertRows(QModelIndex(), rowNumber, rowNumber);
  endInsertRows();
}

void Model::slotObjectBeforeRemoval(KFTPQueue::FailedTransfer *transfer)
{
  const int rowNumber = Manager::self()->failedTransfers()->lastIndexOf(transfer);
  beginRemoveRows(QModelIndex(), rowNumber, rowNumber);
}

void Model::slotObjectAfterRemoval()
{
  endRemoveRows();
}

}

}

#include "model.moc"
