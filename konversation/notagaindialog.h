/***************************************************************************
                          duplicatewarning.h  -  description
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

#ifndef DUPLICATEWARNING_H
#define DUPLICATEWARNING_H

#include <kdialogbase.h>
#include <klocale.h>

#include <qvbox.h>
#include <qlabel.h>
#include <qstring.h>
#include <qcheckbox.h>

/**
  *@author Matthias Gierlings
  */

class NotAgainDialog : public KDialogBase
{
	Q_OBJECT

	public:
		NotAgainDialog(QWidget* parent, QString passed_itemText);
		~NotAgainDialog();

		QVBox			*MainBox;
		QLabel		*MessageLabel;
		QCheckBox *SwitchWarning;
    QString		warningMessage;
		bool			noWarnAgain;

	signals:
		void duplicateWarningClose(bool noWarnAgain);

	protected slots:
		void closeEvent(QCloseEvent *ev);
		void slotOk();
};

#endif
