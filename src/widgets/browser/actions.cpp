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

#include "browser/actions.h"
#include "browser/view.h"
#include "browser/detailsview.h"
#include "browser/dirmodel.h"
//#include "browser/propsplugin.h"
//#include "browser/filterwidget.h"

#include "widgets/popupmessage.h"
#include "widgets/bookmarks/editor.h"

#include "kftpbookmarks.h"
#include "kftpqueue.h"
#include "kftpsession.h"
#include "verifier.h"

#include "misc/config.h"
#include "misc/filter.h"
#include "misc/customcommands/manager.h"

#include <kglobal.h>
#include <kcharsets.h>
#include <kapplication.h>
#include <kmainwindow.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmenu.h>
#include <kinputdialog.h>
#include <kpropertiesdialog.h>
#include <kio/job.h>
#include <klineedit.h>
#include <kfiledialog.h>
#include <kstandarddirs.h>
#include <KStandardAction>
#include <KActionCollection>
#include <KToggleAction>

#include <qdir.h>
#include <qclipboard.h>
#include <krandom.h>

#include <QModelIndex>

using namespace KFTPEngine;
using namespace KFTPCore;
using namespace KFTPCore::Filter;

namespace KFTPWidgets {

namespace Browser {

Actions::Actions(View *parent)
 : QObject(parent),
   m_view(parent)
{
}

void Actions::initActions()
{
  KActionCollection *actionCollection = new KActionCollection(m_view);
  
  // Create all the actions
  m_goUpAction = KStandardAction::up(this, SLOT(slotGoUp()), actionCollection);
  m_goBackAction = KStandardAction::back(this, SLOT(slotGoBack()), actionCollection);
  m_goForwardAction = KStandardAction::forward(this, SLOT(slotGoForward()), actionCollection);
  m_goHomeAction = KStandardAction::home(this, SLOT(slotGoHome()), actionCollection);

  m_reloadAction = KStandardAction::redisplay(this, SLOT(slotReload()), actionCollection);
  m_reloadAction->setText(i18n("&Reload"));
  m_reloadAction->setShortcut(KShortcut(Qt::Key_F5));
  /*
  m_abortAction = new KAction(i18n("&Abort"), "process-stop", KShortcut(), this, SLOT(slotAbort()), m_actionCollection, "abort");
  m_toggleTreeViewAction = new KToggleAction(i18n("&Show Tree View"), "view-sidetree", KShortcut(), this, SLOT(slotShowHideTree()), m_actionCollection, "toggle_tree_view");
  m_toggleFilterAction = new KToggleAction(i18n("Show &Filter"), "filter", KShortcut(), this, SLOT(slotShowHideFilter()), m_actionCollection, "toggle_filter");
  
  m_renameAction = new KAction(i18n("&Rename"), KShortcut(Qt::Key_F2), this, SLOT(slotRename()), m_actionCollection, "edit_rename");
  m_deleteAction = new KAction(i18n("&Delete"), "edit-delete", KShortcut(Qt::Key_Delete), this, SLOT(slotDelete()), m_actionCollection, "edit_delete");
  m_propsAction = new KAction(i18n("&Properties"), KShortcut(), this, SLOT(slotProps()), m_actionCollection, "edit_properties");
  m_shredAction = new KAction(i18n("&Shred"), "edit-delete-shred", KShortcut(), this, SLOT(slotShred()), m_actionCollection, "edit_shred");
  
  m_copyAction = KStandardAction::copy(this, SLOT(slotCopy()), m_actionCollection, "edit_copy");
  m_pasteAction = KStandardAction::paste(this, SLOT(slotPaste()), m_actionCollection, "edit_paste");
  
  m_filterActions = new KActionMenu(i18n("&Filter Options"), "", m_actionCollection, "edit_filter_options");
  m_alwaysSkipAction = new KAction(i18n("Always &skip this file when queuing"), KShortcut(), this, SLOT(slotAlwaysSkip()), m_actionCollection);
  m_topPriorityAction = new KAction(i18n("Make this file &top priority"), KShortcut(), this, SLOT(slotTopPriority()), m_actionCollection);
  m_lowPriorityAction = new KAction(i18n("Make this file &lowest priority"), KShortcut(), this, SLOT(slotLowPriority()), m_actionCollection);
  
  m_filterActions->insert(m_alwaysSkipAction);
  m_filterActions->insert(m_topPriorityAction);
  m_filterActions->insert(m_lowPriorityAction);
  
  m_transferAction = new KAction(i18n("&Transfer"), KShortcut(), this, SLOT(slotTransfer()), m_actionCollection, "transfer");
  m_queueTransferAction = new KAction(i18n("&Queue Transfer"), "mail-queue", KShortcut(), this, SLOT(slotQueueTransfer()), m_actionCollection, "queue_transfer");
  m_createDirAction = new KAction(i18n("&Create Directory..."), "folder-new", KShortcut(), this, SLOT(slotCreateDir()), m_actionCollection, "create_dir");
  m_fileEditAction = new KAction(i18n("&Open file"), "document-open", KShortcut(), this, SLOT(slotFileEdit()), m_actionCollection, "open_file");
  m_verifyAction = new KAction(i18n("&Verify..."), "dialog-ok", KShortcut(), this, SLOT(slotVerify()), m_actionCollection, "verify");

  populateEncodings();
  
  m_moreActions = new KActionMenu(i18n("&More Actions"), "configure", this);
  m_rawCmdAction = new KAction(i18n("&Manual Command Entry..."), "utilities-terminal", KShortcut(), this, SLOT(slotRawCmd()), m_actionCollection, "send_raw_cmd");
  m_exportListingAction = new KAction(i18n("&Export Directory Listing..."), "", KShortcut(), this, SLOT(slotExportListing()), m_actionCollection, "export_listing");
  m_showHiddenFilesAction = new KToggleAction(i18n("Show &Hidden Files && Directories"), KShortcut(), this, SLOT(slotShowHiddenFiles()), m_actionCollection, "show_hidden");
  m_openExternalAction = new KAction(i18n("Open current directory in &Konqueror..."), "konqueror", KShortcut(), this, SLOT(slotOpenExternal()), m_actionCollection, "open_konqi");
  
  m_markItemsAction = new KAction(i18n("Compare &selected items"), "", KShortcut(Qt::Key_Space), this, SLOT(slotMarkItems()), m_actionCollection, "compare_selected");
  m_compareAction = new KAction(i18n("Compare &directories"), "", KShortcut(), this, SLOT(slotCompare()), m_actionCollection, "compare_dirs");
  
  m_showHiddenFilesAction->setChecked(KFTPCore::Config::showHiddenFiles());
  
  m_rawCommandsMenu = CustomCommands::Manager::self()->categories(i18n("Send &Raw Command"), m_view->getSession());
  m_rawCommandsMenu->insert(m_rawCmdAction, 0);
  m_rawCommandsMenu->popupMenu()->insertSeparator(1);
  
  m_moreActions->insert(m_rawCommandsMenu);
  m_moreActions->insert(m_changeEncodingAction);
  m_moreActions->popupMenu()->insertSeparator();
  m_moreActions->insert(m_exportListingAction);
  m_moreActions->insert(m_openExternalAction);
  m_moreActions->insert(m_markItemsAction);
  m_moreActions->insert(m_compareAction);
  m_moreActions->popupMenu()->insertSeparator();
  m_moreActions->insert(m_showHiddenFilesAction);
  
  m_moreActions->setStickyMenu(true);
  m_moreActions->setDelayed(false);
  */
  
  m_toggleTreeViewAction = new KToggleAction(this);
  m_toggleTreeViewAction->setText(i18n("&Toggle Tree View"));
  m_toggleTreeViewAction->setIcon(KIcon("view-sidetree"));
  connect(m_toggleTreeViewAction, SIGNAL(triggered()), this, SLOT(slotToggleTree()));
  
  m_createDirAction = new KAction(this);
  m_createDirAction->setText(i18n("&Create Directory..."));
  m_createDirAction->setIcon(KIcon("folder-new"));
  connect(m_createDirAction, SIGNAL(triggered()), this, SLOT(slotCreateDir()));
  
  m_siteChangeAction = new KActionMenu(KIcon("network-server"), i18n("&Change Site"), this);
  m_quickConnectAction = new KAction(this);
  m_quickConnectAction->setText(i18n("&Quick Connect..."));
  m_quickConnectAction->setIcon(KIcon("network-connect"));
  connect(m_quickConnectAction, SIGNAL(triggered()), this, SLOT(slotQuickConnect()));
  
  m_connectAction = new KActionMenu(i18n("&Connect To"), this);
  
  m_disconnectAction = new KAction(this);
  m_disconnectAction->setText(i18n("&Disconnect"));
  m_disconnectAction->setIcon(KIcon("network-disconnect"));
  connect(m_disconnectAction, SIGNAL(triggered()), this, SLOT(slotDisconnect()));

  m_siteChangeAction->addAction(m_quickConnectAction);
  m_siteChangeAction->addAction(m_connectAction);
  m_siteChangeAction->addAction(m_disconnectAction);
  m_siteChangeAction->setStickyMenu(true);
  m_siteChangeAction->setDelayed(false);
  
  actionCollection->addAction("site_change", m_siteChangeAction);
}

void Actions::populateEncodings()
{
  /*
  // Charsets
  m_changeEncodingAction = new KActionMenu(i18n("Change Remote &Encoding"), "charset", m_actionCollection, "changeremoteencoding");
  m_changeEncodingAction->setDelayed(false);
  
  KMenu *menu = m_changeEncodingAction->popupMenu();
  menu->clear();
  
  QStringList charsets = KGlobal::charsets()->descriptiveEncodingNames();
  int count = 0;
  for (QStringList::iterator i = charsets.begin(); i != charsets.end(); ++i)
    menu->insertItem(*i, this, SLOT(slotCharsetChanged(int)), 0, ++count);
  
  menu->insertSeparator();
  menu->insertItem(i18n("Default"), this, SLOT(slotCharsetReset(int)), 0, ++count);
  menu->setItemChecked(count, true);
  
  m_defaultCharsetOption = count;
  m_curCharsetOption = count;
  */
}

void Actions::updateActions()
{
  LocationNavigator *navigator = m_view->locationNavigator();
  
  int index = 0;
  const QList<LocationNavigator::Element> list = navigator->history(index);
  
  m_goUpAction->setEnabled(navigator->url().upUrl() != navigator->url());
  m_goBackAction->setEnabled(index < list.count() - 1);
  m_goForwardAction->setEnabled(index > 0);
  
  m_disconnectAction->setEnabled(m_view->session()->isRemote() && m_view->session()->isConnected());
  /*
  m_goUpAction->setEnabled(m_view->url().upUrl() != m_view->url());

  // History
  int index = 0;
  const Q3ValueList<LocationNavigator::Element> list = m_view->history(index);
  
  m_goBackAction->setEnabled(index < static_cast<int>(list.count()) - 1);
  m_goForwardAction->setEnabled(index > 0);
  
  m_abortAction->setEnabled(m_view->m_ftpClient->socket()->isBusy());
  m_toggleTreeViewAction->setEnabled(true);
  m_toggleFilterAction->setEnabled(true);

  m_quickConnectAction->setEnabled(m_view->url().isLocalFile());
  m_connectAction->setEnabled(true);
  m_disconnectAction->setEnabled(m_view->m_ftpClient->socket()->isConnected());
  
  const KFileItemList *selectedItems = m_view->selectedItems();
  
  if (selectedItems->count() == 1) {
    m_fileEditAction->setEnabled(!selectedItems->getFirst()->isDir());
    m_verifyAction->setEnabled(selectedItems->getFirst()->isLocalFile() && selectedItems->getFirst()->name(true).right(3) == "sfv");
  } else {
    m_fileEditAction->setEnabled(false);
    m_verifyAction->setEnabled(false);
  }
  
  // Check if we can transfer anything
  KFTPSession::Session *session = m_view->m_session;
  KFTPSession::Session *opposite = KFTPSession::Manager::self()->getActive(oppositeSide(m_view->m_session->getSide()));
  
  m_renameAction->setEnabled(session->isConnected());
  m_deleteAction->setEnabled(session->isConnected());
  m_propsAction->setEnabled(true);
  m_shredAction->setEnabled(!session->isRemote());
  m_copyAction->setEnabled(true);
  m_pasteAction->setEnabled(true);
  
  if ((!session->isRemote() && !opposite->isRemote()) ||
      (
        (session->isRemote() && opposite->isRemote()) &&
        (
          session->getClient()->socket()->protocolName() != opposite->getClient()->socket()->protocolName() ||
          !(session->getClient()->socket()->features() & SF_FXP_TRANSFER)
        )
      )
     ) {
    m_queueTransferAction->setEnabled(false);
    m_transferAction->setEnabled(false);
  } else {
    m_queueTransferAction->setEnabled(true);
    m_transferAction->setEnabled(true);
  }

  if (!session->isRemote() || session->getClient()->socket()->isConnected())
    m_createDirAction->setEnabled(true);
  else
    m_createDirAction->setEnabled(false);

  m_changeEncodingAction->setEnabled(session->isRemote());
  m_rawCmdAction->setEnabled(!m_view->url().isLocalFile() && m_view->m_ftpClient->socket()->features() & SF_RAW_COMMAND);
  m_rawCommandsMenu->setEnabled(m_rawCmdAction->isEnabled());
  m_openExternalAction->setEnabled(!session->isRemote());
  */
}

void Actions::slotGoUp()
{
  m_view->goUp();
}

void Actions::slotGoBack()
{
  m_view->goBack();
}

void Actions::slotGoForward()
{
  m_view->goForward();
}

void Actions::slotReload()
{
  m_view->reload();
}

void Actions::slotGoHome()
{
  m_view->goHome();
}

void Actions::slotQuickConnect()
{
  Bookmarks::Editor editor(0, true);
  if (editor.exec())
    KFTPBookmarks::Manager::self()->connectWithSite(editor.selectedSite(), m_view->session());  
}

void Actions::slotDisconnect()
{
  if (m_view->session()->isRemote() && m_view->session()->isConnected()) {
    if (KFTPCore::Config::confirmDisconnects() && KMessageBox::warningYesNo(0, i18n("Do you want to drop current connection?")) == KMessageBox::No)
      return;

    m_view->session()->disconnectAllConnections();
  }
}

void Actions::slotShred()
{
  /*
  // Shred the file
  if (KMessageBox::warningContinueCancel(0, i18n("Are you sure you want to SHRED this file?"), i18n("Shred File"),KGuiItem(i18n("&Shred"), "editshred")) == KMessageBox::Cancel)
    return;
  
  KShred::shred(m_view->selectedItems()->getFirst()->url().path());
  */
}

void Actions::slotRename()
{
  /*
  KFTPWidgets::Browser::DetailsView *view = m_view->getDetailsView();
  
  // Rename the first file in the current selection
  view->rename(view->K3ListView::selectedItems().at(0), 0);
  
  // Enhanced rename: Don't highlight the file extension. (from Konqueror)
  KLineEdit *le = view->renameLineEdit();
  
  if (le) {
    const QString txt = le->text();
    QString pattern;
    KMimeType::diagnoseFileName(txt, pattern);
    
    if (!pattern.isEmpty() && pattern.at(0) == '*' && pattern.find('*',1) == -1)
      le->setSelection(0, txt.length()-pattern.trimmed().length()+1);
    else {
      int lastDot = txt.findRev('.');
      
      if (lastDot > 0)
        le->setSelection(0, lastDot);
    }
  }
  */
}

void Actions::slotDelete()
{
  /*
  KFTPSession::Session *session = m_view->getSession();
  
  // Delete a file or directory
  KUrl::List selection = m_view->selectedURLs();
  KUrl::List::ConstIterator i = selection.begin();
  QStringList prettyList;
  for (; i != selection.end(); ++i) {
    prettyList.append((*i).pathOrUrl());
  }

  if (KMessageBox::warningContinueCancelList(0,
                                            i18n("Do you really want to delete this item?", "Do you really want to delete these %n items?", prettyList.count()),
                                            prettyList,
                                            i18n("Delete Files"),
                                            KStandardGuiItem::del(),
                                            QString::null,
                                            KMessageBox::Dangerous) == KMessageBox::Cancel)
    return;

  // Go trough all files and delete them
  if (!session->isRemote()) {
    KIO::del(selection);
  } else {
    KUrl::List::Iterator end(selection.end());
    
    for (KUrl::List::Iterator i(selection.begin()); i != end; ++i) {
      if (!(*i).isLocalFile())
        session->getClient()->remove(KUrl((*i).url()));
    }
  }
  */
}

void Actions::slotCopy()
{
  /*
  QClipboard *cb = QApplication::clipboard();
  cb->setData(m_view->getDetailsView()->dragObject(), QClipboard::Clipboard);
  */
}

void Actions::slotPaste()
{
  /*
  // Decode the data and try to init transfer
  KIO::MetaData p_meta;
  KUrl::List p_urls;
  
  if (KURLDrag::decode(QApplication::clipboard()->data(), p_urls, p_meta)) {
    // Add destination url and call the QueueManager
    p_meta.insert("DestURL", m_view->url().url());
    KURLDrag *drag = new KURLDrag(p_urls, p_meta, m_view, name());
    KFTPQueue::Manager::self()->insertTransfer(drag);
  }
  */
}

void Actions::slotProps()
{
  /*
  // Show file properties
  const KFileItemList *selectedItems = m_view->selectedItems();
  KFileItem *item = selectedItems->getFirst();
  
  if (selectedItems->count() == 0) {
    if (m_view->url().isLocalFile())
      item = new KFileItem(m_view->url(), 0, 0);
    else
      return;
  }

  // Show the dialog
  KPropertiesDialog *propsDialog;
  
  if (item->isLocalFile()) {
    if (selectedItems->count() == 0)
      propsDialog = new KPropertiesDialog(item);
    else
      propsDialog = new KPropertiesDialog(*selectedItems);
  } else {
    propsDialog = new KPropertiesDialog(item->name());
    propsDialog->insertPlugin(new KFTPWidgets::Browser::PropsPlugin(propsDialog, *selectedItems));
    propsDialog->insertPlugin(new KFTPWidgets::Browser::PermissionsPropsPlugin(propsDialog, *selectedItems, m_view->getSession()));
  }
  
  propsDialog->exec();
  */
}

void Actions::addPriorityItems(int priority)
{
  /*
  // Add the files to skiplist
  KUrl::List selection = m_view->selectedURLs();
  KUrl::List::Iterator end(selection.end());
    
  for (KUrl::List::Iterator i(selection.begin()); i != end; ++i) {
    Rule *rule = new Rule();
    
    if (priority == 0) {
      rule->setName(i18n("Skip '%1'",(*i).filename()));
      const_cast<ConditionChain*>(rule->conditions())->append(new Condition(Filename, Condition::Is, (*i).filename()));
      const_cast<ActionChain*>(rule->actions())->append(new Action(Action::Skip, QVariant()));
    } else {
      rule->setName(i18n("Priority '%1'",(*i).filename()));
      const_cast<ConditionChain*>(rule->conditions())->append(new Condition(Filename, Condition::Is, (*i).filename()));
      const_cast<ActionChain*>(rule->actions())->append(new Action(Action::Priority, priority));
    }
    
    Filters::self()->append(rule);
  }
  */
}

void Actions::slotAlwaysSkip()
{
  addPriorityItems(0);
}

void Actions::slotTopPriority()
{
  addPriorityItems(1);
}

void Actions::slotLowPriority()
{
  addPriorityItems(-1);
}

void Actions::slotTransfer()
{
  /*
  // Queue a transfer
  KFileItemList list(*m_view->selectedItems());
  KFileItemListIterator i(list);
  KFileItem *item;  
  KFTPSession::Session *opposite = KFTPSession::Manager::self()->getActive(oppositeSide(m_view->m_session->getSide()));
  KFTPQueue::Transfer *transfer = 0L;

  while ((item = i.current()) != 0) {
    KUrl destinationUrl = opposite->getFileView()->url();
    destinationUrl.addPath(item->name());

    transfer = KFTPQueue::Manager::self()->spawnTransfer(
      item->url(),
      destinationUrl,
      item->size(),
      item->isDir(),
      list.count() == 1,
      true,
      0L,
      true
    );
    
    ++i;
  }

  // Execute transfer
  if (transfer)
    static_cast<KFTPQueue::Site*>(transfer->parentObject())->delayedExecute();
  */
}

void Actions::slotQueueTransfer()
{
  KFTPSession::Session *oppositeSession = m_view->session()->oppositeSession();
  QModelIndexList indexes = m_view->selectedIndexes();
  KUrl oppositeUrl = oppositeSession->getFileView()->locationNavigator()->url();
  
  foreach (QModelIndex index, indexes) {
    KFileItem item = index.data(DirModel::FileItemRole).value<KFileItem>();
    KUrl destinationUrl = oppositeUrl;
    destinationUrl.addPath(item.name());
    
    KFTPQueue::Manager::self()->spawnTransfer(
      item.url(),
      destinationUrl,
      item.size(),
      item.isDir(),
      indexes.count() == 1,
      true,
      0,
      indexes.count() > 1
    );
  }
  
  /*
  // Queue a transfer
  KFileItemList list(*m_view->selectedItems());
  KFileItemListIterator i(list);
  KFileItem *item;  
  KFTPSession::Session *opposite = KFTPSession::Manager::self()->getActive(oppositeSide(m_view->m_session->getSide()));

  while ((item = i.current()) != 0) {
    KUrl destinationUrl = opposite->getFileView()->url();
    destinationUrl.addPath(item->name());

    KFTPQueue::Manager::self()->spawnTransfer(
      item->url(),
      destinationUrl,
      item->size(),
      item->isDir(),
      list.count() == 1,
      true,
      0L,
      list.count() > 1
    );
    
    ++i;
  }
  */
}

void Actions::slotCreateDir()
{
  /*
  // Create new directory
  bool ok;
  QString newDirName = KInputDialog::getText(i18n("Create Directory"), i18n("Directory name:"), "", &ok);
  
  if (ok) {
    KUrl url = m_view->url();
    url.addPath(newDirName);
    
    if (url.isLocalFile())
      KIO::mkdir(url);
    else
      m_view->m_ftpClient->mkdir(url);
  }
  */
}

void Actions::slotFileEdit()
{
  /*
  KFileItem *item = m_view->selectedItems()->getFirst();
  
  if (!item->isDir()) {
    if (item->isLocalFile()) {
      item->run();
    } else {
      // Create a new transfer to download the file and open it
      KFTPQueue::TransferFile *transfer = new KFTPQueue::TransferFile(KFTPQueue::Manager::self());
      transfer->setSourceUrl(item->url());
      transfer->setDestUrl(KUrl(KGlobal::dirs()->saveLocation("tmp") + QString("%1-%2").arg(KRandom::randomString(7)).arg(item->name())));
      transfer->addSize(item->size());
      transfer->setTransferType(KFTPQueue::Download);
      transfer->setOpenAfterTransfer(true);
      KFTPQueue::Manager::self()->insertTransfer(transfer);
      
      // Execute the transfer
      transfer->delayedExecute();
    }
  }
  */
}

void Actions::slotAbort()
{
  /*
  KFTPSession::Session *session = KFTPSession::Manager::self()->find(m_view);

  // Abort the session
  if (session)
    session->abort();
  */
}

void Actions::slotRawCmd()
{
  /*
  bool ok;
  QString rawCmd = KInputDialog::getText(i18n("Send Raw Command"), i18n("Command:"), "", &ok);

  if (ok)
    m_view->m_ftpClient->raw(rawCmd);
  */
}

void Actions::slotToggleTree()
{
  m_view->setTreeVisible(m_toggleTreeViewAction->isChecked());
  m_view->m_treeVisibilityChanged = true;
}

void Actions::slotToggleFilter()
{
  /*
  if (m_toggleFilterAction->isChecked()) {
    m_view->m_searchToolBar->show();
    m_view->m_searchFilter->clear();
    m_view->m_searchFilter->setFocus();
  } else {
    m_view->m_searchFilter->clear();
    m_view->m_searchToolBar->hide();
  }
  */
}

void Actions::slotCharsetChanged(int id)
{
  /*
  if (!m_changeEncodingAction->popupMenu()->isItemChecked(id)) {
    QStringList charsets = KGlobal::charsets()->descriptiveEncodingNames();
    QString charset = KGlobal::charsets()->encodingForName(charsets[id - 1]);
    
    // Set the current socket's charset
    m_view->m_ftpClient->socket()->changeEncoding(charset);
    
    // Update checked items
    m_changeEncodingAction->popupMenu()->setItemChecked(id, true);
    m_changeEncodingAction->popupMenu()->setItemChecked(m_curCharsetOption, false);
    m_curCharsetOption = id;
  }
  */
}

void Actions::slotCharsetReset(int id)
{
  /*
  // Revert to default charset if possible
  KFTPBookmarks::Site *site = m_view->m_session->getSite();
  
  if (site) {
    // Set the current socket's charset
    m_view->m_ftpClient->socket()->changeEncoding(site->getProperty("encoding"));
    
    // Update checked items
    m_changeEncodingAction->popupMenu()->setItemChecked(id, true);
    m_changeEncodingAction->popupMenu()->setItemChecked(m_curCharsetOption, false);
    m_curCharsetOption = id;
  }
  */
}

void Actions::slotExportListing()
{
  /*
  QString savePath = KFileDialog::getSaveFileName(QString::null, i18n("*.txt|Directory Dump"), 0, i18n("Export Directory Listing"));
  
  if (!savePath.isEmpty()) {
    QFile file(savePath);
    
    if (!file.open(QIODevice::WriteOnly))
      return;
      
    Q3TextStream stream(&file);
      
    KFileItemList list(*m_view->items());
    KFileItemListIterator i(list);
    KFileItem *item;
    
    while ((item = i.current()) != 0) {
      stream << item->permissionsString() << "\t";
      stream << item->user() << "\t" << item->group() << "\t";
      stream << item->timeString() << "\t";
      stream << item->name() << "\t";
      stream << "\n";
      
      ++i;
    }
      
    file.flush();
    file.close();
  }
  */
}

void Actions::slotVerify()
{
  /*
  KFTPWidgets::Verifier *verifier = new KFTPWidgets::Verifier();
  verifier->setFile(m_view->selectedItems()->getFirst()->url().path());
  verifier->exec();
  
  delete verifier;
  */
}

void Actions::slotShowHiddenFiles()
{
  /*
  m_view->setShowHidden(m_showHiddenFilesAction->isChecked());
  m_view->reload();
  */
}

void Actions::slotOpenExternal()
{
  /*
  KFileItem *folder = new KFileItem(m_view->url(), "inode/directory", S_IFDIR);
  folder->run();
  */
}

void Actions::slotMarkItems()
{
  /*
  KFileItemList list(*m_view->selectedItems());
  KFileItemListIterator i(list);
  KFileItem *item;  
  KFTPSession::Session *opposite = KFTPSession::Manager::self()->getActive(oppositeSide(m_view->m_session->getSide()));
  
  DetailsView *tView = m_view->getDetailsView();
  DetailsView *oView = opposite->getFileView()->getDetailsView();
  
  while ((item = i.current()) != 0) {
    tView->markItem(item);
    oView->markItem(item->name());
    ++i;
  }
  */
}

void Actions::slotCompare()
{
  /*
  KFTPSession::Session *opposite = KFTPSession::Manager::self()->getActive(oppositeSide(m_view->m_session->getSide()));
  
  DetailsView *tView = m_view->getDetailsView();
  DetailsView *oView = opposite->getFileView()->getDetailsView();
  
  // All items in the other list view should be visible by default
  Q3ListViewItemIterator j(oView);
  while (j.current()) {
    KFileItem *oItem = static_cast<KFileListViewItem*>(*j)->fileInfo();
    oView->setItemVisibility(oItem, true);
    
    ++j;
  }
  
  // Compare the two listviews
  Q3ListViewItemIterator i(tView);
  while (i.current()) {
    KFileItem *tItem = static_cast<KFileListViewItem*>(*i)->fileInfo();
    
    if (tItem) {
      KFileItem *oItem = oView->fileItem(tItem->name());
      
      if (oItem && (oItem->size() == tItem->size() || oItem->isDir())) {
        tView->setItemVisibility(tItem, false);
        oView->setItemVisibility(oItem, false);
      } else {
        tView->setItemVisibility(tItem, true);
      }
    }
    
    ++i;
  }
  
  PopupMessage *popup = new PopupMessage(m_view->getStatusLabel(), m_view);
  popup->setText(i18n("Identical files on both sides have been hidden. Only <b>different files</b> are now visible."));
  popup->setImage(SmallIcon("dialog-information"));
  popup->setShowCloseButton(false);
  popup->setShowCounter(false);
  
  popup->reposition();
  popup->display();
  */
}

}

}

#include "actions.moc"
