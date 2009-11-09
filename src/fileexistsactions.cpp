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

#include "fileexistsactions.h"

#include <QGroupBox>
#include <QLabel>
#include <QGridLayout>

#include <KDialog>
#include <KComboBox>
#include <KLocale>

namespace KFTPQueue {

QString &operator<<(QString &s, const FileExistsActions &a)
{
  s.truncate(0);
  
  ActionMap::ConstIterator end( a.m_actions.end() );
  for (ActionMap::ConstIterator i(a.m_actions.begin()); i != end; ++i) {
    s.append(QString("%1;").arg(i.value()));
  }
  
  return s;
}

QString &operator>>(QString &s, FileExistsActions &a)
{
  for (unsigned int i = 0; i < 9; i++) {
    a.m_actions[i+1] = static_cast<FEAction>(s.section(';', i, i).toInt());
  }
  
  return s;
}

QWidget *FileExistsActions::getConfigWidget(QWidget *parent)
{
  QGroupBox *gb = new QGroupBox(i18n("On File Exists Actions (%1)", m_type), parent);
  QGridLayout *gl = new QGridLayout(gb);
  const int m = KDialog::marginHint();
  gl->setSpacing(KDialog::spacingHint());
  gl->setContentsMargins(m, m, m, m);
  
  QLabel *l = new QLabel(i18n("Size/Timestamp"), gb);
  gl->addWidget(l, 1, 0);
  
  l = new QLabel(i18n("Same"), gb);
  gl->addWidget(l, 1, 1);
  
  l = new QLabel(i18n("Older"), gb);
  gl->addWidget(l, 1, 2);
  
  l = new QLabel(i18n("Newer"), gb);
  gl->addWidget(l, 1, 3);
  
  l = new QLabel(i18n("Same"), gb);
  gl->addWidget(l, 2, 0);
  
  l = new QLabel(i18n("Smaller"), gb);
  gl->addWidget(l, 3, 0);
  
  l = new QLabel(i18n("Bigger"), gb);
  gl->addWidget(l, 4, 0);
  
  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 3; col++) {
      KComboBox *cb = new KComboBox(gb);
      m_combos[row][col] = cb;
      
      cb->addItem(i18n("Skip"));
      cb->addItem(i18n("Overwrite"));
      cb->addItem(i18n("Resume"));
      cb->addItem(i18n("Rename"));
      cb->addItem(i18n("Ask"));
      cb->setCurrentIndex(m_actions[row * 3 + col + 1]);
      
      gl->addWidget(cb, row+2, col+1);
    }
  }
  
  return gb;
}

void FileExistsActions::updateWidget()
{
  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 3; col++) {
      m_combos[row][col]->setCurrentIndex(m_actions[row * 3 + col + 1]);
    }
  }
}

void FileExistsActions::updateConfig()
{
  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 3; col++) {
      m_actions[row * 3 + col + 1] = static_cast<FEAction>(m_combos[row][col]->currentIndex());
    }
  }
}

FEAction FileExistsActions::getActionForSituation(filesize_t src_fileSize, time_t src_fileTimestamp,
                                                  filesize_t dst_fileSize, time_t dst_fileTimestamp)
{
  // There are 9 different scenarios
  int situation = -1;
  
  if (dst_fileTimestamp == src_fileTimestamp) {
    // SAME TIMESTAMP
    situation = 1;
  } else if (dst_fileTimestamp < src_fileTimestamp) {
    // OLDER
    situation = 2;
  } else {
    // NEWER
    situation = 3;
  }
  
  if (dst_fileSize < src_fileSize) {
    // SMALLER FILE
    situation += 3;
  } else if (dst_fileSize > src_fileSize) {
    // BIGGER FILE
    situation += 6;
  }
  
  // Situation calculated, now get the action
  return m_actions[situation];
}

}
