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
#ifndef KFTPWIDGETS_BROWSERDIRLISTER_H
#define KFTPWIDGETS_BROWSERDIRLISTER_H

#include <QObject>

#include <KUrl>
#include <KDirLister>

namespace KFTPSession {
  class Session;
}

namespace KFTPEngine {
  class Event;
}

namespace KFTPWidgets {

namespace Browser {

/**
 * A wrapped KDirLister for proper error reporting.
 */
class ReportingDirLister : public KDirLister {
Q_OBJECT
public:
    /**
     * Class constructor.
     */
    ReportingDirLister(QObject *parent);
    
    /**
     * Class destructor.
     */
    virtual ~ReportingDirLister();
signals:
    /**
     * Emitted when an error ocurrs.
     */
    void errorMessage(const QString &message);
protected:
    /**
     * @overload
     * Reimplemented from KDirLister for error handling.
     */
    virtual void handleError(KIO::Job *job);
};

/**
 * This class is a wrapper around KDirLister to support remote listings
 * via engine sockets.
 *
 * @author Jernej Kos
 */
class DirLister : public QObject {
Q_OBJECT
public:
    /**
     * Current mode of operation.
     */
    enum Mode {
      None,
      Local,
      Remote
    };
    
    /**
     * Class constructor.
     *
     * @param parent Parent object
     */
    DirLister(QObject *parent);
    
    /**
     * Class destructor.
     */
    ~DirLister();
    
    /**
     * Set the remote session. If you do not set a valid session, remote operations
     * will always fail.
     *
     * @param session A valid session
     */
    void setSession(KFTPSession::Session *session);
    
    /**
     * Changes the "show hidden files" setting.
     *
     * @param value True to enable showing hidden files, false otherwise
     */
    void setShowingDotFiles(bool value) { m_showHidden = value; }
    
    /**
     * Changes the "show only directories" setting.
     *
     * @param value True to list only directories, false otherwise
     */
    void setDirOnlyMode(bool value) { m_dirOnly = value; }
    
    /**
     * Sets the ignore changes flag. Note that this does not change the
     * behavior of this directory lister, it is just a flag for its
     * subscribers (like the DirModel) to use.
     *
     * @param value True to set the ignore changes flag, false otherwise
     */
    void setIgnoreChanges(bool value) { m_ignoreChanges = value; }
    
    /**
     * Fetch a specific location.
     *
     * @param url The URL to fetch
     * @param flags see KDirLister
     */
    void openUrl(const KUrl &url, KDirLister::OpenUrlFlags _flags = KDirLister::NoFlags );
    
    /**
     * Returns the last open URL.
     */
    KUrl lastUrl() const { return m_lastUrl; }
    
    /**
     * Returns a file item representing the root directory.
     */
    KFileItem *rootItem() const;
    
    /**
     * Returns the session associated with this directory lister.
     */
    KFTPSession::Session *session() const { return m_remoteSession; }
    
    /**
     * Returns the state of ignore changes flag.
     */
    bool ignoringChanges() const { return m_ignoreChanges; }
    
    /**
     * Stop the current listing operation.
     */
    void stop();
protected:
    void setRemoteEnabled(bool enabled, bool withoutLocal = false);
private:
    ReportingDirLister *m_localLister;
    KFTPSession::Session *m_remoteSession;
    KFileItemList m_items;
    KUrl m_lastUrl;
    
    bool m_showHidden;
    bool m_dirOnly;
    bool m_ignoreChanges;
    Mode m_mode;
private slots:
    void slotRemoteEngineEvent(KFTPEngine::Event *event);
signals:
    /**
     * Emitted when the listing operation has been completed.
     */
    void completed();
    
    /**
     * Emitted when there are new items.
     */
    void newItems(KFileItemList items);
    
    /**
     * Emitted when an item has to be removed.
     */
    void deleteItem(const KFileItem &item);
    
    /**
     * Emitted when items should be refreshed.
     */
    void refreshItems(const QList<QPair<KFileItem, KFileItem> > &items);
    
    /**
     * Emitted when all items should be cleared.
     */
    void clear();
    
    /**
     * Emitted when an error ocurrs and a message should be displayed.
     *
     * @param message The message.
     */
    void errorMessage(const QString &message);
    
    /**
     * Emitted when site changes from local to remote or vice-versa.
     *
     * @param url New site URL
     */
    void siteChanged(const KUrl &url);
};

}

}

#endif
