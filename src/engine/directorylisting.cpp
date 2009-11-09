/*
 * This file is part of the KFTPGrabber project
 *
 * Copyright (C) 2003-2006 by the KFTPGrabber developers
 * Copyright (C) 2003-2006 Jernej Kos <kostko@jweb-network.net>
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

#include "directorylisting.h"
#include "misc/filter.h"

#include <QDateTime>

#include <KLocale>
#include <KGlobal>
#include <KMimeType>

#include <sys/stat.h>

using namespace KFTPCore::Filter;
using namespace KIO;

namespace KFTPEngine {

DirectoryEntry::DirectoryEntry()
{
}

KIO::UDSEntry DirectoryEntry::toUdsEntry() const
{
  bool directory = m_type == 'd';
  UDSEntry entry;
  
  entry.insert(UDSEntry::UDS_NAME, m_filename);
  entry.insert(UDSEntry::UDS_SIZE, m_size);
  entry.insert(UDSEntry::UDS_MODIFICATION_TIME, m_time);
  entry.insert(UDSEntry::UDS_USER, m_owner);
  entry.insert(UDSEntry::UDS_GROUP, m_group);
  entry.insert(UDSEntry::UDS_ACCESS, m_permissions);
  
  if (!m_link.isEmpty()) {
    entry.insert(UDSEntry::UDS_LINK_DEST, m_link);
    
    KMimeType::Ptr mime = KMimeType::findByUrl(KUrl("ftp://host/" + m_filename));
    if (mime->name() == KMimeType::defaultMimeType()) {
      entry.insert(UDSEntry::UDS_GUESSED_MIME_TYPE, QString("inode/directory"));
      directory = true;
    }
  }
  
  entry.insert(UDSEntry::UDS_FILE_TYPE, directory ? S_IFDIR : S_IFREG);
  
  return entry;
}

QString DirectoryEntry::timeAsString()
{
  QDateTime dt;
  dt.setTime_t(time());

  return KGlobal::locale()->formatDateTime(dt);
}

bool DirectoryEntry::operator<(const DirectoryEntry &entry) const
{
  const Action *firstAction = Filters::self()->process(*this, Action::Priority);
  const Action *secondAction = Filters::self()->process(entry, Action::Priority);
  
  int priorityFirst = firstAction ? firstAction->value().toInt() : 0;
  int prioritySecond = secondAction ? secondAction->value().toInt() : 0;
  
  if (priorityFirst == prioritySecond) {
    if (isDirectory() != entry.isDirectory())
      return isDirectory();
    
    return m_filename < entry.m_filename;
  }
  
  return priorityFirst > prioritySecond;
}

DirectoryTree::DirectoryTree(DirectoryEntry entry)
  : m_entry(entry)
{
}

DirectoryTree::~DirectoryTree()
{
  // Free all allocated subtrees
  foreach (DirectoryTree *dir, m_directories) {
    delete dir;
  }
}

void DirectoryTree::addFile(DirectoryEntry entry)
{
  m_files.append(entry);
}

DirectoryTree *DirectoryTree::addDirectory(DirectoryEntry entry)
{
  DirectoryTree *tree = new DirectoryTree(entry);
  m_directories.append(tree);
  
  return tree;
}

DirectoryListing::DirectoryListing(const KUrl &path)
  : m_valid(true),
    m_path(path)
{
}

DirectoryListing::~DirectoryListing()
{
  m_list.clear();
}

void DirectoryListing::addEntry(DirectoryEntry entry)
{
  m_list.append(entry);
}

void DirectoryListing::updateEntry(const QString &filename, ::filesize_t size)
{
  QList<DirectoryEntry>::iterator listEnd = m_list.end();
  for (QList<DirectoryEntry>::Iterator i = m_list.begin(); i != listEnd; i++) {
    if ((*i).filename() == filename) {
      (*i).setSize(size);
      return;
    }
  }
  
  // Entry not found, add one
  DirectoryEntry entry;
  entry.setFilename(filename);
  entry.setSize(size);
  
  addEntry(entry);
}

}
