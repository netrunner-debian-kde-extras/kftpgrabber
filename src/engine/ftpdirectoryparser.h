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
 *
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
 
#ifndef KFTPENGINEFTPDIRECTORYPARSER_H
#define KFTPENGINEFTPDIRECTORYPARSER_H

#include <QMap>

#include "directorylisting.h"

namespace KFTPEngine {

class FtpSocket;

class DToken;
class DLine;

/**
 * This class can parse multiple directory formats. Some code portions have
 * been taken from a windows FTP client "FileZilla by Tim Kosse" - the
 * logic is mostly the same, the code has just been ported so it is more Qt
 * and so it integrates nicely with the rest of the engine.
 *
 * @author Jernej Kos <kostko@jweb-network.net>
 * @author Tim Kosse <tim.kosse@gmx.de>
 */
class FtpDirectoryParser {
public:
    FtpDirectoryParser(FtpSocket *socket);
    
    void addData(const char *data, int len);
    void addDataLine(const QString &line);
    
    bool parseLine(const QString &line, DirectoryEntry &entry);
    DirectoryListing getListing() { return m_listing; }
private:
    FtpSocket *m_socket;
    QString m_buffer;
    DirectoryListing m_listing;
    
    QMap<QString, int> m_monthNameMap;

    bool parseMlsd(const QString &line, DirectoryEntry &entry);
    bool parseUnix(DLine *line, DirectoryEntry &entry);
    bool parseDos(DLine *line, DirectoryEntry &entry);
    bool parseVms(DLine *line, DirectoryEntry &entry);
    
    bool parseUnixDateTime(DLine *line, int &index, DirectoryEntry &entry);
    bool parseShortDate(DToken &token, DirectoryEntry &entry);
    bool parseTime(DToken &token, DirectoryEntry &entry);
    
    bool parseComplexFileSize(DToken &token, filesize_t &size);
    
    bool parseUnixPermissions(const QString &permissions, DirectoryEntry &entry);
};

}

#endif
