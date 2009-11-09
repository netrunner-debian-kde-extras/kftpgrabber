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

#include "kftpimportgftpplugin.h"

#include <qdir.h>

#include <kgenericfactory.h>
#include <klocale.h>
#include <kconfig.h>
#include <kcodecs.h>
#include <kconfiggroup.h>

K_EXPORT_COMPONENT_FACTORY(kftpimportplugin_gftp,
                           KGenericFactory<KFTPImportGftpPlugin>("kftpimportplugin_gftp"))

KFTPImportGftpPlugin::KFTPImportGftpPlugin(QObject *parent, const QStringList&)
 : KFTPBookmarkImportPlugin(parent)
{
  KGlobal::locale()->insertCatalog("kftpgrabber");
  m_domDocument.setContent(QString("<category name=\"%1\"/>").arg(i18n("gFTP import")));
}

QDomDocument KFTPImportGftpPlugin::getImportedXml()
{
  return m_domDocument;
}

void KFTPImportGftpPlugin::import(const QString &fileName)
{
  // First we fetch some global settings
  KConfig tmpConfig(userPath(".gftp/gftprc"), KConfig::NoGlobals);
  KConfigGroup group = tmpConfig.group(0);
  
  QString email = group.readEntry("email", "anonymous@");
  int numRetries = group.readEntry<int>("retries", -1);
  int sleepTime = group.readEntry<int>("sleep_time", -1);
  
  // Open the bookmarks file (it has INI-like file format, so we can use the KConfig
  // class to do the parsing and converting)
  KConfig config(fileName, KConfig::SimpleConfig);
  QStringList groupList = config.groupList();
  
  float size = (float) groupList.count();
  if (size == 0) {
    // There are no bookmarks (or incorrect file), we are done
    
    emit progress(100);
    return;
  }
  
  int counter = 0;
  QStringList::Iterator end(groupList.end());
  for( QStringList::Iterator it(groupList.begin()); it != end; ++it) {
    // gFTP bookmarks can have subgroups
    QString groupName = *it;
    QStringList groupNames = groupName.split("/");

    QDomNode groupNode;
    QDomElement parentElement = m_domDocument.documentElement();
    
    group = config.group(groupName);
    QString tmp = group.readEntry("hostname");
    
    for (int i = 0; ! tmp.isNull() && i < groupNames.count() - 1; ++i ) {
      // First see if parenElement has any sub group
      groupNode = findSubGroup(parentElement, groupNames[i]);

      if (groupNode.isNull()) {
        // No, it has no subgroup, let's create one
        while (i < groupNames.count() -1) {
          QDomElement tmpElement = m_domDocument.createElement("category");
          tmpElement.setAttribute("name", groupNames[i]);
          parentElement.appendChild(tmpElement);
          parentElement = tmpElement;
          
          ++i;
        }
      } else {
        // Sub group found, lets check next level
        parentElement = groupNode.toElement();
      }
    }
    
    // Now group tree is updated so lets create the site (if it has hostname)
    if (!tmp.isNull()) {
      // Set name
      QDomElement siteElement = m_domDocument.createElement("server");
      siteElement.setAttribute("name", groupNames.last());
      parentElement.appendChild(siteElement);
      
      // Set host
      tmp = group.readEntry("hostname");
      QDomElement tmpElement = m_domDocument.createElement("host");
      QDomText txtNode = m_domDocument.createTextNode(tmp);
      tmpElement.appendChild(txtNode);
      siteElement.appendChild(tmpElement);
      
      // Set port
      int p = group.readEntry<int>("port", 21);
      tmpElement = m_domDocument.createElement("port");
      txtNode = m_domDocument.createTextNode(QString::number(p));
      tmpElement.appendChild(txtNode);
      siteElement.appendChild(tmpElement);
      
      // Set remote directory
      tmp = group.readEntry("remote directory", "/");
      tmpElement = m_domDocument.createElement("defremotepath");
      txtNode = m_domDocument.createTextNode(tmp);
      tmpElement.appendChild(txtNode);
      siteElement.appendChild(tmpElement);
      
      // Set local directory
      tmp = group.readEntry("local directory", QDir::homePath());
      tmpElement = m_domDocument.createElement("deflocalpath");
      txtNode = m_domDocument.createTextNode(tmp);
      tmpElement.appendChild(txtNode);
      siteElement.appendChild(tmpElement);
      
      // Set username
      tmp = group.readEntry("username", "anonymous");
      tmpElement = m_domDocument.createElement("username");
      txtNode = m_domDocument.createTextNode(tmp);
      tmpElement.appendChild(txtNode);
      siteElement.appendChild(tmpElement);
      
      if (tmp == "anonymous") {
        tmpElement = m_domDocument.createElement("anonlogin");
        txtNode = m_domDocument.createTextNode("1");
        tmpElement.appendChild(txtNode);
        siteElement.appendChild(tmpElement);
      }
      
      // Set password
      tmp = group.readEntry("password", "");
      tmpElement = m_domDocument.createElement("password");
      
      if (tmp == "@EMAIL@" || tmp.isNull() || tmp.isEmpty())
        tmp = email;
      else
        tmp = decodePassword(tmp);
        
      // We have to encode the password
      tmp = KCodecs::base64Encode(tmp.toAscii(), true).data();
      txtNode = m_domDocument.createTextNode(tmp);
      tmpElement.appendChild(txtNode);
      siteElement.appendChild(tmpElement);
      
      // Set retries
      if (numRetries >= 0) {
        tmpElement = m_domDocument.createElement("retrytime");
        txtNode = m_domDocument.createTextNode(QString::number(sleepTime));
        tmpElement.appendChild(txtNode);
        siteElement.appendChild(tmpElement);
        
        tmpElement = m_domDocument.createElement("retrycount");
        txtNode = m_domDocument.createTextNode(QString::number(numRetries));
        tmpElement.appendChild(txtNode);
        siteElement.appendChild(tmpElement);
      }
    }
    
    emit progress(int(float(counter) / size * 100));
    ++counter;
  }
  
  emit progress(100);
}

QString KFTPImportGftpPlugin::decodePassword(const QString &password)
{
  // Leave unencoded passwords as they are
  if (password[0] != '$')
    return password;
    
  QString work = password;
  work.remove(0, 1);
  
  QString result;
  
  for (int i = 0; i < work.length() - 1; i += 2) {
    char c = work.at(i).toLatin1();
    char n = work.at(i+1).toLatin1();
    
    result.append( ((c & 0x3c) << 2) | ((n & 0x3c) >> 2) );
  }

  return result;
}

QDomNode KFTPImportGftpPlugin::findSubGroup(QDomElement parent, const QString& name)
{
  QDomNodeList nodeList = parent.childNodes();
  
  for (int i = 0; i < nodeList.count(); ++i) {
    if(nodeList.item(i).toElement().attribute("name") == name)
      return nodeList.item(i);
  }
  
  return QDomNode();
}

QString KFTPImportGftpPlugin::getDefaultPath()
{
  return QString(".gftp/bookmarks");
}

