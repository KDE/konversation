/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2002 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2005-2008 Eike Hein <hein@kde.org>
*/

#include "query.h"

#include "channel.h"
#include "server.h"
#include "application.h"
#include "mainwindow.h"
#include "viewcontainer.h"
#include "ircinput.h"
#include "ircview.h"
#include "ircviewbox.h"
#include "common.h"
#include "topiclabel.h"

#include <QSplitter>

#include <KMessageBox>
#include <KStringHandler>
#include <KStandardGuiItem>
#include <KMenu>
#include <KHBox>
#include <KAuthorized>

Query::Query(QWidget* parent, QString _name) : ChatWindow(parent)
{
    name=_name; // need the name a little bit earlier for setServer
    // don't setName here! It will break logfiles!
    //   setName("QueryWidget");
    setType(ChatWindow::Query);

    setChannelEncodingSupported(true);

    m_headerSplitter = new QSplitter(Qt::Vertical, this);

    m_initialShow = true;
    awayChanged=false;
    awayState=false;
    KHBox* box = new KHBox(m_headerSplitter);
    m_headerSplitter->setStretchFactor(m_headerSplitter->indexOf(box), 0);
    addresseeimage = new QLabel(box);
    addresseeimage->setObjectName("query_image");
    addresseeimage->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    addresseeimage->hide();
    addresseelogoimage = new QLabel(box);
    addresseelogoimage->setObjectName("query_logo_image");
    addresseelogoimage->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    addresseelogoimage->hide();

    queryHostmask=new QLabel(box);
    queryHostmask->setObjectName("query_hostmask");

    QString whatsthis = i18n("<qt><p>Some details of the person you are talking to in this query is shown in this bar.  The full name and hostmask is shown, along with any image or logo this person has associated with them in the KDE Address Book.</p><p>See the <i>Konversation Handbook</i> for information on associating a nick with a contact in the address book, and for an explanation of what the hostmask is.</p></qt>");
    addresseeimage->setWhatsThis(whatsthis);
    addresseelogoimage->setWhatsThis(whatsthis);
    queryHostmask->setWhatsThis(whatsthis);

    IRCViewBox* ircBox = new IRCViewBox(m_headerSplitter,0);
    m_headerSplitter->setStretchFactor(m_headerSplitter->indexOf(ircBox), 1);
    setTextView(ircBox->ircView());               // Server will be set later in setServer();
    textView->setAcceptDrops(true);
    connect(textView,SIGNAL(filesDropped(const QStringList&)),this,SLOT(filesDropped(const QStringList&)));
    connect(textView,SIGNAL(popupCommand(int)),this,SLOT(popup(int)));

    // link "Whois", "Ignore" ... menu items into ircview popup
    m_actionGroup = new QActionGroup(this);
    connect(m_actionGroup, SIGNAL(triggered(QAction*)), this, SLOT(slotActionTriggered(QAction*)));

    KMenu* popup=textView->getPopup();
    popup->addSeparator();
    m_whoisAction = createAction(popup,i18n("&Whois"),Konversation::Whois);
    m_versionAction = createAction(popup,i18n("&Version"),Konversation::Version);
    m_pingAction = createAction(popup,i18n("&Ping"),Konversation::Ping);
    popup->addSeparator();

    m_ignoreNickAction = createAction(popup,i18n("Ignore"), Konversation::IgnoreNick);
    m_unignoreNickAction = createAction(popup,i18n("Unignore"), Konversation::UnignoreNick);
    m_ignoreNickAction->setVisible(false);
    m_unignoreNickAction->setVisible(false);

    m_dccAction = createAction(popup,i18n("Send &File..."),Konversation::DccSend);
    m_dccAction->setIcon(KIcon("arrow-right-double"));
    m_dccAction->setEnabled(KAuthorized::authorizeKAction("allow_downloading"));

    m_addNotifyAction = createAction(popup,i18n("Add to Watched Nicks"), Konversation::AddNotify);

    // This box holds the input line
    KHBox* inputBox=new KHBox(this);
    inputBox->setObjectName("input_log_box");
    inputBox->setSpacing(spacing());

    awayLabel=new QLabel(i18n("(away)"), inputBox);
    awayLabel->hide();
    blowfishLabel = new QLabel(inputBox);
    blowfishLabel->hide();
    blowfishLabel->setPixmap(KIconLoader::global()->loadIcon("document-encrypt", KIconLoader::Toolbar));
    queryInput=new IRCInput(inputBox);

    getTextView()->installEventFilter(queryInput);
    queryInput->installEventFilter(this);

    // connect the signals and slots
    connect(queryInput,SIGNAL (submit()),this,SLOT (queryTextEntered()) );
    connect(queryInput,SIGNAL (envelopeCommand()),this,SLOT (queryPassthroughCommand()) );
    connect(queryInput,SIGNAL (textPasted(const QString&)),this,SLOT (textPasted(const QString&)) );
    connect(getTextView(), SIGNAL(textPasted(bool)), queryInput, SLOT(paste(bool)));
    connect(getTextView(),SIGNAL (gotFocus()),queryInput,SLOT (setFocus()) );

    connect(textView,SIGNAL (sendFile()),this,SLOT (sendFileMenu()) );
    connect(textView,SIGNAL (extendedPopup(int)),this,SLOT (popup(int)) );
    connect(textView,SIGNAL (autoText(const QString&)),this,SLOT (sendQueryText(const QString&)) );

    updateAppearance();

    setLog(Preferences::self()->log());

    #ifdef HAVE_QCA2
    m_cipher = 0;
    #endif
}

Query::~Query()
{
}

void Query::setServer(Server* newServer)
{
    if (m_server != newServer)
    {
        connect(newServer, SIGNAL(connectionStateChanged(Server*, Konversation::ConnectionState)),
                SLOT(connectionStateChanged(Server*, Konversation::ConnectionState)));
        connect(newServer, SIGNAL(nickInfoChanged(Server*, NickInfoPtr)),
                this, SLOT(updateNickInfo(Server*, NickInfoPtr)));
    }

    ChatWindow::setServer(newServer);

    if (!(newServer->getKeyForRecipient(getName()).isEmpty()))
        blowfishLabel->show();
}

void Query::connectionStateChanged(Server* server, Konversation::ConnectionState state)
{
    if (server == m_server)
    {
        if (state ==  Konversation::SSConnected)
        {
            //HACK the way the notification priorities work sucks, this forces the tab text color to ungray right now.
            if (m_currentTabNotify == Konversation::tnfNone || !Preferences::self()->tabNotificationsEvents())
                Application::instance()->getMainWindow()->getViewContainer()->unsetViewNotification(this);
        }
        else
        {
            //HACK the way the notification priorities work sucks, this forces the tab text color to gray right now.
            if (m_currentTabNotify == Konversation::tnfNone || (!Preferences::self()->tabNotificationsEvents() && m_currentTabNotify == Konversation::tnfControl))
                Application::instance()->getMainWindow()->getViewContainer()->unsetViewNotification(this);
        }
    }
}

void Query::setName(const QString& newName)
{
    //if(ChatWindow::getName() == newName) return;  // no change, so return


    if(ChatWindow::getName() != newName)
    {
        appendCommandMessage(i18n("Nick"),i18n("%1 is now known as %2.", getName(), newName),false);
    }


    ChatWindow::setName(newName);

    // don't change logfile name if query name changes
    // This will prevent Nick-Changers to create more than one log file,
    if (logName.isEmpty())
    {
        QString logName =  (Preferences::self()->lowerLog()) ? getName().toLower() : getName() ;

        if(Preferences::self()->addHostnameToLog())
        {
            if(m_nickInfo)
                logName += m_nickInfo->getHostmask();
        }

        setLogfileName(logName);
    }
}

void Query::setEncryptedOutput(bool e)
{
    if (e)
        blowfishLabel->show();
    else
        blowfishLabel->hide();
}

void Query::queryTextEntered()
{
    QString line=queryInput->toPlainText();
    queryInput->clear();
    if(line.toLower()==Preferences::self()->commandChar()+"clear")
    {
        textView->clear();
    }
    else if(line.length())
    {
         sendQueryText(line);
    }
}

void Query::queryPassthroughCommand()
{
    QString commandChar = Preferences::self()->commandChar();
    QString line = queryInput->toPlainText();

    queryInput->clear();

    if(!line.isEmpty())
    {
        // Prepend commandChar on Ctrl+Enter to bypass outputfilter command recognition
        if (line.startsWith(commandChar))
        {
            line = commandChar + line;
        }
        sendQueryText(line);
    }
}

void Query::sendQueryText(const QString& sendLine)
{
    // create a work copy
    QString outputAll(sendLine);
    // replace aliases and wildcards
    if(m_server->getOutputFilter()->replaceAliases(outputAll))
    {
        outputAll = m_server->parseWildcards(outputAll, m_server->getNickname(), getName(), QString(), QString(), QString());
    }

    // Send all strings, one after another
    QStringList outList = outputAll.split('\n', QString::SkipEmptyParts);
    for(int index=0;index<outList.count();index++)
    {
        QString output(outList[index]);

        // encoding stuff is done in Server()
        Konversation::OutputFilterResult result = m_server->getOutputFilter()->parse(m_server->getNickname(), output, getName());

        if(!result.output.isEmpty())
        {
            if(result.type == Konversation::Action) appendAction(m_server->getNickname(), result.output);
            else if(result.type == Konversation::Command) appendCommandMessage(result.typeString, result.output);
            else if(result.type == Konversation::Program) appendServerMessage(result.typeString, result.output);
            else if(result.type == Konversation::PrivateMessage) msgHelper(result.typeString, result.output);
            else if(!result.typeString.isEmpty()) appendQuery(result.typeString, result.output);
            else appendQuery(m_server->getNickname(), result.output);
        }
        else if (result.outputList.count())
        {
            if (result.type == Konversation::Message)
            {
                QStringListIterator it(result.outputList);

                while (it.hasNext())
                    appendQuery(m_server->getNickname(), it.next());
            }
            else if (result.type == Konversation::Action)
            {
                for (int i = 0; i < result.outputList.count(); ++i)
                {
                    if (i == 0)
                        appendAction(m_server->getNickname(), result.outputList.at(i));
                    else
                        appendQuery(m_server->getNickname(), result.outputList.at(i));
                }
            }
        }

        // Send anything else to the server
        if (!result.toServerList.empty())
            m_server->queueList(result.toServerList);
        else
            m_server->queue(result.toServer);
    } // for
}

void Query::updateAppearance()
{
    QColor fg, bg;

    if (Preferences::self()->inputFieldsBackgroundColor())
    {
        fg = Preferences::self()->color(Preferences::ChannelMessage);
        bg = Preferences::self()->color(Preferences::TextViewBackground);
    }
    else
    {
        fg = palette().windowText().color();
        bg = palette().base().color();
    }

    QPalette queryInputPalette(queryInput->palette());
    queryInputPalette.setColor(QPalette::WindowText, fg);
    queryInputPalette.setColor(QPalette::Text, fg);
    queryInputPalette.setColor(QPalette::Base, bg);

    queryInput->setPalette(queryInputPalette);


    if (Preferences::self()->customTextFont())
        queryInput->setFont(Preferences::self()->textFont());
    else
        queryInput->setFont(KGlobalSettings::generalFont());

    ChatWindow::updateAppearance();
}

void Query::textPasted(const QString& text)
{
    if(m_server)
    {
        QStringList multiline = text.split('\n', QString::SkipEmptyParts);
        for(int index=0;index<multiline.count();index++)
        {
            QString line=multiline[index];
            QString cChar(Preferences::self()->commandChar());
            // make sure that lines starting with command char get escaped
            if(line.startsWith(cChar)) line=cChar+line;
            sendQueryText(line);
        }
    }
}

void Query::indicateAway(bool show)
{
    // QT does not redraw the label properly when they are not on screen
    // while getting hidden, so we remember the "soon to be" state here.
    if(isHidden())
    {
        awayChanged=true;
        awayState=show;
    }
    else
    {
        if(show)
            awayLabel->show();
        else
            awayLabel->hide();
    }
}

// fix QTs broken behavior on hidden QListView pages
void Query::showEvent(QShowEvent*)
{
    if(awayChanged)
    {
        awayChanged=false;
        indicateAway(awayState);
    }

    if(m_initialShow) {
        m_initialShow = false;
        QList<int> sizes;
        sizes << queryHostmask->sizeHint().height() << (height() - queryHostmask->sizeHint().height());
        m_headerSplitter->setSizes(sizes);
    }
}

void Query::popup(int id)
{
    QString name = getName();

    switch (id)
    {
        case Konversation::Whois:
            sendQueryText(Preferences::self()->commandChar()+"WHOIS "+name+' '+name);
            break;

        case Konversation::IgnoreNick:
        {
            if (KMessageBox::warningContinueCancel(
                this,
                i18n("Do you want to ignore %1?",name),
                i18n("Ignore"),
                KGuiItem(i18n("Ignore")),
                KStandardGuiItem::cancel(),
                "IgnoreNick")
                == KMessageBox::Continue)
            {
                sendQueryText(Preferences::self()->commandChar()+"IGNORE -ALL "+name);

                int rc = KMessageBox::questionYesNo(this,
                i18n("Do you want to close this query after ignoring this nickname?"),
                i18n("Close This Query"),
                KGuiItem(i18n("Close")),
                KGuiItem(i18n("Keep Open")),
                "CloseQueryAfterIgnore");

                if (rc == KMessageBox::Yes && m_server)
                    QTimer::singleShot(0, this, SLOT(closeWithoutAsking()));
            }

            break;
        }
        case Konversation::UnignoreNick:
        {
            if (KMessageBox::warningContinueCancel(
                this,
                i18n("Do you want to stop ignoring %1?", name),
                i18n("Unignore"),
                KGuiItem(i18n("Unignore")),
                KStandardGuiItem::cancel(),
                "UnignoreNick")
                ==
                KMessageBox::Continue)
            {
                sendQueryText(Preferences::self()->commandChar()+"UNIGNORE "+name);
            }

            break;
        }
        case Konversation::AddNotify:
        {
            if (m_server->getServerGroup())
            {
                if (!Preferences::isNotify(m_server->getServerGroup()->id(), name))
                    Preferences::addNotify(m_server->getServerGroup()->id(),name);
            }
            break;
        }
        case Konversation::DccSend:
            sendQueryText(Preferences::self()->commandChar()+"DCC SEND "+name);
            break;

        case Konversation::Version:
            sendQueryText(Preferences::self()->commandChar()+"CTCP "+name+" VERSION");
            break;

        case Konversation::Ping:
            sendQueryText(Preferences::self()->commandChar()+"CTCP "+name+" PING");
            break;

        case Konversation::Topic:
            m_server->requestTopic(getTextView()->currentChannel());
            break;
        case Konversation::Names:
            m_server->queue("NAMES " + getTextView()->currentChannel(), Server::LowPriority);
            break;
        case Konversation::Join:
            m_server->queue("JOIN " + getTextView()->currentChannel());
            break;

        default:
            kDebug() << "Popup id " << id << " does not belong to me!";
            break;
    }

    // delete context menu nickname
    textView->clearContextNick();
}

void Query::sendFileMenu()
{
    emit sendFile(getName());
}

void Query::childAdjustFocus()
{
    queryInput->setFocus();
}

void Query::setNickInfo(const NickInfoPtr & nickInfo)
{
    m_nickInfo = nickInfo;
    Q_ASSERT(m_nickInfo); if(!m_nickInfo) return;
    nickInfoChanged();
}

void Query::updateNickInfo(Server* server, NickInfoPtr nickInfo)
{
    if (!m_nickInfo || server != m_server || nickInfo != m_nickInfo)
        return;

    nickInfoChanged();
}

void Query::nickInfoChanged()
{
    if (m_nickInfo)
    {
        setName(m_nickInfo->getNickname());
        QString text = m_nickInfo->getBestAddresseeName();
        if(!m_nickInfo->getHostmask().isEmpty() && !text.isEmpty())
            text += " - ";
        text += m_nickInfo->getHostmask();
        if(m_nickInfo->isAway() && !m_nickInfo->getAwayMessage().isEmpty())
            text += " (" + KStringHandler::rsqueeze(m_nickInfo->getAwayMessage(),100) + ") ";
        queryHostmask->setText(Konversation::removeIrcMarkup(text));

        KABC::Picture pic = m_nickInfo->getAddressee().photo();
        if(pic.isIntern())
        {
            QPixmap qpixmap = QPixmap::fromImage(pic.data().scaledToHeight(queryHostmask->height()));
            if(!qpixmap.isNull())
            {
                addresseeimage->setPixmap(qpixmap);
                addresseeimage->show();
            }
            else
            {
                addresseeimage->hide();
            }
        }
        else
        {
            addresseeimage->hide();
        }
        KABC::Picture logo = m_nickInfo->getAddressee().logo();
        if(logo.isIntern())
        {
            QPixmap qpixmap = QPixmap::fromImage(logo.data().scaledToHeight(queryHostmask->height()));
            if(!qpixmap.isNull())
            {
                addresseelogoimage->setPixmap(qpixmap);
                addresseelogoimage->show();
            }
            else
            {
                addresseelogoimage->hide();
            }
        }
        else
        {
            addresseelogoimage->hide();
        }

        QString strTooltip;
        QTextStream tooltip( &strTooltip, QIODevice::WriteOnly );

        tooltip << "<qt>";

        tooltip << "<table cellspacing=\"0\" cellpadding=\"0\">";

        m_nickInfo->tooltipTableData(tooltip);

        tooltip << "</table></qt>";
        queryHostmask->setToolTip(strTooltip);
        addresseeimage->setToolTip(strTooltip);
        addresseelogoimage->setToolTip(strTooltip);

    }
    else
    {
        addresseeimage->hide();
        addresseelogoimage->hide();
    }

    emit updateQueryChrome(this,getName());
    emitUpdateInfo();
}

NickInfoPtr Query::getNickInfo()
{
    return m_nickInfo;
}

QString Query::getTextInLine() { return queryInput->toPlainText(); }

bool Query::canBeFrontView()        { return true; }
bool Query::searchView()       { return true; }

void Query::appendInputText(const QString& s, bool fromCursor)
{
    if(!fromCursor)
    {
        queryInput->append(s);
    }
    else
    {
        const int position = queryInput->textCursor().position();
        queryInput->textCursor().insertText(s);
        queryInput->textCursor().setPosition(position + s.length());
    }
}

                                                  // virtual
void Query::setChannelEncoding(const QString& encoding)
{
    if(m_server->getServerGroup())
        Preferences::setChannelEncoding(m_server->getServerGroup()->id(), getName(), encoding);
    else
        Preferences::setChannelEncoding(m_server->getDisplayName(), getName(), encoding);
}

QString Query::getChannelEncoding()               // virtual
{
    if(m_server->getServerGroup())
        return Preferences::channelEncoding(m_server->getServerGroup()->id(), getName());
    return Preferences::channelEncoding(m_server->getDisplayName(), getName());
}

QString Query::getChannelEncodingDefaultDesc()    // virtual
{
    return i18n("Identity Default ( %1 )",getServer()->getIdentity()->getCodecName());
}

bool Query::closeYourself(bool confirm)
{
    int result = KMessageBox::Continue;
    if (confirm)
        result=KMessageBox::warningContinueCancel(
            this,
            i18n("Do you want to close your query with %1?", getName()),
            i18n("Close Query"),
            KStandardGuiItem::close(),
            KStandardGuiItem::cancel(),
            "QuitQueryTab");

    if (result == KMessageBox::Continue)
    {
        m_server->removeQuery(this);
        return true;
    }

    return false;
}

void Query::closeWithoutAsking()
{
    m_server->removeQuery(this);
}

void Query::filesDropped(const QStringList& files)
{
    m_server->sendURIs(files, getName());
}

void Query::serverOnline(bool online)
{
    //queryInput->setEnabled(online);
    getTextView()->setNickAndChannelContextMenusEnabled(online);

    KMenu* popup = getTextView()->getPopup();

    if (popup)
    {
        m_whoisAction->setEnabled(online);
        m_versionAction->setEnabled(online);
        m_ignoreNickAction->setEnabled(online);
        m_unignoreNickAction->setEnabled(online);
        m_addNotifyAction->setEnabled(online);
        m_dccAction->setEnabled(online && KAuthorized::authorizeKAction("allow_downloading"));
    }
}

void Query::emitUpdateInfo()
{
    QString info;
    if(m_nickInfo->loweredNickname() == m_server->loweredNickname())
        info = i18n("Talking to yourself");
    else if(m_nickInfo)
        info = m_nickInfo->getBestAddresseeName();
    else
        info = getName();

    emit updateInfo(info);
}

// show quit message of nick if we see it
void Query::quitNick(const QString& reason)
{
    QString displayReason = reason;

    if (displayReason.isEmpty())
    {
        appendCommandMessage(i18n("Quit"),i18n("%1 has left this server.",getName()),false);
    }
    else
    {
        if (displayReason.contains(QRegExp("[\\0000-\\0037]")))
            displayReason+="\017";

        appendCommandMessage(i18n("Quit"),i18n("%1 has left this server (%2).",getName(),displayReason),false);
    }
}

Q_DECLARE_METATYPE(Konversation::PopupIDs)

KAction* Query::createAction(QMenu* menu, const QString& text, Konversation::PopupIDs id)
{
    KAction* action = new KAction(text, menu);
    menu->addAction(action);
    action->setData(QVariant::fromValue(id));
    m_actionGroup->addAction(action);
    return action;
}

void Query::slotActionTriggered(QAction* action)
{
    popup(action->data().value<Konversation::PopupIDs>());
}

#ifdef HAVE_QCA2
Konversation::Cipher* Query::getCipher()
{
    if(!m_cipher)
        m_cipher = new Konversation::Cipher();
    return m_cipher;
}
#endif
