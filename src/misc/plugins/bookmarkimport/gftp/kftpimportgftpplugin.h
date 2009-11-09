/*
 * This file is part of the KFTPGrabber project
 *
 * Copyright (C) 2004 by the KFTPGrabber developers
 * Copyright (C) 2004 Jernej Kos <kostko@jweb-network.net>
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

#ifndef KFTPIMPORTGFTPPLUGIN_H
#define KFTPIMPORTGFTPPLUGIN_H

#include <qdom.h>
#include <kftpbookmarkimportplugin.h>

/**
This plugin can import GFTP bookmarks into KFTPGrabber. This plugin has been ported
from "KBear by Bj�n Sahlstr� <kbjorn@users.sourceforge.net>".

@author Jernej Kos
*/
class KFTPImportGftpPlugin : public KFTPBookmarkImportPlugin
{
public:
    KFTPImportGftpPlugin(QObject *parent, const QStringList&);
    
    /**
     * This method should return the properly formated XML for KFTPGrabber
     * bookmarks that is generated from the import.
     *
     * @return The @ref QDomDocument representation of XML
     */
    QDomDocument getImportedXml();
    
    /**
     * This method should start the import procedure.
     *
     * @param fileName is the path to the file that will be imported
     */
    void import(const QString &fileName);
    
    /**
      * This method should return the default path where the bookmarks could
      * be located. The path must be relative to the user's home directory.
      *
      * @return The default path where bookmarks are located
      */
    QString getDefaultPath();
private:
    QDomDocument m_domDocument;
    
    QDomNode findSubGroup(QDomElement parent, const QString& name);
    QString decodePassword(const QString &password);
};

#endif
