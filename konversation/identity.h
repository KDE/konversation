/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  identity.h  -  This class holds the various user identities
  begin:     Son Feb 9 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef IDENTITY_H
#define IDENTITY_H

#include <ksharedptr.h>
#include <qstringlist.h>

class QTextCodec;

/*
 *@author Dario Abatianni
 */

class Identity : public KShared
{
  public: 
    Identity();
    ~Identity();

    void setName(const QString& name);   // the name of this identity
    QString getName() const;

    void setRealName(const QString& name);
    QString getRealName() const;
    void setIdent(const QString& ident);
    QString getIdent() const;

    void setNickname(int index,const QString& nick);
    QString getNickname(int index) const;

    void setBot(const QString& bot);
    QString getBot() const;
    void setPassword(const QString& password);
    QString getPassword() const;

    void setNicknameList(const QStringList& newList);
    QStringList getNicknameList() const;

    void setPartReason(const QString& reason);
    QString getPartReason() const;
    void setKickReason(const QString& reason);
    QString getKickReason() const;

    void setInsertRememberLineOnAway(bool state);
    bool getInsertRememberLineOnAway();
    void setShowAwayMessage(bool state);
    bool getShowAwayMessage() const;

    void setAwayMessage(const QString& message);
    QString getAwayMessage() const;
    void setReturnMessage(const QString& message);
    QString getReturnMessage() const;

    void setCodecName(const QString &newCodecName);
    QString getCodecName() const;
    QTextCodec* getCodec() const;
    
    QString getAwayNick();
    void setAwayNick(const QString& n);

  protected:
    QString name;

    QString bot;
    QString password;

    QString realName;
    QString ident;

    QStringList nicknameList;

    QString partReason;
    QString kickReason;

    bool insertRememberLineOnAway;
    bool showAwayMessages;
    QString awayMessage;
    QString returnMessage;
    QString m_codecName;
    QTextCodec* m_codec;
    
    QString awayNick;
};

typedef KSharedPtr<Identity> IdentityPtr;

#endif
