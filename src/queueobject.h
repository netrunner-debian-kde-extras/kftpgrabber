/*
 * This file is part of the KFTPGrabber project
 *
 * Copyright (C) 2003-2005 by the KFTPGrabber developers
 * Copyright (C) 2003-2005 Jernej Kos <kostko@jweb-network.net>
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
#ifndef KFTPQUEUEQUEUEOBJECT_H
#define KFTPQUEUEQUEUEOBJECT_H

#include <QObject>
#include <QTimer>
#include <QList>

#include "engine/directorylisting.h"

namespace KFTPQueue {

class QueueGroup;

/**
 * This class represents a basic object that can be queued.
 *
 * @author Jernej Kos
 */
class QueueObject : public QObject
{
friend class QueueGroup;
Q_OBJECT
public:
    enum Type {
      File,
      Directory,
      Site,
      Toplevel
    };
    
    enum Status {
      Unknown,
      Running,
      Stopped,
      Connecting,
      Locked,
      Failed,
      Waiting
    };
    
    /**
     * Class constructor.
     *
     * @param parent Parent object
     * @param type Object type
     */
    QueueObject(QObject *parent, Type type);
    
    /**
     * Class destructor.
     */
    ~QueueObject();
    
    /**
     * Returns true if this object has a parent object.
     *
     * @return True if this object has a parent object
     */
    bool hasParentObject() const { return parent() ? parent()->inherits("KFTPQueue::QueueObject") : false; }
    
    /**
     * Returns the parent QueueObject.
     *
     * @return The parent QueueObject
     */
    QueueObject *parentObject() const { return static_cast<QueueObject*>(parent()); }
    
    /**
     * Do we have any children ?
     *
     * @return True if we have some kids
     */
    bool hasChildren() const { return m_children.count() > 0; }
    
    /**
     * Get object status.
     *
     * @return Status of this object
     */
    Status getStatus() const { return m_status; }
    
    /**
     * This method has to be called after the object has been added into
     * the queue in a proper location. Otherwise the state of this object
     * is undefined.
     */
    void readyObject();
    
    /**
     * Is this object currently running ?
     *
     * @return true if this object's status is set to Running or Connecting
     */
    bool isRunning() const { return m_status == Running || m_status == Connecting || m_status == Waiting; }
    
    /**
     * Is this object currently connecting ?
     *
     * @return True if this object's status is set to Connecting
     */
    bool isConnecting() const { return m_status == Connecting; }
    
    /**
     * Is this object currently waiting for a connection to become available
     * in one of the sessions ?
     *
     * @return True if this object's status is set to Waiting
     */
    bool isWaiting() const { return m_status == Waiting; }
    
    /**
     * Is the object currently locked ?
     *
     * @return true if this object's status is set to Locked
     */
    bool isLocked() const { return m_status == Locked; }
    
    /**
     * Is this object currently aborting ?
     *
     * @return true if this object is currently aborting
     */
    bool isAborting() const { return m_aborting; }
    
    /**
     * Returns the size of this queue object.
     *
     * @return Size
     */
    filesize_t getSize() const { return m_size; }
    
    /**
     * Returns the actual size - that is usefull only if this is a directory since
     * it returns the current size of all its items (if some items were removed
     * from its first scan getSize() will return the initial size and getActualSize()
     * will return current size of all items). If this is not a directory, this
     * will return the same value as getSize();
     *
     * @return Actual directory size
     */
    filesize_t getActualSize() const { return m_actualSize; }
    
    /**
     * Returns the already transfered file/dir size.
     *
     * @return Transfered file/dir size
     */
    filesize_t getCompleted() const { return m_completed; }
    
    /**
     * Returns the number of bytes that have been resumed (using REST).
     *
     * @return Resume offset
     */
    filesize_t getResumed() const { return m_resumed; }
    
    /**
     * Get current transfer speed or 0 if the transfer is stalled.
     *
     * @return Transfer speed
     */
    filesize_t getSpeed() const { return m_speed; }
    
    /**
     * Returns this transfer's current progress.
     *
     * @return A pair of two values - progress and percent resumed
     */
    QPair<int, int> getProgress() const;
    
    /**
     * Adds size bytes to the current transfer size. This will also update all
     * parent transfers (if any).
     *
     * @param size Size to add
     */
    void addSize(filesize_t size);
    
    /**
     * Adds completed bytes to the current completed size. This will also update all
     * parent transfers (if any).
     *
     * @param completed Size to add
     */
    void addCompleted(filesize_t completed);
    
    /**
     * Set the current transfer speed. This will also update all parent transfers.
     *
     * @param speed Speed to set
     */
    void setSpeed(filesize_t speed = 0);
    
    /**
     * Returns the KFTPQueue::Transfer::Type of this transfer. This can either be
     * File or Directory.
     *
     * @return Transfer type
     */
    Type getType() const { return m_type; }
    
    /**
     * Is this object a directory ?
     *
     * @return true if this object's type is set to Directory
     */
    bool isDir() const { return m_type == Directory; }
    
    /**
     * Is this object a transfer ?
     *
     * @return true if this object's type is File or Directory
     */
    bool isTransfer() const { return m_type == File || m_type == Directory; }
    
    /**
     * Is this object a site ?
     *
     * @return True if this object's type is Site
     */
    bool isSite() const { return m_type == Site; }
    
    /**
     * Is this object the toplevel object ?
     *
     * @return True if this object's type is Toplevel
     */
    bool isToplevel() const { return m_type == Toplevel; }
    
    /**
     * Delays transfer execution for msec miliseconds. If this number is greater than
     * 1000, it will be set to 1000.
     *
     * @param msec Number of miliseconds to delay execution
     */
    void delayedExecute(int msec = 100);
    
    /**
     * Set transfer's ID.
     *
     * @param id Transfer identifier (must be unique)
     */
    void setId(long id) { m_id = id; }
    
    /**
     * Get transfer's ID.
     *
     * @return Transfer's unique ID number
     */
    long getId() const { return m_id; }
    
    /**
     * Abort current transfer.
     */
    virtual void abort();
    
    /**
     * Add a child queue object to this object. The object is NOT reparented!
     *
     * @param object The child queue object
     */
    void addChildObject(QueueObject *object);
    
    /**
     * Delete a child queue object from this object. The object is NOT reparented!
     *
     * @param object The child queue object
     */
    void delChildObject(QueueObject *object);
    
    /**
     * Find a QueueObject that is child of the current object by its id. This
     * method goes trough all the objects under this one.
     *
     * @param id Object's id
     * @return A valid QueueObject or NULL if no such object can be found
     */
    QueueObject *findChildObject(long id);
    
    /**
     * Removes all transfers that have been marked for deletion.
     */
    void removeMarkedTransfers();
    
    /**
     * Move a child object up in the queue.
     *
     * @param child The object to move
     */
    void moveChildUp(QueueObject *child);
    
    /**
     * Move a child object down in the queue.
     *
     * @param child The object to move
     */
    void moveChildDown(QueueObject *child);
    
    /**
     * Move a child object to the top.
     *
     * @param child The object to move
     */
    void moveChildTop(QueueObject *child);
    
    /**
     * Move a child object to the bottom.
     *
     * @param child The object to move
     */
    void moveChildBottom(QueueObject *child);
    
    /**
     * Can a child be moved up ?
     *
     * @param child The child to be moved
     * @return True if the child can be moved up
     */
    bool canMoveChildUp(QueueObject *child);
    
    /**
     * Can a child be moved down ?
     *
     * @param child The child to be moved
     * @return True if the child can be moved down
     */
    bool canMoveChildDown(QueueObject *child);
    
    /**
     * Returns the list of this object's child QueueObjects.
     *
     * @return A QueueObject list
     */
    QList<QueueObject*> getChildrenList() const { return m_children; }
    
    /**
     * Returns the child at index position i in the list.
     *
     * @param i Child index
     * @return A valid QueueObject instance
     */
    QueueObject *getChildAt(int i) const { return m_children.at(i); }
    
    /**
     * Returns this object's relative index to the parent object.
     */
    int index() const;
    
    /**
     * Returns the number of child objects.
     */
    int childCount() const { return m_children.size(); }
public slots:
    /**
     * Execute this queue object.
     */
    virtual void execute();
protected:
    bool m_aborting;
    Status m_status;
    
    long m_id;
    Type m_type;
    QList<QueueObject*> m_children;
    
    QTimer m_delayedExecuteTimer;
    
    /* Statistical information */
    filesize_t m_size;
    filesize_t m_actualSize;
    filesize_t m_completed;
    filesize_t m_resumed;
    filesize_t m_speed;
    
    void addActualSize(filesize_t size);
    
    /**
     * This method is called every time the object's statistics must be
     * updated.
     *
     * @warning This is the ONLY place where the Manager's objectChanged
     *          signal may be invoked from!
     */
    virtual void statisticsUpdated();
    
    /**
     * This method should return true if the object can be moved.
     */
    virtual bool canMove();
private slots:
    void slotChildDestroyed(QObject *child);
signals:
    /**
     * This signal gets emitted when the object's state has changed in
     * some way.
     */
    void objectUpdated();
};

}

Q_DECLARE_METATYPE(KFTPQueue::QueueObject*)

#endif
