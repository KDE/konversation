/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  preferences.h  -  description
  begin:     Tue Feb 5 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <qobject.h>
#include <qlist.h>
#include <qsize.h>
#include <qstringlist.h>

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include "serverentry.h"
#include "ignore.h"

/*
  @author Dario Abatianni
*/

class PrefsDialog;
class Preferences : public QObject
{
  Q_OBJECT

  public:
    Preferences();
    ~Preferences();

    int serverWindowToolBarPos;
    int serverWindowToolBarStatus;
    int serverWindowToolBarIconText;
    int serverWindowToolBarIconSize;

    int addServer(const QString& serverString);
    void removeServer(int id);
    QString getServerByIndex(unsigned int);
    QString getServerById(int);
    ServerEntry* getServerEntryById(int id);
    int getServerIdByIndex(unsigned int);
    void clearServerList();
    void changeServerProperty(int id,int property,const QString& value);
    void updateServer(int id,const QString& newDefinition);
    void setLog(bool state) { log=state; };
    bool getLog() { return log; };
    void setBlinkingTabs(bool blink) { blinkingTabs=blink; };
    bool getBlinkingTabs() { return blinkingTabs; };
    /* Hilight list functions */
    QStringList& getHilightList();
    void setHilightList(QStringList& newList);
    QSize& getHilightSize() { return hilightSize; };
    void setHilightSize(QSize newSize) { hilightSize=newSize; };
    void addHilight(QString& newHilight);
    QString getHilightColor();
    void setHilightColor(const QString& color);
    /* Button list functions */
    QSize& getButtonsSize() { return buttonsSize; };
    void setButtonsSize(QSize newSize) { buttonsSize=newSize; };
    QStringList getButtonList() { return buttonList; };
    void setButtonList(QStringList newList);
    /* Ignore list functions */
    void clearIgnoreList() { ignoreList.clear(); };
    void addIgnore(QString newIgnore);
    QSize& getIgnoreSize() { return ignoreSize; };
    void setIgnoreSize(QSize newSize) { ignoreSize=newSize; };
    QPtrList<Ignore> getIgnoreList() { return ignoreList; };
    void setIgnoreList(QPtrList<Ignore> newList);
    /* Part reason */
    QString getPartReason() { return partReason; };
    void setPartReason(QString newReason) { partReason=newReason; };
    /* Kick reason */
    QString getKickReason() { return kickReason; };
    void setKickReason(QString newReason) { kickReason=newReason; };
    /* Nickname List Functions */
    QString getNickname(int index) { return nicknameList[index]; };
    void setNickname(int index,QString newName) { nicknameList[index]=newName; };
    QStringList getNicknameList() { return nicknameList; };
    void setNicknameList(QStringList newList) { nicknameList=newList; };
    QSize& getNicknameSize() { return nicknameSize; };
    void setNicknameSize(QSize newSize) { nicknameSize=newSize; };

    QSize serverWindowSize;
    QString ident;
    QString realname;
    QString logPath;

  signals:
    void requestServerConnection(int number);
    void requestSaveOptions();

  public slots:
    void openPrefsDialog();

  protected:
    bool log;
    bool blinkingTabs; /* Do we want the LEDs on the tabs to blink? */

    QList<ServerEntry> serverList;

    QStringList hilightList;
    QString hilightColor;
    QSize hilightSize;

    QStringList buttonList;
    QSize buttonsSize;

    QPtrList<Ignore> ignoreList;
    QSize ignoreSize;

    QStringList nicknameList;
    QSize nicknameSize;

    QString partReason;
    QString kickReason;

    PrefsDialog* prefsDialog;

  protected slots:
    void connectToServer(int number);
    void saveOptions();
    void closePrefsDialog();
    void clearPrefsDialog();
};

#endif
