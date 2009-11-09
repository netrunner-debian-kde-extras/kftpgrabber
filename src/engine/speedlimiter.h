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
#ifndef KFTPENGINESPEEDLIMITER_H
#define KFTPENGINESPEEDLIMITER_H

#include <QObject>
#include <QList>
#include <QTimer>

namespace KFTPEngine {

class SpeedLimiterPrivate;
class SpeedLimiterItem;

/**
 * This class is used by Socket implementations to enforce speed limits for
 * uploads or downloads. It implements a variant of Token Bucket algorithm.
 *
 * @author Jernej Kos <kostko@unimatrix-one.org>
 */
class SpeedLimiter : public QObject {
Q_OBJECT
friend class SpeedLimiterPrivate;
public:
    /**
     * Possible limit types.
     */
    enum Type {
      Download = 0,
      Upload = 1
    };
    
    /**
     * Returns the global speed limiter instance.
     */
    static SpeedLimiter *self();
    
    /**
     * Set a limit rate.
     *
     * @param type Limit type
     * @param limit Rate
     */
    void setLimit(Type type, int limit);
    
    /**
     * Appends an item to be managed by the speed limiter.
     *
     * @param item Item instance
     * @param type Limit type
     */
    void append(SpeedLimiterItem *item, Type type);
    
    /**
     * Removes an item from the speed limiter.
     *
     * @param item Item instance
     */
    void remove(SpeedLimiterItem *item);
    
    /**
     * Removes an item from the speed limiter.
     *
     * @param item Item instance
     * @param type Limit type
     */
    void remove(SpeedLimiterItem *item, Type type);
protected:
    /**
     * Class constructor.
     */
    SpeedLimiter();
    
    /**
     * Class destructor.
     */
    ~SpeedLimiter();
private:
    QTimer *m_timer;
    int m_limits[2];
    
    QList<SpeedLimiterItem*> m_objects[2];
    
    int m_tokenDebt[2];
private slots:
    void updateLimits();
    void synchronize();
signals:
    void activateTimer(int msec);
};

/**
 * This class represents an item managed by the speed limiter. This is
 * usually a socket.
 *
 * @author Jernej Kos <kostko@unimatrix-one.org>
 */
class SpeedLimiterItem {
friend class SpeedLimiter;
public:
    /**
     * Class constructor.
     */
    SpeedLimiterItem();
    
    /**
     * Returns the number of bytes allowed for consumption.
     */
    int allowedBytes() const { return m_availableBytes; }
protected:
    /**
     * Updates object's byte usage.
     */
    void updateUsage(int bytes);
private:
    int m_availableBytes;
};

}

#endif
