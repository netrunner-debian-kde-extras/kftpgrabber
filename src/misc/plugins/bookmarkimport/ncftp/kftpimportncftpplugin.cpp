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
#include "kftpimportncftpplugin.h"

#include <QDir>
#include <QFile>

#include <kgenericfactory.h>
#include <klocale.h>
#include <kconfig.h>
#include <kcodecs.h>

K_EXPORT_COMPONENT_FACTORY(kftpimportplugin_ncftp,
                           KGenericFactory<KFTPImportNcftpPlugin>("kftpimportplugin_ncftp"))

KFTPImportNcftpPlugin::KFTPImportNcftpPlugin(QObject *parent, const QStringList&)
 : KFTPBookmarkImportPlugin(parent)
{
  KGlobal::locale()->insertCatalog("kftpgrabber");
  m_domDocument.setContent(QString("<category name=\"%1\"/>").arg(i18n("NcFtp import")));
}

QDomDocument KFTPImportNcftpPlugin::getImportedXml()
{
  return m_domDocument;
}

void KFTPImportNcftpPlugin::import(const QString &fileName)
{
  /*
    ARNES FTP serve,ftp.arnes.si,username,*encoded*cGFzc3dvcmQA,,/remote,I,21,4294967295,1,1,-1,1,193.2.1.79,Komentar,,,,,S,-1,/local
    Redhat,ftp.redhat.com,,,,,I,21,1102099812,-1,-1,-1,1,66.187.224.30,,,,,,S,-1,
  */
  
  QFile f(fileName);
  if (!f.open(QIODevice::ReadOnly)) {
    emit progress(100);
    return;
  }
  
  QTextStream stream(&f);
  QString line;
  int lineNum = 0;
  
  while (!stream.atEnd()) {
    line = stream.readLine();
    if (++lineNum <= 2) continue;
    
    // Add the imported bookmark
    QDomElement parentElement = m_domDocument.documentElement();
    
    // Set name
    QDomElement siteElement = m_domDocument.createElement("server");
    siteElement.setAttribute("name", subSection(line, 0));
    parentElement.appendChild(siteElement);
    
    // Set host
    QString tmp = subSection(line, 1);
    QDomElement tmpElement = m_domDocument.createElement("host");
    QDomText txtNode = m_domDocument.createTextNode(tmp);
    tmpElement.appendChild(txtNode);
    siteElement.appendChild(tmpElement);
    
    // Set port
    tmp = subSection(line, 7, "21");
    tmpElement = m_domDocument.createElement("port");
    txtNode = m_domDocument.createTextNode(tmp);
    tmpElement.appendChild(txtNode);
    siteElement.appendChild(tmpElement);
    
    // Set remote directory
    tmp = subSection(line, 5, "/");
    tmpElement = m_domDocument.createElement("defremotepath");
    txtNode = m_domDocument.createTextNode(tmp);
    tmpElement.appendChild(txtNode);
    siteElement.appendChild(tmpElement);
    
    // Set local directory
    tmp = subSection(line, 21, QDir::homePath());
    tmpElement = m_domDocument.createElement("deflocalpath");
    txtNode = m_domDocument.createTextNode(tmp);
    tmpElement.appendChild(txtNode);
    siteElement.appendChild(tmpElement);
    
    // Set username
    tmp = subSection(line, 2, "anonymous");
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
    tmp = subSection(line, 3, "");
    tmp.replace("*encoded*", "");
    
    tmpElement = m_domDocument.createElement("password");
    txtNode = m_domDocument.createTextNode(tmp);
    tmpElement.appendChild(txtNode);
    siteElement.appendChild(tmpElement);
    
    // Set description
    tmp = subSection(line, 14, "");
    if (!tmp.isEmpty()) {
      tmpElement = m_domDocument.createElement("description");
      txtNode = m_domDocument.createTextNode(tmp);
      tmpElement.appendChild(txtNode);
      siteElement.appendChild(tmpElement);
    }
  }
  
  emit progress(100);
}

QString KFTPImportNcftpPlugin::subSection(const QString &text, int section, const QString &def)
{
  QString tmp = text.section(',', section, section);
  
  return tmp.isEmpty() ? def : tmp;
}

QString KFTPImportNcftpPlugin::getDefaultPath()
{
  return QString(".ncftp/bookmarks");
}

#include "kftpimportncftpplugin.moc"
