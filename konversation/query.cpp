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

#include <qhbox.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qpopupmenu.h>
#include <qlineedit.h>

#include <klocale.h>
#include <kstddirs.h>
#include <kdebug.h>
#include <kmessagebox.h>

#include "query.h"
#include "server.h"
#include "konversationapplication.h"
#include "ircinput.h"
#include "ircview.h"

const int POPUP_WHOIS =0xfe;
const int POPUP_IGNORE=0xff;

#ifdef USE_MDI
Query::Query(QString caption) : ChatWindow(caption)
#else
Query::Query(QWidget* parent) : ChatWindow(parent)
#endif
{
  // don't setName here! It will break logfiles!
  //   setName("QueryWidget");
  setType(ChatWindow::Query);
  
  setChannelEncodingEnabled(true);

  awayChanged=false;
  awayState=false;

  queryHostmask=new QLineEdit(this, "query_hostmask");
  queryHostmask->setReadOnly(true);
  queryHostmask->installEventFilter(this);

  setTextView(new IRCView(this,NULL));  // Server will be set later in setServer();

  // link "Whois" and "Ignore" menu items into ircview popup
  QPopupMenu* popup=textView->getPopup();
  popup->insertItem(i18n("Whois"),POPUP_WHOIS);  // TODO: let the ircview give the id back rather than specifying it ourselves?
  popup->insertItem(i18n("Ignore"),POPUP_IGNORE);

  // This box holds the input line
  QHBox* inputBox=new QHBox(this, "input_log_box");
  inputBox->setSpacing(spacing());

  awayLabel=new QLabel(i18n("(away)"),inputBox);
  awayLabel->hide();
  queryInput=new IRCInput(inputBox);
  queryInput->installEventFilter(this);

  setLogfileName(QString::null);

  // connect the signals and slots
  connect(queryInput,SIGNAL (submit()),this,SLOT (queryTextEntered()) );
  connect(queryInput,SIGNAL (textPasted(QString)),this,SLOT (textPasted(QString)) );

  connect(textView,SIGNAL (newText(const QString&,bool)),this,SLOT (newTextInView(const QString&,bool)) );
  connect(textView,SIGNAL (gotFocus()),this,SLOT (adjustFocus()) );
  connect(textView,SIGNAL (sendFile()),this,SLOT (sendFileMenu()) );
  connect(textView,SIGNAL (extendedPopup(int)),this,SLOT (popup(int)) );
  connect(textView,SIGNAL (autoText(const QString&)),this,SLOT (sendQueryText(const QString&)) );

  updateFonts();

  setLog(KonversationApplication::preferences.getLog());
}

Query::~Query()
{
#ifdef USE_MDI
  server->removeQuery(this);
#endif
}

void Query::setName(const QString& newName)
{
  ChatWindow::setName(newName);
  // don't change logfile name if query name changes
  // This will prevent Nick-Changers to create more than one log file,
  // unless we want this by turning the option Log Follows Nick off.

  if((logName.isEmpty()) || (KonversationApplication::preferences.getLogFollowsNick()==false)) {
    setLogfileName((
                      (KonversationApplication::preferences.getLowerLog()) ? getName().lower() : getName()
                  ));
  }
}

void Query::queryTextEntered()
{
  QString line=queryInput->text();
  queryInput->clear();
  if(line.lower()=="/clear") textView->clear();
  if(line.lower()=="/part") server->closeQuery(getName());
  else
  {
    if(line.length()) sendQueryText(line);
  }
}

void Query::sendQueryText(const QString& sendLine)
{
  // create a work copy
  QString output(sendLine);
  // replace aliases and wildcards
  if(server->getOutputFilter()->replaceAliases(output)) {
    output = server->parseWildcards(output, server->getNickname(), getName(), QString::null, QString::null, QString::null);
  }

  // encoding stuff is done in Server()
  Konversation::OutputFilterResult result = server->getOutputFilter()->parse(server->getNickname(), output, getName());

  if(!result.output.isEmpty())
  {
    if(result.type == Konversation::Action) appendAction(server->getNickname(), result.output);
    else if(result.type == Konversation::Command) appendCommandMessage(result.typeString, result.output);
    else if(result.type == Konversation::Program) appendServerMessage(result.typeString, result.output);
    else if(!result.typeString.isEmpty()) appendQuery(result.typeString, result.output);
    else appendQuery(server->getNickname(), result.output);
  }
  
  server->queue(result.toServer);
}

void Query::newTextInView(const QString& highlightColor,bool important)
{
  emit newText(this,highlightColor,important);
}

void Query::setHostmask(const QString& newHostmask)
{
  hostmask=newHostmask;
  queryHostmask->setText(newHostmask);
}

void Query::updateFonts()
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

  queryInput->setPaletteForegroundColor(fg);
  queryInput->setPaletteBackgroundColor(bg);
  queryInput->setFont(KonversationApplication::preferences.getTextFont());

  queryHostmask->setPaletteForegroundColor(fg);
  queryHostmask->setPaletteBackgroundColor(bg);
  queryHostmask->setFont(KonversationApplication::preferences.getTextFont());

  getTextView()->setFont(KonversationApplication::preferences.getTextFont());

  if(KonversationApplication::preferences.getShowBackgroundImage())
      getTextView()->setViewBackground(KonversationApplication::preferences.getColor("TextViewBackground"),
                                   KonversationApplication::preferences.getBackgroundImageName());
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

void Query::indicateAway(bool show)
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
void Query::showEvent(QShowEvent*)
{
  if(awayChanged)
  {
    awayChanged=false;
    indicateAway(awayState);
  }
}

void Query::popup(int id)
{
  if(id==POPUP_WHOIS)
    sendQueryText(KonversationApplication::preferences.getCommandChar()+"WHOIS "+getName());
  else if(id==POPUP_IGNORE)
  {
    sendQueryText(KonversationApplication::preferences.getCommandChar()+"IGNORE -ALL "+getName()+"!*");
    int rc=KMessageBox::questionYesNo(this,
                                      i18n("Do you want to close this query after ignoring this nickname?"),
                                      i18n("Close This Query"),
                                      KStdGuiItem::yes(),
                                      KStdGuiItem::no(),
                                      "CloseQueryAfterIgnore");

    if(rc==KStdGuiItem::Yes) closeYourself();
  }
  else
    kdDebug() << "Query::popup(): Popup id " << id << " does not belong to me!" << endl;
}

void Query::adjustFocus()
{
  queryInput->setFocus();
}

void Query::sendFileMenu()
{
  emit sendFile(getName());
}

QString Query::getTextInLine() { return queryInput->text(); }

bool Query::frontView()        { return true; }
bool Query::searchView()       { return true; }

void Query::appendInputText(const QString& s)
{
  queryInput->setText(queryInput->text() + s);
}

void Query::setChannelEncoding(const QString& encoding)  // virtual
{
  KonversationApplication::preferences.setChannelEncoding(server->getServerGroup(), getName(), encoding);
}

QString Query::getChannelEncoding()  // virtual
{
  return KonversationApplication::preferences.getChannelEncoding(server->getServerGroup(), getName());
}

void Query::closeYourself()
{
#ifndef USE_MDI
  server->removeQuery(this);
#endif
}

#ifdef USE_MDI
void Query::closeYourself(ChatWindow*)
{
  emit chatWindowCloseRequest(this);
}

void Query::serverQuit(const QString&)
{
  closeYourself(this);
}
#endif

#include "query.moc"
