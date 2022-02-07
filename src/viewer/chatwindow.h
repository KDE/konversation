/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2006-2008 Eike Hein <hein@kde.org>
*/

#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include "identity.h"
#include "common.h"

#include <QFile>
#include <QWidget>
#include <QVBoxLayout>


class IRCView;
class IRCInput;
class Server;

class ChatWindow : public QWidget
{
    Q_OBJECT

    public:
        explicit ChatWindow(QWidget* parent);
        ~ChatWindow() override;

        enum WindowType
        {
            Status=0,
            Channel,
            Query,
            DccChat,
            DccTransferPanel,
            RawLog,
            Notice,
            SNotice,
            ChannelList,
            Konsole,
            UrlCatcher,
            NicksOnline,
            LogFileReader
        };

        /** Clean up and close this tab.  Return false if you want to cancel the close. */
        virtual bool closeYourself(bool askForConfirmation = true);

        virtual void cycle();

        /** This should be called and set with a non-null server as soon
         *  as possibly after ChatWindow is created.
         *  @param newServer The server to set it to.
         */
        virtual void setServer(Server* newServer);
        /** This should be called if setServer is not called - e.g.
         *  in the case of konsolepanel.  This should be set as soon
         *  as possible after creation.
         */

        /** Get the server this is linked to.
         *  @return The server it is associated with, or null if none.
         */
        Server* getServer() const;
        void setTextView(IRCView* newView);
        IRCView* getTextView() const;
        void setInputBar(IRCInput* newInputBar) { m_inputBar = newInputBar; }
        IRCInput* getInputBar() const { return m_inputBar; }
        virtual bool log() const;

        QString getName() const;
        QString getTitle() const;
        QString getURI(bool passNetwork = true);

        void setType(WindowType newType);
        WindowType getType() const;
        virtual bool isTopLevelView() const;

        virtual void sendText(const QString& /*text*/) {}

        virtual void append(const QString& nickname,const QString& message, const QHash<QString, QString> &messageTags, const QString& label = QString());
        virtual void appendRaw(const QString& message, bool self = false);
        virtual void appendLog(const QString& message);
        virtual void appendQuery(const QString& nickname,const QString& message, const QHash<QString, QString> &messageTags = QHash<QString, QString>(), bool inChannel = false);
        virtual void appendAction(const QString& nickname,const QString& message, const QHash<QString, QString> &messageTags = QHash<QString, QString>());
        virtual void appendServerMessage(const QString& type,const QString& message, const QHash<QString, QString> &messageTags = QHash<QString, QString>(), bool parseURL = true);
        virtual void appendCommandMessage(const QString& command, const QString& message, const QHash<QString, QString> &messageTags = QHash<QString, QString>(),
            bool parseURL = true, bool self = false);
        virtual void appendBacklogMessage(const QString& firstColumn,const QString& message);

        void clear();

        virtual QString getTextInLine() const;
        /** Reimplement this to return true in all classes that /can/ become front view.
         */
        virtual bool canBeFrontView() const;

        /** Reimplement this to return true in all classes that you can search in - i.e. use "Edit->Find Text" in.
         */
        virtual bool searchView() const;

        bool notificationsEnabled() const { return m_notificationsEnabled; }

        bool eventFilter(QObject* watched, QEvent* e) override;

        QString logFileName() const { return logfile.fileName(); }

        virtual void setChannelEncoding(const QString& /* encoding */) {}
        virtual QString getChannelEncoding() const { return QString(); }
        virtual QString getChannelEncodingDefaultDesc() const { return QString(); }
        bool isChannelEncodingSupported() const;

        /** Force updateInfo(info) to be emitted.
         *  Useful for when this tab has just gained focus
         */
        virtual void emitUpdateInfo();

        /** child classes have to override this and return true if they want the
         *  "insert character" item on the menu to be enabled.
         */
        virtual bool isInsertSupported() const { return m_inputBar != nullptr; }

        /** child classes have to override this and return true if they want the
         *  "irc color" item on the menu to be enabled.
         */
        virtual bool areIRCColorsSupported() const {return false; }

        Konversation::TabNotifyType currentTabNotification() const { return m_currentTabNotify; }
        int unseenEventsCount() const;
        QColor highlightColor();
        void msgHelper(const QString& recipient, const QString& message);

        void setMargin(int margin) { layout()->setContentsMargins(margin, margin, margin, margin); }
        void setSpacing(int spacing) { layout()->setSpacing(spacing); }
        void activateView();

    Q_SIGNALS:
        void nameChanged(ChatWindow* view, const QString& newName);
        //void online(ChatWindow* myself, bool state);
        /** Emit this signal when you want to change the status bar text for this tab.
         *  It is ignored if this tab isn't focused.
         */
        void updateInfo(const QString &info);
        void updateTabNotification(ChatWindow* chatWin, const Konversation::TabNotifyType& type);
        void unseenEventsCountChanged(ChatWindow* chatWin, int unseenEventsCount);

        void setStatusBarTempText(const QString&);
        void clearStatusBarTempText();

        void closing(ChatWindow* myself);
        void showView(ChatWindow* myself);
        void windowActivationRequested();

    public Q_SLOTS:
        virtual void updateAppearance();

        void logText(const QString& text);

        /**
         * This is called when a chat window gains focus.
         * It enables and disables the appropriate menu items,
         * then calls childAdjustFocus.
         * You can call this manually to focus this tab.
         */
        void adjustFocus();

        virtual void appendInputText(const QString& text, bool fromCursor);
        virtual void indicateAway(bool away);


        virtual void setNotificationsEnabled(bool enable) { m_notificationsEnabled = enable; }
        void activateTabNotification(Konversation::TabNotifyType type);
        void resetTabNotification();
        void resetUnseenEventsCount();

    protected Q_SLOTS:
        ///Used to disable functions when not connected
        virtual void serverOnline(bool online);

    protected:
        void childEvent(QChildEvent* event) override;

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

        int spacing();
        int margin();

    protected:
        bool firstLog;
        QString name;
        QString logName;

        QFont font;

        int m_unseenNickEventCount = 0;
        int m_unseenHighlightEventCount = 0;
        int m_unseenPrivateEventCount = 0;
        int m_unseenNormalEventCount = 0;
        int m_unseenSystemEventCount = 0;
        int m_unseenControlEventCount = 0;

        IRCView* textView;
        /** A pointer to the server this chatwindow is part of.
         *  Not always non-null - e.g. for konsolepanel
         */

        IRCInput* m_inputBar;

        Server* m_server;
        QFile logfile;
        WindowType type;

        bool m_isTopLevelView;

        bool m_notificationsEnabled;

        bool m_channelEncodingSupported;

        Konversation::TabNotifyType m_currentTabNotify;

        bool m_recreationScheduled;

        Q_DISABLE_COPY(ChatWindow)
};

#endif
