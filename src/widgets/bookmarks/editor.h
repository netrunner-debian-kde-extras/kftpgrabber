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


#ifndef KFTPBOOKMARKEDITOR_H
#define KFTPBOOKMARKEDITOR_H

#include <KDialog>
#include <QSortFilterProxyModel>

// Ui files
#include "widgets/bookmarks/ui_editor.h"

namespace KFTPBookmarks {
  class Site;
  class Manager;
}

class KPushButton;

namespace KFTPWidgets {

namespace Bookmarks {

class TreeView;

/**
 * @author Jernej Kos
 */
class Editor : public KDialog
{
Q_OBJECT
public:
    /**
     * Class constructor.
     *
     * @param parent Parent widget
     * @param quickConnect Should this dialog behave as quick connect
     */
    Editor(QWidget *parent, bool quickConnect = false);
    
    /**
     * Returns the site selected for connection.
     */
    KFTPBookmarks::Site *selectedSite() const { return m_selectedSite; }
protected:
    /**
     * Saves the currently open item.
     */
    void saveCurrentItem();
    
    /**
     * Disables FTP-related options.
     */
    void disableOptions();
    
    /**
     * Enables FTP-related options.
     */
    void enableOptions();
private:
    QSortFilterProxyModel *m_proxyModel;
    TreeView *m_treeView;
    Ui::Editor m_layout;
    QWidget *m_properties;
    QModelIndex m_currentIndex;
    KFTPBookmarks::Manager *m_manager;
    KFTPBookmarks::Site *m_selectedSite;
    
    KPushButton *m_newSite;
    KPushButton *m_removeSite;
    KPushButton *m_newCategory;
    KPushButton *m_duplicateSite;
    
    bool m_portChanged;
private slots:
    void slotOkClicked();
    void slotCancelClicked();
    void slotConnectClicked();
    
    void slotItemClicked(const QModelIndex &index);
    void slotAnonymousToggled(bool value);
    void slotProtocolChanged(int index);
    void slotPortChanged();
    void slotCurrentNameChanged(const QString &name);
    
    void slotSiteRemoved(KFTPBookmarks::Site *site);
};

}

}

#endif
