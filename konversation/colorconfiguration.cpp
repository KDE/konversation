/***************************************************************************
    colorconfiguration.cpp  -  Color configuration dialog
    -------------------
    begin                : Sat Jul 20 2002
    copyright            : (C) 2002 by Matthias Gierlings
    email                : gismore@users.sourceforge.net
    redesign             : Wed Feb 19 2003
    by                   : eisfuchs@tigress.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qlayout.h>
#include <qhbox.h>

#include <klocale.h>
#include <kdebug.h>

#include "colorconfiguration.h"

ColorConfiguration::ColorConfiguration(QString passed_actionTextColor, QString passed_backlogTextColor,
                                       QString passed_channelTextColor, QString passed_commandTextColor,
                                       QString passed_linkTextColor, QString passed_queryTextColor,
                                       QString passed_serverTextColor, QString passed_timeColor,
                                       QString passed_backgroundColor,
                                       QSize passed_windowSize)
                   : KDialogBase(0, 0, false, i18n("Color Configuration"), Ok|Apply|Cancel, Default, true)
{
  kdDebug() << "ColorConfiguration::ColorConfiguration()" << endl;

  // Create the top level widget
  QWidget* page=new QWidget(this);
  setMainWidget(page);
  // Add the layout to the widget
  QGridLayout* dialogLayout=new QGridLayout(page,9,2);

  QLabel* actionLabel = new QLabel(i18n("Action text color"), page);
  MyColorCombo* actionMessageColorSelection = new MyColorCombo(page);

  QLabel* backlogLabel = new QLabel(i18n("Backlog text color"), page);
  MyColorCombo* backlogMessageColorSelection = new MyColorCombo(page);

  QLabel* channelLabel = new QLabel(i18n("Channel message text color"), page);
  MyColorCombo* channelMessageColorSelection = new MyColorCombo(page);

  QLabel* commandLabel = new QLabel(i18n("Command message text color"), page);
  MyColorCombo* commandMessageColorSelection = new MyColorCombo(page);

  QLabel* linkLabel = new QLabel(i18n("Hyperlink text color"), page);
  MyColorCombo* linkMessageColorSelection = new MyColorCombo(page);

  QLabel* queryLabel = new QLabel(i18n("Query messsage text color"), page);
  MyColorCombo* queryMessageColorSelection = new MyColorCombo(page);

  QLabel* serverLabel = new QLabel(i18n("Server message text color"), page);
  MyColorCombo* serverMessageColorSelection = new MyColorCombo(page);

  QLabel* timeLabel = new QLabel(i18n("Timestamp color"), page);
  MyColorCombo* timeColorSelection = new MyColorCombo(page);

  QLabel* backgroundLabel = new QLabel(i18n("Background color"), page);
  MyColorCombo* backgroundColorSelection = new MyColorCombo(page);

  QHBox* pad=new QHBox(page);

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
  backgroundColor = QColor(passed_backgroundColor.prepend("#"));
  backgroundColorSelection->setColor(backgroundColor);

  // Layout
  int row=0;

  dialogLayout->addWidget(actionLabel,row,0);
  dialogLayout->addWidget(actionMessageColorSelection,row,1);

  row++;
  dialogLayout->addWidget(backlogLabel,row,0);
  dialogLayout->addWidget(backlogMessageColorSelection,row,1);

  row++;
  dialogLayout->addWidget(channelLabel,row,0);
  dialogLayout->addWidget(channelMessageColorSelection,row,1);

  row++;
  dialogLayout->addWidget(commandLabel,row,0);
  dialogLayout->addWidget(commandMessageColorSelection,row,1);

  row++;
  dialogLayout->addWidget(linkLabel,row,0);
  dialogLayout->addWidget(linkMessageColorSelection,row,1);

  row++;
  dialogLayout->addWidget(queryLabel,row,0);
  dialogLayout->addWidget(queryMessageColorSelection,row,1);

  row++;
  dialogLayout->addWidget(serverLabel,row,0);
  dialogLayout->addWidget(serverMessageColorSelection,row,1);

  row++;
  dialogLayout->addWidget(timeLabel,row,0);
  dialogLayout->addWidget(timeColorSelection,row,1);

  row++;
  dialogLayout->addWidget(backgroundLabel,row,0);
  dialogLayout->addWidget(backgroundColorSelection,row,1);

  row++;
  dialogLayout->addMultiCellWidget(pad,row,row,0,1);
  dialogLayout->setRowStretch(row,10);
  dialogLayout->setColStretch(1,10);

  setButtonOKText(i18n("OK"),i18n("Keep changes made to configuration and close the window"));
  setButtonApplyText(i18n("Apply"),i18n("Keep changes made to configuration"));
  setButtonCancelText(i18n("Cancel"),i18n("Discards all changes made"));

  connect(actionMessageColorSelection, SIGNAL(activated(const QColor&)), this, SLOT(setActionTextColor(const QColor&)));
  connect(backlogMessageColorSelection, SIGNAL(activated(const QColor&)), this, SLOT(setBacklogTextColor(const QColor&)));
  connect(channelMessageColorSelection, SIGNAL(activated(const QColor&)), this, SLOT(setChannelTextColor(const QColor&)));
  connect(commandMessageColorSelection, SIGNAL(activated(const QColor&)), this, SLOT(setCommandTextColor(const QColor&)));
  connect(linkMessageColorSelection, SIGNAL(activated(const QColor&)), this, SLOT(setLinkTextColor(const QColor&)));
  connect(queryMessageColorSelection, SIGNAL(activated(const QColor&)), this, SLOT(setQueryTextColor(const QColor&)));
  connect(serverMessageColorSelection, SIGNAL(activated(const QColor&)), this, SLOT(setServerTextColor(const QColor&)));
  connect(timeColorSelection, SIGNAL(activated(const QColor&)), this, SLOT(setTimeColor(const QColor&)));
  connect(backgroundColorSelection, SIGNAL(activated(const QColor&)), this, SLOT(setBackgroundColor(const QColor&)));

  this->resize(passed_windowSize);
}

ColorConfiguration::~ColorConfiguration()
{
}

void ColorConfiguration::closeEvent(QCloseEvent *ev)
{
  ev->ignore();
  emit closeFontColorConfiguration(this->size());
}

void ColorConfiguration::slotOk()
{
  slotApply();
  emit closeFontColorConfiguration(this->size());
}

void ColorConfiguration::slotApply()
{
  QString actionTextColorString = actionTextColor.name().mid(1);
  QString backlogTextColorString = backlogTextColor.name().mid(1);
  QString channelTextColorString = channelTextColor.name().mid(1);
  QString commandTextColorString = commandTextColor.name().mid(1);
  QString linkTextColorString = linkTextColor.name().mid(1);
  QString queryTextColorString = queryTextColor.name().mid(1);
  QString serverTextColorString = serverTextColor.name().mid(1);
  QString timeColorString = timeColor.name().mid(1);
  QString backgroundColorString = backgroundColor.name().mid(1);

  emit saveFontColorSettings(actionTextColorString, backlogTextColorString, channelTextColorString,
                             commandTextColorString, linkTextColorString, queryTextColorString,
                             serverTextColorString, timeColorString, backgroundColorString);
}

void ColorConfiguration::slotCancel()
{
  emit closeFontColorConfiguration(this->size());
}

#include "colorconfiguration.moc"
