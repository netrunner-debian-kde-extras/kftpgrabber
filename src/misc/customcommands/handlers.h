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
#ifndef KFTPCORE_CUSTOMCOMMANDS_HANDLERSHANDLERS_H
#define KFTPCORE_CUSTOMCOMMANDS_HANDLERSHANDLERS_H

#include <qstring.h>
#include <qdom.h>

namespace KFTPCore {

namespace CustomCommands {

namespace Handlers {

/**
 * The handler class is an abstract class which every actual handler
 * must implement.
 *
 * @author Jernej Kos
 */
class Handler {
public:
    /**
     * Class constructor.
     *
     * @param name Handler name
     */
    Handler(const QString &name);
    
    /**
     * Class destructor.
     */
    virtual ~Handler() {}
    
    /**
     * Returns the handler's name.
     */
    QString name() const { return m_name; }
    
    /**
     * This method should be implemented by actual handlers to handler the
     * server response.
     *
     * @param raw Raw FTP response
     * @param arguments Any argument nodes supplied in the XML file
     * @return This method should return a formatted string
     */
    virtual QString handleResponse(const QString &raw, QDomNode arguments) const = 0;
private:
    QString m_name;
};

/**
 * The Raw handler accepts no arguments and simply passes on raw data.
 *
 * @author Jernej Kos
 */
class RawHandler : public Handler {
public:
    /**
     * Class constructor.
     */
    RawHandler();
    
    /**
     * @overload
     * Reimplemented from Handler.
     */
    QString handleResponse(const QString &raw, QDomNode) const { return raw; }
};

/**
 * The Substitue handler always returns a predefined value when the
 * operation is completed successfully. %1 can be used in place of the
 * raw data received from the server.
 *
 * @author Jernej Kos
 */
class SubstituteHandler : public Handler {
public:
    /**
     * Class constructor.
     */
    SubstituteHandler();
    
    /**
     * @overload
     * Reimplemented from Handler.
     */
    QString handleResponse(const QString &raw, QDomNode arguments) const;
};

/**
 * The Regexp handler enables custom response parsing using regular
 * expressions.
 *
 * @author Jernej Kos
 */
class RegexpHandler : public Handler {
public:
    /**
     * Class constructor.
     */
    RegexpHandler();
    
    /**
     * @overload
     * Reimplemented from Handler.
     */
    QString handleResponse(const QString &raw, QDomNode arguments) const;
};

}

}

}

#endif
