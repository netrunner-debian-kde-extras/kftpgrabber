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
#include "manager.h"
#include "entry.h"

#include <QFile>

#include <KStandardDirs>
#include <KGlobal>
#include <KMenu>
#include <KMessageBox>
#include <KLocale>

namespace KFTPCore {

namespace CustomCommands {

class ManagerPrivate {
public:
    Manager instance;
};

K_GLOBAL_STATIC(ManagerPrivate, managerPrivate)

Manager *Manager::self()
{
  return &managerPrivate->instance;
}

Manager::Manager()
  : QObject()
{
  // Populate the handlers list
  m_handlers["Raw"] = new Handlers::RawHandler();
  m_handlers["Substitute"] = new Handlers::SubstituteHandler();
  m_handlers["Regexp"] = new Handlers::RegexpHandler();
}

Manager::~Manager()
{
  // Destroy the handlers
  delete static_cast<Handlers::RawHandler*>(m_handlers["Raw"]);
  delete static_cast<Handlers::SubstituteHandler*>(m_handlers["Substitute"]);
  delete static_cast<Handlers::RegexpHandler*>(m_handlers["Regexp"]);
  
  m_handlers.clear();
}

void Manager::load()
{
  QString filename = KStandardDirs::locateLocal("appdata", "commands.xml");
  
  if (!QFile::exists(filename)) {
    // Copy the default command set over
    QFile source(KStandardDirs::locate("appdata", "commands.xml"));
    QFile destination(filename);
    
    source.open(QIODevice::ReadOnly);
    destination.open(QIODevice::WriteOnly | QIODevice::Truncate);
    
    destination.write(source.readAll());
    source.close();
    destination.close();
  }
  
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly))
    return;
  
  m_document.setContent(&file);
  file.close();
}

void Manager::parseEntries(KActionMenu *parentMenu, const QDomNode &parentNode, KFTPSession::Session *session) const
{
  QDomNode n = parentNode.firstChild();
  
  while (!n.isNull()) {
    if (n.isElement()) {
      QDomElement e = n.toElement();
      QString tagName = e.tagName();
      QString name = e.attribute("name");
      
      if (tagName == "category") {
        KActionMenu *menu = new KActionMenu(KIcon("folder"), name, parentMenu);
        parentMenu->addAction(menu);
        
        // Recurse into this category
        parseEntries(menu, n, session);
      } else if (tagName == "entry") {
        Entry *entry = new Entry((Manager*) this, name);
        entry->setDescription(n.namedItem("description").toElement().text());
        entry->setIcon(e.attribute("icon"));
        entry->setCommand(n.namedItem("command").toElement().text());
        
        QDomNode p = n.namedItem("params").firstChild();
        while (!p.isNull()) {
          QDomElement pElement = p.toElement();
          
          if (pElement.tagName() == "param") {
            QString typeString = pElement.attribute("type");
            Entry::ParameterType type = Entry::String;
            
            if (typeString == "String")
              type = Entry::String;
            else if (typeString == "Password")
              type = Entry::Password;
            else if (typeString == "Integer")
              type = Entry::Integer;
            
            entry->appendParameter(type, pElement.text());
          }
          
          p = p.nextSibling();
        }
        
        QDomElement rElement = n.namedItem("response").toElement();
        entry->setResponseHandler(rElement.attribute("handler"), rElement);
        
        QString displayString = rElement.attribute("display");
        Entry::DisplayType displayType = Entry::None;
        
        if (displayString == "None")
          displayType = Entry::None;
        else if (displayString == "Window")
          displayType = Entry::Window;
        else if (displayString == "MessageBox")
          displayType = Entry::MessageBox;
          
        entry->setDisplayType(displayType);
        
        // Create a new action
        EntryAction *action = new EntryAction(entry, session);
        connect(action, SIGNAL(activated()), this, SLOT(slotActionActivated()));
        
        parentMenu->addAction(action);
      } else if (tagName == "separator") {
        parentMenu->menu()->addSeparator();
      } else {
        KMessageBox::error(0, i18n("Unknown tag while parsing custom site commands."));
      }
    }
    
    n = n.nextSibling();
  }
}

Handlers::Handler *Manager::handler(const QString &name) const
{
  if (m_handlers.contains(name))
    return m_handlers[name];
  
  return 0;
}

KActionMenu *Manager::categories(const QString &name, KFTPSession::Session *session) const
{
  KActionMenu *actionMenu = new KActionMenu(name, 0);
  parseEntries(actionMenu, m_document.documentElement(), session);
  
  return actionMenu;
}

void Manager::slotActionActivated()
{
  EntryAction *action = (EntryAction*) QObject::sender();
  action->entryInfo()->execute(action->session());
}

}

}

#include "manager.moc"

