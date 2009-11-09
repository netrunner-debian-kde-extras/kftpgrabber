/*
 * This file is part of the KFTPGrabber project
 *
 * Copyright (C) 2003-2004 by the KFTPGrabber developers
 * Copyright (C) 2003-2004 Jernej Kos <kostko@jweb-network.net>
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
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#include "importwizard.h"
#include "kftpapi.h"
#include "kftppluginmanager.h"
#include "interfaces/kftpbookmarkimportplugin.h"
#include "kftpbookmarks.h"

#include <qfileinfo.h>
#include <qlabel.h>
//Added by qt3to4:
#include <QPixmap>

#include <kurlrequester.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kprogress.h>
#include <kstandarddirs.h>

using namespace KFTPGrabberBase;

namespace KFTPWidgets {

namespace Bookmarks {

PluginListItem::PluginListItem(K3ListView *parent, KService::Ptr service)
  : K3ListViewItem(parent, service->name(), service->comment()), m_service(service)
{
  setPixmap(0, loadSmallPixmap("filter"));
}

ImportWizard::ImportWizard(QWidget *parent, const char *name)
 : KFTPBookmarkImportLayout(parent, name)
{
  m_pluginList->setFullWidth(true);
  m_pluginList->setAllColumnsShowFocus(true);

  connect(m_pluginList, SIGNAL(clicked(Q3ListViewItem*)), this, SLOT(slotPluginsSelectionChanged(Q3ListViewItem*)));

  setNextEnabled(Step1, false);

  // Set pixmap
  QString pixmapPath = locate("appdata", "kftpgrabber-bi-wizard.png");
  if (!pixmapPath.isNull()) {
    QPixmap pix(pixmapPath);

    m_wizardPixmap->setPixmap(pix);
    m_wizardPixmap_2->setPixmap(pix);
    m_wizardPixmap_3->setPixmap(pix);
  }

  // Disable useless help buttons
  setHelpEnabled(Step1, false);
  setHelpEnabled(Step2, false);
  setHelpEnabled(Step3, false);

  displayPluginList();
}

void ImportWizard::next()
{
  if (currentPage() == Step1) {
    // Load the plugin
    m_plugin = KFTPAPI::getInstance()->pluginManager()->loadImportPlugin(m_service);

    if (!m_plugin) {
      KMessageBox::error(0, i18n("Unable to load the selected import plugin."));
      return;
    } else {
      // Get the default plugin path
      m_importUrl->setURL("~/" + m_plugin->getDefaultPath());
    }
  } else if (currentPage() == Step2) {
    // Check if the file exists
    if (!QFileInfo(m_importUrl->url()).exists() || !QFileInfo(m_importUrl->url()).isReadable()) {
      KMessageBox::error(0, i18n("The selected file does not exist or is not readable."));
      return;
    }
  }

  Q3Wizard::next();

  if (currentPage() == Step3) {
    // Start the import
    setBackEnabled(Step3, false);

    connect(m_plugin, SIGNAL(progress(int)), this, SLOT(slotImportProgress(int)));
    m_plugin->import(m_importUrl->url());
  }
}

void ImportWizard::slotImportProgress(int progress)
{
  m_progressBar->setProgress(progress);

  if (progress == 100) {
    // Import complete
    KMessageBox::information(0, i18n("Bookmark importing is complete."));

    // Add the imported stuff to the current bookmarks
    KFTPBookmarks::Manager::self()->importSites(m_plugin->getImportedXml().documentElement());

    accept();
  }
}

void ImportWizard::displayPluginList()
{
  KTrader::OfferList plugins = KFTPAPI::getInstance()->pluginManager()->getImportPlugins();

  KTrader::OfferList::ConstIterator end(plugins.end());
  for (KTrader::OfferList::ConstIterator i(plugins.begin()); i != end; ++i) {
    KService::Ptr service = *i;

    new PluginListItem(m_pluginList, service);
  }
}

void ImportWizard::slotPluginsSelectionChanged(Q3ListViewItem *i)
{
  if (i) {
    PluginListItem *item = static_cast<PluginListItem*>(i);
    m_service = item->m_service;

    setNextEnabled(Step1, true);
  } else {
    setNextEnabled(Step1, false);
  }
}

}

}

#include "importwizard.moc"
