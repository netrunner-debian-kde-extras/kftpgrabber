/*
 * This file is part of the KFTPGrabber project
 *
 * Copyright (C) 2003-2006 by the KFTPGrabber developers
 * Copyright (C) 2003-2006 Jernej Kos <kostko@jweb-network.net>
 * Copyright (C) 2005 Max Howell <max.howell@methyblue.com>
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
#include "overlaywidget.h"

#include <qpoint.h>

namespace KFTPWidgets {

OverlayWidget::OverlayWidget(QWidget *parent, QWidget *anchor)
  : QFrame(parent->parentWidget()),
    m_parent(parent),
    m_anchor(anchor)
{
  parent->installEventFilter(this);
  hide();
}

void OverlayWidget::reposition()
{
  setMaximumSize(parentWidget()->size());
  adjustSize();
  
  // P is in the alignWidget's coordinates
  QPoint p;
  
  p.setX(m_anchor->width() - width());
  p.setY(-height());
  
  // Position in the toplevelWidget's coordinates
  QPoint pTopLevel = m_anchor->mapTo(topLevelWidget(), p);
  
  // Position in the widget's parentWidget coordinates
  QPoint pParent = parentWidget()->mapFrom(topLevelWidget(), pTopLevel);
  
  if (pParent.x() < 0)
    pParent.rx() = 0;
  
  move(pParent);
}

bool OverlayWidget::event(QEvent *event)
{
  if (event->type() == QEvent::ChildAdded)
    adjustSize();
  
  return QFrame::event(event);
}

}
