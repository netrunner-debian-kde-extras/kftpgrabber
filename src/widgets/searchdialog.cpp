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

#include "widgets/searchdialog.h"
#include "kftpsearchlayout.h"
#include "kftpserverlineedit.h"
#include "kftpbookmarks.h"
#include "kftpqueue.h"

#include <qcheckbox.h>
#include <q3groupbox.h>
#include <qspinbox.h>
#include <qregexp.h>
//Added by qt3to4:
#include <Q3PtrList>

#include <klocale.h>
#include <klineedit.h>
#include <kpassworddialog.h>

namespace KFTPWidgets {

SearchDialog::SearchDialog(QWidget *parent, const char *name)
 : KDialogBase(parent, name, true, i18n("Search & Replace"), Ok|Cancel, Ok)
{
  // Create the main widget
  m_layout = new KFTPSearchLayout(this);
  
  // Set the dialog options
  setMainWidget(m_layout);
  setInitialSize(QSize(500,400));
  
  connect(m_layout->searchServer, SIGNAL(clicked()), this, SLOT(slotSearchServerClicked()));
  connect(m_layout->searchServerName, SIGNAL(siteChanged(KFTPBookmarks::Site*)), this, SLOT(slotSiteChanged(KFTPBookmarks::Site*)));
}

QString SearchDialog::replaceCap(QStringList cap, const QString &text)
{
  QString tmp = text;
  
  QStringList::Iterator end( cap.end() );
  for(QStringList::Iterator i( cap.begin() ); i != end; ++i) {
    tmp.replace("$" + QString::number(cap.findIndex(*i)), *i);
  }
  
  return tmp;
}

void SearchDialog::replace(KFTPQueue::Transfer *i)
{
  QRegExp s, d;
  
  s.setPattern(m_layout->searchSrcPath->text());
  d.setPattern(m_layout->searchDstPath->text());
  
  KUrl tmp = i->getSourceUrl().isLocalFile() ? i->getDestUrl() : i->getSourceUrl();
  tmp.setPath("/");
  
  KUrl match;
  match.setProtocol("ftp");
  match.setHost(m_layout->searchServerHost->text());
  match.setPort(m_layout->searchServerPort->value());
  match.setUser(m_layout->searchServerUser->text());
  match.setPass(m_layout->searchServerPass->password());
  match.setPath("/");
  
  if (s.search(i->getSourceUrl().path()) != -1 && d.search(i->getDestUrl().path()) != -1 &&
      (!m_layout->searchServer->isChecked() || tmp.url() == match.url())) {
    // Do the replacing
    KUrl newSource = i->getSourceUrl();
    KUrl newDest = i->getDestUrl();
    
    newSource.setPath(replaceCap(s.capturedTexts(), m_layout->replaceSrcPath->text()));
    newDest.setPath(replaceCap(d.capturedTexts(), m_layout->replaceDstPath->text()));
    
    i->setSourceUrl(newSource);
    i->setDestUrl(newDest);
    
    i->emitUpdate();
  }
}

void SearchDialog::searchAndReplace(KFTPQueue::QueueObject *parent)
{
  if (parent->isLocked())
    return;
    
  Q3PtrList<KFTPQueue::QueueObject> list = parent->getChildrenList();
  
  KFTPQueue::QueueObject *i;
  for (i = list.first(); i; i = list.next()) {
    if (i->hasChildren() && !i->isLocked()) {
      searchAndReplace(i);
    }
    
    if (i->isTransfer() && !i->isLocked())
      replace(static_cast<KFTPQueue::Transfer*>(i));
  }
}

void SearchDialog::searchAndReplace()
{
  searchAndReplace(KFTPQueue::Manager::self()->topLevelObject());
}

void SearchDialog::slotOk()
{
  searchAndReplace();
  accept();
}

void SearchDialog::slotSearchServerClicked()
{
  m_layout->groupBox1->setEnabled(m_layout->searchServer->isChecked());
}

void SearchDialog::slotSiteChanged(KFTPBookmarks::Site *site)
{
  if (site) {
    m_layout->searchServerHost->setText(site->getProperty("host"));
    m_layout->searchServerPort->setValue(site->getIntProperty("port"));
    m_layout->searchServerUser->setText(site->getProperty("username"));
    m_layout->searchServerPass->erase();
    m_layout->searchServerPass->insert(site->getProperty("password"));
  } else {
    m_layout->searchServerHost->clear();
    m_layout->searchServerPort->setValue(21);
    m_layout->searchServerUser->clear();
    m_layout->searchServerPass->erase();
  }
}

}

#include "searchdialog.moc"
