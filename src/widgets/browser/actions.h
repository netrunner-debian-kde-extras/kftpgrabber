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

#ifndef KFTPFILEDIRVIEWACTIONS_H
#define KFTPFILEDIRVIEWACTIONS_H

#include <QObject>
#include <KAction>
#include <KActionMenu>
#include <KToggleAction>
#include <KRun>
#include <KService>

namespace KFTPWidgets {

namespace Browser {

class View;

/**
 * This class contains all per-view actions.
 *
 * @author Jernej Kos
 */
class Actions : public QObject
{
Q_OBJECT
friend class View;
friend class DetailsView;
public:
    /**
     * Class constructor.
     *
     * @param parent Parent view widget
     */
    Actions(View *parent);

    /**
     * Initialize view's action collection and it's actions.
     */
    void initActions();
public slots:
    /**
     * Properly enable/disable the available actions.
     */
    void updateActions();
private:
    View *m_view;
    
    int m_curCharsetOption;
    int m_defaultCharsetOption;
    
    KAction *m_goUpAction;
    KAction *m_goBackAction;
    KAction *m_goForwardAction;
    KAction *m_goHomeAction;
    KAction *m_reloadAction;
    
    KAction *m_abortAction;
    KToggleAction *m_toggleTreeViewAction;
    KToggleAction *m_toggleFilterAction;
    
    KAction *m_renameAction;
    KAction *m_deleteAction;
    KAction *m_propsAction;
    KAction *m_shredAction;
    
    KAction *m_copyAction;
    KAction *m_pasteAction;
    
    KActionMenu *m_filterActions;
    KAction *m_alwaysSkipAction;
    KAction *m_topPriorityAction;
    KAction *m_lowPriorityAction;

    KAction *m_transferAction;
    KAction *m_queueTransferAction;
    KAction *m_createDirAction;
    KAction *m_fileEditAction;
    KAction *m_verifyAction;

    KActionMenu *m_moreActions;
    KActionMenu *m_rawCommandsMenu;
    KAction *m_rawCmdAction;
    KActionMenu *m_changeEncodingAction;
    KAction *m_exportListingAction;
    KAction *m_showHiddenFilesAction;
    KAction *m_openExternalAction;
    
    KAction *m_markItemsAction;
    KAction *m_compareAction;
    
    KActionMenu *m_siteChangeAction;
    KAction *m_quickConnectAction;
    KActionMenu *m_connectAction;
    KAction *m_disconnectAction;
private:
    /**
     * Populates the encodings list.
     */
    void populateEncodings();
    
    /**
     * A helper function to add the currently selected item(s) to the
     * priority list with the given priority.
     *
     * @param priority The priority to use
     */
    void addPriorityItems(int priority);
public slots:
    void slotGoUp();
    void slotGoBack();
    void slotGoForward();
    void slotGoHome();
    void slotReload();
    
    void slotAbort();
    void slotToggleTree();
    void slotToggleFilter();
    
    void slotRename();
    void slotDelete();
    void slotProps();
    void slotShred();
    
    void slotCopy();
    void slotPaste();
    
    void slotAlwaysSkip();
    void slotTopPriority();
    void slotLowPriority();

    void slotTransfer();
    void slotQueueTransfer();
    void slotCreateDir();
    void slotFileEdit();
    void slotVerify();
    
    void slotRawCmd();
    void slotCharsetChanged(int);
    void slotCharsetReset(int);
    void slotExportListing();
    void slotShowHiddenFiles();
    void slotOpenExternal();
    
    void slotMarkItems();
    void slotCompare();

    void slotQuickConnect();
    void slotDisconnect();
};

}

}

#endif
