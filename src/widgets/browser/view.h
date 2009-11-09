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

#ifndef KFTPFILEDIRVIEW_H
#define KFTPFILEDIRVIEW_H

#include "browser/locationnavigator.h"

#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QSplitter>
#include <QPointer>
#include <QModelIndex>

#include <KUrl>
#include <KFileItem>

namespace KFTPSession {
  class Session;
  class Manager;
}

class KToolBar;
class KHistoryComboBox;

namespace KFTPEngine {
  class Thread;
  class Event;
}

namespace KFTPWidgets {

class PopupMessage;

namespace Browser {

class DetailsView;
class ListView;
class TreeView;
class Actions;
class FilterWidget;
class DirLister;
class DirModel;
class DirSortFilterProxyModel;

/**
 * @author Jernej Kos
 */
class View : public QWidget
{
Q_OBJECT
friend class Actions;
friend class DetailsView;
friend class ListView;
friend class TreeView;
friend class KFTPSession::Manager;
friend class KFTPSession::Session;
public:
    /**
     * Class constructor.
     */
    View(QWidget *parent, KFTPEngine::Thread *client, KFTPSession::Session *session);
    
    /**
     * Class destructor.
     */
    ~View();
    
    /**
     * Changes the visibility of tree widget.
     *
     * @param visible True to display the tree widget, false to hide it
     */
    void setTreeVisible(bool visible);
    
    /**
     * Changes the "show hidden files" setting.
     *
     * @param value True to enable showing hidden files, false otherwise
     */
    void setShowHidden(bool value);
    
    /**
     * Set the home URL.
     *
     * @param url URL to use as home URL
     */
    void setHomeUrl(const KUrl &url);
    
    /**
     * Go one history hop back.
     */
    void goBack();
    
    /**
     * Go one history hop forward.
     */
    void goForward();
    
    /**
     * Go up in the directory structure.
     */
    void goUp();
    
    /**
     * Go the the predefined home URL.
     */
    void goHome();
    
    /**
     * Reload the current directory listing.
     */
    void reload();
    
    /**
     * Renames the provided source file to a new name.
     */
    void rename(const KUrl &source, const QString &name);
    
    /**
     * Returns the details view widget.
     */
    DetailsView *detailsView() const { return m_detailsView; }

    /**
     * Returns the tree view widget.
     */
    TreeView *treeView() const { return m_treeView; }
    
    /**
     * Returns the status label widget.
     */
    QLabel *statusLabel() const { return m_statusMsg; }
    
    /**
     * Returns the associated session.
     */
    KFTPSession::Session *session() const { return m_session; }
    
    /**
     * Returns the location navigator.
     */
    LocationNavigator *locationNavigator() const { return m_locationNavigator; }
    
    /**
     * Returns the directory lister.
     */
    DirLister *dirLister() const { return m_dirLister; }
    
    /**
     * Opens the context menu for the specified index at the specified
     * position.
     *
     * @param indexes A list of selected indexes
     * @param pos Menu position
     */
    void openContextMenu(const QModelIndexList &indexes, const QPoint &pos);
    
    /**
     * Returns the indexes selected by the last operation that requested
     * a context menu.
     */
    QModelIndexList selectedIndexes() const { return m_currentIndexes; }
public slots:
    /**
     * Open an URL. Note that if a remote URL is specified the session needs to
     * be connected to the specified host!
     *
     * @param url URL to open
     */
    void openUrl(const KUrl &url);
    
    /**
     * Opens an URL held by the specified index.
     *
     * @param index A valid model index
     */
    void openIndex(const QModelIndex &index);
    
    /**
     * Displays an error popup message.
     */
    void showError(const QString &message);
    
    /**
     * Displays a nice popup message.
     */
    void showMessage(const QString &message);
protected:
    /**
     * Initialize the widget.
     */
    void init();
    
    /**
     * Populate the toolbar.
     */
    void populateToolbar();
private:
    KFTPSession::Session *m_session;
    KFTPEngine::Thread *m_ftpClient;
    
    DetailsView *m_detailsView;
    TreeView *m_treeView;
    QModelIndexList m_currentIndexes;
    
    Actions *m_actions;

    KToolBar *m_toolBarFirst;
    KToolBar *m_toolBarSecond;
    //KToolBar *m_searchToolBar;

    QLabel *m_statusMsg;
    QLabel *m_connDurationMsg;
    QPushButton *m_statusIcon;
    QSplitter *m_splitter;
    QPointer<PopupMessage> m_infoMessage;

    QTimer *m_connTimer;
    QTime m_connDuration;

    KHistoryComboBox *m_historyCombo;
    //FilterWidget *m_searchFilter;
    
    LocationNavigator *m_locationNavigator;
    DirLister *m_dirLister;
    DirModel *m_dirModel;
    DirSortFilterProxyModel *m_proxyModel;
    bool m_freezeUrlUpdates;
    bool m_treeVisibilityChanged;
public slots:
    void updateBookmarks();
private slots:
    void slotHistoryActivated();
    void slotHistoryActivated(const QString &text);
    void slotHistoryChanged(const KUrl &url);
    void slotUrlChanged(const KUrl &url);
    void slotListingCompleted();
    
    void slotDisplayCertInfo();
    void slotDurationUpdate();

    void slotEngineEvent(KFTPEngine::Event *event);

    void slotConfigUpdate();
signals:
    /**
     * Emitted when the URL has been changed.
     *
     * @param url The new URL
     */
    void urlChanged(const KUrl &url);
};

}

}

#endif
