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
#ifndef KFTPWIDGETS_BROWSERLOCATIONNAVIGATOR_H
#define KFTPWIDGETS_BROWSERLOCATIONNAVIGATOR_H

#include <QObject>
#include <QList>

#include <KUrl>
#include <KFileItem>

namespace KFTPWidgets {

namespace Browser {

class DetailsView;

/**
 * This class contains the current navigational history and enables
 * moving through it.
 *
 * @author Jernej Kos
 */
class LocationNavigator : public QObject {
Q_OBJECT
public:
    /**
     * An Element instance represents one history element. The class contains
     * information about the URL, the selected item and the contents position.
     */
    class Element {
    public:
        /**
         * Class constructor.
         */
        Element();
        
        /**
         * Class constructor.
         *
         * @param url Element's URL
         */
        Element(const KUrl &url);
        
        /**
         * Returns the element's URL.
         */
        const KUrl &url() const { return m_url; }
        
        /**
         * Set currently selected item.
         *
         * @param item The item that is currently selected
         */
        void setCurrentItem(const KFileItem &item) { m_currentItem = item; }
        
        /**
         * Returns the selected filename.
         */
        const KFileItem &currentItem() const { return m_currentItem; }
        
        /**
         * Set current contents X position.
         *
         * @param x Contents X position
         */
        void setContentsX(int x) { m_contentsX = x; }
        
        /**
         * Returns the saved contents X position.
         */
        int contentsX() const { return m_contentsX; }
        
        /**
         * Set current contents Y position.
         *
         * @param y Contents Y position
         */
        void setContentsY(int y) { m_contentsY = y; }
        
        /**
         * Returns the saved contents Y position.
         */
        int contentsY() const { return m_contentsY; }
    private:
        KUrl m_url;
        KFileItem m_currentItem;
        
        int m_contentsX;
        int m_contentsY;
    };
    
    /**
     * Class constructor.
     *
     * @param view Parent view
     */
    LocationNavigator(DetailsView *view);
    
    /**
     * Set a new current URL. Calling this will emit the urlChanged signal.
     *
     * @param url Wanted URL
     */
    void setUrl(const KUrl &url);
    
    /**
     * Returns the current URL.
     */
    const KUrl &url() const;
    
    /**
     * Returns the current history elements.
     *
     * @param index Variable to save the current history position to
     * @return Current history element list
     */
    const QList<Element> history(int &index) const;
    
    /**
     * Go one history hop back.
     */
    void goBack();
    
    /**
     * Go one history hop forward.
     */
    void goForward();
    
    /**
     * Go up in the directory structure.
     */
    void goUp();
    
    /**
     * Go the the predefined home URL.
     */
    void goHome();
    
    /**
     * Set the home URL.
     *
     * @param url URL to use as home URL
     */
    void setHomeUrl(const KUrl &url) { m_homeUrl = url; }
    
    /**
     * Clear current history.
     */
    void clear();
signals:
    /**
     * This signal is emitted whenever the current URL changes.
     *
     * @param url The new URL
     */
    void urlChanged(const KUrl &url);
    
    /**
     * This signal is emitted whenever the history is updated.
     *
     * @param url Current URL
     */
    void historyChanged(const KUrl &url);
private slots:
    void slotContentsMoved(int x, int y);
private:
    DetailsView *m_view;
    int m_historyIndex;
    QList<Element> m_history;
    KUrl m_homeUrl;
    
    void updateCurrentElement();
};

}

}

#endif
