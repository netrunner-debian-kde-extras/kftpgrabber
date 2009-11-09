/*
 * This file is part of the KFTPGrabber project
 *
 * Copyright (C) 2003-2006 by the KFTPGrabber developers
 * Copyright (C) 2003-2006 Jernej Kos <kostko@jweb-network.net>
 * Copyright (C) 2005 Max Howell <max.howell@methyblue.com>
 * Copyright (C) 2005 Seb Ruiz <me@sebruiz.net>
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
#include "popupmessage.h"

#include <kpushbutton.h>
#include <kstandardguiitem.h>
#include <kguiitem.h>

#include <qfont.h>
#include <QFrame>
#include <QLabel>
#include <QPainter>
#include <QTimer>
#include <QToolTip>
#include <QTimerEvent>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace KFTPWidgets {

PopupMessage::PopupMessage(QWidget *parent, QWidget *anchor, int timeout)
  : OverlayWidget(parent, anchor),
    m_anchor(anchor),
    m_parent(parent),
    m_maskEffect(Slide),
    m_dissolveSize(0),
    m_dissolveDelta(-1),
    m_offset(0),
    m_counter(0),
    m_stage(1),
    m_timeout(timeout)
{
  setFrameStyle(QFrame::StyledPanel);
  setWindowFlags(Qt::WX11BypassWM);
  setMinimumSize(360, 78);

  QPalette p = QToolTip::palette();
  setPalette(p);
  setAutoFillBackground(true);

  QHBoxLayout *hbox;
  QLabel *label;
  QLabel *alabel;

  m_layout = new QVBoxLayout(this);
  m_layout->setMargin(5);
  m_layout->setSpacing(5);
  
  hbox = new QHBoxLayout();
  hbox->setParent(m_layout);
  hbox->setSpacing(12);
  m_layout->addLayout(hbox);

  // Setup the icon widget
  m_icon = new QLabel(this);
  hbox->addWidget(m_icon);

  // Setup the text widget
  m_text = new QLabel(this);
  m_text->setTextFormat(Qt::RichText);
  m_text->setWordWrap(true);
  m_text->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
  m_text->setPalette(p);
  hbox->addWidget(m_text);

  hbox = new QHBoxLayout();
  hbox->setParent(m_layout);
  hbox->addItem(new QSpacerItem(4, 4, QSizePolicy::Expanding, QSizePolicy::Preferred));
  m_layout->addLayout(hbox);
  
  m_close = new KPushButton(KStandardGuiItem::close(), this);
  hbox->addWidget(m_close);
  connect(m_close, SIGNAL(clicked()), SLOT(close()));
}

void PopupMessage::addWidget(QWidget *widget)
{
  m_layout->addWidget(widget);
  adjustSize();
}

void PopupMessage::setShowCloseButton(bool show)
{
  m_close->setVisible(show);
  adjustSize();
}

void PopupMessage::setText(const QString &text)
{
  m_text->setText(text);
  adjustSize();
}

void PopupMessage::setImage(const QString &location)
{
  m_icon->setPixmap(QPixmap(location));
  adjustSize();
}

void PopupMessage::setImage(const QPixmap &pix)
{
  m_icon->setPixmap(pix);
  adjustSize();
}

void PopupMessage::close()
{
  m_stage = 3;
  killTimer(m_timerId);
  m_timerId = startTimer(6);
}

void PopupMessage::display()
{
  m_dissolveSize = 24;
  m_dissolveDelta = -1;

  if (m_maskEffect == Dissolve) {
    m_mask = QPixmap(width(), height());
    dissolveMask();
    m_timerId = startTimer(1000 / 30);
  } else {
    m_timerId = startTimer( 6 );
  }
  
  show();
}

void PopupMessage::timerEvent(QTimerEvent*)
{
  switch(m_maskEffect) {
    case Plain: plainMask(); break;
    case Slide: slideMask(); break;
    case Dissolve: dissolveMask(); break;
  }
}

void PopupMessage::countDown()
{
  if (!m_timeout) {
    killTimer(m_timerId);
    return;
  }

  if (!testAttribute(Qt::WA_UnderMouse))
    m_counter++;

  if (m_counter > 20) {
    m_stage = 3;
    killTimer(m_timerId);
    m_timerId = startTimer(6);
  } else {
    killTimer(m_timerId);
    m_timerId = startTimer(m_timeout / 20);
  }
}

void PopupMessage::dissolveMask()
{
  if (m_stage == 1) {
    repaint();
    QPainter maskPainter(&m_mask);

    m_mask.fill(Qt::black);

    maskPainter.setBrush(Qt::white);
    maskPainter.setPen(Qt::white);
    maskPainter.drawRect(m_mask.rect());

    m_dissolveSize += m_dissolveDelta;

    if (m_dissolveSize > 0) {
      maskPainter.setCompositionMode(QPainter::CompositionMode_SourceOut);

      int x, y, s;
      const int size = 16;

      for (y = 0; y < height() + size; y += size) {
        x = width();
        s = m_dissolveSize * x / 128;

        for (; x > size; x -= size, s -= 2) {
          if (s < 0)
            break;

          maskPainter.drawEllipse(x - s / 2, y - s / 2, s, s);
        }
      }
    } else if (m_dissolveSize < 0) {
      m_dissolveDelta = 1;
      killTimer(m_timerId);

      if (m_timeout) {
        m_timerId = startTimer(40);
        m_stage = 2;
      }
    }

    setMask(m_mask);
  } else if (m_stage == 2) {
    countDown();
  } else {
    deleteLater();
  }
}

void PopupMessage::plainMask()
{
  switch (m_stage) {
    case 1: {
      // Raise
      killTimer(m_timerId);
      
      if (m_timeout) {
        m_timerId = startTimer(40);
        m_stage = 2;
      }

      break;
    }
    
    case 2: {
      // Counter
      countDown();
      break;
    }
    
    case 3: {
      // Lower/Remove
      deleteLater();
      break;
    }
  }
}

void PopupMessage::slideMask()
{
  switch (m_stage) {
    case 1: {
      // Raise
      move(0, m_parent->y() - m_offset);
      m_offset++;
      
      if (m_offset > height()) {
        killTimer(m_timerId);

        if (m_timeout) {
          m_timerId = startTimer(40);
          m_stage = 2;
        }
      }
      break;
    }

    case 2: {
      // Fill in pause timer bar
      countDown();
      break;
    }

    case 3: {
      // Lower
      m_offset--;
      move(0, m_parent->y() - m_offset);

      if (m_offset < 0)
        deleteLater();
    }
  }
}

}

#include "popupmessage.moc"
