/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

    SPDX-FileCopyrightText: 2010 Eike Hein <hein@kde.org>
*/

#ifndef IRCCONTEXTMENUS_H
#define IRCCONTEXTMENUS_H

#include <QIcon>
#include <QObject>
#include <QPoint>




class Server;

class QAction;

class QMenu;


class IrcContextMenus : public QObject
{
    Q_OBJECT

    public:
        enum ActionId
        {
            TextCopy, TextSelectAll,
            LinkOpenWith, LinkCopy, LinkBookmark, LinkSaveAs,
            Join, Topic, Names,
            OpenQuery,
            Whois, Version, Ping,
            GiveOp, TakeOp, GiveHalfOp, TakeHalfOp, GiveVoice, TakeVoice,
            Kick, KickBan, BanNick, BanHost, BanDomain, BanUserHost, BanUserDomain,
            KickBanHost, KickBanDomain, KickBanUserHost, KickBanUserDomain,
            IgnoreNick, UnignoreNick,
            AddNotify, RemoveNotify,
            DccSend, StartDccChat, StartDccWhiteboard,
        };

        enum MenuOption
        {
            NoOptions            = 0x00000000,
            ShowTitle            = 0x00000001,
            ShowLinkActions      = 0x00000002,
            ShowFindAction       = 0x00000004,
            ShowNickActions      = 0x00000008,
            ShowChannelActions   = 0x00000010,
            ShowLogAction        = 0x00000020

        };
        Q_DECLARE_FLAGS(MenuOptions, MenuOption)

        ~IrcContextMenus() override = default;

        static void setupUi(QWidget* parent);

        static int textMenu(const QPoint& pos, MenuOptions options, Server* server,
            const QString& selectedText, const QString& link, const QString& nick = QString());

        static void channelMenu(const QPoint& pos, Server* server, const QString& channel);

        static void nickMenu(const QPoint& pos, MenuOptions options, Server* server,
            const QStringList& nicks, const QString& context);
        static void processNickAction(int actionId, Server* server, const QStringList& nicks,
            const QString& context);

        static void processLinkAction(int actionId, const QString& link);

        static void topicHistoryMenu(const QPoint& pos, Server* server, const QString& text,
            const QString& author);


    private Q_SLOTS:
        void processWebShortcutAction();
        void configureWebShortcuts();
        void updateQuickButtonMenu();


    private:
        static IrcContextMenus* self();
        void doSetupUi(QWidget* parent);

        explicit IrcContextMenus();
        friend class IrcContextMenusPrivate;

        void setupQuickButtonMenu();
        bool shouldShowQuickButtonMenu();
        static void processQuickButtonAction(QAction* action, Server* server, const QString& context,
            const QStringList &nicks = QStringList());

        void setupTextMenu();
        void updateWebShortcutsMenu(const QString& selectedText);

        void setupChannelMenu();

        void setupNickMenu();
        void createSharedBasicNickActions();
        void createSharedNickSettingsActions();
        static void updateSharedNickSettingsActions(Server* server, const QStringList& nicks);
        void createSharedDccActions();

        void setupTopicHistoryMenu();

        inline QAction* createAction(ActionId id, const QString& text);
        inline QAction* createAction(ActionId id, const QIcon& icon);
        inline QAction* createAction(ActionId id, const QIcon& icon, const QString& text);
        inline QAction* createAction(QMenu* menu, ActionId id, const QString& text);
        inline QAction* createAction(QMenu* menu, ActionId id, const QIcon& icon,
            const QString& text);

        static int extractActionId(QAction* action);

        static void commandToServer(Server* server, const QString& command,
            const QString& destination = QString());
        static inline void commandToServer(Server* server, const QString& command,
            const QStringList& arguments, const QString& destination = QString());

    private:
        QWidget* m_parent;
        QMenu* m_quickButtonMenu;
        QMenu* m_textMenu;
        QAction* m_textCopyAction;
        QAction* m_textActionsSeparator;
        QList<QAction*> m_linkActions;
        QMenu* m_webShortcutsMenu;
        QMenu* m_channelMenu;
        QMenu* m_nickMenu;
        QList<QAction*> m_sharedBasicNickActions;
        QMenu* m_modesMenu;
        QMenu* m_kickBanMenu;
        QList<QAction*> m_sharedNickSettingsActions;
        QAction* m_ignoreAction;
        QAction* m_unignoreAction;
        QAction* m_addNotifyAction;
        QAction* m_removeNotifyAction;
        QList<QAction*> m_sharedDccActions;
        QMenu* m_topicHistoryMenu;
        QAction* m_queryTopicAuthorAction;

        Q_DISABLE_COPY(IrcContextMenus)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(IrcContextMenus::MenuOptions)

#endif
