/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
    channel.cpp  -  The class that controls a channel
    begin:     Wed Jan 23 2002
    copyright: (C) 2002 by Dario Abatianni
    email:     eisfuchs@tigress.com

    $Id$
*/

#include <qlabel.h>
#include <qvbox.h>
#include <qhbox.h>
#include <qgrid.h>
#include <qsizepolicy.h>
#include <qheader.h>
#include <qregexp.h>
#include <qtooltip.h>
#include <qsplitter.h>
#include <qcheckbox.h>
#include <qtimer.h>

#include <klineedit.h>
#include <klineeditdlg.h>
#include <kpassdlg.h>
#include <klocale.h>
#include <kdebug.h>

#include "konversationapplication.h"
#include "channel.h"
#include "server.h"
#include "nick.h"
#include "nicklistview.h"
#include "nickchangedialog.h"
#include "quickbutton.h"
#include "modebutton.h"
#include "topiccombobox.h"
#include "ircinput.h"
#include "ircview.h"

#if QT_VERSION < 0x030100
#include "main.h"
#endif

Channel::Channel(QWidget* parent) : ChatWindow(parent)
{
  // init variables
  nicks=0;
  ops=0;
  completionPosition=0;
  nickChangeDialog=0;
  topic=QString::null;

  quickButtonsChanged=false;
  quickButtonsState=false;

  splitterChanged=true;
  modeButtonsChanged=false;
  modeButtonsState=false;

  // flag for first seen topic
  topicAuthorUnknown=true;

  setType(ChatWindow::Channel);

  // Build some size policies for the widgets
  QSizePolicy hfixed=QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Preferred);
  QSizePolicy hmodest=QSizePolicy(QSizePolicy::Preferred,QSizePolicy::Expanding);
  QSizePolicy vmodest=QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
  QSizePolicy vfixed=QSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
  QSizePolicy modest=QSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred);
  QSizePolicy greedy=QSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);

  // (this) The main Box, holding the channel view/topic and the input line
  splitter=new QSplitter(this);
  setStretchFactor(splitter,10);
  splitter->setOpaqueResize(true);

  // The grid for the topic line and Channel View
  QVBox* topicViewNicksGrid=new QVBox(splitter);
  topicViewNicksGrid->setSpacing(spacing());

  // The box holding the Topic label/line, and the channel modes
  QHBox* topicBox=new QHBox(topicViewNicksGrid);
  topicBox->setSpacing(spacing());

  QLabel* topicLabel=new QLabel(i18n("Topic:"),topicBox);
  topicLine=new TopicComboBox(topicBox);
  topicLine->setEditable(true);
  topicLine->setAutoCompletion(false);
  topicLine->setInsertionPolicy(QComboBox::NoInsertion);

  // The box holding the channel modes
  modeBox=new QHBox(topicBox);
  modeBox->setSizePolicy(hfixed);
  modeT=new ModeButton("T",modeBox,0);
  modeN=new ModeButton("N",modeBox,1);
  modeS=new ModeButton("S",modeBox,2);
  modeI=new ModeButton("I",modeBox,3);
  modeP=new ModeButton("P",modeBox,4);
  modeM=new ModeButton("M",modeBox,5);
  modeK=new ModeButton("K",modeBox,6);
  modeL=new ModeButton("L",modeBox,7);

  // Tooltips for the ModeButtons
  QToolTip::add(modeT, i18n("Topic can be changed by channel operator only."));
  QToolTip::add(modeN, i18n("No messages to channel from clients on the outside."));
  QToolTip::add(modeS, i18n("Secret channel."));
  QToolTip::add(modeI, i18n("Invite only channel."));
  QToolTip::add(modeP, i18n("Private channel."));
  QToolTip::add(modeM, i18n("Moderated channel."));
  QToolTip::add(modeK, i18n("Protect channel with a password."));
  QToolTip::add(modeL, i18n("Set user limit to channel."));

  connect(modeT,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));
  connect(modeN,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));
  connect(modeS,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));
  connect(modeI,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));
  connect(modeP,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));
  connect(modeM,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));
  connect(modeK,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));
  connect(modeL,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));

  limit=new KLineEdit(modeBox);
  connect(limit,SIGNAL (returnPressed()),this,SLOT (channelLimitChanged()) );

  showModeButtons(KonversationApplication::preferences.getShowModeButtons());

  setTextView(new IRCView(topicViewNicksGrid,NULL));  // Server will be set later in setServer()
  // The box that holds the Nick List and the quick action buttons
  QVBox* nickListButtons=new QVBox(splitter);
  nickListButtons->setSpacing(spacing());

  nicksOps=new QLabel(i18n("Nicks"),nickListButtons);
  nicksOps->setAlignment(AlignVCenter | AlignHCenter);

  nicknameListView=new NickListView(nickListButtons);
  nicknameListView->setSelectionModeExt(KListView::Extended);
  nicknameListView->setAllColumnsShowFocus(true);
  nicknameListView->setSorting(1,true);
  nicknameListView->addColumn(QString::null);
  nicknameListView->addColumn(QString::null);
  nicknameListView->addColumn(QString::null);
  nicknameListView->header()->hide();
//  nicknameListView->setResizeMode(KListView::LastColumn);
  nicknameListView->setColumnWidthMode(1,KListView::Maximum);
  nicknameListView->setColumnWidthMode(2,KListView::Maximum);

  // the grid that holds the quick action buttons
  buttonsGrid=new QGrid(2,nickListButtons);
  // set hide() or show() on grid
  showQuickButtons(KonversationApplication::preferences.getShowQuickButtons());

  for(int index=0;index<8;index++)
  {
    // generate empty buttons first, text will be added by updateQuickButtons() later
    QuickButton* newQuickButton=new QuickButton(QString::null,QString::null,buttonsGrid);
    buttonList.append(newQuickButton);

    connect(newQuickButton,SIGNAL (clicked(const QString &)),this,SLOT (quickButtonClicked(const QString &)) );
  }

  updateQuickButtons(KonversationApplication::preferences.getButtonList());

  // The box holding the Nickname button, Channel input and Log Checkbox
  QHBox* commandLineBox=new QHBox(this);
  commandLineBox->setSpacing(spacing());

  nicknameButton=new QPushButton(i18n("Nickname"),commandLineBox);
  channelInput=new IRCInput(commandLineBox);
  logCheckBox=new QCheckBox(i18n("Log"),commandLineBox);
  logCheckBox->setChecked(KonversationApplication::preferences.getLog());

  // Set the widgets size policies
  topicBox->setSizePolicy(vmodest);
  topicLabel->setSizePolicy(hfixed);  // This should prevent the widget from growing too wide
  topicLine->setSizePolicy(vfixed);

  limit->setMaximumSize(40,100);
  limit->setSizePolicy(hfixed);

  modeT->setMaximumSize(20,100);
  modeT->setSizePolicy(hfixed);
  modeN->setMaximumSize(20,100);
  modeN->setSizePolicy(hfixed);
  modeS->setMaximumSize(20,100);
  modeS->setSizePolicy(hfixed);
  modeI->setMaximumSize(20,100);
  modeI->setSizePolicy(hfixed);
  modeP->setMaximumSize(20,100);
  modeP->setSizePolicy(hfixed);
  modeM->setMaximumSize(20,100);
  modeM->setSizePolicy(hfixed);
  modeK->setMaximumSize(20,100);
  modeK->setSizePolicy(hfixed);
  modeL->setMaximumSize(20,100);
  modeL->setSizePolicy(hfixed);

  getTextView()->setSizePolicy(greedy);
  nicksOps->setSizePolicy(modest);
  nicknameListView->setSizePolicy(hmodest);

  connect(channelInput,SIGNAL (returnPressed()),this,SLOT (channelTextEntered()) );
  connect(channelInput,SIGNAL (nickCompletion()),this,SLOT (completeNick()) );
  connect(channelInput,SIGNAL (endCompletion()),this,SLOT (endCompleteNick()) );
  connect(channelInput,SIGNAL (textPasted(QString)),this,SLOT (textPasted(QString)) );

  connect(channelInput,SIGNAL (pageUp()),getTextView(),SLOT (pageUp()) );
  connect(channelInput,SIGNAL (pageDown()),getTextView(),SLOT (pageDown()) );

  connect(textView,SIGNAL (newText(const QString&)),this,SLOT (newTextInView(const QString&)) );
  connect(textView,SIGNAL (gotFocus()),this,SLOT (adjustFocus()) );
  connect(textView,SIGNAL (sendFile()),this,SLOT (sendFileMenu()) );

  connect(nicknameListView,SIGNAL (popupCommand(int)),this,SLOT (popupCommand(int)) );
  connect(nicknameListView,SIGNAL (doubleClicked(QListViewItem*)),this,SLOT (doubleClickCommand(QListViewItem*)) );
  connect(nicknameButton,SIGNAL (clicked()),this,SLOT (openNickChangeDialog()) );

  connect(topicLine,SIGNAL (topicChanged(const QString&)),this,SLOT (requestNewTopic(const QString&)) );

  nicknameList.setAutoDelete(true);     // delete items when they are removed

  updateFonts();
  setLog(KonversationApplication::preferences.getLog());

  connect(&userhostTimer,SIGNAL (timeout()),this,SLOT (autoUserhost()));
  connect(&KonversationApplication::preferences,SIGNAL (autoUserhostChanged(bool)),this,SLOT (autoUserhostChanged(bool)));

  // every few seconds try to get more userhosts
  autoUserhostChanged(KonversationApplication::preferences.getAutoUserhost());
  userhostTimer.start(10000);
}

Channel::~Channel()
{
  kdDebug() << "Channel::~Channel()" << endl;

  KonversationApplication::preferences.setChannelSplitter(splitter->sizes());

  // Purge nickname list
  Nick* nick=nicknameList.first();
  while(nick)
  {
    // Remove the first element of the list
    nicknameList.removeRef(nick);
    // Again, get the first element in the list
    nick=nicknameList.first();
  }
  // Unlink this channel from channel list
  server->removeChannel(this);
}

void Channel::requestNewTopic(const QString& newTopic)
{
  topicLine->setCurrentText(topic);

  if(newTopic!=topic) sendChannelText(KonversationApplication::preferences.getCommandChar()+"TOPIC "+newTopic);

  channelInput->setFocus();
}

void Channel::textPasted(QString text)
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
      sendChannelText(line);
    }
  }
}

// Will be connected to NickListView::popupCommand(int)
void Channel::popupCommand(int id)
{
  QString pattern;
  QString cc=KonversationApplication::preferences.getCommandChar();

  bool raw=false;

  switch(id)
  {
    case NickListView::GiveOp:
      pattern="MODE %c +o %u";
      raw=true;
      break;
    case NickListView::TakeOp:
      pattern="MODE %c -o %u";
      raw=true;
      break;
    case NickListView::GiveVoice:
      pattern="MODE %c +v %u";
      raw=true;
      break;
    case NickListView::TakeVoice:
      pattern="MODE %c -v %u";
      raw=true;
      break;
    case NickListView::Version:
      pattern="PRIVMSG %u :\x01VERSION\x01";
      raw=true;
      break;
    case NickListView::Whois:
      pattern="WHOIS %u";
      raw=true;
      break;
    case NickListView::Ping:
      {
#if QT_VERSION < 0x030100
        unsigned int time_t=toTime_t(QDateTime::currentDateTime());
#else
        unsigned int time_t=QDateTime::currentDateTime().toTime_t();
#endif
        pattern=QString(KonversationApplication::preferences.getCommandChar()+"CTCP %u PING %1").arg(time_t);
      }
      break;
    case NickListView::Kick:
      pattern=cc+"KICK %u";
      break;
    case NickListView::KickBan:
      pattern=cc+"BAN %u\n"+
              cc+"KICK %u";
      break;
    case NickListView::BanNick:
      pattern=cc+"BAN %u";
      break;
    case NickListView::BanHost:
      pattern=cc+"BAN -HOST %u";
      break;
    case NickListView::BanDomain:
      pattern=cc+"BAN -DOMAIN %u";
      break;
    case NickListView::BanUserHost:
      pattern=cc+"BAN -USERHOST %u";
      break;
    case NickListView::BanUserDomain:
      pattern=cc+"BAN -USERDOMAIN %u";
      break;
    case NickListView::KickBanHost:
      pattern=cc+"BAN -HOST %u\n"+
              cc+"KICK %u";
      break;
    case NickListView::KickBanDomain:
      pattern=cc+"BAN -DOMAIN %u\n"+
              cc+"KICK %u";
      break;
    case NickListView::KickBanUserHost:
      pattern=cc+"BAN -USERHOST %u\n"+
              cc+"KICK %u";
      break;
    case NickListView::KickBanUserDomain:
      pattern=cc+"BAN -USERDOMAIN %u\n"+
              cc+"KICK %u";
      break;
    case NickListView::Query:
      pattern=cc+"QUERY %u";
      break;
    case NickListView::DccSend:
      pattern=cc+"DCC SEND %u";
      break;
  } // switch

  if(!pattern.isEmpty())
  {
    pattern.replace(QRegExp("%c"),getName());

    QStringList nickList=getSelectedNicksList();

    QString command;
    for(QStringList::Iterator nickIterator=nickList.begin();nickIterator!=nickList.end();++nickIterator)
    {
      QStringList patternList=QStringList::split('\n',pattern);

      for(unsigned int index=0;index<patternList.count();index++)
      {
        command=patternList[index];
        command.replace(QRegExp("%u"),*nickIterator);

        if(raw)
          server->queue(command);
        else
          sendChannelText(command);
      }
    }
  }
}

// Will be connected to NickListView::doubleClicked()
void Channel::doubleClickCommand(QListViewItem* item)
{
  if(item)
  {
    // TODO: put the quick button code in another function to make reusal more legitimate
    quickButtonClicked(KonversationApplication::preferences.getChannelDoubleClickAction());
  }
}

void Channel::completeNick()
{
  int pos=channelInput->cursorPosition();
  int oldPos=channelInput->getOldCursorPosition();

  QString line=channelInput->text();
  QString newLine;
  // Check if completion position is out of range
  if(completionPosition>=nicknameList.count()) completionPosition=0;
  // Check, which completion mode is active
  char mode=channelInput->getCompletionMode();
  // Are we in query mode?
  if(mode=='q')
  {
    // Remove /msg <nick> part from string
    line=line.mid(pos);
    // Set cursor to beginning to restart query completion
    pos=0;
  }
  // Or maybe in channel mode?
  else if(mode=='c')
  {
    line.remove(oldPos,pos-oldPos);
    pos=oldPos;
  }
  // If the cursor is at beginning of line, insert /msq <query>
  if(pos==0)
  {
    // Find next query in list
    QString queryName=server->getNextQueryName();
    // Did we find any queries?
    if(!queryName.isEmpty())
    {
      // Prepend /msg name
      newLine="/msg "+queryName+" ";
      // New cursor position is behind nickname
      pos=newLine.length();
      // Add rest of the line
      newLine+=line;
      // Set query completion mode
      channelInput->setCompletionMode('q');
    }
    // No queries at all, so ignore this TAB
    else newLine=line;
  }
  else
  {
    // set channel nicks completion mode
    channelInput->setCompletionMode('c');
    // remember old cursor position in input field
    channelInput->setOldCursorPosition(pos);
    // remember old cursor position locally
    oldPos=pos;
    // remember old nick completion position
    unsigned int oldCompletionPosition=completionPosition;
    // step back to last space or start of line
    while(pos && line[pos-1]!=' ') pos--;
    // copy search pattern (lowercase)
    QString pattern=line.mid(pos,oldPos-pos).lower();
    // copy line to newLine-buffer
    newLine=line;
    // did we find any pattern?
    if(!pattern.isEmpty())
    {
      QString foundNick(QString::null);
      // try to find matching nickname in list of names
      do
      {
        QString lookNick=nicknameList.at(completionPosition)->getNickname();
        if(lookNick.lower().startsWith(pattern)) foundNick=lookNick;
        // increment search position
        completionPosition++;
        // wrap around
        if(completionPosition==nicknameList.count()) completionPosition=0;
        // the search ends when we either find a suitable nick or we end up at the
        // first search position
      } while(completionPosition!=oldCompletionPosition && foundNick.isEmpty());
      // did we find a suitable nick?
      if(!foundNick.isEmpty())
      {
        QString addStart(KonversationApplication::preferences.getNickCompleteSuffixStart());
        QString addMiddle(KonversationApplication::preferences.getNickCompleteSuffixMiddle());
        // remove pattern from line
        newLine.remove(pos,pattern.length());
        // did we find the nick in the middle of the line?
        if(pos)
        {
          newLine.insert(pos,foundNick+addMiddle);
          pos=pos+foundNick.length()+addMiddle.length();
        }
        // no, it was at the beginning
        else
        {
          newLine.insert(pos,foundNick+addStart);
          pos=pos+foundNick.length()+addStart.length();
        }
      }
      // no pattern found, so restore old cursor position
      else pos=oldPos;
    }
  }
  // Set new text and cursor position
  channelInput->setText(newLine);
  channelInput->setCursorPosition(pos);
}

// make sure to step back one position when completion ends so the user starts
// with the last complete they made
void Channel::endCompleteNick()
{
  if(completionPosition) completionPosition--;
  else completionPosition=nicknameList.count()-1;
}

void Channel::setName(const QString& newName)
{
  ChatWindow::setName(newName);
  setLogfileName("konversation_"+newName.lower()+".log");
}

void Channel::setKey(const QString& newKey)
{
//  kdDebug() << "Channel Key set to " << newKey << endl;
  key=newKey;
}

QString Channel::getKey()
{
  return key;
}

void Channel::sendFileMenu()
{
  emit sendFile();
}

void Channel::channelTextEntered()
{
  QString line=channelInput->text();

  if(line.lower()=="/clear")
    textView->clear();
  else
    if(!line.isEmpty()) sendChannelText(line);

  channelInput->clear();
}

void Channel::sendChannelText(const QString& sendLine)
{
  // create a work copy
  QString output(sendLine);
  // replace aliases and wildcards
  if(filter.replaceAliases(output)) output=server->parseWildcards(output,server->getNickname(),getName(),getKey(),getSelectedNicksList(),QString::null);

  // encoding stuff is done in Server()
  output=filter.parse(server->getNickname(),output,getName());

  // Is there something we need to display for ourselves?
  if(!output.isEmpty())
  {
    if(filter.isAction()) appendAction(server->getNickname(),output);
    else if(filter.isCommand()) appendCommandMessage(filter.getType(),output);
    else if(filter.isProgram()) appendServerMessage(filter.getType(),output);
    else if(filter.isQuery()) appendQuery(filter.getType(),output);
    else append(server->getNickname(),output);
  }
  // Send anything else to the server
  server->queue(filter.getServerOutput());
}

void Channel::newTextInView(const QString& highlightColor)
{
  emit newText(this,highlightColor);
}

void Channel::setNickname(const QString& newNickname)
{
  nicknameButton->setText(newNickname);
}

QStringList Channel::getSelectedNicksList()
{
  selectedNicksList.clear();
  Nick* nick=nicknameList.first();

  while(nick)
  {
    if(nick->isSelected()) selectedNicksList.append(nick->getNickname());
    nick=nicknameList.next();
  }

  return selectedNicksList;
}

void Channel::channelLimitChanged()
{
  unsigned int lim=limit->text().toUInt();

  modeButtonClicked(7,lim>0);
}

void Channel::modeButtonClicked(int id,bool on)
{
  char mode[]={'t','n','s','i','p','m','k','l'};
  QString command("MODE %1 %2%3 %4");
  QString args;

  if(mode[id]=='k')
  {
    if(getKey().isEmpty())
    {
      QCString key;

      int result=KPasswordDialog::getPassword(key,i18n("Channel Password"));

      if(result==KPasswordDialog::Accepted && !key.isEmpty()) setKey(key);
    }
    args=getKey();
    if(!on) setKey(QString::null);
  }
  else if(mode[id]=='l')
  {
    if(limit->text().isEmpty() && on)
    {
      bool ok=false;
      // ask user how many nicks should be the limit
      args=KLineEditDlg::getText(i18n("Nick Limit"),
                                 i18n("Enter the new nick limit:"),
                                 limit->text(),                      // will be always "" but what the hell ;)
                                 &ok,
                                 this);
      // leave this function if user cancels
      if(!ok) return;
    }
    else if(on)
      args=limit->text();
  }
  // put together the mode command and send it to the server queue
  server->queue(command.arg(getName()).arg((on) ? "+" : "-").arg(mode[id]).arg(args));
}

void Channel::quickButtonClicked(const QString &buttonText)
{
  // parse wildcards (toParse,nickname,channelName,nickList,queryName,parameter)
  QString out=server->parseWildcards(buttonText,server->getNickname(),getName(),getKey(),getSelectedNicksList(),QString::null);
  // are there any newlines in the definition?
  if(out.find('\n')!=-1)
  {
    // Send all strings, one after another
    QStringList outList=QStringList::split('\n',out);
    for(unsigned int index=0;index<outList.count();index++)
    {
      sendChannelText(outList[index]);
    }
  }
  // single line without newline needs to be copied into input line
  else channelInput->setText(out);
}

void Channel::addNickname(const QString& nickname,const QString& hostmask,bool op,bool voice)
{
  Nick* nick=0;
  Nick* lookNick=nicknameList.first();
  while(lookNick && (nick==0))
  {
    if(lookNick->getNickname().lower()==nickname.lower()) nick=lookNick;
    lookNick=nicknameList.next();
  }

  if(nick==0)
  {
    Nick* nick=new Nick(nicknameListView,nickname,hostmask,op,voice);
    nicknameListView->sort();

    nicknameList.append(nick);
    adjustNicks(1);
    if(op) adjustOps(1);
  }
  else
  {
    nick->setHostmask(hostmask);
    nick->setVoice(voice);

    if(nick->isOp() && (op==false))
    {
      nick->setOp(false);
      adjustOps(-1);
    }
    if((nick->isOp()==false) && op)
    {
      nick->setOp(true);
      adjustOps(1);
    }
  }
}

void Channel::renameNick(const QString& nickname,const QString& newNick)
{
  /* Did we change our nick name? */
  if(nickname==server->getNickname())
  {
    setNickname(newNick);
    appendCommandMessage(i18n("Nick"),i18n("You are now known as %1.").arg(newNick));
  }
  /* No, must've been someone else */
  else appendCommandMessage(i18n("Nick"),i18n("%1 is now known as %2.").arg(nickname).arg(newNick));

  /* Update the nick list */
  Nick* nick=getNickByName(nickname);
  if(nick==0) kdWarning() << "Channel::renameNick(): Nickname " << nickname << " not found!" << endl;
  else nick->setNickname(newNick);

  nicknameListView->sort();
}

void Channel::joinNickname(const QString& nickname,const QString& hostmask)
{
  /* Did we join this channel ourselves? */
  if(nickname==server->getNickname())
  {
    appendCommandMessage(i18n("Join"),i18n("You have joined channel %1. (%2)").arg(getName()).arg(hostmask));
  }
  /* No, it was somebody else */
  else
  {
    appendCommandMessage(i18n("Join"),i18n("%1 has joined this channel. (%2)").arg(nickname).arg(hostmask));
    addNickname(nickname,hostmask,false,false);
  }
  nicknameListView->sort();
}

void Channel::removeNick(const QString &nickname, const QString &reason, bool quit)
{
  if(nickname==server->getNickname())
  {
    if(quit) appendCommandMessage(i18n("Quit"),i18n("You have left this server. (%1)").arg(reason));
    else appendCommandMessage(i18n("Part"),i18n("You have left channel %1. (%2)").arg(getName()).arg(reason));

    delete this;
  }
  else
  {
    if(quit) appendCommandMessage(i18n("Quit"),i18n("%1 has left this server. (%2)").arg(nickname).arg(reason));
    else appendCommandMessage(i18n("Part"),i18n("%1 has left this channel. (%2)").arg(nickname).arg(reason));

    Nick* nick=getNickByName(nickname);
    if(nick==0) kdWarning() << "Channel::removeNick(): Nickname " << nickname << " not found!" << endl;
    else
    {
      if(nick->isOp()) adjustOps(-1);
      adjustNicks(-1);

      nicknameList.removeRef(nick);
    }
  }
}

void Channel::kickNick(const QString &nickname, const QString &kicker, const QString &reason)
{
  if(nickname==server->getNickname())
  {
    if(kicker==server->getNickname())
    {
      appendCommandMessage(i18n("Kick"),i18n("You have kicked yourself from the channel. (%1)").arg(reason));
      /* This message lets the user see what he has done after the channel window went away */
      server->appendStatusMessage(i18n("Kick"),i18n("You have kicked yourself from channel %1. (%2)").arg(getName()).arg(reason));
    }
    else
    {
      appendCommandMessage(i18n("Kick"),i18n("You have been kicked from the channel by %1. (%2)").arg(kicker).arg(reason));
      /* This message lets the user see what had happened after the channel window went away */
      server->appendStatusMessage(i18n("Kick"),i18n("You have been kicked from channel %1 by %2. (%3)").arg(getName()).arg(kicker).arg(reason));
    }

    delete this;
  }
  else
  {
    if(kicker==server->getNickname())
      appendCommandMessage(i18n("Kick"),i18n("You have kicked %1 from the channel. (%2)").arg(nickname).arg(reason));
    else
      appendCommandMessage(i18n("Kick"),i18n("%1 has been kicked from the channel by %2. (%3)").arg(nickname).arg(kicker).arg(reason));

    Nick* nick=getNickByName(nickname);
    if(nick==0) kdWarning() << "Channel::kickNick(): Nickname " << nickname << " not found!" << endl;
    else
    {
      if(nick->isOp()) adjustOps(-1);
      adjustNicks(-1);

      nicknameList.removeRef(nick);
    }
  }
}

Nick* Channel::getNickByName(const QString &lookname)
{
  Nick* nick=nicknameList.first();
  while(nick)
  {
    if(nick->getNickname().lower()==lookname.lower()) return nick;
    nick=nicknameList.next();
  }
  return 0;
}

void Channel::adjustNicks(int value)
{
  nicks+=value;
  updateNicksOps();
}

void Channel::adjustOps(int value)
{
  ops+=value;
  updateNicksOps();
}

void Channel::updateNicksOps()
{
  /* %1 %2 / %3 %4 = 5 Nicks / 3 Ops */
  nicksOps->setText(i18n("%1 %2 / %3 %4").arg(nicks).arg((nicks==1) ? i18n("Nick") : i18n("Nicks")).arg(ops).arg((ops==1) ? i18n("Op") : i18n("Ops")));
}

void Channel::setTopic(const QString &newTopic)
{
  appendCommandMessage(i18n("Topic"),i18n("The channel topic is \"%1\".").arg(newTopic));
  if(topic!=newTopic)
  {
    topicHistory.prepend(i18n("<unknown> %1").arg(newTopic));
    topicLine->clear();
    topicLine->insertStringList(topicHistory);
    topic=newTopic;
    // Add a tool tip to the topic line if it gets too long
    QToolTip::remove(topicLine);

    if(newTopic.length()>80)
    {
      QString toolTip=newTopic;
      toolTip.replace(QRegExp("&",false,true),"&amp;").
                               replace(QRegExp("<",false,true),"&lt;").
                               replace(QRegExp(">",false,true),"&gt;");

      QToolTip::add(topicLine,"<qt>"+toolTip+"</qt>");
    }
  }
}

void Channel::setTopic(const QString &nickname, const QString &newTopic) // Overloaded
{
  if(nickname==server->getNickname())
    appendCommandMessage(i18n("Topic"),i18n("You set the channel topic to \"%1\".").arg(newTopic));
  else
    appendCommandMessage(i18n("Topic"),i18n("%1 sets the channel topic to \"%2\".").arg(nickname).arg(newTopic));

  topicHistory.prepend("<"+nickname+"> "+newTopic);
  topicLine->clear();
  topicLine->insertStringList(topicHistory);
  topic=newTopic;
  // Add a tool tip to the topic line if it gets too long
  QToolTip::remove(topicLine);
  if(newTopic.length()>80)
  {
    QString toolTip=newTopic;
    toolTip.replace(QRegExp("&",false,true),"&amp;").
                             replace(QRegExp("<",false,true),"&lt;").
                             replace(QRegExp(">",false,true),"&gt;");

    QToolTip::add(topicLine,"<qt>"+toolTip+"</qt>");
  }
}

void Channel::setTopicAuthor(const QString& newAuthor)
{
  if(topicAuthorUnknown)
  {
    kdDebug() << "adding unknown topic author " << newAuthor << endl;
    topicHistory[0]="<"+newAuthor+"> "+topicHistory[0].section(' ',1);
    topicLine->clear();
    topicLine->insertStringList(topicHistory);
    topicAuthorUnknown=false;
  }
}

void Channel::updateMode(const QString &sourceNick, char mode, bool plus, const QString &parameter)
{
  QString message(QString::null);
  Nick* nick;

  bool fromMe=false;
  bool toMe=false;

  if(sourceNick.lower()==server->getNickname().lower()) fromMe=true;
  if(parameter.lower()==server->getNickname().lower()) toMe=true;

  switch(mode)
  {
    case 'a': break;

    case 'o':
      if(plus)
      {
        if(fromMe)
        {
          if(toMe)
            message=i18n("You give channel operator privileges to yourself.");
          else
            message=i18n("You give channel operator privileges to %1.").arg(parameter);
        }
        else
        {
          if(toMe)
            message=i18n("%1 gives channel operator privileges to you.").arg(sourceNick);
          else
            message=i18n("%1 gives channel operator privileges to %2.").arg(sourceNick).arg(parameter);
        }
      }
      else
      {
        if(fromMe)
        {
          if(toMe)
            message=i18n("You take channel operator privileges from yourself.");
          else
            message=i18n("You take channel operator privileges from %1.").arg(parameter);
        }
        else
        {
          if(toMe)
            message=i18n("%1 takes channel operator privileges from you.").arg(sourceNick);
          else
            message=i18n("%1 takes channel operator privileges from %2.").arg(sourceNick).arg(parameter);
        }
      }
      nick=getNickByName(parameter);
      if(nick)
      {
        // Only update counter if something has actually changed
        if(plus && !nick->isOp()) adjustOps(1);
        else if(!plus && nick->isOp()) adjustOps(-1);
        nick->setOp(plus);
        updateNicksOps();
        nicknameListView->sort();
      }
    break;

    case 'O': break;

    case 'v':
      if(plus)
      {
        if(fromMe)
        {
          if(toMe) message=i18n("You give yourself the permission to talk.");
          else     message=i18n("You give %1 the permission to talk.").arg(parameter);
        }
        else
        {
          if(toMe) message=i18n("%1 gives you the permission to talk.").arg(sourceNick);
          else     message=i18n("%1 gives %2 the permission to talk.").arg(sourceNick).arg(parameter);
        }
      }
      else
      {
        if(fromMe)
        {
          if(toMe) message=i18n("You take the permission to talk from yourself.");
          else     message=i18n("You take the permission to talk from %1.").arg(parameter);
        }
        else
        {
          if(toMe) message=i18n("%1 takes the permission to talk from you.").arg(sourceNick);
          else     message=i18n("%1 takes the permission to talk from %2.").arg(sourceNick).arg(parameter);
        }
      }
      nick=getNickByName(parameter);
      if(nick)
      {
        nick->setVoice(plus);
        nicknameListView->sort();
      }
    break;

    case 'c':
      if(plus)
      {
        if(fromMe) message=i18n("You set the channel mode to 'no colors allowed'.");
        else message=i18n("%1 sets the channel mode to 'no colors allowed'.").arg(sourceNick);
      }
      else
      {
        if(fromMe) message=i18n("You set the channel mode to 'allow color codes'.");
        else message=i18n("%1 sets the channel mode to 'allow color codes'.").arg(sourceNick);
      }
      modeM->setDown(plus);
    break;

    case 'i':
      if(plus)
      {
        if(fromMe) message=i18n("You set the channel mode to 'invite only'.");
        else message=i18n("%1 sets the channel mode to 'invite only'.").arg(sourceNick);
      }
      else
      {
        if(fromMe) message=i18n("You remove the 'invite only' mode from the channel.");
        else message=i18n("%1 removes the 'invite only' mode from the channel.").arg(sourceNick);
      }
      modeI->setDown(plus);
    break;

    case 'm':
      if(plus)
      {
        if(fromMe) message=i18n("You set the channel mode to 'moderated'.");
        else message=i18n("%1 sets the channel mode to 'moderated'.").arg(sourceNick);
      }
      else
      {
        if(fromMe) message=i18n("You set the channel mode to 'unmoderated'.");
        else message=i18n("%1 sets the channel mode to 'unmoderated'.").arg(sourceNick);
      }
      modeM->setDown(plus);
    break;

    case 'n':
      if(plus)
      {
        if(fromMe) message=i18n("You set the channel mode to 'no messages from outside'.");
        else message=i18n("%1 sets the channel mode to 'no messages from outside'.").arg(sourceNick);
      }
      else
      {
        if(fromMe) message=i18n("You set the channel mode to 'allow messages from outside'.");
        else message=i18n("%1 sets the channel mode to 'allow messages from outside'.").arg(sourceNick);
      }
      modeN->setDown(plus);
    break;

    case 'q':
      if(plus)
      {
        if(fromMe) message=i18n("You set the channel mode to 'quiet'.");
        else message=i18n("%1 sets the channel mode to 'quiet'.").arg(sourceNick);
      }
      else
      {
        if(fromMe) message=i18n("You remove the 'quiet' channel mode.");
        else message=i18n("%1 removes the 'quiet' channel mode.").arg(sourceNick);
      }

    break;

    case 'p':
      if(plus)
      {
        if(fromMe) message=i18n("You set the channel mode to 'private'.");
        else message=i18n("%1 sets the channel mode to 'private'.").arg(sourceNick);
      }
      else
      {
        if(fromMe) message=i18n("You set the channel mode to 'public'.");
        else message=i18n("%1 sets the channel mode to 'public'.").arg(sourceNick);
      }
      modeP->setDown(plus);
      if(plus) modeS->setDown(false);
    break;

    case 's':
      if(plus)
      {
        if(fromMe) message=i18n("You set the channel mode to 'secret'.");
        else message=i18n("%1 sets the channel mode to 'secret'.").arg(sourceNick);
      }
      else
      {
        if(fromMe) message=i18n("You set the channel mode to 'visible'.");
        else message=i18n("%1 sets the channel mode to 'visible'.").arg(sourceNick);
      }
      modeS->setDown(plus);
      if(plus) modeP->setDown(false);
    break;

    case 'r': break;

    case 't':
      if(plus)
      {
        if(fromMe) message=i18n("You switch on 'topic protection'.");
        else message=i18n("%1 switches on 'topic protection'.").arg(sourceNick);
      }
      else
      {
        if(fromMe) message=i18n("You switch off 'topic protection'.");
        else message=i18n("%1 switches off 'topic protection'.").arg(sourceNick);
      }
      modeT->setDown(plus);
    break;

    case 'k': break;

    case 'l':
      if(plus)
      {
        if(fromMe) message=i18n("You set the channel limit to %1 nicks.").arg(parameter);
        else message=i18n("%1 sets the channel limit to %2 nicks.").arg(sourceNick).arg(parameter);
      }
      else
      {
        if(fromMe) message=i18n("You remove the channel limit.");
        else message=i18n("%1 removes the channel limit.").arg(sourceNick);
      }
      modeL->setDown(plus);
      if(plus) limit->setText(parameter);
      else limit->clear();
    break;

    case 'b':
      if(plus)
      {
        if(fromMe) message=i18n("You set a ban on %1.").arg(parameter);
        else message=i18n("%1 sets a ban on %2.").arg(sourceNick).arg(parameter);
      }
      else
      {
        if(fromMe) message=i18n("You remove the ban on %1.").arg(parameter);
        else message=i18n("%1 removes the ban on %2.").arg(sourceNick).arg(parameter);
      }
    break;

    case 'e':
      if(plus)
      {
        if(fromMe) message=i18n("You set a ban exception on %1.").arg(parameter);
        else message=i18n("%1 sets a ban exception on %2.").arg(sourceNick).arg(parameter);
      }
      else
      {
        if(fromMe) message=i18n("You remove the ban exception on %1.").arg(parameter);
        else message=i18n("%1 removes the ban exception on %2.").arg(sourceNick).arg(parameter);
      }
    break;

    case 'I':
      if(plus)
      {
        if(fromMe) message=i18n("You set invitation mask %1.").arg(parameter);
        else message=i18n("%1 sets invitation mask %2.").arg(sourceNick).arg(parameter);
      }
      else
      {
        if(fromMe) message=i18n("You remove the invitation mask %1.").arg(parameter);
        else message=i18n("%1 removes the invitation mask %2.").arg(sourceNick).arg(parameter);
      }
    break;
  }
  if(!message.isEmpty()) appendCommandMessage(i18n("Mode"),message);
  updateModeWidgets(mode,plus,parameter);
}

void Channel::updateModeWidgets(char mode, bool plus, const QString &parameter)
{
  ModeButton* widget=0;

  if(mode=='t') widget=modeT;
  else if(mode=='n') widget=modeN;
  else if(mode=='s') widget=modeS;
  else if(mode=='i') widget=modeI;
  else if(mode=='p') widget=modeP;
  else if(mode=='m') widget=modeM;
  else if(mode=='k') widget=modeK;
  else if(mode=='l')
  {
    widget=modeL;
    if(plus) limit->setText(parameter);
    else limit->clear();
  }

  if(widget) widget->setOn(plus);
}

void Channel::updateQuickButtons(QStringList newButtonList)
{
  for(int index=0;index<8;index++)
  {
    // Get the button definition
    QString buttonText=newButtonList[index];
    // Extract button label
    QString buttonLabel=buttonText.section(',',0,0);
    // Extract button definition
    buttonText=buttonText.section(',',1);

    QuickButton* quickButton=buttonList.at(index);
    quickButton->setText(buttonLabel);
    quickButton->setDefinition(buttonText);

    // Update tool tips
    QToolTip::remove(quickButton);
    QString toolTip=buttonText.replace(QRegExp("&",false,true),"&amp;").
                               replace(QRegExp("<",false,true),"&lt;").
                               replace(QRegExp(">",false,true),"&gt;");

    QToolTip::add(quickButton,toolTip);
  }
}

void Channel::showQuickButtons(bool show)
{
  // QT does not redraw the buttons properly when they are not on screen
  // while getting hidden, so we remember the "soon to be" state here.
  if(isHidden())
  {
    quickButtonsChanged=true;
    quickButtonsState=show;
  }
  else
  {
    if(show)
      buttonsGrid->show();
    else
      buttonsGrid->hide();
  }
}

void Channel::showModeButtons(bool show)
{
  // QT does not redraw the buttons properly when they are not on screen
  // while getting hidden, so we remember the "soon to be" state here.
  if(isHidden())
  {
    modeButtonsChanged=true;
    modeButtonsState=show;
  }
  else
  {
    if(show)
      modeBox->show();
    else
      modeBox->hide();
  }
}

void Channel::showEvent(QShowEvent* event)
{
  // Suppress Compiler Warning
  event->type();

  // If the show quick/mode button settings have changed, apply the changes now
  if(quickButtonsChanged)
  {
    quickButtonsChanged=false;
    showQuickButtons(quickButtonsState);
  }
  if(modeButtonsChanged)
  {
    modeButtonsChanged=false;
    showModeButtons(modeButtonsState);
  }
  if(splitterChanged)
  {
    splitterChanged=false;
    splitter->setSizes(KonversationApplication::preferences.getChannelSplitter());
  }
}

void Channel::updateFonts()
{
  kdDebug() << "Channel::updateFonts()" << endl;

  topicLine->setFont(KonversationApplication::preferences.getTextFont());
  limit->setFont(KonversationApplication::preferences.getTextFont());
  nicknameButton->setFont(KonversationApplication::preferences.getTextFont());
  channelInput->setFont(KonversationApplication::preferences.getTextFont());
  logCheckBox->setFont(KonversationApplication::preferences.getTextFont());
  getTextView()->setFont(KonversationApplication::preferences.getTextFont());
  getTextView()->setViewBackground(KonversationApplication::preferences.getColor("TextViewBackground"),
                                   KonversationApplication::preferences.getBackgroundImageName());

  nicksOps->setFont(KonversationApplication::preferences.getListFont());
  nicknameListView->setFont(KonversationApplication::preferences.getListFont());
  nicknameListView->sort();
}

void Channel::openNickChangeDialog()
{
  if(!nickChangeDialog)
  {
    nickChangeDialog=new NickChangeDialog(this,server->getNickname(),
                                          identity.getNicknameList(),
                                          KonversationApplication::preferences.getNicknameSize());
    connect(nickChangeDialog,SIGNAL (closeDialog(QSize)),this,SLOT (closeNickChangeDialog(QSize)) );
    connect(nickChangeDialog,SIGNAL (newNickname(const QString&)),this,SLOT (changeNickname(const QString&)) );
    nickChangeDialog->show();
  }
}

void Channel::changeNickname(const QString& newNickname)
{
  server->queue("NICK "+newNickname);
}

void Channel::closeNickChangeDialog(QSize newSize)
{
  KonversationApplication::preferences.setNicknameSize(newSize);
  emit prefsChanged();

  delete nickChangeDialog;
  nickChangeDialog=0;
}

QPtrList<Nick> Channel::getNickList()
{
  return nicknameList;
}

void Channel::adjustFocus()
{
  channelInput->setFocus();
}

void Channel::autoUserhost()
{
  if(KonversationApplication::preferences.getAutoUserhost())
  {
    int limit=5;

    QString nickString;
    QPtrList<Nick> nicks=getNickList();

    for(unsigned int index=0;index<nicks.count();index++)
    {
      if(nicks.at(index)->getHostmask().isEmpty())
      {
        if(limit--) nickString=nickString+nicks.at(index)->getNickname()+" ";
        else break;
      }
    }
    if(!nickString.isEmpty()) server->requestUserhost(nickString);
  }
}

void Channel::autoUserhostChanged(bool state)
{
  if(state)
  {
    // restart userhost timer
    userhostTimer.start(10000);
    // if the column was actually gone (just to be sure) ...
    if(nicknameListView->columns()==2)
    {
      // re-add the hostmask column
      nicknameListView->addColumn(QString::null);

      // re-add already known hostmasks
      QListViewItem* item=nicknameListView->itemAtIndex(0);
      while(item)
      {
        Nick* lookNick=getNickByName(item->text(1));
        if(lookNick) item->setText(2,lookNick->getHostmask());
        item=item->itemBelow();
      } // while
    }
  }
  else
  {
    userhostTimer.stop();
    if(nicknameListView->columns()==3) nicknameListView->removeColumn(2);
  }
}

QString Channel::getTextInLine() { return channelInput->text(); }

bool Channel::frontView()        { return true; }
bool Channel::searchView()       { return true; }

void Channel::appendInputText(const QString& s)
{
  channelInput->setText(channelInput->text() + s);
}

void Channel::closeYourself()
{
  server->closeChannel(getName());
}

#include "channel.moc"
