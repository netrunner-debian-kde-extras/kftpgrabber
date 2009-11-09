/*
 * This file is part of the KFTPGrabber project
 *
 * Copyright (C) 2004 by the KFTPGrabber developers
 * Copyright (C) 2004 Jernej Kos <kostko@jweb-network.net>
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

#include "browser/propsplugin.h"
#include "kftpsession.h"

#include <qlayout.h>
#include <qlabel.h>
#include <qstyle.h>
#include <q3groupbox.h>
//Added by qt3to4:
#include <Q3GridLayout>
#include <Q3VBoxLayout>

#include <klocale.h>
#include <kiconloader.h>
#include <kseparator.h>
#include <ksqueezedtextlabel.h>

using namespace KFTPEngine;

namespace KFTPWidgets {

namespace Browser {

PropsPlugin::PropsPlugin(KPropertiesDialog *props, KFileItemList items)
 : KPropsDlgPlugin(props)
{
  QFrame *frame = properties->addPage(i18n("&General"));
  frame->setMinimumWidth(320);
  frame->setMinimumHeight(300);
  
  // Some differences between a single file and multiple files
  KFileItem item = items.first();
  KUrl fileUrl = item.url();
  filesize_t fileSize = item.size();
  QString nameText;
  QString iconText;
  QString mimeComment;
  
  if (items.count() == 1) {
    bool isDir = false;
    
    // Guess file type
    if (item.isDir()) {
      iconText = "folder";
      isDir = true;
      mimeComment = i18n("Remote folder");
    } else if (item.isLink()) {
      // We can't know if the sym-linked file is realy a directory, but most of
      // the time it is. So if we can't determine the MIME type, set it to directory.
      KMimeType::Ptr mimeType = KMimeType::findByURL(fileUrl, 0, false, true);
      
      if (mimeType->name() == KMimeType::defaultMimeType()) {
        iconText = "folder";
        isDir = true;
        mimeComment = i18n("Remote folder");
      } else {
        iconText = mimeType->icon(QString::null, false);
        mimeComment = mimeType->comment();
      }
    } else {
      KMimeType::Ptr mimeType = KMimeType::findByURL(fileUrl, 0, false, true);
      iconText = mimeType->icon(QString::null, false);
      mimeComment = mimeType->comment();
    }
    
    if (mimeComment.isEmpty()) {
      mimeComment = i18n("Unknown");
    }
    
    nameText = item.name();
  } else {
    // Count files and folders selected
    int countFiles = 0;
    int countFolders = 0;
    fileSize = 0;
    
    foreach(const KFileItem& item, items) {
      if (item.isDir())
        countFolders++;
      else
        countFiles++;
      
      fileSize += item.size();
    }
    
    iconText = "kmultiple";
    nameText = KIO::itemsSummaryString(countFiles + countFolders, countFiles, countFolders, 0, false);
  }
  
  Q3VBoxLayout *vbl = new Q3VBoxLayout(frame, 0, KDialog::spacingHint(), "vbl");
  Q3GridLayout *grid = new Q3GridLayout(0, 3);
  grid->setColStretch(0, 0);
  grid->setColStretch(1, 0);
  grid->setColStretch(2, 1);
  grid->addColSpacing(1, KDialog::spacingHint());
  vbl->addLayout(grid);
  
  // Display file name and icon
  QLabel *iconLabel = new QLabel(frame);
  int bsize = 66 + 2 * iconLabel->style().pixelMetric(QStyle::PM_ButtonMargin);
  iconLabel->setFixedSize(bsize, bsize);
  iconLabel->setPixmap(DesktopIcon(iconText));
  grid->addWidget(iconLabel, 0, 0, Qt::AlignLeft);
  
  QLabel *nameLabel = new QLabel(frame);
  nameLabel->setText(nameText);
  grid->addWidget(nameLabel, 0, 2);
  
  KSeparator *sep = new KSeparator(Qt::Horizontal, frame);
  grid->addMultiCellWidget(sep, 2, 2, 0, 2);
  
  // Display file information
  QLabel *l;
  int currentRow = 3;
  
  if (items.count() == 1) {
    l = new QLabel(i18n("Type:"), frame);
    grid->addWidget(l, currentRow, 0);
    
    l = new QLabel(mimeComment, frame);
    grid->addWidget(l, currentRow++, 2);
  }
  
  l = new QLabel(i18n("Location:"), frame);
  grid->addWidget(l, currentRow, 0);
  
  l = new KSqueezedTextLabel(frame);
  l->setText(fileUrl.directory());
  grid->addWidget(l, currentRow++, 2);
  
  l = new QLabel(i18n("Size:"), frame);
  grid->addWidget(l, currentRow, 0);
  
  l = new QLabel(frame);
  grid->addWidget(l, currentRow++, 2);
  
  l->setText(QString::fromLatin1("%1 (%2)").arg(KIO::convertSize(fileSize))
             .arg(KGlobal::locale()->formatNumber(fileSize, 0)));
  
  sep = new KSeparator(Qt::Horizontal, frame);
  grid->addMultiCellWidget(sep, currentRow, currentRow, 0, 2);
  currentRow++;
  
  // Display modification time
  if (items.count() == 1) {
    l = new QLabel(i18n("Created:"), frame);
    grid->addWidget(l, currentRow, 0);
    
    QDateTime dt;
    dt.setTime_t(item.time(KIO::UDS_MODIFICATION_TIME));
    l = new QLabel(KGlobal::locale()->formatDateTime(dt), frame);
    grid->addWidget(l, currentRow++, 2);
  }
  
  vbl->addStretch(1);
}

void PropsPlugin::applyChanges()
{
}

mode_t PermissionsPropsPlugin::fperm[3][4] = {
  {S_IRUSR, S_IWUSR, S_IXUSR, S_ISUID},
  {S_IRGRP, S_IWGRP, S_IXGRP, S_ISGID},
  {S_IROTH, S_IWOTH, S_IXOTH, S_ISVTX}
};

PermissionsPropsPlugin::PermissionsPropsPlugin(KPropertiesDialog *_props, KFileItemList items, KFTPSession::Session *session)
  : KPropsDlgPlugin(_props),
    m_items(items),
    m_session(session),
    m_cbRecursive(0)
{
  QFrame *frame = properties->addPage(i18n("&Permissions"));
  frame->setMinimumWidth(320);
  frame->setMinimumHeight(300);
  
  // Some differences between a single file and multiple files
  KFileItem *item = items.at(0);
  KUrl fileUrl = item.url();
  bool isDir = false;
  
  if (items.count() == 1) {
    // Guess file type
    if (item.isDir()) {
      isDir = true;
    } else if (item.isLink()) {
      // We can't know if the sym-linked file is realy a directory, but most of
      // the time it is. So if we can't determine the MIME type, set it to directory.
      KMimeType::Ptr mimeType = KMimeType::findByURL(fileUrl, 0, false, true);
      
      if (mimeType->name() == KMimeType::defaultMimeType())
        isDir = true;
    }
  } else {
    // Check for directories
    KFileItemListIterator i(items);
    for (; i.current(); ++i) {
      if ((*i)->isDir()) {
        isDir = true;
        break;
      }
    }
  }
  
  Q3BoxLayout *box = new Q3VBoxLayout(frame, 0, KDialog::spacingHint());
  
  Q3GroupBox *gb = new Q3GroupBox(0, Qt::Vertical, i18n("Access Permissions"), frame);
  gb->layout()->setSpacing(KDialog::spacingHint());
  gb->layout()->setMargin(KDialog::marginHint());
  box->addWidget(gb);
  
  Q3GridLayout *gl = new Q3GridLayout(gb->layout(), 6, 6, 15);
  
  QLabel *l = new QLabel(i18n("Class"), gb);
  gl->addWidget(l, 1, 0);
  
  if (isDir)
    l = new QLabel(i18n("Show\nEntries"), gb);
  else
    l = new QLabel(i18n("Read"), gb);
  gl->addWidget(l, 1, 1);
  
  if (isDir)
    l = new QLabel(i18n("Write\nEntries"), gb);
  else
    l = new QLabel(i18n("Write"), gb);
  gl->addWidget(l, 1, 2);
  
  if (isDir)
    l = new QLabel(i18n("Enter folder", "Enter"), gb);
  else
    l = new QLabel(i18n("Exec"), gb);
  
  QSize size = l->sizeHint();
  size.setWidth(size.width() + 15);
  l->setFixedSize(size);
  gl->addWidget(l, 1, 3);
  
  l = new QLabel(i18n("Special"), gb);
  gl->addMultiCellWidget(l, 1, 1, 4, 5);
  
  l = new QLabel(i18n("User"), gb);
  gl->addWidget(l, 2, 0);
  
  l = new QLabel(i18n("Group"), gb);
  gl->addWidget(l, 3, 0);
  
  l = new QLabel(i18n("Others"), gb);
  gl->addWidget(l, 4, 0);
  
  l = new QLabel(i18n("Set UID"), gb);
  gl->addWidget(l, 2, 5);
  
  l = new QLabel(i18n("Set GID"), gb);
  gl->addWidget(l, 3, 5);
  
  l = new QLabel(i18n("Sticky"), gb);
  gl->addWidget(l, 4, 5);
  
  mode_t permissions = item.permissions();
  
  // Display checkboxes
  for (int row = 0; row < 3; ++row) {
    for (int col = 0; col < 4; ++col) {
      QCheckBox *cb = new QCheckBox(gb);
      connect(cb, SIGNAL(clicked()), this, SLOT(setDirty()));
      m_permsCheck[row][col] = cb;
      cb->setChecked(permissions & fperm[row][col]);
      
      gl->addWidget(cb, row + 2, col + 1);
    }
  }
  
  gl->setColStretch(6, 10);
  box->addStretch(10);
  
  if (isDir) {
    m_cbRecursive = new QCheckBox(i18n("Apply changes to all subfolders and their contents"), frame);
    connect(m_cbRecursive, SIGNAL(clicked()), this, SLOT(changed()));
    box->addWidget(m_cbRecursive);
  }
}

void PermissionsPropsPlugin::applyChanges()
{
  // Generate new permissions =)
  int newPerms[4] = {0,};
  
  for (int row = 0; row < 3; ++row) {
    for (int col = 0; col < 4; ++col) {
      if (!m_permsCheck[row][col]->isChecked()) continue;
      
      int x = col < 3 ? col : row;
      int c = 0;
      
      switch (x) {
        case 0: c = 4; break;
        case 1: c = 2; break;
        case 2: c = 1; break;
      }
      
      if (col < 3) {
        newPerms[row + 1] += c;
      } else {
        newPerms[0] += c;
      }
    }
  }
  
  // Actually do a chmod
  int mode = newPerms[0] * 1000 + newPerms[1] * 100 + newPerms[2] * 10 + newPerms[3];
  bool recursive = m_cbRecursive && m_cbRecursive->isChecked();
  
  KFileItemListIterator i(m_items);
  for (; i.current(); ++i) {
    if ((*i)->isDir())
      m_session->getClient()->chmod((*i)->url(), mode, recursive);
    else
      m_session->getClient()->chmod((*i)->url(), mode);
  }
}

}

}

#include "propsplugin.moc"
