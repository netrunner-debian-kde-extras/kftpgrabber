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
#include "kftpserverlineedit.h"
#include "kftpselectserverdialog.h"

#include <kdialog.h>
#include <kpushbutton.h>
#include <klocale.h>

#include <qlayout.h>
//Added by qt3to4:
#include <Q3HBoxLayout>

KFTPServerLineEdit::KFTPServerLineEdit(QWidget *parent, const char *name, Qt::WFlags f)
 : QWidget(parent, name, f), m_currentSite(0)
{
  Q3HBoxLayout *layout = new Q3HBoxLayout(this, 0, KDialog::spacingHint());
  
  m_lineEdit = new KLineEdit(this);
  m_lineEdit->setReadOnly(true);

  KPushButton *selectButton = new KPushButton(i18n("Select..."), this);
  selectButton->setFlat(true);
  connect(selectButton, SIGNAL(clicked()), this, SLOT(slotSelectButtonClicked()));

  layout->addWidget(m_lineEdit);
  layout->addWidget(selectButton);
}

KFTPServerLineEdit::~KFTPServerLineEdit()
{
}

void KFTPServerLineEdit::setCurrentSite(KFTPBookmarks::Site *site)
{
  if (site) {
    m_currentSite = site;
    m_lineEdit->setText(m_currentSite->getAttribute("name"));
    
    emit siteChanged(m_currentSite);
  } else {
    m_currentSite = 0L;
    
    clear();
  }
}

void KFTPServerLineEdit::clear()
{
	m_lineEdit->clear();
}

void KFTPServerLineEdit::slotSelectButtonClicked()
{
  KFTPSelectServerDialog *dialog = new KFTPSelectServerDialog(this);
  
  if (dialog->exec() == QDialog::Accepted) {
    m_currentSite = dialog->getSelectedSite();
    
    if (m_currentSite)
      m_lineEdit->setText(m_currentSite->getAttribute("name"));
    else
      m_lineEdit->setText(i18n("No name"));
    
    emit siteChanged(m_currentSite);
  }
    
  delete dialog;
}



#include "kftpserverlineedit.moc"
