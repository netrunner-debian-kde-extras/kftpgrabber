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

#ifndef KFTPQUEUECONVERTER_H
#define KFTPQUEUECONVERTER_H

#include <qobject.h>
#include <qdom.h>

namespace KFTPQueue {
  class Transfer;
}

/**
This class provides queue export/import to XML files.

@author Jernej Kos
*/
class KFTPQueueConverter : public QObject
{
Q_OBJECT
public:
    KFTPQueueConverter(QObject *parent = 0);
    
    /**
     * Import queue from XML file. When called, this function will create
     * new KFTPQueueTransfers.
     *
     * @param filename XML file that contains the queue
     */
    void importQueue(const QString &filename);
    
    /**
     * Export queue to XML file. It will take all current KFTPQueueTransfers
     * and convert their properties to XML format.
     *
     * @param filename File where queue will be exported
     */
    void exportQueue(const QString &filename);
private:
    QDomDocument m_xml;
    
    void generateXML(KFTPQueue::Transfer *transfer, QDomNode parent);
    void createTextNode(const QString &name, const QString &value, QDomNode parent);
    
    void importNode(QDomNode node, QObject *parent = 0);
    QString getTextNode(const QString &name, QDomNode parent);
};

#endif
