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
#include "queueobject.h"
#include "kftpqueue.h"

#include <qtimer.h>

namespace KFTPQueue {

QueueObject::QueueObject(QObject *parent, Type type)
 : QObject(parent),
   m_aborting(false),
   m_status(Unknown),
   m_type(type),
   m_size(0),
   m_actualSize(0),
   m_completed(0),
   m_resumed(0),
   m_speed(0)
{
  // Add the transfer
  if (hasParentObject())
    static_cast<QueueObject*>(parent)->addChildObject(this);
  
  // Connect the delayed execution timer
  connect(&m_delayedExecuteTimer, SIGNAL(timeout()), this, SLOT(execute()));
}


QueueObject::~QueueObject()
{
}

int QueueObject::index() const
{
  if (m_type == Toplevel)
    return 0;
  
  QList<QueueObject*> children = parentObject()->getChildrenList();
  
  for (int i = 0; i < children.size(); i++) {
    if (children.at(i) == this)
      return i;
  }
  
  Q_ASSERT(false);
  return -1;
}

void QueueObject::readyObject()
{
  m_status = Stopped;
}

void QueueObject::delayedExecute(int msec)
{
  /* Execute the transfer with delay - using a QTimer */
  if (msec > 1000)
    msec = 1000;
  
  if (!m_delayedExecuteTimer.isActive()) {
    m_delayedExecuteTimer.setSingleShot(true);
    m_delayedExecuteTimer.start(msec);
  }
}

void QueueObject::execute()
{
}

QPair<int, int> QueueObject::getProgress() const
{
  int progress = 0;
  int resumed = 0;
  
  if (m_size)
    progress = m_completed * 100 / m_size;
  
  if (m_resumed)
    resumed = m_resumed * 100 / m_size;
  
  return QPair<int, int>(progress, resumed);
}

void QueueObject::addActualSize(filesize_t size)
{
  if (size == 0)
    return;
  
  m_actualSize += size;
  
  if (hasParentObject())
    parentObject()->addActualSize(size);
    
  statisticsUpdated();
}

void QueueObject::addSize(filesize_t size)
{
  if (size == 0)
    return;
  
  m_size += size;
  m_actualSize += size;
  
  if (hasParentObject())
    parentObject()->addSize(size);
  
  statisticsUpdated();
}

void QueueObject::addCompleted(filesize_t completed)
{
  if (completed == 0)
    return;
  
  m_completed += completed;
  
  if (hasParentObject())
    parentObject()->addCompleted(completed);
    
  statisticsUpdated();
}

void QueueObject::setSpeed(filesize_t speed)
{
  if (speed != 0 && m_speed == speed)
    return;
  
  m_speed = speed;
  
  foreach (QueueObject *i, m_children) {
    m_speed += i->getSpeed();
  }
  
  if (hasParentObject())
    parentObject()->setSpeed();
    
  statisticsUpdated();
}

void QueueObject::statisticsUpdated()
{
  if (m_status == Unknown)
    return;
  
  emit objectUpdated();
  emit KFTPQueue::Manager::self()->objectChanged(this);
}

void QueueObject::abort()
{
}

void QueueObject::addChildObject(QueueObject *object)
{
  m_children.append(object);
  
  connect(object, SIGNAL(destroyed(QObject*)), this, SLOT(slotChildDestroyed(QObject*)));
}

void QueueObject::delChildObject(QueueObject *object)
{
  m_children.removeAll(object);
}

void QueueObject::slotChildDestroyed(QObject *child)
{
  // Remove the transfer
  delChildObject(static_cast<QueueObject*>(child));
}

QueueObject *QueueObject::findChildObject(long id)
{
  foreach (QueueObject *i, m_children) {
    if (i->getId() == id)
      return i;
      
    if (i->hasChildren()) {
      QueueObject *tmp = i->findChildObject(id);
      
      if (tmp)
        return tmp;
    }
  }
  
  return NULL;
}

void QueueObject::removeMarkedTransfers()
{
  foreach (QueueObject *i, m_children) {
    if (i->hasChildren())
      i->removeMarkedTransfers();
    
    if (i->isTransfer() && static_cast<Transfer*>(i)->isDeleteMarked())
      Manager::self()->removeTransfer(static_cast<Transfer*>(i));
  }
}

bool QueueObject::canMove()
{
  return true;
}

void QueueObject::moveChildUp(QueueObject *child)
{
  int pos = m_children.indexOf(child);
  
  if (pos != -1) {
    emit KFTPQueue::Manager::self()->objectRemoved(child);
    
    m_children.removeAll(child);
    m_children.insert(pos - 1, child);
    
    emit KFTPQueue::Manager::self()->objectAdded(child);
  }
}

void QueueObject::moveChildDown(QueueObject *child)
{
  int pos = m_children.indexOf(child);
  
  if (pos != -1) {
    emit KFTPQueue::Manager::self()->objectRemoved(child);
    
    m_children.removeAll(child);
    m_children.insert(pos + 1, child);
    
    emit KFTPQueue::Manager::self()->objectAdded(child);
  }
}

void QueueObject::moveChildTop(QueueObject *child)
{
  emit KFTPQueue::Manager::self()->objectRemoved(child);
  
  m_children.removeAll(child);
  m_children.prepend(child);
  
  emit KFTPQueue::Manager::self()->objectAdded(child);
}

void QueueObject::moveChildBottom(QueueObject *child)
{
  emit KFTPQueue::Manager::self()->objectRemoved(child);
  
  m_children.removeAll(child);
  m_children.append(child);
  
  emit KFTPQueue::Manager::self()->objectAdded(child);
}

bool QueueObject::canMoveChildUp(QueueObject *child)
{
  if (!child->canMove())
    return false;
  
  if (m_children.first() == child)
    return false;
  
  QueueObject *upper = m_children.value(m_children.indexOf(child) - 1);
  
  if (upper && !upper->canMove())
    return false;
  
  return true;
}

bool QueueObject::canMoveChildDown(QueueObject *child)
{
  if (!child->canMove())
    return false;
  
  if (m_children.last() == child)
    return false;
  
  QueueObject *lower = m_children.value(m_children.indexOf(child) - 1);
  
  if (lower && !lower->canMove())
    return false;
  
  return true;
}

}
#include "queueobject.moc"
