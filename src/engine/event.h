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

#ifndef KFTPNETWORKEVENT_H
#define KFTPNETWORKEVENT_H

#include <QObject>
#include <QEvent>
#include <QList>

#include "directorylisting.h"

namespace KFTPEngine {

/**
 * Engine reset codes. TODO description of each reset code.
 */
enum ResetCode {
  Ok,
  UserAbort,
  Failed,
  FailedSilently
};

/**
 * Engine error codes. TODO: description of each error code.
 */
enum ErrorCode {
  ConnectFailed,
  LoginFailed,
  PermissionDenied,
  FileNotFound,
  OperationFailed,
  ListFailed,
  FileOpenFailed
};

/**
 * A wakeup event is a special type event used to transfer some response from
 * the GUI to the engine that has been temporarly suspended. After receiving
 * this event, the current command handler's wakeup() method will be called
 * with this event as a parameter.
 *
 * @author Jernej Kos <kostko@jweb-network.net>
 */
class WakeupEvent {
public:
    /**
     * Possible wakeup event types. Each type should subclass this class to
     * provide any custom methods needed.
     */
    enum Type {
      WakeupFileExists,
      WakeupPubkey,
      WakeupPeerVerify
    };
    
    /**
     * Constructs a new wakeup event of specified type.
     *
     * @param type Event type
     */
    WakeupEvent(Type type) : m_type(type) {}
private:
    Type m_type;
};

/**
 * A file exists wakeup event that is used to continue pending transfers.
 *
 * @author Jernej Kos <kostko@jweb-network.net>
 */
class FileExistsWakeupEvent : public WakeupEvent {
public:
    /**
     * Possible actions the engine can take.
     */
    enum Action {
      Overwrite,
      Rename,
      Resume,
      Skip
    };
    
    /**
     * Constructs a new file exists wakeup event with Skip action as default.
     */
    FileExistsWakeupEvent() : WakeupEvent(WakeupFileExists), action(Skip) {}
    
    Action action;
    QString newFileName;
};

/**
 * A public key password response event for SFTP connections.
 *
 * @author Jernej Kos <kostko@jweb-network.net>
 */
class PubkeyWakeupEvent : public WakeupEvent {
public:
    /**
     * Constructs a new public key wakeup event.
     */
    PubkeyWakeupEvent() : WakeupEvent(WakeupPubkey) {}
    
    QString password;
};

/**
 * A peer verification response event for FTPS and SFTP connections.
 *
 * @author Jernej Kos <kostko@jweb-network.net>
 */
class PeerVerifyWakeupEvent : public WakeupEvent {
public:
    /**
     * Constructs a new peer verify wakeup event.
     */
    PeerVerifyWakeupEvent() : WakeupEvent(WakeupPeerVerify) {}
    
    bool peerOk;
};

/**
 * This class represents an event that is passed to the EventHandler for
 * processing. It can have multiple EventParameters.
 *
 * @author Jernej Kos <kostko@jweb-network.net>
 */
class Event : public QEvent {
public:
    enum Type {
      EventMessage,
      EventCommand,
      EventResponse,
      EventMultiline,
      EventRaw,
      EventDirectoryListing,
      EventDisconnect,
      EventError,
      EventConnect,
      EventReady,
      EventState,
      EventScanComplete,
      EventRetrySuccess,
      EventReloadNeeded,
      
      // Transfer events
      EventTransferComplete,
      EventResumeOffset,
      
      // Events that require wakeup events
      EventFileExists,
      EventPubkeyPassword,
      EventPeerVerify
    };

    /**
     * Construct a new event with a parameter list.
     *
     * @param params Parameter list
     */
    Event(Type type, QList<QVariant> params);
    ~Event();
    
    /**
     * Return the event's type.
     *
     * @return Event's type
     */
    Type type() { return m_type; }
    
    /**
     * Returns the parameter with a specific index.
     *
     * @param index Parameter's index
     * @return A parameter as QVariant
     */
    QVariant getParameter(int index) { return m_params[index]; }
protected:
    Type m_type;
    QList<QVariant> m_params;
};

class Thread;

/**
 * This class handles events receieved from the thread and passes them
 * on to the GUI as normal Qt signals.
 *
 * @author Jernej Kos <kostko@jweb-network.net>
 */
class EventHandler : public QObject {
Q_OBJECT
public:
    /**
     * Construct a new event handler.
     *
     * @param thread The thread this event handler belongs to
     */
    EventHandler(Thread *thread);
protected:
    void customEvent(QEvent *e);
protected:
    Thread *m_thread;
signals:
    void engineEvent(KFTPEngine::Event *event);
    
    void connected();
    void disconnected();
    void gotResponse(const QString &text);
    void gotRawResponse(const QString &text);
};

}

Q_DECLARE_METATYPE(KFTPEngine::WakeupEvent*)
Q_DECLARE_METATYPE(KFTPEngine::ErrorCode)

#endif
