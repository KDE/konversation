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

#ifndef DCCCHAT_H
#define DCCCHAT_H

#include <chatwindow.h>

/*
 @author Dario Abatianni
*/

class IRCInput;
class KLineEdit;
class Server;

namespace KNetwork
{
  class KServerSocket;
  class KStreamSocket;
}

class DccChat : public ChatWindow
{
  Q_OBJECT

  public:
#ifdef USE_MDI
    DccChat(QString caption,Server* newServer,const QString& myNickname,const QString& nickname,const QStringList& parameters,bool listen);
#else
    DccChat(QWidget* parent,Server* newServer,const QString& myNickname,const QString& nickname,const QStringList& parameters,bool listen);
#endif
    ~DccChat();

    virtual QString getTextInLine();
    virtual void closeYourself();
    virtual bool frontView();
    virtual bool searchView();

    int getPort();
    
    virtual void setChannelEncoding(const QString& encoding);
    virtual QString getChannelEncoding();
    virtual QString getChannelEncodingDefaultDesc();

  public slots:
    void appendInputText(const QString& s);

  signals:
    void newText(QWidget* query,const QString& highlightColor,bool important);

  protected slots:
    void lookupFinished();
    void dccChatConnectionSuccess();
    void dccChatBroken(int error);
    void readData();
    void dccChatTextEntered();
    void sendDccChatText(const QString& sendLine);
    void textPasted(QString text);
    void newTextInView(const QString& highlightColor, bool important);
    void heardPartner();

  protected:
    void listenForPartner();
    void connectToPartner();
#ifdef USE_MDI
    virtual void closeYourself(ChatWindow*);
#endif

    /** Called from ChatWindow adjustFocus */
    virtual void childAdjustFocus();
    
    QString myNick;
    QString nick;
    QString host;

    QString ip;
    int port;

    KLineEdit* sourceLine;
    IRCInput* dccChatInput;
    KNetwork::KStreamSocket* dccSocket;
    KNetwork::KServerSocket* listenSocket;
    
    QString m_encoding;
};

#endif
