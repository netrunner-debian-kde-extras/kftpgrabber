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
#include "bookmarks/treeview.h"
#include "bookmarks/model.h"
#include "kftpbookmarks.h"

#include <QEvent>
#include <QMenu>
#include <QContextMenuEvent>
#include <QHeaderView>

#include <KLocale>
#include <KIcon>
#include <KInputDialog>
#include <KMessageBox>

using namespace KFTPBookmarks;

namespace KFTPWidgets {

namespace Bookmarks {

TreeView::TreeView(Manager *manager, QWidget* parent)
  : QTreeView(parent),
    m_manager(manager)
{
  setAcceptDrops(true);
  setDragEnabled(true);
  setUniformRowHeights(true);
  setSelectionMode(QAbstractItemView::SingleSelection);
  setEditTriggers(QAbstractItemView::NoEditTriggers);
  setSortingEnabled(true);
  //setFrameStyle(QFrame::NoFrame);
  setDragDropMode(QAbstractItemView::InternalMove);
  setDropIndicatorShown(true);
  setAutoExpandDelay(300);
  setRootIsDecorated(true);

  viewport()->setAttribute(Qt::WA_Hover);
  
  connect(this, SIGNAL(clicked(const QModelIndex&)), this, SLOT(slotItemSelected(const QModelIndex&)));
}

bool TreeView::event(QEvent* event)
{
  if (event->type() == QEvent::Polish)
    header()->hide();

  return QTreeView::event(event);
}

void TreeView::contextMenuEvent(QContextMenuEvent *event)
{
  m_currentIndex = indexAt(event->pos());
  
  // Show a context menu
  QMenu menu(this);
  menu.addAction(KIcon("bookmark-new"), i18n("&Create new site..."), this, SLOT(slotCreateNewSite()));
  menu.addAction(KIcon("bookmark-folder"), i18n("Create sub&category..."), this, SLOT(slotCreateSubcategory()));
  
  if (m_currentIndex.isValid()) {
    Site *site = m_currentIndex.data(Model::SiteRole).value<Site*>();

    menu.addAction(KIcon("edit-delete"), i18n("&Remove"), this, SLOT(slotRemove()));
    
    if (site->isSite())
      menu.addAction(KIcon("edit-copy"), i18n("&Duplicate site"), this, SLOT(slotDuplicate()));
  }
  
  menu.exec(event->globalPos());
}

void TreeView::slotItemSelected(const QModelIndex &index)
{
  m_currentIndex = index;
}

void TreeView::slotCreateNewSite()
{
  Site *parentSite;
  
  if (m_currentIndex.isValid())
    parentSite = m_currentIndex.data(Model::SiteRole).value<Site*>();
  else
    parentSite = m_manager->rootSite();
  
  if (parentSite->isSite())
    parentSite = parentSite->getParentSite();
  
  Site *site = parentSite->addSite(KInputDialog::getText(i18n("Create site"), i18n("Name")));
  m_currentIndex = model()->index(site->index(), 0, m_currentIndex);
  setCurrentIndex(m_currentIndex);
  emit clicked(m_currentIndex);
}

void TreeView::slotCreateSubcategory()
{
  Site *parentSite;
  
  if (m_currentIndex.isValid())
    parentSite = m_currentIndex.data(Model::SiteRole).value<Site*>();
  else
    parentSite = m_manager->rootSite();
  
  if (parentSite->isSite())
    parentSite = parentSite->getParentSite();
  
  parentSite->addCategory(KInputDialog::getText(i18n("Create subcategory"), i18n("Name")));
}

void TreeView::slotRemove()
{
  if (KMessageBox::warningYesNo(this, i18n("Are you sure you wish to remove this site or category?")) != KMessageBox::Yes)
    return;
  
  Site *site = m_currentIndex.data(Model::SiteRole).value<Site*>();
  m_manager->removeSite(site);
  m_currentIndex = QModelIndex();
  setCurrentIndex(m_currentIndex);
}

void TreeView::slotDuplicate()
{
  Site *site = m_currentIndex.data(Model::SiteRole).value<Site*>();
  site->duplicate();
}

}

}

#include "treeview.moc"
