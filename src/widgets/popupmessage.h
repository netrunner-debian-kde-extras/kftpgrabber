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
#ifndef KFTPWIDGETSPOPUPMESSAGE_H
#define KFTPWIDGETSPOPUPMESSAGE_H

#include "overlaywidget.h"

#include <QBitmap>
#include <QPixmap>
#include <QTimerEvent>

class QLabel;
class KPushButton;
class QVBoxLayout;

namespace KFTPWidgets {

/**
 * Widget that animates itself into a position relative to an anchor widget.
 */
class PopupMessage : public OverlayWidget {
Q_OBJECT
public:
    /**
     * Possible animation effects.
     */
    enum MaskEffect {
      Plain,
      Slide,
      Dissolve
    };
    
    /**
     * Class constructor.
     *
     * @param parent Parent widget
     * @param anchor Which widget to tie the popup widget to
     * @param timeout How long to wait before auto closing
     */
    PopupMessage(QWidget *parent, QWidget *anchor, int timeout = 5000);

    void addWidget(QWidget *widget);
    void setShowCloseButton(bool show);
    void setImage(const QString &location);
    void setImage(const QPixmap &pixmap);
    void setMaskEffect(MaskEffect type) { m_maskEffect = type; }
    void setText(const QString &text);
    void setTimeout(int timeout) { m_timeout = timeout; }
    
    //QSize sizeHint() const;
public slots:
    void close();
    void display();
protected:
    void timerEvent(QTimerEvent *event);
    void countDown();
    
    void dissolveMask();
    void plainMask();
    void slideMask();
private:
    QLabel *m_icon;
    QLabel *m_text;
    KPushButton *m_close;
    
    QVBoxLayout *m_layout;
    QWidget *m_anchor;
    QWidget *m_parent;
    QBitmap m_mask;
    MaskEffect m_maskEffect;
    
    int m_dissolveSize;
    int m_dissolveDelta;
    
    int m_offset;
    int m_counter;
    int m_stage;
    int m_timeout;
    int m_timerId;
};

}

#endif
