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

#include <klistview.h>

#ifndef NICK_H
#define NICK_H

/*
  @author Dario Abatianni
*/

class Nick
{
  public:
    Nick(KListView* listView,QString& nickname,QString& hostmask,bool op,bool voice);
    ~Nick();

    bool isOp() { return op; };
    bool hasVoice() { return voice; };
    void setOp(bool setop);
    void setVoice(bool setvoice);
    bool isSelected() { return listViewItem->isSelected(); } ;

    QString& getNickname() { return nickname; };
    QString& getHostmask() { return hostmask; };
    void setHostmask(QString& newMask) { hostmask=newMask; };
    void setNickname(QString& newName);

  protected:
    bool op;
    bool voice;

    QString nickname;
    QString hostmask;

    KListViewItem* listViewItem;
};

#endif
