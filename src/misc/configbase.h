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
 *
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
 
#ifndef KFTPCORECONFIGBASE_H
#define KFTPCORECONFIGBASE_H

#include <KConfigSkeleton>

#include "certificatestore.h"
#include "fileexistsactions.h"

namespace KFTPCore {

/**
 * This is a base class for KFTPGrabber's configuration. It is inherited by
 * auto-generated KConfigXT class KFTPCore::Config that adds all the configuration
 * options.
 *
 * @author Jernej Kos
 */
class ConfigBase : public KConfigSkeleton
{
Q_OBJECT
public:
    ConfigBase(const QString &fileName);
    
    /**
     * Does some post initialization stuff that couldn't be done in the constructor due
     * to use of Config singleton.
     */
    void postInit();
    
    /**
     * Does some pre write stuff (eg. exporting the actions).
     */
    void saveConfig();
    
    /**
     * Returns a proper mode for the requested file. If the current mode is set to AUTO
     * the list of ascii file patterns is consulted.
     *
     * @param filename The filename for which the mode should be returned
     * @return A valid FTP transfer mode
     */
    char ftpMode(const QString &filename);
    
    /**
     * Set the global transfer mode.
     *
     * @param mode Transfer mode
     */
    void setGlobalMode(char mode) { m_transMode = mode; }
    
    /**
     * Get the global transfer mode.
     *
     * @return The transfer mode currently in use
     */
    char getGlobalMode() const { return m_transMode; }
    
    /**
     * Get the download actions object.
     *
     * @return The FileExistsActions object for download actions
     */
    KFTPQueue::FileExistsActions *dActions() { return &m_fileExistsDownActions; }
    
    /**
     * Get the upload actions object.
     *
     * @return The FileExistsActions object for upload actions
     */
    KFTPQueue::FileExistsActions *uActions() { return &m_fileExistsUpActions; }
    
    /**
     * Get the fxp actions object.
     *
     * @return The FileExistsActions object for fxp actions
     */
    KFTPQueue::FileExistsActions *fActions() { return &m_fileExistsFxpActions; }
    
    /**
     * Returns the global CertificateStore object.
     */
    CertificateStore *certificateStore() { return &m_certificateStore; }
public slots:
    /**
     * Emits the configChanged() signal.
     */
    void emitChange();
protected:
    QString getGlobalMail();
private:
    KFTPQueue::FileExistsActions m_fileExistsDownActions;
    KFTPQueue::FileExistsActions m_fileExistsUpActions;
    KFTPQueue::FileExistsActions m_fileExistsFxpActions;
    CertificateStore m_certificateStore;
    
    char m_transMode;
signals:
    void configChanged();
};

}

#endif
