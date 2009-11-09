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
#ifndef KFTPWIDGETS_BROWSERFILTERWIDGET_H
#define KFTPWIDGETS_BROWSERFILTERWIDGET_H

#include <k3listviewsearchline.h>
//Added by qt3to4:
#include <Q3PopupMenu>

namespace KFTPWidgets {

namespace Browser {

class DetailsView;

/**
 * This class is a simple filtering widget that accepts wildcard
 * patterns and filters listviews. Note that this widget only
 * filters on the first column.
 *
 * @author Jernej Kos <kostko@jweb-network.net>
 */
class FilterWidget : public K3ListViewSearchLine {
Q_OBJECT
public:
    /**
     * Class constructor.
     *
     * @param parent The parent widget
     * @param view The view you want to filter
     */
    FilterWidget(QWidget *parent, DetailsView *view);
protected:
    enum {
      FilterDirectories = 1,
      FilterSymlinks = 2,
      Qt::CaseSensitive = 3
    };
    
    /**
     * @overload
     * Reimplemented from K3ListViewSearchLine to support wildcard
     * matching schemes.
     */
    bool itemMatches(const Q3ListViewItem *item, const QString &pattern) const;
    
    /**
     * @overload
     * Reimplemented from K3ListViewSearchLine to remove multiple
     * columns selection, since this widget only operates on the
     * first column.
     */
    Q3PopupMenu *createPopupMenu();
private:
    bool m_filterDirectories;
    bool m_filterSymlinks;
    bool m_caseSensitive;
private slots:
    void slotOptionsMenuActivated(int id);
};

}

}

#endif
