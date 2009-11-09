/*
 * This file is part of the KFTPGrabber project
 *
 * Copyright (C) 2007 by the KFTPGrabber developers
 * Copyright (C) 2007 Jernej Kos <kostko@jweb-network.net>
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

#include <KUrl>
#include <KGlobal>
 
#include "wallet.h"

namespace KFTPCore {

class WalletPrivate {
public:
    Wallet instance;
};

K_GLOBAL_STATIC(WalletPrivate, walletPrivate)

Wallet *Wallet::self()
{
  return &walletPrivate->instance;
}

Wallet::Wallet()
 : QObject()
{
  m_wallet = 0L;
  m_walletRefCount = 0;
}


Wallet::~Wallet()
{
  slotWalletClosed();
}

void Wallet::slotWalletClosed()
{
  m_walletRefCount--;
  
  if (m_walletRefCount == 0) {
    delete m_wallet;
    m_wallet = 0L;
  }
}

QList<KUrl> Wallet::getSiteList()
{
  QList<KUrl> sites;
  
  if (!KWallet::Wallet::folderDoesNotExist(KWallet::Wallet::NetworkWallet(), KWallet::Wallet::PasswordFolder())) {
    if (!m_wallet) {
      m_wallet = KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(), 0, KWallet::Wallet::Synchronous);
      
      if (m_wallet) {
        m_walletRefCount++;
        connect(m_wallet, SIGNAL(walletClosed()), this, SLOT(slotWalletClosed()));
      }
    }
    
    if (!m_wallet)
      return sites;
    
    // Get the site list from our wallet
    m_wallet->setFolder(KWallet::Wallet::PasswordFolder());
    
    QStringList list = m_wallet->entryList();
    QStringList::iterator i;
    
    for (i = list.begin(); i != list.end(); ++i) {
      QMap<QString, QString> map;
      
      if ((*i).startsWith("ftp-") && m_wallet->readMap(*i, map) == 0) {
        QString name = *i;
        name.replace("ftp-", "ftp://");
        
        KUrl siteUrl(name);
        siteUrl.setUser(map["login"]);
        siteUrl.setPass(map["password"]);
        
        if (siteUrl.port() == 0)
          siteUrl.setPort(21);
          
        if (sites.contains(siteUrl) == 0)
          sites.append(siteUrl);
      }
    }
  }
  
  return sites;
}

QString Wallet::getPassword(const QString &whatFor)
{
  if (!KWallet::Wallet::folderDoesNotExist(KWallet::Wallet::NetworkWallet(), "KFTPGrabber")) {
    // We have our own folder
    if (!m_wallet) {
      m_wallet = KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(), 0, KWallet::Wallet::Synchronous);
      
      if (m_wallet) {
        m_walletRefCount++;
        connect(m_wallet, SIGNAL(walletClosed()), this, SLOT(slotWalletClosed()));
      }
    }
    
    // Try to read the password from the wallet
    QString pass;
    if (m_wallet && m_wallet->setFolder("KFTPGrabber") && m_wallet->readPassword(whatFor, pass) == 0) {
      return pass;
    }
  }
  
  return QString::null;
}

void Wallet::setPassword(const QString &whatFor, const QString &password)
{
  if (KWallet::Wallet::isEnabled()) {
    if (!m_wallet) {
      m_wallet = KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(), 0, KWallet::Wallet::Synchronous);
      
      if (m_wallet) {
        m_walletRefCount++;
        connect(m_wallet, SIGNAL(walletClosed()), this, SLOT(slotWalletClosed()));
      }
    }
    
    if (m_wallet) {
      // Create our folder
      if (!m_wallet->hasFolder("KFTPGrabber")) {
        m_wallet->createFolder("KFTPGrabber");
      }
      
      m_wallet->setFolder("KFTPGrabber");
      m_wallet->writePassword(whatFor, password);
    }
  }
}

}

#include "wallet.moc"
