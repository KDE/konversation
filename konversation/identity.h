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

  $Id$
*/

#ifndef IDENTITY_H
#define IDENTITY_H

#include <qstringlist.h>

/*
 *@author Dario Abatianni
 */

class Identity
{
  public: 
    Identity();
    ~Identity();

    void setName(const QString& name);   // the name of this identity
    const QString& getName();

    void setRealName(const QString& name);
    const QString getRealName();
    void setIdent(const QString& ident);
    const QString& getIdent();

    void setNickname(int index,const QString& nick);
    const QString& getNickname(int index);

    void setNicknameList(const QStringList& newList);
    const QStringList& getNicknameList();

    void setPartReason(const QString& reason);
    const QString& getPartReason();
    void setKickReason(const QString& reason);
    const QString& getKickReason();

    void setShowAwayMessage(bool state);
    bool getShowAwayMessage();

    void setAwayMessage(const QString& message);
    const QString& getAwayMessage();
    void setReturnMessage(const QString& message);
    const QString& getReturnMessage();

  protected:
    QString name;

    QString realName;
    QString ident;

    QStringList nicknameList;

    QString partReason;
    QString kickReason;

    bool showAwayMessages;
    QString awayMessage;
    QString returnMessage;
};

#endif
