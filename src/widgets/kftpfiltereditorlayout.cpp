#include <klocale.h>
/****************************************************************************
** Form implementation generated from reading ui file '/home/kostko/development/kftpgrabber/src/widgets/kftpfiltereditorlayout.ui'
**
** Created: Mon Oct 20 16:14:00 2003
**      by: The User Interface Compiler ($Id: kftpfiltereditorlayout.cpp,v 1.1.1.1 2004/02/13 13:33:43 kostko Exp $)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#include "kftpfiltereditorlayout.h"

#include <qvariant.h>
#include <qpushbutton.h>
#include <qtabwidget.h>
//Added by qt3to4:
#include <Q3VBoxLayout>
#include <Q3GridLayout>
#include <kpushbutton.h>
#include <q3header.h>
#include <k3listview.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <q3whatsthis.h>

/* 
 *  Constructs a KFTPFilterEditorLayout as a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f'.
 */
KFTPFilterEditorLayout::KFTPFilterEditorLayout( QWidget* parent, const char* name, Qt::WFlags fl )
    : QWidget( parent, name, fl )
{
    if ( !name )
	setName( "KFTPFilterEditorLayout" );
    KFTPFilterEditorLayoutLayout = new Q3GridLayout( this, 1, 1, 11, 6, "KFTPFilterEditorLayoutLayout"); 

    tabWidget2 = new QTabWidget( this, "tabWidget2" );

    tab = new QWidget( tabWidget2, "tab" );
    tabLayout = new Q3GridLayout( tab, 1, 1, 11, 6, "tabLayout"); 

    layout1 = new Q3VBoxLayout( 0, 0, 6, "layout1"); 

    addPatternButton = new KPushButton( tab, "addPatternButton" );
    layout1->addWidget( addPatternButton );

    editPatternButton = new KPushButton( tab, "editPatternButton" );
    layout1->addWidget( editPatternButton );

    removePatternButton = new KPushButton( tab, "removePatternButton" );
    layout1->addWidget( removePatternButton );

    tabLayout->addLayout( layout1, 0, 1 );
    QSpacerItem* spacer = new QSpacerItem( 31, 111, QSizePolicy::Minimum, QSizePolicy::Expanding );
    tabLayout->addItem( spacer, 1, 1 );

    layout2 = new Q3VBoxLayout( 0, 0, 6, "layout2"); 

    patternList = new K3ListView( tab, "patternList" );
    patternList->addColumn( tr2i18n( "Pattern" ) );
    patternList->addColumn( tr2i18n( "Color" ) );
    layout2->addWidget( patternList );

    enabledCheck = new QCheckBox( tab, "enabledCheck" );
    layout2->addWidget( enabledCheck );

    tabLayout->addMultiCellLayout( layout2, 0, 1, 0, 0 );
    tabWidget2->insertTab( tab, QString("") );

    tab_2 = new QWidget( tabWidget2, "tab_2" );

    textLabel1 = new QLabel( tab_2, "textLabel1" );
    textLabel1->setGeometry( QRect( 10, 10, 130, 20 ) );
    tabWidget2->insertTab( tab_2, QString("") );

    tab_3 = new QWidget( tabWidget2, "tab_3" );

    textLabel1_2 = new QLabel( tab_3, "textLabel1_2" );
    textLabel1_2->setGeometry( QRect( 10, 10, 130, 20 ) );
    tabWidget2->insertTab( tab_3, QString("") );

    KFTPFilterEditorLayoutLayout->addWidget( tabWidget2, 0, 0 );
    languageChange();
    resize( QSize(456, 299).expandedTo(minimumSizeHint()) );
    clearWState( WState_Polished );
}

/*
 *  Destroys the object and frees any allocated resources
 */
KFTPFilterEditorLayout::~KFTPFilterEditorLayout()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void KFTPFilterEditorLayout::languageChange()
{
    setCaption( tr2i18n( "Form1" ) );
    addPatternButton->setText( tr2i18n( "Add pattern" ) );
    editPatternButton->setText( tr2i18n( "Edit" ) );
    removePatternButton->setText( tr2i18n( "Remove" ) );
    patternList->header()->setLabel( 0, tr2i18n( "Pattern" ) );
    patternList->header()->setLabel( 1, tr2i18n( "Color" ) );
    enabledCheck->setText( tr2i18n( "Enabled" ) );
    tabWidget2->changeTab( tab, tr2i18n( "Highlighting" ) );
    textLabel1->setText( tr2i18n( "<b>Not yet implemented.</b>" ) );
    tabWidget2->changeTab( tab_2, tr2i18n( "Skip List" ) );
    textLabel1_2->setText( tr2i18n( "<b>Not yet implemented.</b>" ) );
    tabWidget2->changeTab( tab_3, tr2i18n( "ASCII extensions" ) );
}

#include "kftpfiltereditorlayout.moc"
