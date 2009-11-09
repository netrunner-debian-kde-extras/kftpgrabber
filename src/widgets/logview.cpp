/*
 * This file is part of the KFTPGrabber project
 *
 * Copyright (C) 2003-2008 by the KFTPGrabber developers
 * Copyright (C) 2003-2008 Jernej Kos <kostko@jweb-network.net>
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

#include "logview.h"
#include "misc/config.h"

#include <klocale.h>
#include <kstandardaction.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <KGlobal>
#include <KComponentData>
#include <KAboutData>

#include <QFile>
#include <QTextStream>
#include <QFont>
#include <QKeyEvent>
#include <QPainter>
#include <QScrollBar>
#include <QAbstractSlider>
#include <QMenu>

namespace KFTPWidgets {

LogView::LogView(QWidget *parent)
  : QPlainTextEdit(parent)
{
  setReadOnly(true);
  setMaximumBlockCount(200);
  setCenterOnScroll(true);
  setFrameStyle(QFrame::NoFrame);

  // Init actions
  m_saveToFileAction = KStandardAction::saveAs(this, SLOT(slotSaveToFile()), this);
  m_clearLogAction = KStandardAction::clear(this, SLOT(clear()), this);
}

LogView::~LogView()
{
}

void LogView::append(const QString &str, LineType type)
{
  switch (type) {
    case FtpResponse: {
      // Break response into code and text to format them differently
      QString prefix = str.section(" ", 0, 0);
      QString text = str.mid(str.indexOf(' '));

      appendHtml(QString("<font color='%1'><b>%2</b> %3</font><br/>").arg(KFTPCore::Config::logResponsesColor().name())
                                                                     .arg(prefix)
                                                                     .arg(text));
      break;
    }
    case FtpCommand: {
      // Hide password if this is a PASS command
      QString text = str;
      if (text.left(4) == "PASS")
        text = "PASS (hidden)";

      appendHtml(QString("<font color='%1'><b>%2</b></font><br/>").arg(KFTPCore::Config::logCommandsColor().name())
                                                                  .arg(text));
      break;
    }
    case FtpMultiline: {
      appendHtml(QString("<font color='%1'>%2</font><br/>").arg(KFTPCore::Config::logMultilineColor().name())
                                                           .arg(str));
      break;
    }
    case FtpStatus: {
      appendHtml(QString("<font color='%1'><b>*** %2</b></font><br/>").arg(KFTPCore::Config::logStatusColor().name())
                                                                      .arg(str));
      break;
    }
    case FtpError: {
      appendHtml(QString("<font color='%1'><b>*** %2</b></font><br/>").arg(KFTPCore::Config::logErrorColor().name())
                                                                      .arg(str));
      break;
    }
    case Plain: {
      appendHtml(QString("%1<br/>").arg(str));
      break;
    }
  }
}

void LogView::contextMenuEvent(QContextMenuEvent *event)
{
  QMenu menu;
  
  menu.addAction(m_saveToFileAction);
  menu.addSeparator();
  menu.addAction(m_clearLogAction);
  menu.exec(event->globalPos());
}

void LogView::slotSaveToFile()
{
  QString savePath = KFileDialog::getSaveFileName(KUrl(), "*.txt");
  
  if (!savePath.isEmpty()) {
    QFile file(savePath);

    if (file.open(QIODevice::WriteOnly)) {
      QTextStream stream(&file);
      stream << toPlainText();
      file.close();
    } else {
      KMessageBox::error(0L, i18n("Unable to open file for writing."));
    }
  }
}

}

#include "logview.moc"
