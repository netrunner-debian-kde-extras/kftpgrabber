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
#ifndef KFTPCORE_CUSTOMCOMMANDSENTRY_H
#define KFTPCORE_CUSTOMCOMMANDSENTRY_H

#include <QDomNode>
#include <QList>

#include <KAction>

namespace KFTPSession {
  class Session;
}

namespace KFTPCore {

namespace CustomCommands {

/**
 * This class represents a single custom command entry. A tree of
 * such objects is constructed from an XML file.
 *
 * @author Jernej Kos
 */
class Entry : public QObject {
Q_OBJECT
public:
    /**
     * Possible parameter types.
     */
    enum ParameterType {
      String,
      Password,
      Integer
    };
    
    /**
     * Possible display types.
     */
    enum DisplayType {
      None,
      Window,
      MessageBox
    };
    
    /**
     * A single command parameter.
     */
    class Parameter {
    public:
        /**
         * Class constructor.
         */
        Parameter();
        
        /**
         * Class constructor.
         *
         * @param type Parameter type
         * @param name Parameter name
         */
        Parameter(ParameterType type, const QString &name);
        
        /**
         * Returns the parameter type.
         */
        ParameterType type() const { return m_type; }
        
        /**
         * Returns the parameter name.
         */
        QString name() const { return m_name; }
    private:
        ParameterType m_type;
        QString m_name;
    };
    
    /**
     * Class constructor.
     *
     * @param name Short entry name
     */
    Entry(QObject *parent, const QString &name);
    
    /**
     * Returns entry's name.
     */
    QString name() const { return m_name; }
    
    /**
     * Returns entry's description.
     */
    QString description() const { return m_description; }
    
    /**
     * Returns entry's icon name.
     */
    QString icon() const { return m_icon; }
    
    /**
     * Sets entry's description.
     *
     * @param description A longer entry description; can be rich text
     */
    void setDescription(const QString &description) { m_description = description; }
    
    /**
     * Set entry's icon.
     *
     * @param icon An icon name
     */
    void setIcon(const QString &icon) { m_icon = icon; }
    
    /**
     * Sets the raw command to be sent.
     *
     * @param command A valid FTP command with optional parameter placeholders
     */
    void setCommand(const QString &command) { m_command = command; }
    
    /**
     * Appends a command parameter.
     *
     * @param type Parameter type
     * @param name Human readable parameter name
     */
    void appendParameter(ParameterType type, const QString &name);
    
    /**
     * Sets response display type.
     *
     * @param type Display type
     */
    void setDisplayType(DisplayType type) { m_displayType = type; }
    
    /**
     * Sets the response handler to use.
     *
     * @param handler Handler name
     * @param args Optional argument node
     */
    void setResponseHandler(const QString &handler, QDomNode args);
    
    /**
     * Executes this entry. This will actually generate and show a proper
     * user input dialog, execute the command with the provided parameters,
     * pass the raw response to a selected handler and properly display
     * the result.
     *
     * @param session A remote session where command should be executed
     */
    void execute(KFTPSession::Session *session);
private slots:
    void handleResponse(const QString &response);
private:
    QString m_name;
    QString m_description;
    QString m_icon;
    QString m_command;
    QString m_handler;
    DisplayType m_displayType;
    
    QList<Parameter> m_params;
    QDomNode m_handlerArguments;
    
    KFTPSession::Session *m_lastSession;
};

/**
 * This class is a wrapper action, so a proper entry gets pulled and
 * executed.
 *
 * @author Jernej Kos
 */
class EntryAction : public KAction {
public:
    /**
     * Class constructor.
     *
     * @param entry Associated entry
     * @param session Associated session
     */
    EntryAction(Entry *entry, KFTPSession::Session *session);
    
    /**
     * Returns the associated entry instance.
     */
    Entry *entryInfo() const { return m_entryInfo; }
    
    /**
     * Returns the associated session instance.
     */
    KFTPSession::Session *session() const { return m_session; }
private:
    Entry *m_entryInfo;
    KFTPSession::Session *m_session;
};

}

}

#endif
