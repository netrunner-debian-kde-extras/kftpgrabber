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
#include "directoryscanner.h"
#include "kftpqueue.h"

#include "misc/config.h"
#include "misc/filter.h"

#include <QDir>

using namespace KFTPQueue;
using namespace KFTPCore::Filter;
using namespace KFTPEngine;

DirectoryScanner::DirectoryScanner(Transfer *transfer)
  : m_transfer(transfer),
    m_abort(false)
{
  // Lock the transfer
  transfer->lock();
  
  // Construct a new thread and run it
  m_thread = new ScannerThread(this, transfer);
  connect(m_thread, SIGNAL(finished()), this, SLOT(threadFinished()));
  m_thread->start();
}

void DirectoryScanner::abort()
{
  m_abort = true;
  
  if (m_thread->isRunning())
    m_thread->abort();
}

void DirectoryScanner::addScannedDirectory(KFTPEngine::DirectoryTree *tree, KFTPQueue::Transfer *parent)
{
  if (m_abort)
    return;
  
  // Directories
  DirectoryTree::DirIterator dirEnd = tree->directories()->constEnd();
  for (DirectoryTree::DirIterator i = tree->directories()->constBegin(); i != dirEnd; i++) {
    if (m_abort)
      return;
    
    KUrl sourceUrlBase = parent->getSourceUrl();
    KUrl destUrlBase = parent->getDestUrl();
    
    sourceUrlBase.addPath((*i)->info().filename());
    destUrlBase.addPath((*i)->info().filename());
    
    if (KFTPCore::Config::skipEmptyDirs() && !(*i)->directories()->count())
      continue;
    
    // Add directory transfer
    KFTPQueue::TransferDir *transfer = new KFTPQueue::TransferDir(parent);
    transfer->setSourceUrl(sourceUrlBase);
    transfer->setDestUrl(destUrlBase);
    transfer->setTransferType(parent->getTransferType());
    transfer->setId(KFTPQueue::Manager::self()->nextTransferId());
    
    emit KFTPQueue::Manager::self()->objectAdded(transfer);
    transfer->readyObject();
    
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    addScannedDirectory(*i, transfer);
  }
  
  // Files
  DirectoryTree::FileIterator fileEnd = tree->files()->constEnd();
  for (DirectoryTree::FileIterator i = tree->files()->constBegin(); i != fileEnd; i++) {
    if (m_abort)
      return;
    
    KUrl sourceUrlBase = parent->getSourceUrl();
    KUrl destUrlBase = parent->getDestUrl();
    
    sourceUrlBase.addPath((*i).filename());
    destUrlBase.addPath((*i).filename());
    
    // Add file transfer
    KFTPQueue::TransferFile *transfer = new KFTPQueue::TransferFile(parent);
    transfer->addSize((*i).size());
    transfer->setSourceUrl(sourceUrlBase);
    transfer->setDestUrl(destUrlBase);
    transfer->setTransferType(parent->getTransferType());
    transfer->setId(KFTPQueue::Manager::self()->nextTransferId());
    
    emit KFTPQueue::Manager::self()->objectAdded(transfer);
    transfer->readyObject();
    
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
  }
}

void DirectoryScanner::threadFinished()
{
  // Create required transfers
  DirectoryTree *tree = m_thread->tree();
  addScannedDirectory(tree, m_transfer);
  delete tree;
  
  m_transfer->unlock();
  emit completed();
  
  // Destroy thiy object
  deleteLater();
}

DirectoryScanner::ScannerThread::ScannerThread(QObject *parent, Transfer *item)
  : QThread(),
    m_parent(parent),
    m_item(item),
    m_abort(false)
{
}

void DirectoryScanner::ScannerThread::run()
{
  m_tree = new DirectoryTree();
  scanFolder(m_item->getSourceUrl().path(), m_tree);
}

void DirectoryScanner::ScannerThread::abort()
{
  m_abort = true;
}

void DirectoryScanner::ScannerThread::scanFolder(const QString &path, DirectoryTree *tree)
{
  if (m_abort)
    return;
  
  QList<DirectoryEntry> list;
  QDir fs(path);
  fs.setFilter(QDir::Readable | QDir::Hidden | QDir::TypeMask);

  foreach (QFileInfo file, fs.entryInfoList()) {
    if (m_abort)
      return;
    
    if (file.fileName() == "." || file.fileName() == "..")
      continue;
    
    KUrl sourceUrl;
    sourceUrl.setPath(file.absoluteFilePath());
    
    // Check if we should skip this entry
    const ActionChain *actionChain = Filters::self()->process(sourceUrl, file.size(), file.isDir());
     
    if (actionChain && actionChain->getAction(Action::Skip))
      continue;
    
    DirectoryEntry entry;
    entry.setFilename(file.fileName());
    entry.setType(file.isDir() ? 'd' : 'f');
    entry.setSize(file.size());
    
    list.append(entry);
  }
  
  // Sort by priority
  qSort(list);
  
  foreach (DirectoryEntry entry, list) {
    if (m_abort)
      return;
    
    if (entry.isDirectory()) {
      DirectoryTree *child = tree->addDirectory(entry);
      scanFolder(path + "/" + entry.filename(), child);
    } else {
      tree->addFile(entry);
    }
  }
}

#include "directoryscanner.moc"


