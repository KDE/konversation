/***************************************************************************
                          notagaindialog.cpp  -  description
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

#include "notagaindialog.h"

// EIS: Hier hab ich den void* ersetzt und den Cast rausgenommen. Funzt prima.
NotAgainDialog::NotAgainDialog(QWidget* parent, QString passed_itemText) : KDialogBase(parent, 0, false, i18n("Warning"), Ok, Ok, true)
{

//  connect(this, SIGNAL(okClicked()), this, SLOT(closeEvent()));

	MainBox = makeVBoxMainWidget();
	warningMessage=i18n("There already exists an item called \"%1\" in your Highlight list.\nThe second entry will be ignored for highlighting.\n").arg(passed_itemText);
	MessageLabel = new QLabel(warningMessage, MainBox);
	SwitchWarning = new QCheckBox(i18n("Don't show this message again."), MainBox);
}

NotAgainDialog::~NotAgainDialog()
{
}

void NotAgainDialog::closeEvent(QCloseEvent *ev)
{
	ev->ignore();
	emit duplicateWarningClose(SwitchWarning->isChecked());
}

void NotAgainDialog::slotOk()
{
	emit duplicateWarningClose(SwitchWarning->isChecked());
}
