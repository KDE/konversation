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
class KExtendedSocket;

class DccChat : public ChatWindow
{
  Q_OBJECT

  public:
    DccChat(QWidget* parent,const QString& myNickname,const QString& nickname,const QStringList& parameters,bool listen);
    ~DccChat();

    virtual QString getTextInLine();
    virtual void closeYourself();
    virtual bool frontView();
    virtual bool searchView();

    int getPort();

  public slots:
    void appendInputText(const QString& s);
    virtual void adjustFocus();

  signals:
    void newText(QWidget* query,const QString& highlightColor);

  protected slots:
    void lookupFinished(int numberOfResults);
    void dccChatConnectionSuccess();
    void dccChatBroken(int error);
    void readData();
    void dccChatTextEntered();
    void textPasted(QString text);
    void newTextInView(const QString& highlightColor);
    void heardPartner();

  protected:
    void listenForPartner();
    void connectToPartner();
    void sendDccChatText(const QString& sendLine);

    QString myNick;
    QString nick;
    QString host;

    QString ip;
    int port;

    KLineEdit* sourceLine;
    IRCInput* dccChatInput;
    KExtendedSocket* dccSocket;
    KExtendedSocket* listenSocket;
};

#endif
