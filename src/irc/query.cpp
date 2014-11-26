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
#include "awaylabel.h"
#include "common.h"

#include <QSplitter>

#include <KIconLoader>
#include <KMessageBox>
#include <KSqueezedTextLabel>

using namespace Konversation;

Query::Query(QWidget* parent, const QString& _name) : ChatWindow(parent)
{
    name=_name; // need the name a little bit earlier for setServer
    // don't setName here! It will break logfiles!
    //   setName("QueryWidget");
    setType(ChatWindow::Query);
    m_isTopLevelView = false;

    setChannelEncodingSupported(true);

    m_headerSplitter = new QSplitter(Qt::Vertical, this);

    m_initialShow = true;
    awayChanged=false;
    awayState=false;

    queryHostmask=new KSqueezedTextLabel(m_headerSplitter);
    m_headerSplitter->setStretchFactor(m_headerSplitter->indexOf(queryHostmask), 0);
    queryHostmask->setTextElideMode(Qt::ElideRight);
    queryHostmask->setObjectName(QStringLiteral("query_hostmask"));

    QString whatsthis = i18n("<qt><p>Some details of the person you are talking to in this query is shown in this bar. The full name and hostmask is shown.</p><p>See the <i>Konversation Handbook</i> for an explanation of what the hostmask is.</p></qt>");
    queryHostmask->setWhatsThis(whatsthis);

    IRCViewBox* ircViewBox = new IRCViewBox(m_headerSplitter);
    m_headerSplitter->setStretchFactor(m_headerSplitter->indexOf(ircViewBox), 1);
    setTextView(ircViewBox->ircView());               // Server will be set later in setServer();
    ircViewBox->ircView()->setContextMenuOptions(IrcContextMenus::ShowNickActions, true);
    textView->setAcceptDrops(true);
    connect(textView,SIGNAL(urlsDropped(QList<QUrl>)),this,SLOT(urlsDropped(QList<QUrl>)));

    // This box holds the input line
    QWidget* inputBox=new QWidget(this);
    QHBoxLayout* inputBoxLayout = new QHBoxLayout(inputBox);
    inputBox->setObjectName(QStringLiteral("input_log_box"));
    inputBoxLayout->setSpacing(spacing());
    inputBoxLayout->setMargin(0);

    awayLabel=new AwayLabel(inputBox);
    inputBoxLayout->addWidget(awayLabel);
    awayLabel->hide();
    blowfishLabel = new QLabel(inputBox);
    inputBoxLayout->addWidget(blowfishLabel);
    blowfishLabel->hide();
    blowfishLabel->setPixmap(KIconLoader::global()->loadIcon(QStringLiteral("document-encrypt"), KIconLoader::Toolbar));
    m_inputBar=new IRCInput(inputBox);
    inputBoxLayout->addWidget(m_inputBar);

    getTextView()->installEventFilter(m_inputBar);
    m_inputBar->installEventFilter(this);

    // connect the signals and slots
    connect(m_inputBar, &IRCInput::submit, this, &Query::queryTextEntered);
    connect(m_inputBar, &IRCInput::envelopeCommand, this, &Query::queryPassthroughCommand);
    connect(m_inputBar, &IRCInput::textPasted, this, &Query::textPasted);
    connect(getTextView(), SIGNAL(textPasted(bool)), m_inputBar, SLOT(paste(bool)));
    connect(getTextView(),SIGNAL (gotFocus()),m_inputBar,SLOT (setFocus()) );

    connect(textView,SIGNAL (sendFile()),this,SLOT (sendFileMenu()) );
    connect(textView,SIGNAL (autoText(QString)),this,SLOT (sendText(QString)) );

    updateAppearance();

    #ifdef HAVE_QCA2
    m_cipher = 0;
    #endif
}

Query::~Query()
{
    if (m_recreationScheduled)
    {
        qRegisterMetaType<NickInfoPtr>("NickInfoPtr");

        QMetaObject::invokeMethod(m_server, "addQuery", Qt::QueuedConnection,
            Q_ARG(NickInfoPtr, m_nickInfo), Q_ARG(bool, true));
    }
}

void Query::setServer(Server* newServer)
{
    if (m_server != newServer)
    {
        connect(newServer, SIGNAL(connectionStateChanged(Server*,Konversation::ConnectionState)),
                SLOT(connectionStateChanged(Server*,Konversation::ConnectionState)));
        connect(newServer, SIGNAL(nickInfoChanged(Server*,NickInfoPtr)),
                this, SLOT(updateNickInfo(Server*,NickInfoPtr)));
    }

    ChatWindow::setServer(newServer);

    if (!(newServer->getKeyForRecipient(getName()).isEmpty()))
        blowfishLabel->show();

    connect(awayLabel, SIGNAL(unaway()), m_server, SLOT(requestUnaway()));
    connect(awayLabel, SIGNAL(awayMessageChanged(QString)), m_server, SLOT(requestAway(QString)));
}

void Query::connectionStateChanged(Server* server, Konversation::ConnectionState state)
{
    if (server == m_server)
    {
        ViewContainer* viewContainer = Application::instance()->getMainWindow()->getViewContainer();

        if (state ==  Konversation::SSConnected)
        {
            //HACK the way the notification priorities work sucks, this forces the tab text color to ungray right now.
            if (viewContainer->getFrontView() == this
                || m_currentTabNotify == Konversation::tnfNone
                || !Preferences::self()->tabNotificationsEvents())
            {
                viewContainer->unsetViewNotification(this);
            }
        }
        else
        {
            //HACK the way the notification priorities work sucks, this forces the tab text color to gray right now.
            if (viewContainer->getFrontView() == this
                || m_currentTabNotify == Konversation::tnfNone
                || (!Preferences::self()->tabNotificationsEvents() && m_currentTabNotify == Konversation::tnfControl))
            {
                viewContainer->unsetViewNotification(this);
            }
        }
    }
}

void Query::setName(const QString& newName)
{
    //if(ChatWindow::getName() == newName) return;  // no change, so return


    if(ChatWindow::getName() != newName)
    {
        appendCommandMessage(i18n("Nick"),i18n("%1 is now known as %2.", getName(), newName));
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
    QString line=m_inputBar->toPlainText();

    m_inputBar->clear();

    if (!line.isEmpty()) sendText(sterilizeUnicode(line));
}

void Query::queryPassthroughCommand()
{
    QString commandChar = Preferences::self()->commandChar();
    QString line = m_inputBar->toPlainText();

    m_inputBar->clear();

    if(!line.isEmpty())
    {
        // Prepend commandChar on Ctrl+Enter to bypass outputfilter command recognition
        if (line.startsWith(commandChar))
        {
            line = commandChar + line;
        }
        sendText(sterilizeUnicode(line));
    }
}

void Query::sendText(const QString& sendLine)
{
    // create a work copy
    QString outputAll(sendLine);

    // replace aliases and wildcards
    m_server->getOutputFilter()->replaceAliases(outputAll, this);

    // Send all strings, one after another
    QStringList outList = outputAll.split(QLatin1Char('\n'), QString::SkipEmptyParts);
    for(int index=0;index<outList.count();index++)
    {
        QString output(outList[index]);

        // encoding stuff is done in Server()
        Konversation::OutputFilterResult result = m_server->getOutputFilter()->parse(m_server->getNickname(), output, getName(), this);

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

void Query::textPasted(const QString& text)
{
    if(m_server)
    {
        QStringList multiline = text.split(QLatin1Char('\n'), QString::SkipEmptyParts);
        for(int index=0;index<multiline.count();index++)
        {
            QString line=multiline[index];
            QString cChar(Preferences::self()->commandChar());
            // make sure that lines starting with command char get escaped
            if(line.startsWith(cChar)) line=cChar+line;
            sendText(line);
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

void Query::sendFileMenu()
{
    emit sendFile(getName());
}

void Query::childAdjustFocus()
{
    m_inputBar->setFocus();
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
            text += QStringLiteral(" - ");
        text += m_nickInfo->getHostmask();
        if(m_nickInfo->isAway() && !m_nickInfo->getAwayMessage().isEmpty())
            text += QStringLiteral(" (") + m_nickInfo->getAwayMessage() + QStringLiteral(") ");
        queryHostmask->setText(Konversation::removeIrcMarkup(text));

        QString strTooltip;
        QTextStream tooltip( &strTooltip, QIODevice::WriteOnly );

        tooltip << "<qt>";

        tooltip << "<table cellspacing=\"5\" cellpadding=\"0\">";

        m_nickInfo->tooltipTableData(tooltip);

        tooltip << "</table></qt>";
        queryHostmask->setToolTip(strTooltip);
    }

    emit updateQueryChrome(this,getName());
    emitUpdateInfo();
}

NickInfoPtr Query::getNickInfo()
{
    return m_nickInfo;
}

bool Query::canBeFrontView()        { return true; }
bool Query::searchView()       { return true; }
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
            QStringLiteral("QuitQueryTab"));

    if (result == KMessageBox::Continue)
    {
        m_server->removeQuery(this);

        return true;
    }
    else
        m_recreationScheduled = false;

    return false;
}

void Query::urlsDropped(const QList<QUrl>& urls)
{
    m_server->sendURIs(urls, getName());
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
        appendCommandMessage(i18nc("Message type", "Quit"), i18nc("%1 = nick, %2 = hostmask", "%1 (%2) has left this server.",
            getName(), getNickInfo()->getHostmask()), false);
    }
    else
    {
        if (hasIRCMarkups(displayReason))
            displayReason+=QStringLiteral("\017");

        appendCommandMessage(i18nc("Message type", "Quit"), i18nc("%1 = nick, %2 = hostmask, %3 = reason", "%1 (%2) has left this server (%3).",
            getName(), getNickInfo()->getHostmask(), displayReason), false);
    }
}

#ifdef HAVE_QCA2
Konversation::Cipher* Query::getCipher()
{
    if(!m_cipher)
        m_cipher = new Konversation::Cipher();
    return m_cipher;
}
#endif
