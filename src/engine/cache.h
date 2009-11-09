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
 *
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#ifndef KFTPENGINECACHE_H
#define KFTPENGINECACHE_H

#include <QMap>
#include <KUrl>

#include "directorylisting.h"

namespace KFTPEngine {

class Socket;
class CachePrivate;

/**
 * This class provides a cache of paths and directory listings to be used for
 * faster operations.
 *
 * @author Jernej Kos <kostko@jweb-network.net>
 */
class Cache {
friend class CachePrivate;
public:
    static Cache *self();

    /**
     * Cache a directory listing.
     *
     * @param url The listing url (including host information)
     * @param listing The directory listing to cache
     */
    void addDirectory(KUrl &url, DirectoryListing listing);
    
    /**
     * Cache a directory listing, extracting the host information from the
     * socket and using the current directory path.
     *
     * @param socket The socket to extract the host info from
     * @param listing The directory listing to cache
     */
    void addDirectory(Socket *socket, DirectoryListing listing);
    
    /**
     * Updates a single directory entry.
     *
     * @param socket The socket to extract the host info from
     * @param path Entry location
     * @param filesize New file size
     */
    void updateDirectoryEntry(Socket *socket, KUrl &path, filesize_t filesize);
    
    /**
     * Cache path information.
     *
     * @param url The url (including host information)
     * @param target Actual target directory
     */
    void addPath(KUrl &url, const QString &target);
    
    /**
     * Cache path information, extracting the host information from the
     * socket and using the current directory path.
     *
     * @param socket The socket to extract the host info from
     * @param target Actual target directory
     */
    void addPath(Socket *socket, const QString &target);
    
    /**
     * Invalidate a cached entry.
     *
     * @param url Url of the entry
     */
    void invalidateEntry(KUrl &url);
    
    /**
     * Invalidate a cached entry.
     *
     * @param socket The socket to extract the host info from
     * @param path Path of the entry
     */
    void invalidateEntry(Socket *socket, const QString &path);
    
    /**
     * Invalidate a cached path.
     *
     * @param url Url of the entry
     */
    void invalidatePath(KUrl &url);
    
    /**
     * Invalidate a cached path.
     *
     * @param socket The socket to extract the host info from
     * @param path Path of the entry
     */
    void invalidatePath(Socket *socket, const QString &path);
    
    /**
     * Retrieve a cached directory listing.
     *
     * @param url Url of the entry
     * @return A valid DirectoryListing if found, an empty DirectoryListing otherwise
     */
    DirectoryListing findCached(KUrl &url);
    
    /**
     * Retrieve a cached directory listing.
     *
     * @param socket The socket to extract the host info from
     * @param path Path of the entry
     * @return A valid DirectoryListing if found, an empty DirectoryListing otherwise
     */
    DirectoryListing findCached(Socket *socket, const QString &path);
    
    /**
     * Retrieve a cached path.
     *
     * @param url Url of the entry
     * @return A target path if found, QString::null otherwise
     */
    QString findCachedPath(KUrl &url);
    
    /**
     * Retrieve a cached path.
     *
     * @param socket The socket to extract the host info from
     * @param path Path of the entry
     * @return A target path if found, QString::null otherwise
     */
    QString findCachedPath(Socket *socket, const QString &path);
protected:
    /**
     * Class constructor.
     */
    Cache();
    
    /**
     * Class destructor.
     */
    ~Cache();
private:
    QMap<KUrl, DirectoryListing> m_listingCache;
    QMap<KUrl, QString> m_pathCache;
};

}

#endif
