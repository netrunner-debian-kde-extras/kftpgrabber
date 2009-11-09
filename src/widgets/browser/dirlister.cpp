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
#include "dirlister.h"
#include "kftpsession.h"

#include "engine/thread.h"
#include "engine/cache.h"

#include <KLocale>
#include <kio/jobclasses.h>

using namespace KFTPEngine;
using namespace KFTPSession;

namespace KFTPWidgets {

namespace Browser {

ReportingDirLister::ReportingDirLister(QObject *parent)
  : KDirLister(parent)
{
}

ReportingDirLister::~ReportingDirLister()
{
}

void ReportingDirLister::handleError(KIO::Job *job)
{
  emit errorMessage(job->errorString());
}

DirLister::DirLister(QObject *parent)
  : QObject(parent),
    m_remoteSession(0),
    m_showHidden(false),
    m_dirOnly(false),
    m_ignoreChanges(false),
    m_mode(None)
{
  m_localLister = new ReportingDirLister(this);
  m_localLister->setAutoUpdate(true);
}

DirLister::~DirLister()
{
}

void DirLister::setSession(Session *session)
{
  m_remoteSession = session;
}

KFileItem *DirLister::rootItem() const
{
  KUrl url = m_lastUrl;
  url.setPath("/");
  
  return new KFileItem(url, "inode/directory", S_IFDIR);
}

void DirLister::openUrl(const KUrl &url, KDirLister::OpenUrlFlags flags)
{
  if (m_lastUrl.isLocalFile() != url.isLocalFile()) {
    emit siteChanged(url);
    emit clear();
  }
  
  if (!(flags & KDirLister::Keep))
    m_ignoreChanges = false;
  
  m_lastUrl = url;
  m_lastUrl.adjustPath(KUrl::RemoveTrailingSlash);
  
  if (url.isLocalFile()) {
    setRemoteEnabled(false);
    
    m_localLister->stop();
    m_localLister->setShowingDotFiles(m_showHidden);
    m_localLister->setDirOnlyMode(m_dirOnly);
    m_localLister->openUrl(url, flags);
  } else if (m_remoteSession && m_remoteSession->isConnected()) {
    setRemoteEnabled(true);
    
    if (flags & KDirLister::Reload) {
      KUrl tmp = url;
      Cache::self()->invalidateEntry(tmp);
    }
    
    if (!(flags & KDirLister::Keep))
      emit clear();
    
    m_remoteSession->getClient()->list(url);
  }
}

void DirLister::setRemoteEnabled(bool enabled, bool withoutLocal)
{
  Thread *client = m_remoteSession->getClient();
  
  // Disconnect everything and reset to default mode
  m_localLister->QObject::disconnect(this);
  client->eventHandler()->QObject::disconnect(this);
  
  if (enabled) {
    connect(client->eventHandler(), SIGNAL(engineEvent(KFTPEngine::Event*)), this, SLOT(slotRemoteEngineEvent(KFTPEngine::Event*)));
    
    m_mode = Remote;
  } else if (!withoutLocal) {
    connect(m_localLister, SIGNAL(clear()), this, SIGNAL(clear()));
    connect(m_localLister, SIGNAL(completed()), this, SIGNAL(completed()));
    connect(m_localLister, SIGNAL(deleteItem(const KFileItem&)), this, SIGNAL(deleteItem(const KFileItem&)));
    connect(m_localLister, SIGNAL(refreshItems(const QList<QPair<KFileItem, KFileItem> >&)), this, SIGNAL(refreshItems(const QList<QPair<KFileItem, KFileItem> >&)));
    connect(m_localLister, SIGNAL(newItems(KFileItemList)), this, SIGNAL(newItems(KFileItemList)));
    connect(m_localLister, SIGNAL(errorMessage(const QString&)), this, SIGNAL(errorMessage(const QString&)));
    
    m_mode = Local;
  }
}

void DirLister::stop()
{
  if (m_lastUrl.isLocalFile())
    m_localLister->stop();
}

void DirLister::slotRemoteEngineEvent(KFTPEngine::Event *event)
{
  switch (event->type()) {
    case Event::EventError: {
      emit errorMessage(i18n("Could not enter folder %1.", m_lastUrl.path()));
      setRemoteEnabled(false, true);
      break;
    }
    case Event::EventDirectoryListing: {
      m_items.clear();
      
      // Populate the item list
      QList<DirectoryEntry> list = event->getParameter(0).value<DirectoryListing>().list();
      QList<DirectoryEntry>::ConstIterator end(list.end());
      for (QList<DirectoryEntry>::ConstIterator i(list.begin()); i != end; ++i) {
        if (!m_showHidden && (*i).filename().at(0) == '.')
          continue;
        
        if (m_dirOnly && !(*i).isDirectory())
          continue;
        
        m_items.append(KFileItem((*i).toUdsEntry(), m_lastUrl, false, true));
      }
      
      setRemoteEnabled(false, true);
      emit newItems(m_items);
      emit completed();
      break;
    }
    default: break;
  }
}

}

}

#include "dirlister.moc"
