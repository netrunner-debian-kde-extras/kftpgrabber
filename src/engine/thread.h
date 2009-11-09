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
#ifndef KFTPENGINETHREAD_H
#define KFTPENGINETHREAD_H

#include <QThread>
#include <QSemaphore>
#include <QWaitCondition>
#include <QMutex>
#include <QList>
#include <QHash>

#include "event.h"
#include "directorylisting.h"
#include "commands.h"
#include "socket.h"

namespace KFTPEngine {

class Settings;

/**
 * The command queue handles any incoming requests for execution of
 * individual commands on sockets.
 *
 * @author Jernej Kos <kostko@unimatrix-one.org>
 */
class CommandQueue : public QObject {
public:
    /**
     * This event should be used to notify this class of newly
     * created pending commands.
     */
    class Event : public QEvent {
    public:
      /**
       * Class constructor.
       *
       * @param type Command type
       */
      Event(Commands::Type type);
      
      /**
       * Returns the event command type.
       */
      Commands::Type command() { return m_type; }
      
      /**
       * Adds a new command parameter.
       */
      void addParameter(QVariant parameter) { m_parameters.append(parameter); }
      
      /**
       * Returns a specified parameter.
       */
      QVariant parameter(int index) { return m_parameters[index]; }
    private:
      Commands::Type m_type;
      QList<QVariant> m_parameters;
    };
    
    /**
     * Class constructor.
     */
    CommandQueue(Thread *thread);
protected:
    /**
     * This method gets called when an event is delivered to
     * the command queue.
     */
    void customEvent(QEvent *event);
private:
    Thread *m_thread;
};

/**
 * This class represents a socket thread. It serves as a command queue to
 * the underlying socket implementation and also as an abstraction layer
 * to support multiple protocols.
 *
 * @author Jernej Kos <kostko@unimatrix-one.org>
 */
class Thread : public QThread
{
Q_OBJECT
friend class CommandQueue;
friend class EventHandler;
friend class Socket;
public:
    /**
     * Class constructor.
     */
    Thread();
    
    /**
     * Class destructor.
     */
    ~Thread();
    
    /**
     * Returns the event handler for this thread. Should be used to connect
     * to any signals this thread may emit.
     *
     * @return A pointer to the EventHandler object
     */
    EventHandler *eventHandler() const { return m_eventHandler; }
    
    /**
     * Returns the underlying socket object.
     *
     * @return A pointer to the Socket object
     */
    Socket *socket() const { return m_socket; }
    
    /**
     * Returns the settings object.
     */
    Settings *settings() const { return m_settings; }
    
    /**
     * Schedules a wakeup event to be passed on to the underlying socket.
     *
     * @param e The wakeup event to pass on
     */
    void wakeup(WakeupEvent *e);

    /**
     * Requests the thread to shutdown.
     */
    void shutdown();
    
    void abort();
    void connect(const KUrl &url);
    void disconnect();
    void list(const KUrl &url);
    void scan(const KUrl &url);
    void get(const KUrl &source, const KUrl &destination);
    void put(const KUrl &source, const KUrl &destination);
    void remove(const KUrl &url);
    void rename(const KUrl &source, const KUrl &destination);
    void chmod(const KUrl &url, int mode, bool recursive = false);
    void mkdir(const KUrl &url);
    void raw(const QString &raw);
    void siteToSite(Thread *thread, const KUrl &source, const KUrl &destination);
protected:
    /**
     * Thread entry point.
     */
    void run();
    
    /**
     * Emits an event to the outside world.
     */
    void emitEvent(Event::Type type, QList<QVariant> params);
    
    /**
     * Blocks the execution of this thread until a wakeup event is received. This
     * method should only be used for things like peer certificate verification
     * where user input is requested in place and methods cannot be deferred. Other
     * than that this method should NOT be used.
     *
     * @warning Such an event MUST be replied to by the main thread. Failure to
     *          do so will permanently lock this thread!
     *
     * @return A valid WakeupEvent instance or NULL if an abort ocurred
     */
    WakeupEvent *waitForWakeup();
    
    /**
     * Sets the protocol implementation to be used by the current thread. It
     * has to be set before the connect method is called, and MUST be called
     * from within this thread.
     *
     * @param protocol Wanted protocol name
     */
    void setCurrentProtocol(const QString &protocol);
    
    /**
     * @overload
     * This overloaded method is provided for convenience.
     *
     * @param url Url from which to extract the protocol
     */
    void setCurrentProtocol(const KUrl &url);
    
    /**
     * Delivers the specified command event to the command queue.
     *
     * @param event Command event
     * @param priority Event priority
     */
    void notifyCommandQueue(CommandQueue::Event *event, int priority = Qt::NormalEventPriority);
    
    /**
     * Requests a deferred execution of the next command in the
     * socket's command chain.
     *
     * This method should only be called from inside this thread!
     */
    void deferCommandExec();
protected slots:
    /**
     * Terminates the thread and schedules its deletion.
     */
    void shutdownWithTerminate();
protected:
    EventHandler *m_eventHandler;
    Socket *m_socket;
    Settings *m_settings;
    
    QHash<QString, Socket*> m_sockets;
    CommandQueue *m_commandQueue;
    QSemaphore m_startupSem;
    bool m_exit;
    int m_commandsDeferred;
    
    QMutex m_imWakeupMutex;
    QWaitCondition m_imWakeupCond;
    WakeupEvent *m_imWakeupEvent;
    bool m_immediateWakeup;
};

}

#endif
