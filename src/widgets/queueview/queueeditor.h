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

#ifndef KFTPQUEUEEDITOR_H
#define KFTPQUEUEEDITOR_H

#include "kftpqueue.h"
#include "kftpbookmarks.h"

#include <kdialogbase.h>
#include <qdom.h>

class KFTPQueueEditorLayout;

namespace KFTPWidgets {

/**
@author Jernej Kos
*/
class QueueEditor : public KDialogBase
{
Q_OBJECT
public:
    QueueEditor(QWidget *parent = 0, const char *name = 0);

    void setData(KFTPQueue::Transfer *transfer);
    void saveData();
private:
    KFTPQueueEditorLayout *m_layout;
    KFTPQueue::Transfer *m_transfer;
    KFTPQueue::TransferType m_lastTransferType;

    void resetTabs();
    void resetServerData();

    bool sourceIsValid();
    bool destIsValid();
    
    void recursiveSaveData(KFTPQueue::TransferDir *parent, const KUrl &srcUrl, const KUrl &dstUrl);
private slots:
    void slotTextChanged();
    void slotTransferModeChanged(int index);
    
    void slotSourceSiteChanged(KFTPBookmarks::Site *site);
    void slotDestSiteChanged(KFTPBookmarks::Site *site);
};

}

#endif
