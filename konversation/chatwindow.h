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
class KonversationMainWindow;
#include "konvidebug.h"

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

    /** This should be called and set with a non-null server as soon
     *  as possibly after ChatWindow is created.
     *  Some chatWindows don't have a server - like konsolepanel.
     *  In these cases, you should call setMainWindow.
     *  @param newServer The server to set it to.
     *  @see setMainWindow(KonversationMainWindow *mainWindow)
     */
    void setServer(Server* newServer);
    /** This should be called if setServer is not called - e.g.
     *  in the case of konsolepanel.  This should be set as soon
     *  as possible after creation.
     */
    void setMainWindow(KonversationMainWindow *mainWindow);
    /** Get the server this is linked to.
     *  @return The server it is associated with, or null if none.
     */
    Server* getServer();
    void setIdentity(const Identity *newIdentity);
    void setTextView(IRCView* newView);
    IRCView* getTextView();
    void setLog(bool activate);

    QString& getName();

    void setType(WindowType newType);
    WindowType getType();

    void insertRememberLine();
    void append(const QString& nickname,const QString& message);
    void appendRaw(const QString& message, bool suppressTimestamps=false);
    void appendQuery(const QString& nickname,const QString& message, bool usenotifications = false);
    void appendAction(const QString& nickname,const QString& message, bool usenotifications = false);
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
    /** Clean up and close this tab.  Return false if you want to cancel the close. */
    virtual bool closeYourself();
    virtual bool frontView();
    virtual bool searchView();

    virtual bool notificationsEnabled() { return m_notificationsEnabled; }
    
    virtual bool eventFilter(QObject* watched, QEvent* e);
    
    QString logFileName() { return logfile.name(); }
    
    virtual void setChannelEncoding(const QString& /* encoding */) {}
    virtual QString getChannelEncoding() { return QString::null; }
    virtual QString getChannelEncodingDefaultDesc() { return QString::null; }
    bool isChannelEncodingSupported() const;
    
    /** Force updateInfo(info) to be emitted.
     *  Useful for when this tab has just gained focus
     */
    virtual void emitUpdateInfo();
    
  signals:
    void nameChanged(ChatWindow* view,const QString& newName);
    void online(ChatWindow* myself,bool state);
    void chatWindowCloseRequest(ChatWindow* view); // USE_MDI
    void setNotification(ChatWindow* view,const QIconSet& newIconSet,const QString& color);   // USE_MDI - used for blinking tabs
    /** Emit this signal when you want to change the status bar text for this tab.
     *  It is ignored if this tab isn't focused.
     */
    void updateInfo(const QString &info);
    
  public slots:
    void logText(const QString& text);
    void serverOnline(bool state);

    /** 
     * This is called when a chat window gains focus.
     * It enables and disables the appropriate menu items,
     * then calls childAdjustFocus.
     * You can call this manually to focus this tab.
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
    
    /** Some children may handle the name themselves, and not want this public.
     *  Increase the visibility in the subclass if you want outsiders to call this.
     *  The name is the string that is shown in the tab.
     *  @param newName The name to show in the tab
     */
    virtual void setName(const QString& newName);
    
    /** Called from adjustFocus */
    virtual void childAdjustFocus() = 0;
    
    void setLogfileName(const QString& name);
    void setChannelEncodingSupported(bool enabled);
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

    bool log;
    bool firstLog;
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

    QIconSet iconOn;
    QIconSet iconOff;
    QTimer blinkTimer;
#endif

    IRCView* textView;
    /** A pointer to the server this chatwindow is part of.
     *  Not always non-null - e.g. for konsolepanel
     */
    Server* server;
        Identity identity;
    QFile logfile;
    WindowType type;

    bool m_notificationsEnabled;
    
    bool m_channelEncodingSupported;
  private:
    /** This should always be non-null.  Used to enable/disable mainWindow
     *  kactions.  Private because this shouldn't be modified directly
     *  by anything by setMainWindow.
     *  @see setMainWindow
     */
    KonversationMainWindow *m_mainWindow;
};

#endif
