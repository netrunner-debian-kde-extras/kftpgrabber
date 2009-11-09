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
#ifndef KFTPWIDGETS_QUEUEMODEL_H
#define KFTPWIDGETS_QUEUEMODEL_H

#include <QAbstractItemModel>

namespace KFTPQueue {
  class QueueObject;
}

namespace KFTPWidgets {

namespace Queue {

/**
 * A queue model that can be used to display the current queue.
 *
 * @author Jernej Kos
 */
class Model : public QAbstractItemModel {
Q_OBJECT
public:
    /**
     * Additional index roles.
     */
    enum {
      TypeRole = Qt::UserRole + 1,
      ObjectRole
    };
    
    /**
     * Default model columns.
     */
    enum ModelColumns {
      Name = 0,
      Size,
      Source,
      Destination,
      Progress,
      Speed,
      ETA,
      ColumnCount
    };
    
    /**
     * Class constructor.
     *
     * @param parent An optional parent object
     */
    Model(QObject *parent = 0);
    
    /**
     * @overload
     * Reimplemented from QAbstractItemModel.
     */
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    
    /**
     * @overload
     * Reimplemented from QAbstractItemModel.
     */
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    
    /**
     * @overload
     * Reimplemented from QAbstractItemModel.
     */
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    
    /**
     * @overload
     * Reimplemented from QAbstractItemModel.
     */
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    
    /**
     * @overload
     * Reimplemented from QAbstractItemModel.
     */
    virtual QModelIndex parent(const QModelIndex &index) const;
    
    /**
     * @overload
     * Reimplemented from QAbstractItemModel.
     */
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
private slots:
    void slotObjectAdded(KFTPQueue::QueueObject *object);
    void slotObjectRemoved(KFTPQueue::QueueObject *object);
    void slotObjectChanged(KFTPQueue::QueueObject *object);
    
    void slotObjectBeforeRemoval(KFTPQueue::QueueObject *object);
    void slotObjectAfterRemoval();
};

}

}

#endif
