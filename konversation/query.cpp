/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  query.cpp  -  description
  begin:     Mon Jan 28 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <qhbox.h>

#include <kdialog.h>
#include <klocale.h>
#include <kstddirs.h>
#include <kdebug.h>

#include "query.h"
#include "konversationapplication.h"

Query::Query(QWidget* parent) : ChatWindow(parent)
{
  kdDebug() << "Query::Query()" << endl;

  setType(ChatWindow::Query);

  /* (this) is the main box */
  setMargin(margin());
  setSpacing(spacing());

  /* This box holds the hostmask and the close button */
  QHBox* maskQuitBox=new QHBox(this);
  maskQuitBox->setSpacing(spacing());

  queryHostmask=new QLineEdit(maskQuitBox);
  queryHostmask->setReadOnly(true);

  KStandardDirs kstd;
  QString prefix=kstd.findResource("data","konversation/images/");
  QPushButton* closeButton=new QPushButton("",maskQuitBox);
  closeButton->setPixmap(prefix+"close_pane.png");
  closeButton->setMaximumWidth(20);

  setTextView(new IRCView(this,NULL));  // Server will be set later in setServer();

  /* This box holds the input line and the log checkbox */
  QHBox* inputLogBox=new QHBox(this);
  inputLogBox->setSpacing(spacing());

  queryInput=new IRCInput(inputLogBox);

  logCheckBox=new QCheckBox(i18n("Log"),inputLogBox);
  logCheckBox->setChecked(KonversationApplication::preferences.getLog());
  setLogfileName("");

  /* connect the signals and slots */
  connect(closeButton,SIGNAL (clicked()),this,SLOT (close()) );

  connect(queryInput,SIGNAL (returnPressed()),this,SLOT (queryTextEntered()) );
  connect(queryInput,SIGNAL (textPasted(QString)),this,SLOT (textPasted(QString)) );

  connect(textView,SIGNAL (newText()),this,SLOT (newTextInView()) );
  connect(textView,SIGNAL (gotFocus()),queryInput,SLOT (setFocus()) );

  setLog(KonversationApplication::preferences.getLog());
}

Query::~Query()
{
  kdDebug() << "Query::~Query(" << getName() << ")" << endl;
}

void Query::close()
{
  emit closed(this);
}

void Query::setName(const QString& newName)
{
  ChatWindow::setName(newName);
  // don't change logfile name if query name changes
  // This will prevent Nick-Changers to create more than one log file,
  // unless we want this by turning the option Log Follows Nick off.

  if((logName=="") || (KonversationApplication::preferences.getLogFollowsNick()==false))
    setLogfileName("konversation_"+
                  ((KonversationApplication::preferences.getLowerLog())
                    ? getName().lower()
                    : getName())+".log");
}

void Query::setServer(Server* newServer)
{
  getTextView()->setServer(newServer);
  ChatWindow::setServer(newServer);
}

void Query::queryTextEntered()
{
  QString line=queryInput->text();
  if(line.lower()=="/clear") textView->clear();
  else
  {
    if(line.length()) sendQueryText(line);
  }
  queryInput->clear();
}

void Query::sendQueryText(QString line)
{
  QString output=filter.parse(server->getNickname(),line,getName());

  if(output!="")
  {
    if(filter.isAction()) appendAction(server->getNickname(),output);
    else if(filter.isCommand()) appendCommandMessage(filter.getType(),output);
    else if(filter.isProgram()) appendServerMessage(filter.getType(),output);
    else appendQuery(server->getNickname(),output);
  }
  server->queue(filter.getServerOutput());
}

void Query::newTextInView()
{
  emit newText(this);
}

void Query::setHostmask(const QString& newHostmask)
{
  hostmask=newHostmask;
  queryHostmask->setText(newHostmask);
}

int Query::spacing()
{
  return KDialog::spacingHint();
}

int Query::margin()
{
  return KDialog::marginHint();
}

void Query::updateFonts()
{
  kdDebug() << "Query::updateFonts()" << endl;

  getTextView()->setFont(KonversationApplication::preferences.getTextFont());
}

void Query::textPasted(QString text)
{
  if(server)
  {
    QStringList multiline=QStringList::split('\n',text);
    for(unsigned int index=0;index<multiline.count();index++) sendQueryText(multiline[index]);
  }
}
