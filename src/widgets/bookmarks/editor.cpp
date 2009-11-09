/*
 * This file is part of the KFTPGrabber project
 *
 * Copyright (C) 2003-2007 by the KFTPGrabber developers
 * Copyright (C) 2003-2007 Jernej Kos <kostko@jweb-network.net>
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
#include "bookmarks/editor.h"
#include "bookmarks/treeview.h"
#include "bookmarks/model.h"

#include "misc/config.h"

#include "kftpbookmarks.h"

#include <QGridLayout>

#include <KHBox>
#include <KVBox>
#include <KLocale>
#include <KGlobal>
#include <KCharsets>
#include <KPushButton>

using namespace KFTPBookmarks;

namespace KFTPWidgets {

namespace Bookmarks {

Editor::Editor(QWidget *parent, bool quickConnect)
 : KDialog(parent),
   m_selectedSite(0),
   m_portChanged(false)
{
  // Create a copy of the current bookmark manager for our working needs
  m_manager = new Manager(Manager::self());
  
  KHBox *layout = new KHBox(this);
  
  Model *bookmarksModel = new Model(m_manager, this);
  m_proxyModel = new QSortFilterProxyModel(this);
  m_proxyModel->setSourceModel(bookmarksModel);
  m_proxyModel->setSortRole(Model::TypeRole);
  
  KVBox *leftPart = new KVBox(layout);
  m_treeView = new TreeView(m_manager, leftPart);
  m_treeView->setModel(m_proxyModel);
  
  m_properties = new QWidget(layout);
  m_layout.setupUi(m_properties);
  m_properties->setEnabled(false);
  
  //KHBox *buttons = new KHBox(leftPart);
  QWidget *widget = new QWidget(leftPart);
  QGridLayout *buttons = new QGridLayout(widget);
  buttons->setContentsMargins(0, 0, 0, 0);
  
  m_newSite = new KPushButton(KIcon("bookmark-new"), i18n("&Add Site"), widget);
  m_removeSite = new KPushButton(KIcon("edit-delete"), i18n("&Remove"), widget);
  m_removeSite->setEnabled(false);
  m_newCategory = new KPushButton(KIcon("folder-new"), i18n("&Add Category"), widget);
  m_duplicateSite = new KPushButton(KIcon("edit-copy"), i18n("&Duplicate Site"), widget);
  m_duplicateSite->setEnabled(false);
  buttons->addWidget(m_newSite, 0, 0);
  buttons->addWidget(m_removeSite, 0, 1);
  buttons->addWidget(m_newCategory, 1, 0);
  buttons->addWidget(m_duplicateSite, 1, 1);
  buttons->setSpacing(2);
  
  connect(m_newSite, SIGNAL(clicked(bool)), m_treeView, SLOT(slotCreateNewSite()));
  connect(m_removeSite, SIGNAL(clicked(bool)), m_treeView, SLOT(slotRemove()));
  connect(m_newCategory, SIGNAL(clicked(bool)), m_treeView, SLOT(slotCreateSubcategory()));
  connect(m_duplicateSite, SIGNAL(clicked(bool)), m_treeView, SLOT(slotDuplicate()));
  
  if (quickConnect)
    setWindowTitle(i18n("Quick Connect"));
  else
    setWindowTitle(i18n("Bookmark Editor"));
  
  setButtons(Cancel | Ok | User1);
  setButtonText(User1, i18n("Connect With Selected Site"));
  setButtonIcon(User1, KIcon("network-connect"));
  enableButton(User1, false);
  setMainWidget(layout);
  
  showButton(Ok, !quickConnect);
  
  // Some minor setup
  m_layout.localPath->setMode(KFile::Directory | KFile::LocalOnly);
  m_layout.notes->setAcceptRichText(false);
  
  // Populate the charsets combo
  foreach (QString description, KGlobal::charsets()->descriptiveEncodingNames()) {
    m_layout.encoding->addItem(description, KGlobal::charsets()->encodingForName(description));
  }
  
  connect(m_treeView, SIGNAL(clicked(const QModelIndex&)), this, SLOT(slotItemClicked(const QModelIndex&)));
  connect(m_layout.anonymous, SIGNAL(toggled(bool)), this, SLOT(slotAnonymousToggled(bool)));
  connect(m_layout.protocol, SIGNAL(currentIndexChanged(int)), this, SLOT(slotProtocolChanged(int)));
  connect(m_layout.port, SIGNAL(valueChanged(int)), this, SLOT(slotPortChanged()));
  connect(m_layout.siteName, SIGNAL(textChanged(const QString&)), this, SLOT(slotCurrentNameChanged(const QString&)));
  
  connect(this, SIGNAL(okClicked()), this, SLOT(slotOkClicked()));
  connect(this, SIGNAL(cancelClicked()), this, SLOT(slotCancelClicked()));
  connect(this, SIGNAL(user1Clicked()), this, SLOT(slotConnectClicked()));
  
  connect(m_manager, SIGNAL(siteRemoved(KFTPBookmarks::Site*)), this, SLOT(slotSiteRemoved(KFTPBookmarks::Site*)));
}

void Editor::slotOkClicked()
{
  saveCurrentItem();
  
  // Merge our modified bookmarks back to the original ones
  Manager::self()->setBookmarks(m_manager);
  delete m_manager;
}

void Editor::slotCancelClicked()
{
  // Any modifications have been lost
  delete m_manager;
}

void Editor::slotConnectClicked()
{
  slotOkClicked();
  accept();
}

void Editor::slotItemClicked(const QModelIndex &index)
{
  QModelIndex realIndex = m_proxyModel->mapToSource(index);
  
  if (realIndex.isValid()) {
    if (m_currentIndex != realIndex) {
      // Index has been changed, save the old site
      saveCurrentItem();
      m_currentIndex = realIndex;
    }
    
    Site *site = realIndex.data(Model::SiteRole).value<Site*>();
    m_selectedSite = site;
    m_layout.tabWidget->setCurrentIndex(0);
    m_removeSite->setEnabled(true);
    
    if (site->isSite()) {
      enableButton(User1, true);
      m_duplicateSite->setEnabled(true);
      m_properties->setEnabled(true);
      
      // Populate the properties widget
      m_layout.siteName->setText(site->getAttribute("name"));
      m_layout.protocol->setCurrentIndex(site->protocol());
      m_layout.ipAddress->setText(site->getProperty("host"));
      m_layout.port->setValue(site->getIntProperty("port"));
      
      if (site->getIntProperty("anonymous")) {
        m_layout.anonymous->setChecked(true);
      } else {
        m_layout.anonymous->setChecked(false);
        m_layout.userName->setText(site->getProperty("username"));
        m_layout.password->setText(site->getProperty("password"));
      }
      
      m_layout.remotePath->setText(site->getProperty("pathRemote"));
      m_layout.localPath->setPath(site->getProperty("pathLocal"));
      m_layout.notes->setPlainText(site->getProperty("description"));
      
      m_layout.disableExtendedPassive->setChecked(site->getIntProperty("disableEPSV"));
      m_layout.disablePassive->setChecked(site->getIntProperty("disablePASV"));
      m_layout.useSiteIp->setChecked(site->getIntProperty("pasvSiteIp"));
      m_layout.disablePresetIp->setChecked(site->getIntProperty("disableForceIp"));
      m_layout.useStat->setChecked(site->getIntProperty("statListings"));
      m_layout.disableThreads->setChecked(site->getIntProperty("disableThreads"));
      
      QString encoding = site->getProperty("encoding");
      if (encoding.isEmpty())
        encoding = KFTPCore::Config::defEncoding();
      
      m_layout.encoding->setCurrentIndex(m_layout.encoding->findData(encoding));
      m_layout.retry->setChecked(site->getIntProperty("retryEnabled"));
      m_layout.retryDelay->setValue(site->getIntProperty("retryDelay"));
      m_layout.retryCount->setValue(site->getIntProperty("retryCount"));
      m_layout.keepalive->setChecked(site->getIntProperty("keepaliveEnabled"));
      m_layout.keepaliveFrequency->setValue(site->getIntProperty("keepaliveFrequency"));
      
      // Select the proper security widget
      slotProtocolChanged(site->protocol());
      
      switch (site->protocol()) {
        case Site::ProtoFtp: {
          m_layout.sslNegotiationType->setCurrentIndex(site->getIntProperty("sslNegotiationMode"));
          m_layout.sslProtMode->setCurrentIndex(site->getIntProperty("sslProtMode"));
          m_layout.sslClientCert->setChecked(site->getIntProperty("sslCertificateEnabled"));
          m_layout.sslCertPath->setPath(site->getProperty("sslCertificatePath"));
          m_layout.sslKeyPath->setPath(site->getProperty("sslKeyfilePath"));
          
          enableOptions();
          break;
        }
        case Site::ProtoSftp: {
          m_layout.sshPubkey->setChecked(site->getIntProperty("sshPubkeyAuth"));
          m_layout.sshPublicKey->setPath(site->getProperty("sshPublicKey"));
          m_layout.sshPrivateKey->setPath(site->getProperty("sshPrivateKey"));
          
          disableOptions();
          break;
        }
      }
      
      return;
    }
  } else {
    m_removeSite->setEnabled(false);
  }
  
  m_duplicateSite->setEnabled(false);
  enableButton(User1, false);
  m_properties->setEnabled(false);
      
  // Clear the properties widget
  m_layout.siteName->clear();
  m_layout.protocol->setCurrentIndex(0);
  m_layout.securityStack->setCurrentIndex(0);
  m_layout.ipAddress->clear();
  m_layout.port->setValue(21);
  m_layout.anonymous->setChecked(false);
  m_layout.userName->clear();
  m_layout.password->clear();
  m_layout.remotePath->clear();
  m_layout.localPath->clear();
  m_layout.notes->clear();
  
  m_layout.disableExtendedPassive->setChecked(false);
  m_layout.disablePassive->setChecked(false);
  m_layout.useSiteIp->setChecked(false);
  m_layout.disablePresetIp->setChecked(false);
  m_layout.useStat->setChecked(false);
  m_layout.disableThreads->setChecked(false);
}

void Editor::saveCurrentItem()
{
  if (!m_currentIndex.isValid())
    return;
  
  Site *site = m_currentIndex.data(Model::SiteRole).value<Site*>();
  
  if (site->isSite()) {
    site->setAttribute("name", m_layout.siteName->text());
    site->setProperty("protocol", m_layout.protocol->currentIndex());
    site->setProperty("host", m_layout.ipAddress->text());
    site->setProperty("port", m_layout.port->value());
    
    site->setProperty("anonymous", m_layout.anonymous->isChecked());
    site->setProperty("username", m_layout.userName->text());
    site->setProperty("password", m_layout.password->text());
    
    site->setProperty("pathRemote", m_layout.remotePath->text());
    site->setProperty("pathLocal", m_layout.localPath->url().path());
    site->setProperty("description", m_layout.notes->toPlainText());
    
    site->setProperty("disableEPSV", m_layout.disableExtendedPassive->isChecked());
    site->setProperty("disablePASV", m_layout.disablePassive->isChecked());
    site->setProperty("pasvSiteIp", m_layout.useSiteIp->isChecked());
    site->setProperty("disableForceIp", m_layout.disablePresetIp->isChecked());
    site->setProperty("statListings", m_layout.useStat->isChecked());
    site->setProperty("disableThreads", m_layout.disableThreads->isChecked());
    
    site->setProperty("encoding", m_layout.encoding->itemData(m_layout.encoding->currentIndex()).toString());
    site->setProperty("retryEnabled", m_layout.retry->isChecked());
    site->setProperty("retryDelay", m_layout.retryDelay->value());
    site->setProperty("retryCount", m_layout.retryCount->value());
    site->setProperty("keepaliveEnabled", m_layout.keepalive->isChecked());
    site->setProperty("keepaliveFrequency", m_layout.keepaliveFrequency->value());
    
    switch (site->protocol()) {
      case Site::ProtoFtp: {
        site->setProperty("sslNegotiationMode", m_layout.sslNegotiationType->currentIndex());
        site->setProperty("sslProtMode", m_layout.sslProtMode->currentIndex());
        site->setProperty("sslCertificateEnabled", m_layout.sslClientCert->isChecked());
        site->setProperty("sslCertificatePath", m_layout.sslCertPath->url().path());
        site->setProperty("sslKeyfilePath", m_layout.sslKeyPath->url().path());
        break;
      }
      case Site::ProtoSftp: {
        site->setProperty("sshPubkeyAuth", m_layout.sshPubkey->isChecked());
        site->setProperty("sshPublicKey", m_layout.sshPublicKey->url().path());
        site->setProperty("sshPrivateKey", m_layout.sshPrivateKey->url().path());
        break;
      }
    }
  }
}

void Editor::disableOptions()
{
  m_layout.anonymous->setChecked(false);
  m_layout.anonymous->setEnabled(false);
  m_layout.disableExtendedPassive->setChecked(false);
  m_layout.disableExtendedPassive->setEnabled(false);
  m_layout.disablePassive->setChecked(false);
  m_layout.disablePassive->setEnabled(false);
  m_layout.useSiteIp->setChecked(false);
  m_layout.useSiteIp->setEnabled(false);
  m_layout.disablePresetIp->setChecked(false);
  m_layout.disablePresetIp->setEnabled(false);
  m_layout.useStat->setChecked(false);
  m_layout.useStat->setEnabled(false);
}

void Editor::enableOptions()
{
  m_layout.anonymous->setEnabled(true);
  m_layout.disableExtendedPassive->setEnabled(true);
  m_layout.disablePassive->setEnabled(true);
  m_layout.useSiteIp->setEnabled(true);
  m_layout.disablePresetIp->setEnabled(true);
  m_layout.useStat->setEnabled(true);
}

void Editor::slotCurrentNameChanged(const QString &name)
{
  if (!m_currentIndex.isValid())
    return;
  
  Site *site = m_currentIndex.data(Model::SiteRole).value<Site*>();
  
  if (site->isSite()) {
    // Current name has changed, so let's update the attribute
    site->setAttribute("name", name);
  }
}

void Editor::slotPortChanged()
{
  m_portChanged = true;
}

void Editor::slotProtocolChanged(int index)
{
  int securityWidget, port = -1;
  
  switch (index) {
    case Site::ProtoFtp: {
      securityWidget = 1;
      
      if (m_layout.port->value() == 22)
        port = 21;
      
      enableOptions();
      break;
    }
    case Site::ProtoSftp: {
      securityWidget = 2;
      
      if (m_layout.port->value() == 21)
        port = 22;
      
      disableOptions();
      break;
    }
    default: securityWidget = 0; break;
  }
  
  m_layout.securityStack->setCurrentIndex(securityWidget);
  
  if (!m_portChanged && port > 0) {
    m_layout.port->setValue(port);
    m_portChanged = false;
  }
}

void Editor::slotAnonymousToggled(bool value)
{
  if (value) {
    m_layout.userName->setEnabled(false);
    m_layout.password->setEnabled(false);
    m_layout.userName->setText("anonymous");
    
    if (!KFTPCore::Config::anonMail().isEmpty())
      m_layout.password->setText(KFTPCore::Config::anonMail());
    else
      m_layout.password->setText("userlogin@anonymo.us");
  } else {
    m_layout.userName->setEnabled(true);
    m_layout.password->setEnabled(true);
    m_layout.userName->clear();
    m_layout.password->clear();
  }
}

void Editor::slotSiteRemoved(KFTPBookmarks::Site *site)
{
  if (m_currentIndex.isValid()) {
    Site *currentSite = m_currentIndex.data(Model::SiteRole).value<Site*>();
    
    if (site == currentSite) {
      slotItemClicked(QModelIndex());
      m_currentIndex = QModelIndex();
    }
  }
}

}

}

#include "editor.moc"
