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
#include "browser/filterwidget.h"
#include "browser/detailsview.h"

#include <q3listview.h>
#include <qregexp.h>
#include <q3popupmenu.h>

#include <klocale.h>

namespace KFTPWidgets {

namespace Browser {

FilterWidget::FilterWidget(QWidget *parent, DetailsView *view)
  : K3ListViewSearchLine(parent, view),
    m_filterDirectories(true),
    m_filterSymlinks(true),
    m_caseSensitive(false)
{
  connect(view, SIGNAL(itemsChanged()), this, SLOT(updateSearch()));
}

bool FilterWidget::itemMatches(const Q3ListViewItem *item, const QString &pattern) const
{
  if (!pattern.isEmpty()) {
    const KFileListViewItem *i = dynamic_cast<const KFileListViewItem*>(item);
    
    if (i) {
      if (i->fileInfo()->isDir() && !m_filterDirectories)
        return true;
      else if (i->fileInfo()->isLink() && !m_filterSymlinks)
        return true;
    }
    
    QRegExp filter(pattern);
    filter.setCaseSensitive(m_caseSensitive);
    filter.setWildcard(true);
    
    return filter.search(item->text(0)) > -1;
  }
  
  return true;
}

Q3PopupMenu *FilterWidget::createPopupMenu()
{
  Q3PopupMenu *popup = KLineEdit::createPopupMenu();
  
  Q3PopupMenu *subMenu = new Q3PopupMenu(popup);
  connect(subMenu, SIGNAL(activated(int)), this, SLOT(slotOptionsMenuActivated(int)));
  
  popup->insertSeparator();
  popup->insertItem(i18n("Filter Options"), subMenu);
  
  subMenu->insertItem(i18n("Filter Directories"), FilterWidget::FilterDirectories);
  subMenu->setItemChecked(FilterWidget::FilterDirectories, m_filterDirectories);
  
  subMenu->insertItem(i18n("Filter Symlinks"), FilterWidget::FilterSymlinks);
  subMenu->setItemChecked(FilterWidget::FilterSymlinks, m_filterSymlinks);
  
  subMenu->insertItem(i18n("Case Sensitive"), FilterWidget::CaseSensitive);
  subMenu->setItemChecked(FilterWidget::CaseSensitive, m_caseSensitive);
  
  return popup;
}

void FilterWidget::slotOptionsMenuActivated(int id)
{
  switch (id) {
    case FilterDirectories: m_filterDirectories = !m_filterDirectories; break;
    case FilterSymlinks: m_filterSymlinks = !m_filterSymlinks; break;
    case Qt::CaseSensitive: m_caseSensitive = !m_caseSensitive; break;
    default: break;
  }
  
  updateSearch();
}

}

}

#include "filterwidget.moc"
