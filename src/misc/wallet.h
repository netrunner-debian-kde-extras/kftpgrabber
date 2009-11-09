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

#ifndef KFTPCOREWALLET_H
#define KFTPCOREWALLET_H

#include <KWallet/Wallet>
#include <QObject>

namespace KFTPCore {

class WalletPrivate;

/**
 * Enables communication with KDE's wallet system (KWallet).
 *
 * @author Jernej Kos <kostko@unimatrix-one.org
 */
class Wallet : public QObject
{
Q_OBJECT
friend class WalletPrivate;
public:
    /**
     * Returns the global Wallet class instance.
     */
    static Wallet *self();
    
    /**
     * Retrieves the FTP site list stored in system-wide wallet.
     */
    QList<KUrl> getSiteList();
    
    /**
     * Returns a password stored in the wallet.
     *
     * @param whatFor Password key name
     */
    QString getPassword(const QString &whatFor);
    
    /**
     * Stores a password to the wallet.
     *
     * @param whatFor Password key name
     * @param password Value
     */
    void setPassword(const QString &whatFor, const QString &password);
private:
    /**
     * Class constructor.
     */
    Wallet();
    
    /**
     * Class destructor.
     */
    ~Wallet();
private:
    KWallet::Wallet *m_wallet;
    uint m_walletRefCount;
private slots:
    void slotWalletClosed();
};

}

#endif
