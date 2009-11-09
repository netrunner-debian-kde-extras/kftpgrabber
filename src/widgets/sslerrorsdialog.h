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
#ifndef KFTPWIDGETS_SSLERRORSDIALOG_H
#define KFTPWIDGETS_SSLERRORSDIALOG_H

#include <KDialog>
#include <QVariant>
#include <QSslCertificate>

#include "widgets/ui_sslerrorsdialog.h"

namespace KFTPWidgets {

/**
 * A simple dialog that displays errors ocurred during the SSL
 * negotiation phase. Also offers a link to review the peer
 * certificate information.
 *
 * @author Jernej Kos <kostko@unimatrix-one.org>
 */
class SslErrorsDialog : public KDialog {
Q_OBJECT
public:
    /**
     * Class constructor.
     */
    SslErrorsDialog(QWidget *parent = 0);
    
    /**
     * Sets the errors that should be displayed by this dialog. The list
     * should contain variants that can be converted to QSslError
     * instances.
     *
     * @param errors A list of errors ocurred during negotiation
     */
    void setErrors(const QVariantList &errors);
protected slots:
    /**
     * @overload
     * Reimplemented from KDialog.
     */
    virtual void slotButtonClicked(int button);
private:
    Ui::SslErrorsDialog ui;
    QSslCertificate m_certificate;
};

}

#endif
