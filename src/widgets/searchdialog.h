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

#ifndef KFTPWIDGETSSEARCHDIALOG_H
#define KFTPWIDGETSSEARCHDIALOG_H

#include <qdom.h>
#include <kdialogbase.h>

namespace KFTPQueue {
  class Transfer;
  class QueueObject;
}

namespace KFTPBookmarks {
  class Site;
}

class KFTPSearchLayout;

namespace KFTPWidgets {

/**
 * This dialog provides search & replace functionality for queued transfers.
 *
 * @author Jernej Kos
 */
class SearchDialog : public KDialogBase
{
Q_OBJECT
public:
    /**
     * Class constructor.
     */
    SearchDialog(QWidget *parent = 0, const char *name = 0);
private:
    KFTPSearchLayout *m_layout;
    
    void replace(KFTPQueue::Transfer *i);
    
    QString replaceCap(QStringList cap, const QString &text);
    void searchAndReplace(KFTPQueue::QueueObject *parent);
    void searchAndReplace();
private slots:
    virtual void slotOk();
    
    void slotSearchServerClicked();
    void slotSiteChanged(KFTPBookmarks::Site *site);
};

}

#endif
