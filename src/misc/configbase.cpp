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
 
#include "configbase.h"
#include "config.h"
#include "filter.h"

#include <qregexp.h>

#include <kapplication.h>
#include <kconfig.h>
#include <kemailsettings.h>
#include <klocale.h>

namespace KFTPCore {

ConfigBase::ConfigBase(const QString &fileName)
  : KConfigSkeleton(fileName)
{
  m_fileExistsDownActions.setTypeText(i18n("Download"));
  m_fileExistsUpActions.setTypeText(i18n("Upload"));
  m_fileExistsFxpActions.setTypeText(i18n("FXP"));
  
  m_transMode = 'I';
}

void ConfigBase::postInit()
{
  // Restore the actions
  QString tmp = Config::downloadActions();
  tmp >> m_fileExistsDownActions;
  
  tmp = Config::uploadActions();
  tmp >> m_fileExistsUpActions;
  
  tmp = Config::fxpActions();
  tmp >> m_fileExistsFxpActions;
}

void ConfigBase::saveConfig()
{
  // Save actions before writing
  QString tmp;
  tmp << m_fileExistsDownActions;
  Config::setDownloadActions(tmp);
  
  tmp << m_fileExistsUpActions;
  Config::setUploadActions(tmp);
  
  tmp << m_fileExistsFxpActions;
  Config::setFxpActions(tmp);
  
  // Save filters
  Filter::Filters::self()->save();
  
  // Write the config
  writeConfig();
}

void ConfigBase::emitChange()
{
  emit configChanged();
}

char ConfigBase::ftpMode(const QString &filename)
{
  // Get FTP mode (binary/ascii)
  switch (m_transMode) {
    case 'A': return 'A'; break;
    case 'I': return 'I'; break;
    case 'X':
    default: {
      char mode = 'I';
      QRegExp e;
      e.setPatternSyntax(QRegExp::Wildcard);
      
      QStringList list = Config::asciiList();
      QStringList::iterator end(list.end());
      for (QStringList::iterator i(list.begin()); i != end; ++i) {
        e.setPattern((*i));
        
        if (e.exactMatch(filename)) {
          mode = 'A';
          break;
        }
      }
    
      return mode;
    }
  }
}

QString ConfigBase::getGlobalMail()
{
  KEMailSettings kes;
  kes.setProfile(kes.defaultProfileName());
  return kes.getSetting(KEMailSettings::EmailAddress);
}

}

#include "configbase.moc"
