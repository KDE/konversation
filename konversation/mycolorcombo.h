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

// KDE color selection combo box

// layout managment added Oct 1997 by Mario Weilguni
// <mweilguni@sime.com>


#ifndef _MyColorCombo_H__
#define _MyColorCombo_H__

#include <qcombobox.h>
#include <kcolordlg.h>
#include "kselect.h"


class MyColorComboInternal;

/*
 * Combobox for colors.
 */
class MyColorCombo : public QComboBox
{
    Q_OBJECT
    Q_PROPERTY( QColor color READ color WRITE setColor )

public:
    /**
     * Constructs a color combo box.
     */
    MyColorCombo( QWidget *parent, const char *name = 0L );
    ~MyColorCombo();

    /**
     * Selects the color @p col.
     */
    void setColor( const QColor &col );
    /**
     * Returns the currently selected color.
     **/
    QColor color() const;


    /**
     * Clear the colour list and don't show it, till the next setColor() call
     **/
     void showEmptyList();

signals:
    /**
     * Emitted when a new color box has been selected.
     */
    void activated( const QColor &col );
    /**
     * Emitted when a new item has been highlighted.
     */
    void highlighted( const QColor &col );

protected:
        /**
         * @reimplemented
         */
	virtual void resizeEvent( QResizeEvent *re );

private slots:
	void slotActivated( int index );
	void slotHighlighted( int index );

private:
	void addColors();
	QColor customColor;
	QColor internalcolor;

protected:
	virtual void virtual_hook( int id, void* data );
private:
	class MyColorComboPrivate;
	MyColorComboPrivate *d;
};

#endif	// __MyColorCombo_H__
