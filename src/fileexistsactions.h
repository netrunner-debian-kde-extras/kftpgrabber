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

#ifndef KFTPQUEUEFILEEXISTSACTIONS_H
#define KFTPQUEUEFILEEXISTSACTIONS_H

#include <sys/types.h>

#include <QWidget>
#include <QMap>

typedef qulonglong filesize_t;

class KComboBox;

namespace KFTPQueue {

enum FEAction {
  FE_DISABLE_ACT = -1,
  FE_SKIP_ACT = 0,
  FE_OVERWRITE_ACT = 1,
  FE_RESUME_ACT = 2,
  FE_RENAME_ACT = 3,
  FE_USER_ACT = 4
};

typedef QMap<int, FEAction> ActionMap;

/**
 * This class provides configurable "on file exists" actions. They are
 * represented in a 3x3 matrix which determines the 9 possible scenarios.
 * The matrix goes like this:
 * <pre>
 *               | SAME TIMESTAMP | OLDER | NEWER
 * SAME FILESIZE |       1        |   2   |   3
 *       SMALLER |       4        |   5   |   6
 *        BIGGER |       7        |   8   |   9
 * </pre>
 *
 * @author Jernej Kos
 */
class FileExistsActions
{
public:
    /**
     * This method will construct a new widget that will represent
     * the current status of the overwrite matrix and the ability to
     * change the values.
     *
     * @param parent Widget's parent
     * @return A new @ref QWidget
     */
    QWidget *getConfigWidget(QWidget *parent = 0);
    
    /**
     * Set action for a specific file exists situation.
     *
     * @param situation Situation (acoording to the above matrix)
     * @action A FEAction that determines the appropriate action
     */
    void setActionForSituation(int situation, FEAction action) { m_actions[situation] = action; }
    
    /**
     * Get action for specific file exists situation.
     *
     * @param src_fileSize File size of the file that exists
     * @param src_fileTimestamp File timestamp of the file that exists
     * @param dst_fileSize File size of the file that will (or not) replace the old one
     * @param dst_fileTimestamp File timestamp of the file that will (or not) replace the old one
     * @return An action as @ref FEAction
     */
    FEAction getActionForSituation(filesize_t src_fileSize, time_t src_fileTimestamp,
                                   filesize_t dst_fileSize, time_t dst_fileTimestamp);
                                   
    /**
     * Sets a text that will be used as type for these actions (like download/upload).
     *
     * @param text The text
     */
    void setTypeText(const QString &text) { m_type = text; }
    
    /**
     * Update the current GUI widget with new settings.
     */
    void updateWidget();
    
    /**
     * Update the current configuration with the new settings (as dictated by the
     * GUI).
     */
    void updateConfig();
private:
    ActionMap m_actions;
    QString m_type;
    KComboBox *m_combos[3][3];
    
    friend QString &operator<<(QString &s, const FileExistsActions &a);
    friend QString &operator>>(QString &s, FileExistsActions &a);
};

QString &operator<<(QString &s, const FileExistsActions &a);
QString &operator>>(QString &s, FileExistsActions &a);

}

#endif
