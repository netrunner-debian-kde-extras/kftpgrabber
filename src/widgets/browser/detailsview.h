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
#ifndef KFTPWIDGETS_BROWSERDETAILSVIEW_H
#define KFTPWIDGETS_BROWSERDETAILSVIEW_H

#include "browser/locationnavigator.h"

#include <QStyleOptionViewItem>
#include <QTreeView>

namespace KFTPSession {
  class Session;
}

namespace KFTPWidgets {

namespace Browser {

class DirLister;
class TreeView;
class View;

/**
 * This class represents a detailed list view for displaying local and
 * remote directory contents. It is based upon QTreeView but uses
 * a custom (wrapped) DirLister for actual listings.
 *
 * @author Jernej Kos
 */
class DetailsView : public QTreeView {
Q_OBJECT
public:
    DetailsView(QWidget *parent, View *view);
    virtual ~DetailsView();
protected:
    virtual bool event(QEvent* event);
    virtual void contextMenuEvent(QContextMenuEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void dragEnterEvent(QDragEnterEvent* event);
    virtual void dropEvent(QDropEvent* event);
    virtual void scrollContentsBy(int dx, int dy);
signals:
    /**
     * This signals gets emitted when user scrolls the widget.
     *
     * @param x New X position
     * @param y New Y position
     */
    void contentsMoved(int x, int y);
    
    /**
     * This signal is emitted when items change.
     */
    void itemsChanged();
private:
    QStyleOptionViewItem m_viewOptions;
    View *m_view;
};

}

}

#endif
