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

#include "kftpimportkftpplugin.h"

#include <qdir.h>
#include <qfile.h>

#include <kgenericfactory.h>
#include <klocale.h>
#include <kconfig.h>
#include <kcodecs.h>

K_EXPORT_COMPONENT_FACTORY(kftpimportplugin_kftp,
                           KGenericFactory<KFTPImportKftpPlugin>("kftpimportplugin_kftp"))

KFTPImportKftpPlugin::KFTPImportKftpPlugin(QObject *parent, const QStringList&)
 : KFTPBookmarkImportPlugin(parent)
{
  KGlobal::locale()->insertCatalog("kftpgrabber");
}

QDomDocument KFTPImportKftpPlugin::getImportedXml()
{
  return m_domDocument;
}

void KFTPImportKftpPlugin::import(const QString &fileName)
{
  m_domDocument.setContent(QString("<category name=\"%1\"/>").arg(i18n("KFTPGrabber import")));
  
  // There is actually nothing to import, we just have to read the existing XML and
  // remove site ids.
  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly)) {
    emit progress(100);
    return;
  }
  
  m_workDocument.setContent(&file);
  file.close();
  
  // Strip all ids
  stripIds();
  
  // Now append the bookmarks
  QDomNode n = m_workDocument.documentElement().firstChild();
  
  while (!n.isNull()) {
    QDomNode import = m_domDocument.importNode(n, true);
    m_domDocument.documentElement().appendChild(import);
    
    n = n.nextSibling();
  }
  
  emit progress(100);
}

void KFTPImportKftpPlugin::stripIds(QDomNode node)
{
  if (node.isNull())
    node = m_workDocument.documentElement();
    
  QDomNode n = node.firstChild();

  while (!n.isNull()) {
    if (n.toElement().tagName() == "category") {
      if (!n.toElement().hasAttribute("id"))
        n.toElement().removeAttribute("id");
      
      stripIds(n);
    } else if (n.toElement().tagName() == "server") {
      if (n.toElement().hasAttribute("id"))
        n.toElement().removeAttribute("id");
    }
    
    n = n.nextSibling();
  }
}

QString KFTPImportKftpPlugin::getDefaultPath()
{
  return QString("");
}

#include "kftpimportkftpplugin.moc"
