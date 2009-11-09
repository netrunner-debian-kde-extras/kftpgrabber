/*
 * This file is part of the KFTPGrabber project
 *
 * Copyright (C) 2003-2007 by the KFTPGrabber developers
 * Copyright (C) 2003-2007 Jernej Kos <kostko@jweb-network.net>
 * Copyright (C) 2005 Markus Brueffer <markus@brueffer.de>
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

#include "queueview/queueview.h"
#include "queueview/treeview.h"
#include "queueview/model.h"

//Added by qt3to4:
#include <QLabel>
#include <Q3PtrList>
#include <Q3VBoxLayout>
#include "kftpqueue.h"
//#include "queueeditor.h"
//#include "widgets/searchdialog.h"
#include "misc/config.h"

#include <kapplication.h>
#include <kfiledialog.h>
#include <kiconloader.h>
#include <kio/job.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmimetype.h>
#include <kmenu.h>
#include <k3listviewsearchline.h>
#include <kdebug.h>

#include <QPixmap>
#include <QPixmapCache>

#include <qspinbox.h>
#include <qtooltip.h>
#include <kglobal.h>

#include <KToolBar>
#include <KActionCollection>
#include <KAction>

using namespace KFTPCore;
using namespace KFTPWidgets::Queue;


namespace KFTPWidgets {

QueueView::QueueView(QWidget *parent)
 : QWidget(parent)
{
  Q3VBoxLayout *layout = new Q3VBoxLayout(this);

  m_toolBar = new KToolBar(this, "queueToolBar");
  m_toolBar->setIconSize(QSize(16, 16));
  m_toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
  layout->addWidget(m_toolBar);
  
  /*
  m_searchToolBar = new KToolBar(this, "searchToolBar");
  m_searchToolBar->setContextMenuEnabled(false);
  m_searchToolBar->setMovable(false);*/
  //m_searchToolBar->setFullSize(true);

  // Create the erase button
  //m_searchToolBar->insertButton(QApplication::isRightToLeft() ? "clear_left" :"locationbar_erase", 0, SIGNAL(clicked()), this, SLOT(slotSearchEraseClicked()), true);
  
  // Create the labels
  /*
  QLabel *searchLabel = new QLabel(i18n("Filter: "), m_searchToolBar);
  m_searchToolBar->addWidget(searchLabel);*/
  /*1, 35, */
  Model *model = new Model(this);
  m_tree = new TreeView(this);
  m_tree->setModel(model);
  layout->addWidget(m_tree);
  
  // Create the search field
  //m_searchField = new K3ListViewSearchLine(m_searchToolBar, m_queue);
  
  // Do some more stuff
  //m_searchToolBar->setItemAutoSized(1, true);
  //m_searchToolBar->setStretchableWidget(m_searchField);
  //m_searchToolBar->updateRects(true);
  //m_searchToolBar->hide();
  
  //layout->addWidget(m_searchToolBar);

  
  // Load the listview layout
  loadLayout();
  
  // Create the context menu actions
  initActions();
  initToolBar();
  updateActions();
  
  setMinimumHeight(150);
}

void QueueView::saveLayout()
{
  //m_queue->saveLayout(KGlobal::config(), "queueViewLayout");
}

void QueueView::loadLayout()
{
  //m_queue->restoreLayout(KGlobal::config(), "queueViewLayout");
}

void QueueView::initToolBar()
{
  // Plug all actions
  m_toolBar->addAction(m_loadAction);
  m_toolBar->addAction(m_saveAction);
  m_toolBar->addSeparator();
  m_toolBar->addAction(m_startAction);
  m_toolBar->addAction(m_pauseAction);
  m_toolBar->addAction(m_stopAction);
  m_toolBar->addSeparator();
  m_toolBar->addAction(m_searchAction);
  m_toolBar->addAction(m_filterAction);
  
  // Create speed control widgets
  m_toolBar->addSeparator();
  
  QSpinBox *downloadSpeed = new QSpinBox(m_toolBar);
  downloadSpeed->setMinimum(0);
  downloadSpeed->setMaximum(10240);
  downloadSpeed->setToolTip(i18n("Limit download transfer speed"));
  m_toolBar->addWidget(new QLabel(i18n("Down: "), m_toolBar));
  m_toolBar->addWidget(downloadSpeed);
  downloadSpeed->setValue(Config::downloadSpeedLimit());
  connect(downloadSpeed, SIGNAL(valueChanged(int)), this, SLOT(slotDownloadLimitChanged(int)));
  
  m_toolBar->addSeparator();
  
  QSpinBox *uploadSpeed = new QSpinBox(m_toolBar);
  uploadSpeed->setMinimum(0);
  uploadSpeed->setMaximum(10240);
  uploadSpeed->setToolTip(i18n("Limit upload transfer speed"));
  m_toolBar->addWidget(new QLabel(i18n("Up: "), m_toolBar));
  m_toolBar->addWidget(uploadSpeed);
  uploadSpeed->setValue(Config::uploadSpeedLimit());
  connect(uploadSpeed, SIGNAL(valueChanged(int)), this, SLOT(slotUploadLimitChanged(int)));
  
  // Create thread count control widget
  m_toolBar->addSeparator();
  
  QSpinBox *threadCount = new QSpinBox(m_toolBar);
  threadCount->setMinimum(1);
  threadCount->setMaximum(10);
  threadCount->setToolTip(i18n("Per-session transfer thread count"));
  m_toolBar->addWidget(new QLabel(i18n("Threads: "), m_toolBar));
  m_toolBar->addWidget(threadCount);
  threadCount->setValue(Config::threadCount());
  connect(threadCount, SIGNAL(valueChanged(int)), this, SLOT(slotThreadCountChanged(int)));
}

void QueueView::slotDownloadLimitChanged(int value)
{
  Config::setDownloadSpeedLimit(value);
  Config::self()->emitChange();
}

void QueueView::slotUploadLimitChanged(int value)
{
  Config::setUploadSpeedLimit(value);
  Config::self()->emitChange();
}

void QueueView::slotThreadCountChanged(int value)
{
  Config::setThreadCount(value);
  Config::self()->emitChange();
}

void QueueView::initActions()
{
  KActionCollection *actions = new KActionCollection(this);

  // Create the toolbar actions
  m_loadAction = new KAction(this);
  m_loadAction->setText(i18n("&Load Queue From File"));
  m_loadAction->setIcon(KIcon("document-open"));
  connect(m_loadAction, SIGNAL(triggered()), this, SLOT(slotLoad()));
  
  m_saveAction = new KAction(this);
  m_saveAction->setText(i18n("&Save Queue to File"));
  m_saveAction->setIcon(KIcon("document-save-as"));
  connect(m_saveAction, SIGNAL(triggered()), this, SLOT(slotSave()));
  
  m_startAction = new KAction(this);
  m_startAction->setText(i18n("S&tart"));
  m_startAction->setIcon(KIcon("media-playback-start"));
  connect(m_startAction, SIGNAL(triggered()), this, SLOT(slotStart()));
  
  m_pauseAction = new KAction(this);
  m_pauseAction->setText(i18n("&Pause"));
  m_pauseAction->setIcon(KIcon("media-playback-pause"));
  connect(m_pauseAction, SIGNAL(triggered()), this, SLOT(slotPause()));
  
  m_stopAction = new KAction(this);
  m_stopAction->setText(i18n("St&op"));
  m_stopAction->setIcon(KIcon("media-playback-stop"));
  connect(m_stopAction, SIGNAL(triggered()), this, SLOT(slotStop()));
  
  m_searchAction = new KAction(this);
  m_searchAction->setText(i18n("&Search && Replace..."));
  m_searchAction->setIcon(KIcon("find"));
  connect(m_searchAction, SIGNAL(triggered()), this, SLOT(slotSearch()));
  
  m_filterAction = new KAction(this);
  m_filterAction->setText(i18n("Show &Filter"));
  m_filterAction->setIcon(KIcon("filter"));
  connect(m_filterAction, SIGNAL(triggered()), this, SLOT(slotFilter()));
    
  m_saveAction->setEnabled(false);
  m_startAction->setEnabled(false);
  m_pauseAction->setEnabled(false);
  m_stopAction->setEnabled(false);
  m_searchAction->setEnabled(false);
  m_filterAction->setEnabled(true);
}

void QueueView::updateActions()
{
  /*
  m_startAction->setEnabled(!KFTPQueue::Manager::self()->isProcessing() && KFTPQueue::Manager::self()->topLevelObject()->hasChildren() && !KFTPQueue::Manager::self()->getNumRunning());
  m_stopAction->setEnabled(KFTPQueue::Manager::self()->isProcessing());
  m_removeAllAction->setEnabled(!KFTPQueue::Manager::self()->isProcessing());
  
  Q3PtrList<Q3ListViewItem> selection = m_queue->selectedItems();
  QueueViewItem *firstItem = static_cast<QueueViewItem*>(selection.first());
  
  m_removeAction->setEnabled((bool) firstItem);
  
  if (!firstItem || !firstItem->getObject())
    return;
    
  bool locked = firstItem->getObject()->isLocked();
  bool parentRunning = false;
  
  if (firstItem->getObject()->hasParentObject())
    parentRunning = firstItem->getObject()->parentObject()->isRunning();
  
  m_launchAction->setEnabled(!firstItem->getObject()->isRunning() && !KFTPQueue::Manager::self()->isProcessing() && !locked);
  m_abortAction->setEnabled(firstItem->getObject()->isRunning() && !KFTPQueue::Manager::self()->isProcessing());
  m_removeAction->setEnabled(!firstItem->getObject()->isRunning() && !KFTPQueue::Manager::self()->isProcessing() && !locked);
  m_editAction->setEnabled(!firstItem->getObject()->isRunning() && firstItem->getObject()->parentObject()->getType() == KFTPQueue::QueueObject::Site && !locked);
  
  // Only allow moving of multi selections if they have the same parent
  bool allowMove = true;
  for (Q3ListViewItem *i = selection.first(); i; i = selection.next()) {
    if (i->parent() != static_cast<Q3ListViewItem*>(firstItem)->parent()) {
      allowMove = false;
      break;
    }
  }
  
  m_moveUpAction->setEnabled(allowMove && KFTPQueue::Manager::self()->canBeMovedUp(firstItem->getObject()) && !locked);
  m_moveDownAction->setEnabled(allowMove && KFTPQueue::Manager::self()->canBeMovedDown(static_cast<QueueViewItem*>(selection.last())->getObject()) && !locked);
  
  m_moveTopAction->setEnabled(allowMove && KFTPQueue::Manager::self()->canBeMovedUp(firstItem->getObject()) && !locked);
  m_moveBottomAction->setEnabled(allowMove && KFTPQueue::Manager::self()->canBeMovedDown(static_cast<QueueViewItem*>(selection.last())->getObject()) && !locked);
  */
}

void QueueView::slotSearch()
{
  /*
  SearchDialog *dialog = new SearchDialog();
  
  dialog->exec();
  delete dialog;
  */
}

void QueueView::slotLoad()
{
  /*
  if (m_queue->childCount() && KMessageBox::warningContinueCancel(0L, i18n("Loading a new queue will overwrite the existing one; are you sure you want to continue?"), i18n("Load Queue")) == KMessageBox::Cancel)
    return;

  QString loadPath = KFileDialog::getOpenFileName();
  
  if (!loadPath.isEmpty()) {
    KFTPQueue::Manager::self()->getConverter()->importQueue(loadPath);
  }
  */
}

void QueueView::slotSave()
{
  /*
  QString savePath = KFileDialog::getSaveFileName();
  
  if (!savePath.isEmpty()) {
    KFTPQueue::Manager::self()->getConverter()->exportQueue(savePath);
  }
  */
}

void QueueView::slotStart()
{
  // Begin queue processing
  KFTPQueue::Manager::self()->start();
}

void QueueView::slotPause()
{
}

void QueueView::slotStop()
{
  // Abort queue processing
  KFTPQueue::Manager::self()->abort();
}

void QueueView::slotAdd()
{
}

void QueueView::slotFilter()
{
/*
  if (m_filterAction->isChecked())
    m_searchToolBar->show();
  else
    m_searchToolBar->hide();*/
}

}

#include "queueview.moc"
