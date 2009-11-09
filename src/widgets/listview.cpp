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

#include "listview.h"

#include <qpainter.h>
#include <qstringlist.h>
//Added by qt3to4:
#include <QResizeEvent>

namespace KFTPWidgets {

ListView::ListView(QWidget *parent)
 : K3ListView(parent)
{
}


ListView::~ListView()
{
}

void ListView::resizeEvent(QResizeEvent *e)
{
  K3ListView::resizeEvent(e);
  triggerUpdate();
}

void ListView::setEmptyListText(const QString &text)
{
  m_emptyListText = text;
  triggerUpdate();
}

void ListView::drawContentsOffset(QPainter * p, int ox, int oy, int cx, int cy, int cw, int ch)
{
  K3ListView::drawContentsOffset(p, ox, oy, cx, cy, cw, ch);

  if (childCount() == 0 && !m_emptyListText.isEmpty()) {
    p->setPen(Qt::darkGray);

    QStringList lines = m_emptyListText.split("\n");
    int ypos = 10 + p->fontMetrics().height();

    QStringList::Iterator end(lines.end());
    for (QStringList::Iterator str( lines.begin() ); str != end; ++str) {
      p->drawText((viewport()->width()/2)-(p->fontMetrics().width(*str)/2), ypos, *str);
      ypos += p->fontMetrics().lineSpacing();
    }
  }
}

}

#include "listview.moc"
