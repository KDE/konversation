/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

    SPDX-FileCopyrightText: 2010 Eike Hein <hein@kde.org>
*/

#include "irccontextmenus.h"
#include "application.h"
#include "nick.h"
#include "server.h"
#include "ircinput.h"

#include <QClipboard>

#include <KActionCollection>
#include <KAuthorized>
#include <KBookmarkDialog>
#include <KBookmarkManager>
#include <QFileDialog>
#include <KIO/CopyJob>
#include <KIO/ApplicationLauncherJob>
#include <kio_version.h>
#include <KIO/JobUiDelegateFactory>
#include <QMenu>
#include <KMessageBox>
#include <KStandardAction>
#include <KStringHandler>
#include <KToggleAction>

// For the Web Shortcuts context menu sub-menu.
#include <KIO/CommandLauncherJob>
#include <KUriFilter>


class IrcContextMenusPrivate
{
    public:
        IrcContextMenusPrivate();
        ~IrcContextMenusPrivate();

        IrcContextMenus instance;
};

Q_GLOBAL_STATIC(IrcContextMenusPrivate, s_ircContextMenusPrivate)

IrcContextMenusPrivate::IrcContextMenusPrivate()
{
}

IrcContextMenusPrivate::~IrcContextMenusPrivate()
{
}

IrcContextMenus::IrcContextMenus() : QObject()
{
}

void IrcContextMenus::setupUi(QWidget* parent)
{
    self()->doSetupUi(parent);
}

void IrcContextMenus::doSetupUi(QWidget* parent)
{
    m_parent = parent;
    createSharedBasicNickActions();
    createSharedNickSettingsActions();
    createSharedDccActions();

    setupQuickButtonMenu();
    setupNickMenu();
    setupTextMenu(); // creates m_textCopyAction needed by the channel menu
    setupChannelMenu();
    setupTopicHistoryMenu();

    updateQuickButtonMenu();
}

IrcContextMenus* IrcContextMenus::self()
{
    return &s_ircContextMenusPrivate->instance;
}

void IrcContextMenus::setupQuickButtonMenu()
{
    //NOTE: if we depend on m_nickMenu we get an we an cyclic initialising
    m_quickButtonMenu = new QMenu();
    m_quickButtonMenu->setTitle(i18n("Quick Buttons"));
    connect(Application::instance(), &Application::appearanceChanged, this, &IrcContextMenus::updateQuickButtonMenu);
}

bool IrcContextMenus::shouldShowQuickButtonMenu()
{
    return Preferences::self()->showQuickButtonsInContextMenu() && !m_quickButtonMenu->isEmpty();
}

void IrcContextMenus::updateQuickButtonMenu()
{
    m_quickButtonMenu->clear();

    QAction * action;
    QString pattern;

    const auto buttons = Preferences::quickButtonList();
    for (const QString& button : buttons) {
        pattern = button.section(QLatin1Char(','), 1);

        if (pattern.contains(QLatin1String("%u")))
        {
            action = new QAction(button.section(QLatin1Char(','), 0, 0), m_quickButtonMenu);
            action->setData(pattern);
            m_quickButtonMenu->addAction(action);
        }
    }
}

void IrcContextMenus::processQuickButtonAction(QAction* action, Server* server, const QString& context, const QStringList &nicks)
{
    ChatWindow* chatWindow = server->getChannelOrQueryByName(context);
    QString line = server->parseWildcards(action->data().toString(), chatWindow, nicks);

    if (line.contains(QLatin1Char('\n')))
        chatWindow->sendText(line);
    else
    {
        if (chatWindow->getInputBar())
            chatWindow->getInputBar()->setText(line, true);
    }
}

void IrcContextMenus::setupTextMenu()
{
    m_textMenu = new QMenu(m_parent);

    m_textMenu->addSeparator();

    m_linkActions << createAction(m_textMenu, LinkCopy, QIcon::fromTheme(QStringLiteral("edit-copy")), i18n("Copy Link Address"));
    // Not using KStandardAction is intentional here since the Ctrl+B
    // shortcut it would show in the menu is already used by our IRC-
    // wide bookmarking feature.
    m_linkActions << createAction(m_textMenu, LinkBookmark, QIcon::fromTheme(QStringLiteral("bookmark-new")), i18n("Add to Bookmarks"));
    m_linkActions << createAction(m_textMenu, LinkOpenWith, i18n("Open With..."));
    m_linkActions << createAction(m_textMenu, LinkSaveAs, QIcon::fromTheme(QStringLiteral("document-save")), i18n("Save Link As..."));

    m_textMenu->addSeparator();

    m_textCopyAction = KStandardAction::copy(nullptr, nullptr, this);
    m_textCopyAction->setData(TextCopy);
    m_textMenu->addAction(m_textCopyAction);
    m_textCopyAction->setEnabled(false);

    QAction* action = KStandardAction::selectAll(nullptr, nullptr, this);
    action->setData(TextSelectAll);
    m_textMenu->addAction(action);

    m_webShortcutsMenu = new QMenu(m_parent);
    m_webShortcutsMenu->menuAction()->setIcon(QIcon::fromTheme(QStringLiteral("preferences-web-browser-shortcuts")));
    m_webShortcutsMenu->menuAction()->setVisible(false);
    m_textMenu->addMenu(m_webShortcutsMenu);

    m_textActionsSeparator = m_textMenu->addSeparator();

    for (QAction* action : std::as_const(m_sharedBasicNickActions))
        m_textMenu->addAction(action);

    m_textMenu->addSeparator();

    m_textMenu->addMenu(m_quickButtonMenu);

    m_textMenu->addSeparator();

    for (QAction* action : std::as_const(m_sharedNickSettingsActions))
        m_textMenu->addAction(action);

    m_textMenu->addSeparator();

    for (QAction* action : std::as_const(m_sharedDccActions))
        m_textMenu->addAction(action);

    m_textMenu->addSeparator();
}

int IrcContextMenus::textMenu(const QPoint& pos, MenuOptions options, Server* server,
    const QString& selectedText, const QString& link, const QString& nick)
{
    QMenu* textMenu = self()->m_textMenu;

    KActionCollection* actionCollection = Application::instance()->getMainWindow()->actionCollection();

    auto* toggleMenuBarAction = qobject_cast<KToggleAction*>(actionCollection->action(QStringLiteral("options_show_menubar")));

    if (toggleMenuBarAction && !toggleMenuBarAction->isChecked())
        textMenu->insertAction(textMenu->actions().first(), toggleMenuBarAction);

    bool showLinkActions = options.testFlag(ShowLinkActions);

    for (QAction* action : std::as_const(self()->m_linkActions))
        action->setVisible(showLinkActions);

    self()->m_textCopyAction->setEnabled(!selectedText.isEmpty());

    self()->updateWebShortcutsMenu(selectedText);

    bool showNickActions = options.testFlag(ShowNickActions);

    for (QAction* action : std::as_const(self()->m_sharedBasicNickActions))
        action->setVisible(showNickActions);

    self()->m_quickButtonMenu->menuAction()->setVisible(showNickActions && self()->shouldShowQuickButtonMenu());

    if (showNickActions)
    {
        bool connected = server->isConnected();

        for (QAction* action : std::as_const(self()->m_sharedBasicNickActions))
            action->setEnabled(connected);

        updateSharedNickSettingsActions(server, QStringList { nick });

        for (QAction* action : std::as_const(self()->m_sharedDccActions))
            action->setEnabled(connected);
    }
    else
    {
        for (QAction* action : std::as_const(self()->m_sharedNickSettingsActions))
            action->setVisible(false);
    }

    for (QAction* action : std::as_const(self()->m_sharedDccActions))
        action->setVisible(showNickActions);

    if (options.testFlag(ShowFindAction))
        textMenu->insertAction(self()->m_textActionsSeparator, actionCollection->action(KStandardAction::name(KStandardAction::Find)));

    if (options.testFlag(ShowLogAction))
        textMenu->addAction(actionCollection->action(QStringLiteral("open_logfile")));

    if (options.testFlag(ShowChannelActions))
        textMenu->addAction(actionCollection->action(QStringLiteral("channel_settings")));

    QAction* action = textMenu->exec(pos);

    int actionId = extractActionId(action);

    if (showLinkActions)
        processLinkAction(actionId, link);

    if (self()->m_quickButtonMenu->actions().contains(action))
        processQuickButtonAction(action, server, nick, QStringList { nick });

    textMenu->removeAction(toggleMenuBarAction);
    textMenu->removeAction(actionCollection->action(KStandardAction::name(KStandardAction::Find)));
    textMenu->removeAction(actionCollection->action(QStringLiteral("open_logfile")));
    textMenu->removeAction(actionCollection->action(QStringLiteral("channel_settings")));

    return actionId;
}

void IrcContextMenus::updateWebShortcutsMenu(const QString& selectedText)
{
    m_webShortcutsMenu->menuAction()->setVisible(false);
    m_webShortcutsMenu->clear();

    if (selectedText.isEmpty())
        return;

    QString searchText = selectedText;
    searchText = searchText.replace(QLatin1Char('\n'), QLatin1Char(' ')).replace(QLatin1Char('\r'), QLatin1Char(' ')).simplified();

    if (searchText.isEmpty())
        return;

    KUriFilterData filterData(searchText);

    filterData.setSearchFilteringOptions(KUriFilterData::RetrievePreferredSearchProvidersOnly);

    if (KUriFilter::self()->filterSearchUri(filterData, KUriFilter::NormalTextFilter))
    {
        const QStringList searchProviders = filterData.preferredSearchProviders();

        if (!searchProviders.isEmpty())
        {
            m_webShortcutsMenu->setTitle(i18n("Search for '%1' with",  KStringHandler::rsqueeze(searchText, 21)));

            QAction * action = nullptr;

            for (const QString& searchProvider : searchProviders) {
                action = new QAction(searchProvider, m_webShortcutsMenu);
                action->setIcon(QIcon::fromTheme(filterData.iconNameForPreferredSearchProvider(searchProvider)));
                action->setData(filterData.queryForPreferredSearchProvider(searchProvider));
                connect(action, &QAction::triggered, this, &IrcContextMenus::processWebShortcutAction);
                m_webShortcutsMenu->addAction(action);
            }

            m_webShortcutsMenu->addSeparator();

            action = new QAction(i18n("Configure Web Shortcuts..."), m_webShortcutsMenu);
            action->setIcon(QIcon::fromTheme(QStringLiteral("configure")));
            connect(action, &QAction::triggered, this, &IrcContextMenus::configureWebShortcuts);
            m_webShortcutsMenu->addAction(action);

            m_webShortcutsMenu->menuAction()->setVisible(true);
        }
    }
}

void IrcContextMenus::processWebShortcutAction()
{
    auto * action = qobject_cast<QAction*>(sender());

    if (action)
    {
        KUriFilterData filterData(action->data().toString());

        if (KUriFilter::self()->filterSearchUri(filterData, KUriFilter::WebShortcutFilter))
            Application::openUrl(filterData.uri().url());
    }
}

void IrcContextMenus::configureWebShortcuts()
{
    auto job = new KIO::CommandLauncherJob(QStringLiteral("kcmshell5"), {QStringLiteral("webshortcuts")});
    job->start();
}

void IrcContextMenus::setupChannelMenu()
{
    m_channelMenu = new QMenu(m_parent);

    QAction* defaultAction = createAction(m_channelMenu, Join, QIcon::fromTheme(QStringLiteral("irc-join-channel")), i18n("&Join Channel..."));
    m_channelMenu->setDefaultAction(defaultAction);

    createAction(m_channelMenu, Topic, i18n("Get &topic"));
    createAction(m_channelMenu, Names, i18n("Get &user list"));
    
    m_channelMenu->addAction(m_textCopyAction);
}

void IrcContextMenus::channelMenu(const QPoint& pos, Server* server, const QString& channel)
{
    QMenu* channelMenu = self()->m_channelMenu;

    if (!channel.isEmpty())
        channelMenu->setTitle(KStringHandler::rsqueeze(channel, 15));

    bool connected = server->isConnected();

    const auto channelMenuActions = channelMenu->actions();
    for (QAction* action : channelMenuActions)
        action->setEnabled(connected);

    QAction* action = channelMenu->exec(pos);

    self()->m_textCopyAction->setEnabled(false);
    
    switch (extractActionId(action))
    {
        case Join:
            commandToServer(server, QLatin1String("join ") + channel);
            break;
        case Topic:
            server->requestTopic(channel);
            break;
        case Names:
            commandToServer(server, QLatin1String("names ") + channel);
            break;
        case TextCopy:
            qApp->clipboard()->setText(channel, QClipboard::Clipboard);
            break;
        default:
            break;
    }
}

void IrcContextMenus::setupNickMenu()
{
    m_nickMenu = new QMenu(m_parent);

    QAction* defaultAction = createAction(m_nickMenu, OpenQuery, i18n("Open Query"));
    m_nickMenu->setDefaultAction(defaultAction);

    m_nickMenu->addSeparator();

    for (QAction* action : std::as_const(m_sharedBasicNickActions))
        m_nickMenu->addAction(action);

    m_nickMenu->addSeparator();

    m_modesMenu = new QMenu(m_parent);
    m_nickMenu->addMenu(m_modesMenu);
    m_modesMenu->setTitle(i18n("Modes"));
    createAction(m_modesMenu, GiveOp, QIcon::fromTheme(QStringLiteral("irc-operator")), i18n("Give Op"));
    createAction(m_modesMenu, TakeOp, QIcon::fromTheme(QStringLiteral("irc-remove-operator")), i18n("Take Op"));
    createAction(m_modesMenu, GiveHalfOp, i18n("Give HalfOp"));
    createAction(m_modesMenu, TakeHalfOp, i18n("Take HalfOp"));
    createAction(m_modesMenu, GiveVoice, QIcon::fromTheme(QStringLiteral("irc-voice")), i18n("Give Voice"));
    createAction(m_modesMenu, TakeVoice, QIcon::fromTheme(QStringLiteral("irc-unvoice")), i18n("Take Voice"));

    m_kickBanMenu = new QMenu(m_parent);
    m_nickMenu->addMenu(m_kickBanMenu);
    m_kickBanMenu->setTitle(i18n("Kick / Ban"));
    createAction(m_kickBanMenu, Kick, i18n("Kick"));
    createAction(m_kickBanMenu, KickBan, i18n("Kickban"));
    createAction(m_kickBanMenu, BanNick, i18n("Ban Nickname"));
    m_kickBanMenu->addSeparator();
    createAction(m_kickBanMenu, BanHost, i18n("Ban *!*@*.host"));
    createAction(m_kickBanMenu, BanDomain, i18n("Ban *!*@domain"));
    createAction(m_kickBanMenu, BanUserHost, i18n("Ban *!user@*.host"));
    createAction(m_kickBanMenu, BanUserDomain, i18n("Ban *!user@domain"));
    m_kickBanMenu->addSeparator();
    createAction(m_kickBanMenu, KickBanHost, i18n("Kickban *!*@*.host"));
    createAction(m_kickBanMenu, KickBanDomain, i18n("Kickban *!*@domain"));
    createAction(m_kickBanMenu, KickBanUserHost, i18n("Kickban *!user@*.host"));
    createAction(m_kickBanMenu, KickBanUserDomain, i18n("Kickban *!user@domain"));

    m_nickMenu->addMenu(m_quickButtonMenu);

    m_nickMenu->addSeparator();

    for (QAction* action : std::as_const(m_sharedNickSettingsActions))
        m_nickMenu->addAction(action);

    m_nickMenu->addSeparator();

    for (QAction* action : std::as_const(m_sharedDccActions))
        m_nickMenu->addAction(action);
}

void IrcContextMenus::createSharedBasicNickActions()
{
    m_sharedBasicNickActions << createAction(Whois, i18n("&Whois"));
    m_sharedBasicNickActions << createAction(Version, i18n("&Version"));
    m_sharedBasicNickActions << createAction(Ping, i18n("&Ping"));
}

void IrcContextMenus::createSharedNickSettingsActions()
{
    m_ignoreAction = createAction(IgnoreNick, i18n("Ignore"));
    m_sharedNickSettingsActions << m_ignoreAction;
    m_unignoreAction = createAction(UnignoreNick, i18n("Unignore"));
    m_sharedNickSettingsActions << m_unignoreAction;

    m_addNotifyAction = createAction(AddNotify, QIcon::fromTheme(QStringLiteral("list-add-user")), i18n("Add to Watched Nicks"));
    m_sharedNickSettingsActions << m_addNotifyAction;
    m_removeNotifyAction = createAction(RemoveNotify, QIcon::fromTheme(QStringLiteral("list-remove-user")), i18n("Remove From Watched Nicks"));
    m_sharedNickSettingsActions << m_removeNotifyAction;
}

void IrcContextMenus::createSharedDccActions()
{
    if (KAuthorized::authorizeAction(QStringLiteral("allow_downloading")))
        m_sharedDccActions << createAction(DccSend, QIcon::fromTheme(QStringLiteral("arrow-right-double")), i18n("Send &File..."));

    m_sharedDccActions << createAction(StartDccChat, i18n("Open DCC Chat"));
    m_sharedDccActions << createAction(StartDccWhiteboard, i18n("Open DCC Whiteboard"));
}

void IrcContextMenus::nickMenu(const QPoint& pos, MenuOptions options, Server* server,
    const QStringList& nicks, const QString& context)
{
    QMenu* nickMenu = self()->m_nickMenu;

    if (options.testFlag(ShowTitle) && nicks.count() == 1)
        nickMenu->setTitle(KStringHandler::rsqueeze(nicks.first(), 15));

    const auto nickMenuActions = nickMenu->actions();
    for (QAction* action : nickMenuActions)
        action->setVisible(true);

    self()->m_modesMenu->menuAction()->setVisible(options.testFlag(ShowChannelActions));
    self()->m_kickBanMenu->menuAction()->setVisible(options.testFlag(ShowChannelActions));
    self()->m_quickButtonMenu->menuAction()->setVisible(self()->shouldShowQuickButtonMenu());

    bool connected = server->isConnected();

    for (QAction* action : std::as_const(self()->m_sharedBasicNickActions))
        action->setEnabled(connected);

    for (QAction* action : std::as_const(self()->m_sharedDccActions))
        action->setEnabled(connected);

    self()->m_modesMenu->menuAction()->setEnabled(connected);
    self()->m_kickBanMenu->menuAction()->setEnabled(connected);

    updateSharedNickSettingsActions(server, nicks);

    QAction* action = nickMenu->exec(pos);

    if (self()->m_quickButtonMenu->actions().contains(action))
        processQuickButtonAction(action, server, context, nicks);
    else
        processNickAction(extractActionId(action), server, nicks, context);
}

void IrcContextMenus::processNickAction(int actionId, Server* server, const QStringList& nicks,
    const QString& context)
{
    QString channel;

    if (server->getChannelByName(context))
        channel = context;

    QString pattern;
    QChar mode;

    switch (actionId)
    {
        case OpenQuery:
            commandToServer(server, QStringLiteral("query %1"), nicks);
            break;
        case Whois:
            commandToServer(server, QStringLiteral("whois %1 %1"), nicks);
            break;
        case Version:
            commandToServer(server, QStringLiteral("ctcp %1 VERSION"), nicks);
            break;
        case Ping:
            commandToServer(server, QStringLiteral("ctcp %1 PING"), nicks);
            break;
        case GiveOp:
            if (channel.isEmpty()) break;
            pattern = QStringLiteral("MODE %c +%m %l");
            mode = QLatin1Char('o');
            break;
        case TakeOp:
            if (channel.isEmpty()) break;
            pattern = QStringLiteral("MODE %c -%m %l");
            mode = QLatin1Char('o');
            break;
        case GiveHalfOp:
            if (channel.isEmpty()) break;
            pattern = QStringLiteral("MODE %c +%m %l");
            mode = QLatin1Char('h');
            break;
        case TakeHalfOp:
            if (channel.isEmpty()) break;
            pattern = QStringLiteral("MODE %c -%m %l");
            mode = QLatin1Char('h');
            break;
        case GiveVoice:
            if (channel.isEmpty()) break;
            pattern = QStringLiteral("MODE %c +%m %l");
            mode = QLatin1Char('v');
            break;
        case TakeVoice:
            if (channel.isEmpty()) break;
            pattern = QStringLiteral("MODE %c -%m %l");
            mode = QLatin1Char('v');
            break;
        case Kick:
            commandToServer(server, QStringLiteral("kick %1"), nicks, channel);
            break;
        case KickBan:
            commandToServer(server, QStringLiteral("kickban %1"), nicks, channel);
            break;
        case BanNick:
            commandToServer(server, QStringLiteral("ban %1"), nicks, channel);
            break;
        case BanHost:
            commandToServer(server, QStringLiteral("ban -HOST %1"), nicks, channel);
            break;
        case BanDomain:
            commandToServer(server, QStringLiteral("ban -DOMAIN %1"), nicks, channel);
            break;
        case BanUserHost:
            commandToServer(server, QStringLiteral("ban -USERHOST %1"), nicks, channel);
            break;
        case BanUserDomain:
            commandToServer(server, QStringLiteral("ban -USERDOMAIN %1"), nicks, channel);
            break;
        case KickBanHost:
            commandToServer(server, QStringLiteral("kickban -HOST %1"), nicks, channel);
            break;
        case KickBanDomain:
            commandToServer(server, QStringLiteral("kickban -DOMAIN %1"), nicks, channel);
            break;
        case KickBanUserHost:
            commandToServer(server, QStringLiteral("kickban -USERHOST %1"), nicks, channel);
            break;
        case KickBanUserDomain:
            commandToServer(server, QStringLiteral("kickban -USERDOMAIN %1"), nicks, channel);
            break;
        case IgnoreNick:
        {
            QString question;

            if (nicks.size() == 1)
                question = i18n("Do you want to ignore %1?", nicks.first());
            else
                question = i18n("Do you want to ignore the selected users?");

            if (KMessageBox::warningContinueCancel(
                Application::instance()->getMainWindow(),
                question,
                i18n("Ignore"),
                KGuiItem(i18n("Ignore")),
                KStandardGuiItem::cancel(),
                QStringLiteral("IgnoreNick")
                ) ==
                KMessageBox::Continue)
            {
                commandToServer(server, QLatin1String("ignore -ALL ") + nicks.join(QLatin1Char(' ')));
            }

            break;
        }
        case UnignoreNick:
        {
            QString question;
            QStringList selectedIgnoredNicks;

            for (const QString& nick : nicks) {
                if (Preferences::isIgnored(nick))
                    selectedIgnoredNicks << nick;
            }

            if (selectedIgnoredNicks.count() == 1)
                question = i18n("Do you want to stop ignoring %1?", selectedIgnoredNicks.first());
            else
                question = i18n("Do you want to stop ignoring the selected users?");

            if (KMessageBox::warningContinueCancel(
                Application::instance()->getMainWindow(),
                question,
                i18n("Unignore"),
                KGuiItem(i18n("Unignore")),
                KStandardGuiItem::cancel(),
                QStringLiteral("UnignoreNick")) ==
                KMessageBox::Continue)
            {
                commandToServer(server, QLatin1String("unignore ") + selectedIgnoredNicks.join(QLatin1Char(' ')));
            }

            break;
        }
        case AddNotify:
        {
            if (!server->getServerGroup()) break;

            for(const QString& nick : nicks)
                Preferences::addNotify(server->getServerGroup()->id(), nick);

            break;
        }
        case RemoveNotify:
        {
            if (!server->getServerGroup()) break;

            for(const QString& nick : nicks)
                Preferences::removeNotify(server->getServerGroup()->id(), nick);

            break;
        }
        case DccSend:
            commandToServer(server, QStringLiteral("dcc send %1"), nicks);
            break;
        case StartDccChat:
            commandToServer(server, QStringLiteral("dcc chat %1"), nicks);
            break;
        case StartDccWhiteboard:
            commandToServer(server, QStringLiteral("dcc whiteboard %1"), nicks);
            break;
        default:
            break;
    }

    if (!pattern.isEmpty())
    {
        pattern.replace(QLatin1String("%c"), channel);

        QString command;
        QStringList partialList;
        int modesCount = server->getModesCount();

        for (int index = 0; index < nicks.count(); index += modesCount)
        {
            command = pattern;
            partialList = nicks.mid(index, modesCount);
            command.replace(QLatin1String("%l"), partialList.join(QLatin1Char(' ')));
            const QString repeatedMode = QString(partialList.count(), mode);

            command.replace(QLatin1String("%m"), repeatedMode);

            server->queue(command);
        }
    }
}

void IrcContextMenus::updateSharedNickSettingsActions(Server* server, const QStringList& nicks)
{
    int ignoreCounter = 0;
    int unignoreCounter = 0;
    int addNotifyCounter = 0;
    int removeNotifyCounter = 0;

    int serverGroupId = -1;

    if (server->getServerGroup())
        serverGroupId = server->getServerGroup()->id();

    for (const QString& nick : nicks) {
        if (Preferences::isIgnored(nick))
            ++unignoreCounter;
        else
            ++ignoreCounter;

        if (serverGroupId != -1)
        {
            if (Preferences::isNotify(serverGroupId, nick))
                ++removeNotifyCounter;
            else
                ++addNotifyCounter;
        }
    }

    self()->m_ignoreAction->setVisible(ignoreCounter);
    self()->m_unignoreAction->setVisible(unignoreCounter);

    self()->m_addNotifyAction->setVisible(serverGroupId == -1 || addNotifyCounter);
    self()->m_addNotifyAction->setEnabled(serverGroupId != -1);
    self()->m_removeNotifyAction->setVisible(removeNotifyCounter);
}

void IrcContextMenus::processLinkAction(int  actionId, const QString& link)
{
   if (actionId == -1 || link.isEmpty())
       return;

    switch (actionId)
    {
        case LinkCopy:
        {
            QClipboard* clipboard = qApp->clipboard();
            clipboard->setText(link, QClipboard::Selection);
            clipboard->setText(link, QClipboard::Clipboard);

            break;
        }
        case LinkBookmark:
        {
            auto manager = std::make_unique<KBookmarkManager>(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/konqueror/bookmarks.xml"));
            auto dialog = std::make_unique<KBookmarkDialog>(manager.get(), Application::instance()->getMainWindow());

            dialog->addBookmark(link, QUrl(link), QString());

            break;
        }
        case LinkOpenWith:
        {
            // ApplicationLauncherJob ctor without args will invoke the open-with dialog
            auto *job = new KIO::ApplicationLauncherJob();
            job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, Application::instance()->getMainWindow()));
            job->setUrls({ QUrl(link) });
            job->start();

            break;
        }
        case LinkSaveAs:
        {
            QUrl srcUrl(link);

            QUrl saveUrl = QFileDialog::getSaveFileUrl(Application::instance()->getMainWindow(), i18n("Save link as"), QUrl::fromLocalFile(srcUrl.fileName()));

            if (saveUrl.isEmpty() || !saveUrl.isValid())
                break;

            KIO::copy(srcUrl, saveUrl);
            break;
        }
        default:
            break;
    }
}

void IrcContextMenus::setupTopicHistoryMenu()
{
    m_topicHistoryMenu = new QMenu(m_parent);

    m_topicHistoryMenu->addAction(m_textCopyAction);

    m_queryTopicAuthorAction = createAction(m_topicHistoryMenu, OpenQuery, i18nc("Opens a query (private message) window with the author of the topic", "Query author"));
}

void IrcContextMenus::topicHistoryMenu(const QPoint& pos, Server* server, const QString& text, const QString& author)
{
    QMenu* topicHistoryMenu = self()->m_topicHistoryMenu;

    self()->m_textCopyAction->setEnabled(true);
    self()->m_queryTopicAuthorAction->setEnabled(!author.isEmpty());

    QAction* action = topicHistoryMenu->exec(pos);

    switch (extractActionId(action))
    {
        case TextCopy:
            qApp->clipboard()->setText(text, QClipboard::Clipboard);
            break;
        case OpenQuery:
            commandToServer(server, QStringLiteral("query %1").arg(author));
            break;
        default:
            break;
    }
}

QAction* IrcContextMenus::createAction(ActionId id, const QString& text)
{
    auto* action = new QAction(text, m_parent);

    action->setData(id);

    return action;
}

QAction* IrcContextMenus::createAction(ActionId id, const QIcon& icon)
{
    auto* action = new QAction(m_parent);

    action->setData(id);
    action->setIcon(icon);

    return action;
}

QAction* IrcContextMenus::createAction(ActionId id, const QIcon& icon, const QString& text)
{
    auto* action = new QAction(icon, text, m_parent);

    action->setData(id);

    return action;
}

QAction* IrcContextMenus::createAction(QMenu* menu, ActionId id, const QString& text)
{
    QAction* action = createAction(id, text);

    menu->addAction(action);

    return action;
}

QAction* IrcContextMenus::createAction(QMenu* menu, ActionId id, const QIcon& icon, const QString& text)
{
    QAction* action = createAction(id, text);

    action->setIcon(icon);

    menu->addAction(action);

    return action;
}

int IrcContextMenus::extractActionId(QAction* action)
{
    if (action)
    {
        bool ok = false;

        int actionId = action->data().toInt(&ok);

        if (ok)
            return actionId;
    }

    return -1;
}

void IrcContextMenus::commandToServer(Server* server, const QString& command, const QString& destination)
{
    const Konversation::OutputFilterResult result = server->getOutputFilter()->parse(QString(), Preferences::self()->commandChar() + command, destination);

    server->queue(result.toServer);

    if (!result.output.isEmpty())
        server->appendMessageToFrontmost(result.typeString, result.output);
    else if (!result.outputList.isEmpty())
    {
        for (const QString& output : result.outputList)
            server->appendMessageToFrontmost(result.typeString, output);
    }
}

void IrcContextMenus::commandToServer(Server* server, const QString& command,
    const QStringList& arguments, const QString& destination)
{
    for (const QString& argument : arguments)
        commandToServer(server, command.arg(argument), destination);
}

#include "moc_irccontextmenus.cpp"
