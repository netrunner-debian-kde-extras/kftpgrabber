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
#ifndef DIRECTORYSCANNER_H
#define DIRECTORYSCANNER_H

#include <QObject>
#include <QThread>

namespace KFTPQueue {
  class Transfer;
  class Manager;
}

namespace KFTPEngine {
  class DirectoryTree;
}

/**
 * This class can be used to scan a local directory using a separate
 * thread and create any needed child transfers.
 *
 * @author Jernej Kos <kostko@jweb-network.net>
 */
class DirectoryScanner : public QObject {
Q_OBJECT
friend class KFTPQueue::Manager;
public:
    /**
     * Class constructor.
     *
     * @param transfer The transfer to scan
     */
    DirectoryScanner(KFTPQueue::Transfer *transfer);
    
    /**
     * Aborts the scanning process.
     */
    void abort();
private:
    /**
     * The actual thread that does the scanning.
     */
    class ScannerThread : public QThread {
    public:
        /**
         * Class constructor.
         */
        ScannerThread(QObject *parent, KFTPQueue::Transfer *item);
        
        /**
         * Aborts this scanning thread.
         */
        void abort();
        
        /**
         * Returns the directory tree that has resulted from the scan.
         */
        KFTPEngine::DirectoryTree *tree() const { return m_tree; }
    protected:
        /**
         * Thread entry point.
         */
        void run();
    private:
        QObject *m_parent;
        KFTPEngine::DirectoryTree *m_tree;
        KFTPQueue::Transfer *m_item;
        bool m_abort;
        
        /**
         * A method to recursively scan a given folder.
         */
        void scanFolder(const QString &path, KFTPEngine::DirectoryTree *tree);
    };
    
    /**
     * A helper method for creating transfer objects out of directory structure.
     */
    void addScannedDirectory(KFTPEngine::DirectoryTree *tree, KFTPQueue::Transfer *parent);
    
    ScannerThread *m_thread;
    KFTPQueue::Transfer *m_transfer;
    bool m_abort;
protected slots:
    void threadFinished();
signals:
    /**
     * This signal is emitted when scanning complets. This object is automaticly
     * destroyed immediately after the signal returns!
     */
    void completed();
};

#endif
