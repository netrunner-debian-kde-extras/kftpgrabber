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
#include "entry.h"
#include "kftpsession.h"
#include "manager.h"

#include "parameterentrydialog.h"
#include "responsedialog.h"

#include <KLocale>
#include <KMessageBox>

namespace KFTPCore {

namespace CustomCommands {

Entry::Entry(QObject *parent, const QString &name)
  : QObject(parent),
    m_name(name)
{
}

void Entry::appendParameter(ParameterType type, const QString &name)
{
  m_params.append(Parameter(type, name));
}

void Entry::setResponseHandler(const QString &handler, QDomNode args)
{
  m_handler = handler;
  m_handlerArguments = args;
}

void Entry::execute(KFTPSession::Session *session)
{
  // Create a dialog for parameter input
  QString command = m_command;
  
  if (m_params.count() > 0) {
    ParameterEntryDialog *dialog = new ParameterEntryDialog(this, m_params);
    if (dialog->exec() != QDialog::Accepted) {
      delete dialog;
      return;
    }
    
    command = dialog->formatCommand(command);
    delete dialog;
  }
  
  // Execute the command with proper parameters
  m_lastSession = session;
  
  connect(session->getClient()->eventHandler(), SIGNAL(gotRawResponse(const QString&)), this, SLOT(handleResponse(const QString&)));
  session->getClient()->raw(command);
}

void Entry::handleResponse(const QString &response)
{
  if (!m_lastSession)
    return;
  
  m_lastSession->getClient()->eventHandler()->QObject::disconnect(this);
  m_lastSession = 0;
  
  // Invoke the proper handler
  QString expectedReturn = m_handlerArguments.namedItem("expected").toElement().attribute("code");
  
  if (!response.startsWith(expectedReturn)) {
    KMessageBox::error(0, i18n("<qt>Requested operation has failed - response from server was:<br/><br /><b>%1</b></qt>",response));
    return;
  }
  
  Handlers::Handler *handler = Manager::self()->handler(m_handler);
  
  if (!handler) {
    KMessageBox::error(0, i18n("<qt>The handler named <b>%1</b> cannot be found for response parsing.</qt>",m_handler));
    return;
  }
  
  QString parsed = handler->handleResponse(response, m_handlerArguments);
  
  // Find the proper way to display the parsed response
  switch (m_displayType) {
    case None: return;
    case Window: {
      ResponseDialog *dialog = new ResponseDialog(m_name, parsed);
      dialog->exec();
      delete dialog;
      break;
    }
    case MessageBox: {
      KMessageBox::information(0, parsed);
      break;
    }
  }
}

Entry::Parameter::Parameter()
  : m_type(String),
    m_name("<invalid>")
{
}

Entry::Parameter::Parameter(ParameterType type, const QString &name)
  : m_type(type),
    m_name(name)
{
}

EntryAction::EntryAction(Entry *entry, KFTPSession::Session *session)
  : KAction(KIcon(entry->icon()), entry->name(), 0),
    m_entryInfo(entry),
    m_session(session)
{
}

}

}

#include "entry.moc"

