/***************************************************************************
                          colorconfiguration.h  -  description
                             -------------------
    begin                : Sat Jul 20 2002
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

#ifndef COLORCONFIGURATION_H
#define COLORCONFIGURATION_H

#include <kdialogbase.h>
#include <kcolorcombo.h>
#include <qvbox.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qwidget.h>
#include <qcolor.h>
#include <qsize.h>
#include <qstring.h>

/**
  *@author Matthias Gierlings
  */

class ColorConfiguration : public KDialogBase
{
	Q_OBJECT

	public:
		ColorConfiguration(QString passed_actionTextColor, QString passed_backlogTextColor,
											 QString passed_channelTextColor, QString passed_commandTextColor,
											 QString passed_linkTextColor, QString passed_queryTextColor,
											 QString passed_serverTextColor, QSize passed_windowSize);
		~ColorConfiguration();

	private:
		QVBox					*MainBox;
		QHBox					*channelBox, *queryBox, *serverBox, *actionBox, *backlogBox, *commandBox, *linkBox;
		QLabel				*channelLabel, *queryLabel, *serverLabel, *actionLabel, *backlogLabel, *commandLabel,
									*linkLabel;
		QColor				channelTextColor, queryTextColor, serverTextColor, actionTextColor, backlogTextColor,
									commandTextColor, linkTextColor;
		QString				channelTextColorString, queryTextColorString, serverTextColorString, actionTextColorString,
									backlogTextColorString, commandTextColorString, linkTextColorString;
		QSize					windowSize;
		KColorCombo	  *channelMessageColorSelection, *queryMessageColorSelection, *serverMessageColorSelection,
									*actionMessageColorSelection, *backlogMessageColorSelection, *commandMessageColorSelection,
									*linkMessageColorSelection;
			
	protected:
/*		void setColors(QString passed_channelTextColor, QString passed_queryTextColor, QString passed_serverTextColor,
									 QString passed_actionTextColor, QString passed_backlogTextColor, QString passed_commandTextColor,
									 QString passed_linkTextColor);*/
		

	signals:
		void closeFontColorConfiguration(QSize windowSize);
		void saveFontColorSettings(QString channelTextColor, QString queryTextColor, QString serverTextColor,
															 QString actionTextColor, QString backlogTextColor, QString commandTextColor,
															 QString linkTextColor);
	
	protected slots:
		void closeEvent(QCloseEvent *ev);
		void setChannelTextColor(const QColor& passed_color) {channelTextColor = passed_color;}
		void setQueryTextColor(const QColor& passed_color) {queryTextColor = passed_color;}
		void setServerTextColor(const QColor& passed_color) {serverTextColor = passed_color;}
		void setActionTextColor(const QColor& passed_color) {actionTextColor = passed_color;}
		void setBacklogTextColor(const QColor& passed_color) {backlogTextColor = passed_color;}
		void setCommandTextColor(const QColor& passed_color) {commandTextColor = passed_color;}
		void setLinkTextColor(const QColor& passed_color) {linkTextColor = passed_color;}
    void slotOk();
		void slotApply();
		void slotCancel();
};

#endif
