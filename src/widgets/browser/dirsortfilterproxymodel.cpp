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
#include "browser/dirsortfilterproxymodel.h"
#include "browser/dirmodel.h"

#include <KFileItem>
#include <KDateTime>
#include <KLocale>

namespace KFTPWidgets {

namespace Browser {

DirSortFilterProxyModel::DirSortFilterProxyModel(QObject *parent)
  : QSortFilterProxyModel(parent)
{
  setDynamicSortFilter(true);

  // Sort by the user visible string for now
  setSortCaseSensitivity(Qt::CaseInsensitive);
  sort(DirModel::Name, Qt::AscendingOrder);
}

DirSortFilterProxyModel::~DirSortFilterProxyModel()
{
}

bool DirSortFilterProxyModel::hasChildren(const QModelIndex& parent) const
{
  const QModelIndex sourceParent = mapToSource(parent);
  return sourceModel()->hasChildren(sourceParent);
}

bool DirSortFilterProxyModel::canFetchMore(const QModelIndex& parent) const
{
  const QModelIndex sourceParent = mapToSource(parent);
  return sourceModel()->canFetchMore(sourceParent);
}

int DirSortFilterProxyModel::naturalCompare(const QString& a, const QString& b)
{
  // This method chops the input a and b into pieces of
  // digits and non-digits (a1.05 becomes a | 1 | . | 05)
  // and compares these pieces of a and b to each other
  // (first with first, second with second, ...).
  //
  // This is based on the natural sort order code code by Martin Pool
  // http://sourcefrog.net/projects/natsort/
  // Martin Pool agreed to license this under LGPL or GPL.

  const QChar* currA = a.unicode(); // iterator over a
  const QChar* currB = b.unicode(); // iterator over b

  if (currA == currB) {
    return 0;
  }

  const QChar* begSeqA = currA; // beginning of a new character sequence of a
  const QChar* begSeqB = currB;

  while (!currA->isNull() && !currB->isNull()) {
    // find sequence of characters ending at the first non-character
    while (!currA->isNull() && !currA->isDigit()) {
      ++currA;
    }

    while (!currB->isNull() && !currB->isDigit()) {
      ++currB;
    }

    // compare these sequences
    const QString subA(begSeqA, currA - begSeqA);
    const QString subB(begSeqB, currB - begSeqB);
    const int cmp = QString::localeAwareCompare(subA, subB);
    if (cmp != 0) {
      return cmp;
    }

    if (currA->isNull() || currB->isNull()) {
      break;
    }

    // now some digits follow...
    if ((*currA == '0') || (*currB == '0')) {
      // one digit-sequence starts with 0 -> assume we are in a fraction part
      // do left aligned comparison (numbers are considered left aligned)
      while (1) {
        if (!currA->isDigit() && !currB->isDigit()) {
          break;
        } else if (!currA->isDigit()) {
          return -1;
        } else if (!currB->isDigit()) {
          return + 1;
        } else if (*currA < *currB) {
          return -1;
        } else if (*currA > *currB) {
          return + 1;
        }
        
        ++currA;
        ++currB;
      }
    } else {
      // No digit-sequence starts with 0 -> assume we are looking at some integer
      // do right aligned comparison.
      //
      // The longest run of digits wins. That aside, the greatest
      // value wins, but we can't know that it will until we've scanned
      // both numbers to know that they have the same magnitude.

      int weight = 0;
      while (1) {
        if (!currA->isDigit() && !currB->isDigit()) {
          if (weight != 0) {
            return weight;
          }
          
          break;
        } else if (!currA->isDigit()) {
          return -1;
        } else if (!currB->isDigit()) {
          return + 1;
        } else if ((*currA < *currB) && (weight == 0)) {
          weight = -1;
        } else if ((*currA > *currB) && (weight == 0)) {
          weight = + 1;
        }
        
        ++currA;
        ++currB;
      }
    }

    begSeqA = currA;
    begSeqB = currB;
  }

  if (currA->isNull() && currB->isNull()) {
    return 0;
  }

  return currA->isNull() ? -1 : + 1;
}

bool DirSortFilterProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
  DirModel *dirModel = static_cast<DirModel*>(sourceModel());

  const KFileItem leftFileItem = dirModel->itemForIndex(left);
  const KFileItem rightFileItem = dirModel->itemForIndex(right);

  // On our priority, folders go above regular files
  if (leftFileItem.isDir() && !rightFileItem.isDir()) {
    return true;
  } else if (!leftFileItem.isDir() && rightFileItem.isDir()) {
    return false;
  }

  // Hidden elements go before visible ones, if they both are
  // folders or files
  if (leftFileItem.isHidden() && !rightFileItem.isHidden()) {
    return true;
  } else if (!leftFileItem.isHidden() && rightFileItem.isHidden()) {
    return false;
  }

  switch (left.column()) {
    case DirModel::Name: {
      // So we are in the same priority, what counts now is their names
      const QVariant leftData = dirModel->data(left, DirModel::Name);
      const QVariant rightData = dirModel->data(right, DirModel::Name);
      const QString leftValueString(leftData.toString());
      const QString rightValueString(rightData.toString());

      return sortCaseSensitivity() ? (naturalCompare(leftValueString, rightValueString) < 0) :
                                     (naturalCompare(leftValueString.toLower(), rightValueString.toLower()) < 0);
    }
    
    case DirModel::Size: {
      // If we have two folders, what we have to measure is the number of
      // items that contains each other
      if (leftFileItem.isDir() && rightFileItem.isDir()) {
        QVariant leftValue = dirModel->data(left, DirModel::ChildCountRole);
        int leftCount = leftValue.type() == QVariant::Int ? leftValue.toInt() : DirModel::ChildCountUnknown;

        QVariant rightValue = dirModel->data(right, DirModel::ChildCountRole);
        int rightCount = rightValue.type() == QVariant::Int ? rightValue.toInt() : DirModel::ChildCountUnknown;

        // In the case they two have the same child items, we sort them by
        // their names. So we have always everything ordered. We also check
        // if we are taking in count their cases
        if (leftCount == rightCount) {
          return sortCaseSensitivity() ? (naturalCompare(leftFileItem.name(), rightFileItem.name()) < 0) :
                                         (naturalCompare(leftFileItem.name().toLower(), rightFileItem.name().toLower()) < 0);
        }

        // If they had different number of items, we sort them depending
        // on how many items had each other
        return leftCount < rightCount;
      }

      // If what we are measuring is two files and they have the same size,
      // sort them by their file names.
      if (leftFileItem.size() == rightFileItem.size()) {
        return sortCaseSensitivity() ? (naturalCompare(leftFileItem.name(), rightFileItem.name()) < 0) :
                                       (naturalCompare(leftFileItem.name().toLower(), rightFileItem.name().toLower()) < 0);
      }

      // If their sizes are different, sort them by their sizes, as expected.
      return leftFileItem.size() < rightFileItem.size();
    }
    
    case DirModel::ModifiedTime: {
      KDateTime leftTime = leftFileItem.time(KFileItem::ModificationTime);
      KDateTime rightTime = rightFileItem.time(KFileItem::ModificationTime);

      if (leftTime == rightTime) {
        return sortCaseSensitivity() ? (naturalCompare(leftFileItem.name(), rightFileItem.name()) < 0) :
                                       (naturalCompare(leftFileItem.name().toLower(), rightFileItem.name().toLower()) < 0);
      }

      return leftTime > rightTime;
    }
  
    case DirModel::Permissions: {
      if (leftFileItem.permissionsString() == rightFileItem.permissionsString()) {
        return sortCaseSensitivity() ? (naturalCompare(leftFileItem.name(), rightFileItem.name()) < 0) :
                                       (naturalCompare(leftFileItem.name().toLower(), rightFileItem.name().toLower()) < 0);
      }

      return naturalCompare(leftFileItem.permissionsString(), rightFileItem.permissionsString()) < 0;
    }
  
    case DirModel::Owner: {
      if (leftFileItem.user() == rightFileItem.user()) {
        return sortCaseSensitivity() ? (naturalCompare(leftFileItem.name(), rightFileItem.name()) < 0) :
                                       (naturalCompare(leftFileItem.name().toLower(), rightFileItem.name().toLower()) < 0);
      }

      return naturalCompare(leftFileItem.user(), rightFileItem.user()) < 0;
    }
  
    case DirModel::Group: {
      if (leftFileItem.group() == rightFileItem.group()) {
        return sortCaseSensitivity() ? (naturalCompare(leftFileItem.name(), rightFileItem.name()) < 0) :
                                       (naturalCompare(leftFileItem.name().toLower(), rightFileItem.name().toLower()) < 0);
      }

      return naturalCompare(leftFileItem.group(), rightFileItem.group()) < 0;
    }
  }

  // We have set a SortRole and trust the ProxyModel to do the right thing for now
  return QSortFilterProxyModel::lessThan(left, right);
}

}

}

#include "dirsortfilterproxymodel.moc"
