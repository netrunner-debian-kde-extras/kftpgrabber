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

#include "queueeditor.h"
//Added by qt3to4:
#include <Q3PtrList>
#include "kftpserverlineedit.h"
#include "kftpbookmarks.h"
#include "kftpqueueeditorlayout.h"

#include <klineedit.h>
#include <kpassworddialog.h>
#include <kcombobox.h>
#include <klocale.h>

#include <qspinbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qtabwidget.h>

#define REMOTE_PROTOCOL m_transfer->getSourceUrl().isLocalFile() ? m_transfer->getDestUrl().protocol() : m_transfer->getSourceUrl().protocol()

namespace KFTPWidgets {

QueueEditor::QueueEditor(QWidget *parent, const char *name)
: KDialogBase(parent, name, true, "Edit queue", KDialogBase::Ok | KDialogBase::Cancel,
              KDialogBase::Ok, true)
{
  m_layout = new KFTPQueueEditorLayout(this);
  setMainWidget(m_layout);

  connect(m_layout->srcPath, SIGNAL(textChanged(const QString&)), this, SLOT(slotTextChanged()));
  connect(m_layout->dstPath, SIGNAL(textChanged(const QString&)), this, SLOT(slotTextChanged()));
  
  connect(m_layout->srcHost, SIGNAL(textChanged(const QString&)), this, SLOT(slotTextChanged()));
  connect(m_layout->srcUser, SIGNAL(textChanged(const QString&)), this, SLOT(slotTextChanged()));
  connect(m_layout->srcPass, SIGNAL(textChanged(const QString&)), this, SLOT(slotTextChanged()));
  
  connect(m_layout->dstHost, SIGNAL(textChanged(const QString&)), this, SLOT(slotTextChanged()));
  connect(m_layout->dstUser, SIGNAL(textChanged(const QString&)), this, SLOT(slotTextChanged()));
  connect(m_layout->dstPass, SIGNAL(textChanged(const QString&)), this, SLOT(slotTextChanged()));

  connect(m_layout->srcName, SIGNAL(siteChanged(KFTPBookmarks::Site*)), this, SLOT(slotSourceSiteChanged(KFTPBookmarks::Site*)));
  connect(m_layout->dstName, SIGNAL(siteChanged(KFTPBookmarks::Site*)), this, SLOT(slotDestSiteChanged(KFTPBookmarks::Site*)));

  connect(m_layout->transferType, SIGNAL(activated(int)), this, SLOT(slotTransferModeChanged(int)));

  setMaximumHeight(250);
  setInitialSize(QSize(500, 250));

  enableButtonOk(false);
}

void QueueEditor::resetTabs()
{
  m_layout->serverTab->setTabEnabled(m_layout->tab, false);
  m_layout->serverTab->setTabEnabled(m_layout->tab_2, false);
}

void QueueEditor::resetServerData()
{
  // Source
  m_layout->srcName->clear();
  m_layout->srcHost->setText("");
  m_layout->srcPort->setValue(21);
  m_layout->srcUser->setText("");
  m_layout->srcPass->erase();

  // Destination
  m_layout->dstName->clear();
  m_layout->dstHost->setText("");
  m_layout->dstPort->setValue(21);
  m_layout->dstUser->setText("");
  m_layout->dstPass->erase();
}

void QueueEditor::slotTransferModeChanged(int index)
{
  if (m_lastTransferType == index)
    return;
  else
    m_lastTransferType = (KFTPQueue::TransferType) index;

  resetTabs();
  resetServerData();
  
  switch (index) {
    case 0: {
      // Download - source: remote dest: local
      m_layout->serverTab->setTabEnabled(m_layout->tab, true);
      m_layout->serverTab->showPage(m_layout->tab);
      break;
    }
    case 1: {
      // Upload - source: local dest: remote
      m_layout->serverTab->setTabEnabled(m_layout->tab_2, true);
      m_layout->serverTab->showPage(m_layout->tab_2);
      break;
    }
    case 2: {
      // FXP - source: remote dest: remote
      m_layout->serverTab->setTabEnabled(m_layout->tab, true);
      m_layout->serverTab->setTabEnabled(m_layout->tab_2, true);
      m_layout->serverTab->showPage(m_layout->tab);
      break;
    }
  }

  slotTextChanged();
}

bool QueueEditor::sourceIsValid()
{
  if (m_lastTransferType == 1) return true;

  if (m_layout->srcHost->text().trimmed().isEmpty() || m_layout->srcUser->text().trimmed().isEmpty())
    return false;
  else
    return true;
}

bool QueueEditor::destIsValid()
{
  if (m_lastTransferType == 0) return true;

  if (m_layout->dstHost->text().trimmed().isEmpty() || m_layout->dstUser->text().trimmed().isEmpty())
    return false;
  else
    return true;
}

void QueueEditor::slotTextChanged()
{
  if (m_layout->srcPath->text().trimmed().isEmpty() || m_layout->dstPath->text().trimmed().isEmpty() ||
      m_layout->srcPath->text().left(1) != "/" || m_layout->dstPath->text().left(1) != "/" ||
      !sourceIsValid() || !destIsValid() )
    enableButtonOk(false);
  else
    enableButtonOk(true);
}

void QueueEditor::setData(KFTPQueue::Transfer *transfer)
{
  KUrl sUrl, dUrl;
  
  m_layout->srcPath->setText(transfer->getSourceUrl().path());
  m_layout->dstPath->setText(transfer->getDestUrl().path());

  // Source
  sUrl = transfer->getSourceUrl();
  
  if (!sUrl.isLocalFile()) {
    m_layout->srcName->setCurrentSite(KFTPBookmarks::Manager::self()->findSite(sUrl));
    m_layout->srcHost->setText(sUrl.host());
    m_layout->srcPort->setValue(sUrl.port());
    m_layout->srcUser->setText(sUrl.user());
    
    m_layout->srcPass->erase();
    m_layout->srcPass->insert(sUrl.pass());
  } else {
    m_layout->serverTab->setTabEnabled(m_layout->tab, false);
  }

  // Destination
  dUrl = transfer->getDestUrl();
  
  if (!dUrl.isLocalFile()) {
    m_layout->dstName->setCurrentSite(KFTPBookmarks::Manager::self()->findSite(dUrl));
    m_layout->dstHost->setText(dUrl.host());
    m_layout->dstPort->setValue(dUrl.port());
    m_layout->dstUser->setText(dUrl.user());
    
    m_layout->dstPass->erase();
    m_layout->dstPass->insert(dUrl.pass());
  } else {
    m_layout->serverTab->setTabEnabled(m_layout->tab_2, false);
  }

  // Transfer type
  m_lastTransferType = transfer->getTransferType();
  m_layout->transferType->setCurrentItem(m_lastTransferType);

  m_transfer = transfer;
}

void QueueEditor::saveData()
{
  KUrl sUrl, dUrl;

  if (m_lastTransferType != 1) {
    sUrl.setProtocol(REMOTE_PROTOCOL);
    sUrl.setHost(m_layout->srcHost->text());
    sUrl.setPort(m_layout->srcPort->value());
    sUrl.setUser(m_layout->srcUser->text());
    sUrl.setPass(m_layout->srcPass->password());
    
    if (m_transfer->getSourceUrl().pass().isEmpty() && sUrl.pass().isEmpty())
      sUrl.setPass(QString::null);
  } else {
    sUrl.setProtocol("file");
  }
  
  sUrl.setPath(m_layout->srcPath->text());

  if (m_lastTransferType != 0) {
    dUrl.setProtocol(REMOTE_PROTOCOL);
    dUrl.setHost(m_layout->dstHost->text());
    dUrl.setPort(m_layout->dstPort->value());
    dUrl.setUser(m_layout->dstUser->text());
    dUrl.setPass(m_layout->dstPass->password());
    
    if (m_transfer->getDestUrl().pass().isEmpty() && dUrl.pass().isEmpty())
      dUrl.setPass(QString::null);
  } else {
    dUrl.setProtocol("file");
  }
  
  dUrl.setPath(m_layout->dstPath->text());

  m_transfer->setSourceUrl(sUrl);
  m_transfer->setDestUrl(dUrl);
  m_transfer->setTransferType(m_lastTransferType);
  
  // If the transfer is a directory, we have to update all child transfers
  // as well.
  if (m_transfer->isDir())
    recursiveSaveData(static_cast<KFTPQueue::TransferDir*>(m_transfer), sUrl, dUrl);
}

void QueueEditor::recursiveSaveData(KFTPQueue::TransferDir *parent, const KUrl &srcUrl, const KUrl &dstUrl)
{
  KFTPQueue::QueueObject *o;
  Q3PtrList<KFTPQueue::QueueObject> children = parent->getChildrenList();
  
  KUrl sUrl, dUrl;
  
  for (o = children.first(); o; o = children.next()) {
    KFTPQueue::Transfer *i = static_cast<KFTPQueue::Transfer*>(o);
    
    // Modify the urls
    sUrl = srcUrl;
    dUrl = dstUrl;
    
    sUrl.addPath(i->getSourceUrl().fileName());
    dUrl.addPath(i->getDestUrl().fileName());
    
    // Set the urls
    i->setSourceUrl(sUrl);
    i->setDestUrl(dUrl);
    i->setTransferType(m_lastTransferType);
    i->emitUpdate();
    
    if (i->isDir())
      recursiveSaveData(static_cast<KFTPQueue::TransferDir*>(i), sUrl, dUrl);
  }
}

void QueueEditor::slotSourceSiteChanged(KFTPBookmarks::Site *site)
{
  if (site) {
    m_layout->srcHost->setText(site->getProperty("host"));
    m_layout->srcPort->setValue(site->getIntProperty("port"));
    m_layout->srcUser->setText(site->getProperty("username"));
    m_layout->srcPass->erase();
    m_layout->srcPass->insert(site->getProperty("password"));
  } else {
    m_layout->srcHost->clear();
    m_layout->srcPort->setValue(21);
    m_layout->srcUser->clear();
    m_layout->srcPass->erase();
  }
}

void QueueEditor::slotDestSiteChanged(KFTPBookmarks::Site *site)
{
  if (site) {
    m_layout->dstHost->setText(site->getProperty("host"));
    m_layout->dstPort->setValue(site->getIntProperty("port"));
    m_layout->dstUser->setText(site->getProperty("username"));
    m_layout->dstPass->erase();
    m_layout->dstPass->insert(site->getProperty("password"));
  } else {
    m_layout->dstHost->clear();
    m_layout->dstPort->setValue(21);
    m_layout->dstUser->clear();
    m_layout->dstPass->erase();
  }
}

}

#include "queueeditor.moc"

