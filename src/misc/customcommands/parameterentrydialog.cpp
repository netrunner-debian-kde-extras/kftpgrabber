/*
 * This file is part of the KFTPGrabber project
 *
 * Copyright (C) 2003-2006 by the KFTPGrabber developers
 * Copyright (C) 2003-2006 Jernej Kos <kostko@jweb-network.net>
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
#include "parameterentrydialog.h"
#include "entry.h"

#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QList>
#include <QFrame>

#include <KIconLoader>
#include <KLineEdit>
#include <KNumInput>

namespace KFTPCore {

namespace CustomCommands {

ParameterEntryDialog::ParameterEntryDialog(Entry *entry, QList<Entry::Parameter> params)
  : KDialog(0),
    m_params(params)
{
  setCaption(entry->name());
  setModal(true);
  setButtons(Ok | Cancel);
  
  QFrame *mainWidget = new QFrame(this);
  QVBoxLayout *mainLayout = new QVBoxLayout(mainWidget);
  setMainWidget(mainWidget);
  
  QHBoxLayout *headerLayout = new QHBoxLayout(mainWidget);
  QLabel *icon = new QLabel(mainWidget);
  icon->setPixmap(DesktopIcon(entry->icon(), 32));
  headerLayout->addWidget(icon);
  
  QVBoxLayout *headerTextLayout = new QVBoxLayout(mainWidget);
  headerTextLayout->addWidget(new QLabel(QString("<b>%1</b>").arg(entry->name()), mainWidget));
  headerTextLayout->addWidget(new QLabel(entry->description(), mainWidget));
  headerLayout->addLayout(headerTextLayout, 1);
  
  mainLayout->addLayout(headerLayout);
  mainLayout->addSpacing(5);
  
  QFrame *frame = new QFrame(mainWidget);
  frame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
  
  QVBoxLayout *frameLayout = new QVBoxLayout(frame);
  frameLayout->setMargin(10);
  mainLayout->addWidget(frame);
  
  int num = 0;
  QList<Entry::Parameter>::ConstIterator lend = params.constEnd();
  for (QList<Entry::Parameter>::ConstIterator i = params.constBegin(); i != lend; ++i) {
    QHBoxLayout *layout = new QHBoxLayout(frame);
    QWidget *entryWidget = 0;
    QString name = QString("param_%1").arg(num++);
    
    switch ((*i).type()) {
      case Entry::String: entryWidget = new KLineEdit(frame); break;
      case Entry::Password: {
        entryWidget = new KLineEdit(frame);
        static_cast<KLineEdit*>(entryWidget)->setPasswordMode(true);
        break;
      }
      case Entry::Integer: entryWidget = new KIntNumInput(frame); break;
    }
    
    entryWidget->setObjectName(name);
    
    // The first widget should have focus
    if (num == 1)
      entryWidget->setFocus();
    
    layout->addWidget(new QLabel((*i).name() + ":", frame));
    layout->addStretch(1);
    layout->addWidget(entryWidget);
    frameLayout->addLayout(layout);
    frameLayout->addSpacing(5);
  }
  
  setMaximumWidth(350);
  resize(350, minimumHeight());
}

QString ParameterEntryDialog::formatCommand(const QString &command)
{
  QString tmp = command;
  
  int num = 0;
  QList<Entry::Parameter>::ConstIterator lend = m_params.constEnd();
  for (QList<Entry::Parameter>::ConstIterator i = m_params.constBegin(); i != lend; ++i) {
    QObject *entryWidget = findChild<QObject*>(QString("param_%1").arg(num++));
    
    switch ((*i).type()) {
      case Entry::String: tmp = tmp.arg(static_cast<KLineEdit*>(entryWidget)->text()); break;
      case Entry::Password: tmp = tmp.arg(static_cast<KLineEdit*>(entryWidget)->text()); break;
      case Entry::Integer: tmp = tmp.arg(static_cast<KIntNumInput*>(entryWidget)->value()); break;
    }
  }
  
  return tmp;
}

}

}

#include "parameterentrydialog.moc"

