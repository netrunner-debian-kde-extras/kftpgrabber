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

#ifndef KFTPSESSION_H
#define KFTPSESSION_H

#include <QObject>
#include <QPointer>
#include <qdom.h>
#include <QList>
#include <QEvent>

#include <KTabWidget>

#include "kftpqueue.h"
#include "widgets/logview.h"

#include "engine/thread.h"

namespace KFTPWidgets {
  namespace Browser {
    class Actions;
    class View;
  }
}

namespace KFTPBookmarks {
  class Site;
}

namespace KFTPSession {

class Session;

enum Side {
  LeftSide,
  RightSide,
  IgnoreSide
};

#define oppositeSide(x) (x == KFTPSession::LeftSide ? KFTPSession::RightSide : KFTPSession::LeftSide)

/**
 * The Connection class represents a session's connection to a ftp server. There
 * can be many connections in the same session (to the same server), thus providing
 * support for multiple threads at once.
 *
 * Individual transfers must acquire connections before they can use them. When a
 * connection is acquired it cannot be used by another transfer or another remote
 * operation.
 *
 * @author Jernej Kos
 */
class Connection : public QObject
{
Q_OBJECT
public:
    /**
     * Class constructor.
     *
     * @param session The parent session
     * @param primary Set to true if this is session's primary connection
     */
    Connection(Session *session, bool primary = false);
    
    /**
     * Class destructor.
     */
    ~Connection();

    /**
     * Returns the client thread for this connection.
     *
     * @return A KFTPClientThr object representing a client.
     */
    KFTPEngine::Thread *getClient() const { return m_client; }
    
    /**
     * Returns the URL this connection is connected to.
     *
     * @return A KUrl this connection is connected to.
     */
    KUrl getUrl() { return m_client->socket()->getCurrentUrl(); }

    /**
     * Returns the current transfer if this connection is locked by one. If
     * it isn't locked NULL will be returned.
     */
    KFTPQueue::Transfer *getTransfer() const { return m_transfer; }

    /**
     * Lock this connection for a specific transfer. While the connection is
     * locked no other transfer may use it. The connection will be automaticly
     * unlocked when the transfer completes.
     *
     * @param transfer The transfer which is locking this connection.
     */
    void acquire(KFTPQueue::Transfer *transfer);

    /**
     * Release existing connection lock. Only the transfer who locked the connection
     * should do this!
     */
    void release();

    /**
     * Abort any actions going via this connection. It will call abort on the
     * underlying client and emit the aborting signal.
     */
    void abort();
    
    /**
     * Connect to the previously connected URL. If this connection is already
     * established this method does nothing.
     */
    void reconnect();

    /**
     * Can this connection be used to perform an operation ?
     *
     * @return Returns true if the current connection is busy, false otherwise.
     */
    bool isBusy() const;

    /**
     * Is the current connection actually connected to a server ?
     *
     * @return Returns true if a connection to a server is established.
     */
    bool isConnected();
    
    /**
     * Is the current connection the primary session connection ?
     *
     * @return Returns true if the current connection is primary, false otherwise
     */
    bool isPrimary() const { return m_primary; }
    
    /**
     * Scans a directory - usually called from KFTPSession for remote scans.
     *
     * @param parent The transfer that requested the scan
     */
    void scanDirectory(KFTPQueue::Transfer *parent);
private:
    void addScannedDirectory(KFTPEngine::DirectoryTree *tree, KFTPQueue::Transfer *parent);    
private:
    bool m_primary;
    bool m_busy;
    bool m_aborting;
    bool m_scanning;

    QPointer<KFTPQueue::Transfer> m_transfer;
    KFTPEngine::Thread *m_client;
private slots:
    void slotTransferCompleted();
    
    void slotEngineEvent(KFTPEngine::Event *event);
signals:
    /**
     * This signal gets emitted when the connection is acquired for exclusive
     * use by a transfer.
     */
    void connectionAcquired();
    
    /**
     * This signal gets emitted when connection is returned to the pool and is
     * no longer locked.
     */
    void connectionRemoved();
    
    /**
     * This signal gets emitted when the connection is lost.
     *
     * @param connection This connection instance
     */
    void connectionLost(KFTPSession::Connection *connection);
    
    /**
     * This signal gets emitted when connection with the remote server is
     * established.
     */
    void connectionEstablished();

    /**
     * This signal gets emitted when this connection is in the process of
     * aborting any currently running operations.
     */
    void aborting();
};

/**
 * A Session instance connects all the relevant elements together. Via the
 * manager this abstraction allows many independent sessions inside one
 * applications.
 *
 * Each session can either be local or remote. A remote session can have
 * multiple connections open (each connection is represented by a Connection
 * instance).
 *
 * @author Jernej Kos
 */
class Session : public QObject
{
Q_OBJECT
friend class KFTPWidgets::Browser::Actions;
friend class Manager;
friend class Connection;
public:
    /**
     * Class constructor.
     *
     * @param side The side the session should be located on
     */
    Session(Side side);
    
    /**
     * Class destructor.
     */
    ~Session();
    
    /**
     * Has the session registration procedure already been completed ?
     *
     * @return True if the registration procedure has been completed, false otherwise
     */
    bool isRegistred() const { return m_registred; }

    /**
     * Is this a remote session ?
     *
     * @return Returns true if this is a remote session.
     */
    bool isRemote() const { return m_remote; }

    /**
     * Is this session currently active (=visible to the user) ?
     *
     * @return Returns true if this session is currently active.
     */
    bool isActive() const { return m_active; }

    /**
     * Is this session currently connected to a server ? This actually checks if
     * the primary connection is connected to a server.
     *
     * @return Returns true if this session is connected to a server.
     */
    bool isConnected();

    /**
     * Get the site in bookmarks this session is asociated with or NULL if there
     * is no such site.
     *
     * @return Returns the asociated bookmarks site.
     */
    KFTPBookmarks::Site *getSite() const { return m_site; }
    
    /**
     * Returns the session opposite this one.
     */
    Session *oppositeSession() const;

    /**
     * Get the session's client thread. This actually returns the client thread
     * of the primary session's connection.
     *
     * @return A KFTPEngine::Thread object representing a client.
     */
    KFTPEngine::Thread *getClient();

    /**
     * Get this session's log widget.
     *
     * @return Returns the session's log widget.
     */
    KFTPWidgets::LogView *getLog() const { return m_log; }

    /**
     * Get this session's file view.
     *
     * @return Returns the session's file view.
     */
    KFTPWidgets::Browser::View *getFileView() const { return m_fileView; }

    /**
     * Get the side on which this session is located.
     *
     * @return Returns the session's side.
     */
    Side getSide() const { return m_side; }

    /**
     * Set bookmark site asociation.
     *
     * @param site A valid site to which this session is asociated.
     */
    void setSite(KFTPBookmarks::Site *site) { m_site = site; }

    /**
     * Are there any free connections (or if some new can be created) to lock ?
     *
     * @return Returns true if there is a connection that can be locked.
     */
    bool isFreeConnection();

    /**
     * Assigns a free connection if there is one. A connection can be created if
     * the limit hasn't yet been reached. If there are no free connections this
     * method returns NULL.
     *
     * @return A free Connection or NULL if there is none.
     */
    Connection *assignConnection();

    /**
     * Disconnects all connections for this session.
     */
    void disconnectAllConnections();

    /**
     * Get the list of current connections for this session.
     *
     * @return A list of current connections.
     */
    QList<Connection*> *getConnectionList() { return &m_connections; }

    /**
     * Reconnect to a new URL. The current connections will be droped and reconnected
     * to the new URL.
     *
     * @param url The URL to connect to.
     */
    void reconnect(const KUrl &url);

    /**
     * Abort this session. This will actually call abort on all connections for
     * this session.
     */
    void abort();

    /**
     * Initiate a directory scan, adding all new files and directories under the
     * transfer specified as parent. This method will change the current transfer's
     * status to Locked and will return imediately. The actual scan will take
     * place in a separate thread.
     *
     * @param parent The transfer which requested the scan
     * @param connection An optional connection to use
     */
    void scanDirectory(KFTPQueue::Transfer *parent, Connection *connection = 0);
    
    /**
     * Returns the URL of the primary connection.
     */
    KUrl getUrl() { return getClient()->socket()->getCurrentUrl(); }
private:
    Side m_side;
    bool m_remote;
    bool m_active;
    bool m_aborting;
    bool m_registred;

    // Session description
    KFTPBookmarks::Site *m_site;
    KFTPWidgets::LogView *m_log;
    KFTPWidgets::Browser::View *m_fileView;
    KUrl m_lastUrl;
    KUrl m_reconnectUrl;

    // Connection list
    QList<Connection*> m_connections;

    int getMaxThreadCount();
private slots:
    void slotClientEngineEvent(KFTPEngine::Event *event);
    void slotStartReconnect();
signals:
    void dirScanDone();
    void aborting();

    void freeConnectionAvailable();
};

typedef QList<Session*> SessionList;

/**
 * The Manager class provides access to sessions, their registration and deletion.
 *
 * @author Jernej Kos
 */
class Manager : public QObject
{
Q_OBJECT
public:
    /**
     * Get a global manager instance.
     */
    static Manager *self();
    
    /**
     * Class constructor.
     *
     * @param parent Parent object
     * @param stat A widget that contains log tabs
     * @param left A widget that contains sessions on the left side
     * @param right A widget that contains sessions on the right side
     */
    Manager(QObject *parent, QTabWidget *stat, KTabWidget *left, KTabWidget *right);

    /**
     * Spawn a new local (=unconnected) session. This method may reuse an old local session.
     *
     * @param side The side on which the session should be created.
     * @param forceNew Should a new session be created if a similar session already exists.
     * @return Allways returns a valid session.
     */
    Session *spawnLocalSession(Side side, bool forceNew = false);

    /**
     * Spawn a new remote session. This method may reuse an old remote session. It may also
     * spawn a new local session if the URL appears local.
     *
     * @param side The side on which the session should be created.
     * @param remoteUrl URL to which the session should connect upon creation.
     * @param site The bookmarked site the session is connecting to.
     * @param mustUnlock Must the returned session be unlocked ?
     * @return Allways returns a valid session.
     */
    Session *spawnRemoteSession(Side side, const KUrl &remoteUrl, KFTPBookmarks::Site *site = 0, bool mustUnlock = false);

    /**
     * Register a new session with the session manager. Every session calls this method in
     * its constructor to init the session - this method shouldn't be called otherwise.
     *
     * @param session A new session.
     */
    void registerSession(Session *session);

    /**
     * Destroy the session. All connections and transfers related to this session are
     * aborted and disconnected first.
     *
     * @param session The session that is going to be destroyed.
     */
    void unregisterSession(Session *session);
    
    /**
     * Disconnects all sessions and their connections.
     */
    void disconnectAllSessions();

    /**
     * Find a session related to a client thread.
     *
     * @param client The client thread related to a session.
     * @return Returns a valid session if one is found, NULL otherwise.
     */
    Session *find(KFTPEngine::Thread *client);

    /**
     * Find a session related to a file view widget.
     *
     * @param fileView The file view widget related to a session.
     * @return Returns a valid session if one is found, NULL otherwise.
     */
    Session *find(KFTPWidgets::Browser::View *fileView);

    /**
     * Find a session related to a log widget.
     *
     * @param log The log widget related to a session.
     * @return Returns a valid session if one is found, NULL otherwise.
     */
    Session *find(KFTPWidgets::LogView *log);

    /**
     * Find a session by the url it is connected to.
     *
     * @param url The URL a session is connected to.
     * @param mustUnlock Must the session be unlocked ?
     * @return Returns a valid session if one is found, NULL otherwise.
     */
    Session *find(const KUrl &url, bool mustUnlock = false);

    /**
     * Finds a session by its state (remote/local).
     *
     * @param local Must a session be local ?
     * @return Returns a valid session if one is found, NULL otherwise.
     */
    Session *find(bool local);

    /**
     * Finds a session that was last connected to a specific URL that is placed
     * on a specific side.
     *
     * @param url The URL to which the session was connected to.
     * @param side The side where the session must be.
     * @return Returns a valid session if one is found, NULL otherwise.
     */
    Session *findLast(const KUrl &url, Side side);

    /**
     * Get the list of all sessions in existance.
     *
     * @return The session list.
     */
    SessionList *getSessionList() { return &m_sessionList; }

    /**
     * Emits the update signal.
     */
    void doEmitUpdate();

    /**
     * Returns the tab widget that holds the log widgets.
     *
     * @return Returns a QTabWidget that holds the log widgets.
     */
    QTabWidget *getStatTabs() const { return m_statTabs; }

    /**
     * Returns the tab widget that holds the sessions on a specific side.
     *
     * @param side The side of the tab widget.
     * @return Returns a KTabWidget that holds the sessions.
     */
    KTabWidget *getTabs(Side side);

    /**
     * Make a session active (=visible to the user).
     *
     * @param session Session to be made active.
     */
    void setActive(Session *session);

    /**
     * Get the active session on a specific side.
     *
     * @param side The side where the session is active.
     * @return Returns a valid session.
     */
    Session *getActive(Side side);
    
    /**
     * Get the currently active view.
     *
     * @return The active view instance
     */
    KFTPWidgets::Browser::View *getActiveView();
    
    /**
     * Get the currently active session.
     */
    Session *getActiveSession();
protected:
    static Manager *m_self;
    
    /**
     * Event filter handler.
     */
    bool eventFilter(QObject *object, QEvent *event);
    
    /**
     * Change the currently active session to the browser view inheriting
     * the passed object.
     *
     * @param object The object that has browser view as some parent
     */
    void switchFocusToObject(const QObject *object);
private:
    SessionList m_sessionList;

    // These variables should be assigned right after construction
    QTabWidget *m_statTabs;
    KTabWidget *m_leftTabs;
    KTabWidget *m_rightTabs;

    // Currently active sessions
    Session *m_active;
    Session *m_leftActive;
    Session *m_rightActive;
private slots:
    void slotActiveChanged(QWidget *page);
    void slotSwitchFocus();
public slots:
    void slotSessionCloseRequest(QWidget *page);
signals:
    void update();
};

}

#endif
