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
*/

#include <iostream>

#include <qhbox.h>

#include <kdialog.h>
#include <klocale.h>
#include <kstddirs.h>

#include "query.h"
#include "konversationapplication.h"

Query::Query(QWidget* parent) : ChatWindow(parent)
{
  cerr << "Query::Query()" << endl;
  /* This is the main box */
  queryPane=new QVBox(parent);
  queryPane->setMargin(margin());
  queryPane->setSpacing(spacing());

  /* This box holds the hostmask and the close button */
  QHBox* maskQuitBox=new QHBox(queryPane);
  maskQuitBox->setSpacing(spacing());

  queryHostmask=new QLineEdit(maskQuitBox);
  queryHostmask->setReadOnly(true);

  KStandardDirs kstd;
  QString prefix=kstd.findResource("data","konversation/images/");
  QPushButton* closeButton=new QPushButton("",maskQuitBox);
  closeButton->setPixmap(prefix+"close_pane.png");
  closeButton->setMaximumWidth(20);

  setTextView(new IRCView(queryPane));

  /* This box holds the input line and the log checkbox */
  QHBox* inputLogBox=new QHBox(queryPane);
  inputLogBox->setSpacing(spacing());

  queryInput=new IRCInput(inputLogBox);

  logCheckBox=new QCheckBox(i18n("Log"),inputLogBox);
  logCheckBox->setChecked(KonversationApplication::preferences.getLog());
  setLogfileName("");

  /* connect the signals and slots */
  connect(closeButton,SIGNAL (clicked()),this,SLOT (close()) );
  connect(queryInput,SIGNAL (returnPressed()),this,SLOT (queryTextEntered()) );
  connect(textView,SIGNAL (newText()),this,SLOT (newTextInView()) );
  connect(textView,SIGNAL (gotFocus()),queryInput,SLOT (setFocus()) );

  setLog(KonversationApplication::preferences.getLog());
}

Query::~Query()
{
  cerr << "Query::~Query(" << getQueryName() << ")" << endl;
}

void Query::close()
{
  emit closed(this);
}

void Query::setQueryName(const QString& newName)
{
  queryName=newName;
  if(logName=="") setLogfileName("konversation_"+getQueryName().lower()+".log");
}

void Query::queryTextEntered()
{
  QString line=queryInput->text();
  if(line.lower()=="/clear") textView->clear();  // FIXME: to get rid of too wide lines
  else
{
  if(line.length())
  {
    QString output=filter.parse(line,getQueryName());

    if(output!="")
    {
      if(filter.isAction()) appendAction(server->getNickname(),output);
      else appendQuery(server->getNickname(),output);
    }

    server->queue(filter.getServerOutput());
  }
}
  queryInput->clear();
}

void Query::newTextInView()
{
  emit newText(getQueryPane());
}

void Query::setHostmask(const QString& newHostmask)
{
  hostmask=newHostmask;
  queryHostmask->setText(newHostmask);
}

QWidget* Query::getQueryPane()
{
  return queryPane;
}

QString& Query::getQueryName()
{
  return queryName;
}

int Query::spacing()
{
  return KDialog::spacingHint();
}

int Query::margin()
{
  return KDialog::marginHint();
}
