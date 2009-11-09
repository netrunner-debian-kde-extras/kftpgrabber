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

#ifndef KFTPNETWORKDIRECTORYLISTING_H
#define KFTPNETWORKDIRECTORYLISTING_H

#include <kio/global.h>
#include <kio/udsentry.h>
#include <KUrl>

#include <QList>

#include <time.h>
#include <sys/time.h>

typedef qulonglong filesize_t;

namespace KFTPEngine {

class DirectoryEntry {
public:
    DirectoryEntry();
    
    void setFilename(const QString &filename) { m_filename = filename; }
    void setOwner(const QString &owner) { m_owner = owner; }
    void setGroup(const QString &group) { m_group = group; }
    void setLink(const QString &link) { m_link = link; }
    void setPermissions(int permissions) { m_permissions = permissions; }
    void setSize(filesize_t size) { m_size = size; }
    void setType(char type) { m_type = type; }
    void setTime(time_t time) { m_time = time; }
    
    QString filename() const { return m_filename; }
    QString owner() const { return m_owner; }
    QString group() const { return m_group; }
    QString link() const { return m_link; }
    int permissions() const { return m_permissions; }
    filesize_t size() const { return m_size; }
    char type() const { return m_type; }
    time_t time() const { return m_time; }
    QString timeAsString();
    
    bool isDirectory() const { return m_type == 'd'; }
    bool isFile() const { return m_type == 'f'; }
    bool isDevice() const { return m_type == 'c' || m_type == 'b'; }
    bool isSymlink() const { return !m_link.isEmpty(); }
    
    KIO::UDSEntry toUdsEntry() const;
    
    struct tm timeStruct;
    
    bool operator<(const DirectoryEntry &entry) const;
private:
    QString m_filename;
    QString m_owner;
    QString m_group;
    QString m_link;
    
    int m_permissions;
    filesize_t m_size;
    char m_type;
    time_t m_time;
};

class DirectoryTree {
public:
    typedef QList<DirectoryEntry>::ConstIterator FileIterator;
    typedef QList<DirectoryTree*>::ConstIterator DirIterator;
    
    DirectoryTree() {}
    DirectoryTree(DirectoryEntry entry);
    ~DirectoryTree();

    void addFile(DirectoryEntry entry);
    DirectoryTree *addDirectory(DirectoryEntry entry);
    
    DirectoryEntry info() { return m_entry; }
    
    QList<DirectoryEntry> *files() { return &m_files; }
    QList<DirectoryTree*> *directories() { return &m_directories; }
private:
    DirectoryEntry m_entry;
    QList<DirectoryEntry> m_files;
    QList<DirectoryTree*> m_directories;
};

/**
 * @author Jernej Kos <kostko@jweb-network.net>
 */
class DirectoryListing {
public:
    DirectoryListing(const KUrl &path = KUrl());
    ~DirectoryListing();
    
    void addEntry(DirectoryEntry entry);
    void updateEntry(const QString &filename, filesize_t size);
    QList<DirectoryEntry> list() { return m_list; }
    
    void setValid(bool value) { m_valid = value; }
    bool isValid() { return m_valid; }
private:
    bool m_valid;
    KUrl m_path;
    QList<DirectoryEntry> m_list;
};

}

Q_DECLARE_METATYPE(KFTPEngine::DirectoryListing)
Q_DECLARE_METATYPE(KFTPEngine::DirectoryTree*)

#endif
