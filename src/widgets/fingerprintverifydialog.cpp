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
#include "fingerprintverifydialog.h"
#include "misc/config.h"

#include <KLocale>
#include <KIconLoader>

using namespace KFTPCore;

namespace KFTPWidgets {

FingerprintVerifyDialog::FingerprintVerifyDialog(QWidget *parent)
  : KDialog(parent)
{
  setCaption(i18n("Fingerprint Verification"));
  setButtons(Cancel | Ok);
  setButtonText(Ok, i18n("Continue"));
  
  QWidget *widget = new QWidget(this);
  ui.setupUi(widget);
  setMainWidget(widget);
  resize(QSize(440, 170));
  
  ui.icon->setPixmap(DesktopIcon("decrypted"));
}

void FingerprintVerifyDialog::setFingerprint(const QByteArray &fingerprint, const KUrl &url)
{
  QByteArray f = fingerprint.toHex();
  
  for (int i = 2; i < f.size(); i += 2) {
    f.insert(i++, ':');
  }
  
  ui.fingerprint->setText(QString("<b>%1</b>").arg(QString(f)));
  m_url = url;
  m_fingerprint = fingerprint;
}

void FingerprintVerifyDialog::slotButtonClicked(int button)
{
  switch (button) {
    case Ok: {
      // Save the fingerprint
      CertificateStore *store = Config::self()->certificateStore();
      store->addFingerprint(m_url, m_fingerprint);
      store->save();
      
      accept();
      break;
    }
    case Cancel: reject(); break;
  }
}

}

#include "fingerprintverifydialog.moc"
