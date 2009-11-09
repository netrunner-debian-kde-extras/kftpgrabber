/*
 * This file is part of the KFTPGrabber project
 *
 * Copyright (C) 2003-2008 by the KFTPGrabber developers
 * Copyright (C) 2003-2008 Jernej Kos <kostko@jweb-network.net>
 * Copyright (C) 2004 Markus Brueffer <markus@brueffer.de>
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

#include "mainwindow.h"
#include "misc/config.h"

#include <kuniqueapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <KLocale>
#include <kstandarddirs.h>
#include <ksplashscreen.h>
#include <QPixmap>

#include "kftpsession.h"

static const char description[] =
    I18N_NOOP("KFTPgrabber - an FTP client for KDE");

static const char version[] = "0.8.99";

int main(int argc, char **argv)
{
    KAboutData about("kftpgrabber", 0, ki18n("KFTPgrabber"), version, ki18n(description),
                     KAboutData::License_GPL, ki18n("(C) 2008, The KFTPgrabber developers"), KLocalizedString(), "http://www.kftp.org");
    about.addAuthor(ki18n("Jernej Kos"), ki18n("Lead developer"), "kostko@unimatrix-one.org");
    about.addAuthor(ki18n("Markus Brüffer"), ki18n("Developer"), "markus@brueffer.de");
    
    about.addCredit(ki18n("Lee Joseph"), ki18n("Fedora ambassador responsible for promotion, testing and debugging; also a package maintainer for Fedora-compatible distributions"), "cyberspy@cyberspy.ws");
    about.addCredit(ki18n("libssh2 Developers"), ki18n("SSH library"), "libssh2-devel@lists.sourceforge.net");
    about.addCredit(ki18n("Anthony D. Urso"), ki18n("otpCalc code"));
    about.addCredit(ki18n("Kopete Developers"), ki18n("KopeteBalloon popup code"), "kopete-devel@kde.org");
    about.addCredit(ki18n("KSysGuard Developers"), ki18n("Traffic graph widget"), "cs@kde.org");
    about.addCredit(ki18n("Bob Ziuchkovski"), ki18n("Icon design"), "ziuchkov@uiuc.edu");
    about.addCredit(ki18n("Tobias Ussing"), ki18n("Testing and debugging"), "thehole@mail.seriesdb.com");
    about.addCredit(ki18n("Tim Kosse"), ki18n("Directory parser code"), "tim.kosse@gmx.de");
    about.addCredit(ki18n("Peter Penz"), ki18n("Listview column handling code"), "peter.penz@gmx.at");

    KCmdLineArgs::init(argc, argv, &about);

    KCmdLineOptions options;
    options.add("+[url]", ki18n("An optional URL to connect to"));
    KCmdLineArgs::addCmdLineOptions(options);
    KUniqueApplication app;
    
    if (app.isSessionRestored()) {
      RESTORE(MainWindow);
    } else {
      MainWindow *mainWindow = 0;
      KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

      KSplashScreen *splash = 0L;
      QString splashPath = KStandardDirs::locate("appdata", "kftpgrabber-logo.png");
      if (!KFTPCore::Config::startMinimized() && KFTPCore::Config::showSplash()) {
        // Show the splash screen
        if (!splashPath.isNull()) {
          QPixmap splashImage = QPixmap(splashPath);
          splash = new KSplashScreen(splashImage);
          splash->setMaximumWidth(400);
          splash->show();
        }
      }
      
      mainWindow = new MainWindow();
      if (!KFTPCore::Config::startMinimized())
        mainWindow->show();
        
      // Check if an URL was passed as a command line argument
      if (args->count() == 1) {
        KUrl remoteUrl = args->url(0);
        
        if (!remoteUrl.isLocalFile()) {
          if (!remoteUrl.port())
            remoteUrl.setPort(21);
            
          if (!remoteUrl.hasUser())
            remoteUrl.setUser("anonymous");
            
          if (!remoteUrl.hasPass()) {
            if (!KFTPCore::Config::anonMail().isEmpty())
              remoteUrl.setPass(KFTPCore::Config::anonMail());
            else
              remoteUrl.setPass("userlogin@anonymo.us");
          }

          KFTPSession::Manager::self()->spawnRemoteSession(KFTPSession::IgnoreSide, remoteUrl);
        }
      }
        
      if (splash != 0L) {
        splash->finish(mainWindow);
        delete splash;
      }
      
      args->clear();
    }
    
    return app.exec();
}

