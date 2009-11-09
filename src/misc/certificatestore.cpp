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
#include "certificatestore.h"

#include <kstandarddirs.h>
#include <QFile>
#include <QDataStream>

namespace KFTPCore {

CertificateStore::CertificateStore()
{
  // Load certificates
  m_store = QSslCertificate::fromPath(KStandardDirs::locateLocal("appdata", "certstore"));
  
  // Load fingerprints
  QFile file(KStandardDirs::locateLocal("appdata", "fpstore"));
  if (file.open(QIODevice::ReadOnly)) {
    QDataStream stream(&file);
    stream >> m_fingerprints;
    file.close();
  }
}

CertificateStore::CertificateStore(const CertificateStore &store)
{
  m_store = store.m_store;
}

void CertificateStore::addCertificate(const QSslCertificate &certificate)
{
  if (!m_store.contains(certificate))
    m_store.append(certificate);
}

void CertificateStore::addFingerprint(const KUrl &url, const QByteArray &fingerprint)
{
  m_fingerprints[normalizeUrl(url)] = fingerprint;
}

bool CertificateStore::verifyFingerprint(const KUrl &url, const QByteArray &fingerprint) const
{
  return m_fingerprints[normalizeUrl(url)] == fingerprint;
}

QList<QSslCertificate> CertificateStore::trustedCertificates() const
{
  return m_store;
}

KUrl CertificateStore::normalizeUrl(const KUrl &url) const
{
  KUrl normalizedUrl = url;
  normalizedUrl.setPath("/");
  normalizedUrl.setUser("");
  normalizedUrl.setPass("");
  
  return normalizedUrl;
}

void CertificateStore::save()
{
  // Save certificates
  QFile file(KStandardDirs::locateLocal("appdata", "certstore"));
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    return;
  
  foreach (QSslCertificate certificate, m_store) {
    file.write(certificate.toPem());
  }
  
  file.close();
  
  // Save fingerprints
  file.setFileName(KStandardDirs::locateLocal("appdata", "fpstore"));
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    return;
  
  QDataStream stream(&file);
  stream << m_fingerprints;
  
  file.close();
}

}

