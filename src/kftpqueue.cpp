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

#include <math.h>

#include "kftpqueue.h"
#include "kftpbookmarks.h"
#include "widgets/systemtray.h"
#include "kftpqueueprocessor.h"
#include "kftpsession.h"

#include "misc/config.h"
#include "misc/filter.h"

#include <KMessageBox>
#include <KLocale>
#include <kio/renamedlg.h>
#include <kfileitem.h>
#include <kopenwithdialog.h>
#include <KGlobal>
#include <kservice.h>
#include <kstandarddirs.h>
#include <krun.h>
#include <kcodecs.h>
#include <KMimeTypeTrader>
#include <k3process.h>

#include <QObject>
#include <QFile>

using namespace KFTPEngine;
using namespace KFTPCore::Filter;

namespace KFTPQueue {

OpenedFile::OpenedFile(TransferFile *transfer)
  : m_source(transfer->getSourceUrl()),
    m_dest(transfer->getDestUrl()),
    m_hash(QString::null)
{
  // Calculate the file's MD5 hash
  QFile file(m_dest.path());
  if (!file.open(QIODevice::ReadOnly)) {
    return;
  }
  
  KMD5 context;
  if (context.update(file))
    m_hash = QString(context.hexDigest());
  file.close();
}

bool OpenedFile::hasChanged()
{
  // Compare the file's MD5 hash with stored value
  QFile file(m_dest.path());
  if (!file.open(QIODevice::ReadOnly)) {
    return false;
  }
  
  QString tmp = QString::null;
  KMD5 context;
  if (context.update(file))
    tmp = QString(context.hexDigest());
  file.close();
  
  return tmp != m_hash;
}

UserDialogRequest::UserDialogRequest(TransferFile *transfer, filesize_t srcSize, time_t srcTime,
                  filesize_t dstSize, time_t dstTime)
  : m_transfer(transfer),
    m_srcSize(srcSize),
    m_srcTime(srcTime),
    m_dstSize(dstSize),
    m_dstTime(dstTime)
{
}

void UserDialogRequest::sendResponse(FileExistsWakeupEvent *event)
{
  m_transfer->wakeup(event);
  delete this;
}

class ManagerPrivate {
public:
    Manager instance;
};

K_GLOBAL_STATIC(ManagerPrivate, managerPrivate)

Manager *Manager::self()
{
  return &managerPrivate->instance;
}

Manager::Manager()
  : m_topLevel(new QueueObject(this, QueueObject::Toplevel)),
    m_processingQueue(false),
    m_feDialogOpen(false),
    m_defaultFeAction(FE_DISABLE_ACT)
{
  m_topLevel->setId(0);
  
  m_lastQID = 1;
  m_curDownSpeed = 0;
  m_curUpSpeed = 0;

  m_emitUpdate = true;

  // Create the queue processor object
  m_queueProc = new KFTPQueueProcessor(this);

  connect(m_queueProc, SIGNAL(queueComplete()), this, SLOT(slotQueueProcessingComplete()));
  connect(m_queueProc, SIGNAL(queueAborted()), this, SLOT(slotQueueProcessingAborted()));

  // Create the queue converter object
  m_converter = new KFTPQueueConverter(this);
}

Manager::~Manager()
{
}

void Manager::stopAllTransfers()
{
  if (isProcessing()) {
    abort();
  } else {
    foreach (QueueObject *i, topLevelObject()->getChildrenList()) {
      if (i->isRunning()) {
        i->abort();
      } else {
        foreach (QueueObject *t, i->getChildrenList()) {
          if (t->isRunning())
            t->abort();
        }
      }
    }
  }
}

Transfer *Manager::findTransfer(long id)
{
  // First try the cache
  QueueObject *object = m_queueObjectCache[id];
  
  if (!object) {
    object = m_topLevel->findChildObject(id);
    m_queueObjectCache.insert(id, object);
  }
  
  return static_cast<Transfer*>(object);
}

Site *Manager::findSite(KUrl url, bool noCreate)
{
  // Reset path
  url.setPath("/");
  
  if (url.isLocalFile())
    return NULL;
  
  // Find the appropriate site and if one doesn't exist create a new one
  foreach (QueueObject *i, topLevelObject()->getChildrenList()) {
    if (i->getType() == QueueObject::Site) {
      Site *site = static_cast<Site*>(i);
      
      if (site->getUrl() == url)
        return site;
    }
  }
  
  // The site doesn't exist, let's create one
  if (!noCreate) {
    Site *site = new Site(topLevelObject(), url);
    site->setId(m_lastQID++);
    emit objectAdded(site);
    site->readyObject();
    
    return site;
  }
  
  return 0;
}

void Manager::insertTransfer(Transfer *transfer)
{
  // Set id
  transfer->setId(m_lastQID++);
  
  // Reparent transfer
  filesize_t size = transfer->getSize();
  transfer->addSize(-size);
  
  if (transfer->hasParentObject())
    transfer->parentObject()->delChildObject(transfer);

  Site *site = 0;
  
  switch (transfer->getTransferType()) {
    case Download: site = findSite(transfer->getSourceUrl()); break;
    case Upload: site = findSite(transfer->getDestUrl()); break;
    case FXP: site = findSite(transfer->getSourceUrl()); break;
  }
  
  transfer->setParent(site);
  site->addChildObject(transfer);
  transfer->addSize(size);
  
  emit objectAdded(transfer);
  transfer->readyObject();
  
  if (m_emitUpdate)
    emit queueUpdate();
}

Transfer *Manager::spawnTransfer(KUrl sourceUrl, KUrl destinationUrl, filesize_t size, bool dir, bool ignoreSkip,
                                 bool insertToQueue, QObject *parent, bool noScan)
{
  const ActionChain *actionChain = Filters::self()->process(sourceUrl, size, dir);
  
  if (!ignoreSkip && (actionChain && actionChain->getAction(Action::Skip)))
    return 0;
  
  // Determine transfer type
  TransferType type;
  
  if (sourceUrl.isLocalFile())
    type = Upload;
  else if (destinationUrl.isLocalFile())
    type = Download;
  else
    type = FXP;

  // Should we lowercase the destination path ?
  if (actionChain && actionChain->getAction(Action::Lowercase))
    destinationUrl.setPath(destinationUrl.directory() + "/" + destinationUrl.fileName().toLower());
  
  // Reset a possible preconfigured default action
  setDefaultFileExistsAction();

  if (!parent)
    parent = this;

  Transfer *transfer = 0L;

  if (dir) {
    transfer = new TransferDir(parent);
  } else {
    transfer = new TransferFile(parent);
    transfer->addSize(size);
  }

  transfer->setSourceUrl(sourceUrl);
  transfer->setDestUrl(destinationUrl);
  transfer->setTransferType(type);

  if (insertToQueue) {
    insertTransfer(transfer);
  } else {
    transfer->setId(m_lastQID++);
    emit objectAdded(transfer);
    transfer->readyObject();
  }

  if (dir && !noScan) {
    // This is a directory, we should scan the directory and add all files/dirs found
    // as parent of current object
    static_cast<TransferDir*>(transfer)->scan();
  }

  return transfer;
}

void Manager::removeTransfer(Transfer *transfer, bool abortSession)
{
  if (!transfer)
    return;
    
  transfer->abort();
  long id = transfer->getId();
  long sid = transfer->parentObject()->getId();
  
  // Remove transfer from cache
  m_queueObjectCache.remove(id);

  // Should the site be removed as well ?
  QueueObject *site = 0;
  if (transfer->parentObject()->getType() == QueueObject::Site && transfer->parentObject()->getChildrenList().count() == 1)
    site = transfer->parentObject();

  // Signal destruction & delete transfer
  emit objectBeforeRemoval(transfer);
  transfer->faceDestruction(abortSession);
  delete transfer;
  emit objectAfterRemoval();
  
  if (site) {
    emit objectRemoved(site);
    delete site;
  }
  
  if (m_emitUpdate)
    emit queueUpdate();
}

void Manager::revalidateTransfer(Transfer *transfer)
{
  QueueObject *i = transfer;
  
  while (i) {
    if (i->parentObject() == topLevelObject())
      break;
      
    i = i->parentObject();
  }
  
  // We have the site
  Site *curSite = static_cast<Site*>(i);
  Site *site = 0;
  
  switch (transfer->getTransferType()) {
    case Download: site = findSite(transfer->getSourceUrl()); break;
    case Upload: site = findSite(transfer->getDestUrl()); break;
    case FXP: site = findSite(transfer->getSourceUrl()); break;
  }
  
  // If the sites don't match, reparent transfer
  if (site != curSite) {
    transfer->parentObject()->delChildObject(transfer);
    transfer->setParent(site);
    site->addChildObject(transfer);
    
    emit objectRemoved(transfer);
    emit objectAdded(transfer);
    
    if (curSite->getChildrenList().count() == 0) {
      emit objectRemoved(curSite);
      curSite->deleteLater();
    }
  }
}

void Manager::removeFailedTransfer(FailedTransfer *transfer)
{
  // Remove the transfer and signal removal
  emit failedTransferBeforeRemoval(transfer);
  m_failedTransfers.removeAll(transfer);
  delete transfer;
  emit failedTransferAfterRemoval();
}

void Manager::clearFailedTransferList()
{
  // Clear the failed transfers list
  foreach (FailedTransfer *transfer, m_failedTransfers) {
    removeFailedTransfer(transfer);
  }
}

void Manager::doEmitUpdate()
{
  m_curDownSpeed = 0;
  m_curUpSpeed = 0;

  topLevelObject()->removeMarkedTransfers();
  
  // Get download/upload speeds
  foreach (QueueObject *i, topLevelObject()->getChildrenList()) {
    foreach (QueueObject *t, i->getChildrenList()) {
      KFTPQueue::Transfer *tmp = static_cast<Transfer*>(t);
      
      switch (tmp->getTransferType()) {
        case Download: m_curDownSpeed += tmp->getSpeed(); break;
        case Upload: m_curUpSpeed += tmp->getSpeed(); break;
        case FXP: {
          m_curDownSpeed += tmp->getSpeed();
          m_curUpSpeed += tmp->getSpeed();
          break;
        }
      }
    }
  }

  // Emit global update to all GUI objects
  emit queueUpdate();
}

void Manager::start()
{
  if (m_processingQueue)
    return;
    
  m_processingQueue = true;
    
  // Now, go trough all queued files and execute them - try to do as little server connects
  // as possible
  m_queueProc->startProcessing();
}

void Manager::abort()
{
  m_processingQueue = false;
  
  // Stop further queue processing
  m_queueProc->stopProcessing();

  emit queueUpdate();
}

void Manager::slotQueueProcessingComplete()
{
  m_processingQueue = false;
  
  // Queue processing is now complete
  if (KFTPCore::Config::showBalloons())
    KFTPWidgets::SystemTray::self()->showMessage(i18n("Information"), i18n("All queued transfers have been completed."));

  emit queueUpdate();
}

void Manager::slotQueueProcessingAborted()
{
  m_processingQueue = false;
}

void Manager::clearQueue()
{
  foreach (QueueObject *i, topLevelObject()->getChildrenList()) {
    foreach (QueueObject *t, i->getChildrenList())
      removeTransfer(static_cast<Transfer*>(t));
  }
}

int Manager::getTransferPercentage()
{
  return 0;
}

int Manager::getNumRunning(bool onlyDirs)
{
  int running = 0;
  
  foreach (QueueObject *i, topLevelObject()->getChildrenList()) {
    foreach (QueueObject *t, i->getChildrenList()) {
      if (t->isRunning() && (!onlyDirs || t->isDir()))
        running++;
    }
    
    if (i->isRunning())
      running++;
  }
  
  return running;
}

int Manager::getNumRunning(const KUrl &remoteUrl)
{
  int running = 0;
  Site *site = findSite(remoteUrl, true);
  
  if (site) {
    foreach (QueueObject *i, site->getChildrenList()) {
      if (i->isRunning())
        running++;
    }
  }
  
  return running;
}

KFTPEngine::FileExistsWakeupEvent *Manager::fileExistsAction(TransferFile *transfer,
                                                             QList<KFTPEngine::DirectoryEntry> stat)
{
  FileExistsWakeupEvent *event = new FileExistsWakeupEvent();
  FileExistsActions *fa = NULL;
  FEAction action;
  
  filesize_t srcSize = 0;
  time_t srcTime = 0;
  
  filesize_t dstSize = 0;
  time_t dstTime = 0;
  
  // Check if there is a default action set
  action = getDefaultFileExistsAction();
  
  if (action == FE_DISABLE_ACT) {
    switch (transfer->getTransferType()) {
      case KFTPQueue::Download: {
        KFileItem info(KFileItem::Unknown, KFileItem::Unknown, transfer->getDestUrl());
        dstSize = info.size();
        dstTime = info.time(KIO::UDSEntry::UDS_MODIFICATION_TIME);
        
        srcSize = stat[0].size();
        srcTime = stat[0].time();
        
        fa = KFTPCore::Config::self()->dActions();
        break;
      }
      case KFTPQueue::Upload: {
        KFileItem info(KFileItem::Unknown, KFileItem::Unknown, transfer->getSourceUrl());
        srcSize = info.size();
        srcTime = info.time(KIO::UDSEntry::UDS_MODIFICATION_TIME);
        
        dstSize = stat[0].size();
        dstTime = stat[0].time();
        
        fa = KFTPCore::Config::self()->uActions();
        break;
      }
      case KFTPQueue::FXP: {
        srcSize = stat[0].size();
        srcTime = stat[0].time();
        
        dstSize = stat[1].size();
        dstTime = stat[1].time();
        
        fa = KFTPCore::Config::self()->fActions();
        break;
      }
    }
    
    // Now that we have all data, get the action and do it
    action = fa->getActionForSituation(srcSize, srcTime, dstSize, dstTime);
  }
  
  switch (action) {
    default:
    case FE_SKIP_ACT: event->action = FileExistsWakeupEvent::Skip; break;
    case FE_OVERWRITE_ACT: event->action =  FileExistsWakeupEvent::Overwrite; break;
    case FE_RESUME_ACT: event->action =  FileExistsWakeupEvent::Resume; break;
    case FE_RENAME_ACT:
    case FE_USER_ACT: {
      appendUserDialogRequest(new UserDialogRequest(transfer, srcSize, srcTime, dstSize, dstTime));
      
      // Event shall be deferred
      delete event;
      event = 0;
    }
  }
  
  return event;
}

void Manager::appendUserDialogRequest(UserDialogRequest *request)
{
  m_userDialogRequests.append(request);
  
  if (m_userDialogRequests.count() == 1) {
    processUserDialogRequest();
  }
}

void Manager::processUserDialogRequest()
{
  UserDialogRequest *request = m_userDialogRequests.first();
  if (!request)
    return;
  
  FEAction action = getDefaultFileExistsAction();
  FileExistsWakeupEvent *event = new FileExistsWakeupEvent();
  
  if (action == FE_DISABLE_ACT || action == FE_USER_ACT) {
    // A dialog really needs to be displayed
    TransferFile *transfer = request->getTransfer();
    
    KIO::RenameDialog dlg(
      (QWidget*)0, // ### parent
      i18n("File Exists"),
      transfer->getSourceUrl(),
      transfer->getDestUrl(),
      (KIO::RenameDialog_Mode) (KIO::M_OVERWRITE | KIO::M_RESUME | KIO::M_SKIP | KIO::M_MULTI),
      request->sourceSize(),
      request->destinationSize(),
      request->sourceTime(),
      request->destinationTime()
    );
    int result = dlg.exec();
    KUrl newDestUrl = dlg.newDestUrl();

    switch (result) {
      case KIO::R_RENAME: {
        transfer->setDestUrl(newDestUrl);
        
        event->action = FileExistsWakeupEvent::Rename;
        event->newFileName = newDestUrl.pathOrUrl();
        break;
      }
      case KIO::R_CANCEL: {
        // Abort queue processing
        abort();
        transfer->abort();
        
        // An event is not required, since we will not be recalling the process
        delete event;
        event = 0;
        break;
      }
      case KIO::R_AUTO_SKIP: setDefaultFileExistsAction(FE_SKIP_ACT);
      case KIO::R_SKIP: event->action = FileExistsWakeupEvent::Skip; break;
      case KIO::R_RESUME_ALL: setDefaultFileExistsAction(FE_RESUME_ACT);
      case KIO::R_RESUME: event->action = FileExistsWakeupEvent::Resume; break;
      case KIO::R_OVERWRITE_ALL: setDefaultFileExistsAction(FE_OVERWRITE_ACT);
      default: event->action = FileExistsWakeupEvent::Overwrite; break;
    }
  } else {
    switch (action) {
      default:
      case FE_SKIP_ACT: event->action = FileExistsWakeupEvent::Skip; break;
      case FE_OVERWRITE_ACT: event->action =  FileExistsWakeupEvent::Overwrite; break;
      case FE_RESUME_ACT: event->action =  FileExistsWakeupEvent::Resume; break;
    }
  }
  
  // Send a response to this request
  request->sendResponse(event);
  
  m_userDialogRequests.removeFirst();
  
  if (!m_userDialogRequests.isEmpty())
    processUserDialogRequest();
}

void Manager::openAfterTransfer(TransferFile *transfer)
{
  QString mimeType = KMimeType::findByUrl(transfer->getDestUrl(), 0, true, true)->name();
  KService::Ptr offer = KMimeTypeTrader::self()->preferredService(mimeType, "Application");
  
  if (!offer) {
    KOpenWithDialog dialog(KUrl::List(transfer->getDestUrl()));
    
    if (dialog.exec() == QDialog::Accepted) {
      offer = dialog.service();
      
      if (!offer)
        offer = new KService("", dialog.text(), "");
    } else {
      return;
    }
  }
  
  QStringList params = KRun::processDesktopExec(*offer, KUrl::List(transfer->getDestUrl()), false);
  K3Process *p = new K3Process(this);
  *p << params;
  
  connect(p, SIGNAL(processExited(K3Process*)), this, SLOT(slotEditProcessTerminated(K3Process*)));
  
  p->start();
  
  // Save the process
  m_editProcessList.insert(p->pid(), OpenedFile(transfer));
}

void Manager::slotEditProcessTerminated(K3Process *p)
{
  // A process has terminated, we should reupload
  OpenedFile file = m_editProcessList[p->pid()];
  
  // Only upload a file if it has been changed
  if (file.hasChanged()) {
    TransferFile *transfer = new TransferFile(KFTPQueue::Manager::self());
    transfer->setSourceUrl(file.destination());
    transfer->setDestUrl(file.source());
    transfer->setTransferType(KFTPQueue::Upload);
    transfer->addSize(KFileItem(KFileItem::Unknown, KFileItem::Unknown, file.destination()).size());
    insertTransfer(transfer);
    
    // Execute the transfer
    transfer->delayedExecute();
  }
  
  // Cleanup
  m_editProcessList.remove(p->pid());
  p->deleteLater();
}

}

#include "kftpqueue.moc"
