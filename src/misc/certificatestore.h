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
 *
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */ 
#ifndef KFTPCORECERTIFICATESTORE_H
#define KFTPCORECERTIFICATESTORE_H

#include <QSslCertificate>
#include <QList>
#include <QMap>
#include <QByteArray>

#include <KUrl>

namespace KFTPCore {

/**
 * A global store for trusted SSL certificates and SSH fingerprints.
 *
 * @author Jernej Kos <kostko@unimatrix-one.org>
 */
class CertificateStore {
public:
    /**
     * Class constructor.
     */
    CertificateStore();
    
    /**
     * Copy constructor.
     */
    CertificateStore(const CertificateStore &store);
    
    /**
     * Adds a certificate to the certificate store.
     */
    void addCertificate(const QSslCertificate &certificate);
    
    /**
     * Adds a fingerprint to the store.
     */
    void addFingerprint(const KUrl &url, const QByteArray &fingerprint);
    
    /**
     * Returns true if the fingerprint is authentic, false otherwise.
     */
    bool verifyFingerprint(const KUrl &url, const QByteArray &fingerprint) const;
    
    /**
     * Returns a list of trusted certificates currently in the certificate
     * store.
     */
    QList<QSslCertificate> trustedCertificates() const;
    
    /**
     * Saves the certificate store to disk in PEM format.
     */
    void save();
protected:
    /**
     * Normalizes an URL.
     */
    KUrl normalizeUrl(const KUrl &url) const;
private:
    QList<QSslCertificate> m_store;
    QMap<KUrl, QByteArray> m_fingerprints;
};

}

#endif
