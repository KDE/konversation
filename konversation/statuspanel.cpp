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
*/

#include <qpushbutton.h>
#include <qlabel.h>

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>

#include "statuspanel.h"
#include "konversationapplication.h"
#include "ircinput.h"
#include "ircview.h"
#include "server.h"

StatusPanel::StatusPanel(QWidget* parent) :
              ChatWindow(parent)
{
  setType(ChatWindow::Status);

  awayChanged=false;
  awayState=false;

  // set up text view, will automatically take care of logging
  setTextView(new IRCView(this,NULL));  // Server will be set later in setServer()

  QHBox* commandLineBox=new QHBox(this);
  commandLineBox->setSpacing(spacing());
  commandLineBox->setMargin(0);

  nicknameButton=new QPushButton(i18n("Nickname"),commandLineBox);
  awayLabel=new QLabel(i18n("(away)"),commandLineBox);
  awayLabel->hide();
  statusInput=new IRCInput(commandLineBox);

  setLog(KonversationApplication::preferences.getLog());
  setLogfileName("konversation");

  connect(getTextView(),SIGNAL (gotFocus()),statusInput,SLOT (setFocus()) );

  connect(getTextView(),SIGNAL (newText(const QString&,bool)),this,SLOT (newTextInView(const QString&,bool)) );
  connect(getTextView(),SIGNAL (sendFile()),this,SLOT (sendFileMenu()) );
  connect(getTextView(),SIGNAL (autoText(const QString&)),this,SLOT (sendStatusText(const QString&)) );

  connect(statusInput,SIGNAL (pageUp()),getTextView(),SLOT (pageUp()) );
  connect(statusInput,SIGNAL (pageDown()),getTextView(),SLOT (pageDown()) );

  connect(statusInput,SIGNAL (submit()),this,SLOT(statusTextEntered()) );
  connect(statusInput,SIGNAL (textPasted(QString)),this,SLOT(textPasted(QString)) );

  updateFonts();
}

StatusPanel::~StatusPanel()
{
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
  // create a work copy
  QString output(sendLine);
  // replace aliases and wildcards
  if(filter.replaceAliases(output)) output=server->parseWildcards(output,server->getNickname(),QString::null,QString::null,QString::null,QString::null);

  // encoding stuff is done in Server()
  output=filter.parse(server->getNickname(),output,QString::null);

  if(!output.isEmpty()) appendServerMessage(filter.getType(),output);

  server->queue(filter.getServerOutput());
}

void StatusPanel::statusTextEntered()
{
  QString line=statusInput->text();
  statusInput->clear();

  if(line.lower()=="/clear") textView->clear();
  else
  {
    if(line.length()) sendStatusText(line);
  }
}

void StatusPanel::newTextInView(const QString& highlightColor,bool important)
{
  emit newText(this,highlightColor,important);
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
  QString fgString;
  QString bgString;

  if(KonversationApplication::preferences.getColorInputFields())
  {
    fgString="#"+KonversationApplication::preferences.getColor("ChannelMessage");
    bgString="#"+KonversationApplication::preferences.getColor("TextViewBackground");
  }
  else
  {
    fgString=colorGroup().foreground().name();
    bgString=colorGroup().base().name();
  }

  const QColor fg(fgString);
  const QColor bg(bgString);

  statusInput->setPaletteForegroundColor(fg);
  statusInput->setPaletteBackgroundColor(bg);
  statusInput->setFont(KonversationApplication::preferences.getTextFont());

  getTextView()->setFont(KonversationApplication::preferences.getTextFont());
  getTextView()->setViewBackground(KonversationApplication::preferences.getColor("TextViewBackground"),
                                   KonversationApplication::preferences.getBackgroundImageName());
  nicknameButton->setFont(KonversationApplication::preferences.getTextFont());
}

void StatusPanel::sendFileMenu()
{
  emit sendFile();
}

void StatusPanel::indicateAway(bool show)
{
  // QT does not redraw the label properly when they are not on screen
  // while getting hidden, so we remember the "soon to be" state here.
  if(isHidden())
  {
    awayChanged=true;
    awayState=show;
  }
  else
  {
    if(show)
      awayLabel->show();
    else
      awayLabel->hide();
  }
}

// fix QTs broken behavior on hidden QListView pages
void StatusPanel::showEvent(QShowEvent*)
{
  if(awayChanged)
  {
    awayChanged=false;
    indicateAway(awayState);
  }
}

QString StatusPanel::getTextInLine() { return statusInput->text(); }

bool StatusPanel::frontView()        { return true; }
bool StatusPanel::searchView()       { return true; }

void StatusPanel::closeYourself()
{
  int result=KMessageBox::warningYesNo(
                this,
                i18n("Do you really want to disconnect from '%1'?").arg(server->getServerName()),
                i18n("Quit Server"),
                KStdGuiItem::yes(),
                KStdGuiItem::no(),
                "QuitServerTab");

  if(result==KMessageBox::Yes)
  {
    server->quitServer();
    delete server;
    delete this;
  }
}

#include "statuspanel.moc"
