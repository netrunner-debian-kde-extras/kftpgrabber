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

#ifndef COMMANDS_H
#define COMMANDS_H

#include "event.h"

#define ENGINE_STANDARD_COMMAND_CONSTRUCTOR(class, type, cmd) public: \
                                                              class(type *socket) : Commands::Base(socket, Commands::cmd), currentState(None) {} \
                                                              private: \
                                                              State currentState; \
                                                              \
                                                              type *socket() {\
                                                                return static_cast<type*>(m_socket);\
                                                              }\
                                                              public: 

#define ENGINE_CANCELLATION_POINT { if (isDestructable()) \
                                      return; }

#define setupCommandClass(class) if (m_cmdData) \
                                   delete m_cmdData; \
                                 m_cmdData = new class(this);

#define chainCommandClass(class) Commands::Base *_cmd = new class(socket()); \
                                 socket()->addToCommandChain(_cmd); \
                                 socket()->nextCommand(); \
                                 return;

#define activateCommandClass(class) if (m_cmdData) { \
                                      Commands::Base *_cmd = new class(this); \
                                      addToCommandChain(_cmd); \
                                      nextCommand(); \
                                    } else { \
                                      m_cmdData = new class(this); \
                                      m_cmdData->process(); \
                                    }

namespace KFTPEngine {

class Socket;

namespace Commands {

enum Type {
  CmdNone,
  CmdWakeup,
  
  // Actual commands
  CmdConnect,
  CmdConnectRetry,
  CmdDisconnect,
  CmdList,
  CmdScan,
  CmdGet,
  CmdPut,
  CmdDelete,
  CmdRename,
  CmdMkdir,
  CmdChmod,
  CmdRaw,
  CmdFxp,
  CmdKeepAlive,
  CmdAbort
};

class Base {
public:
    Base(Socket *socket, Type type);
    virtual ~Base() {}

    void setProcessing(bool value);
    bool isProcessing() { return m_processing > 0; }
    
    void autoDestruct(ResetCode code);
    bool isDestructable() { return m_autoDestruct && !isProcessing(); }
    ResetCode resetCode() { return m_resetCode; }
    
    bool isClean() { return m_clean; }
    
    Type command() { return m_command; }
    
    bool isWakeup() { return m_wakeupEvent != 0; }
    virtual void wakeup(WakeupEvent *event);
    virtual void process() = 0;
    virtual void cleanup() {}
protected:
    void markClean() { m_clean = true; }
protected:
    Type m_command;
    Socket *m_socket;
    WakeupEvent *m_wakeupEvent;
    
    int m_processing;
    bool m_autoDestruct;
    ResetCode m_resetCode;
    bool m_clean;
};

}

}

#endif
