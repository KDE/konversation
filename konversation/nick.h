/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  nick.h  -  description
  begin:     Fri Jan 25 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef NICK_H
#define NICK_H

/*
  @author Dario Abatianni
*/

class KListView;
class LedListViewItem;

class Nick
{
  public:
    Nick(KListView* listView,
         const QString &nickname,
         const QString &hostmask,
         bool admin,
         bool owner,
         bool op,
         bool halfop,
         bool voice);

    ~Nick();

    bool isAdmin();
    bool isOwner();
    bool isOp();
    bool isHalfop();
    bool hasVoice();

    void setAdmin(bool state);
    void setOwner(bool state);
    void setOp(bool state);
    void setHalfop(bool state);
    void setVoice(bool state);

    bool isSelected();

    QString getNickname();
    QString getHostmask();

    void setHostmask(const QString& newMask);
    void setNickname(const QString& newName);

  protected:
    bool admin;
    bool owner;
    bool op;
    bool halfop;
    bool voice;

    QString nickname;
    QString hostmask;

    LedListViewItem* listViewItem;
};

#endif
