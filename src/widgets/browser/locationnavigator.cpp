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
#include "locationnavigator.h"
#include "detailsview.h"
#include "dirmodel.h"
#include <QScrollBar>

namespace KFTPWidgets {

namespace Browser {

LocationNavigator::Element::Element()
  : m_url(),
    m_contentsX(0),
    m_contentsY(0)
{
}

LocationNavigator::Element::Element(const KUrl &url)
  : m_url(url),
    m_contentsX(0),
    m_contentsY(0)
{
}

LocationNavigator::LocationNavigator(DetailsView *view)
  : m_view(view),
    m_historyIndex(0)
{
  connect(view, SIGNAL(contentsMoved(int, int)), this, SLOT(slotContentsMoved(int, int)));
}

void LocationNavigator::setUrl(const KUrl &url)
{
  if (m_historyIndex > 0) {
    const KUrl &nextUrl = m_history[m_historyIndex - 1].url();
    
    if (url == nextUrl) {
      goForward();
      return;
    }
  }
  
  // Check for duplicates
  if (m_history.count() > m_historyIndex) {
    const KUrl &currentUrl = m_history[m_historyIndex].url();
    if (currentUrl == url)
      return;
  }
  
  updateCurrentElement();
  m_history.insert(m_historyIndex, Element(url));
  
  emit urlChanged(url);
  emit historyChanged(url);
  
  // Cleanup history when it becomes too big
  if (m_historyIndex > 100) {
    m_history.removeFirst();
    m_historyIndex--;
  }
}

const KUrl& LocationNavigator::url() const
{
  return m_history[m_historyIndex].url();
}

const QList<LocationNavigator::Element> LocationNavigator::history(int &index) const
{
  index = m_historyIndex;
  return m_history;
}

void LocationNavigator::goBack()
{
  updateCurrentElement();
  const int count = m_history.count();
  
  if (m_historyIndex < count - 1) {
    m_historyIndex++;
    
    emit urlChanged(url());
    emit historyChanged(url());
  }
}

void LocationNavigator::goForward()
{
  if (m_historyIndex > 0) {
    m_historyIndex--;
    
    emit urlChanged(url());
    emit historyChanged(url());
  }
}

void LocationNavigator::goUp()
{
  setUrl(url().upUrl());
}

void LocationNavigator::goHome()
{
  setUrl(m_homeUrl);
}

void LocationNavigator::clear()
{
  Element element = m_history[m_historyIndex];
  
  m_history.clear();
  m_historyIndex = 0;
  
  m_history.append(element);
}

void LocationNavigator::slotContentsMoved(int x, int y)
{
  m_history[m_historyIndex].setContentsX(x);
  m_history[m_historyIndex].setContentsY(y);
}

void LocationNavigator::updateCurrentElement()
{
  if (m_history.isEmpty())
    return;
  
  QModelIndex currentIndex = m_view->currentIndex();
  
  if (currentIndex.isValid()) {
    const KFileItem item = currentIndex.data(DirModel::FileItemRole).value<KFileItem>();
    m_history[m_historyIndex].setCurrentItem(item);
  }
  
  m_history[m_historyIndex].setContentsX(m_view->horizontalScrollBar()->value());
  m_history[m_historyIndex].setContentsY(m_view->verticalScrollBar()->value());
}

}

}

#include "locationnavigator.moc"

