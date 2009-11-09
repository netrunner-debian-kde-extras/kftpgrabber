/*
 * This file is part of the KFTPGrabber project
 *
 * Copyright (C) 2004 by the KFTPGrabber developers
 * Copyright (C) 2004 Jernej Kos <kostko@jweb-network.net>
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

#include "kftpbookmarks.h"
#include "kftpselectserverdialog.h"
#include "kftpbookmarks.h"
#include "bookmarks/listview.h"

#include <k3listview.h>
#include <kiconloader.h>
#include <klocale.h>

using namespace KFTPGrabberBase;

KFTPSelectServerDialog::KFTPSelectServerDialog(QWidget *parent, const char *name)
 : KDialogBase(parent, name, true, "Select a server", KDialogBase::Ok | KDialogBase::Cancel,
               KDialogBase::Ok), m_selectedSite(0)
{
  m_tree = new KFTPWidgets::Bookmarks::ListView(KFTPBookmarks::Manager::self(), this);  
  m_tree->setMinimumWidth(270);
  m_tree->fillBookmarkData();
  
  connect(m_tree, SIGNAL(clicked(Q3ListViewItem*)), this, SLOT(slotTreeClicked()));
  
  // Set some stuff
  setMainWidget(m_tree);
  enableButtonOk(false);
}

void KFTPSelectServerDialog::slotTreeClicked()
{
  enableButtonOk(false);
  
  if (m_tree->selectedItem()) {
    if (static_cast<KFTPWidgets::Bookmarks::ListViewItem*>(m_tree->selectedItem())->m_type == 1) {
      // Set the active server
      m_selectedSite = static_cast<KFTPWidgets::Bookmarks::ListViewItem*>(m_tree->selectedItem())->m_site;
      
      enableButtonOk(true);
      return;
    }
  }
}


#include "kftpselectserverdialog.moc"
