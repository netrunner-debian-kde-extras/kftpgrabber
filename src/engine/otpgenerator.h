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

#ifndef KFTPENGINEOTPGENERATOR_H
#define KFTPENGINEOTPGENERATOR_H

#include <QString>

namespace KFTPEngine {

/**
 * One-Time Password generator required by some servers for authentication. Based
 * on code from otpCalc by Anthony D. Urso.
 *
 * @author Jernej Kos <kostko@unimatrix-one.org>
 * @author Anthony D. Urso
 */
class OtpGenerator {
public:
    /**
     * Supported hash algorithms.
     */
    enum Algorithm {
      None,
      AlgoMD4,
      AlgoMD5,
      AlgoRMD160,
      AlgoSHA1
    };
    
    /**
     * Class constructor.
     *
     * @param challenge Server's challenge
     * @param password User's password
     */
    OtpGenerator(const QString &challenge, const QString &password);
    
    /**
     * Generates a response to the provided challenge.
     */
    QString response();
private:
    QString m_seed;
    QString m_password;
    Algorithm m_algorithm;
    int m_seq;

    void genDigest(char *msg, unsigned int len);
    void genDigestMD(int type, char *msg, unsigned int len);
    void genDigestRS(int type, char *msg, unsigned int len);
    static unsigned short extract(char *s, int start, int len);
    unsigned char parity(unsigned char *msg);
    unsigned char *sixWords(unsigned char *msg, char *response);
};

}

#endif