/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  chatwindow.h  -  Base class for all chat panels
  begin:     Fri Feb 1 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <qfile.h>

#ifdef USE_MDI
#include <kmdichildview.h>
#include <qtimer.h>
#else
#include <qvbox.h>
#endif

#include "identity.h"

#ifdef USE_MDI
#include "images.h"
#endif

/*
  @author Dario Abatianni
*/

class QFile;

class IRCView;
class Server;

#ifdef USE_MDI
#define BASE_CLASS KMdiChildView
class QVBoxLayout;
#else
#define BASE_CLASS QVBox
class KMdiChildView;
class QIconSet;
#endif

class ChatWindow : public BASE_CLASS
{
  Q_OBJECT

  public:
#ifdef USE_MDI
    ChatWindow(QString caption);
#else
    ChatWindow(QWidget* parent);
#endif
    ~ChatWindow();

    enum WindowType
    {
      Status=0,
      Channel,
      Query,
      DccChat,
      DccPanel,
      RawLog,
      Notice,
      SNotice,
      ChannelList,
      Konsole,
      UrlCatcher,
      NicksOnline,
      LogFileReader
    };

    void setServer(Server* newServer);
    Server* getServer();
    void setIdentity(const Identity *newIdentity);
    void setTextView(IRCView* newView);
    IRCView* getTextView();
    void setLog(bool activate);

    void setName(const QString& newName);
    QString& getName();

    void setType(WindowType newType);
    WindowType getType();

    void insertRememberLine();
    void append(const QString& nickname,const QString& message);
    void appendRaw(const QString& message, bool suppressTimestamps=false);
    void appendQuery(const QString& nickname,const QString& message);
    void appendAction(const QString& nickname,const QString& message);
    void appendServerMessage(const QString& type,const QString& message);
    void appendCommandMessage(const QString& command, const QString& message, bool important = true,
      bool parseURL = true, bool self = false);
    void appendBacklogMessage(const QString& firstColumn,const QString& message);
    
#ifdef USE_MDI
    void setIconSet(QIconSet newIconSet);
    const QIconSet& getIconSet();
    void setLabelColor(const QString& color);
    void setLedColor(int color);
    void setOn(bool on, bool important=true);
#else
    QWidget* parentWidget;
#endif

    virtual QString getTextInLine();
    virtual void closeYourself();
    virtual bool frontView();
    virtual bool searchView();

    virtual bool notificationsEnabled() { return m_notificationsEnabled; }
    
    virtual bool eventFilter(QObject* watched, QEvent* e);
    
    QString logFileName() { return logfile.name(); }
    
    virtual void setChannelEncoding(const QString& /* encoding */) {}
    virtual QString getChannelEncoding() { return QString::null; }
    bool getChannelEncodingEnabled() const;
    
  signals:
    void nameChanged(ChatWindow* view,const QString& newName);
    void online(ChatWindow* myself,bool state);
    void chatWindowCloseRequest(ChatWindow* view); // USE_MDI
    void setNotification(ChatWindow* view,const QIconSet& newIconSet,const QString& color);   // USE_MDI - used for blinking tabs

  public slots:
    void logText(const QString& text);
    void serverOnline(bool state);

    /** 
     * This is called when a chat window gains focus.
     * It enables and disables the appropriate menu items,
     * then calls childAdjustFocus
     */
    void adjustFocus();
    virtual void appendInputText(const QString&);
    virtual void indicateAway(bool away);

    virtual void setNotificationsEnabled(bool enable) { m_notificationsEnabled = enable; }

    void closeRequest(KMdiChildView* view); // USE_MDI

  protected slots:  // USE_MDI
    void serverQuit(const QString& reason);  // USE_MDI
    void blinkTimeout();  // USE_MDI

  protected:
    bool log;
    bool firstLog;
    
    /** Called from adjustFocus */
    virtual void childAdjustFocus() = 0;
    
    void setLogfileName(const QString& name);
    void setChannelEncodingEnabled(bool enabled);
    void cdIntoLogPath();

    /** child classes have to override this and return true if they want the
     *  "insert character" item on the menu to be enabled.
     */
    virtual bool isInsertCharacterSupported() { return false; }

    /** child classes have to override this and return true if they want the
     *  "irc color" item on the menu to be enabled.
     */
    virtual bool areIRCColorsSupported() {return false; }
    
    int spacing();
    int margin();

    QString name;
    QString logName;

    QFont font;
#ifdef USE_MDI
    enum StateType
    {
      Off=0,
      Slow,
      Fast
    };

    QVBoxLayout* mainLayout;
    virtual void closeYourself(ChatWindow* view);

    QString labelColor;
    int ledColor;      // color of the LED
    StateType state;// if and how fast the LED should blink
    bool blinkOn;   // true, if blinking LED is on at this moment

    Images images;
    QIconSet iconOn;
    QIconSet iconOff;
    QTimer blinkTimer;
#endif

    IRCView* textView;
    Server* server;
    Identity identity;
    QFile logfile;
    WindowType type;

    bool m_notificationsEnabled;
    
    bool m_channelEncodingEnabled;
};

#endif
