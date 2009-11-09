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

#include "kftpimportfz3plugin.h"

#include <QFile>

#include <kgenericfactory.h>
#include <klocale.h>
#include <kconfig.h>
#include <kcodecs.h>

K_EXPORT_COMPONENT_FACTORY(kftpimportplugin_filezilla3,
                           KGenericFactory<KFTPImportFz3Plugin>("kftpimportplugin_filezilla3"))

KFTPImportFz3Plugin::KFTPImportFz3Plugin(QObject *parent, const QStringList&)
 : KFTPBookmarkImportPlugin(parent)
{
  KGlobal::locale()->insertCatalog("kftpgrabber");
}

QDomDocument KFTPImportFz3Plugin::getImportedXml()
{
  return m_domDocument;
}

void KFTPImportFz3Plugin::import(const QString &fileName)
{
  m_domDocument.setContent(QString("<category name=\"%1\"/>").arg(i18n("FileZilla 3 import")));
  
  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly)) {
    emit progress(100);
    return;
  }
  
  m_workDocument.setContent(&file);
  file.close();
  
  // Import categories recursively
  importCategory(m_domDocument.documentElement(), m_workDocument.documentElement().firstChild());
  
  emit progress(100);
}

void KFTPImportFz3Plugin::importCategory(QDomNode parent, const QDomNode &node)
{
  QDomNode n = node.firstChild();
  
  while (!n.isNull()) {
    if (!n.isElement()) {
      n = n.nextSibling();
      continue;
    }
    
    QDomElement e = n.toElement();
    
    if (e.tagName() == "Folder") {
      QDomElement categoryElement = m_domDocument.createElement("category");
      categoryElement.setAttribute("name", e.firstChild().nodeValue().trimmed());
      parent.appendChild(categoryElement);
      
      importCategory(categoryElement, n);
    } else if (e.tagName() == "Server") {
      QString name = e.lastChild().nodeValue().trimmed();
      QString host = e.namedItem("Host").toElement().text();
      QString port = e.namedItem("Port").toElement().text();
      QString localDir = e.namedItem("LocalDir").toElement().text();
      QString remoteDir = e.namedItem("RemoteDir").toElement().text();
      QString username = e.namedItem("User").toElement().text();
      QString password = e.namedItem("Pass").toElement().text();
      
      // Set name
      QDomElement siteElement = m_domDocument.createElement("server");
      siteElement.setAttribute("name", name);
      parent.appendChild(siteElement);
      
      // Set host
      QDomElement tmpElement = m_domDocument.createElement("host");
      QDomText txtNode = m_domDocument.createTextNode(host);
      tmpElement.appendChild(txtNode);
      siteElement.appendChild(tmpElement);
      
      // Set port
      tmpElement = m_domDocument.createElement("port");
      txtNode = m_domDocument.createTextNode(port);
      tmpElement.appendChild(txtNode);
      siteElement.appendChild(tmpElement);
      
      // Set remote directory
      tmpElement = m_domDocument.createElement("defremotepath");
      txtNode = m_domDocument.createTextNode(remoteDir);
      tmpElement.appendChild(txtNode);
      siteElement.appendChild(tmpElement);
      
      // Set local directory
      tmpElement = m_domDocument.createElement("deflocalpath");
      txtNode = m_domDocument.createTextNode(localDir);
      tmpElement.appendChild(txtNode);
      siteElement.appendChild(tmpElement);
      
      // Set username
      if (username.isNull()) {
        username = "anonymous";
        
        tmpElement = m_domDocument.createElement("anonlogin");
        txtNode = m_domDocument.createTextNode("1");
        tmpElement.appendChild(txtNode);
        siteElement.appendChild(tmpElement);
      }
      
      tmpElement = m_domDocument.createElement("username");
      txtNode = m_domDocument.createTextNode(username);
      tmpElement.appendChild(txtNode);
      siteElement.appendChild(tmpElement);
      
      // Set password
      tmpElement = m_domDocument.createElement("password");
      txtNode = m_domDocument.createTextNode(KCodecs::base64Encode(password.toAscii(), true).data());
      tmpElement.appendChild(txtNode);
      siteElement.appendChild(tmpElement);
    }
    
    n = n.nextSibling();
  }
}

QString KFTPImportFz3Plugin::getDefaultPath()
{
  return QString(".filezilla/sitemanager.xml");
}

#include "kftpimportfz3plugin.moc"
