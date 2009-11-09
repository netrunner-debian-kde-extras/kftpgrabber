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

#include "configdialog.h"
#include "misc/config.h"
#include "widgets/systemtray.h"

#include <klocale.h>
#include <kfontdialog.h>
#include <kcolorbutton.h>
#include <kurlrequester.h>
#include <klineedit.h>
#include <knuminput.h>
#include <kglobal.h>
#include <kcharsets.h>
#include <kcombobox.h>

#include <qframe.h>
#include <qlayout.h>
#include <qcheckbox.h>

// Config layouts
#include "ui/ui_config_general.h"
#include "ui/ui_config_transfers.h"
#include "ui/ui_config_log.h"
#include "ui/ui_config_display.h"

#include "configfilter.h"

namespace KFTPWidgets {

template<class T>
KPageWidgetItem *_addPage(KConfigDialog *dialog, const QString &itemName, const QString &pixmapName = QString())
{
  T ui;
  QWidget *page = new QWidget();
  ui.setupUi(page);
  return dialog->addPage(page, itemName, pixmapName);
}

ConfigDialog::ConfigDialog(QWidget *parent, const QString &name)
  : KConfigDialog(parent, name, KFTPCore::Config::self())
{
  // Add all standard pages
  _addPage<Ui::GeneralSettings>(this, "General", "preferences-other");
  _addPage<Ui::TransferSettings>(this, "Transfers", "network-workgroup");
  _addPage<Ui::LogSettings>(this, "Log", "utilities-log-viewer");
  _addPage<Ui::DisplaySettings>(this, "Display", "preferences-desktop-theme");

  // Add  the actions page
  QFrame *aFrame = new QFrame();
  QVBoxLayout *actionsLayout = new QVBoxLayout(aFrame);
  actionsLayout->setContentsMargins(0, 0, 0, 0);
  actionsLayout->addWidget(KFTPCore::Config::self()->dActions()->getConfigWidget(aFrame));
  actionsLayout->addSpacing(KDialog::spacingHint());
  actionsLayout->addWidget(KFTPCore::Config::self()->uActions()->getConfigWidget(aFrame));
  actionsLayout->addSpacing(KDialog::spacingHint());
  actionsLayout->addWidget(KFTPCore::Config::self()->fActions()->getConfigWidget(aFrame));
  actionsLayout->addStretch(1);
  addPage(aFrame, i18n("Actions"), "system-run");
  
  // Add the filter page
  m_configFilter = new ConfigFilter(this);
  addPage(m_configFilter, i18n("Filters"), "view-filter");
  
  // Setup some stuff
  findChild<KUrlRequester*>("kcfg_defLocalDir")->setMode(KFile::Directory | KFile::ExistingOnly | KFile::LocalOnly);
  
  // Let the config be up-to-date
  connect(this, SIGNAL(settingsChanged(const QString&)), KFTPCore::Config::self(), SLOT(emitChange()));
  connect(this, SIGNAL(okClicked()), this, SLOT(slotSettingsChanged()));
}

void ConfigDialog::prepareDialog()
{
  // Update the actions
  KFTPCore::Config::self()->dActions()->updateWidget();
  KFTPCore::Config::self()->uActions()->updateWidget();
  KFTPCore::Config::self()->fActions()->updateWidget();
  
  // Populate charsets
  KComboBox *encoding = findChild<KComboBox*>("cfg_defEncoding");
  
  foreach (QString description, KGlobal::charsets()->descriptiveEncodingNames()) {
    encoding->addItem(description, KGlobal::charsets()->encodingForName(description));
  }
  
  encoding->setCurrentIndex(encoding->findData(KFTPCore::Config::defEncoding()));
  m_configFilter->loadSettings();
}

void ConfigDialog::slotSettingsChanged()
{
  // Update the actions
  KFTPCore::Config::self()->dActions()->updateConfig();
  KFTPCore::Config::self()->uActions()->updateConfig();
  KFTPCore::Config::self()->fActions()->updateConfig();
  
  m_configFilter->saveSettings();
  
  // Save encoding
  KComboBox *encoding = findChild<KComboBox*>("cfg_defEncoding");
  KFTPCore::Config::setDefEncoding(encoding->itemData(encoding->currentIndex()).toString());
  
  // Show/hide the systray icon
  if (KFTPCore::Config::showSystrayIcon())
    SystemTray::self()->show();
  else
    SystemTray::self()->hide();
}

}

#include "configdialog.moc"
