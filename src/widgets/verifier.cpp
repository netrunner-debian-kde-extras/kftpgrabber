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

#include "verifier.h"
#include "listviewitem.h"

#include <klocale.h>
#include <k3listview.h>
#include <kmessagebox.h>
#include <kurl.h>

#include <q3header.h>
#include <qlabel.h>

namespace KFTPWidgets {

Verifier::Verifier(QWidget *parent)
 : KDialog(parent),
   m_verifier(0)
{
  //, name, true, i18n("Checksum verifier"), Cancel, Cancel, true
  QWidget *widget = new QWidget(this);
  ui.setupUi(widget);
  setMainWidget(widget);
  
  // Create columns
  ui.fileList->addColumn(i18n("Filename"));
  ui.fileList->addColumn(i18n("Checksum"), 100);
  
  ui.fileList->setAllColumnsShowFocus(true);
  ui.fileList->header()->setStretchEnabled(true, 0);
}

Verifier::~Verifier()
{
  delete m_verifier;
}

void Verifier::setFile(const QString &filename)
{
  // Create the verifier
  m_verifier = new KFTPCore::ChecksumVerifier(filename);
  ui.currentFile->setText(KUrl(filename).fileName());
  
  connect(m_verifier, SIGNAL(fileList(QList<QPair<QString, QString> >)), this, SLOT(slotHaveFileList(QList<QPair<QString, QString> >)));
  connect(m_verifier, SIGNAL(fileDone(const QString&, KFTPCore::ChecksumVerifier::Result)), this, SLOT(slotFileDone(const QString&, KFTPCore::ChecksumVerifier::Result)));
  connect(m_verifier, SIGNAL(progress(int)), this, SLOT(slotProgress(int)));
  connect(m_verifier, SIGNAL(error()), this, SLOT(slotError()));
  
  // Start the verification
  m_verifier->verify();
}

void Verifier::slotHaveFileList(QList<QPair<QString, QString> > list)
{
  for (QList<QPair<QString, QString> >::iterator i = list.begin(); i != list.end(); ++i) {
    KFTPWidgets::ListViewItem *item = new KFTPWidgets::ListViewItem(ui.fileList);
    item->setText(0, (*i).first);
    item->setText(1, (*i).second);
  }
}

void Verifier::slotFileDone(const QString &filename, KFTPCore::ChecksumVerifier::Result result)
{
  KFTPWidgets::ListViewItem *item = static_cast<KFTPWidgets::ListViewItem*>(ui.fileList->findItem(filename, 0));
  
  if (item) {
    switch (result) {
      case KFTPCore::ChecksumVerifier::Ok: {
        //item->setPixmap(0, loadSmallPixmap("dialog-ok"));
        // "ok" icon should probably be "dialog-success", but we don't have that icon in KDE 4.0
        item->setTextColor(0, QColor(0, 200, 0));
        item->setTextColor(1, QColor(0, 200, 0));
        break;
      }
      case KFTPCore::ChecksumVerifier::NotFound: {
        //item->setPixmap(0, loadSmallPixmap("dialog-error"));
        item->setTextColor(0, QColor(128, 128, 128));
        item->setTextColor(1, QColor(128, 128, 128));
        break;
      }
      case KFTPCore::ChecksumVerifier::Error: {
        //item->setPixmap(0, loadSmallPixmap("dialog-error"));
        item->setTextColor(0, QColor(255, 0, 0));
        item->setTextColor(1, QColor(255, 0, 0));
        break;
      }
    }
    
    ui.fileList->ensureItemVisible(item);
  }
}

void Verifier::slotProgress(int percent)
{
  ui.checkProgress->setValue(percent);
  
  if (percent == 100) {
    KMessageBox::information(0, i18n("Verification complete."));
  }
}

void Verifier::slotError()
{
  KMessageBox::error(0, i18n("Unable to open checksum file, or file has an incorrect format."));
  close();
}

}
#include "verifier.moc"
