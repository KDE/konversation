/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  statuspanel.cpp  -  The panel where the server status messages go
  begin:     Sam Jan 18 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>

#include "statuspanel.h"
#include "konversationapplication.h"
#include "ircinput.h"

StatusPanel::StatusPanel(QWidget* parent) :
              ChatWindow(parent)
{
  kdDebug() << "StatusPanel::StatusPanel()" << endl;

  setType(ChatWindow::Status);

  // set up text view, will automatically take care of logging
  setTextView(new IRCView(this,NULL));  // Server will be set later in setServer()

  QHBox* commandLineBox=new QHBox(this);
  commandLineBox->setSpacing(spacing());
  commandLineBox->setMargin(0);

  nicknameButton=new QPushButton(i18n("Nickname"),commandLineBox);
  statusInput=new IRCInput(commandLineBox);

  lagOMeter=new QLabel(i18n("Lag: not known"),commandLineBox,"status_panel_lagometer");
  
  setLog(KonversationApplication::preferences.getLog());
  setLogfileName("konversation.log");

  connect(getTextView(),SIGNAL (gotFocus()),statusInput,SLOT (setFocus()) );

  connect(getTextView(),SIGNAL (sendFile()),this,SLOT (sendFileMenu()) );

  connect(statusInput,SIGNAL (pageUp()),getTextView(),SLOT (pageUp()) );
  connect(statusInput,SIGNAL (pageDown()),getTextView(),SLOT (pageDown()) );

  connect(statusInput,SIGNAL (returnPressed()),this,SLOT(statusTextEntered()) );
  connect(statusInput,SIGNAL (textPasted(QString)),this,SLOT(textPasted(QString)) );

  updateFonts();
}

StatusPanel::~StatusPanel()
{
  kdDebug() << "StatusPanel::~StatusPanel()" << endl;
}

void StatusPanel::setNickname(const QString& newNickname)
{
  nicknameButton->setText(newNickname);
}

void StatusPanel::adjustFocus()
{
  statusInput->setFocus();
}

void StatusPanel::sendStatusText(QString sendLine)
{
  // encoding stuff is done in Server()
  QString output=filter.parse(server->getNickname(),sendLine, QString::null);

  if(!output.isEmpty()) appendServerMessage(filter.getType(),output);

  server->queue(filter.getServerOutput());
}

void StatusPanel::statusTextEntered()
{
  QString line=statusInput->text();

  if(line.lower()=="/clear") textView->clear();
  else
  {
    if(line.length()) sendStatusText(line);
  }

  statusInput->clear();
}

void StatusPanel::newTextInView()
{
  // kdDebug() << "StatusPanel::newTextInView(): this=" << this << endl;
  emit newText(this);
}

void StatusPanel::textPasted(QString text)
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
      sendStatusText(line);
    }
  }
}

void StatusPanel::updateFonts()
{
  kdDebug() << "StatusPanel::updateFonts()" << endl;

  statusInput->setFont(KonversationApplication::preferences.getTextFont());
  getTextView()->setFont(KonversationApplication::preferences.getTextFont());
  getTextView()->setPaper(QColor("#"+KonversationApplication::preferences.getTextViewBackground()));
  nicknameButton->setFont(KonversationApplication::preferences.getTextFont());
}

void StatusPanel::sendFileMenu()
{
  emit sendFile();
}

void StatusPanel::updateLag(int msec)
{
  lagOMeter->setText(i18n("Lag: %1 ms").arg(msec));
  emit lag(getServer(),msec);
}

void StatusPanel::closeYourself()
{
  int result=KMessageBox::warningYesNo(
                this,
                i18n("Do you really want to disconnect from %1?").arg(server->getServerName()),
                i18n("Quit server"),
                KStdGuiItem::yes(),
                KStdGuiItem::no(),
                "QuitServerOnTabClose");

  if(result==KMessageBox::Yes)
  {
    server->quitServer();
    delete server;
    delete this;
  }
}

#include "statuspanel.moc"
