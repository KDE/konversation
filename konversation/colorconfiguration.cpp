/***************************************************************************
                          colorconfiguration.cpp  -  description
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

#include <klocale.h>
#include <kdebug.h>
#include "colorconfiguration.h"

ColorConfiguration::ColorConfiguration(QString passed_actionTextColor, QString passed_backlogTextColor,
																			 QString passed_channelTextColor, QString passed_commandTextColor,
																			 QString passed_linkTextColor, QString passed_queryTextColor,
																			 QString passed_serverTextColor, QString passed_timeColor,
                                       QSize passed_windowSize)
                   : KDialogBase(0, 0, false, i18n("Color Configuration"), Ok|Apply|Cancel, Default, true)
{
	kdDebug() << "ColorConfiguration::ColorConfiguration()" << endl;

	windowSize = passed_windowSize;
 	MainBox = makeVBoxMainWidget();
	
	//upperPadBox = new QHBox(MainBox);

	widgetBox = new QHBox(MainBox);
  //leftPadBox = new QVBox(widgetBox);
	centerBox = new QVBox(widgetBox);
	rightPadBox = new QVBox(widgetBox);
	
	lowerPadBox = new QVBox(MainBox);

	centerBox->setFixedWidth(250);
	centerBox->setFixedHeight(180);

	actionBox = new QHBox(centerBox);
	actionLabel = new QLabel(i18n("Action text color"), actionBox);
	actionMessageColorSelection = new MyColorCombo(actionBox);
	actionMessageColorSelection->setMinimumWidth(50);
	actionMessageColorSelection->setMaximumWidth(50);
	
	backlogBox = new QHBox(centerBox);
	backlogLabel = new QLabel(i18n("Backlog text color"), backlogBox);
	backlogMessageColorSelection = new MyColorCombo(backlogBox);
	backlogMessageColorSelection->setMinimumWidth(50);
	backlogMessageColorSelection->setMaximumWidth(50);

	channelBox = new QHBox(centerBox);
	channelLabel = new QLabel(i18n("Channel message text color"), channelBox);
	channelMessageColorSelection = new MyColorCombo(channelBox);
	channelMessageColorSelection->setMinimumWidth(50);
	channelMessageColorSelection->setMaximumWidth(50);

	commandBox = new QHBox(centerBox);
	commandLabel = new QLabel(i18n("Command message text color"), commandBox);
	commandMessageColorSelection = new MyColorCombo(commandBox);
	commandMessageColorSelection->setMinimumWidth(50);
	commandMessageColorSelection->setMaximumWidth(50);

	linkBox = new QHBox(centerBox);
	linkLabel = new QLabel(i18n("Hyperlink text color"), linkBox);
	linkMessageColorSelection = new MyColorCombo(linkBox);
	linkMessageColorSelection->setMinimumWidth(50);
	linkMessageColorSelection->setMaximumWidth(50);

	queryBox = new QHBox(centerBox);
	queryLabel = new QLabel(i18n("Query messsage text color"), queryBox);
	queryMessageColorSelection = new MyColorCombo(queryBox);
	queryMessageColorSelection->setMinimumWidth(50);
	queryMessageColorSelection->setMaximumWidth(50);

	serverBox = new QHBox(centerBox);
	serverLabel = new QLabel(i18n("Server message text color"), serverBox);
	serverMessageColorSelection = new MyColorCombo(serverBox);
	serverMessageColorSelection->setMinimumWidth(50);
	serverMessageColorSelection->setMaximumWidth(50);

	timeBox = new QHBox(centerBox);
	timeLabel = new QLabel(i18n("Timestamp color"), timeBox);
	timeColorSelection = new MyColorCombo(timeBox);
	timeColorSelection->setMinimumWidth(50);
	timeColorSelection->setMaximumWidth(50);

	setButtonOKText(i18n("OK"),i18n("Keep changes made to configuration and close the window"));
  setButtonApplyText(i18n("Apply"),i18n("Keep changes made to configuration"));
  setButtonCancelText(i18n("Cancel"),i18n("Discards all changes made"));

	channelTextColor = QColor(passed_channelTextColor.prepend("#"));
	channelMessageColorSelection->setColor(channelTextColor);
	queryTextColor = QColor(passed_queryTextColor.prepend("#"));
	queryMessageColorSelection->setColor(queryTextColor);
	serverTextColor = QColor(passed_serverTextColor.prepend("#"));
	serverMessageColorSelection->setColor(serverTextColor);
	actionTextColor = QColor(passed_actionTextColor.prepend("#"));
	actionMessageColorSelection->setColor(actionTextColor);
	backlogTextColor = QColor(passed_backlogTextColor.prepend("#"));
	backlogMessageColorSelection->setColor(backlogTextColor);
	commandTextColor = QColor(passed_commandTextColor.prepend("#"));
	commandMessageColorSelection->setColor(commandTextColor);
	linkTextColor = QColor(passed_linkTextColor.prepend("#"));
	linkMessageColorSelection->setColor(linkTextColor);
	timeColor = QColor(passed_timeColor.prepend("#"));
	timeColorSelection->setColor(timeColor);

	connect(actionMessageColorSelection, SIGNAL(activated(const QColor&)), this, SLOT(setActionTextColor(const QColor&)));
	connect(backlogMessageColorSelection, SIGNAL(activated(const QColor&)), this, SLOT(setBacklogTextColor(const QColor&)));
	connect(channelMessageColorSelection, SIGNAL(activated(const QColor&)), this, SLOT(setChannelTextColor(const QColor&)));
	connect(commandMessageColorSelection, SIGNAL(activated(const QColor&)), this, SLOT(setCommandTextColor(const QColor&)));
	connect(linkMessageColorSelection, SIGNAL(activated(const QColor&)), this, SLOT(setLinkTextColor(const QColor&)));
	connect(queryMessageColorSelection, SIGNAL(activated(const QColor&)), this, SLOT(setQueryTextColor(const QColor&)));
	connect(serverMessageColorSelection, SIGNAL(activated(const QColor&)), this, SLOT(setServerTextColor(const QColor&)));
	connect(timeColorSelection, SIGNAL(activated(const QColor&)), this, SLOT(setTimeColor(const QColor&)));

	this->resize(windowSize);
}

ColorConfiguration::~ColorConfiguration()
{
}

void ColorConfiguration::closeEvent(QCloseEvent *ev)
{
	ev->ignore();
	emit closeFontColorConfiguration(this->size());
}

/*void ColorConfiguration::setColors(QString passed_channelTextColor, QString passed_queryTextColor, QString passed_serverTextColor,
																	 QString passed_actionTextColor, QString passed_backlogTextColor, QString passed_commandTextColor,
																	 QString passed_linkTextColor)
{
	channelTextColor = QColor(passed_channelTextColor);
	channelMessageColorSelection->setColor(channelTextColor);
	queryTextColor = QColor(passed_queryTextColor);
	queryMessageColorSelection->setColor(queryTextColor);
	serverTextColor = QColor(passed_serverTextColor);
	serverMessageColorSelection->setColor(serverTextColor);
	actionTextColor = QColor(passed_actionTextColor);
	actionMessageColorSelection->setColor(actionTextColor);
	backlogTextColor = QColor(passed_backlogTextColor);
	backlogMessageColorSelection->setColor(backlogTextColor);
	commandTextColor = QColor(passed_commandTextColor);
	commandMessageColorSelection->setColor(commandTextColor);
	linkTextColor = QColor(passed_linkTextColor);
	linkMessageColorSelection->setColor(linkTextColor);
} */

void ColorConfiguration::slotOk()
{
  slotApply();
  emit closeFontColorConfiguration(this->size());
}

void ColorConfiguration::slotApply()
{
	actionTextColorString = actionTextColor.name();
	actionTextColorString = actionTextColorString.right(6);
	backlogTextColorString = backlogTextColor.name();
	backlogTextColorString = backlogTextColorString.right(6);
	channelTextColorString = channelTextColor.name();
	channelTextColorString = channelTextColorString.right(6);
	commandTextColorString = commandTextColor.name();
	commandTextColorString = commandTextColorString.right(6);
	linkTextColorString = linkTextColor.name();
	linkTextColorString = linkTextColorString.right(6);
	queryTextColorString = queryTextColor.name();
	queryTextColorString = queryTextColorString.right(6);
	serverTextColorString = serverTextColor.name();
	serverTextColorString = serverTextColorString.right(6);
	timeColorString = timeColor.name();
	timeColorString = timeColorString.right(6);

	emit saveFontColorSettings(actionTextColorString, backlogTextColorString, channelTextColorString,
														 commandTextColorString, linkTextColorString, queryTextColorString,
														 serverTextColorString, timeColorString);
}

void ColorConfiguration::slotCancel()
{
	emit closeFontColorConfiguration(this->size());
}
