/*
 * This file is part of the KFTPGrabber project
 *
 * Copyright (C) 2003-2004 by the KFTPGrabber developers
 * Copyright (C) 2003-2004 Jernej Kos <kostko@jweb-network.net>
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

#include <KActionMenu>

#include <kmenu.h>
#include <klocale.h>

#include "widgets/systemtray.h"
#include "mainwindow.h"
#include "kftpbookmarks.h"
#include "misc/config.h"

namespace KFTPWidgets {

SystemTray *SystemTray::m_self = 0L;

SystemTray::SystemTray(MainWindow *parent)
  : KSystemTrayIcon(parent)
{
  m_self = this;
  
  // Set icon and show it
  setIcon(KIcon("kftpgrabber"));
  
  if (KFTPCore::Config::showSystrayIcon())
    show();

  // Add some actions
  m_bookmarkMenu = new KActionMenu(i18n("Bookmarks"), this);
  slotUpdateBookmarks();

  // Let our bookmarks be up to date
  connect(KFTPBookmarks::Manager::self(), SIGNAL(update()), this, SLOT(slotUpdateBookmarks()));
  
  contextMenu()->addAction(m_bookmarkMenu);
  
  // Ensure that we actually quit
  connect(this, SIGNAL(quitSelected()), this, SLOT(slotQuitSelected()));
}

SystemTray::~SystemTray()
{
  delete m_bookmarkMenu;
}

void SystemTray::slotQuitSelected()
{
  //m_actions->m_closeApp = true;
}

void SystemTray::slotUpdateBookmarks()
{
  // Re-create the bookmarks menu
  m_bookmarkMenu->menu()->clear();
  KFTPBookmarks::Manager::self()->populateBookmarksMenu(m_bookmarkMenu);
}

}

#include "systemtray.moc"
