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

#include "kftpbookmarks.h"
#include "kftpqueue.h"
#include "kftpsession.h"

#include "misc/config.h"
#include "misc/wallet.h"
#include "misc/zeroconf.h"

#include "engine/thread.h"
#include "engine/settings.h"

#include <QFile>
#include <QSslCertificate>
#include <QSslKey>
#include <QQueue>

#include <kdebug.h>
#include <KLocale>
#include <KIconLoader>
#include <KPasswordDialog>
#include <KMessageBox>
#include <KApplication>
#include <KGlobal>
#include <KRandom>
#include <KCodecs>
#include <KStandardDirs>

using namespace KFTPCore;

Q_DECLARE_METATYPE(QSslCertificate)
Q_DECLARE_METATYPE(QSslKey)

namespace KFTPBookmarks {

Site::Site(Manager *manager, QDomNode node)
{
  // Use the specified node as data source
  refresh(manager, node);
}

Site::~Site()
{
}

void Site::refresh(Manager *manager, QDomNode node)
{
  m_manager = manager;
  m_element = node.toElement();
  
  if (m_element.tagName() == "category") {
    m_type = ST_CATEGORY;

    if (getAttribute("id").isEmpty())
      setAttribute("id", QString("cat-%1").arg(KRandom::randomString(7)), false);
  } else if (m_element.tagName() == "server") {
    m_type = ST_SITE;

    if (getAttribute("id").isEmpty())
      setAttribute("id", QString("site-%1").arg(KRandom::randomString(7)), false);
  } else if (m_element.tagName() == "sites") {
    m_type = ST_ROOT;
    return;
  }

  // Set the id
  m_id = getAttribute("id");
}

Site *Site::duplicate()
{
  if (m_type == ST_ROOT || m_type == ST_CATEGORY)
    return 0;
  
  Site *site = new Site(m_manager, m_element.cloneNode());
  site->setAttribute("name", i18n("Copy of") + " " + getAttribute("name"), false);
  site->setAttribute("id", QString::null, false);
  site->refresh(m_manager, site->m_element);

  m_element.parentNode().appendChild(site->m_element);
  m_manager->cacheSite(site);
  emit m_manager->siteAdded(site);

  return site;
}

void Site::reparentSite(Site *site)
{
  if (site->isRoot())
    return;
  
  // Move site's parent
  emit m_manager->siteRemoved(site);
  m_element.appendChild(site->m_element);
  emit m_manager->siteAdded(site);
  
}

Site *Site::addSite(const QString &name)
{
  QDomElement node = m_element.ownerDocument().createElement("server");

  // Create a new site
  Site *site = new Site(m_manager, node);
  site->setAttribute("name", name, false);
  m_element.appendChild(site->m_element);
  m_manager->cacheSite(site);
  
  emit m_manager->siteAdded(site);
  
  site->setProperty("protocol", ProtoFtp);
  site->setProperty("port", 21);
  
  return site;
}

void Site::addCategory(const QString &name)
{
  QDomElement cat = m_element.ownerDocument().createElement("category");

  // Create a new category
  Site *site = new Site(m_manager, cat);
  site->setAttribute("name", name, false);
  m_element.appendChild(cat);
  m_manager->cacheSite(site);
  
  emit m_manager->siteAdded(site);
}

KUrl Site::getUrl()
{
  KUrl url;

  // Properly translate the protocol value
  switch (protocol()) {
    case ProtoFtp: url.setProtocol("ftp"); break;
    case ProtoSftp: url.setProtocol("sftp"); break;
  }
  
  url.setHost(getProperty("host"));
  url.setPort(getIntProperty("port"));
  url.setUser(getProperty("username"));
  url.setPass(getProperty("password"));

  return url;
}

Site *Site::getParentSite()
{
  QDomNode parentNode = m_element.parentNode();

  if (parentNode.isNull())
    return 0;
  else
    return m_manager->siteForNode(parentNode);
}

Site *Site::child(int index)
{
  if (m_type == ST_CATEGORY || m_type == ST_ROOT) {
    QDomNode childNode = m_element.childNodes().at(index);
    
    if (!childNode.isNull())
      return m_manager->siteForNode(childNode);
  }
  
  return 0;
}

int Site::index() const
{
  if (m_type == ST_ROOT)
    return 0;
  
  QDomNodeList children = m_element.parentNode().childNodes();
  
  for (int i = 0; i < children.size(); i++) {
    if (children.at(i) == m_element)
      return i;
  }
  
  Q_ASSERT(false);
  return -1;
}

uint Site::childCount() const
{
  return (m_type == ST_CATEGORY || m_type == ST_ROOT) ? m_element.childNodes().size() : 0;
}

QString Site::getProperty(const QString &name) const
{
  if (m_type != ST_SITE)
    return QString::null;
  
  QDomNodeList nodes = m_element.elementsByTagName(name);

  if (nodes.length() > 0) {
    QString property = nodes.item(0).toElement().text();
    property.trimmed();

    // Transparently decode passwords
    if (name == "password")
      property = KCodecs::base64Decode(property.toAscii()).data();

    return property;
  } else {
    return QString::null;
  }
}

int Site::getIntProperty(const QString &name) const
{
  return getProperty(name).toInt();
}

void Site::setProperty(const QString &name, const QString &value)
{
  if (m_type != ST_SITE)
    return;
  
  // First delete the old property if it exists
  QDomNodeList nodes = m_element.elementsByTagName(name);

  if (nodes.length() > 0)
    m_element.removeChild(nodes.item(0));
  
  // Transparently encode passwords
  if (name == "password")
    const_cast<QString&>(value) = KCodecs::base64Encode(value.toAscii(), true).data();

  // Now add a new one
  QDomElement property = m_element.ownerDocument().createElement(name);
  m_element.appendChild(property);

  QDomText text = m_element.ownerDocument().createTextNode(value);
  property.appendChild(text);
  
  emit m_manager->siteChanged(this);
}

void Site::setProperty(const QString &name, int value)
{
  setProperty(name, QString::number(value));
}

void Site::setAttribute(const QString &name, const QString &value, bool notify)
{
  m_element.setAttribute(name, value);
  
  if (notify)
    emit m_manager->siteChanged(this);
}

QString Site::getAttribute(const QString &name) const
{
  return m_element.attribute(name);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

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
  : QObject(),
    m_rootSite(0)
{
  // Init the DOM document
  m_document = QDomDocument("KFTPgrabber");
}

Manager::Manager(const Manager *bookmarks)
  : QObject(),
    m_rootSite(0)
{
  // Init the DOM document
  m_document = QDomDocument("KFTPgrabber");

  // Copy the bookmarks
  QDomNode tmp = m_document.importNode(bookmarks->m_document.documentElement(), true);
  m_document.appendChild(tmp);
}

Manager::~Manager()
{
  qDeleteAll(m_siteCache);
  delete m_rootSite;
}

void Manager::setBookmarks(KFTPBookmarks::Manager *bookmarks)
{
  // Init the DOM document
  m_document = QDomDocument("KFTPgrabber");

  QDomNode tmp = m_document.importNode(bookmarks->m_document.documentElement(), true);
  m_document.appendChild(tmp);

  // Refresh all sites in the cache to use the new nodes
  foreach (Site *site, m_siteCache) {
    QDomNode node = findSiteElementById(site->id());
    
    if (!node.isNull())
      site->refresh(this, node);
  }
  
  // Refresh all sites in the foreign cache to use the new nodes
  foreach (Site *site, bookmarks->m_siteCache) {
    QDomNode node = findSiteElementById(site->id());
    
    if (!node.isNull())
      site->refresh(this, node);
  }
  
  // Clear foreign cache to prevent site pointer deletion
  bookmarks->m_siteCache.clear();

  emit update();
}

void Manager::importSites(QDomNode node)
{
  QDomNode import = m_document.importNode(node, true);
  m_document.documentElement().appendChild(import);
  
  // Run sanity checks to generate missing ids
  Manager::validate();
}

void Manager::load(const QString &filename)
{
  m_filename = filename;

  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly)) {
    // Create a new empty XML
    m_document.setContent(QString("<sites version=\"%1\"></sites>").arg(KFTP_BOOKMARKS_VERSION));

    return;
  }

  // Check if the file is encrpyted
  QByteArray content = file.readAll();
  
  if (Config::encryptBookmarks()) {
    // File is encrypted
    bool saveToWallet = false;

    QString password = Wallet::self()->getPassword("bookmarkDecryptPwd");
    if (password.isNull()) {
      // Ask the user for a password
      KPasswordDialog dialog(0, KPasswordDialog::ShowKeepPassword);
      dialog.setPrompt(i18n("This bookmark file is encrypted. Please enter key for decryption."));
      
      if (!dialog.exec())
        password = "";
      else
        password = dialog.password();
    }
    
    KMessageBox::error(0, "ENCRYPTED BOOKMARK FILE NOT YET REIMPLEMENTED! ALL BOOKMARKS WILL BE LOST UNLESS YOU KILL THE APPLICATION NOW!");
    
    content = "";
    // TODO use QCA
#if 0
    // Try to decrypt
    DESEncryptor des;
    des.setKey(p_pass);
    des.decrypt(content);

    if (des.output().left(6) != "<sites" && des.output().left(9) != "<!DOCTYPE") {
      // Clear any saved passwords
      KFTPAPI::getInstance()->walletConnection()->setPassword("bookmarkDecryptPwd", QString::null);

      if (KMessageBox::warningContinueCancel(
            KFTPAPI::getInstance()->mainWindow(),
            i18n("<qt>Bookmark file decryption has failed with provided key. Do you want to <b>overwrite</b> bookmarks with an empty file?<br><br><font color=\"red\"><b>Warning:</b> If you overwrite, all current bookmarks will be lost.</font></qt>"),
            i18n("Decryption Failed"),
            KGuiItem(i18n("&Overwrite Bookmarks"))
          ) != KMessageBox::Continue)
      {
        // Request the password again
        goto passwordEntry;
      }

      // Create new empty XML
      m_document.setContent(QString("<sites version=\"%1\"></sites>").arg(KFTP_BOOKMARKS_VERSION));

      file.close();
      return;
    }

    // Save the password for later encryption
    m_decryptKey = p_pass;
    content = des.output().ascii();

    if (saveToWallet) {
      // Save the password to KWallet
      KFTPAPI::getInstance()->walletConnection()->setPassword("bookmarkDecryptPwd", p_pass);
    }
#endif
  }

  m_document.setContent(QString::fromLocal8Bit(content));
  file.close();

  // Check for XML document version updates
  versionUpdate();

  // Document validation
  Manager::validate();
  
  // We have just loaded the bookmarks, so update all the menus
  emit update();
}

void Manager::save()
{
  // Save the new XML file
  if (m_filename.isEmpty())
    return;

  QFile file(m_filename);
  if (!file.open(QIODevice::WriteOnly))
    return;

  // Should we encrypt the data ?
  QString content = m_document.toString();
  if (Config::encryptBookmarks()) {
    KMessageBox::error(0, "ENCRYPTED BOOKMARK FILE NOT YET REIMPLEMENTED! ALL BOOKMARKS WILL BE LOST UNLESS YOU KILL THE APPLICATION NOW!");
#if 0
    DESEncryptor des;

    if (m_decryptKey.isEmpty()) {
      // Ask the user for the password
      KPasswordDialog::getPassword(m_decryptKey, i18n("Enter key for bookmark file encryption."));
    }

    des.setKey(m_decryptKey);
    des.encrypt(content);

    content = des.output();
#endif
  }

  // Write the XML data to the stream
  QTextStream fileStream(&file);
  fileStream << content;
  file.flush();
  file.close();
}

void Manager::validate(QDomNode node)
{
  if (node.isNull())
    node = m_document.documentElement();
    
  QDomNode n = node.firstChild();

  while (!n.isNull()) {
    if (n.toElement().tagName() == "category") {
      if (!n.toElement().hasAttribute("id"))
        n.toElement().setAttribute("id", QString("cat-%1").arg(KRandom::randomString(7)));
      
      Manager::validate(n);
    } else if (n.toElement().tagName() == "server") {
      if (!n.toElement().hasAttribute("id"))
        n.toElement().setAttribute("id", QString("site-%1").arg(KRandom::randomString(7)));
    }
    
    n = n.nextSibling();
  }
}

void Manager::versionUpdate()
{
  int version = m_document.documentElement().attribute("version").toInt();

  if (version < KFTP_BOOKMARKS_VERSION) {
    // Conversion from an old bookmark file - save backup
    QFile file(KStandardDirs::locateLocal("appdata", "bookmarks.bak"));
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      file.write(m_document.toByteArray());
      file.close();
    }

    // NOTE: There are no breaks here, since every update method updates to a specific
    // version. So in order to convert to the most current from the oldest version, all
    // methods need to be called!
    switch (version) {
      case 0:
      case 1: versionFrom1Update();
      case 2: versionFrom2Update();
    }

    // Fix the version
    m_document.documentElement().setAttribute("version", KFTP_BOOKMARKS_VERSION);
    
    QString message = i18n("Bookmarks have been successfully converted from a previous version format.<br/><br/>In case an unexpected problem has occurred during this conversion process, a backup version of your old-format bookmarks has been saved to the following location:<br/><b>%1</b>.", file.fileName());
    
    if (version <= 2) {
      message += "<br/><br/>" + i18n("Please note that since encoding names have changed in KDE4 all charset settings might be incorrect and you will have to change them to proper values.");
    }
    
    KMessageBox::information(0, "<qt>" + message + "</qt>", i18n("Bookmarks Converted"));
  }
}

void Manager::versionFrom1Update(QDomNode parent)
{
  // The original format had no site ids, so we have to generate them. Also the old
  // format used something wierd called "options", we have to convert them as well.
  // The username/password fields now have differend names.

  if (parent.isNull())
    parent = m_document.documentElement();

  QDomNode n = parent.firstChild();

  while (!n.isNull()) {
    if (n.toElement().tagName() == "category") {
      // Update the category id and all children
      n.toElement().setAttribute("id", QString("cat-%1").arg(KRandom::randomString(7)));

      versionFrom1Update(n);
    } else if (n.toElement().tagName() == "server") {
      // Update the server id
      n.toElement().setAttribute("id", QString("site-%1").arg(KRandom::randomString(7)));

      // Convert the "options"
      QDomNodeList nodes = n.toElement().elementsByTagName("option");

      if (nodes.length() > 0) {
        for (int i = 0; i < nodes.count(); i++) {
          QDomNode node = nodes.item(i);

          // Add a new standard property
          QDomElement property = m_document.createElement(node.toElement().attribute("name"));
          n.appendChild(property);

          QDomText text = m_document.createTextNode(node.toElement().attribute("value"));
          property.appendChild(text);

          // And remove the option :>
          node.parentNode().removeChild(node);
          i--;
        }
      }

      // Rename the username/password fields
      nodes = n.toElement().elementsByTagName("downuser");
      if (nodes.length() > 0) {
        for (int i = 0; i < nodes.count(); i++) {
          QDomNode node = nodes.item(i);
          node.toElement().setTagName("username");
        }
      }

      nodes = n.toElement().elementsByTagName("downpass");
      if (nodes.length() > 0) {
        for (int i = 0; i < nodes.count(); i++) {
          QDomNode node = nodes.item(i);
          node.toElement().setTagName("password");
        }
      }
    }

    n = n.nextSibling();
  }
}

void renameField(QDomNode n, const QString &name, const QString &newName)
{
  QDomNodeList nodes = n.toElement().elementsByTagName(name);
  if (nodes.length() > 0) {
    for (int i = 0; i < nodes.count(); i++) {
      QDomNode node = nodes.item(i);
      node.toElement().setTagName(newName);
    }
  }
}

void removeField(QDomNode n, const QString &name)
{
  QDomNodeList nodes = n.toElement().elementsByTagName(name);
  if (nodes.length() > 0) {
    for (int i = 0; i < nodes.count(); i++) {
      QDomNode node = nodes.item(i);
      node.parentNode().removeChild(node);
    }
  }
}

QString retrieveField(QDomNode n, const QString &name)
{
  QDomNodeList nodes = n.toElement().elementsByTagName(name);

  if (nodes.length() > 0) {
    QString property = nodes.item(0).toElement().text();
    return property.trimmed();
  }
  
  return QString::null;
}

void changeField(QDomNode n, const QString &name, const QString &value)
{
  // First delete the old property if it exists
  QDomNodeList nodes = n.toElement().elementsByTagName(name);

  if (nodes.length() > 0)
    n.toElement().removeChild(nodes.item(0));
  
  // Transparently encode passwords
  if (name == "password")
    const_cast<QString&>(value) = KCodecs::base64Encode(value.toAscii(), true).data();

  // Now add a new one
  QDomElement property = n.toElement().ownerDocument().createElement(name);
  n.toElement().appendChild(property);

  QDomText text = n.toElement().ownerDocument().createTextNode(value);
  property.appendChild(text);
}

void Manager::versionFrom2Update(QDomNode parent)
{
  // Version 2 format had some nasty properties that have to be renamed and
  // converted.
  
  if (parent.isNull())
    parent = m_document.documentElement();

  QDomNode n = parent.firstChild();

  while (!n.isNull()) {
    if (n.toElement().tagName() == "category") {
      versionFrom2Update(n);
    } else if (n.toElement().tagName() == "server") {
      //  Convert the protocol field
      QString protocol = retrieveField(n, "protocol");
      int convertedProtocol = Site::ProtoFtp;
      
      if (protocol == "ftp")
        convertedProtocol = Site::ProtoFtp;
      else if (protocol == "sftp")
        convertedProtocol = Site::ProtoSftp;
      
      changeField(n, "protocol", QString::number(convertedProtocol));
      
      // Rename/remove SSL fields
      int negotiationMode = Site::SslNone;
      
      if (retrieveField(n, "use_tls").toInt())
        negotiationMode = Site::SslAuthTLS;
      
      if (retrieveField(n, "use_implicit").toInt())
        negotiationMode = Site::SslImplicit;
      
      QString useCert = retrieveField(n, "use_cert");
      
      if (useCert == "false")
        changeField(n, "use_cert", "0");
      else if (useCert == "true")
        changeField(n, "use_cert", "1");
      
      changeField(n, "sslNegotiationMode", QString::number(negotiationMode));
      renameField(n, "use_cert", "sslCertificateEnabled");
      renameField(n, "tls_cert_path", "sslCertificatePath");
      renameField(n, "tls_data_mode", "sslProtMode");
      removeField(n, "use_tls");
      removeField(n, "use_implicit");
      removeField(n, "tls_mode");
      
      // Rename retry fields
      renameField(n, "doRetry", "retryEnabled");
      renameField(n, "retrytime", "retryDelay");
      renameField(n, "retrycount", "retryCount");
      
      // Rename keepalive fields
      renameField(n, "doKeepalive", "keepaliveEnabled");
      renameField(n, "keepaliveTimeout", "keepaliveFrequency");
      
      // Rename path fields
      renameField(n, "deflocalpath", "pathLocal");
      renameField(n, "defremotepath", "pathRemote");
      
      // Rename anonymous field
      renameField(n, "anonlogin", "anonymous");
      
      // Remove distributed FTPd field (it is autodetected anyway)
      removeField(n, "dist_ftpd");
      
      // Fix disable threads field
      QString disableThreads = retrieveField(n, "disableThreads");
      
      if (disableThreads == "true")
        changeField(n, "disableThreads", "1");
      else if (disableThreads == "false")
        changeField(n, "disableThreads", "0");
    }

    n = n.nextSibling();
  }
}

Site *Manager::siteForNode(QDomNode node)
{
  QString id = node.toElement().attribute("id");
  Site *site = m_siteCache.value(id);
  
  if (!site) {
    site = new Site(this, node);
    m_siteCache.insert(site->id(), site);
  }
  
  return site;
}

void Manager::cacheSite(Site *site)
{
  m_siteCache.insert(site->id(), site);
}

Site *Manager::rootSite()
{
  if (!m_rootSite)
    m_rootSite = new Site(this, m_document.documentElement());
  
  return m_rootSite;
}

Site *Manager::findSite(const KUrl &url)
{
  // Find the node, then see if it is already present in the cache
  QDomNode siteElement = findSiteElementByUrl(url);

  if (!siteElement.isNull()) {
    // Try to get a cached version
    Site *site = m_siteCache.value(siteElement.toElement().attribute("id"));

    if (!site) {
      site = new Site(this, siteElement);
      m_siteCache.insert(siteElement.toElement().attribute("id"), site);
    }

    return site;
  } else {
    return NULL;
  }
}

Site *Manager::findSite(const QString &id)
{
  if (id.isNull())
    return NULL;

  // Try the cache first
  Site *site = m_siteCache.value(id);

  if (!site) {
    // The site was not found, search in the DOM tree and add it to the
    // cache if found.
    QDomNode siteElement = findSiteElementById(id);

    if (siteElement.isNull())
      return NULL;

    site = new Site(this, siteElement);
    m_siteCache.insert(id, site);
  }

  return site;
}

QDomNode Manager::findSiteElementByUrl(const KUrl &url, QDomNode parent)
{
  if (parent.isNull())
    parent = m_document.documentElement();

  QDomNode n = parent.firstChild();

  while (!n.isNull()) {
    if (n.toElement().tagName() == "category") {
      // Check the category
      QDomNode site = findSiteElementByUrl(url, n);

      if (!site.isNull())
        return site;
    } else if (n.toElement().tagName() == "server") {
      // Check if the server matches
      Site *tmp = new Site(this, n);

      if (tmp->getProperty("host") == url.host() &&
          tmp->getIntProperty("port") == url.port() &&
          tmp->getProperty("username") == url.user() &&
          tmp->getProperty("password") == url.pass())
      {
        delete tmp;
        return n;
      }

      delete tmp;
    }

    n = n.nextSibling();
  }

  return QDomNode();
}

QDomNode Manager::findSiteElementById(const QString &id, QDomNode parent)
{
  if (parent.isNull())
    parent = m_document.documentElement();

  QDomNode n = parent.firstChild();

  while (!n.isNull()) {
    if (n.toElement().tagName() == "category") {
      // Check the category
      QDomNode site = findSiteElementById(id, n);

      if (!site.isNull())
        return site;
    } else if (n.toElement().tagName() == "server") {
      // Check if the server matches
      if (n.toElement().attribute("id") == id)
        return n;
    }

    n = n.nextSibling();
  }

  return QDomNode();
}

QDomNode Manager::findCategoryElementById(const QString &id, QDomNode parent)
{
  if (id == "root")
    return m_document.documentElement();

  if (parent.isNull())
    parent = m_document.documentElement();

  QDomNode n = parent.firstChild();

  while (!n.isNull()) {
    if (n.toElement().tagName() == "category") {
      if (n.toElement().attribute("id") == id)
        return n;

      // Check the category
      QDomNode site = findCategoryElementById(id, n);

      if (!site.isNull())
        return site;
    }

    n = n.nextSibling();
  }

  return QDomNode();
}

Site *Manager::findCategory(const QString &id)
{
  // Try the cache first
  Site *site = m_siteCache.value(id);

  if (!site) {
    // The site was not found, search in the DOM tree and add it to the
    // cache if found.
    QDomNode siteElement = findCategoryElementById(id);

    if (siteElement.isNull())
      return NULL;

    site = new Site(this, siteElement);
    m_siteCache.insert(id, site);
  }

  return site;
}

void Manager::removeSite(Site *site)
{
  emit siteRemoved(site);
  
  // Remove the node from the DOM tree
  site->m_element.parentNode().removeChild(site->m_element);

  // Remove the site from cache and it will be automaticly deleted
  m_siteCache.remove(site->id());
  
  emit update();
}

void Manager::setupClient(Site *site, KFTPEngine::Thread *client, KFTPEngine::Thread *primary)
{
  KFTPEngine::Settings *settings = client->settings();
  settings->initConfig();
  
  if (site) {
    if (site->getIntProperty("retryEnabled")) {
      settings->setConfig("retry", true);
      settings->setConfig("max_retries", site->getIntProperty("retryCount"));
      settings->setConfig("retry_delay", site->getIntProperty("retryDelay"));
    } else {
      settings->setConfig("retry", false);
    }
    
    settings->setConfig("keepalive.enabled", site->getIntProperty("keepaliveEnabled"));
    settings->setConfig("keepalive.timeout", site->getIntProperty("keepaliveFrequency"));
    
    settings->setConfig("encoding", site->getProperty("encoding"));
    
    if (site->protocol() == Site::ProtoFtp) {
      settings->setConfig("feat.pasv", site->getIntProperty("disablePASV") != 1);
      settings->setConfig("feat.epsv", site->getIntProperty("disableEPSV") != 1);
      settings->setConfig("pasv.use_site_ip", site->getIntProperty("pasvSiteIp"));
      settings->setConfig("active.no_force_ip", site->getIntProperty("disableForceIp"));
      settings->setConfig("stat_listings", site->getIntProperty("statListings"));
      
      if (site->getIntProperty("sslNegotiationMode") != Site::SslNone) {
        settings->setConfig("ssl.use_tls", true);
        
        if (site->getIntProperty("sslNegotiationMode") == Site::SslImplicit)
          settings->setConfig("ssl.use_implicit", true);
        
        settings->setConfig("ssl.prot_mode", site->getIntProperty("sslProtMode"));
        
        if (primary) {
          // A primary client has been given, so this is a secondary connection
          settings->setConfig("ssl.ignore_errors", true);
          settings->setConfig("ssl.certificate", primary->settings()->getConfig("ssl.certificate"));
          settings->setConfig("ssl.private_key", primary->settings()->getConfig("ssl.private_key"));
        } else if (site->getIntProperty("sslCertificateEnabled")) {
          // Ask the user for the decryption password
          QString password;
          KPasswordDialog dialog;
          dialog.setPrompt(i18n("Please provide your SSL private key decryption password."));
          
          if (dialog.exec())
            password = dialog.password();
          
          // Load certificate and private key
          QFile file(site->getProperty("sslKeyfilePath"));
          if (!file.open(QIODevice::ReadOnly)) {
            KMessageBox::error(0, i18n("Unable to open local private key file."));
          } else {
            QSslCertificate certificate = QSslCertificate::fromPath(site->getProperty("sslCertificatePath")).first();
            QSslKey key(&file, QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey, password.toAscii());
            
            if (!certificate.isValid() || key.isNull()) {
              KMessageBox::error(0, i18n("Local SSL certificate and/or private key is not valid."));
            } else {
              QVariant certificateVariant;
              certificateVariant.setValue(certificate);
              
              QVariant keyVariant;
              keyVariant.setValue(key);
              
              settings->setConfig("ssl.certificate", certificateVariant);
              settings->setConfig("ssl.private_key", keyVariant);
            }
          }
        }
      }
    } else if (site->protocol() == Site::ProtoSftp) {
      if (primary) {
        // A primary client has been given, so this is a secondary connection
        settings->setConfig("auth.ignore_fingerprint", true);
        settings->setConfig("auth.privkey_password", primary->settings()->getConfig("auth.privkey_password"));
      }
      
      // Should we use public key authentication ?
      if (site->getIntProperty("sshPubkeyAuth")) {
        QString pubkeyPath = site->getProperty("sshPublicKey");
        QString privkeyPath = site->getProperty("sshPrivateKey");
        
        if (!QFileInfo(pubkeyPath).exists() || !QFileInfo(privkeyPath).exists()) {
          if (!primary)
            KMessageBox::error(0, i18n("SSH public or private key file does not exist - pubkey authentication disabled."));
        } else {
          settings->setConfig("auth.pubkey", true);
          settings->setConfig("auth.pubkey_path", pubkeyPath);
          settings->setConfig("auth.privkey_path", privkeyPath);
        }
      }
    }
  }
}

QList<QAction*> Manager::populateBookmarksMenu(KActionMenu *parentMenu, KFTPSession::Session *session)
{
  QList<QAction*> list;
  QQueue<QPair<QDomElement, KActionMenu*> > categoryQueue, siteQueue;
  categoryQueue.enqueue(qMakePair(m_document.documentElement(), parentMenu));
  
  // Create all the categories
  while (!categoryQueue.isEmpty()) {
    QPair<QDomElement, KActionMenu*> category = categoryQueue.dequeue();
    QDomNode n = category.first.firstChild();
    
    while (!n.isNull()) {
      QDomElement e = n.toElement();
  
      if (e.tagName() == "category") {
        KActionMenu *menu;
        
        if (category.second) {
          menu = new KActionMenu(KIcon("folder-bookmarks"), e.attribute("name"), category.second);
          category.second->addAction(menu);
        } else {
          menu = new KActionMenu(KIcon("folder-bookmarks"), e.attribute("name"), this);
          list.append(menu);
        }
        
        categoryQueue.enqueue(qMakePair(e, menu));
      } else if (e.tagName() == "server") {
        siteQueue.enqueue(qMakePair(e, category.second));
      }
  
      n = n.nextSibling();
    }
  }
  
  // Now insert all sites
  while (!siteQueue.isEmpty()) {
    QPair<QDomElement, KActionMenu*> site = siteQueue.dequeue();
    
    BookmarkActionData actionData;
    actionData.siteId = site.first.attribute("id");
    actionData.session = session;
    
    QVariant dataVariant;
    dataVariant.setValue(actionData);
    
    KAction *action = site.second ? new KAction(site.second) : new KAction(this);
    action->setText(site.first.attribute("name"));
    action->setIcon(KIcon("bookmarks"));
    action->setData(dataVariant);
    connect(action, SIGNAL(triggered()), this, SLOT(slotBookmarkExecuted()));

    if (site.second)
      site.second->addAction(action);
    else
      list.append(action);
  }
  
  return list;
}

void Manager::populateZeroconfMenu(KActionMenu *parentMenu)
{
  // Clear the menu
  parentMenu->menu()->clear();
  QList<DNSSD::RemoteService::Ptr> list = ZeroConf::self()->getServiceList();

  if (!list.empty()) {
    foreach (DNSSD::RemoteService::Ptr p, list) {
      KUrl url;
      url.setHost(p->hostName());
      url.setPort(p->port());
      
      KAction *action = new KAction(this);
      action->setText(p->serviceName());
      action->setIcon(KIcon("network-workgroup"));
      action->setData(url);
      connect(action, SIGNAL(triggered), this, SLOT(slotZeroconfExecuted()));

      parentMenu->addAction(action);
    }
  } else {
    KAction *disabledAction = new KAction(this);
    disabledAction->setText(i18n("<No Services Published>"));
    disabledAction->setEnabled(false);
    parentMenu->addAction(disabledAction);
  }
}

void Manager::populateWalletMenu(KActionMenu *parentMenu)
{
  // Clear the menu
  parentMenu->menu()->clear();
  
  // Populate
  QList<KUrl> list = Wallet::self()->getSiteList();

  if (!list.empty()) {
    foreach (KUrl url, list) {
      QString desc;
      
      if (url.port() != 21)
        desc = QString("%1@%2:%3").arg(url.user()).arg(url.host()).arg(url.port());
      else
        desc = QString("%1@%2").arg(url.user()).arg(url.host());
      
      KAction *action = new KAction(this);
      action->setText(desc);
      action->setIcon(KIcon("folder-remote"));
      action->setData(url);
      connect(action, SIGNAL(triggered()), this, SLOT(slotWalletExecuted()));

      parentMenu->addAction(action);
    }
  } else {
    KAction *disabledAction = new KAction(this);
    disabledAction->setText(i18n("<No Sites In KWallet>"));
    disabledAction->setEnabled(false);
    parentMenu->addAction(disabledAction);
  }
}

void Manager::slotBookmarkExecuted()
{
  // Get the sender
  KAction *action = (KAction*) QObject::sender();
  BookmarkActionData data = action->data().value<BookmarkActionData>();
  Site *site = findSite(data.siteId);
  connectWithSite(site, data.session);
}

void Manager::connectWithSite(Site *site, KFTPSession::Session *session)
{
  // Get the node data from bookmarks
  KUrl siteUrl = site->getUrl();
  
  // Handle empty usernames and passwords for non-anonymous sites
  if (!siteUrl.hasUser() ||
      (!siteUrl.hasPass() && siteUrl.user() != "anonymous" &&
       (site->protocol() != Site::ProtoSftp || !site->getIntProperty("sshPubkeyAuth"))
      )
     ) {
    KPasswordDialog *dlg = new KPasswordDialog(0, KPasswordDialog::ShowUsernameLine | KPasswordDialog::ShowKeepPassword);
    dlg->setPrompt(i18n("Please provide your username and password for connecting to this site."));
    dlg->addCommentLine(i18n("Site:"), QString("%1:%2").arg(siteUrl.host()).arg(siteUrl.port()));
    dlg->setUsername(siteUrl.user());
    
    if (dlg->exec()) {
      siteUrl.setUser(dlg->username());
      siteUrl.setPass(dlg->password());
      
      if (dlg->keepPassword()) {
        // Save password to the bookmarked site
        site->setProperty("username", dlg->username());
        site->setProperty("password", dlg->password());
      }
      
      delete dlg;
    } else {
      // Abort connection attempt
      delete dlg;
      return;
    }
  }

  if (session) {
    // Set the correct client for connection
    KFTPEngine::Thread *client = session->getClient();

    // Now, connect to the server
    if (session->isRemote() && session->isConnected()) {
      if (Config::confirmDisconnects() &&
          KMessageBox::warningYesNo(0, i18n("Do you want to drop current connection?")) == KMessageBox::No)
        return;
    }

    client->socket()->setCurrentUrl(siteUrl);

    // Set the session's site and connect
    session->setSite(site);
    session->reconnect(siteUrl);
  } else {
    // Just spawn a new session
    session = KFTPSession::Manager::self()->spawnRemoteSession(KFTPSession::IgnoreSide, siteUrl, site);
    KFTPSession::Manager::self()->setActive(session);
  }
}

void Manager::slotWalletExecuted()
{
  // Get the sender
  KAction *action = (KAction*) QObject::sender();

  // Just spawn a new session
  KFTPSession::Manager::self()->spawnRemoteSession(KFTPSession::IgnoreSide, action->data().value<KUrl>());
}

void Manager::slotZeroconfExecuted()
{
  // Get the sender
  KAction *action = (KAction*) QObject::sender();
  KUrl url = action->data().value<KUrl>();
  
  // FIXME
  //KFTPAPI::getInstance()->mainWindow()->slotQuickConnect("", url.host(), url.port());
}

}


#include "kftpbookmarks.moc"
