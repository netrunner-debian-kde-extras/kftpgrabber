/*
 * This file is part of the KFTPGrabber project
 *
 * Copyright (C) 2007 by the KFTPGrabber developers
 * Copyright (C) 2007 Jernej Kos <kostko@jweb-network.net>
 * Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>
 * Copyright (C) 2006 by Dominic Battre <dominic@battre.de>
 * Copyright (C) 2006 by Martin Pool <mbp@canonical.com>
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
#ifndef KFTPWIDGETS_BROWSERDIRSORTFILTERPROXYMODEL_H
#define KFTPWIDGETS_BROWSERDIRSORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>

namespace KFTPWidgets {

namespace Browser {

/**
 * Acts as proxy model for DirModel to sort and filter KFileItems.
 *
 * A natural sorting is done. This means that items like:
 * - item_10.png
 * - item_1.png
 * - item_2.png
 * are sorted like
 * - item_1.png
 * - item_2.png
 * - item_10.png
 *
 * It is assured that directories are always sorted before files.
 *
 * @author Jernej Kos, Dominic Battre, Martin Pool and Peter Penz
 */
class DirSortFilterProxyModel : public QSortFilterProxyModel
{
Q_OBJECT
public:
    DirSortFilterProxyModel(QObject* parent = 0);
    virtual ~DirSortFilterProxyModel();

    /**
     * Reimplemented from QAbstractItemModel. Returns true for directories.
     */
    virtual bool hasChildren(const QModelIndex& parent = QModelIndex()) const;

    /**
     * Reimplemented from QAbstractItemModel.
     * Returns true for 'empty' directories so they can be populated later.
     */
    virtual bool canFetchMore(const QModelIndex& parent) const;

    /**
     * Does a natural comparing of the strings. -1 is returned if \a a
     * is smaller than \a b. +1 is returned if \a a is greater than \a b. 0
     * is returned if both values are equal.
     */
    static int naturalCompare(const QString& a, const QString& b);
protected:
    /**
     * Reimplemented from QAbstractItemModel to use naturalCompare.
     */
    virtual bool lessThan(const QModelIndex& left, const QModelIndex& right) const;
};

}

}

#endif
