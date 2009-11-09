/*
 * This file is part of the KFTPGrabber project
 *
 * Copyright (C) 2003-2005 by the KFTPGrabber developers
 * Copyright (C) 2003-2005 Jernej Kos <kostko@jweb-network.net>
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

#ifndef KFTPQUEUESTATISTICS_H
#define KFTPQUEUESTATISTICS_H

#include <QObject>
#include <QMap>

#include <kurl.h>
#include <kio/global.h>

namespace KFTPQueue {

class StatisticsPrivate;

/**
 * This class represents a statistics for a single site.
 *
 * @author Jernej Kos <kostko@unimatrix-one.org>
 */
class StatisticsSite {
public:
    StatisticsSite();
    
    /**
     * Set the site's last FXP speed.
     *
     * @param speed Site's last FXP speed
     */
    void setLastFxpSpeed(double speed) { m_lastFxpSpeed = speed; }
    
    /**
     * Get the site's last FXP speed.
     *
     * @return Site's last FXP speed
     */
    double lastFxpSpeed() const { return m_lastFxpSpeed; }
private:
    double m_lastFxpSpeed;
};

/**
 * This class provides different kind of per-site statistics.
 *
 * @author Jernej Kos <kostko@unimatrix-one.org>
 */
class Statistics : public QObject
{
Q_OBJECT
friend class StatisticsPrivate;
public:
    /**
     * Returns the global statistics class instance.
     */
    static Statistics *self();
    
    /**
     * Returns a site that corresponds to the given URL. If the site doesn't
     * exist it is created.
     *
     * @param url The site's URL
     * @return A valid StatisticsSite pointer
     */
    StatisticsSite *getSite(const KUrl &url);
protected:
    /**
     * Class constructor.
     */
    Statistics();
    
    /**
     * Class destructor.
     */
    ~Statistics();
private:
    QMap<KUrl, StatisticsSite*> m_sites;
};

}

#endif
