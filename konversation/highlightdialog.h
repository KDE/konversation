/***************************************************************************
                          highlightdialog.h  -  description
                             -------------------
    begin                : Sat Jun 15 2002
    copyright            : (C) 2002 by Matthias Gierlings
    email                : gismore@users.sourceforge.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef HIGHLIGHTDIALOG_H
#define HIGHLIGHTDIALOG_H

#include <kdialogbase.h>
#include <klineedit.h>
#include <kpushbutton.h>
#include <kcolorcombo.h>

#include <qvgroupbox.h>
#include <qvbox.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qptrlist.h>
#include <qstring.h>
#include <qcheckbox.h>

#include "highlight.h"
#include "highlightview.h"
#include "highlightviewitem.h"

/**
  *@author Matthias Gierlings
  */

class HighlightDialog : public KDialogBase
{
	Q_OBJECT

	public:
		HighlightDialog(QWidget* parent, QPtrList<Highlight> passed_HighlightList, QSize passed_WindowSize);
		~HighlightDialog();

	private:
		QVGroupBox				*HighlightBrowserBox;
    QCheckBox*        highlightNickCheck;
    QCheckBox*        highlightOwnLinesCheck;
		QVBox							*MainBox;
		QHBox							*InputLineBox;
		QLabel						*InputLineLabel;
		QSize							windowSize;
		QString						backupHighlightText;
		KLineEdit					*InputLine;
		KPushButton				*RemoveButton;
		KColorCombo				*ColorSelection;
		HighlightView			*HighlightBrowser;
		HighlightViewItem	*currentHighlightViewItem, *selectedHighlightViewItem,
											*oldSelectedHighlightViewItem, *backupHighlightViewItem;
		Highlight					*currentHighlight;
		bool							/*usedID[16384], */ showDuplicateWarning, /* freeIDfound, */ highlightEdited;
//		int								freeID, highestID;

		QPtrList<Highlight>										highlightList;
		QPtrList<HighlightViewItem>						highlightItemList;

		void addHighlightList();

	protected:
    QPtrList<Highlight> getHighlightList();
//		int assignID();

	protected slots:
		void addHighlight();
		void removeHighlight();
    void noEmptyPatterns();
    void highlightNickChanged(int state);
    void highlightOwnLinesChanged(int state);
    void nickColorChanged(const QColor& newColor);
    void ownLinesColorChanged(const QColor& newColor);
    void changeHighlightColor(const QColor& passed_itemColor);
		void changeColorSelectionColor(QListViewItem* passed_selectedHighlightViewItem);
		void changeHighlightText();
		void unselectHighlight(QListViewItem* passed_selectedHighlightViewItem);
		void undoHighlightTextChange(QListViewItem* passed_selectedHighlightViewItem);
		void updateInputLine(QListViewItem* passed_selectedHighlightViewItem);
		void updateHighlight(const QString& passed_InputLineText);
		void slotOk();
		void slotApply();
		void slotCancel();
//		void closeEvent(QCloseEvent *ev);		 // trap the native window close event to manage the windows manually.

signals:
		void duplicateItemDetected(QString duplicateItemText);
		void highlightChanged(QListViewItem* passed_selectedHighlightViewItem);

    void applyClicked(QPtrList<Highlight> highlightList);
    void cancelClicked(QSize);	//returns the current size of the dialog window.

};

#endif
