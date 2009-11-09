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

#include "kftpsession.h"

#include "browser/detailsview.h"
#include "browser/view.h"

#include "kftpbookmarks.h"
#include "widgets/systemtray.h"
#include "widgets/sslerrorsdialog.h"
#include "widgets/fingerprintverifydialog.h"

#include "misc/config.h"
#include "misc/filter.h"

#include <QEvent>

#include <kmessagebox.h>
#include <klocale.h>
#include <kdebug.h>
#include <kpassworddialog.h>

using namespace KFTPEngine;
using namespace KFTPCore::Filter;
using namespace KFTPWidgets;

namespace KFTPSession {

//////////////////////////////////////////////////////////////////
////////////////////////    Connection     ///////////////////////
//////////////////////////////////////////////////////////////////

Connection::Connection(Session *session, bool primary)
  : QObject(session),
    m_primary(primary),
    m_busy(false),
    m_aborting(false),
    m_scanning(false)
{
  // Create the actual connection client
  m_client = new KFTPEngine::Thread();
  
  connect(m_client->eventHandler(), SIGNAL(engineEvent(KFTPEngine::Event*)), this, SLOT(slotEngineEvent(KFTPEngine::Event*)));

  // If this is not a core session connection, connect
  if (!primary) {
    // Connect to the server
    KUrl url = session->getClient()->socket()->getCurrentUrl();
    
    KFTPBookmarks::Manager::self()->setupClient(session->getSite(), m_client, session->getClient());
    m_client->connect(url);
  }
}

Connection::~Connection()
{
  m_client->shutdown();
}

bool Connection::isConnected()
{
  return !static_cast<Session*>(parent())->isRemote() || m_client->socket()->isConnected();
}

void Connection::acquire(KFTPQueue::Transfer *transfer)
{
  if (m_busy || !static_cast<Session*>(parent())->isRemote())
    return;

  m_transfer = transfer;
  m_busy = true;

  connect(transfer, SIGNAL(transferComplete(long)), this, SLOT(slotTransferCompleted()));
  connect(transfer, SIGNAL(transferAbort(long)), this, SLOT(slotTransferCompleted()));

  emit connectionAcquired();
}

void Connection::release()
{
  // Disconnect all signals from the transfer
  if (m_transfer)
    m_transfer->QObject::disconnect(this);

  m_transfer = 0L;
  m_busy = false;

  emit connectionRemoved();
  emit static_cast<Session*>(parent())->freeConnectionAvailable();
}

void Connection::abort()
{
  if (m_scanning) {
    // Abort scanning
    m_scanning = false;
    m_transfer->unlock();
    release();
  }
  
  if (m_aborting || !m_client->socket()->isBusy())
    return;

  // Emit the signal before aborting
  emit aborting();

  // Abort transfer
  m_aborting = true;
  m_client->abort();
  m_aborting = false;
}

bool Connection::isBusy() const
{
  return m_busy || m_client->socket()->isBusy();
}

void Connection::scanDirectory(KFTPQueue::Transfer *parent)
{
  // Lock the connection and the transfer
  acquire(parent);
  parent->lock();
  
  m_scanning = true;
  
  if (isConnected())
    m_client->scan(parent->getSourceUrl());
}

void Connection::addScannedDirectory(KFTPEngine::DirectoryTree *tree, KFTPQueue::Transfer *parent)
{
  if (!m_scanning)
    return;
  
  // Directories
  DirectoryTree::DirIterator dirEnd = tree->directories()->constEnd();
  for (DirectoryTree::DirIterator i = tree->directories()->constBegin(); i != dirEnd; i++) {
    if (!m_scanning)
      return;
    
    KUrl sourceUrlBase = parent->getSourceUrl();
    KUrl destUrlBase = parent->getDestUrl();
    
    sourceUrlBase.addPath((*i)->info().filename());
    destUrlBase.addPath((*i)->info().filename());
    
    // Check if we should skip this entry
    const ActionChain *actionChain = Filters::self()->process(sourceUrlBase, 0, true);
     
    if (actionChain && actionChain->getAction(Action::Skip))
      continue;
    
    // Add directory transfer
    KFTPQueue::TransferDir *transfer = new KFTPQueue::TransferDir(parent);
    transfer->setSourceUrl(sourceUrlBase);
    transfer->setDestUrl(destUrlBase);
    transfer->setTransferType(parent->getTransferType());
    transfer->setId(KFTPQueue::Manager::self()->nextTransferId());
    
    emit KFTPQueue::Manager::self()->objectAdded(transfer);
    transfer->readyObject();
    
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    addScannedDirectory(*i, transfer);
    
    if (KFTPCore::Config::skipEmptyDirs() && !transfer->hasChildren())
      KFTPQueue::Manager::self()->removeTransfer(transfer, false);
  }
  
  // Files
  DirectoryTree::FileIterator fileEnd = tree->files()->constEnd();
  for (DirectoryTree::FileIterator i = tree->files()->constBegin(); i != fileEnd; i++) {
    if (!m_scanning)
      return;
    
    KUrl sourceUrlBase = parent->getSourceUrl();
    KUrl destUrlBase = parent->getDestUrl();
    
    sourceUrlBase.addPath((*i).filename());
    destUrlBase.addPath((*i).filename());
    
    // Check if we should skip this entry
    const ActionChain *actionChain = Filters::self()->process(sourceUrlBase, (*i).size(), false);
     
    if (actionChain && actionChain->getAction(Action::Skip))
      continue;
    
    // Add file transfer
    KFTPQueue::TransferFile *transfer = new KFTPQueue::TransferFile(parent);
    transfer->addSize((*i).size());
    transfer->setSourceUrl(sourceUrlBase);
    transfer->setDestUrl(destUrlBase);
    transfer->setTransferType(parent->getTransferType());
    transfer->setId(KFTPQueue::Manager::self()->nextTransferId());
    
    emit KFTPQueue::Manager::self()->objectAdded(transfer);
    transfer->readyObject();
    
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
  }
}

void Connection::slotEngineEvent(KFTPEngine::Event *event)
{
  switch (event->type()) {
    case Event::EventDisconnect: {
      emit connectionLost(this);
      break;
    }
    case Event::EventConnect: {
      emit connectionEstablished();
      
      if (m_scanning) {
        // Connected successfully, let's scan
        m_client->scan(m_transfer->getSourceUrl());
      }
      break;
    }
    case Event::EventError: {
      ErrorCode error = static_cast<ErrorCode>(event->getParameter(0).toInt());
      
      if (m_scanning && (error == ConnectFailed || error == LoginFailed || error == OperationFailed)) {
        // Scanning should be aborted, since there was an error
        m_scanning = false;
        m_transfer->unlock();
        release();
        
        emit static_cast<Session*>(parent())->dirScanDone();
      }
      break;
    }
    case Event::EventScanComplete: {
      if (m_scanning) {
        // We have the listing
        DirectoryTree *tree = event->getParameter(0).value<DirectoryTree*>();
        addScannedDirectory(tree, m_transfer);
        delete tree;
        
        m_scanning = false;
        m_transfer->unlock();
        release();
        
        emit static_cast<Session*>(parent())->dirScanDone();
      }
      break;
    }
    case Event::EventReady: {
      if (!m_busy) {
        emit static_cast<Session*>(parent())->freeConnectionAvailable();
      }
      break;
    }
    default: break;
  }
}

void Connection::slotTransferCompleted()
{
  // Remove the lock
  release();
}

void Connection::reconnect()
{
  if (!m_client->socket()->isConnected()) {
    KFTPBookmarks::Manager::self()->setupClient(static_cast<Session*>(parent())->getSite(), m_client);
    m_client->connect(m_client->socket()->getCurrentUrl());
  }
}

////////////////////////////////////////////////////////
////////////////////    Session     ////////////////////
////////////////////////////////////////////////////////

Session::Session(Side side)
  : QObject(),
    m_side(side),
    m_remote(false),
    m_aborting(false),
    m_registred(false),
    m_site(0)
{
  // Register this session
  Manager::self()->registerSession(this);
}

Session::~Session()
{
}

Session *Session::oppositeSession() const
{
  return Manager::self()->getActive(oppositeSide(m_side));
}

KFTPEngine::Thread *Session::getClient()
{
  // Return the first (core) connection's client
  return m_connections.at(0)->getClient();
}

bool Session::isConnected()
{
  // If there are no connections, just check if the session is remote
  if (m_connections.count() == 0)
    return !m_remote;
  
  return m_connections.at(0)->isConnected();
}

void Session::slotClientEngineEvent(KFTPEngine::Event *event)
{
  switch (event->type()) {
    case Event::EventConnect: {
      // ***************************************************************************
      // ****************************** EventConnect *******************************
      // ***************************************************************************
      m_remote = true;
      m_aborting = false;
      m_lastUrl = getClient()->socket()->getCurrentUrl();
      
      QString siteName;
      if (m_site)
        siteName = m_site->getAttribute("name");
      else
        siteName = m_lastUrl.host();
  
      Manager::self()->getTabs(m_side)->setTabText(Manager::self()->getTabs(m_side)->indexOf(m_fileView), siteName);
      Manager::self()->getStatTabs()->setTabText(Manager::self()->getStatTabs()->indexOf(m_log), i18n("Log (%1)",siteName));
      Manager::self()->getStatTabs()->setCurrentIndex(Manager::self()->getStatTabs()->indexOf(m_log));
      Manager::self()->doEmitUpdate();
      
      KUrl homeUrl = getClient()->socket()->getCurrentUrl();
      
      if (m_site && !m_site->getProperty("pathRemote").isEmpty())
        homeUrl.setPath(m_site->getProperty("pathRemote"));
      else
        homeUrl.setPath(getClient()->socket()->getDefaultDirectory());
      
      m_fileView->setHomeUrl(homeUrl);
      m_fileView->goHome();
      
      Session *opposite = Manager::self()->getActive(oppositeSide(m_side));
      
      if (m_site && !opposite->isRemote()) {
        QString localPath = m_site->getProperty("pathLocal");
        
        if (!localPath.isEmpty())
          opposite->getFileView()->openUrl(KUrl(localPath));
      }
      break;
    }
    case Event::EventDisconnect: {
      // ***************************************************************************
      // **************************** EventDisconnect ******************************
      // ***************************************************************************
      m_remote = false;
      m_aborting = false;
    
      Manager::self()->getTabs(m_side)->setTabText(Manager::self()->getTabs(m_side)->indexOf(m_fileView), i18n("Local Session"));
      Manager::self()->getStatTabs()->setTabText(Manager::self()->getStatTabs()->indexOf(m_log), "[" + i18n("Not Connected") + "]");
      Manager::self()->doEmitUpdate();
      
      m_fileView->setHomeUrl(KUrl(KFTPCore::Config::defLocalDir()));
      m_fileView->goHome();
      break;
    }
    case Event::EventCommand: m_log->append(event->getParameter(0).toString(), LogView::FtpCommand);  break;
    case Event::EventMultiline: m_log->append(event->getParameter(0).toString(), LogView::FtpMultiline); break;
    case Event::EventResponse: m_log->append(event->getParameter(0).toString(), LogView::FtpResponse); break;
    case Event::EventMessage: m_log->append(event->getParameter(0).toString(), LogView::FtpStatus); break;
    case Event::EventError: {
      // ***************************************************************************
      // ****************************** EventError *********************************
      // ***************************************************************************
      ErrorCode error = static_cast<ErrorCode>(event->getParameter(0).toInt());
      QString details = event->getParameter(1).toString();
      
      if (error != ConnectFailed && error != LoginFailed)
        return;
      
      KMessageBox::detailedError(0, i18n("Unable to establish a connection with the remote server."), details);
      break;
    }
    case Event::EventRetrySuccess: {
      // ***************************************************************************
      // ************************** EventRetrySuccess ******************************
      // ***************************************************************************
      if (KFTPCore::Config::showRetrySuccessBalloon()) {
        KFTPWidgets::SystemTray::self()->showMessage(i18n("Information"), i18n("Connection with %1 has been successfully established.", getClient()->socket()->getCurrentUrl().host()));
      }
      break;
    }
    case Event::EventReloadNeeded: {
      // We should only do refreshes if the queue is not being processed
      if (KFTPQueue::Manager::self()->getNumRunning() == 0)
        m_fileView->reload();
      break;
    }
    case Event::EventPubkeyPassword: {
      // A public-key authentication password was requested
      KPasswordDialog dialog;
      dialog.setPrompt(i18n("Please provide your private key decryption password."));
      
      if (dialog.exec()) {
        PubkeyWakeupEvent *response = new PubkeyWakeupEvent();
        response->password = dialog.password();
        
        getClient()->wakeup(response);
      } else {
        getClient()->abort();
      }
      break;
    }
    case Event::EventPeerVerify: {
      // Peer requires verification
      QString protocol = getUrl().protocol();
      PeerVerifyWakeupEvent *response = new PeerVerifyWakeupEvent();
      
      if (protocol == "ftp") {
        SslErrorsDialog dialog;
        dialog.setErrors(event->getParameter(0).toList());
        response->peerOk = (bool) dialog.exec();
      } else if (protocol == "sftp") {
        FingerprintVerifyDialog dialog;
        dialog.setFingerprint(event->getParameter(0).toByteArray(), getUrl());
        response->peerOk = (bool) dialog.exec();
      }
      
      getClient()->wakeup(response);
      break;
    }
    default: break;
  }
}

void Session::scanDirectory(KFTPQueue::Transfer *parent, Connection *connection)
{
  // Go trough all files in path and add them as transfers that have parent as their parent
  // transfer
  KUrl path = parent->getSourceUrl();

  if (path.isLocalFile()) {
    connect(new DirectoryScanner(parent), SIGNAL(completed()), this, SIGNAL(dirScanDone()));
  } else if (m_remote) {
    if (!connection) {
      if (!isFreeConnection()) {
        emit dirScanDone();
        return;
      }
      
      // Assign a new connection (it might be unconnected!)
      connection = assignConnection();
    }
    
    connection->scanDirectory(parent);
  }
}

void Session::abort()
{
  if (m_aborting)
    return;

  m_aborting = true;

  emit aborting();

  // Abort all connections
  foreach (Connection *c, m_connections) {
    c->abort();
  }

  m_aborting = false;
}

void Session::reconnect(const KUrl &url)
{
  // Set the reconnect url
  m_reconnectUrl = url;
  
  if (m_remote && getClient()->socket()->isConnected()) {
    abort();
    
    connect(getClient()->eventHandler(), SIGNAL(disconnected()), this, SLOT(slotStartReconnect()));
    getClient()->disconnect();
  } else {
    // The session is already disconnected, just call the slot
    slotStartReconnect();
  }
}

void Session::slotStartReconnect()
{
  disconnect(getClient()->eventHandler(), SIGNAL(disconnected()), this, SLOT(slotStartReconnect()));
  
  // Reconnect only if this is a remote url
  if (!m_reconnectUrl.isLocalFile()) {
    KFTPBookmarks::Manager::self()->setupClient(m_site, getClient());
    getClient()->connect(m_reconnectUrl);
  }
  
  // Invalidate the url
  m_reconnectUrl = KUrl();
}

int Session::getMaxThreadCount()
{
  // First get the global thread count
  int count = KFTPCore::Config::threadCount();
  
  if (!KFTPCore::Config::threadUsePrimary())
    count++;

  // Try to see if threads are disabled for this site
  if (count > 1 && isRemote()) {
    if (m_site && m_site->getIntProperty("disableThreads"))
      return 1;
  }

  return count;
}

bool Session::isFreeConnection()
{
  int max = getMaxThreadCount();
  int free = 0;

  if ((m_connections.count() < max && max > 1) || !isRemote())
    return true;

  foreach (Connection *c, m_connections) {
    if (!c->isBusy() && (!c->isPrimary() || KFTPCore::Config::threadUsePrimary() || max == 1))
      free++;
  }

  return free > 0;
}

Connection *Session::assignConnection()
{
   int max = getMaxThreadCount();

  if (m_connections.count() == 0) {
    // We need a new core connection
    Connection *c = new Connection(this, true);
    m_connections.append(c);

    Manager::self()->doEmitUpdate();
    return c;
  } else {
    // Find a free connection
    foreach (Connection *c, m_connections) {
      if (!c->isBusy() && (!c->isPrimary() || KFTPCore::Config::threadUsePrimary() || max == 1))
        return c;
    }

    // No free connection has been found, but we may be able to create
    // another (if we are within limits)
    if (m_connections.count() < max) {
      Connection *c = new Connection(this);
      m_connections.append(c);

      Manager::self()->doEmitUpdate();
      return c;
    }
  }

  return 0;
}

void Session::disconnectAllConnections()
{
  // Abort any possible transfers first
  abort();
  
  // Now disconnect all connections
  foreach (Connection *c, m_connections) {
    if (c->getClient()->socket()->isConnected()) {
      c->getClient()->disconnect();
    }
  }
}

////////////////////////////////////////////////////////
/////////////////////// Manager ////////////////////////
////////////////////////////////////////////////////////

Manager *Manager::m_self = 0;

Manager *Manager::self()
{
  return m_self;
}

Manager::Manager(QObject *parent, QTabWidget *stat, KTabWidget *left, KTabWidget *right)
  : QObject(parent),
    m_statTabs(stat),
    m_leftTabs(left),
    m_rightTabs(right),
    m_active(0),
    m_leftActive(0),
    m_rightActive(0)
{
  Manager::m_self = this;

  // Connect some signals
  connect(left, SIGNAL(currentChanged(QWidget*)), this, SLOT(slotActiveChanged(QWidget*)));
  connect(right, SIGNAL(currentChanged(QWidget*)), this, SLOT(slotActiveChanged(QWidget*)));

  connect(left, SIGNAL(closeRequest(QWidget*)), this, SLOT(slotSessionCloseRequest(QWidget*)));
  connect(right, SIGNAL(closeRequest(QWidget*)), this, SLOT(slotSessionCloseRequest(QWidget*)));
}

void Manager::registerSession(Session *session)
{
  m_active = session;
  
  // Create some new stuff and assign it to the session
  session->assignConnection();
  session->m_fileView = new KFTPWidgets::Browser::View(0, session->getClient(), session);
  session->m_log = new KFTPWidgets::LogView(m_statTabs);
  
  // Setup default home URL
  session->m_fileView->setHomeUrl(KUrl(KFTPCore::Config::defLocalDir()));
  
  // Install event filters
  /*
  session->getFileView()->getDetailsView()->installEventFilter(this);
  session->getFileView()->getTreeView()->installEventFilter(this);
  session->getFileView()->m_toolBarFirst->installEventFilter(this);
  session->getFileView()->m_toolBarSecond->installEventFilter(this);

  connect(session->getFileView()->getDetailsView(), SIGNAL(clicked(Q3ListViewItem*)), this, SLOT(slotSwitchFocus()));
  connect(session->getFileView()->getTreeView(), SIGNAL(clicked(Q3ListViewItem*)), this, SLOT(slotSwitchFocus()));
  connect(session->getFileView()->m_toolBarFirst, SIGNAL(clicked(int)), this, SLOT(slotSwitchFocus()));
  connect(session->getFileView()->m_toolBarSecond, SIGNAL(clicked(int)), this, SLOT(slotSwitchFocus()));
  */
  // Connect some signals
  connect(session->getClient()->eventHandler(), SIGNAL(engineEvent(KFTPEngine::Event*)), session, SLOT(slotClientEngineEvent(KFTPEngine::Event*)));

  // Assign GUI positions
  KTabWidget *tabs = getTabs(session->getSide());
  m_statTabs->addTab(session->m_log, "[" + i18n("Not Connected") + "]");
  tabs->addTab(session->m_fileView, KIcon("system"), i18n("Session"));

  // Actually add the session
  m_sessionList.append(session);
  session->m_registred = true;

  if (tabs->count() > 1)
    tabs->setTabBarHidden(false);
}

KFTPWidgets::Browser::View *Manager::getActiveView()
{
  if (m_active)
    return m_active->getFileView();
  
  return 0;
}

Session *Manager::getActiveSession()
{
  return m_active;
}

bool Manager::eventFilter(QObject *object, QEvent *event)
{
  if (event->type() == QEvent::FocusIn || event->type() == QEvent::MouseButtonPress) {
    switchFocusToObject(object);
  }
  
  return false;
}

void Manager::slotSwitchFocus()
{
  switchFocusToObject(QObject::sender());
}

void Manager::switchFocusToObject(const QObject *object)
{
  if (!object)
    return;
  
  for (;;) {
    if (object->inherits("KFTPWidgets::Browser::View"))
      break;
    
    if (!(object = object->parent()))
      break;
  }
  
  if (object) {
    // We have the proper object
    Session *session = find(static_cast<const KFTPWidgets::Browser::View*>(object));
    
    if (session && session != m_active) {
      m_active = session;
      
      // Open the current session's log tab
      if (session->isRemote())
        m_statTabs->setCurrentIndex(m_statTabs->indexOf(session->getLog()));
    }
  }
}

void Manager::unregisterSession(Session *session)
{
  KTabWidget *tabs = getTabs(session->getSide());

  if (tabs->count() == 2)
    tabs->setTabBarHidden(true);

  // Destroy all objects related to the session and remove it
  tabs->removePage(session->m_fileView);
  m_statTabs->removeTab(m_statTabs->indexOf(session->m_log));

  if (session->getClient()->socket()->isConnected()) {
    session->abort();
    session->getClient()->disconnect();
  }

  // Delete objects
  session->m_fileView->deleteLater();
  session->m_log->deleteLater();

  // Actually remove the session
  m_sessionList.removeAll(session);
  delete session;
 
  emit update();
}

void Manager::doEmitUpdate()
{
  emit update();
}

void Manager::disconnectAllSessions()
{
  foreach (Session *s, m_sessionList) {
    s->disconnectAllConnections();
  }
}

Session *Manager::find(KFTPEngine::Thread *client)
{
  foreach (Session *s, m_sessionList) {
    if (s->getClient() == client)
      return s;
  }

  return 0;
}

Session *Manager::find(KFTPWidgets::Browser::View *fileView)
{
  foreach (Session *s, m_sessionList) {
    if (s->m_fileView == fileView)
      return s;
  }

  return 0;
}

Session *Manager::find(KFTPWidgets::LogView *log)
{
  foreach (Session *s, m_sessionList) {
    if (s->m_log == log)
      return s;
  }

  return 0L;
}

Session *Manager::find(const KUrl &url, bool mustUnlock)
{
  if (url.isLocalFile())
    return find(true);

  foreach (Session *s, m_sessionList) {
    KUrl tmp = s->getClient()->socket()->getCurrentUrl();
    tmp.setPath(url.path());

    if (tmp == url && s->isRemote() && s->isConnected() && (!mustUnlock || s->isFreeConnection()))
      return s;
  }
  
  return 0;
}

Session *Manager::find(bool local)
{
  foreach (Session *s, m_sessionList) {
    if (s->m_remote != local)
      return s;
  }

  return 0;
}

Session *Manager::findLast(const KUrl &url, Side side)
{
  if (url.isLocalFile())
    return find(true);

  foreach (Session *s, m_sessionList) {
    KUrl tmp = s->m_lastUrl;
    tmp.setPath(url.path());

    if (tmp == url && !s->isRemote() && (s->getSide() || side == IgnoreSide))
      return s;
  }
  
  return 0;
}

Session *Manager::spawnLocalSession(Side side, bool forceNew)
{
  // Creates a new local session
  Session *session = 0;

  if (forceNew || (session = find(true)) == 0 || (session->m_side != side && side != IgnoreSide)) {
    side = side == IgnoreSide ? LeftSide : side;

    session = new Session(side);
    session->m_remote = false;
    getTabs(side)->setTabText(getTabs(side)->indexOf(session->m_fileView), i18n("Local Session"));
    getStatTabs()->setTabText(getStatTabs()->indexOf(session->m_log), "[" + i18n("Not Connected") + "]");
  }

  setActive(session);
  return session;
}

Session *Manager::spawnRemoteSession(Side side, const KUrl &remoteUrl, KFTPBookmarks::Site *site, bool mustUnlock)
{
  // Creates a new remote session and connects it to the correct server
  Session *session;

  if (remoteUrl.isLocalFile())
    return spawnLocalSession(side);

  if ((session = find(remoteUrl, mustUnlock)) == 0L || (session->m_side != side && side != IgnoreSide)) {
    // Try to find the session that was last connected to this URL
    if ((session = findLast(remoteUrl, side)) == 0L) {
      // Attempt to reuse a local session if one exists one the right side
      session = getActive(RightSide);
      
      if (session->isRemote()) {
        side = side == IgnoreSide ? RightSide : side;
        session = new Session(side);
      }
    }
  
    // Try to find the site by url if it is not set
    if (!site)
      site = KFTPBookmarks::Manager::self()->findSite(remoteUrl);

    // Set properties
    session->m_remote = true;
    session->m_site = site;
    m_active = session;

    KFTPBookmarks::Manager::self()->setupClient(site, session->getClient());
    session->getClient()->connect(remoteUrl);
  }

  return session;
}

void Manager::setActive(Session *session)
{
  if (!session)
    return;
  
  // Make a session active on its own side
  Session *oldActive = getActive(session->m_side);

  if (oldActive)
    oldActive->m_active = false;
  
  session->m_active = true;

  switch (session->m_side) {
    case LeftSide: m_leftActive = session; break;
    case RightSide: m_rightActive = session; break;
    case IgnoreSide: qDebug("Invalid side specified!"); return;
  }

  // Refresh the GUI
  getTabs(session->m_side)->setCurrentIndex(getTabs(session->m_side)->indexOf(session->m_fileView));
}

Session *Manager::getActive(Side side)
{
  switch (side) {
    case LeftSide: return m_leftActive;
    case RightSide: return m_rightActive;
    case IgnoreSide: qDebug("Invalid side specified!"); break;
  }

  return NULL;
}

KTabWidget *Manager::getTabs(Side side)
{
  switch (side) {
    case LeftSide: return m_leftTabs;
    case RightSide: return m_rightTabs;
    case IgnoreSide: qDebug("Invalid side specified!"); break;
  }

  return NULL;
}

void Manager::slotActiveChanged(QWidget *page)
{
  Session *session = find(static_cast<KFTPWidgets::Browser::View*>(page));
  setActive(session);
}

void Manager::slotSessionCloseRequest(QWidget *page)
{
  Session *session = find(static_cast<KFTPWidgets::Browser::View*>(page));

  if (getTabs(session->m_side)->count() == 1) {
    KMessageBox::error(0L, i18n("At least one session must remain open on each side."));
    return;
  }

  if ((session->m_remote && session->getClient()->socket()->isBusy()) || !session->isFreeConnection()) {
    KMessageBox::error(0L, i18n("Please finish all transfers before closing the session."));
    return;
  } else {
    // Remove the session
    if (session->getClient()->socket()->isConnected()) {
      if (KMessageBox::questionYesNo(0L, i18n("This session is currently connected. Are you sure you wish to disconnect?"), i18n("Close Session")) == KMessageBox::No)
        return;
    }

    unregisterSession(session);
  }
}

}

#include "kftpsession.moc"
