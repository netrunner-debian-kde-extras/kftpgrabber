/*
 * This file is part of the KFTPGrabber project
 *
 * Copyright (C) 2003-2004 by the KFTPGrabber developers
 * Copyright (C) 2003-2004 Jernej Kos <kostko@jweb-network.net>
 * Copyright (C) 2004 Markus Brueffer <markus@brueffer.de>
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

#include <qlabel.h>
#include <qlayout.h>
#include <qpainter.h>
#include <q3textedit.h>
#include <qtabwidget.h>
#include <q3hbox.h>
#include <q3vbox.h>
#include <qthread.h>

#include <QHBoxLayout>
#include <QList>
#include <QSplitter>
#include <QPixmap>
#include <QDockWidget>

#include <kapplication.h>
#include <kmenubar.h>
#include <kmenu.h>
#include <ktoolbar.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kstatusbar.h>
#include <klocale.h>
#include <kio/job.h>
#include <kpassworddialog.h>
#include <kstandarddirs.h>
#include <KActionCollection>
#include <KStandardAction>

// Widgets
#include "widgets/configdialog.h"
#include "widgets/systemtray.h"
//#include "bookmarks/sidebar.h"
#include "bookmarks/editor.h"
//#include "bookmarks/listview.h"
#include "widgets/logview.h"
#include "widgets/queueview/queueview.h"
#include "widgets/queueview/threadview.h"
//#include "widgets/quickconnect.h"
//#include "kftpserverlineedit.h"
//#include "browser/view.h"
//#include "kftpzeroconflistview.h"
#include "widgets/trafficgraph.h"
#include "widgets/failedtransfers/view.h"

#include "misc/config.h"
#include "misc/filter.h"
#include "misc/customcommands/manager.h"

#include "mainwindow.h"
#include "kftpbookmarks.h"
#include "kftpqueue.h"
#include "kftpsession.h"
#include "kftpqueueconverter.h"
#include "misc/pluginmanager.h"
#include "engine/thread.h"

MainWindow::MainWindow()
  : KXmlGuiWindow(),
    m_trafficGraph(0),
    m_configDialog(0)
{
  connect(KApplication::kApplication(), SIGNAL(aboutToQuit()), this, SLOT(appShutdown()));

  // Restore size and position
  resize(KFTPCore::Config::size());
  move(KFTPCore::Config::position());
  //setCaption("KFTPGrabber");

  KFTPCore::Config::self()->postInit();

  // Load plugins
  KFTPCore::PluginManager::self()->loadPlugins();
  
  // Load custom commands
  KFTPCore::CustomCommands::Manager::self()->load();

  connect(KFTPQueue::Manager::self(), SIGNAL(queueUpdate()), this, SLOT(slotUpdateStatusBar()));
  connect(KFTPBookmarks::Manager::self(), SIGNAL(update()), this, SLOT(initBookmarkMenu()));
  connect(KFTPCore::Config::self(), SIGNAL(configChanged()), this, SLOT(slotConfigChanged()));

  // Init the gui system
  initTrafficGraph();
  initMainView();
  initStatusBar();
  initBookmarkMenu();
  setupActions();

  // Create the systray icon
  new KFTPWidgets::SystemTray(this);

  // Create base two sessions
  KFTPSession::Manager::self()->spawnLocalSession(KFTPSession::LeftSide);
  KFTPSession::Manager::self()->spawnLocalSession(KFTPSession::RightSide);

  // Load bookmarks
  QTimer::singleShot(0, this, SLOT(slotLoader()));

  // Check for the uirc file
  if (KGlobal::dirs()->findResource("appdata", "kftpgrabberui.rc") == QString::null) {
    KMessageBox::error(0, i18n("<qt>Unable to find %1 XML GUI descriptor file. Please check that you have installed the application correctly. If you have any questions please ask on %2.<br><br><b>Warning:</b> Current GUI will be incomplete.</qt>", xmlFile(), "irc.freenode.net/#kftpgrabber"));
  }

  createGUI("kftpgrabberui.rc");

  // Auto-save toolbar/menubar/statusbar settings
  setAutoSaveSettings(QString::fromLatin1("MainWindow"), false);
}

void MainWindow::setupActions()
{
  // Setup file menu
  QAction *quickConnect = actionCollection()->addAction("quick_connect");
  quickConnect->setText(i18n("&Connect..."));
  quickConnect->setIcon(KIcon("network-connect"));
  connect(quickConnect, SIGNAL(triggered()), this, SLOT(slotQuickConnect()));
  
  KActionMenu *newSession = new KActionMenu(KIcon("document-new"), i18n("&New Session"), this);
  actionCollection()->addAction("new_session", newSession);
  
  QAction *sessionLeft = new QAction(this);
  sessionLeft->setText(i18n("&Left Side"));
  connect(sessionLeft, SIGNAL(triggered()), this, SLOT(slotNewSessionLeft()));
  
  QAction *sessionRight = new QAction(this);
  sessionRight->setText(i18n("&Right Side"));
  connect(sessionRight, SIGNAL(triggered()), this, SLOT(slotNewSessionRight()));
  
  newSession->addAction(sessionLeft);
  newSession->addAction(sessionRight);
  newSession->setStickyMenu(true);
  newSession->setDelayed(false);
  
  KActionMenu *transferMode = new KActionMenu(KIcon("application-octet-stream"), i18n("&Mode (Auto)"), this);
  actionCollection()->addAction("transfer_mode", transferMode);
  
  QAction *asciiMode = new QAction(this);
  asciiMode->setText(i18n("&ASCII"));
  asciiMode->setIcon(KIcon("text-x-generic"));
  connect(asciiMode, SIGNAL(triggered()), this, SLOT(slotModeAscii()));
  
  QAction *binaryMode = new QAction(this);
  binaryMode->setText(i18n("&Binary"));
  binaryMode->setIcon(KIcon("application-octet-stream"));
  connect(binaryMode, SIGNAL(triggered()), this, SLOT(slotModeBinary()));
  
  QAction *autoMode = new QAction(this);
  autoMode->setText(i18n("A&uto"));
  connect(autoMode, SIGNAL(triggered()), this, SLOT(slotModeAuto()));
  
  // Set grouping so only one action can be selected
  QActionGroup *modeGroup = new QActionGroup(this);
  modeGroup->setExclusive(true);
  modeGroup->addAction(asciiMode);
  modeGroup->addAction(binaryMode);
  modeGroup->addAction(autoMode);

  // Insert the actions into the menu
  transferMode->addAction(asciiMode);
  transferMode->addAction(binaryMode);
  transferMode->addSeparator();
  transferMode->addAction(autoMode);
  transferMode->setStickyMenu(true);
  transferMode->setDelayed(false);
  
  KStandardAction::preferences(this, SLOT(slotSettingsConfig()), actionCollection());
  KStandardAction::saveOptions(this, SLOT(slotSettingsSave()), actionCollection());
  KStandardAction::quit(this, SLOT(slotFileQuit()), actionCollection());
}

void MainWindow::slotLoader()
{
  // Load bookmarks and custom site commands
  KFTPBookmarks::Manager::self()->load(KStandardDirs::locateLocal("appdata", "bookmarks.xml"));

  // Load the saved queue
  KFTPQueue::Manager::self()->getConverter()->importQueue(KStandardDirs::locateLocal("appdata", "queue"));

  // Update the bookmark menu
  initBookmarkMenu();
}

void MainWindow::appShutdown()
{
  KFTPQueue::Manager::self()->stopAllTransfers();
  KFTPSession::Manager::self()->disconnectAllSessions();
  
  // Save the queueview layout
  m_queueView->saveLayout();

  // Save the config data on shutdown
  KFTPCore::Config::setSize(size());
  KFTPCore::Config::setPosition(pos());
  
  KFTPCore::Config::self()->saveConfig();
  KFTPCore::Filter::Filters::self()->close();

  // Save current queue
  KFTPQueue::Manager::self()->getConverter()->exportQueue(KStandardDirs::locateLocal("appdata", "queue"));
}

bool MainWindow::queryClose()
{
#if 0
  if(KApplication::kApplication()->sessionSaving()) {
    m_actions->m_closeApp = true;
  }

  if (!KFTPCore::Config::exitOnClose() && KFTPCore::Config::showSystrayIcon() && !m_actions->m_closeApp) {
    /*
      * This code was adopted from the Konversation project
      * copyright: (C) 2003 by Dario Abatianni, Peter Simonsson
      * email:     eisfuchs@tigress.com, psn@linux.se
      */

    // Compute size and position of the pixmap to be grabbed:
    QPoint g = KFTPWidgets::SystemTray::self()->mapToGlobal(KFTPWidgets::SystemTray::self()->pos());
    int desktopWidth  = kapp->desktop()->width();
    int desktopHeight = kapp->desktop()->height();
    int tw = KFTPWidgets::SystemTray::self()->width();
    int th = KFTPWidgets::SystemTray::self()->height();
    int w = desktopWidth / 4;
    int h = desktopHeight / 9;
    int x = g.x() + tw/2 - w/2; // Center the rectange in the systray icon
    int y = g.y() + th/2 - h/2;
    if ( x < 0 )                 x = 0; // Move the rectangle to stay in the desktop limits
    if ( y < 0 )                 y = 0;
    if ( x + w > desktopWidth )  x = desktopWidth - w;
    if ( y + h > desktopHeight ) y = desktopHeight - h;

    // Grab the desktop and draw a circle arround the icon:
    QPixmap shot = QPixmap::grabWindow( qt_xrootwin(),  x,  y,  w,  h );
    QPainter painter( &shot );
    const int MARGINS = 6;
    const int WIDTH   = 3;
    int ax = g.x() - x - MARGINS -1;
    int ay = g.y() - y - MARGINS -1;
    painter.setPen(  QPen( Qt::red,  WIDTH ) );
    painter.drawArc( ax,  ay,  tw + 2*MARGINS,  th + 2*MARGINS,  0,  16*360 );
    painter.end();

    // Associate source to image and show the dialog:
    Q3MimeSourceFactory::defaultFactory()->setPixmap( "systray_shot",  shot );
    KMessageBox::information( this,
    i18n( "<p>Closing the main window will keep KFTPGrabber running in the system tray. "
          "Use <b>Quit</b> from the <b>KFTPGrabber</b> menu to quit the application.</p>"
          "<p><center><img source=\"systray_shot\"></center></p>" ),
    i18n( "Docking in System Tray" ),  "HideMenuBarWarning" );
    hide();

    return false;
  }

  if (KFTPCore::Config::confirmExit() && KFTPQueue::Manager::self()->getNumRunning() > 0) {
    if (KMessageBox::questionYesNo(0, i18n("There is currently a transfer running.",
                                           "There are currently %1 transfers running.",
                                           KFTPQueue::Manager::self()->getNumRunning()) + i18n("\nAre you sure you want to quit?"),
                                      i18n("Quit")) == KMessageBox::No)
    {
      return false;
    }
  }
#endif
  // Save XML bookmarks here, because the user may be prompted for an encryption password
  KFTPBookmarks::Manager::self()->save();

  return true;
}

MainWindow::~MainWindow()
{
}

void MainWindow::initTrafficGraph()
{
  // Setup traffic graph
  m_graphTimer = new QTimer(this);
  connect(m_graphTimer, SIGNAL(timeout()), this, SLOT(slotUpdateTrafficGraph()));
  m_graphTimer->start(1000);

  // Create and configure the traffic graph
  m_trafficGraph = new KFTPWidgets::TrafficGraph(0);
  m_trafficGraph->setObjectName("trafficgraph");
  m_trafficGraph->setShowLabels(true);

  m_trafficGraph->addBeam(QColor(255, 0, 0));
  m_trafficGraph->addBeam(QColor(0, 0, 255));

  m_trafficGraph->repaint();
}

void MainWindow::showBookmarkEditor()
{
  KFTPWidgets::Bookmarks::Editor editor(this);
  editor.exec();
}

void MainWindow::initBookmarkMenu()
{
  QAction *editBookmarks = actionCollection()->addAction("edit_bookmarks");
  editBookmarks->setText(i18n("&Edit Bookmarks..."));
  editBookmarks->setIcon(KIcon("bookmark-new"));
  connect(editBookmarks, SIGNAL(triggered()), this, SLOT(showBookmarkEditor()));
  
  unplugActionList("bookmarks_list");
  plugActionList("bookmarks_list", KFTPBookmarks::Manager::self()->populateBookmarksMenu());
  
  KActionMenu *zeroconfSites = actionCollection()->add<KActionMenu>("zeroconf_sites");
  zeroconfSites->setText(i18n("&FTP Sites Near Me"));
  KFTPBookmarks::Manager::self()->populateZeroconfMenu(zeroconfSites);
  
  if (KFTPCore::Config::showWalletSites()) {
    KActionMenu *kwalletSites = actionCollection()->add<KActionMenu>("kwallet_sites");
    kwalletSites->setText(i18n("Sites in K&Wallet"));
    kwalletSites->setIcon(KIcon("wallet-open"));
    KFTPBookmarks::Manager::self()->populateWalletMenu(kwalletSites);
  }
}

void MainWindow::initStatusBar()
{
  statusBar()->insertItem(i18n("idle"), 1, 1);
  statusBar()->setItemAlignment(1, Qt::AlignLeft);

  statusBar()->insertItem(i18nc("Download traffic rate", "Down: %1/s", KIO::convertSize(KFTPQueue::Manager::self()->getDownloadSpeed())), 2);
  statusBar()->insertItem(i18nc("Upload traffic rate", "Up: %1/s", KIO::convertSize(KFTPQueue::Manager::self()->getUploadSpeed())), 3);
}

void MainWindow::initMainView()
{
  setCentralWidget(new QWidget(this));
  QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget());
  mainLayout->setMargin(0);
  mainLayout->setSpacing(0);

  QSplitter *splitter = new QSplitter(centralWidget());
  mainLayout->addWidget(splitter);

  // Session layouts
  QWidget *leftSessions = new QWidget(splitter);
  QWidget *rightSessions = new QWidget(splitter);

  QHBoxLayout *leftLayout = new QHBoxLayout(leftSessions);
  QHBoxLayout *rightLayout = new QHBoxLayout(rightSessions);
  leftLayout->setMargin(0);
  leftLayout->setSpacing(0);
  rightLayout->setMargin(0);
  rightLayout->setSpacing(0);

  KTabWidget *leftTabs  = new KTabWidget(leftSessions);
  KTabWidget *rightTabs = new KTabWidget(rightSessions);

  leftTabs->setCloseButtonEnabled(true);
  leftTabs->setTabBarHidden(true);
  rightTabs->setCloseButtonEnabled(true);
  rightTabs->setTabBarHidden(true);

  leftLayout->addWidget(leftTabs);
  rightLayout->addWidget(rightTabs);

  // Create the session manager
  QTabWidget *logs = new QTabWidget(0);
  new KFTPSession::Manager(this, logs, leftTabs, rightTabs);
  
  // Create docks
  setDockOptions(QMainWindow::DockOptions(AllowNestedDocks | AllowTabbedDocks | VerticalTabs | AnimatedDocks));
  QDockWidget *queueDock = new QDockWidget(i18n("Queue"));
  queueDock->setObjectName("queue");
  queueDock->setWidget(new KFTPWidgets::QueueView(queueDock));
  
  addDockWidget(Qt::BottomDockWidgetArea, queueDock);
  
  QDockWidget *failedDock = new QDockWidget(i18n("Failed Transfers"));
  failedDock->setObjectName("failed_transfers");
  failedDock->setWidget(new KFTPWidgets::FailedTransfers::View(failedDock));
  
  addDockWidget(Qt::BottomDockWidgetArea, failedDock);
  tabifyDockWidget(queueDock, failedDock);
  
  QDockWidget *logDock = new QDockWidget(i18n("Log"));
  logDock->setObjectName("log");
  logDock->setWidget(logs);
  
  addDockWidget(Qt::BottomDockWidgetArea, logDock);
  tabifyDockWidget(failedDock, logDock);
  
  QDockWidget *graphDock = new QDockWidget(i18n("Traffic"));
  graphDock->setObjectName("traffic");
  graphDock->setWidget(m_trafficGraph);
  
  addDockWidget(Qt::BottomDockWidgetArea, graphDock);
  tabifyDockWidget(logDock, graphDock);
}

void MainWindow::slotUpdateStatusBar()
{
  // Status bar
  statusBar()->changeItem(i18nc("Download traffic rate", "Down: %1/s", KIO::convertSize(KFTPQueue::Manager::self()->getDownloadSpeed())), 2);
  statusBar()->changeItem(i18nc("Upload traffic rate", "Up: %1/s", KIO::convertSize(KFTPQueue::Manager::self()->getUploadSpeed())), 3);
}

void MainWindow::slotUpdateTrafficGraph()
{
  // Update the traffic graph
  if (m_trafficGraph) {
    QList<double> trafficList;
    trafficList.append((double) KFTPQueue::Manager::self()->getDownloadSpeed() / 1024);
    trafficList.append((double) KFTPQueue::Manager::self()->getUploadSpeed() / 1024);

    m_trafficGraph->addSample(trafficList);
  }
}

void MainWindow::slotQuickConnect()
{
  KFTPWidgets::Bookmarks::Editor editor(this, true);
  if (editor.exec())
    KFTPBookmarks::Manager::self()->connectWithSite(editor.selectedSite());
}

void MainWindow::slotConfigChanged()
{
}

void MainWindow::slotModeAscii()
{
  QAction *transferMode = actionCollection()->action("transfer_mode");
  transferMode->setIcon(KIcon("text-x-generic"));
  transferMode->setText(i18n("&Mode (ASCII)"));
  KFTPCore::Config::self()->setGlobalMode('A');
}

void MainWindow::slotModeBinary()
{
  QAction *transferMode = actionCollection()->action("transfer_mode");
  transferMode->setIcon(KIcon("application-octet-stream"));
  transferMode->setText(i18n("&Mode (Binary)"));
  KFTPCore::Config::self()->setGlobalMode('I');
}

void MainWindow::slotModeAuto()
{
  QAction *transferMode = actionCollection()->action("transfer_mode");
  transferMode->setText(i18n("&Mode (Auto)"));
  transferMode->setIcon(KIcon("application-octet-stream"));
  KFTPCore::Config::self()->setGlobalMode('X');
}

void MainWindow::slotFileQuit()
{
  //m_closeApp = true;
  close();
}

void MainWindow::slotSettingsSave()
{
  KFTPCore::Config::self()->saveConfig();
}

void MainWindow::slotSettingsConfig()
{
  if (!m_configDialog)
    m_configDialog = new KFTPWidgets::ConfigDialog(this);

  m_configDialog->prepareDialog();
  m_configDialog->exec();
}

void MainWindow::slotNewSessionLeft()
{
  KFTPSession::Manager::self()->spawnLocalSession(KFTPSession::LeftSide, true);
}

void MainWindow::slotNewSessionRight()
{
  KFTPSession::Manager::self()->spawnLocalSession(KFTPSession::RightSide, true);
}

#include "mainwindow.moc"
