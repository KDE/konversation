/***************************************************************************
                          highlightdialog.cpp  -  description
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

#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>

#include <qstring.h>
#include <iostream.h>
#include <stdlib.h>

#include "highlightdialog.h"

HighlightDialog::HighlightDialog(QWidget* parent, QPtrList<Highlight> passed_HighlightList, QSize passed_windowSize) : KDialogBase(parent, 0, false, "HighlightEdit", Ok|Cancel, Default, false)
{
	highlightList = passed_HighlightList;
	windowSize = passed_windowSize;

	MainBox = makeVBoxMainWidget();
	HighlightBrowserBox = new QVGroupBox(i18n("Highlight list"), MainBox);
	
  HighlightBrowser = new HighlightView(HighlightBrowserBox);
  HighlightBrowser->setFullWidth();
	HighlightBrowser->addColumn(i18n ("Highlights"));
	HighlightBrowser->setSelectionMode(QListView::Single);

	InputLineBox = new QHBox(HighlightBrowserBox);
	InputLineBox->setSpacing(KDialog::spacingHint());

	InputLineLabel = new QLabel(i18n("Pattern:"), InputLineBox);
	InputLine = new KLineEdit(InputLineBox);
	InputLine->setFocus();

	ColorSelection = new KColorCombo(InputLineBox);
	ColorSelection->setMinimumWidth(50);
	ColorSelection->setMaximumWidth(50);

	RemoveButton = new KPushButton(i18n("Remove"), InputLineBox);

  connect(InputLine, SIGNAL(returnPressed()), this, SLOT(addHighlight()));
	connect(InputLine, SIGNAL(returnPressed()), this, SLOT(changeHighlightText()));
	connect(InputLine, SIGNAL(textChanged(const QString&)), this, SLOT(updateHighlight(const QString&)));
	connect(ColorSelection, SIGNAL(activated(const QColor&)), this, SLOT(changeHighlightColor(const QColor&)));
	connect(RemoveButton, SIGNAL(clicked()), this, SLOT(removeHighlight()));
	connect(HighlightBrowser, SIGNAL(selectionChanged(QListViewItem*)), this, SLOT(changeColorSelectionColor(QListViewItem*)));
	connect(HighlightBrowser, SIGNAL(selectionChanged(QListViewItem*)), this, SLOT(updateInputLine(QListViewItem*)));
	connect(HighlightBrowser, SIGNAL(clicked(QListViewItem*)), this, SLOT(unselectHighlight(QListViewItem*)));
	connect(HighlightBrowser, SIGNAL(clicked(QListViewItem*)), this, SLOT(undoHighlightTextChange(QListViewItem*)));
	connect(this, SIGNAL(highlightChanged(QListViewItem*)), this, SLOT(unselectHighlight(QListViewItem*)));

	highlightEdited = false;
	this->show();
	this->addHighlightList();
}

HighlightDialog::~HighlightDialog()
{
}

void HighlightDialog::addHighlightList()
{
	for(unsigned int i = 0; i != highlightList.count(); i++)
	{
		currentHighlight = highlightList.at(i);
		usedID[currentHighlight->getID()] = true;
		if(highestID < currentHighlight->getID()) highestID = currentHighlight->getID();
	  currentHighlightViewItem = new HighlightViewItem(HighlightBrowser, currentHighlight);
		highlightItemList.append(currentHighlightViewItem);
		ColorSelection->setColor(currentHighlightViewItem->getColor());
	}
}
/*
int	HighlightDialog::assignID()
{
	freeIDfound = false;

	for(int i = 0; i <= highestID; i++)
	{
		if(usedID[i] == 0)
		{
			usedID[i] = true;
			freeID = i;
			freeIDfound = true;
		}
   }
	if(freeIDfound == false)
	{
		highestID++;
		usedID[highestID] = true;
		freeID = highestID;
	}
	return freeID;
}
*/
void HighlightDialog::addHighlight()
{
	if((selectedHighlightViewItem = HighlightBrowser->selectedItem()) == 0)
	{
		if((InputLine->text() != ""))
		{
			if(HighlightBrowser->findItem(InputLine->text(), 0, Qt::ExactMatch) != 0)
			{
				emit duplicateItemDetected(InputLine->text());
			}
//			currentHighlight = new Highlight(InputLine->text(), ColorSelection->color(), this->assignID());
			currentHighlight = new Highlight(InputLine->text(), ColorSelection->color());
			highlightList.append(currentHighlight);
			currentHighlightViewItem = new HighlightViewItem(HighlightBrowser, currentHighlight);
			InputLine->setText("");
		}
		else noEmptyPatterns();
	}
}

void HighlightDialog::noEmptyPatterns()
{
  KMessageBox::sorry(this,i18n("You can not add an Highlight with empty pattern."),i18n("Empty Pattern"));
}

void HighlightDialog::changeHighlightColor(const QColor& passed_itemColor)
{
	selectedHighlightViewItem = HighlightBrowser->selectedItem();
	if(selectedHighlightViewItem != 0)
	{
		selectedHighlightViewItem->setColor(passed_itemColor);
		selectedHighlightViewItem->repaint();
	}
}

void HighlightDialog::changeColorSelectionColor(QListViewItem* passed_selectedHighlightViewItem)
{
	selectedHighlightViewItem = (HighlightViewItem*) passed_selectedHighlightViewItem;
	if(selectedHighlightViewItem != 0)
	{
		ColorSelection->setColor(selectedHighlightViewItem->getColor());
	}
}

void HighlightDialog::removeHighlight()
{
	if(selectedHighlightViewItem != 0)
	{
		HighlightBrowser->removeItem(HighlightBrowser->selectedItem());
		InputLine->setText("");
	}
}

void HighlightDialog::unselectHighlight(QListViewItem* passed_selectedHighlightViewItem)
{
	if((selectedHighlightViewItem = (HighlightViewItem*) passed_selectedHighlightViewItem) == oldSelectedHighlightViewItem)
	{
		HighlightBrowser->clearSelection();
		InputLine->setText("");
		oldSelectedHighlightViewItem = 0;
	}
	else oldSelectedHighlightViewItem = selectedHighlightViewItem;
}

void HighlightDialog::updateInputLine(QListViewItem* passed_selectedHighlightViewItem)
{
	selectedHighlightViewItem = (HighlightViewItem*) passed_selectedHighlightViewItem;
	if(selectedHighlightViewItem != 0)
	{
		InputLine->setText(selectedHighlightViewItem->getText());
	}
}

void HighlightDialog::updateHighlight(const QString& passed_InputLineText)
{
	if((selectedHighlightViewItem = HighlightBrowser->selectedItem()) != 0)
 	{
  	if(highlightEdited == false)
		{
			backupHighlightText = selectedHighlightViewItem->getText();
			backupHighlightViewItem = selectedHighlightViewItem;
			highlightEdited = true;
		}
		selectedHighlightViewItem->setText(0, passed_InputLineText);
	}
}

void HighlightDialog::changeHighlightText()
{

// EIS: if-Bedingung für beide Abfragen schachteln, um den Segfault zu beheben
  if((/*selectedHighlightViewItem = */HighlightBrowser->selectedItem()) != 0)
  {
		if(selectedHighlightViewItem->getText() != "")
  	{
  		highlightEdited = false;
	  	InputLine->setText("");
		  emit highlightChanged((QListViewItem*) selectedHighlightViewItem);
  	}
  	else if(selectedHighlightViewItem->getText() == "") noEmptyPatterns();
  }
}

void HighlightDialog::undoHighlightTextChange(QListViewItem* passed_selectedHighlightViewItem)
{
	if(highlightEdited == true)
	{
		backupHighlightViewItem->setText(0, backupHighlightText);
		highlightEdited = false;
	}
}

void HighlightDialog::slotOk()
{
  slotApply();
  slotCancel();
}
/*
void HighlightDialog::closeEvent(QCloseEvent *ev)
{
  ev->ignore();
  slotCancel();
}
*/
void HighlightDialog::slotApply()
{
  kdDebug() << "slotApply" << endl;
  emit applyClicked(highlightList);
}
  
void HighlightDialog::slotCancel()
{
  kdDebug() << "slotCancel" << endl;
  emit cancelClicked(size());
}
