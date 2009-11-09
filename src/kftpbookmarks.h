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

#ifndef KFTPBOOKMARKS_H
#define KFTPBOOKMARKS_H

#include <qstring.h>
#include <qdom.h>

#include <QList>
#include <QHash>

#include <KMenu>
#include <KAction>
#include <KActionMenu>
#include <KUrl>

namespace KFTPEngine {
  class Thread;
}

namespace KFTPSession {
  class Session;
}

namespace KFTPWidgets {
namespace Bookmarks {
  class ListView;
  class ListViewItem;
}
}

#define KFTP_BOOKMARKS_VERSION 3

namespace KFTPBookmarks {

class Manager;
class ManagerPrivate;

enum SiteType {
  ST_SITE,
  ST_CATEGORY,
  ST_ROOT
};

class BookmarkActionData {
public:
    QString siteId;
    KFTPSession::Session *session;
};

class Site {
friend class Manager;
public:
    /**
     * Currently valid remote protocols.
     */
    enum Protocol {
      ProtoFtp = 0,
      ProtoSftp
    };
    
    /**
     * Currently valid SSL negotiation modes.
     */
    enum SslNegotiationMode {
      SslNone = 0,
      SslAuthTLS,
      SslImplicit
    };
    
    Site(Manager *manager, QDomNode node);
    ~Site();
    
    /**
     * Refreshes this site by using the new node as the site
     * element.
     *
     * @param manager A valid Manager instance
     * @param node A valid site node
     */
    void refresh(Manager *manager, QDomNode node);
    
    void reparentSite(Site *site);
    
    Site *addSite(const QString &name);
    void addCategory(const QString &name);
    
    KUrl getUrl();
    Site *getParentSite();
    
    /**
     * Returns the child site identified by index. Note that NULL will
     * be returned if this is not a category!
     *
     * @param index Sequential child index
     * @return A valid Site instance or NULL if index is invalid
     */
    Site *child(int index);
    
    /**
     * Returns this node's relative index to the parent node.
     */
    int index() const;
    
    /**
     * Returns the number of child sites if this is a category node.
     * Otherwise this returns 0.
     */
    uint childCount() const;
    
    /**
     * Returns true if this Site instance represents the root node.
     */
    bool isRoot() const { return m_type == ST_ROOT; }
    
    /**
     * Returns true if this Site instance represents a category.
     */
    bool isCategory() const { return m_type == ST_CATEGORY; }
    
    /**
     * Returns true if this Site instance represents a site.
     */
    bool isSite() const { return m_type == ST_SITE; }
    
    /**
     * Returns this site's protocol.
     */
    int protocol() const { return getIntProperty("protocol"); }
    
    /**
     * Returns the bookmark manager associated with this site.
     */
    Manager *manager() const { return m_manager; }
    
    Site *duplicate();
    
    QString getProperty(const QString &name) const;
    int getIntProperty(const QString &name) const;
    
    void setProperty(const QString &name, const QString &value);
    void setProperty(const QString &name, int value);
    
    void setAttribute(const QString &name, const QString &value, bool notify = true);
    QString getAttribute(const QString &name) const;
    
    SiteType type() const { return m_type; }   
    QString id() const { return m_id; }
private:
    Manager *m_manager;
    SiteType m_type;
    QString m_id;
    QDomElement m_element;
};

class Manager : public QObject {
friend class Site;
friend class ManagerPrivate;
Q_OBJECT
public:
    /**
     * Returns the global Manager instance.
     */
    static Manager *self();
    
    /**
     * Class constructor.
     */
    Manager();
    
    /**
     * Class constructor.
     *
     * @param manager A manager whose contents to copy
     */
    Manager(const Manager *manager);
    
    /**
     * Class destructor.
     */
    ~Manager();
    
    void setBookmarks(KFTPBookmarks::Manager *bookmarks);
    void importSites(QDomNode node);
    
    void load(const QString &filename);
    void save();
    
    /**
     * Returns a (possibly cached) Site instance for a given node.
     *
     * @param node A valid QDomNode instance
     * @return A valid Site instance for the given node
     */
    Site *siteForNode(QDomNode node);
    
    /**
     * Adds the given site to the site cache.
     *
     * @param site The site to insert
     */
    void cacheSite(Site *site);
    
    /**
     * Returns the root site.
     */
    Site *rootSite();
    
    Site *findSite(const QString &id);
    Site *findSite(const KUrl &url) KDE_DEPRECATED;
    
    Site *findCategory(const QString &id);
    
    /**
     * Removes the specified site.
     *
     * @param site The site instance to remove
     */
    void removeSite(Site *site);
    
    /**
     * Configures the specified client with site parameters.
     *
     * @param site Site instance to get the parameters from
     * @param client Client thread to configure
     * @param primary Optional primary client thread for secondary connections
     */
    void setupClient(Site *site, KFTPEngine::Thread *client, KFTPEngine::Thread *primary = 0);
    
    /**
     * Populates the given KActionMenu with bookmark actions.
     *
     * @param parentMenu Optional menu to use as root
     * @param session Optional session pointer
     * @return If no parentMenu was specified a list of actions is returned
     */
    QList<QAction*> populateBookmarksMenu(KActionMenu *parentMenu = 0, KFTPSession::Session *session = 0);

    /**
     * Populates the given KActionMenu with published Zeroconf services.
     *
     * @param parentMenu Menu to use as root
     */
    void populateZeroconfMenu(KActionMenu *parentMenu);
    
    /**
     * Populates the given KActionMenu with sites from KWallet.
     *
     * @param parentMenu Menu to use as root
     */
    void populateWalletMenu(KActionMenu *parentMenu);
    
    /**
     * Establishes a connection with the given site.
     *
     * @param site A valid Site instance
     * @param session Optional session instance
     */
    void connectWithSite(Site *site, KFTPSession::Session *session = 0);
    
    void emitUpdate() { emit update(); }
protected:
    static Manager *m_self;
private:
    QHash<QString, Site*> m_siteCache;
    QDomDocument m_document;
    Site *m_rootSite;
    
    QString m_decryptKey;
    QString m_filename;
    
    QDomNode findSiteElementByUrl(const KUrl &url, QDomNode parent = QDomNode());
    QDomNode findSiteElementById(const QString &id, QDomNode parent = QDomNode());
    QDomNode findCategoryElementById(const QString &id, QDomNode parent = QDomNode());
    
    // Validation
    void validate(QDomNode node = QDomNode());
    
    // XML conversion methods
    void versionUpdate();
    void versionFrom1Update(QDomNode parent = QDomNode());
    void versionFrom2Update(QDomNode parent = QDomNode());
private slots:
    void slotBookmarkExecuted();
    void slotZeroconfExecuted();
    void slotWalletExecuted();
signals:
    /**
     * Emitted when bookmarks get updated.
     */
    void update();
    
    /**
     * Emitted when a site gets added.
     */
    void siteAdded(KFTPBookmarks::Site *site);
    
    /**
     * Emitted after a site has been removed.
     */
    void siteRemoved(KFTPBookmarks::Site *site);
    
    /**
     * Emitted when site's properties change.
     */
    void siteChanged(KFTPBookmarks::Site *site);
};

}

Q_DECLARE_METATYPE(KFTPBookmarks::Site*)
Q_DECLARE_METATYPE(KFTPBookmarks::BookmarkActionData)

#endif
