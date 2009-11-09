/*
 * This file is part of the KFTPGrabber project
 *
 * Copyright (C) 2003-2007 by the KFTPGrabber developers
 * Copyright (C) 2003-2007 Jernej Kos <kostko@jweb-network.net>
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

#ifndef KFTPQUEUE_H
#define KFTPQUEUE_H

#include <QString>
#include <QList>
#include <QCache>
#include <QMap>

#include <KUrl>

#include "kftpqueueprocessor.h"
#include "kftpqueueconverter.h"
#include "fileexistsactions.h"

#include "engine/directorylisting.h"
#include "engine/event.h"

#include "directoryscanner.h"
#include "kftptransfer.h"
#include "kftptransferfile.h"
#include "kftptransferdir.h"
#include "site.h"

namespace KFTPSession {
  class Session;
  class Connection;
}

class KFTPQueueConverter;

class K3Process;

typedef QList<KFTPQueue::Transfer> KFTPQueueTransfers;

namespace KFTPQueue {

class FailedTransfer;
class ManagerPrivate;

/**
 * This class represents an opened remote file. The file is stored locally
 * while its being displayed to the user.
 *
 * @author Jernej Kos <kostko@unimatrix-one.org>
 */
class OpenedFile {
public:
    OpenedFile() {}
    
    /**
     * Creates a new OpenedFile object.
     *
     * @param transfer The transfer used to transfer the file
     */
    OpenedFile(TransferFile *transfer);
    
    /**
     * Get file's source (remote).
     *
     * @return File's remote source URL
     */
    KUrl source() { return m_source; }
    
    /**
     * Get file's destination (local).
     *
     * @return File's local destination URL
     */
    KUrl destination() { return m_dest; }
    
    /**
     * Has the file changed since the transfer ?
     *
     * @return True if the file has been changed since being transfered
     */
    bool hasChanged();
private:
    KUrl m_source;
    KUrl m_dest;
    
    QString m_hash;
};

/**
 * This class represents a request for "file already exists" dialog
 * display request.
 *
 * @author Jernej Kos <kostko@unimatrix-one.org>
 */
class UserDialogRequest {
public:
    /**
     * Class constructor.
     */
    UserDialogRequest(TransferFile *transfer, filesize_t srcSize, time_t srcTime,
                      filesize_t dstSize, time_t dstTime);
    
    /**
     * Sends a response to this request.
     *
     * @param event A valid file exists wakeup event
     */
    void sendResponse(KFTPEngine::FileExistsWakeupEvent *event);
    
    /**
     * Returns the transfer that initiated the request.
     */
    TransferFile *getTransfer() const { return m_transfer; }
    
    /**
     * Returns source file size.
     */
    filesize_t sourceSize() const { return m_srcSize; }
    
    /**
     * Returns source file time.
     */
    time_t sourceTime() const { return m_srcTime; }
    
    /**
     * Returns destination file size.
     */
    filesize_t destinationSize() const { return m_dstSize; }
    
    /**
     * Returns destination file time.
     */
    time_t destinationTime() const { return m_dstTime; }
private:
    TransferFile *m_transfer;
    filesize_t m_srcSize;
    time_t m_srcTime;
    filesize_t m_dstSize;
    time_t m_dstTime;
};

/**
 * This class is responsible for managing the complete queue hierarchy. All
 * queued items descend from QueueObject and are contained in a simple tree
 * model. Statistics and abort requests propagate from bottom to top and exec
 * requests go in the other direction.
 *
 * @author Jernej Kos <kostko@unimatrix-one.org>
 */
class Manager : public QObject {
Q_OBJECT
friend class KFTPSession::Session;
friend class KFTPSession::Connection;
friend class ::KFTPQueueConverter;
friend class ::DirectoryScanner;
friend class FailedTransfer;
friend class QueueObject;
friend class ManagerPrivate;
public:
    /**
     * Returns the global manager instance.
     */
    static Manager *self();
    
    /**
     * Stop all queued transfers.
     */
    void stopAllTransfers();
    
    /**
     * Get the toplevel queue object. The direct children of this object are different
     * KFTPQueue::Site objects that represent separate sites.
     *
     * @return A QueueObject representing the toplevel object
     */
    QueueObject *topLevelObject() const { return m_topLevel; }
    
    /**
     * Queues a new transfer. This method will create the site if one doesn't exist yet
     * for this transfer. The object will be reparented under the assigned site.
     *
     * @param transfer The transfer to be queued
     */
    void insertTransfer(Transfer *transfer);
    
    /**
     * Remove a transfer from the queue. The faceDestruction method will be called on the
     * transfer object before removal. After calling this method, you shouldn't use the
     * object anymore!
     *
     * @param transfer The transfer to be removed from queue
     * @param abortSession If true any session that this transfer is using is aborted
     */
    void removeTransfer(Transfer *transfer, bool abortSession = true);
    
    /**
     * This method removes all the transfers from the queue.
     */
    void clearQueue();
    
    /**
     * Check if the transfer is under the correct site and move it if not.
     *
     * @param transfer The transfer to check
     */
    void revalidateTransfer(Transfer *transfer);
    
    /**
     * Finds a transfer by its id.
     *
     * @param id The transfer's id
     * @return The transfer object
     */
    Transfer *findTransfer(long id);
    
    /**
     * Finds a site by its URL.
     *
     * @param url The site's URL
     * @param noCreate If set to true the site will not be created when not found
     *                 and NULL will be returned
     * @return The site object
     */
    Site *findSite(KUrl url, bool noCreate = false);
    
    /**
     * Remove a failed transfer from the list.
     *
     * @param transfer The failed transfer object to be removed
     */
    void removeFailedTransfer(FailedTransfer *transfer);
    
    /**
     * Remove all failed transfers from the list. This method actually calls the
     * removeFailedTransfer for every failed transfer present.
     */
    void clearFailedTransferList();
    
    /**
     * Returns the list of failed transfers.
     *
     * @return The QList of FailedTransfer objects
     */
    QList<KFTPQueue::FailedTransfer*> *failedTransfers() { return &m_failedTransfers; }
    
    /**
     * Return the queue converter (exporter).
     *
     * @return the KFTPQueueConverter object
     */
    KFTPQueueConverter *getConverter() const { return m_converter; }
    
    /**
     * Opens the file with the registred application for it's MIME type and waits
     * for the process to exit (then it will reupload the file if it has changed).
     *
     * @param transfer The transfer whose destination should be opened
     */
    void openAfterTransfer(TransferFile *transfer);
    
    /**
     * Should the update() be emitted on changes ?
     *
     * @param value True if the value should be emitted, false otherwise
     */
    void setEmitUpdate(bool value) { m_emitUpdate = value; }
    
    /**
     * Does a global queue update and removes all transfers that have the "delete me"
     * variable set.
     */
    void doEmitUpdate();

    /**
     * Get the current download speed.
     *
     * @return The current download speed
     */
    filesize_t getDownloadSpeed() const { return m_curDownSpeed; }
    
    /**
     * Get the current upload speed.
     *
     * @return The current upload speed
     */
    filesize_t getUploadSpeed() const { return m_curUpSpeed; }
    
    /**
     * Get the percentage of the queue's completion.
     *
     * @return The percentage of the queue's completion
     */
    int getTransferPercentage();
    
    /**
     * Get the number of currently running transfers.
     *
     * @param onlyDirs Should only directories be counted
     * @return The number of currently running transfers
     */
    int getNumRunning(bool onlyDirs = false);
    
    /**
     * Get the number of currently running transfers under a specific
     * site.
     *
     * @param url The remote URL
     * @return The number of currently running transfers
     */
    int getNumRunning(const KUrl &remoteUrl);
    
    /**
     * Start the queue processing.
     */
    void start();
    
    /**
     * Abort the queue processing.
     */
    void abort();
    
    /**
     * Is the queue being processed ?
     *
     * @return True if the queue is being processed, false otherwise
     */
    bool isProcessing() { return m_queueProc->isRunning(); }
    
    /**
     * Return the next available transfer id and reserve it.
     *
     * @return The next available transfer id
     */
    long nextTransferId() { return m_lastQID++; }
    
    /**
     * Set a default action to take when encountering an existing file situation. Note that
     * the action set here will override any preconfigured user actions unless set to the
     * value of FE_DISABLE_ACT.
     *
     * @param action The action to take
     */
    void setDefaultFileExistsAction(FEAction action = FE_DISABLE_ACT) { m_defaultFeAction = action; }
    
    /**
     * Get the default action preset for situations where a file already exists.
     *
     * @return A valid FEAction
     */
    FEAction getDefaultFileExistsAction() const { return m_defaultFeAction; }
    
    /**
     * Decides what to do with the existing file. It will return a valid wakeup event to
     * dispatch. It will first consider the pre-configured "on file exists" action matrix.
     *
     * @param transfer The transfer object
     * @param srcStat Source file information (if remote)
     * @param dstStat Destination file information (if remote)
     * @return A FileExistsWakeupEvent that will be sent to the engine
     */
    KFTPEngine::FileExistsWakeupEvent *fileExistsAction(KFTPQueue::TransferFile *transfer,
                                                        QList<KFTPEngine::DirectoryEntry> stat);
    
    /**
     * Spawn a new transfer.
     *
     * @param sourceUrl Source URL
     * @param destinationUrl Destination URL
     * @param size Filesize
     * @param dir True if this transfer represents a directory
     * @param ignoreSkip Ignore skiplist for this transfer
     * @param insertToQueue Should the new transfer be queued
     * @param parent Optional parent object
     * @param noScan True if directory transfers shouldn't be scanned
     * @return A valid KFTPQueue::Transfer instance
     */
    KFTPQueue::Transfer *spawnTransfer(KUrl sourceUrl, KUrl destinationUrl, filesize_t size, bool dir,
                                       bool ignoreSkip = false, bool insertToQueue = true, QObject *parent = 0L, bool noScan = false);
protected:
    /**
     * Class constructor.
     */
    Manager();
    
    /**
     * Class destructor.
     */
    ~Manager();
    
    /**
     * Appends a new user dialog request.
     *
     * @param request Request instance
     */
    void appendUserDialogRequest(UserDialogRequest *request);
    
    /**
     * Processes the top user dialog request by opening the desired "file
     * already exists" dialog.
     */
    void processUserDialogRequest();
private:
    QueueObject *m_topLevel;
    QCache<long, QueueObject> m_queueObjectCache;
    
    QMap<pid_t, OpenedFile> m_editProcessList;
    QList<KFTPQueue::FailedTransfer*> m_failedTransfers;
    KFTPQueueProcessor *m_queueProc;
    KFTPQueueConverter *m_converter;
    
    long m_lastQID;
    bool m_emitUpdate;
    bool m_processingQueue;

    filesize_t m_curDownSpeed;
    filesize_t m_curUpSpeed;
    
    bool m_feDialogOpen;
    FEAction m_defaultFeAction;
    QList<UserDialogRequest*> m_userDialogRequests;
private slots:
    void slotQueueProcessingComplete();
    void slotQueueProcessingAborted();
    
    void slotEditProcessTerminated(K3Process *p);
signals:
    void objectAdded(KFTPQueue::QueueObject *object);
    void objectRemoved(KFTPQueue::QueueObject *object);
    void objectChanged(KFTPQueue::QueueObject *object);
    
    void objectBeforeRemoval(KFTPQueue::QueueObject *object);
    void objectAfterRemoval();
    
    void queueUpdate();
    
    void failedTransferAdded(KFTPQueue::FailedTransfer*);
    void failedTransferBeforeRemoval(KFTPQueue::FailedTransfer*);
    void failedTransferAfterRemoval();
};

}

#endif
