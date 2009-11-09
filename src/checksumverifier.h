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

#ifndef KFTPCORE_CHECKSUMVERIFIER_H
#define KFTPCORE_CHECKSUMVERIFIER_H

#include <qstring.h>
#include <qthread.h>
#include <qobject.h>
#include <q3valuelist.h>
#include <qpair.h>
//Added by qt3to4:
#include <QEvent>

namespace KFTPCore {

class ChecksumVerifierThread;

/**
 * @author Jernej Kos
 */
class ChecksumVerifier : public QObject
{
friend class ChecksumVerifierThread;
Q_OBJECT
public:
    enum Type {
      CheckMd5,
      CheckSfv
    };
    
    enum Result {
      Ok,
      NotFound,
      Error
    };
    
    ChecksumVerifier(const QString &filename, Type type = CheckSfv);
    ~ChecksumVerifier();
    
    void verify();
protected slots:
    void customEvent(QEvent *e);
private:
    QString m_filename;
    Type m_type;
    
    ChecksumVerifierThread *m_thread;
signals:
    void progress(int percent);
    void fileDone(const QString &filename, KFTPCore::ChecksumVerifier::Result result);
    void fileList(QList<QPair<QString, QString> > list);
    void error();
};

#define CV_THR_EVENT_ID 65300

class ChecksumVerifierThreadEvent : public QEvent
{
friend class ChecksumVerifier;
public:
    ChecksumVerifierThreadEvent(const QString &filename, ChecksumVerifier::Result result)
      : QEvent((QEvent::Type) CV_THR_EVENT_ID),
        m_type(0),
        m_filename(filename),
        m_result(result)
    {}
    
    ChecksumVerifierThreadEvent(Q3ValueList<QPair<QString, QString> > list)
      : QEvent((QEvent::Type) CV_THR_EVENT_ID),
        m_type(1),
        m_list(list)
    {}
    
    ChecksumVerifierThreadEvent(int type, int progress = 0)
      : QEvent((QEvent::Type) CV_THR_EVENT_ID),
        m_type(type),
        m_progress(progress)
    {}
private:
    int m_type;
    int m_progress;
    QString m_filename;
    ChecksumVerifier::Result m_result;
    QList<QPair<QString, QString> > m_list;
};

class ChecksumVerifierThread : public QThread
{
public:
    ChecksumVerifierThread(ChecksumVerifier *verifier);
protected:
    void run();
private:
    ChecksumVerifier *m_verifier;
    
    void checkSFV(const QString &sfvfile, const QString &fileToCheck = QString::null);
    
    static inline long UpdateCRC(register unsigned long CRC, register char *buffer, register long count);
    static long getFileCRC(const char *filename);
};

}

#endif
