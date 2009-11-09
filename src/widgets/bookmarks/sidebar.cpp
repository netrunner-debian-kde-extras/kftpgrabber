/*
 * This file is part of the KFTPGrabber project
 *
 * Copyright (C) 2004 by the KFTPGrabber developers
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

#include <qlayout.h>
#include <qsplitter.h>
#include <q3vbox.h>
//Added by qt3to4:
#include <Q3VBoxLayout>

#include <kaction.h>
#include <klocale.h>
#include <ktoolbar.h>

#include "kftpapi.h"
#include "kftpbookmarks.h"
#include "editor.h"
#include "listview.h"

#include "sidebar.h"

namespace KFTPWidgets {

namespace Bookmarks {

KActionCollection *Sidebar::actionCollection()
{
  return KFTPAPI::getInstance()->mainWindow()->actionCollection();
}

Sidebar::Sidebar(QWidget *parent, const char *name)
 : QWidget(parent, name)
{
  Q3VBoxLayout *layout = new Q3VBoxLayout(this);

  m_toolBar = new KToolBar(this, "bookmarkToolBar");
  m_toolBar->setIconSize(16);
  layout->addWidget(m_toolBar);

  // Create the list view for editing bookmarks
  m_tree = new ListView(KFTPBookmarks::Manager::self(), this);
  m_tree->setAutoUpdate(true);
  m_tree->setConnectBookmark(true);
  m_tree->setEditMenuItem(true);

  layout->addWidget(m_tree);

  m_editAction = new KAction(i18n("&Edit..."), "document-properties", KShortcut(), this, SLOT(slotEditAction()), actionCollection(), "bookmark_edit2");
  connect(m_tree, SIGNAL(bookmarkClicked(Q3ListViewItem*)), this, SLOT(slotClicked(Q3ListViewItem*)));
  connect(m_tree, SIGNAL(bookmarkNew(ListViewItem*, KFTPBookmarks::Site*)), this, SLOT(slotNewAction(ListViewItem*, KFTPBookmarks::Site*)));

  // Get the new bookmark data
  m_tree->fillBookmarkData();

  // Init the Actions
  slotClicked(0L);

  setMinimumWidth(200);
}

Sidebar::~Sidebar()
{
}

void Sidebar::refresh()
{
  m_tree->clear();
  m_tree->fillBookmarkData();
}

void Sidebar::slotEditAction()
{
  ListViewItem* item = static_cast<ListViewItem*>(m_tree->selectedItems().at(0));

  if (item) {
    BookmarkEditor *editor = new BookmarkEditor(item, this);

    editor->exec();
    delete editor;

    // Update the bookmarks globaly
    KFTPBookmarks::Manager::self()->emitUpdate();
  }
}

void Sidebar::slotNewAction(ListViewItem*, KFTPBookmarks::Site *site)
{
  BookmarkEditor *editor = new BookmarkEditor(static_cast<ListViewItem*>(m_tree->selectedItems().at(0)), this);

  if (!editor->exec()) {
    // If the user clicks Abort, remove the newly created server
    KFTPBookmarks::Manager::self()->delSite(site);
  }

  delete editor;
}

void Sidebar::slotClicked(Q3ListViewItem *item)
{
  // When nodes are expanded, item is 0, although an item is still selected, so grab it here
  item = m_tree->selectedItems().at(0);

  // Enable/Disable actions for the toolbar
  if (!item) {
    actionCollection()->action("bookmark_delete")->setEnabled(false);
    actionCollection()->action("bookmark_subcat")->setEnabled(true);
    m_editAction->setEnabled(false);
    return;
  }

  actionCollection()->action("bookmark_delete")->setEnabled(true);

  if (static_cast<ListViewItem*>(item)->m_type == BT_CATEGORY) {
    m_editAction->setEnabled(false);
    actionCollection()->action("bookmark_subcat")->setEnabled(true);
  } else {
    m_editAction->setEnabled(true);
    actionCollection()->action("bookmark_subcat")->setEnabled(false);
  }
}

}

}

#include "sidebar.moc"
