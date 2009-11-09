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

#include "cache.h"
#include "socket.h"

#include <KGlobal>

namespace KFTPEngine {

class CachePrivate {
public:
    Cache instance;
};

K_GLOBAL_STATIC(CachePrivate, cachePrivate)

Cache *Cache::self()
{
  return &cachePrivate->instance;
}

Cache::Cache()
{
}

Cache::~Cache()
{
}

void Cache::addDirectory(KUrl &url, DirectoryListing listing)
{
  url.adjustPath(KUrl::RemoveTrailingSlash);
  m_listingCache[url] = listing;
}

void Cache::addDirectory(Socket *socket, DirectoryListing listing)
{
  KUrl url = socket->getCurrentUrl();
  url.setPath(socket->getCurrentDirectory());
  
  addDirectory(url, listing);
}

void Cache::updateDirectoryEntry(Socket *socket, KUrl &path, filesize_t filesize)
{
  KUrl url = socket->getCurrentUrl();
  url.setPath(path.directory());
  url.adjustPath(KUrl::RemoveTrailingSlash);
  
  if (m_listingCache.contains(url)) {
    DirectoryListing listing = m_listingCache[url];
    listing.updateEntry(path.fileName(), filesize);
    
    m_listingCache.insert(url, listing);
  }
}

void Cache::addPath(KUrl &url, const QString &target)
{
  url.adjustPath(KUrl::RemoveTrailingSlash);
  m_pathCache[url] = target;
}

void Cache::addPath(Socket *socket, const QString &target)
{
  KUrl url = socket->getCurrentUrl();
  url.setPath(socket->getCurrentDirectory());
  
  addPath(url, target);
}

void Cache::invalidateEntry(KUrl &url)
{
  url.adjustPath(KUrl::RemoveTrailingSlash);
  m_listingCache.remove(url);
}

void Cache::invalidateEntry(Socket *socket, const QString &path)
{
  KUrl url = socket->getCurrentUrl();
  url.setPath(path);
  
  invalidateEntry(url);
}

void Cache::invalidatePath(KUrl &url)
{
  url.adjustPath(KUrl::RemoveTrailingSlash);
  m_pathCache.remove(url);
}

void Cache::invalidatePath(Socket *socket, const QString &path)
{
  KUrl url = socket->getCurrentUrl();
  url.setPath(path);
  
  invalidatePath(url);
}

DirectoryListing Cache::findCached(KUrl &url)
{
  url.adjustPath(KUrl::RemoveTrailingSlash);
  
  if (m_listingCache.contains(url))
    return m_listingCache[url];
    
  DirectoryListing invalid;
  invalid.setValid(false);
  
  return invalid;
}

DirectoryListing Cache::findCached(Socket *socket, const QString &path)
{
  KUrl url = socket->getCurrentUrl();
  url.setPath(path);
  
  return findCached(url);
}

QString Cache::findCachedPath(KUrl &url)
{
  url.adjustPath(KUrl::RemoveTrailingSlash);
  
  if (m_pathCache.contains(url))
    return m_pathCache[url];
  
  return QString::null;
}

QString Cache::findCachedPath(Socket *socket, const QString &path)
{
  KUrl url = socket->getCurrentUrl();
  url.setPath(path);
  
  return findCachedPath(url);
}

}
