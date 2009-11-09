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

#include "kftpqueueconverter.h"
#include "kftpqueue.h"

#include <QList>
#include <QTextStream>

#include <kfilterdev.h>

KFTPQueueConverter::KFTPQueueConverter(QObject *parent)
 : QObject(parent)
{
}

void KFTPQueueConverter::importQueue(const QString &filename)
{
  m_xml = QDomDocument("KFTPGrabberQueue");
  KFTPQueue::Manager::self()->clearQueue();
  
  // Load from file
  QIODevice *file = KFilterDev::deviceForFile(filename);
  m_xml.setContent(file);
  file->close();
  delete file;
  
  // Parse XML and create KFTPQueueTransfers
  QDomNode n = m_xml.documentElement().firstChild();
  while (!n.isNull()) {
    importNode(n);
    
    n = n.nextSibling();
  }
  
  KFTPQueue::Manager::self()->doEmitUpdate();
}

void KFTPQueueConverter::exportQueue(const QString &filename)
{
  m_xml = QDomDocument("KFTPGrabberQueue");
  m_xml.setContent(QString("<queue></queue>"));
  
  // Go trough all KFTPQueueTransfers and generate XML
  QList<KFTPQueue::QueueObject*> sites = KFTPQueue::Manager::self()->topLevelObject()->getChildrenList();
  
  foreach (KFTPQueue::QueueObject *i, sites) {
    foreach (KFTPQueue::QueueObject *t, i->getChildrenList())
      generateXML(static_cast<KFTPQueue::Transfer*>(t), m_xml.documentElement());
  }
  
  // Save to file
  QIODevice *file = KFilterDev::deviceForFile(filename, "application/x-gzip");
  if (!file->open(QIODevice::WriteOnly)) {
    qDebug("WARNING: Unable to open xml for writing!");
    return;
  }
  
  QTextStream fileStream(file);
  m_xml.save(fileStream, 2);
  file->close();
  
  delete file;
}

void KFTPQueueConverter::generateXML(KFTPQueue::Transfer *transfer, QDomNode parent)
{
  // Create the item
  QDomElement item = m_xml.createElement("item");
  parent.appendChild(item);

  // Create text nodes
  createTextNode("source", transfer->getSourceUrl().url(), item);
  createTextNode("dest", transfer->getDestUrl().url(), item);
  createTextNode("size", QString::number(transfer->getSize()), item);
  createTextNode("type", transfer->isDir() ? "directory" : "file", item);
  
  if (transfer->isDir() && transfer->hasChildren()) {
    // Transfer has children, add them as well
    QDomElement tag = m_xml.createElement("children");
    item.appendChild(tag);
    
    foreach (KFTPQueue::QueueObject *i, transfer->getChildrenList()) {
      generateXML(static_cast<KFTPQueue::Transfer*>(i), tag);
    }
  }
}

void KFTPQueueConverter::importNode(QDomNode node, QObject *parent)
{
  // Get node data
  KUrl srcUrl = KUrl(getTextNode("source", node));
  KUrl dstUrl = KUrl(getTextNode("dest", node));
  filesize_t size = getTextNode("size", node).toULongLong();
  bool dir = getTextNode("type", node) == "directory";
  
  KFTPQueue::TransferType transType = KFTPQueue::Download;
  
  if (srcUrl.isLocalFile() && !dstUrl.isLocalFile()) {
    transType = KFTPQueue::Upload;
  } else if (!srcUrl.isLocalFile() && dstUrl.isLocalFile()) {
    transType = KFTPQueue::Download;
  } else if (!srcUrl.isLocalFile() && !dstUrl.isLocalFile()) {
    transType = KFTPQueue::FXP;
  }
  
  // Create new transfer
  if (!parent)
    parent = KFTPQueue::Manager::self()->topLevelObject();
    
  KFTPQueue::Transfer *transfer = 0L;
  if (dir)
    transfer = new KFTPQueue::TransferDir(parent);
  else
    transfer = new KFTPQueue::TransferFile(parent);
  
  transfer->setSourceUrl(srcUrl);
  transfer->setDestUrl(dstUrl);
  transfer->addSize(dir ? 0 : size);
  transfer->setTransferType(transType);
  
  if (parent == KFTPQueue::Manager::self()->topLevelObject()) {
    KFTPQueue::Manager::self()->insertTransfer(transfer);
  } else {
    transfer->setId(KFTPQueue::Manager::self()->m_lastQID++);
    emit KFTPQueue::Manager::self()->objectAdded(transfer);
    transfer->readyObject();
  }
  
  QDomNodeList tagNodes = node.toElement().elementsByTagName("children");
  if (dir && tagNodes.length() > 0) {
    // Import all child nodes
    QDomNode n = node.firstChild();
    while (!n.isNull()) {
      if (n.toElement().tagName() == "children") {
        n = n.firstChild();
        break;
      }
      
      n = n.nextSibling();
    }
    
    while (!n.isNull()) {
      importNode(n, transfer);
      
      n = n.nextSibling();
    }
  }
}

void KFTPQueueConverter::createTextNode(const QString &name, const QString &value, QDomNode parent)
{
  QDomElement tag = m_xml.createElement(name);
  parent.appendChild(tag);
  
  QDomText textNode = m_xml.createTextNode(value);
  tag.appendChild(textNode);
}

QString KFTPQueueConverter::getTextNode(const QString &name, QDomNode parent)
{
  QDomNodeList tagNodes = parent.toElement().elementsByTagName(name);
  
  if (tagNodes.length() > 0) {
    return tagNodes.item(0).toElement().text().trimmed();
  } else {
    return QString::null;
  }
}

#include "kftpqueueconverter.moc"
