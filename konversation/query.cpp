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

#include <klocale.h>
#include <kstddirs.h>
#include <kdebug.h>

#include "query.h"
#include "konversationapplication.h"
#include "ircinput.h"

Query::Query(QWidget* parent) : ChatWindow(parent)
{
  setName("QueryWidget");
  kdDebug() << "Query::Query()" << endl;

  setType(ChatWindow::Query);

  queryHostmask=new QLineEdit(this, "query_hostmask");
  queryHostmask->setReadOnly(true);

  setTextView(new IRCView(this,NULL));  // Server will be set later in setServer();

  // This box holds the input line and the log checkbox
  QHBox* inputLogBox=new QHBox(this, "input_log_box");
  inputLogBox->setSpacing(spacing());

  queryInput=new IRCInput(inputLogBox);

  logCheckBox=new QCheckBox(i18n("Log"),inputLogBox);
  logCheckBox->setChecked(KonversationApplication::preferences.getLog());
  setLogfileName(QString::null);

  // connect the signals and slots
  connect(queryInput,SIGNAL (returnPressed()),this,SLOT (queryTextEntered()) );
  connect(queryInput,SIGNAL (textPasted(QString)),this,SLOT (textPasted(QString)) );

  connect(queryInput,SIGNAL (pageUp()),getTextView(),SLOT (pageUp()) );
  connect(queryInput,SIGNAL (pageDown()),getTextView(),SLOT (pageDown()) );
  
  connect(textView,SIGNAL (newText(const QString&)),this,SLOT (newTextInView(const QString&)) );
  connect(textView,SIGNAL (gotFocus()),this,SLOT (adjustFocus()) );
  connect(textView,SIGNAL (sendFile()),this,SLOT (sendFileMenu()) );


  updateFonts();

  setLog(KonversationApplication::preferences.getLog());
}

Query::~Query()
{
  kdDebug() << "Query::~Query(" << getName() << ")" << endl;
}

void Query::setName(const QString& newName)
{
  ChatWindow::setName(newName);
  // don't change logfile name if query name changes
  // This will prevent Nick-Changers to create more than one log file,
  // unless we want this by turning the option Log Follows Nick off.

  if((logName.isEmpty()) || (KonversationApplication::preferences.getLogFollowsNick()==false))
    setLogfileName("konversation_"+
                  ((KonversationApplication::preferences.getLowerLog())
                    ? getName().lower()
                    : getName())+".log");
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

void Query::sendQueryText(const QString& sendLine)
{
  // encoding stuff is done in Server()
  QString output=filter.parse(server->getNickname(),sendLine,getName());

  if(!output.isEmpty())
  {
    if(filter.isAction()) appendAction(server->getNickname(),output);
    else if(filter.isCommand()) appendCommandMessage(filter.getType(),output);
    else if(filter.isProgram()) appendServerMessage(filter.getType(),output);
    else appendQuery(server->getNickname(),output);
  }
  server->queue(filter.getServerOutput());
}

void Query::newTextInView(const QString& highlightColor)
{
  emit newText(this,highlightColor);
}

void Query::setHostmask(const QString& newHostmask)
{
  hostmask=newHostmask;
  queryHostmask->setText(newHostmask);
}

void Query::updateFonts()
{
  kdDebug() << "Query::updateFonts()" << endl;

  queryHostmask->setFont(KonversationApplication::preferences.getTextFont());
  queryInput->setFont(KonversationApplication::preferences.getTextFont());
  logCheckBox->setFont(KonversationApplication::preferences.getTextFont());
  getTextView()->setFont(KonversationApplication::preferences.getTextFont());
  getTextView()->setPaper(QColor("#"+KonversationApplication::preferences.getTextViewBackground()));
}

void Query::textPasted(QString text)
{
  if(server)
  {
    QStringList multiline=QStringList::split('\n',text);
    for(unsigned int index=0;index<multiline.count();index++)
    {
      QString line=multiline[index];
      QString cChar(KonversationApplication::preferences.getCommandChar());
      // make sure that lines starting with command char get escaped
      if(line.startsWith(cChar)) line=cChar+line;
      sendQueryText(line);
    }
  }
}

void Query::adjustFocus()
{
  queryInput->setFocus();
}

void Query::sendFileMenu()
{
  emit sendFile(getName());
}

void Query::closeYourself()
{
  server->removeQuery(this);
}


#include "query.moc"
