/*
 * This file is part of the KFTPGrabber project
 *
 * Copyright (C) 2003-2004 by the KFTPGrabber developers
 * Copyright (C) 2003-2004 Jernej Kos <kostko@jweb-network.net>
 * Copyright (C) 2004 Markus Brueffer <markus@brueffer.de>
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

#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_

#include <config-kftpgrabber.h>

#include <qtimer.h>

#include <kxmlguiwindow.h>
#include <kaction.h>

namespace KFTPWidgets {
namespace Bookmarks {
  class Sidebar;
}

  class ConfigDialog;
  class TrafficGraph;
  class QueueView;
}

/**
 * @short Application Main Window
 * @author Jernej Kos <kostko@jweb-network.net>
 */
class MainWindow : public KXmlGuiWindow
{
Q_OBJECT
public:
    /**
     * Class constructor.
     */
    MainWindow();
    
    /**
     * Class destructor.
     */
    ~MainWindow();
protected:
    /**
     * Setup main window actions.
     */
    void setupActions();
    
    /**
     * Initialize the traffic graph widget.
     */
    void initTrafficGraph();
    
    /**
     * Initialize the status bar.
     */
    void initStatusBar();
    
    /**
     * Create the main user interface.
     */
    void initMainView();
        
    /**
     * Initialize sidebars.
     */
    void initSidebars();
    
    /**
     * This method gets called when the user attempts to close the
     * application.
     */
    bool queryClose();
private:
    QTimer *m_graphTimer;

    KFTPWidgets::TrafficGraph *m_trafficGraph;
    KFTPWidgets::QueueView *m_queueView;
    KFTPWidgets::ConfigDialog *m_configDialog;
public slots:
    void initBookmarkMenu();
private slots:
    /**
     * Does a delayed loading of stuff that can take some time to load.
     */
    void slotLoader();
    
    void appShutdown();
    void showBookmarkEditor();
    void slotUpdateStatusBar();
    void slotUpdateTrafficGraph();
    void slotConfigChanged();
    
    void slotFileQuit();
    void slotQuickConnect();
    
    void slotNewSessionLeft();
    void slotNewSessionRight();

    void slotSettingsSave();
    void slotSettingsConfig();
    
    void slotModeAscii();
    void slotModeBinary();
    void slotModeAuto();
};

#endif
