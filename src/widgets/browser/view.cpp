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

#include "browser/view.h"
#include "browser/detailsview.h"
#include "browser/treeview.h"
#include "browser/actions.h"
//#include "browser/filterwidget.h"
#include "browser/dirlister.h"
#include "browser/dirmodel.h"
#include "browser/dirsortfilterproxymodel.h"

#include "widgets/popupmessage.h"

#include "kftpbookmarks.h"
#include "misc/config.h"
#include "kftpsession.h"

#include "engine/ftpsocket.h"

#include <KLocale>
#include <KToolBar>
#include <KStatusBar>
#include <KComboBox>
#include <KAction>
#include <KMessageBox>
#include <KLineEdit>
#include <KPixmapProvider>
#include <KHistoryComboBox>
#include <KFileItem>
#include <KFileItemDelegate>

#include <kio/job.h>

#include <QLabel>
#include <QMenu>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollBar>

using namespace KFTPEngine;

namespace KFTPWidgets {

namespace Browser {

class HistoryPixmapProvider : public KPixmapProvider {
public:
    QPixmap pixmapFor(const QString &text, int size = 0)
    {
      Q_UNUSED(text)
      Q_UNUSED(size)

      return KIcon("folder").pixmap(16, 16);
    }
};

View::View(QWidget *parent, KFTPEngine::Thread *client, KFTPSession::Session *session)
 : QWidget(parent),
   m_session(session),
   m_ftpClient(client),
   m_freezeUrlUpdates(false),
   m_treeVisibilityChanged(false)
{
  m_connTimer = new QTimer(this);

  // Create the GUI
  init();
  populateToolbar();
  
  // Let us be up to date with bookmark changes
  connect(KFTPBookmarks::Manager::self(), SIGNAL(update()), this, SLOT(updateBookmarks()));
  
  // Some other stuff
  connect(m_ftpClient->eventHandler(), SIGNAL(engineEvent(KFTPEngine::Event*)), this, SLOT(slotEngineEvent(KFTPEngine::Event*)));
  connect(m_connTimer, SIGNAL(timeout()), this, SLOT(slotDurationUpdate()));
  
  // Config updates to hide/show the tree
  connect(KFTPCore::Config::self(), SIGNAL(configChanged()), this, SLOT(slotConfigUpdate()));
  slotConfigUpdate();
}

View::~View()
{
}

void View::init()
{
  // Init actions
  m_actions = new Actions(this);
  m_actions->initActions();

  // Layout
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setMargin(0);
  layout->setSpacing(0);

  // Create the toolbars
  m_toolBarFirst = new KToolBar(this, false, false);
  m_toolBarSecond = new KToolBar(this, false, false);
  //m_searchToolBar = new KToolBar(this, false, false);
  
  m_toolBarFirst->setContextMenuPolicy(Qt::NoContextMenu);
  m_toolBarFirst->setMovable(false);
  //m_toolBarFirst->setFullSize(true);

  m_toolBarSecond->setContextMenuPolicy(Qt::NoContextMenu);
  m_toolBarSecond->setMovable(false);
  //m_toolBarSecond->setFullSize(true);
  
  /*
  m_searchToolBar->setContextMenuEnabled(false);
  m_searchToolBar->setMovable(false);
  //m_searchToolBar->setFullSize(true);
  
  QLabel *filterLabel = new QLabel(i18n("Filter: "), m_searchToolBar);
  m_searchToolBar->insertWidget(1, 35, filterLabel);
  */
  
  // Create the labels
  QLabel *pathLabel = new QLabel(i18n("Path: "), m_toolBarSecond);
  m_toolBarSecond->addWidget(pathLabel);

  // Create the history combo
  m_historyCombo = new KHistoryComboBox(true, m_toolBarSecond);
  m_toolBarSecond->addWidget(m_historyCombo);
  m_historyCombo->setPixmapProvider(new HistoryPixmapProvider());
  m_historyCombo->setMaxCount(25);
  m_historyCombo->setMaxVisibleItems(25);
  m_historyCombo->setDuplicatesEnabled(false);
  m_historyCombo->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
  
  connect(m_historyCombo, SIGNAL(activated(const QString&)), this, SLOT(slotHistoryActivated(const QString&)));
  connect(m_historyCombo->lineEdit(), SIGNAL(returnPressed()), this, SLOT(slotHistoryActivated()));

  // Create a splitter
  m_splitter = new QSplitter(this);
  m_splitter->setOpaqueResize(true);
  m_splitter->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

  // Create a status bar
  QHBoxLayout *statusLayout = new QHBoxLayout(this);
  statusLayout->setMargin(0);

  m_connDurationMsg = new QLabel(this);
  m_connDurationMsg->setAlignment(Qt::AlignCenter);
  m_connDurationMsg->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
  m_connDurationMsg->setMinimumWidth(100);

  m_statusIcon = new QPushButton(this);
  m_statusIcon->setFlat(true);
  m_statusIcon->setIcon(KIcon("object-locked"));
  m_statusIcon->setEnabled(false);

  connect(m_statusIcon, SIGNAL(clicked()), this, SLOT(slotDisplayCertInfo()));

  m_statusMsg = new QLabel(this);
  m_statusMsg->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
  m_statusMsg->setText(i18n("Idle."));

  statusLayout->addWidget(m_statusMsg, 1);
  statusLayout->addWidget(m_connDurationMsg);
  statusLayout->addWidget(m_statusIcon);

  // Add toolbars to the layout
  layout->addWidget(m_toolBarFirst);
  layout->addWidget(m_toolBarSecond);
  layout->addWidget(m_splitter/*, 10*/);
  //layout->addWidget(m_searchToolBar);
  layout->addLayout(statusLayout);
  
  m_dirLister = new DirLister(this);
  m_dirLister->setSession(m_session);
  connect(m_dirLister, SIGNAL(completed()), m_actions, SLOT(updateActions()));
  connect(m_dirLister, SIGNAL(errorMessage(const QString&)), this, SLOT(showError(const QString&)));
  
  // Create the tree view
  m_treeView = new TreeView(m_splitter, this);
  m_splitter->addWidget(m_treeView);
  m_treeView->hide();
  
  m_dirModel = new DirModel(m_dirLister, this);
  m_dirModel->setDropsAllowed(DirModel::DropOnDirectory);
  
  m_proxyModel = new DirSortFilterProxyModel(this);
  m_proxyModel->setSourceModel(m_dirModel);

  // Create the details view
  m_detailsView = new DetailsView(m_splitter, this);
  m_detailsView->setModel(m_proxyModel);
  m_detailsView->sortByColumn(DirModel::Name, Qt::AscendingOrder);
  m_splitter->addWidget(m_detailsView);
  
  m_locationNavigator = new LocationNavigator(m_detailsView);
  connect(m_locationNavigator, SIGNAL(urlChanged(const KUrl&)), this, SLOT(slotUrlChanged(const KUrl&)));
  connect(m_locationNavigator, SIGNAL(historyChanged(const KUrl&)), this, SLOT(slotHistoryChanged(const KUrl&)));
  
  openUrl(KUrl(KFTPCore::Config::defLocalDir()));
  
  // TODO
  
  // Create the filter widget
  /*
  m_searchFilter = new FilterWidget(m_searchToolBar, m_detailsView);
  m_searchToolBar->setItemAutoSized(2, true);
  m_searchToolBar->setStretchableWidget(m_searchFilter);
  
  m_searchToolBar->updateRects(true);
  m_searchToolBar->hide();
  */
}

void View::slotUrlChanged(const KUrl &url)
{
  if (!m_freezeUrlUpdates)
    connect(m_dirLister, SIGNAL(completed()), this, SLOT(slotListingCompleted()));
  else
    m_freezeUrlUpdates = false;
  
  m_dirLister->openUrl(url);
}

void View::slotListingCompleted()
{
  int index = 0;
  const QList<LocationNavigator::Element> history = m_locationNavigator->history(index);
  if (!history.isEmpty()) {
    LocationNavigator::Element element = history[index];
    const KFileItem fileItem = element.currentItem();
    
    if (!fileItem.isNull()) {
      m_detailsView->setCurrentIndex(m_proxyModel->mapFromSource(m_dirModel->indexForItem(fileItem)));
      m_detailsView->horizontalScrollBar()->setValue(element.contentsX());
      m_detailsView->verticalScrollBar()->setValue(element.contentsY());
    }
  }
  
  disconnect(m_dirLister, SIGNAL(completed()), this, SLOT(slotListingCompleted()));
  emit urlChanged(m_locationNavigator->url());
}

void View::openUrl(const KUrl &url)
{
  m_locationNavigator->setUrl(url);
}

void View::openIndex(const QModelIndex &index)
{
  KFileItem item = index.data(DirModel::FileItemRole).value<KFileItem>();
  
  if (item.isDir()) {
    m_freezeUrlUpdates = true;
    openUrl(item.url());
  }
}

void View::setShowHidden(bool value)
{
  m_dirLister->setShowingDotFiles(value);
}

void View::setHomeUrl(const KUrl &url)
{
  m_locationNavigator->setHomeUrl(url);
}

void View::goHome()
{
  m_locationNavigator->goHome();
}

void View::goBack()
{
  m_locationNavigator->goBack();
}

void View::goForward()
{
  m_locationNavigator->goForward();
}

void View::goUp()
{
  m_locationNavigator->goUp();
}

void View::reload()
{
  m_dirLister->openUrl(m_locationNavigator->url(), KDirLister::Reload);
}

void View::rename(const KUrl &source, const QString &name)
{
  KUrl dest(source.upUrl());
  dest.addPath(name);
  
  if (source.isLocalFile())
    KIO::rename(source, dest, KIO::HideProgressInfo); 
  else
    m_session->getClient()->rename(source, dest);
}
  
void View::slotConfigUpdate()
{
  if (!m_treeVisibilityChanged)
    setTreeVisible(KFTPCore::Config::showTree());

  m_detailsView->setColumnHidden(DirModel::Owner, !KFTPCore::Config::showOwnerGroup());
  m_detailsView->setColumnHidden(DirModel::Group, !KFTPCore::Config::showOwnerGroup());
}

void View::setTreeVisible(bool visible)
{
  m_treeView->setVisible(visible);
  m_actions->m_toggleTreeViewAction->setChecked(visible);
}

void View::populateToolbar()
{
  // Add the actions to the toolbar
  m_toolBarFirst->addAction(m_actions->m_siteChangeAction);
  m_toolBarFirst->addSeparator();
  m_toolBarFirst->addAction(m_actions->m_goUpAction);
  m_toolBarFirst->addAction(m_actions->m_goBackAction);
  m_toolBarFirst->addAction(m_actions->m_goForwardAction);
  m_toolBarFirst->addAction(m_actions->m_reloadAction);
  m_toolBarFirst->addSeparator();
  m_toolBarFirst->addAction(m_actions->m_goHomeAction);
  m_toolBarFirst->addAction(m_actions->m_createDirAction);
  m_toolBarFirst->addSeparator();
  m_toolBarFirst->addAction(m_actions->m_toggleTreeViewAction);
  
  /*m_actions->m_siteChangeAction->plug(m_toolBarFirst);

  m_toolBarFirst->insertSeparator();

  m_actions->m_goUpAction->plug(m_toolBarFirst);
  m_actions->m_goBackAction->plug(m_toolBarFirst);
  m_actions->m_goForwardAction->plug(m_toolBarFirst);
  m_actions->m_reloadAction->plug(m_toolBarFirst);

  m_toolBarFirst->insertSeparator();

  m_actions->m_goHomeAction->plug(m_toolBarFirst);
  m_actions->m_createDirAction->plug(m_toolBarFirst);
  
  m_toolBarFirst->insertSeparator();
  
  m_actions->m_abortAction->plug(m_toolBarFirst);
  m_actions->m_toggleTreeViewAction->plug(m_toolBarFirst);
  m_actions->m_toggleFilterAction->plug(m_toolBarFirst);
  
  m_toolBarFirst->insertSeparator();
  
  m_actions->m_moreActions->plug(m_toolBarFirst);*/
}

void View::updateBookmarks()
{
  // Repopulate bookmarks menu on updates
  m_actions->m_connectAction->menu()->clear();
  KFTPBookmarks::Manager::self()->populateBookmarksMenu(m_actions->m_connectAction, m_session);
}

void View::slotHistoryActivated()
{
  KUrl dest = m_locationNavigator->url();
  dest.setPath(m_historyCombo->lineEdit()->text());
  
  openUrl(dest);
}

void View::slotHistoryActivated(const QString &text)
{
  KUrl dest = m_locationNavigator->url();
  dest.setPath(text);
  
  openUrl(dest);
}

void View::slotHistoryChanged(const KUrl &url)
{
  QString path = url.path(KUrl::RemoveTrailingSlash);
  
  m_historyCombo->addToHistory(path);
  m_historyCombo->setCurrentIndex(0);
}

void View::slotDisplayCertInfo()
{
  /*
  if (m_ftpClient->socket()->protocolName() == "ftp" && m_ftpClient->socket()->isEncrypted()) {
    KSSLInfoDlg *sslInfo = new KSSLInfoDlg(true, this);
    sslInfo->exec();
  } else if (m_ftpClient->socket()->protocolName() == "sftp") {
    KMessageBox::information(this, i18n("This is a SSH encrypted connection. No certificate info is currently available."));
  } else {
    KSSLInfoDlg *sslInfo = new KSSLInfoDlg(false, this);
    sslInfo->exec();
  }
  */
}

void View::slotDurationUpdate()
{
  m_connDuration = m_connDuration.addSecs(1);
  m_connDurationMsg->setText(m_connDuration.toString("hh:mm:ss"));
}

void View::slotEngineEvent(KFTPEngine::Event *event)
{
  switch (event->type()) {
    case Event::EventState: {
      // Set new state
      m_statusMsg->setText(event->getParameter(0).toString());
      break;
    }
    case Event::EventConnect:
    case Event::EventDisconnect: {
      // Change encryption icon
      m_statusIcon->setIcon(KIcon(m_ftpClient->socket()->isEncrypted() ? "object-locked" : "object-unlocked"));
      m_statusIcon->setEnabled(m_ftpClient->socket()->isConnected());
    
      // Start or stop the duration timer
      if (m_ftpClient->socket()->isConnected()) {
        m_connTimer->start(1000);
        m_connDuration.setHMS(0, 0, 0);
      } else {
        m_connTimer->stop();
        m_connDurationMsg->setText("");
      }
      
      // Reset selected charset to default
      /*
      KMenu *menu = m_actions->m_changeEncodingAction->popupMenu();
      menu->setItemChecked(m_actions->m_defaultCharsetOption, true);
      menu->setItemChecked(m_actions->m_curCharsetOption, false);
      m_actions->m_curCharsetOption = m_actions->m_defaultCharsetOption;
      */
      break;
    }
    default: break;
  }

  /*
  if (m_ftpClient->socket()->isBusy()) {
    m_tree->setEnabled(false);
    m_detailsView->setEnabled(false);
    m_toolBarSecond->setEnabled(false);
  } else if (KFTPQueue::Manager::self()->getNumRunning(m_ftpClient->socket()->getCurrentUrl()) == 0) {
    m_tree->setEnabled(true);
    m_detailsView->setEnabled(true);
    m_toolBarSecond->setEnabled(true);
  }
  */
  
  // Update actions
  m_actions->updateActions();
}

void View::showError(const QString &message)
{
  if (m_infoMessage)
    delete m_infoMessage;
  
  m_infoMessage = new PopupMessage(m_statusMsg, this);
  m_infoMessage->setText(i18n("<b>Error</b><br />") + message);
  m_infoMessage->setImage(KIcon("dialog-error").pixmap(32, 32));
  m_infoMessage->setShowCloseButton(false);
  
  m_infoMessage->reposition();
  m_infoMessage->display();
}

void View::showMessage(const QString &message)
{
  if (m_infoMessage)
    delete m_infoMessage;
  
  m_infoMessage = new PopupMessage(m_statusMsg, this);
  m_infoMessage->setText(i18n("<b>Information</b><br />") + message);
  m_infoMessage->setImage(KIcon("info").pixmap(32, 32));
  m_infoMessage->setShowCloseButton(false);
  
  m_infoMessage->reposition();
  m_infoMessage->display();
}

void View::openContextMenu(const QModelIndexList &indexes, const QPoint &pos)
{
  QMenu menu(this);
  m_currentIndexes = indexes;
  
  // TODO
  // ContextMenu menu(pos, indexes);
  // menu.open();
  
  menu.addAction(KIcon("list-add"), i18n("&Queue Transfer"), m_actions, SLOT(slotQueueTransfer()));
  
  menu.exec(pos);
}

}

}

#include "view.moc"
