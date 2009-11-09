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

#include "configfilter.h"
#include "filtereditor.h"

#include "misc/config.h"

#include <klocale.h>
#include <kpushbutton.h>
#include <kmessagebox.h>

#include <klineedit.h>
#include <kcolorbutton.h>

#include <qcheckbox.h>
#include <qlayout.h>
#include <qtabwidget.h>

namespace KFTPWidgets {

ConfigFilter::ConfigFilter(QWidget *parent)
 : QWidget(parent)
{
  // Create the main widget
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  
  QWidget *widget = new QWidget(this);
  ui.setupUi(widget);
  mainLayout->addWidget(widget);
  
  m_filterEditor = new FilterEditor(this);
  ui.tabWidget->insertTab(0, m_filterEditor, i18n("Filters"));
  ui.tabWidget->setCurrentIndex(0);

  loadSettings();

  // Connect the slots
  connect(ui.addExtButton, SIGNAL(clicked()), this, SLOT(slotAddAscii()));
  connect(ui.removeExtButton, SIGNAL(clicked()), this, SLOT(slotRemoveAscii()));
}

void ConfigFilter::loadSettings()
{
  //m_filterEditor->reset();
  asciiLoadExtensions();
}

void ConfigFilter::saveSettings()
{
  // Save the settings
  KFTPCore::Config::setAsciiList(asciiToStringList());
}

void ConfigFilter::slotAddAscii()
{
  if (!ui.newExtension->text().trimmed().isEmpty()) {
    ui.extensionList->addItem(ui.newExtension->text().trimmed());
    ui.newExtension->clear();
  }
}

void ConfigFilter::slotRemoveAscii()
{
  delete ui.extensionList->currentItem();
}

void ConfigFilter::asciiLoadExtensions()
{
  // Load the ascii extensions
  ui.extensionList->clear();
  ui.extensionList->addItems(KFTPCore::Config::asciiList());
}

QStringList ConfigFilter::asciiToStringList()
{
  QStringList extensions;
  
  for (int i = 0; i < ui.extensionList->count(); i++) {
    extensions << ui.extensionList->item(i)->text();
  }

  return extensions;
}

}
#include "configfilter.moc"
