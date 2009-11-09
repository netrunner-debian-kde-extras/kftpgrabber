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

#ifndef KFTPQUEUEKFTPTRANSFER_H
#define KFTPQUEUEKFTPTRANSFER_H

#include "queueobject.h"

#include <qobject.h>
#include <qtimer.h>
#include <qpointer.h>

#include <kurl.h>

namespace KFTPSession {
  class Session;
  class Connection;
}

namespace KFTPQueue {

enum TransferType {
    Download = 0,
    Upload = 1,
    FXP = 2
};

class TransferFile;

/**
 * This class represents a failed transfer. Such a transfer is removed
 * from queue so the error message can later be examined and the transfer
 * restarted.
 *
 * @author Jernej Kos
 */
class FailedTransfer : public QObject
{
Q_OBJECT
public:
    /**
     * Constructs a new failed transfer object. The actual transfer
     * will be reparented (the FailedTransfer object will become its
     * parent).
     */
    FailedTransfer(QObject *parent, TransferFile *transfer, const QString &error);
    ~FailedTransfer();
    
    /**
     * Returns the error message.
     *
     * @return The error message.
     */
    QString getError() const { return m_error; }
    
    /**
     * Add this transfer back to the queue. The FailedTransfer object
     * will be destroyed afterwards!
     *
     * @return Pointer to the TransferFile object that was just restored.
     */
    TransferFile *restore();
    
    /**
     * Returns the actual transfer object that failed. This transfer is
     * marked as failed so execute() method can't be called!
     *
     * @return A KFTPQueue::TransferFile object.
     */
    TransferFile *getTransfer() const { return m_transfer; }
    
    /**
     * Use this method to declare a transfer as failed. The transfer will
     * be aborted, removed from queue and added to the failed transfer
     * list.
     *
     * @param transfer Pointer to the transfer object that failed.
     * @param error The error that ocurred.
     */
    static void fail(TransferFile *transfer, const QString &error);
private:
    QPointer<TransferFile> m_transfer;
    QString m_error;
};

/**
 * This class is the base class for all transfers used in KFTPGrabber. It
 * provides some basic methods that are extended by KFTPQueue::TransferFile and
 * KFTPQueue::TransferDir for specific file or dir operations.
 *
 * @author Jernej Kos
 */
class Transfer : public QueueObject
{
friend class FailedTransfer;
friend class TransferDir;
friend class Manager;
friend class KFTPSession::Session;
friend class KFTPSession::Connection;
Q_OBJECT
public:
    Transfer(QObject *parent, Type type);
    ~Transfer();
    
    /**
     * Returns the source KUrl of this transfer.
     *
     * @return Source url
     */
    KUrl getSourceUrl() const { return m_sourceUrl; }
    
    /**
     * Returns the destination KUrl of this transfer.
     *
     * @return Destination url
     */
    KUrl getDestUrl() const { return m_destUrl; }
    
    /**
     * Set the source KUrl of this transfer.
     *
     * @param url Source url wannabe
     */
    void setSourceUrl(const KUrl &url) { m_sourceUrl = url; }
    
    /**
     * Set the destination url of this transfer.
     *
     * @param url Destination url wannabe
     */
    void setDestUrl(const KUrl &url) { m_destUrl = url; }
        
    /**
     * Return the KFTPQueue::TransferType -- that is if this transfer is an Upload, Download
     * or FXP transfer.
     *
     * @return Upload, Download or FXP
     */
    TransferType getTransferType() const { return m_transferType; }
    
    /**
     * Set current KFTPQueue::TransferType -- that is Upload, Download or FXP
     *
     * @param type Upload, Download or FXP
     */
    void setTransferType(TransferType type) { m_transferType = type; }
    
    /**
     * Get the source session for this transfer.
     *
     * @return A valid KFTPSession::Session instance or 0 if not started
     */
    KFTPSession::Session *getSourceSession() const { return m_srcSession; }
    
    /**
     * Get the destination session for this transfer.
     *
     * @return A valid KFTPSession::Session instance or 0 if not started
     */
    KFTPSession::Session *getDestinationSession() const { return m_dstSession; }
    
    /**
     * Returns the connection opposite of one that is passed. So if you
     * pass the source connection, the destination one is returned and
     * vice-versa.
     *
     * @param conn The connection
     * @return The opposite Connection
     */
    KFTPSession::Connection *getOppositeConnection(KFTPSession::Connection *conn);
    
    /**
     * Returns the remote connection. If both connections are remote, this
     * method returns the source connection.
     *
     * @return The remote connection
     */
    KFTPSession::Connection *remoteConnection();
    
    /**
     * Is this transfer a child of another transfer ?
     *
     * @return true if this transfer is a child of another KFTPQueue::Transfer
     */
    bool hasParentTransfer() const { return parent()->inherits("KFTPQueue::Transfer"); }
    
    /**
     * Should a transfered file be automagicly opened after transfer ? This only applies for
     * download transfers.
     *
     * @param value The setting
     */
    void setOpenAfterTransfer(bool value) { m_openAfterTransfer = value; }
    
    /**
     * Is this transfer marked for deletion ?
     *
     * @return true if this transfer is marked for deletion
     */
    bool isDeleteMarked() const { return m_deleteMe; }
    
    /**
     * Get the transfer's parent transfer.
     *
     * @return Transfer's parent or NULL if isChild() returns false
     */
    Transfer *parentTransfer();
    
    /**
     * Lock this transfer for further changes. 
     */
    void lock();
    
    /**
     * Unlock a previously locked transfer.
     */
    void unlock();
    
    /**
     * Abort current transfer.
     */
    virtual void abort();
    
    /**
     * Just emits the objectUpdated() signal.
     */
    void emitUpdate() { emit objectUpdated(); }
    
    /**
     * Assign sessions to this transfer in advance (= before starting the
     * actual transfer). Both sessions must have free connections. If you
     * pass NULL to both parameters sessions will be looked up and might
     * be spawned.
     *
     * Note that the sessions MUST be the right ones based on the transfer's
     * URL, otherwise unexpected results will ocurr!
     *
     * @param source The source session
     * @param destination The destination session
     * @return True if the sessions are ready for immediate use
     */
    virtual bool assignSessions(KFTPSession::Session *source = 0, KFTPSession::Session *destination = 0);
    
    /**
     * This method returns true if both connections have been properly
     * initialized.
     */ 
    bool connectionsReady();
protected:
    bool m_deleteMe;
    bool m_openAfterTransfer;
    TransferType m_transferType;
    
    /* Source/destination URL */
    KUrl m_sourceUrl;
    KUrl m_destUrl;
    
    /* Transfer sessions */
    KFTPSession::Session *m_srcSession;
    KFTPSession::Session *m_dstSession;
    
    /* Source/destination connections */
    KFTPSession::Connection *m_srcConnection;
    KFTPSession::Connection *m_dstConnection;
    
    int m_retryCount;
    
    void showTransCompleteBalloon();
    void resetTransfer();
    
    void update();
    bool canMove();
    
    /**
     * This method gets called just before the transfer is removed.
     *
     * @param abortSession If true any session that this transfer is using is aborted
     */
    void faceDestruction(bool abortSession = true);
    
    /**
     * Initialize the specified session for use with this transfer.
     *
     * @param session The session to use
     * @return A valid Connection or NULL if one wasn't available
     */
    KFTPSession::Connection *initializeSession(KFTPSession::Session *session);
    
    /**
     * Deinitialize currently acquired connections. Do not call this method
     * unless you know what you are doing.
     */
    void deinitializeConnections();
private slots:
    void slotConnectionAvailable();
    void slotConnectionConnected();
signals:
    void transferStart(long id);
    void transferComplete(long id);
    void transferAbort(long id);
};

}

Q_DECLARE_METATYPE(KFTPQueue::FailedTransfer*)

#endif

