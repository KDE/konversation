/*
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License or (at your option) version 3 or any later version
  accepted by the membership of KDE e.V. (or its successor appro-
  ved by the membership of KDE e.V.), which shall act as a proxy
  defined in Section 14 of version 3 of the license.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see http://www.gnu.org/licenses/.
*/

/*
  Copyright (C) 2010 Eike Hein <hein@kde.org>
*/

#include "irccontextmenus.h"
#include "application.h"
#include "nick.h"
#include "server.h"
#include "linkaddressbook/addressbook.h"
#include "linkaddressbook/linkaddressbookui.h"

#include <QClipboard>

#include <KActionCollection>
#include <KAuthorized>
#include <KBookmarkDialog>
#include <KBookmarkManager>
#include <KFileDialog>
#include <KIO/CopyJob>
#include <KMenu>
#include <KMessageBox>
#include <KStandardAction>
#include <KStringHandler>
#include <KToggleAction>

// For the Web Shortcuts context menu sub-menu.
#if KDE_IS_VERSION(4, 5, 0)
#include <KToolInvocation>
#include <KUriFilter>
#endif


class IrcContextMenusPrivate
{
    public:
        IrcContextMenusPrivate();
        ~IrcContextMenusPrivate();

        IrcContextMenus instance;
};

K_GLOBAL_STATIC(IrcContextMenusPrivate, s_ircContextMenusPrivate);

IrcContextMenusPrivate::IrcContextMenusPrivate()
{
    qAddPostRoutine(s_ircContextMenusPrivate.destroy);
}

IrcContextMenusPrivate::~IrcContextMenusPrivate()
{
    qRemovePostRoutine(s_ircContextMenusPrivate.destroy);
}


IrcContextMenus::IrcContextMenus()
{
    createSharedBasicNickActions();
    createSharedNickSettingsActions();
    createSharedDccActions();

    setupTextMenu();
    setupChannelMenu();
    setupNickMenu();

#if !(QT_VERSION >= QT_VERSION_CHECK(4, 7, 0))
    setupLinkMenu();
#endif
}

IrcContextMenus::~IrcContextMenus()
{
    delete m_textMenu;
    delete m_channelMenu;
    delete m_nickMenu;
    delete m_addressBookMenu;

#if !(QT_VERSION >= QT_VERSION_CHECK(4, 7, 0))
    delete m_linkMenu;
#endif
}

IrcContextMenus* IrcContextMenus::self()
{
    return &s_ircContextMenusPrivate->instance;
}

void IrcContextMenus::setupTextMenu()
{
    m_textMenu = new KMenu();

    m_textMenu->addSeparator();

    m_linkActions << createAction(m_textMenu, LinkCopy, KIcon("edit-copy"), i18n("Copy Link Address"));
    // Not using KStandardAction is intentional here since the Ctrl+B
    // shortcut it would show in the menu is already used by our IRC-
    // wide bookmarking feature.
    m_linkActions << createAction(m_textMenu, LinkBookmark, KIcon("bookmark-new"), i18n("Add to Bookmarks"));
    m_linkActions << createAction(m_textMenu, LinkSaveAs, KIcon("document-save"), i18n("Save Link As..."));

    m_textMenu->addSeparator();

    m_textCopyAction = KStandardAction::copy(0, 0, this);
    m_textCopyAction->setData(TextCopy);
    m_textMenu->addAction(m_textCopyAction);
    m_textCopyAction->setEnabled(false);

    QAction* action = KStandardAction::selectAll(0, 0, this);
    action->setData(TextSelectAll);
    m_textMenu->addAction(action);

#if KDE_IS_VERSION(4, 5, 0)
    m_webShortcutsMenu = new KMenu(m_textMenu);
    m_webShortcutsMenu->menuAction()->setIcon(KIcon("preferences-web-browser-shortcuts"));
    m_webShortcutsMenu->menuAction()->setVisible(false);
    m_textMenu->addMenu(m_webShortcutsMenu);
#endif

    m_textActionsSeparator = m_textMenu->addSeparator();

    foreach(QAction* action, m_sharedBasicNickActions)
        m_textMenu->addAction(action);

    m_textMenu->addSeparator();

    foreach(QAction* action, m_sharedNickSettingsActions)
        m_textMenu->addAction(action);

    m_textMenu->addSeparator();

    foreach(QAction* action, m_sharedDccActions)
        m_textMenu->addAction(action);

    m_textMenu->addSeparator();
}

int IrcContextMenus::textMenu(const QPoint& pos, MenuOptions options, Server* server,
    const QString& selectedText, const QString& link, const QString& nick)
{
    KMenu* textMenu = self()->m_textMenu;

    KActionCollection* actionCollection = Application::instance()->getMainWindow()->actionCollection();

    KToggleAction* toggleMenuBarAction = static_cast<KToggleAction*>(actionCollection->action("options_show_menubar"));

    if (toggleMenuBarAction && !toggleMenuBarAction->isChecked())
        textMenu->insertAction(textMenu->actions().first(), toggleMenuBarAction);

    bool showLinkActions = options.testFlag(ShowLinkActions);

    foreach(QAction* action, self()->m_linkActions)
        action->setVisible(showLinkActions);

    self()->m_textCopyAction->setEnabled(!selectedText.isEmpty());

    self()->updateWebShortcutsMenu(selectedText);

    bool showNickActions = options.testFlag(ShowNickActions);

    foreach(QAction* action, self()->m_sharedBasicNickActions)
        action->setVisible(showNickActions);

    if (showNickActions)
    {
        bool connected = server->isConnected();

        foreach(QAction* action, self()->m_sharedBasicNickActions)
            action->setEnabled(connected);

        updateSharedNickSettingsActions(server, QStringList() << nick);

        foreach(QAction* action, self()->m_sharedDccActions)
            action->setEnabled(connected);
    }
    else
    {
        foreach(QAction* action, self()->m_sharedNickSettingsActions)
            action->setVisible(false);
    }

    foreach(QAction* action, self()->m_sharedDccActions)
        action->setVisible(showNickActions);

    if (options.testFlag(ShowFindAction))
        textMenu->insertAction(self()->m_textActionsSeparator, actionCollection->action(KStandardAction::name(KStandardAction::Find)));

    if (options.testFlag(ShowLogAction))
        textMenu->addAction(actionCollection->action("open_logfile"));

    if (options.testFlag(ShowChannelActions))
        textMenu->addAction(actionCollection->action("channel_settings"));

    QAction* action = textMenu->exec(pos);

    int actionId = extractActionId(action);

    if (showLinkActions)
        processLinkAction(actionId, link);

    textMenu->removeAction(toggleMenuBarAction);
    textMenu->removeAction(actionCollection->action(KStandardAction::name(KStandardAction::Find)));
    textMenu->removeAction(actionCollection->action("open_logfile"));
    textMenu->removeAction(actionCollection->action("channel_settings"));

    return actionId;
}

void IrcContextMenus::updateWebShortcutsMenu(const QString& selectedText)
{
#if KDE_IS_VERSION(4, 5, 0)
    m_webShortcutsMenu->menuAction()->setVisible(false);
    m_webShortcutsMenu->clear();

    if (selectedText.isEmpty())
        return;

    QString searchText = selectedText;
    searchText = searchText.replace('\n', ' ').replace('\r', ' ').simplified();

    if (searchText.isEmpty())
        return;

    KUriFilterData filterData(searchText);

#if KDE_IS_VERSION(4, 5, 67)
    filterData.setSearchFilteringOptions(KUriFilterData::RetrievePreferredSearchProvidersOnly);

    if (KUriFilter::self()->filterSearchUri(filterData, KUriFilter::NormalTextFilter))
#else
    filterData.setAlternateDefaultSearchProvider("google");

    if (KUriFilter::self()->filterUri(filterData, QStringList() << "kuriikwsfilter"))
#endif
    {
        const QStringList searchProviders = filterData.preferredSearchProviders();

        if (!searchProviders.isEmpty())
        {
            m_webShortcutsMenu->setTitle(i18n("Search for '%1' with",  KStringHandler::rsqueeze(searchText, 21)));

            KAction* action = 0;

            foreach(const QString& searchProvider, searchProviders)
            {
                action = new KAction(searchProvider, m_webShortcutsMenu);
                action->setIcon(KIcon(filterData.iconNameForPreferredSearchProvider(searchProvider)));
                action->setData(filterData.queryForPreferredSearchProvider(searchProvider));
                connect(action, SIGNAL(triggered()), this, SLOT(handleWebShortcutAction()));
                m_webShortcutsMenu->addAction(action);
            }

            m_webShortcutsMenu->addSeparator();

            action = new KAction(i18n("Configure Web Shortcuts..."), m_webShortcutsMenu);
            action->setIcon(KIcon("configure"));
            connect(action, SIGNAL(triggered()), this, SLOT(configureWebShortcuts()));
            m_webShortcutsMenu->addAction(action);

            m_webShortcutsMenu->menuAction()->setVisible(true);
        }
    }
#endif
}

void IrcContextMenus::handleWebShortcutAction()
{
#if KDE_IS_VERSION(4, 5, 0)
    KAction* action = qobject_cast<KAction*>(sender());

    if (action)
    {
        KUriFilterData filterData(action->data().toString());

#if KDE_IS_VERSION(4, 5, 67)
        if (KUriFilter::self()->filterSearchUri(filterData, KUriFilter::WebShortcutFilter))
#else
        if (KUriFilter::self()->filterUri(filterData, QStringList() << "kurisearchfilter"))
#endif
            Application::instance()->openUrl(filterData.uri().url());
    }
#endif
}

void IrcContextMenus::configureWebShortcuts()
{
#if KDE_IS_VERSION(4, 5, 0)
    KToolInvocation::kdeinitExec("kcmshell4", QStringList() << "ebrowsing");
#endif
}

void IrcContextMenus::setupChannelMenu()
{
    m_channelMenu = new KMenu();

    QAction* defaultAction = createAction(m_channelMenu, Join, KIcon("irc-join-channel"), i18n("&Join Channel..."));
    m_channelMenu->setDefaultAction(defaultAction);

    createAction(m_channelMenu, Topic, i18n("Get &topic"));
    createAction(m_channelMenu, Names, i18n("Get &user list"));
}

void IrcContextMenus::channelMenu(const QPoint& pos, Server* server, const QString& channel)
{
    KMenu* channelMenu = self()->m_channelMenu;

    QAction* title = 0;

    if (!channel.isEmpty())
        title = channelMenu->addTitle(KStringHandler::rsqueeze(channel, 15), channelMenu->actions().first());

    bool connected = server->isConnected();

    foreach(QAction* action, channelMenu->actions())
        action->setEnabled(connected);

    QAction* action = channelMenu->exec(pos);

    if (title)
    {
        channelMenu->removeAction(title);

        delete title;
    }

    switch (extractActionId(action))
    {
        case Join:
            commandToServer(server, "join " + channel);
            break;
        case Topic:
            server->requestTopic(channel);
            break;
        case Names:
            commandToServer(server, "names " + channel);
            break;
        default:
            break;
    }
}

void IrcContextMenus::setupNickMenu()
{
    m_nickMenu = new KMenu();

    QAction* defaultAction = createAction(m_nickMenu, OpenQuery, i18n("Open Query"));
    m_nickMenu->setDefaultAction(defaultAction);

    m_nickMenu->addSeparator();

    foreach(QAction* action, m_sharedBasicNickActions)
        m_nickMenu->addAction(action);

    m_nickMenu->addSeparator();

    m_modesMenu = new KMenu(m_nickMenu);
    m_nickMenu->addMenu(m_modesMenu);
    m_modesMenu->setTitle(i18n("Modes"));
    createAction(m_modesMenu, GiveOp, KIcon("irc-operator"), i18n("Give Op"));
    createAction(m_modesMenu, TakeOp, KIcon("irc-remove-operator"), i18n("Take Op"));
    createAction(m_modesMenu, GiveHalfOp, i18n("Give HalfOp"));
    createAction(m_modesMenu, TakeHalfOp, i18n("Take HalfOp"));
    createAction(m_modesMenu, GiveVoice, KIcon("irc-voice"), i18n("Give Voice"));
    createAction(m_modesMenu, TakeVoice, KIcon("irc-unvoice"), i18n("Take Voice"));

    m_kickBanMenu = new KMenu(m_nickMenu);
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

    m_nickMenu->addSeparator();

    foreach(QAction* action, m_sharedNickSettingsActions)
        m_nickMenu->addAction(action);

    m_nickMenu->addSeparator();

    foreach(QAction* action, m_sharedDccActions)
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

    m_addNotifyAction = createAction(AddNotify, KIcon("list-add-user"), i18n("Add to Watched Nicks"));
    m_sharedNickSettingsActions << m_addNotifyAction;
    m_removeNotifyAction = createAction(RemoveNotify, KIcon("list-remove-user"), i18n("Remove From Watched Nicks"));
    m_sharedNickSettingsActions << m_removeNotifyAction;

    m_addressBookMenu = new KMenu();
    m_addressBookMenu->setIcon(KIcon("office-address-book"));
    m_sharedNickSettingsActions << m_addressBookMenu->menuAction();
    m_addressBookNewAction = createAction(AddressbookNew, KIcon("contact-new"));
    m_addressBookChangeAction = createAction(AddressbookChange, KIcon("office-address-book"));
    m_addressBookEditAction = createAction(AddressbookEdit, KIcon("document-edit"));
    m_addressBookDeleteAction = createAction(AddressbookDelete, KIcon("edit-delete"));

    m_sendMailAction = createAction(SendEmail, KIcon("mail-send"), ("&Send Email..."));
    m_sharedNickSettingsActions << m_sendMailAction;
}

void IrcContextMenus::createSharedDccActions()
{
    if (KAuthorized::authorizeKAction("allow_downloading"))
        m_sharedDccActions << createAction(DccSend, KIcon("arrow-right-double"), i18n("Send &File..."));

    m_sharedDccActions << createAction(StartDccChat, i18n("Open DCC Chat"));
    m_sharedDccActions << createAction(StartDccWhiteboard, i18n("Open DCC Whiteboard"));
}

void IrcContextMenus::nickMenu(const QPoint& pos, MenuOptions options, Server* server,
    const QStringList& nicks, const QString& channel)
{
    KMenu* nickMenu = self()->m_nickMenu;

    QAction* title = 0;

    if (options.testFlag(ShowTitle) && nicks.count() == 1)
        title = nickMenu->addTitle(KStringHandler::rsqueeze(nicks.first(), 15), nickMenu->actions().first());

    foreach(QAction* action, nickMenu->actions())
        action->setVisible(true);

    self()->m_modesMenu->menuAction()->setVisible(options.testFlag(ShowChannelActions));
    self()->m_kickBanMenu->menuAction()->setVisible(options.testFlag(ShowChannelActions));

    bool connected = server->isConnected();

    foreach(QAction* action, self()->m_sharedBasicNickActions)
        action->setEnabled(connected);

    foreach(QAction* action, self()->m_sharedDccActions)
        action->setEnabled(connected);

    self()->m_modesMenu->menuAction()->setEnabled(connected);
    self()->m_kickBanMenu->menuAction()->setEnabled(connected);

    updateSharedNickSettingsActions(server, nicks);

    QAction* action = nickMenu->exec(pos);

    if (title)
    {
        nickMenu->removeAction(title);

        delete title;
    }

    processNickAction(extractActionId(action), server, nicks, channel);
}

void IrcContextMenus::processNickAction(int actionId, Server* server, const QStringList& nicks,
    const QString& channel)
{
    QString pattern;
    QString mode;

    switch (actionId)
    {
        case OpenQuery:
            commandToServer(server, "query %1", nicks);
            break;
        case Whois:
            commandToServer(server, "whois %1 %1", nicks);
            break;
        case Version:
            commandToServer(server, "ctcp %1 VERSION", nicks);
            break;
        case Ping:
            commandToServer(server, "ctcp %1 PING", nicks);
            break;
        case GiveOp:
            if (channel.isEmpty()) break;
            pattern = "MODE %c +%m %l";
            mode = 'o';
            break;
        case TakeOp:
            if (channel.isEmpty()) break;
            pattern = "MODE %c -%m %l";
            mode = 'o';
            break;
        case GiveHalfOp:
            if (channel.isEmpty()) break;
            pattern = "MODE %c +%m %l";
            mode = 'h';
            break;
        case TakeHalfOp:
            if (channel.isEmpty()) break;
            pattern = "MODE %c -%m %l";
            mode = 'h';
            break;
        case GiveVoice:
            if (channel.isEmpty()) break;
            pattern = "MODE %c +%m %l";
            mode = 'v';
            break;
        case TakeVoice:
            if (channel.isEmpty()) break;
            pattern = "MODE %c -%m %l";
            mode = 'v';
            break;
        case Kick:
            commandToServer(server, "kick %1", nicks, channel);
            break;
        case KickBan:
            commandToServer(server, "kickban %1", nicks, channel);
            break;
        case BanNick:
            commandToServer(server, "ban %1", nicks, channel);
            break;
        case BanHost:
            commandToServer(server, "ban -HOST %1", nicks, channel);
            break;
        case BanDomain:
            commandToServer(server, "ban -DOMAIN %1", nicks, channel);
            break;
        case BanUserHost:
            commandToServer(server, "ban -USERHOST %1", nicks, channel);
            break;
        case BanUserDomain:
            commandToServer(server, "ban -USERDOMAIN %1", nicks, channel);
            break;
        case KickBanHost:
            commandToServer(server, "kickban -HOST %1", nicks, channel);
            break;
        case KickBanDomain:
            commandToServer(server, "kickban -DOMAIN %1", nicks, channel);
            break;
        case KickBanUserHost:
            commandToServer(server, "kickban -USERHOST %1", nicks, channel);
            break;
        case KickBanUserDomain:
            commandToServer(server, "kickban -USERDOMAIN %1", nicks, channel);
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
                "IgnoreNick"
                ) ==
                KMessageBox::Continue)
            {
                commandToServer(server, "ignore -ALL " + nicks.join(" "));
            }

            break;
        }
        case UnignoreNick:
        {
            QString question;
            QStringList selectedIgnoredNicks;

            foreach(const QString& nick, nicks)
            {
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
                "UnignoreNick") ==
                KMessageBox::Continue)
            {
                commandToServer(server, "unignore " + selectedIgnoredNicks.join(" "));
            }

            break;
        }
        case AddNotify:
        {
            if (!server->getServerGroup()) break;

            foreach(const QString& nick, nicks)
            {
                if (!Preferences::isNotify(server->getServerGroup()->id(), nick))
                    Preferences::addNotify(server->getServerGroup()->id(), nick);
            }

            break;
        }
        case RemoveNotify:
        {
            if (!server->getServerGroup()) break;

            foreach(const QString& nick, nicks)
            {
                if (Preferences::isNotify(server->getServerGroup()->id(), nick))
                    Preferences::removeNotify(server->getServerGroup()->id(), nick);
            }

            break;
        }
        case DccSend:
            commandToServer(server, "dcc send %1", nicks);
            break;
        case StartDccChat:
            commandToServer(server, "dcc chat %1", nicks);
            break;
        case StartDccWhiteboard:
            commandToServer(server, "dcc whiteboard %1", nicks);
            break;
        case AddressbookEdit:
        {
            foreach(const QString& nick, nicks)
            {
                NickInfoPtr nickInfo = server->getNickInfo(nick);

                if (nickInfo.isNull())
                    continue;

                KABC::Addressee addressee = nickInfo->getAddressee();

                if(addressee.isEmpty())
                    break;

                Konversation::Addressbook::self()->editAddressee(addressee.uid());
            }

            break;
        }
        case AddressbookNew:
        case AddressbookDelete:
        {
            Konversation::Addressbook* addressbook = Konversation::Addressbook::self();

            // Handle all the selected nicks in one go. Either they all save, or none do.
            if (addressbook->getAndCheckTicket())
            {
                foreach(const QString& nick, nicks)
                {
                    NickInfoPtr nickInfo = server->getNickInfo(nick);

                    if (nickInfo.isNull())
                        continue;

                    if (actionId == AddressbookDelete)
                    {
                        KABC::Addressee addr = nickInfo->getAddressee();

                        addressbook->unassociateNick(addr, nick, server->getServerName(), server->getDisplayName());
                    }
                    else
                    {
                        // Make new addressbook contact.
                        KABC::Addressee addr;

                        if (nickInfo->getRealName().isEmpty())
                            addr.setGivenName(nickInfo->getNickname());
                        else
                            addr.setGivenName(nickInfo->getRealName());

                        addr.setNickName(nick);

                        addressbook->associateNickAndUnassociateFromEveryoneElse(addr, nick, server->getServerName(),
                            server->getDisplayName());
                    }
                }

                // This will refresh the nicks automatically for us. At least, if it doesn't, it's a bug :)
                addressbook->saveTicket();
            }

            break;
        }
        case AddressbookChange:
        {
            foreach(const QString& nick, nicks)
            {
                NickInfoPtr nickInfo = server->getNickInfo(nick);

                if (nickInfo.isNull())
                    continue;

                LinkAddressbookUI* linkaddressbookui = new LinkAddressbookUI(Application::instance()->getMainWindow(),
                    nick, server->getServerName(), server->getDisplayName(), nickInfo->getRealName());

                linkaddressbookui->show();
            }
            break;
        }
        case SendEmail:
        {
            NickInfoList nickInfos;

            foreach(const QString& nick, nicks)
            {
                NickInfoPtr nickInfo = server->getNickInfo(nick);

                if (nickInfo.isNull())
                    continue;

                nickInfos.append(nickInfo);
            }

            if (!nickInfos.isEmpty())
                Konversation::Addressbook::self()->sendEmail(nickInfos);

            break;
        }
        default:
            break;
    }

    if (!pattern.isEmpty())
    {
        pattern.replace("%c", channel);

        QString command;
        QStringList partialList;
        int modesCount = server->getModesCount();

        for (int index = 0; index < nicks.count(); index += modesCount)
        {
            command = pattern;
            partialList = nicks.mid(index, modesCount);
            command = command.replace("%l", partialList.join(" "));
            const QString repeatedMode = mode.repeated(partialList.count());

            command = command.replace("%m", repeatedMode);

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

    foreach(const QString& nick, nicks)
    {
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

    updateAddressBookActions(server, nicks);
}

void IrcContextMenus::updateAddressBookActions(Server* server, const QStringList& nicks)
{
    KMenu* addressBookMenu = self()->m_addressBookMenu;

    addressBookMenu->setTitle(i18np("Address Book Association", "Address Book Associations", nicks.count()));

    addressBookMenu->clear();

    bool existingAssociation = false;
    bool noAssociation = false;
    bool emailAddress = false;

    foreach(const QString& nick, nicks)
    {
        NickInfoPtr nickInfo =  server->getNickInfo(nick);

        if (nickInfo.isNull())
            continue;

        KABC::Addressee addr = nickInfo->getAddressee();

        if (addr.isEmpty())
        {
            noAssociation = true;

            if (existingAssociation && emailAddress)
                break;
        }
        else
        {
            if (!emailAddress && !addr.preferredEmail().isEmpty())
                emailAddress = true;

            existingAssociation = true;

            if (noAssociation && emailAddress)
                break;
        }
    }
    if (!noAssociation && existingAssociation)
    {
       self()->m_addressBookEditAction->setText(i18np("Edit Contact...",
            "Edit Contacts...", nicks.count()));
        addressBookMenu->addAction(self()->m_addressBookEditAction);
        addressBookMenu->addSeparator();
    }

    if (noAssociation && existingAssociation)
    {
        self()->m_addressBookChangeAction->setText(i18np("Choose/Change Association...",
            "Choose/Change Associations...", nicks.count()));
        addressBookMenu->addAction(self()->m_addressBookChangeAction);
    }
    else if (noAssociation)
    {
        self()->m_addressBookChangeAction->setText(i18np("Choose Contact...",
            "Choose Contacts...", nicks.count()));
        addressBookMenu->addAction(self()->m_addressBookChangeAction);
    }
    else
    {
        self()->m_addressBookChangeAction->setText(i18np("Change Association...",
            "Change Associations...", nicks.count()));
        addressBookMenu->addAction(self()->m_addressBookChangeAction);
    }

    if (noAssociation && !existingAssociation)
    {
        self()->m_addressBookNewAction->setText(i18np("Create New Contact...",
            "Create New Contacts...", nicks.count()));
        addressBookMenu->addAction(self()->m_addressBookNewAction);
    }

    if (existingAssociation)
    {
        self()->m_addressBookDeleteAction->setText(i18np("Delete Association",
            "Delete Associations", nicks.count()));
        addressBookMenu->addAction(self()->m_addressBookDeleteAction);
    }

    self()->m_sendMailAction->setEnabled(emailAddress);

    addressBookMenu->menuAction()->setVisible(true);
}

#if !(QT_VERSION >= QT_VERSION_CHECK(4, 7, 0))
void IrcContextMenus::setupLinkMenu()
{
    m_linkMenu = new KMenu();

    foreach(QAction* action, m_linkActions)
        m_linkMenu->addAction(action);
}

void IrcContextMenus::linkMenu(const QPoint& pos, const QString& link)
{
    QAction* action = self()->m_linkMenu->exec(pos);

    processLinkAction(extractActionId(action), link);
}
#endif

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
            KBookmarkManager* manager = KBookmarkManager::userBookmarksManager();
            KBookmarkDialog* dialog = new KBookmarkDialog(manager, Application::instance()->getMainWindow());

            dialog->addBookmark(link, link);

            delete dialog;

            break;
        }
        case LinkSaveAs:
        {
            KUrl srcUrl(link);

            KUrl saveUrl = KFileDialog::getSaveUrl(srcUrl.fileName(KUrl::ObeyTrailingSlash), QString(),
                Application::instance()->getMainWindow(), i18n("Save link as"));

            if (saveUrl.isEmpty() || !saveUrl.isValid())
                break;

            KIO::copy(srcUrl, saveUrl);

            break;
        }
        default:
            break;
    }
}

QAction* IrcContextMenus::createAction(ActionId id, const QString& text)
{
    QAction* action = new QAction(text, this);

    action->setData(id);

    return action;
}

QAction* IrcContextMenus::createAction(ActionId id, const QIcon& icon)
{
    QAction* action = new QAction(this);

    action->setData(id);
    action->setIcon(icon);

    return action;
}

QAction* IrcContextMenus::createAction(ActionId id, const QIcon& icon, const QString& text)
{
    QAction* action = new QAction(icon, text, this);

    action->setData(id);

    return action;
}

QAction* IrcContextMenus::createAction(KMenu* menu, ActionId id, const QString& text)
{
    QAction* action = createAction(id, text);

    menu->addAction(action);

    return action;
}

QAction* IrcContextMenus::createAction(KMenu* menu, ActionId id, const QIcon& icon, const QString& text)
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
    Konversation::OutputFilterResult result = server->getOutputFilter()->parse("", Preferences::self()->commandChar() + command, destination);

    server->queue(result.toServer);

    if (!result.output.isEmpty())
        server->appendMessageToFrontmost(result.typeString, result.output);
    else if (!result.outputList.isEmpty())
    {
        foreach(const QString& output, result.outputList)
            server->appendMessageToFrontmost(result.typeString, output);
    }
}

void IrcContextMenus::commandToServer(Server* server, const QString& command,
    const QStringList& arguments, const QString& destination)
{
    foreach(const QString& argument, arguments)
        commandToServer(server, command.arg(argument), destination);
}
