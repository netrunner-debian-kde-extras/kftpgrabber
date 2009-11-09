/*
 * This file is part of the KFTPGrabber project
 *
 * Copyright (C) 2003-2007 by the KFTPGrabber developers
 * Copyright (C) 2003-2007 Jernej Kos <kostko@jweb-network.net>
 * Copyright (C) 2006 David Faure
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
#include "browser/dirmodel.h"
#include "browser/dirlister.h"

#include "kftpsession.h"

#include <KFileItem>
#include <KDateTime>
#include <KIcon>
#include <KLocale>
#include <kglobal.h>
#include <KUrl>
#include <QMimeData>
#include <QFile>
#include <QFileInfo>
#include <QDir>

#include <sys/types.h>
#include <dirent.h>

namespace KFTPWidgets {

namespace Browser {

class DirModelNode;
class DirModelDirNode;

/**
 * An internal node representation. Used to construct a virtual tree
 * that is used by the model.
 */
class DirModelNode
{
public:
    /**
     * Possible node types:
     *   FilesystemEntry - an actual entry in the filesystem
     *   CustomEntry - a "fake" shortcut entry
     */
    enum Type {
      FilesystemEntry,
      CustomEntry
    };
    
    /**
     * Constructs a new filesystem entry node.
     *
     * @param parent Parent directory node
     * @param item The KFileItem describing the file
     */
    DirModelNode(DirModelDirNode *parent, const KFileItem &item)
      : m_parent(parent),
        m_item(item),
        m_preview(),
        m_type(FilesystemEntry),
        m_dirty(false)
    {
    }
    
    /**
     * Constructs a new custom entry node.
     *
     * @param parent Parent directory node
     * @param url Shortcut destination URL
     * @param icon Wanted item icon
     * @param text Wanted item text
     */
    DirModelNode(DirModelDirNode *parent, const KUrl &url, QIcon icon, const QString &text)
      : m_parent(parent),
        m_item(url, "inode/directory", S_IFDIR),
        m_preview(),
        m_icon(icon),
        m_text(text),
        m_type(CustomEntry),
        m_dirty(false)
    {
    }
    
    /**
     * Returns the underlying KFileItem.
     */
    KFileItem item() const { return m_item; }
    
    /**
     * Returns the node's parent node.
     */
    DirModelDirNode *parent() const { return m_parent; }
    
    /**
     * Returns the row number relative to the parent node.
     */
    int rowNumber() const;
    
    QIcon preview() const { return m_preview; }
    QIcon icon() const { return m_icon; }
    QString text() const { return m_text; }
    Type type() const { return m_type; }
    bool dirty() const { return m_dirty; }
    
    void addPreview(const QPixmap &pix) { m_preview.addPixmap(pix); }
    void setPreview(const QIcon &icn) { m_preview = icn; }
    
    /**
     * Changes this node's file item.
     *
     * @param item New file item
     */
    void setFileItem(const KFileItem &item) { m_item = item; }
    
    /**
     * Sets the dirty state of this file node.
     */
    void setDirty(bool value) { m_dirty = value; }
private:
    DirModelDirNode *m_parent;
    KFileItem m_item;
    QIcon m_preview;
    QIcon m_icon;
    QString m_text;
    Type m_type;
    bool m_dirty;
};

/**
 * An internal directory node that can contain other nodes.
 */
class DirModelDirNode : public DirModelNode
{
public:
    /**
     * Class constructor.
     *
     * @see DirModelNode::DirModelNode
     */
    DirModelDirNode(DirModelDirNode *parent, const KFileItem &item)
      : DirModelNode(parent, item),
        m_childNodes(),
        m_childCount(DirModel::ChildCountUnknown),
        m_populated(false)
    {
    }
    
    /**
     * Class constructor.
     *
     * @see DirModelNode::DirModelNode
     */
    DirModelDirNode(DirModelDirNode *parent, const KUrl &url, QIcon icon, const QString &text)
      : DirModelNode(parent, url, icon, text),
        m_childNodes(),
        m_childCount(DirModel::ChildCountUnknown),
        m_populated(false)
    {
    }
    
    /**
     * Class destructor.
     */
    ~DirModelDirNode()
    {
      qDeleteAll(m_childNodes);
    }
    
    QList<DirModelNode*> m_childNodes;
    QHash<QString, DirModelNode*> m_childHash;
    
    int childCount() const { return m_childNodes.isEmpty() ? m_childCount : m_childNodes.count(); }
    void setChildCount(int count) { m_childCount = count; }
    bool isPopulated() const { return m_populated; }
    void setPopulated(bool populated) { m_populated = populated; }
    
    void append(DirModelNode *node);
    void remove(int row);
private:
    int m_childCount;
    bool m_populated;
};

int DirModelNode::rowNumber() const
{
  if (!m_parent)
    return 0;
  
  return m_parent->m_childNodes.indexOf(const_cast<DirModelNode*>(this));
}

void DirModelDirNode::append(DirModelNode *node)
{
  m_childNodes.append(node);
  
  if (node->type() == FilesystemEntry)
    m_childHash.insert(node->item().name(), node);
}

void DirModelDirNode::remove(int row)
{
  DirModelNode *node = m_childNodes.takeAt(row);
  
  if (node->type() == FilesystemEntry)
    m_childHash.remove(node->item().name());
  
  delete node;
}

class DirModelPrivate
{
public:
    DirModelPrivate(DirModel* model)
      : q(model), m_dirLister(0),
        m_rootNode(new DirModelDirNode(0, KFileItem())),
        m_dropsAllowed(DirModel::NoDrops),
        m_treeViewBehavior(false)
    {
    }
    
    ~DirModelPrivate()
    {
      delete m_rootNode;
    }
    
    void clear()
    {
      delete m_rootNode;
      m_rootNode = new DirModelDirNode(0, KFileItem());
      
      if (m_treeViewBehavior) {
        q->beginInsertRows(QModelIndex(), 0, 0);
        
        // Create the virtual root node
        QString text;
        KUrl url;
        KIcon icon;
        
        if (m_dirLister->session()->isRemote()) {
          url = m_dirLister->session()->getUrl();
          url.setPath("/");
          text = url.host();
          icon = KIcon("network-wired");
        } else {
          url = KUrl("file:///");
          text = i18n("Root Directory");
          icon = KIcon("computer");
        }
        
        m_slashNode = new DirModelDirNode(m_rootNode, url, icon, text);
        m_rootNode->append(m_slashNode);
        m_toplevelUrl = url;
        
        q->endInsertRows();
      } else {
        m_slashNode = m_rootNode;
      }
    }
    
    QPair<int, DirModelNode*> createStubs(const KUrl &_url) const;
    QPair<int, DirModelNode*> nodeForUrl(const KUrl& url) const;
    DirModelNode *nodeForIndex(const QModelIndex& index) const;
    QModelIndex indexForNode(DirModelNode* node, int rowNumber = -1) const;
    
    bool isDir(DirModelNode *node) const
    {
      return (node == m_rootNode) || node->item().isDir();
    }

    DirModel *q;
    DirLister *m_dirLister;
    DirModelDirNode *m_rootNode;
    DirModelDirNode *m_slashNode;
    DirModel::DropsAllowed m_dropsAllowed;
    bool m_treeViewBehavior;
    KUrl m_toplevelUrl;
};

QPair<int, DirModelNode*> DirModelPrivate::createStubs(const KUrl &_url) const
{
  KUrl url(_url);
  url.adjustPath(KUrl::RemoveTrailingSlash);
  
  const QString urlStr = url.path();
  DirModelDirNode *dirNode = m_slashNode;
  KUrl nodeUrl = m_toplevelUrl;
  
  foreach (QString p, urlStr.split('/', QString::SkipEmptyParts)) {
    DirModelNode *node = dirNode->m_childHash[p];
    
    if (!node) {
      // Create a stub entry
      KUrl tmp = nodeUrl;
      tmp.cd(p);
      
      const int rowCount = dirNode->m_childNodes.count();
      q->beginInsertRows(indexForNode(dirNode), rowCount, rowCount);
      
      DirModelDirNode *stub = new DirModelDirNode(dirNode, KFileItem(tmp, "inode/directory", S_IFDIR));
      dirNode->append(stub);
      dirNode = stub;
      
      q->endInsertRows();
    } else {
      dirNode = static_cast<DirModelDirNode*>(node);
    }
    
    nodeUrl = dirNode->item().url();
  }
  
  return qMakePair(0, static_cast<DirModelNode*>(dirNode));
}

QPair<int, DirModelNode*> DirModelPrivate::nodeForUrl(const KUrl& _url) const // O(n*m)
{
  KUrl url(_url);
  url.adjustPath(KUrl::RemoveTrailingSlash);
  
  if (url == m_toplevelUrl)
    return qMakePair(0, static_cast<DirModelNode*>(m_slashNode));
  
  const QString pathStr = url.path();
  DirModelDirNode *dirNode = m_slashNode;
  KUrl nodeUrl = m_toplevelUrl;
  
  if (!pathStr.startsWith(nodeUrl.path()))
    return qMakePair(0, static_cast<DirModelNode*>(0));
  
  while (nodeUrl != url) {
    Q_ASSERT(pathStr.startsWith(nodeUrl.path()));
    bool foundChild = false;
    QList<DirModelNode*>::ConstIterator it = dirNode->m_childNodes.constBegin();
    const QList<DirModelNode*>::ConstIterator end = dirNode->m_childNodes.constEnd();
    int row = 0;
    
    for (; it != end ; ++it, ++row) {
      const KUrl u = (*it)->item().url();
      if (u == url)
        return qMakePair(row, *it);
      
      if (pathStr.startsWith(u.path() + '/')) {
        Q_ASSERT(isDir(*it));
        dirNode = static_cast<DirModelDirNode*>(*it);
        foundChild = true;
        break;
      }
    }
    
    if (!foundChild)
      return qMakePair(0, static_cast<DirModelNode*>(0));
    
    nodeUrl = dirNode->item().url();
  }
  
  return qMakePair(0, static_cast<DirModelNode*>(0));
}

// node -> index. If rowNumber is set (or node is root): O(1). Otherwise: O(n).
QModelIndex DirModelPrivate::indexForNode(DirModelNode* node, int rowNumber) const
{
  if (node == m_rootNode)
    return QModelIndex();
  
  Q_ASSERT(node->parent());
  return q->createIndex(rowNumber == -1 ? node->rowNumber() : rowNumber, 0, node);
}

// index -> node. O(1)
DirModelNode* DirModelPrivate::nodeForIndex(const QModelIndex& index) const
{
  return index.isValid() ? static_cast<DirModelNode*>(index.internalPointer()) : m_rootNode;
}

DirModel::DirModel(DirLister *lister, QObject *parent)
  : QAbstractItemModel(parent),
    d(new DirModelPrivate(this))
{
  setDirLister(lister);
}

DirModel::~DirModel()
{
  delete d;
}

void DirModel::setTreeViewBehavior(bool value)
{
  d->m_treeViewBehavior = value;
  d->clear();
}

void DirModel::setDirLister(DirLister *lister)
{
  if (d->m_dirLister) {
    d->clear();
    delete d->m_dirLister;
  }
  
  d->m_dirLister = lister;
  
  if (!d->m_dirLister->parent())
    d->m_dirLister->setParent(this);
  
  // Connect the lister
  connect(d->m_dirLister, SIGNAL(newItems(KFileItemList)), this, SLOT(slotNewItems(KFileItemList)));
  connect(d->m_dirLister, SIGNAL(deleteItem(const KFileItem&)), this, SLOT(slotDeleteItem(const KFileItem&)));
  connect(d->m_dirLister, SIGNAL(refreshItems(const QList<QPair<KFileItem, KFileItem> >&)), this, SLOT(slotRefreshItems(const QList<QPair<KFileItem, KFileItem> >&)));
  connect(d->m_dirLister, SIGNAL(clear()), this, SLOT(slotClear()));
  connect(d->m_dirLister, SIGNAL(siteChanged(const KUrl&)), this, SLOT(slotUnconditionalClear()));
}

DirLister *DirModel::dirLister() const
{
  return d->m_dirLister;
}

void DirModel::slotNewItems(const KFileItemList &items)
{
  if (!items.count())
    return;
  
  KUrl dir(items.first().url().upUrl());
  dir.adjustPath(KUrl::RemoveTrailingSlash);
  
  if (!d->m_treeViewBehavior) {
    if (d->m_dirLister->ignoringChanges())
      return;
    
    d->m_toplevelUrl = dir;
  }
  
  QPair<int, DirModelNode*> result = d->nodeForUrl(dir); // O(n*m)
  
  if (!result.second && d->m_treeViewBehavior) {
    // There is no parent node, but stubs have been enabled so we can
    // create one on the fly.
    result = d->createStubs(dir);
  }
  
  Q_ASSERT(result.second);
  Q_ASSERT(d->isDir(result.second));
  DirModelDirNode *dirNode = static_cast<DirModelDirNode*>(result.second);
  
  if (d->m_treeViewBehavior) {
    // Mark all children as clean
    foreach (DirModelNode *node, dirNode->m_childNodes)
      node->setDirty(false);
  }

  const QModelIndex index = d->indexForNode(dirNode, result.first); // O(1)
  int newItemsCount = items.count();
  
  // Determine how many rows are actually new
  KFileItemList::ConstIterator it = items.begin();
  const KFileItemList::ConstIterator end = items.end();
  
  for (; it != end; ++it) {
    if (dirNode->m_childHash.contains((*it).name()) || (d->m_treeViewBehavior && !(*it).isDir()))
      newItemsCount--;
  }
  
  // Process the items - append or overwrite
  int newRowCount = dirNode->m_childNodes.count() + newItemsCount;
  
  if (newItemsCount > 0)
    beginInsertRows(index, dirNode->m_childNodes.count(), newRowCount - 1);
  
  for (it = items.begin(); it != end ; ++it) {
    if (d->m_treeViewBehavior && !(*it).isDir())
      continue;
    
    DirModelNode *node = dirNode->m_childHash.value((*it).name());
    
    if (!node) {
      node = (*it).isDir() ? new DirModelDirNode(dirNode, *it) : new DirModelNode(dirNode, *it);
      node->setDirty(true);
      dirNode->append(node);
    } else {
      node->setFileItem(*it);
      node->setDirty(true);
    }
  }
  
  if (newItemsCount > 0)
    endInsertRows();
  
  if (d->m_treeViewBehavior) {
    // Remove any nodes that are not dirty
    for (int i = 0; i < dirNode->m_childNodes.size(); i++) {
      if (!dirNode->m_childNodes.at(i)->dirty()) {
        dirNode->remove(i);
        
        beginRemoveRows(index, i, i);
        endRemoveRows();
        
        i--;
      }
    }
  }
}

void DirModel::slotDeleteItem(const KFileItem &item)
{
  const QPair<int, DirModelNode*> result = d->nodeForUrl(item.url()); // O(n*m)
  const int rowNumber = result.first;
  DirModelNode *node = result.second;
  
  if (!node)
    return;

  DirModelDirNode *dirNode = node->parent();
  if (!dirNode)
    return;
  
  dirNode->remove(rowNumber);

  QModelIndex parentIndex = d->indexForNode(dirNode); // O(n)
  beginRemoveRows(parentIndex, rowNumber, rowNumber);
  endRemoveRows();
}

void DirModel::slotRefreshItems(const QList<QPair<KFileItem, KFileItem> > &items)
{
  QModelIndex topLeft, bottomRight;
  
  for (QList<QPair<KFileItem, KFileItem> >::ConstIterator fit = items.begin(), fend = items.end(); fit != fend; ++fit) {
    const QModelIndex index = indexForUrl((*fit).first.url()); // O(n*m); maybe we could look up to the parent only once
    d->nodeForIndex(index)->setFileItem((*fit).second);
    
    if (!topLeft.isValid() || index.row() < topLeft.row())
      topLeft = index;
    
    if (!bottomRight.isValid() || index.row() > bottomRight.row())
      bottomRight = index;
  }
  
  bottomRight = bottomRight.sibling(bottomRight.row(), ColumnCount-1);
  emit dataChanged(topLeft, bottomRight);
}

void DirModel::slotClear()
{
  if (!d->m_treeViewBehavior)
    slotUnconditionalClear();
}

void DirModel::slotUnconditionalClear()
{
  const int numRows = d->m_rootNode->m_childNodes.count();
  beginRemoveRows(QModelIndex(), 0, numRows);
  endRemoveRows();
  
  d->clear();
}

void DirModel::itemChanged(const QModelIndex& index)
{
  emit dataChanged(index, index);
}

int DirModel::columnCount(const QModelIndex &) const
{
  return ColumnCount;
}

QVariant DirModel::data(const QModelIndex &index, int role) const
{
  if (index.isValid()) {
    DirModelNode *node = static_cast<DirModelNode*>(index.internalPointer());
    const KFileItem item = node->item();
    
    if (node->type() == DirModelNode::FilesystemEntry) {
      switch (role) {
        case Qt::DisplayRole: {
          switch (index.column()) {
            case Name: return item.text();
            case Size:
                // FIXME
                //return KIO::convertSize(item->size());
                // Default to "file size in bytes" like in kde3's filedialog
                return KGlobal::locale()->formatNumber(item.size(), 0);
            case ModifiedTime: return item.timeString(KFileItem::ModificationTime);
            case Permissions: return item.permissionsString();
            case Owner: return item.user();
            case Group: return item.group();
          }
          break;
        }
        case Qt::DecorationRole: {
          if (index.column() == Name) {
            // FIXME is this really needed ? probably not
            if (!node->preview().isNull())
              return node->preview();
            
            QStringList overlays = item.overlays();
            return KIcon(item.iconName(), 0, overlays);
          }
          break;
        }
        case FileItemRole: return QVariant::fromValue(item);
        case ChildCountRole: {
          if (!item.isDir()) {
            return ChildCountUnknown;
          } else {
            DirModelDirNode *dirNode = static_cast<DirModelDirNode *>(node);
            
            int count = dirNode->childCount();
            if (count == ChildCountUnknown && item.isReadable()) {
              const QString path = item.localPath();
              
              // Only for local directories
              if (!path.isEmpty()) {
                DIR *dir = ::opendir(QFile::encodeName(path));
                if (dir) {
                  count = 0;
                  struct dirent *dirEntry = 0;
                  
                  while ((dirEntry = ::readdir(dir))) {
                    if (dirEntry->d_name[0] == '.') {
                      // Skip '.'
                      if (dirEntry->d_name[1] == '\0')
                        continue;
                      
                      // Skip '..'
                      if (dirEntry->d_name[1] == '.' && dirEntry->d_name[2] == '\0')
                        continue;
                    }
                    
                    count++;
                  }
                  
                  ::closedir(dir);
                }
                
                dirNode->setChildCount(count);
              }
            }
            
            return count;
          }
        }
      }
    } else if (node->type() == DirModelNode::CustomEntry) {
      switch (role) {
        case Qt::DisplayRole: return node->text();
        case Qt::DecorationRole: return node->icon();
        case FileItemRole: return QVariant::fromValue(item);
      }
    }
  }
  
  return QVariant();
}

void DirModel::sort(int column, Qt::SortOrder order)
{
  // FIXME Not implemented - we should probably use QSortFilterProxyModel instead.
  return QAbstractItemModel::sort(column, order);
}

bool DirModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  switch (role) {
    case Qt::DisplayRole:
        // TODO handle renaming here?
        break;
    case Qt::DecorationRole: {
      if (index.column() == Name) {
        DirModelNode *node = static_cast<DirModelNode*>(index.internalPointer());
        
        if (value.type() == QVariant::Icon) {
          const QIcon icon(qvariant_cast<QIcon>(value));
          node->setPreview(icon);
        } else if (value.type() == QVariant::Pixmap) {
          node->addPreview(qvariant_cast<QPixmap>(value));
        }
        
        emit dataChanged(index, index);
        return true;
      }
      
      break;
    }
    default: break;
  }
  
  return false;
}

int DirModel::rowCount(const QModelIndex &parent) const
{
  DirModelDirNode* parentNode = static_cast<DirModelDirNode *>(d->nodeForIndex(parent));
  return parentNode->m_childNodes.count();
}

QModelIndex DirModel::parent(const QModelIndex &index) const
{
  if (!index.isValid())
    return QModelIndex();
  
  DirModelNode* childNode = static_cast<DirModelNode*>(index.internalPointer());
  DirModelNode* parentNode = childNode->parent();
  
  return d->indexForNode(parentNode); // O(n)
}

QStringList DirModel::mimeTypes() const
{
  return QStringList() << QLatin1String("text/uri-list")
                        << QLatin1String( "application/x-kde-cutselection" ) // TODO
                        << QLatin1String( "text/plain" )
                        << QLatin1String( "application/x-kde-urilist" );
}

QMimeData *DirModel::mimeData(const QModelIndexList &indexes) const
{
  KUrl::List urls;
  foreach (const QModelIndex &index, indexes) {
    urls << d->nodeForIndex(index)->item().url();
  }
  
  QMimeData *data = new QMimeData();
  urls.populateMimeData(data);
  return data;
}

KFileItem DirModel::itemForIndex(const QModelIndex &index) const
{
  if (!index.isValid()) {
    return *d->m_dirLister->rootItem();
  } else {
    KFileItem item = static_cast<DirModelNode*>(index.internalPointer())->item();
    return item;
  }
}

QModelIndex DirModel::indexForItem(const KFileItem *item) const
{
  return indexForUrl(item->url()); // O(n*m)
}

QModelIndex DirModel::indexForItem(const KFileItem &item) const
{
  return indexForUrl(item.url()); // O(n*m)
}

QModelIndex DirModel::indexForUrl(const KUrl &url) const
{
  const QPair<int, DirModelNode*> result = d->nodeForUrl(url); // O(n*m) (m is the depth from the root)
  
  if (!result.second)
    return QModelIndex();
  
  return d->indexForNode(result.second, result.first); // O(1)
}

QModelIndex DirModel::index(int row, int column, const QModelIndex &parent) const
{
  DirModelNode *parentNode = d->nodeForIndex(parent); // O(1)
  DirModelNode *childNode = static_cast<DirModelDirNode*>(parentNode)->m_childNodes.value(row); // O(1)
  
  if (childNode)
    return createIndex(row, column, childNode);
  else
    return QModelIndex();
}

QVariant DirModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  Q_UNUSED(orientation);
  
  switch (role) {
    case Qt::DisplayRole: {
      switch (section) {
        case Name: return i18nc("@title:column", "Name");
        case Size: return i18nc("@title:column", "Size");
        case ModifiedTime: return i18nc("@title:column", "Date");
        case Permissions: return i18nc("@title:column", "Permissions");
        case Owner: return i18nc("@title:column", "Owner");
        case Group: return i18nc("@title:column", "Group");
      }
    }
  }
  
  return QVariant();
}

bool DirModel::hasChildren(const QModelIndex & parent) const
{
  if (!parent.isValid())
      return true;

  const KFileItem parentItem = static_cast<DirModelNode*>(parent.internalPointer())->item();
  return parentItem.isDir();
}

Qt::ItemFlags DirModel::flags(const QModelIndex &index) const
{
  Qt::ItemFlags f = Qt::ItemIsEnabled;
  
  if (index.column() == Name) {
    f |= Qt::ItemFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled);
  }

  // Allow dropping onto this item?
  if (d->m_dropsAllowed != NoDrops) {
    if (!index.isValid()) {
      if (d->m_dropsAllowed & DropOnDirectory) {
        f |= Qt::ItemIsDropEnabled;
      }
    } else {
      KFileItem item = itemForIndex(index);
      
      if (item.isNull()) {
        return f;
      } else if (item.isDir()) {
        if (d->m_dropsAllowed & DropOnDirectory) {
          f |= Qt::ItemIsDropEnabled;
        }
      } else {
        if (d->m_dropsAllowed & DropOnAnyFile) {
          f |= Qt::ItemIsDropEnabled;
        }
      }
    }
  }

  return f;
}

bool DirModel::canFetchMore(const QModelIndex &parent) const
{
  if (!parent.isValid())
      return false;

  DirModelNode *node = static_cast<DirModelNode*>(parent.internalPointer());
  const KFileItem item = node->item();
  
  return item.isDir() && !static_cast<DirModelDirNode*>(node)->isPopulated() &&
         static_cast<DirModelDirNode*>(node)->m_childNodes.isEmpty();
}

void DirModel::fetchMore(const QModelIndex &parent)
{
  if (!parent.isValid())
    return;

  DirModelNode *parentNode = static_cast<DirModelNode*>(parent.internalPointer());
  const KFileItem parentItem = parentNode->item();
  
  DirModelDirNode *dirNode = static_cast<DirModelDirNode*>(parentNode);
  if (dirNode->isPopulated())
    return;
  
  dirNode->setPopulated(true);

  const KUrl url = parentItem.url();
  d->m_dirLister->setIgnoreChanges(true);
  d->m_dirLister->openUrl(url, KDirLister::Keep);
}

bool DirModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
  Q_UNUSED(data);
  Q_UNUSED(action);
  Q_UNUSED(row);
  Q_UNUSED(column);
  Q_UNUSED(parent);
  
  return false;
}

void DirModel::setDropsAllowed(DropsAllowed dropsAllowed)
{
  d->m_dropsAllowed = dropsAllowed;
}

}

}

#include "dirmodel.moc"
