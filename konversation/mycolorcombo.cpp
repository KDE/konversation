/* This file is part of the KDE libraries
    Copyright (C) 1997 Martin Jones (mjones@kde.org)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/
//-----------------------------------------------------------------------------

/* MyColorCombo is a slightly modified version of KColorCombo.

   There were only one two little changes made to the class definition
   concerning the functions "addColors()" and "slotActivated()". We needed to
   overwrite the call to QPainter in these functions to avoid the "Custom..."
   label to be printed into the widget when its value is set to a non standard
   color. Deriving a new class and overwriting the memberfunctions "addColors()"
   and "slotActivated()" seems to be far to much trouble to us because of the
   use of variables declared as "private" in this function.
*/

//-----------------------------------------------------------------------------

// KDE color selection dialog.
//
// 1999-09-27 Espen Sand <espensa@online.no>
// KColorDialog is now subclassed from KDialogBase. I have also extended
// KColorDialog::getColor() so that in contains a parent argument. This
// improves centering capability.
//
// layout managment added Oct 1997 by Mario Weilguni
// <mweilguni@sime.com>
//


#include <stdio.h>
#include <stdlib.h>

#include <qdrawutil.h>
#include <qevent.h>
#include <qfile.h>
#include <qimage.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qvalidator.h>
#include <qpainter.h>
#include <qpushbutton.h>
#include <qtimer.h>

#include <kapplication.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <klistbox.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kseparator.h>
#include <kpalette.h>
#include <kimageeffect.h>

//#include "kcolordlg.h"
//#include "kcolordrag.h"
#include "mycolorcombo.h"

// This is repeated from the KColorDlg, but I didn't
// want to make it public BL.
// We define it out when compiling with --enable-final in which case
// we use the version defined in KColorDlg

#ifndef KDE_USE_FINAL
#define STANDARD_PAL_SIZE 17

static QColor *standardPalette = 0;

static void createStandardPalette()
{
    if ( standardPalette )
	return;

    standardPalette = new QColor [STANDARD_PAL_SIZE];

    int i = 0;

    standardPalette[i++] = Qt::red;
    standardPalette[i++] = Qt::green;
    standardPalette[i++] = Qt::blue;
    standardPalette[i++] = Qt::cyan;
    standardPalette[i++] = Qt::magenta;
    standardPalette[i++] = Qt::yellow;
    standardPalette[i++] = Qt::darkRed;
    standardPalette[i++] = Qt::darkGreen;
    standardPalette[i++] = Qt::darkBlue;
    standardPalette[i++] = Qt::darkCyan;
    standardPalette[i++] = Qt::darkMagenta;
    standardPalette[i++] = Qt::darkYellow;
    standardPalette[i++] = Qt::white;
    standardPalette[i++] = Qt::lightGray;
    standardPalette[i++] = Qt::gray;
    standardPalette[i++] = Qt::darkGray;
    standardPalette[i++] = Qt::black;
}
#endif

class MyColorCombo::MyColorComboPrivate
{
	protected:
	friend class MyColorCombo;
	MyColorComboPrivate(){}
	~MyColorComboPrivate(){}
	bool showEmptyList;
};

MyColorCombo::MyColorCombo( QWidget *parent, const char *name )
	: QComboBox( parent, name )
{
	d=new MyColorComboPrivate();
	d->showEmptyList=false;

	customColor.setRgb( 255, 255, 255 );
	internalcolor.setRgb( 255, 255, 255 );

	createStandardPalette();

	addColors();

	connect( this, SIGNAL( activated(int) ), SLOT( slotActivated(int) ) );
	connect( this, SIGNAL( highlighted(int) ), SLOT( slotHighlighted(int) ) );
}


MyColorCombo::~MyColorCombo()
{
	delete d;
}
/**
   Sets the current color
 */
void MyColorCombo::setColor( const QColor &col )
{
	internalcolor = col;
	d->showEmptyList=false;
	addColors();
}


/**
   Returns the currently selected color
 */
QColor MyColorCombo::color() const {
  return internalcolor;
}

void MyColorCombo::resizeEvent( QResizeEvent *re )
{
	QComboBox::resizeEvent( re );

	addColors();
}

/**
   Show an empty list, till the next colour is set with setColor
 */
void MyColorCombo::showEmptyList()
{
	d->showEmptyList=true;
	addColors();
}

void MyColorCombo::slotActivated( int index )
{
	if ( index == 0 )
	{
	    if ( KColorDialog::getColor( customColor, this ) == QDialog::Accepted )
		{
			QPainter painter;
			QPen pen;
			QRect rect( 0, 0, width(), QFontMetrics(painter.font()).height()+4);
			QPixmap pixmap( rect.width(), rect.height() );

			if ( qGray( customColor.rgb() ) < 128 )
				pen.setColor( white );
			else
				pen.setColor( black );

			painter.begin( &pixmap );
			QBrush brush( customColor );
			painter.fillRect( rect, brush );
			painter.setPen( pen );
			painter.drawText( 2, QFontMetrics(painter.font()).ascent()+2, "" );
			painter.end();

			changeItem( pixmap, 0 );
			pixmap.detach();
		}

		internalcolor = customColor;
	}
	else
		internalcolor = standardPalette[ index - 1 ];

	emit activated( internalcolor );
}

void MyColorCombo::slotHighlighted( int index )
{
	if ( index == 0 )
		internalcolor = customColor;
	else
		internalcolor = standardPalette[ index - 1 ];

	emit highlighted( internalcolor );
}

void MyColorCombo::addColors()
{
	QPainter painter;
	QPen pen;
	QRect rect( 0, 0, width(), QFontMetrics(painter.font()).height()+4 );
	QPixmap pixmap( rect.width(), rect.height() );
	int i;

	clear();
	if (d->showEmptyList) return;

	createStandardPalette();

	for ( i = 0; i < STANDARD_PAL_SIZE; i++ )
		if ( standardPalette[i] == internalcolor ) break;

	if ( i == STANDARD_PAL_SIZE )
		customColor = internalcolor;

	if ( qGray( customColor.rgb() ) < 128 )
		pen.setColor( white );
	else
		pen.setColor( black );

	painter.begin( &pixmap );
	QBrush brush( customColor );
	painter.fillRect( rect, brush );
	painter.setPen( pen );
	painter.drawText( 2, QFontMetrics(painter.font()).ascent()+2, "" );
	painter.end();

	insertItem( pixmap );
	pixmap.detach();

	for ( i = 0; i < STANDARD_PAL_SIZE; i++ )
	{
		painter.begin( &pixmap );
		QBrush brush( standardPalette[i] );
		painter.fillRect( rect, brush );
		painter.end();

		insertItem( pixmap );
		pixmap.detach();

		if ( standardPalette[i] == internalcolor )
			setCurrentItem( i + 1 );
	}
}

void MyColorCombo::virtual_hook( int, void* )
{ /*BASE::virtual_hook( id, data );*/ }

//#include "mycolorcombo.moc"
