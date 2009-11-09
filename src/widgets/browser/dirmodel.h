/*
 * This file is part of the KFTPGrabber project
 *
 * Copyright (C) 2003-2007 by the KFTPGrabber developers
 * Copyright (C) 2003-2007 Jernej Kos <kostko@jweb-network.net>
 * Copyright (C) 2006 David Faure
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
#ifndef KFTPWIDGETS_BROWSERDIRMODEL_H
#define KFTPWIDGETS_BROWSERDIRMODEL_H

#include <QAbstractItemModel>
#include <KFileItem>

namespace KFTPWidgets {

namespace Browser {

class DirLister;
class DirModelPrivate;

/**
 * A custom directory model implementation based on KDirModel from
 * the KIO library. We need our own, because we can't use KIO slaves
 * for remote operations.
 *
 * @author Jernej Kos <kostko@unimatrix-one.org>
 * @author David Faure
 */
class DirModel : public QAbstractItemModel
{
Q_OBJECT
public:
    /**
     * Default model columns.
     */
    enum ModelColumns {
      Name = 0,
      Size,
      ModifiedTime,
      Permissions,
      Owner,
      Group,
      ColumnCount
    };
    
    /**
     * Flag to set when child count is not known.
     */
    enum { ChildCountUnknown = -1 };
    
    /**
     * Specialized item roles for KFileItemDelegate.
     */
    enum AdditionalRoles {
      FileItemRole = 0x07A263FF,
      ChildCountRole = 0x2C4D0A40
    };
    
    /**
     * Drag and drop behavior.
     */
    enum DropsAllowedFlag {
      NoDrops = 0,
      DropOnDirectory = 1,
      DropOnAnyFile = 2,
      DropOnLocalExecutable = 4
    };
    
    Q_DECLARE_FLAGS(DropsAllowed, DropsAllowedFlag)
    
    /**
     * Class constructor.
     *
     * @param lister Directory lister instance
     * @param parent Optional parent object
     */
    DirModel(DirLister *lister, QObject *parent = 0);
    
    /**
     * Class destructor.
     */
    ~DirModel();
    
    /**
     * Changes the directory lister this model uses.
     *
     * @param lister New lister instance
     */
    void setDirLister(DirLister *lister);
    
    /**
     * This method enables or disables this model's support for treeview
     * behavior. This includes creation of the toplevel node, on the fly
     * creation of stub directories etc. It will also limit display to
     * directories only, so you can (and should) use the same directory
     * lister as for the detailed view (if any).
     *
     * You MUST set this to true if you wish to use this model in a tree-view
     * like fashion, otherwise things will not work as they should!
     *
     * @param value True to enable tree-view behavior, false otherwise
     */
    void setTreeViewBehavior(bool value);
    
    /**
     * Returns the directory lister associated with this model.
     */
    DirLister *dirLister() const;
    
    KFileItem itemForIndex(const QModelIndex &index) const;
    
    QModelIndex indexForItem(const KFileItem &item) const;
    
    QModelIndex indexForItem(const KFileItem *item) const;
    
    QModelIndex indexForUrl(const KUrl &url) const;
    
    void itemChanged(const QModelIndex &index);
    
    void setDropsAllowed(DropsAllowed dropsAllowed);
    
    /**
     * @overload
     * Reimplemented from QAbstractItemModel.
     */
    virtual bool canFetchMore(const QModelIndex &parent) const;
    
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
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
    
    /**
     * @overload
     * Reimplemented from QAbstractItemModel.
     */
    virtual void fetchMore(const QModelIndex &parent);
    
    /**
     * @overload
     * Reimplemented from QAbstractItemModel.
     */
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    
    /**
     * @overload
     * Reimplemented from QAbstractItemModel.
     */
    virtual bool hasChildren(const QModelIndex &parent = QModelIndex()) const;
    
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
    virtual QMimeData *mimeData(const QModelIndexList &indexes) const;
    
    /**
     * @overload
     * Reimplemented from QAbstractItemModel.
     */
    virtual QStringList mimeTypes() const;
    
    /**
     * @overload
     * Reimplemented from QAbstractItemModel.
     */
    virtual QModelIndex parent(const QModelIndex &index) const;
    
    /**
     * @overload
     * Reimplemented from QAbstractItemModel.
     */
    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const;
    
    /**
     * @overload
     * Reimplemented from QAbstractItemModel.
     */
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    
    /**
     * @overload
     * Reimplemented from QAbstractItemModel.
     */
    virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
private slots:
    void slotNewItems(const KFileItemList &items);
    void slotDeleteItem(const KFileItem &item);
    void slotRefreshItems(const QList<QPair<KFileItem, KFileItem> > &items);
    void slotClear();
    void slotUnconditionalClear();
private:
    virtual bool insertRows(int , int, const QModelIndex& = QModelIndex()) { return false; }
    virtual bool insertColumns(int, int, const QModelIndex& = QModelIndex()) { return false; }
    virtual bool removeRows(int, int, const QModelIndex& = QModelIndex()) { return false; }
    virtual bool removeColumns(int, int, const QModelIndex& = QModelIndex()) { return false; }
private:
    friend class DirModelPrivate;
    DirModelPrivate *const d;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(DirModel::DropsAllowed)

}

}

#endif
