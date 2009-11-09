/*
 * This file is part of the KFTPGrabber project
 *
 * Copyright (C) 2003-2007 by the KFTPGrabber developers
 * Copyright (C) 2003-2007 Jernej Kos <kostko@jweb-network.net>
 * Copyright (C) 2005 Markus Brueffer <markus@brueffer.de>
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

#ifndef KFTPQUEUEVIEW_H
#define KFTPQUEUEVIEW_H

#include <QWidget>

class KToolBar;
class KAction;

namespace KFTPWidgets {

namespace Queue {
  class TreeView;
}

/**
 * A widget for displaying and manipulating the current queue.
 *
 * @author Jernej Kos
 */
class QueueView : public QWidget
{
Q_OBJECT
public:
    /**
     * Class constructor.
     */
    QueueView(QWidget *parent);
    
    /**
     * Load queue list layout from the configuration file.
     */
    void loadLayout();
    
    /**
     * Save queue list layout to the configuration file.
     */
    void saveLayout();
public slots:    
    void updateActions();
protected:
    /**
     * Initialize actions.
     */
    void initActions();
    
    /**
     * Initialize toolbar widgets.
     */
    void initToolBar();
private:
    // Toolbar Actions
    KAction *m_loadAction;
    KAction *m_saveAction;
    KAction *m_startAction;
    KAction *m_pauseAction;
    KAction *m_stopAction;
    KAction *m_addAction;
    KAction *m_searchAction;
    KAction *m_filterAction;
    
    //K3ListViewSearchLine *m_searchField;

    KToolBar *m_toolBar;
    KToolBar *m_searchToolBar;
    Queue::TreeView *m_tree;
private slots:
    // Slots for actions
    void slotLoad();
    void slotSave();
    void slotStart();
    void slotPause();
    void slotStop();
    void slotAdd();
    void slotSearch();
    void slotFilter();
    
    void slotDownloadLimitChanged(int value);
    void slotUploadLimitChanged(int value);
    void slotThreadCountChanged(int value);
};

}

#endif
