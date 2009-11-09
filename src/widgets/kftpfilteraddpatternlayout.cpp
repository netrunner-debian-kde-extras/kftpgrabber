#include <klocale.h>
/****************************************************************************
** Form implementation generated from reading ui file '/home/kostko/development/kftpgrabber/src/widgets/kftpfilteraddpatternlayout.ui'
**
** Created: Mon Oct 20 18:26:31 2003
**      by: The User Interface Compiler ($Id: kftpfilteraddpatternlayout.cpp,v 1.1.1.1 2004/02/13 13:33:43 kostko Exp $)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#include "kftpfilteraddpatternlayout.h"

#include <qvariant.h>
#include <qpushbutton.h>
#include <q3groupbox.h>
#include <qlabel.h>
//Added by qt3to4:
#include <Q3VBoxLayout>
#include <Q3GridLayout>
#include <Q3HBoxLayout>
#include <klineedit.h>
#include <kcolorbutton.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <q3whatsthis.h>

/* 
 *  Constructs a KFTPFilterAddPatternLayout as a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f'.
 */
KFTPFilterAddPatternLayout::KFTPFilterAddPatternLayout( QWidget* parent, const char* name, Qt::WFlags fl )
    : QWidget( parent, name, fl )
{
    if ( !name )
	setName( "KFTPFilterAddPatternLayout" );
    KFTPFilterAddPatternLayoutLayout = new Q3GridLayout( this, 1, 1, 11, 6, "KFTPFilterAddPatternLayoutLayout"); 

    groupBox1 = new Q3GroupBox( this, "groupBox1" );
    groupBox1->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)5, (QSizePolicy::SizeType)2, 0, 0, groupBox1->sizePolicy().hasHeightForWidth() ) );
    groupBox1->setColumnLayout(0, Qt::Vertical );
    groupBox1->layout()->setSpacing( 6 );
    groupBox1->layout()->setMargin( 11 );
    groupBox1Layout = new Q3GridLayout( groupBox1->layout() );
    groupBox1Layout->setAlignment( Qt::AlignTop );

    layout9 = new Q3HBoxLayout( 0, 0, 6, "layout9"); 

    layout8 = new Q3VBoxLayout( 0, 0, 6, "layout8"); 

    textLabel1 = new QLabel( groupBox1, "textLabel1" );
    layout8->addWidget( textLabel1 );

    textLabel2 = new QLabel( groupBox1, "textLabel2" );
    layout8->addWidget( textLabel2 );
    layout9->addLayout( layout8 );

    layout7 = new Q3VBoxLayout( 0, 0, 6, "layout7"); 

    kLineEdit1 = new KLineEdit( groupBox1, "kLineEdit1" );
    layout7->addWidget( kLineEdit1 );

    kColorButton1 = new KColorButton( groupBox1 );
    kColorButton1->setObjectName( "kColorButton1" );
    kColorButton1->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)0, (QSizePolicy::SizeType)0, 0, 0, kColorButton1->sizePolicy().hasHeightForWidth() ) );
    layout7->addWidget( kColorButton1 );
    layout9->addLayout( layout7 );

    groupBox1Layout->addLayout( layout9, 0, 0 );

    KFTPFilterAddPatternLayoutLayout->addWidget( groupBox1, 0, 0 );
    languageChange();
    resize( QSize(380, 110).expandedTo(minimumSizeHint()) );
    clearWState( WState_Polished );
}

/*
 *  Destroys the object and frees any allocated resources
 */
KFTPFilterAddPatternLayout::~KFTPFilterAddPatternLayout()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void KFTPFilterAddPatternLayout::languageChange()
{
    setCaption( tr2i18n( "Form1" ) );
    groupBox1->setTitle( tr2i18n( "New Pattern" ) );
    textLabel1->setText( tr2i18n( "Filename pattern:" ) );
    textLabel2->setText( tr2i18n( "Color:" ) );
    kColorButton1->setText( QString::null );
}

#include "kftpfilteraddpatternlayout.moc"
