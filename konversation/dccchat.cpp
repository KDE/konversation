/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
    dccchat.cpp  -  The class that controls dcc chats
    begin:     Sun Nov 16 2003
    copyright: (C) 2002 by Dario Abatianni
    email:     eisfuchs@tigress.com
*/

#include <stdlib.h>

#include <qvbox.h>
#include <qhostaddress.h>

#include <klineedit.h>
#include <kextsock.h>
#include <konversationapplication.h>
#include <klocale.h>
#include <kdebug.h>

#include "ircview.h"
#include "ircinput.h"
#include "dccchat.h"

DccChat::DccChat(QWidget* parent,const QString& myNickname,const QString& nickname,const QStringList& parameters,bool listen) :
      ChatWindow(parent)
{
  kdDebug() << "DccChat::DccChat()" << endl;

  dccSocket=0;

  setType(ChatWindow::DccChat);
  setName("-"+nickname+"-");

  myNick=myNickname;
  nick=nickname;
  host=parameters[1];
  port=parameters[2].toInt();

  // this is the main box
  QVBox* mainBox=new QVBox(this);
  mainBox->setSpacing(spacing());

  sourceLine=new KLineEdit(mainBox);
  sourceLine->setReadOnly(true);

  setTextView(new IRCView(mainBox,NULL));

  dccChatInput=new IRCInput(mainBox);

  // connect the signals and slots
  connect(dccChatInput,SIGNAL (returnPressed()),this,SLOT (dccChatTextEntered()) );
  connect(dccChatInput,SIGNAL (textPasted(QString)),this,SLOT (textPasted(QString)) );

  connect(getTextView(),SIGNAL (gotFocus()),this,SLOT (adjustFocus()) );
  connect(getTextView(),SIGNAL (newText(const QString&)),this,SLOT (newTextInView(const QString&)) );

  if(listen);
  else
  {
    connectToPartner();
  }
}

DccChat::~DccChat()
{
  kdDebug() << "DccChat::~DccChat()" << endl;
  if(dccSocket) delete dccSocket;
}

void DccChat::newTextInView(const QString& highlightColor)
{
  emit newText(this,highlightColor);
}

void DccChat::connectToPartner()
{
  QHostAddress ip;

  ip.setAddress(host.toUInt());
  host=ip.toString();

  getTextView()->append(i18n("Info"),i18n("Establishing DCC Chat connection to %1 (%2:%3)").arg(nick).arg(host).arg(port));

  dccSocket=new KExtendedSocket(host,port,KExtendedSocket::inetSocket);

  dccSocket->enableRead(false);
  dccSocket->enableWrite(false);
  dccSocket->setTimeout(5);

  connect(dccSocket,SIGNAL (lookupFinished(int))  ,this,SLOT (lookupFinished(int)) );
  connect(dccSocket,SIGNAL (connectionSuccess())  ,this,SLOT (dccChatConnectionSuccess()) );
  connect(dccSocket,SIGNAL (connectionFailed(int)),this,SLOT (dccChatBroken(int)) );

  connect(dccSocket,SIGNAL (readyRead()),this,SLOT (readData()) );

  dccSocket->startAsyncConnect();

  getTextView()->append(i18n("Info"),i18n("Looking for host %1 ...").arg(host));
}

void DccChat::lookupFinished(int numOfResults)
{
  if(numOfResults)
    getTextView()->append(i18n("Info"),i18n("Host found, connecting ..."));
  else
    getTextView()->append(i18n("Error"),i18n("Host %1 not found. Connection aborted.").arg(host));
}

void DccChat::dccChatConnectionSuccess()
{
  getTextView()->append(i18n("Info"),i18n("Connection established!"));

  dccSocket->enableRead(true);
}

void DccChat::dccChatBroken(int error)
{
  getTextView()->append(i18n("Error"),i18n("Connection broken, error code %1!").arg(error));
}

void DccChat::readData()
{
  int actual=0;
  char* buffer=0;
  QString line;

  do
  {
    buffer=static_cast<char *>(malloc(1025));
    if(buffer)
    {
      actual=dccSocket->readBlock(buffer,1024);
      if(actual>0)
      {
        buffer[actual]=0;
        line.append(buffer);
      }
      else kdDebug() << "Read 0 bytes from DCC Chat." << endl;
      free(buffer);
    }
    else kdDebug() << "DCC Chat input buffer broken." << endl;

  } while(buffer && actual==1024);

  if(!line.isEmpty())
  {
    QStringList lines=QStringList::split('\n',line);

    for(unsigned int index=0;index<lines.count();index++)
    {
      if(line.startsWith("\x01"))
      {
        // cut out the CTCP command
        QString ctcp=line.mid(1,line.find(1,1)-1);

        QString ctcpCommand=ctcp.section(" ",0,0);
        QString ctcpArgument=ctcp.section(" ",1);

        if(ctcpCommand.lower()=="action")
          getTextView()->appendAction(nick,ctcpArgument);
        else
          getTextView()->append(i18n("CTCP"),i18n("Received unknown CTCP-%1 request from %2").arg(ctcp).arg(nick));
      }
      else getTextView()->append(nick,lines[index]);
    }
  }
}

void DccChat::dccChatTextEntered()
{
  QString line=dccChatInput->text();
  if(line.lower()=="/clear") textView->clear();
  else
  {
    if(line.length()) sendDccChatText(line);
  }
  dccChatInput->clear();
}

void DccChat::sendDccChatText(const QString& sendLine)
{
  // create a work copy
  QString output(sendLine);
  QString cc=KonversationApplication::preferences.getCommandChar();

  if(!output.isEmpty())
  {
    QStringList lines=QStringList::split('\n',output);
    // wrap socket into a stream
    QTextStream stream(dccSocket);
    // init stream props
    stream.setEncoding(QTextStream::Locale);

/*
      QString codecName=identity->getCodec();
      // convert encoded data to IRC ascii only when we don't have the same codec locally
      if(QString(QTextCodec::codecForLocale()->name()).lower()!=codecName.lower())
      {
        stream.setCodec(QTextCodec::codecForName(codecName.ascii()));
      }
*/
    for(unsigned int index=0;index<lines.count();index++)
    {
      QString line(lines[index]);

  // replace aliases and wildcards
  //  if(filter.replaceAliases(line)) line=server->parseWildcards(line,nick,getName(),QString::null,QString::null,QString::null);

  //  line=filter.parse(nick,line,getName());

      // convert /me actions
      if(line.lower().startsWith(cc+"me "))
      {
        getTextView()->appendAction(myNick,line.section(" ",1));
        line=QString("\x01%1 %2\x01").arg("ACTION").arg(line.section(" ",1));
      }
      else getTextView()->append(myNick,line);

      stream << line << endl;
    } // endfor

    // detach stream
    stream.unsetDevice();
  }
}

void DccChat::textPasted(QString /* text */ )
{
}

void DccChat::adjustFocus()
{
  dccChatInput->setFocus();
}

bool DccChat::frontView()        { return true; }
bool DccChat::searchView()       { return true; }

QString DccChat::getTextInLine() { return dccChatInput->text(); }

void DccChat::appendInputText(const QString& s)
{
  dccChatInput->setText(dccChatInput->text() + s);
}

void DccChat::closeYourself()
{
  delete this;
}

#include "dccchat.moc"
