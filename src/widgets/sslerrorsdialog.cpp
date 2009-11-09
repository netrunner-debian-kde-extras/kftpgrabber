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
#include "sslerrorsdialog.h"
#include "misc/config.h"

#include <QSslError>
#include <KLocale>
#include <KIconLoader>

Q_DECLARE_METATYPE(QSslError)

using namespace KFTPCore;

namespace KFTPWidgets {

SslErrorsDialog::SslErrorsDialog(QWidget *parent)
  : KDialog(parent)
{
  setCaption(i18n("SSL Negotiation Failed"));
  setButtons(Cancel | Ok | User1);
  setButtonText(Ok, i18n("Continue"));
  setButtonText(User1, i18n("View Certificate..."));
  
  QWidget *widget = new QWidget(this);
  ui.setupUi(widget);
  setMainWidget(widget);
  
  ui.icon->setPixmap(DesktopIcon("decrypted"));
}

void SslErrorsDialog::setErrors(const QVariantList &errors)
{
  foreach (QVariant error, errors) {
    QSslError e = error.value<QSslError>();
    ui.errors->addItem(new QListWidgetItem(KIcon("no"), e.errorString()));
    m_certificate = e.certificate();
    
    switch (e.error()) {
      case QSslError::CertificateExpired: {
        ui.trustCertificate->setEnabled(false);
        break;
      }
      default: break;
    }
  }
}

void SslErrorsDialog::slotButtonClicked(int button)
{
  switch (button) {
    case Ok: {
      if (ui.trustCertificate->isChecked()) {
        // Save site certificate to trusted certificates
        CertificateStore *store = Config::self()->certificateStore();
        store->addCertificate(m_certificate);
        store->save();
      }
      
      accept();
      break;
    }
    case Cancel: reject(); break;
    case User1: {
      // Show certificate information dialog
      break;
    }
  }
}

}

#include "sslerrorsdialog.moc"
