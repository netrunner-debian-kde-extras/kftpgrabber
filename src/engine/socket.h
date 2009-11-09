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
#ifndef KFTPENGINESOCKET_H
#define KFTPENGINESOCKET_H

#include <KUrl>
#include <KRemoteEncoding>

#include <QStack>
#include <QPointer>
#include <QDateTime>
#include <QHostAddress>

#include "settings.h"
#include "commands.h"

namespace KFTPEngine {

class ConnectionRetry;

/**
 * A representation of a socket address.
 */
struct SocketAddress {
    QHostAddress ip;
    quint16 port;
};

enum SocketFeatures {
  SF_FXP_TRANSFER = 1,
  SF_RAW_COMMAND  = 2
};

/**
 * The socket class provides an abstract class for all implemented protocols. It
 * provides basic methods and also some remote operations (recursive scan,
 * recursive removal).
 *
 * @author Jernej Kos <kostko@jweb-network.net>
 */
class Socket
{
friend class Thread;
friend class FtpCommandStat;
public:
    /**
     * Constructs a new socket.
     *
     * @param thread The thread that created this socket
     * @param protocol The protocol name
     */
    Socket(Thread *thread, const QString &protocol);
    virtual ~Socket();
    
    /**
     * Set an internal config value.
     *
     * @param key Key
     * @param value Value
     */
    void setConfig(const QString &key, const QVariant &value) { m_settings->setConfig(key, value); }
    
    /**
     * Get an internal config value as string.
     *
     * @param key Key
     * @return The key's value or an empty string if the key doesn't exist
     */
    QString getConfig(const QString &key) const { return m_settings->getConfig(key); }
    
    /**
     * Get an internal config value.
     *
     * @param key Key
     * @param default Default value to use instead
     * @return The key's value or the default value if not found
     */
    template <typename T>
    T getConfig(const QString &key, const T &def = T()) const { return m_settings->getConfig<T>(key, def); }
    
    /**
     * This method should trigger the connection process.
     *
     * @param url Remote url to connect to
     */
    virtual void protoConnect(const KUrl &url) = 0;
    
    /**
     * This method should disconnect from the remote host.
     */
    virtual void protoDisconnect();
    
    /**
     * This method should abort any ongoing action.
     */
    virtual void protoAbort();
    
    /**
     * This method should download a remote file and save it localy.
     *
     * @param source The source url
     * @param destination The destination url
     */
    virtual void protoGet(const KUrl &source, const KUrl &destination) = 0;
    
    /**
     * This method should upload a local file and save it remotely.
     *
     * @param source The source url
     * @param destination The destination url
     */
    virtual void protoPut(const KUrl &source, const KUrl &destination) = 0;
    
    /**
     * Each protocol should implement this method. It should remove just one
     * single entry. A config variable "params.remove.directory" will be set
     * to 1 if the entry to remove is a directory and to 0 if it should expect
     * a file.
     *
     * @warning You should NOT use this method directly! Use @ref protoDelete
     *          instead!
     * @param path The path to the entry to remove
     */
    virtual void protoRemove(const KUrl &path) = 0;
    
    /**
     * This method should rename/move a remote file.
     *
     * @param source The source file path
     * @param destination The destination file path
     */
    virtual void protoRename(const KUrl &source, const KUrl &destination) = 0;
    
    /**
     * This method should change file's mode.
     *
     * * @warning You should NOT use this method directly! Use @ref protoChmod
     *            instead!
     * @param path The file's path
     * @param mode The new file mode
     */
    virtual void protoChmodSingle(const KUrl &path, int mode) = 0;
    
    /**
     * This method should create a new remote directory.
     *
     * @param path Path of the newly created remote directory
     */
    virtual void protoMkdir(const KUrl &path) = 0;
    
    /**
     * This method should fetch the remote directory listing for a specified
     * directory. Note that this method could be called as a chained command,
     * so it MUST NOT emit an EventDirectoryListing event if isChained returns
     * true! In this case it should save the directory listing to the
     * m_lastDirectoryListing member variable.
     *
     * @param path The path to list
     */
    virtual void protoList(const KUrl &path) = 0;
    
    /**
     * This method should fetch the information about the given path. It is
     * usualy called as a chained command.
     *
     * @param path The path to stat
     */
    virtual void protoStat(const KUrl &path);
    
    /**
     * This method should send a raw command in case the protocol supports it
     * (the SF_RAW_COMMAND is among features).
     *
     * @param command The command to send
     */
    virtual void protoRaw(const QString&) {}
    
    /**
     * This method should initiate a site to site transfer in case the protocol
     * supports it (the SF_FXP_TRANSFER is among features).
     *
     * @param socket The destination socket
     * @param source The source url
     * @param destination The destination url
     */
    virtual void protoSiteToSite(Socket*, const KUrl&, const KUrl&) {}
    
    /**
     * Send a packet to keep the connection alive.
     */
    virtual void protoKeepAlive() {}
    
    /**
     * Recursively scan a directory and emit a DirectoryTree that can be used to
     * create new transfers for addition to the queue.
     *
     * @param path The path to recursively scan
     */
    void protoScan(const KUrl &path);
    
    /**
     * Identify if the remote path is a file or a directory and recursively remove
     * it if so. The difference between this command and @ref protoRemove is, that
     * protoRemove removes just one entry, and doesn't identify file type.
     *
     * @param path The path to remove
     */
    void protoDelete(const KUrl &path);
    
    /**
     * Change file or directory mode. Also supports recursive mode changes.
     *
     * @param path The file's path
     * @param mode The new file mode
     * @param recursive Should the mode be recursively changed
     */
    void protoChmod(const KUrl &path, int mode, bool recursive);
    
    /**
     * Returns this socket's parent thread.
     *
     * @return Socket's parent thread
     */
    Thread *thread() { return m_thread; }
    
    /**
     * Returns the protocol name of this socket.
     *
     * @return This socket's protocol name
     */
    QString protocolName() { return m_protocol; }
    
    /**
     * This method should return the socket's features by or-ing the values in
     * SocketFeatures enum.
     *
     * @return Socket's features
     */
    virtual int features() = 0;
    
    /**
     * This method should return true if this socket is connected.
     *
     * @return True if the socket has successfully connected
     */
    virtual bool isConnected() = 0;
    
    /**
     * This method should return true if the connection is encrypted by some method.
     *
     * @return True if the connection is encrypted
     */
    virtual bool isEncrypted() = 0;
    
    /**
     * Returns true if the socket is currently busy performing an action.
     *
     * @return True if the socket is busy
     */
    virtual bool isBusy() { return m_currentCommand != Commands::CmdNone; }

    /**
     * Emit an engine error code.
     *
     * @param code The error code
     * @param param1 Optional string parameter
     */
    void emitError(ErrorCode code, const QString &param1 = 0);
    
    /**
     * Emit an engine event.
     *
     * @param type Event type
     * @param param1 Optional string parameter
     * @param param2 Optional string parameter
     */
    void emitEvent(Event::Type type, const QString &param1 = 0, const QString &param2 = 0);
    
    /**
     * Emit an engine event containing a directory listing.
     *
     * @param type Event type
     * @param param1 The DirectoryListing parameter
     */
    void emitEvent(Event::Type type, DirectoryListing param1);
    
    /**
     * Emit an engine event containing a directory tree.
     *
     * @param type Event type
     * @param param1 The DirectoryTree parameter
     */
    void emitEvent(Event::Type type, DirectoryTree *param1);
    
    /**
     * Emit an engine event containing a custom QVariant.
     *
     * @param type Event type
     * @param param1 The custom variant
     */
    void emitEvent(Event::Type type, const QVariant &param1);
    
    /**
     * Emits an engine event containing a custom QVariant and blocks
     * until a response is received.
     *
     * @warning See Thread::waitForWakeup warning before using this method!
     * @param type Event type
     * @param param1 The custom variant
     * @return The WakeupEvent that responded to this call
     */
    WakeupEvent *emitEventAndWait(Event::Type type, const QVariant &param1 = QVariant());
    
    /**
     * This method will set the socket's remote encoding which will be used when
     * converting filenames into UTF-8 and back.
     *
     * @param encoding A valid encoding name
     */
    virtual void changeEncoding(const QString &encoding);
    
    /**
     * Retrieve the KRemoteEncoding object for this socket set to the appropriate
     * encoding.
     *
     * @return The KRemoteEncoding object
     */
    KRemoteEncoding *remoteEncoding() { return m_remoteEncoding; }
    
    /**
     * Sets the current directory path.
     *
     * @param path The current directory path
     */
    void setCurrentDirectory(const QString &path) { m_currentDirectory = path; }
    
    /**
     * Get the current directory path.
     *
     * @return The current directory path.
     */
    virtual QString getCurrentDirectory() { return m_currentDirectory; }
    
    /**
     * Sets the default directory path (like a remote home directory).
     *
     * @param path The default directory path
     */
    void setDefaultDirectory(const QString &path) { m_defaultDirectory = path; }
    
    /**
     * Get the default directory path.
     *
     * @return The default directory path
     */
    virtual QString getDefaultDirectory() { return m_defaultDirectory; }
    
    /**
     * Sets the url this socket is connected to.
     *
     * @param url The url this socket is connected to
     */
    void setCurrentUrl(const KUrl &url) { m_currentUrl = url; }
    
    /**
     * Get the url this socket is connected to.
     *
     * @return The url this socket is currently connected to
     */
    KUrl getCurrentUrl() { return m_currentUrl; }
    
    /**
     * Sets the command the socket is currently executing.
     *
     * @param type Command type
     */
    void setCurrentCommand(Commands::Type type) { m_currentCommand = type; }
    
    /**
     * Get the current socket command.
     *
     * @return The current socket command
     */
    Commands::Type getCurrentCommand();
    
    /**
     * Get the toplevel socket command in the command chain.
     *
     * @return The toplevel socket command
     */
    Commands::Type getToplevelCommand();
    
    /**
     * Get the command that executed the current command. Note that this
     * is valid only if the current command is chained. Otherwise this
     * method returns Commands::CmdNone.
     *
     * @return The previous command
     */
    Commands::Type getPreviousCommand();
    
    /**
     * Get the last directory listing made by protoList.
     *
     * @return The last directory listing
     */
    DirectoryListing getLastDirectoryListing() { return m_lastDirectoryListing; }
    
    /**
     * Get the last stat response made by protoStat.
     *
     * @return The last stat response
     */
    DirectoryEntry getStatResponse() { return m_lastStatResponse; }
    
    /**
     * Get the number of bytes transfered from the beginning of the transfer.
     *
     * @return The number of bytes transfered
     */
    filesize_t getTransferBytes() { return m_transferBytes; }
    
    /**
     * Get the current transfer speed.
     *
     * @return The current transfer speed.
     */
    filesize_t getTransferSpeed();
    
    /**
     * Wakeup the last command processor with a specific wakeup event. This
     * is used for async two-way communication between the engine and the
     * GUI (wakeup event is a reply from the GUI).
     *
     * By default this method just passes the event to the currently active
     * command processor.
     *
     * @param event The wakeup event that should be passed to the command class
     */
    virtual void wakeup(WakeupEvent *event);
    
    /**
     * Reset the current command class, possibly invoking the calling chained
     * command class or completing the operation.
     *
     * @param code The result code
     */
    virtual void resetCommandClass(ResetCode code = Ok);
    
    /**
     * Add a command class to the command chain so that it will be executed next.
     *
     * @param cmd The command class to add
     */
    void addToCommandChain(Commands::Base *cmd) { m_commandChain.append(cmd); }
    
    /**
     * Execute the next command.
     */
    void nextCommand();
    
    /**
     * Schedule the execution of the next command in the next thread loop.
     */
    void nextCommandAsync();
    
    /**
     * Returns true if the current command has been chained from another command class.
     *
     * @return True if the current command has been chained
     */
    bool isChained() const { return m_commandChain.count() > 0; }
    
    /**
     * Set the error reporting on or off. This variable is then used by some
     * command classes do determine if they should emit errors and reset with
     * failure or if they should just silently ignore the error and reset
     * the command class with an Ok code.
     *
     * @param value Error reporting value
     */
    void setErrorReporting(bool value) { m_errorReporting = value; }
    
    /**
     * Get the current error reporting setting. This only makes sense if the
     * class is chained - otherwise this allways returns true.
     *
     * @param ignoreChaining Set to true to always return the current setting
     * @return The current error reporting setting
     */
    bool errorReporting(bool ignoreChaining = false) const;

    /**
     * This method can be used to pass values from one chained command to the
     * other. Usually a result of some action should be saved here.
     *
     * @param value Chained command return value
     */
    void setReturnValue(const QVariant &value) { setConfig("__returnvalue", value); }
    
    /**
     * Returns the return value that was set last.
     */
    template <typename T>
    T returnValue() const { return getConfig<T>("__returnvalue"); }
protected:
    /**
     * Call this method when a long wait period has started or ended. If the wait
     * isn't nulled before the timeout is reached the current action will be aborted
     * and the socket will be disconnected.
     *
     * @param start True if the wait period should start, false if it should end
     */
    void timeoutWait(bool start);
    
    /**
     * Reset the timeout counter. Call this once in a while during long wait periods
     * to notify the engine that the socket is still responsive.
     */
    void timeoutPing();
    
    /**
     * Check if we should timeout. This method might cause a disconnect if the timeout
     * value is reached.
     */
    void timeoutCheck();
    
    /**
     * Enable the issue of keepalive packets.
     */
    void keepaliveStart();
    
    /**
     * Check if we should transmit a new keepalive packet.
     */
    void keepaliveCheck();
protected:
    KRemoteEncoding *m_remoteEncoding;
    
    Commands::Base *m_cmdData;
    QStack<Commands::Base*> m_commandChain;
    
    Settings *m_settings;
    Thread *m_thread;
    DirectoryListing m_lastDirectoryListing;
    DirectoryEntry m_lastStatResponse;
    
    filesize_t m_transferBytes;
    time_t m_speedLastTime;
    filesize_t m_speedLastBytes;
    
    QTime m_timeoutCounter;
    QTime m_keepaliveCounter;
private:
    QString m_currentDirectory;
    QString m_defaultDirectory;
    KUrl m_currentUrl;
    QString m_protocol;
    Commands::Type m_currentCommand;
    bool m_errorReporting;
    QPointer<ConnectionRetry> m_connectionRetry;
};

}

Q_DECLARE_METATYPE(KFTPEngine::Socket*)

#endif
