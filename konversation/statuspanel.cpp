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
#include <qcombobox.h>
#include <qlabel.h>
#include <qhbox.h>
#include <qtextcodec.h>

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>

#include "statuspanel.h"
#include "konversationapplication.h"
#include "ircinput.h"
#include "ircview.h"
#include "server.h"


#ifdef USE_MDI
StatusPanel::StatusPanel(QString caption) : ChatWindow(caption)
#else
StatusPanel::StatusPanel(QWidget* parent) : ChatWindow(parent)
#endif
{
  setType(ChatWindow::Status);

  setChannelEncodingSupported(true);

  awayChanged=false;
  awayState=false;

  // set up text view, will automatically take care of logging
  setTextView(new IRCView(this,NULL));  // Server will be set later in setServer()

  QHBox* commandLineBox=new QHBox(this);
  commandLineBox->setSpacing(spacing());
  commandLineBox->setMargin(0);

  nicknameCombobox=new QComboBox(commandLineBox);
  nicknameCombobox->setEditable(true);
  nicknameCombobox->insertStringList(KonversationApplication::preferences.getNicknameList());
  nicknameCombobox->installEventFilter(this);
  oldNick=nicknameCombobox->currentText();
  awayLabel=new QLabel(i18n("(away)"),commandLineBox);
  awayLabel->hide();
  statusInput=new IRCInput(commandLineBox);
  statusInput->installEventFilter(this);


  setLog(KonversationApplication::preferences.getLog());
  setLogfileName("konversation");

  connect(getTextView(),SIGNAL (gotFocus()),statusInput,SLOT (setFocus()) );

  connect(getTextView(),SIGNAL (newText(const QString&,bool)),this,SLOT (newTextInView(const QString&,bool)) );
  connect(getTextView(),SIGNAL (sendFile()),this,SLOT (sendFileMenu()) );
  connect(getTextView(),SIGNAL (autoText(const QString&)),this,SLOT (sendStatusText(const QString&)) );

  connect(statusInput,SIGNAL (submit()),this,SLOT(statusTextEntered()) );
  connect(statusInput,SIGNAL (textPasted(const QString&)),this,SLOT(textPasted(const QString&)) );
  connect(getTextView(), SIGNAL(textPasted()), statusInput, SLOT(paste()));

  connect(nicknameCombobox,SIGNAL (activated(int)),this,SLOT(nicknameComboboxChanged(int)));

  updateFonts();
}

StatusPanel::~StatusPanel()
{
}

void StatusPanel::setNickname(const QString& newNickname)
{
  nicknameCombobox->setCurrentText(newNickname);
}

void StatusPanel::childAdjustFocus()
{
  statusInput->setFocus();
}

void StatusPanel::sendStatusText(const QString& sendLine)
{
  // create a work copy
  QString output(sendLine);
  // replace aliases and wildcards
  if(server->getOutputFilter()->replaceAliases(output)) {
    output = server->parseWildcards(output, server->getNickname(), QString::null, QString::null, QString::null, QString::null);
  }

  // encoding stuff is done in Server()
  Konversation::OutputFilterResult result = server->getOutputFilter()->parse(server->getNickname(), output, QString::null);

  if(!result.output.isEmpty()) {
    appendServerMessage(result.typeString, result.output);
  }

  server->queue(result.toServer);
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

void StatusPanel::textPasted(const QString& text)
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

  if(KonversationApplication::preferences.getShowBackgroundImage()) {
    getTextView()->setViewBackground(KonversationApplication::preferences.getColor("TextViewBackground"),
                                  KonversationApplication::preferences.getBackgroundImageName());
  } else {
    getTextView()->setViewBackground(KonversationApplication::preferences.getColor("TextViewBackground"),
      QString::null);
  }

  nicknameCombobox->setFont(KonversationApplication::preferences.getTextFont());
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

bool StatusPanel::closeYourself()
#ifdef USE_MDI
{
	return true;
}
void StatusPanel::closeYourself(ChatWindow*)
#endif
{
  int result=KMessageBox::warningYesNo(
                this,
                i18n("Do you want to disconnect from '%1'?").arg(server->getServerName()),
                i18n("Disconnect From Server"),
                KStdGuiItem::yes(),
                KStdGuiItem::cancel(),
                "QuitServerTab");

  if(result==KMessageBox::Yes)
  {
    server->quitServer();
     //Why are these seperate?  why would deleting the server not quit it? FIXME
    delete server;
    server=0;
#ifdef USE_MDI
    emit chatWindowCloseRequest(this);
#else
    this->deleteLater(); //NO NO!  Deleting the server should delete this! FIXME
    return true;
#endif
  }
  return false;
}

void StatusPanel::nicknameComboboxChanged(int /*index*/)
{
  QString newNick=nicknameCombobox->currentText();
  oldNick=server->getNickname();
  nicknameCombobox->setCurrentText(oldNick);
  server->queue("NICK "+newNick);
}

void StatusPanel::changeNickname(const QString& newNickname)
{
  server->queue("NICK "+newNickname);
}

void StatusPanel::appendInputText(const QString& text)
{
  statusInput->setText(statusInput->text() + text);
}

void StatusPanel::setChannelEncoding(const QString& encoding)  // virtual
{
  KonversationApplication::preferences.setChannelEncoding(server->getServerGroup(), ":server", encoding);
}

QString StatusPanel::getChannelEncoding()  // virtual
{
  return KonversationApplication::preferences.getChannelEncoding(server->getServerGroup(), ":server");
}

QString StatusPanel::getChannelEncodingDefaultDesc()  // virtual
{
  return i18n("Identity Default ( %1 )").arg(getServer()->getIdentity()->getCodecName());
}

#include "statuspanel.moc"
