/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2004, 2016 Peter Simonsson <peter.simonsson@gmail.com>
    SPDX-FileCopyrightText: 2006-2008 Eike Hein <hein@kde.org>
*/

#include "inputfilter.h"

#include "server.h"
#include "replycodes.h"
#include "application.h"
#include "version.h"
#include "query.h"
#include "channel.h"
#include "statuspanel.h"
#include "common.h"
#include "notificationhandler.h"
#include "konversation_log.h"
#include <config-konversation.h>

#include <QDateTime>
#include <QRegularExpression>
#include <QLocale>


InputFilter::InputFilter()
    : m_server(nullptr),
      m_lagMeasuring(false)
{
    m_connecting = false;
}

InputFilter::~InputFilter()
{
}

void InputFilter::setServer(Server* newServer)
{
    m_server = newServer;
}

template<typename T>
int posOrLen(T chr, const QString& str, int from=0)
{
    int p=str.indexOf(QLatin1Char(chr), from);
    if (p<0)
        return str.size();
    return p;
}

/// "[22:08] >> :thiago!n=thiago@kde/thiago QUIT :Read error: 110 (Connection timed out)"
/// "[21:47] >> :Zarin!n=x365@kde/developer/lmurray PRIVMSG #plasma :If the decoration doesn't have paint( QPixmap ) it falls back to the old one"
/// "[21:49] >> :niven.freenode.net 352 argonel #kde-forum i=beezle konversation/developer/argonel irc.freenode.net argonel H :0 Konversation User "
void InputFilter::parseLine(const QString& line)
{
    int start=0;
    QHash<QString, QString> messageTags;

    if (line[0] == QLatin1Char('@'))
    {
        messageTags = parseMessageTags(line, &start);
    }

    QString prefix;
    int end(posOrLen(' ', line, start));

    if (line[start]==QLatin1Char(':'))
    {
        prefix = line.mid(start + 1, end - start - 1); //skips the colon and does not include the trailing space
        start = end + 1;
        end = posOrLen(' ', line, start);
    }

    //even though the standard is UPPER CASE, someone when through a great deal of trouble to make this lower case...
    QString command = QString(line.mid(start, end-start)).toLower();
    start=end+1;

    int trailing=line.indexOf(QLatin1String(" :"), end);
    if (trailing >= 0)
        end=trailing;
    else
        end=line.size();

    QStringList parameterList;

    while (start < end)
    {
        if (line[start]==QLatin1Char(' '))
            start++;
        else
        {
            int p=line.indexOf(QLatin1Char(' '), start); //easier to have Qt loop for me :)
            if (p<0)
                p=end;
            parameterList << line.mid(start, p-start);
            start=p+1;
        }
    };

    /* Quote: "The final colon is specified as a "last argument" designator, and
     * is always valid before the final argument."
     * Quote: "The last parameter may be an empty string."
     * Quote: "After extracting the parameter list, all parameters are equal
     * whether matched by <middle> or <trailing>. <trailing> is just a
     * syntactic trick to allow SPACE within the parameter."
     */

    if (trailing >= 0 ) //<-- trailing == ":" - per above we need the empty string
        parameterList << line.mid( qMin(trailing+2, line.size()) );

    Q_ASSERT(m_server); //how could we have gotten a line without a server?


    // Server command, if no "!" was found in prefix
    if ((!prefix.contains(QLatin1Char('!'))) && (prefix != m_server->getNickname()))
    {
        parseServerCommand(prefix, command, parameterList, messageTags);
    }
    else
    {
        parseClientCommand(prefix, command, parameterList, messageTags);
    }
}

#define trailing (parameterList.isEmpty() ? QString() : parameterList.last())
#define plHas(x) _plHas(parameterList.count(), (x))

bool _plHad=false;
int _plWanted = 0;

bool _plHas(int count, int x)
{
    _plHad=(count >= x);
    _plWanted = x;
    if (!_plHad)
        qCDebug(KONVERSATION_LOG) << "plhad" << count << "wanted" << x;
    return _plHad;
}

void InputFilter::parseClientCommand(const QString &prefix, const QString &command, QStringList &parameterList, const QHash<QString, QString> &messageTags)
{
    Application* konv_app = Application::instance();
    Q_ASSERT(konv_app);
    Q_ASSERT(m_server);

    // Extract nickname from prefix
    int pos = prefix.indexOf(QLatin1Char('!'));
    QString sourceNick = prefix.left(pos);
    QString sourceHostmask = prefix.mid(pos + 1);

    // remember hostmask for this nick, it could have changed
    m_server->addHostmaskToNick(sourceNick, sourceHostmask);

    bool isNumeric = false;
    int numeric = command.toInt(&isNumeric);

    if(isNumeric)
    {
        parseNumeric(prefix, numeric, parameterList, messageTags);
    }
    //PRIVMSG #channel :message
    else if (command == QLatin1String("privmsg") && plHas(2))
    {
        bool isChan = isAChannel(parameterList.value(0));
        // CTCP message?
        if (m_server->identifyMsg() && (trailing.length() > 1 && (trailing.at(0) == QLatin1Char('+') || trailing.at(0) == QLatin1Char('-'))))
        {
            trailing.remove(0, 1);
        }

        if (!trailing.isEmpty() && trailing.at(0)==QChar(0x01))
        {
            // cut out the CTCP command
            QString ctcp = trailing.mid(1,trailing.indexOf(QChar(0x01),1)-1);

            //QString::left(-1) returns the entire string
            QString ctcpCommand = ctcp.left(ctcp.indexOf(QLatin1Char(' '))).toLower();
            bool hasArg = ctcp.indexOf(QLatin1Char(' ')) > 0;
            //QString::mid(-1)+1 = 0, which returns the entire string if there is no space, resulting in command==arg
            QString ctcpArgument = hasArg ? ctcp.mid(ctcp.indexOf(QLatin1Char(' '))+1) : QString();
            hasArg = !ctcpArgument.isEmpty();
            if (hasArg)
                ctcpArgument = konv_app->doAutoreplace(ctcpArgument, false).first;

            // If it was a ctcp action, build an action string
            if (ctcpCommand == QLatin1String("action") && isChan)
            {
                if (!isIgnore(prefix, Ignore::Channel))
                {
                    Channel* channel = m_server->getChannelByName( parameterList.value(0) );

                    if (!channel) {
                        qCCritical(KONVERSATION_LOG) << "Didn't find the channel " << parameterList.value(0);
                        return;
                    }

                    channel->appendAction(sourceNick, ctcpArgument, messageTags);

                    if (sourceNick != m_server->getNickname())
                    {
                        const QRegularExpression re(QLatin1String("(^|[^\\d\\w])")
                                                    + QRegularExpression::escape(m_server->loweredNickname())
                                                    + QLatin1String("([^\\d\\w]|$)"));
                        if (hasArg && ctcpArgument.toLower().contains(re))
                        {
                            konv_app->notificationHandler()->nick(channel, sourceNick, ctcpArgument);
                        }
                        else
                        {
                            konv_app->notificationHandler()->message(channel, sourceNick, ctcpArgument);
                        }
                    }
                }
            }
            // If it was a ctcp action, build an action string
            else if (ctcpCommand == QLatin1String("action") && !isChan)
            {
                // Check if we ignore queries from this nick
                if (!isIgnore(prefix, Ignore::Query))
                {
                    NickInfoPtr nickinfo = m_server->obtainNickInfo(sourceNick);
                    nickinfo->setHostmask(sourceHostmask);

                    // create new query (server will check for dupes)
                    Query* query = m_server->addQuery(nickinfo, false /* we didn't initiate this*/ );

                    // send action to query
                    query->appendAction(sourceNick, ctcpArgument, messageTags);

                    if (sourceNick != m_server->getNickname() && query)
                        konv_app->notificationHandler()->queryMessage(query, sourceNick, ctcpArgument);
                }
            }

            // Answer ping requests
            else if (ctcpCommand == QLatin1String("ping") && hasArg)
            {
                if (!isIgnore(prefix,Ignore::CTCP))
                {
                    if (isChan)
                    {
                        m_server->appendMessageToFrontmost(i18n("CTCP"),
                            i18n("Received CTCP-PING request from %1 to channel %2, sending answer.",
                                 sourceNick, parameterList.value(0)),
                            messageTags
                            );
                    }
                    else
                    {
                        m_server->appendMessageToFrontmost(i18n("CTCP"),
                            i18n("Received CTCP-%1 request from %2, sending answer.",
                                 QStringLiteral("PING"), sourceNick),
                            messageTags
                            );
                    }
                    m_server->ctcpReply(sourceNick, QStringLiteral("PING %1").arg(ctcpArgument));
                }
            }

            // Maybe it was a version request, so act appropriately
            else if (ctcpCommand == QLatin1String("version"))
            {
                if(!isIgnore(prefix,Ignore::CTCP))
                {
                    if (isChan)
                    {
                        m_server->appendMessageToFrontmost(i18n("CTCP"),
                            i18n("Received Version request from %1 to channel %2.",
                                 sourceNick, parameterList.value(0)),
                            messageTags
                            );
                    }
                    else
                    {
                        m_server->appendMessageToFrontmost(i18n("CTCP"),
                            i18n("Received Version request from %1.",
                                 sourceNick),
                            messageTags
                            );
                    }

                    QString reply;
                    if (Preferences::self()->customVersionReplyEnabled())
                    {
                        reply = Preferences::self()->customVersionReply().trimmed();
                    }
                    else
                    {
                        // Do not internationalize the below version string
                        reply = QStringLiteral("Konversation %1 Copyright 2002-2020 by the Konversation team")
                            .arg(QStringLiteral(KONVI_VERSION_STRING));
                    }

                    if (!reply.isEmpty())
                        m_server->ctcpReply(sourceNick,QStringLiteral("VERSION ")+reply);
                }
            }
            // DCC request?
            else if (ctcpCommand==QStringLiteral("dcc") && !isChan && hasArg)
            {
                if (!isIgnore(prefix,Ignore::DCC))
                {
                    // Extract DCC type and argument list
                    QString dccType=ctcpArgument.toLower().section(QLatin1Char(' '),0,0);

                    // Support file names with spaces
                    QString dccArguments = ctcpArgument.mid(ctcpArgument.indexOf(QLatin1Char(' '))+1);
                    QStringList dccArgumentList;

                    if ((dccArguments.count(QLatin1Char('\"')) >= 2) && (dccArguments.startsWith(QLatin1Char('\"'))))
                    {
                        int lastQuotePos = dccArguments.lastIndexOf(QLatin1Char('\"'));
                        if (dccArguments[lastQuotePos+1] == QLatin1Char(' '))
                        {
                            QString fileName = dccArguments.mid(1, lastQuotePos-1);
                            dccArguments = dccArguments.mid(lastQuotePos+2);

                            dccArgumentList.append(fileName);
                        }
                    }
                    dccArgumentList += dccArguments.split(QLatin1Char(' '), Qt::SkipEmptyParts);

                    if (dccType==QStringLiteral("send"))
                    {
                        if (dccArgumentList.count()==4)
                        {
                            // incoming file
                            konv_app->notificationHandler()->dccIncoming(m_server->getStatusView(), sourceNick);
                            Q_EMIT addDccGet(sourceNick,dccArgumentList);
                        }
                        else if (dccArgumentList.count() >= 5)
                        {
                            if (dccArgumentList[dccArgumentList.size() - 3] == QLatin1Char('0'))
                            {
                                // incoming file (Reverse DCC)
                                konv_app->notificationHandler()->dccIncoming(m_server->getStatusView(), sourceNick);
                                Q_EMIT addDccGet(sourceNick,dccArgumentList);
                            }
                            else
                            {
                                // the receiver accepted the offer for Reverse DCC
                                Q_EMIT startReverseDccSendTransfer(sourceNick,dccArgumentList);
                            }
                        }
                        else
                        {
                            m_server->appendMessageToFrontmost(i18n("DCC"),
                                i18n("Received invalid DCC SEND request from %1.",
                                     sourceNick),
                                messageTags
                                );
                        }
                    }
                    else if (dccType==QStringLiteral("accept"))
                    {
                        // resume request was accepted
                        if (dccArgumentList.count() >= 3)
                        {
                            Q_EMIT resumeDccGetTransfer(sourceNick,dccArgumentList);
                        }
                        else
                        {
                            m_server->appendMessageToFrontmost(i18n("DCC"),
                                i18n("Received invalid DCC ACCEPT request from %1.",
                                     sourceNick),
                                messageTags
                                );
                        }
                    }
                    // Remote client wants our sent file resumed
                    else if (dccType==QStringLiteral("resume"))
                    {
                        if (dccArgumentList.count() >= 3)
                        {
                            Q_EMIT resumeDccSendTransfer(sourceNick,dccArgumentList);
                        }
                        else
                        {
                            m_server->appendMessageToFrontmost(i18n("DCC"),
                                i18n("Received invalid DCC RESUME request from %1.",
                                     sourceNick),
                                messageTags
                                );
                        }
                    }
                    else if (dccType==QStringLiteral("chat"))
                    {
                        if (dccArgumentList.count() == 3)
                        {
                            // incoming chat
                            Q_EMIT addDccChat(sourceNick,dccArgumentList);
                        }
                        else if (dccArgumentList.count() == 4)
                        {
                            if (dccArgumentList[dccArgumentList.size() - 2] == QLatin1Char('0'))
                            {
                                // incoming chat (Reverse DCC)
                                Q_EMIT addDccChat(sourceNick,dccArgumentList);
                            }
                            else
                            {
                                // the receiver accepted the offer for Reverse DCC chat
                                Q_EMIT startReverseDccChat(sourceNick,dccArgumentList);
                            }
                        }
                        else
                        {
                            m_server->appendMessageToFrontmost(i18n("DCC"),
                                                             i18n("Received invalid DCC CHAT request from %1.",
                                                             sourceNick), messageTags
                                );
                        }
                    }
                    else
                    {
                        m_server->appendMessageToFrontmost(i18n("DCC"),
                            i18n("Unknown DCC command %1 received from %2.",
                                 ctcpArgument, sourceNick), messageTags
                            );
                    }
                }
            }
            else if (ctcpCommand==QStringLiteral("clientinfo") && !isChan)
            {
                if (!isIgnore(prefix, Ignore::CTCP))
                {
                    m_server->appendMessageToFrontmost(i18n("CTCP"),
                        i18n("Received CTCP-%1 request from %2, sending answer.",
                            QStringLiteral("CLIENTINFO"), sourceNick), messageTags
                        );
                    m_server->ctcpReply(sourceNick, QStringLiteral("CLIENTINFO ACTION CLIENTINFO DCC PING TIME VERSION"));
                }
            }
            else if (ctcpCommand==QStringLiteral("time") && !isChan)
            {
                if (!isIgnore(prefix, Ignore::CTCP))
                {
                    m_server->appendMessageToFrontmost(i18n("CTCP"),
                        i18n("Received CTCP-%1 request from %2, sending answer.",
                            QStringLiteral("TIME"), sourceNick), messageTags
                        );
                    m_server->ctcpReply(sourceNick, QStringLiteral("TIME ")+QDateTime::currentDateTime().toString());
                }
            }

            // No known CTCP request, give a general message
            else
            {
                if (!isIgnore(prefix,Ignore::CTCP))
                {
                    if (isChan)
                        m_server->appendServerMessageToChannel(
                            parameterList.value(0),
                            QStringLiteral("CTCP"),
                            i18n("Received unknown CTCP-%1 request from %2 to Channel %3.",
                                 ctcp, sourceNick, parameterList.value(0)),
                            messageTags
                            );
                    else
                        m_server->appendMessageToFrontmost(i18n("CTCP"),
                            i18n("Received unknown CTCP-%1 request from %2.",
                                 ctcp, sourceNick),
                            messageTags
                            );
                }
            }
        }
        // No CTCP, so it's an ordinary channel or query message
        else
        {
            parsePrivMsg(prefix, parameterList, messageTags);
        }
    }
    else if (command==QStringLiteral("notice") && plHas(2))
    {
        if (!isIgnore(prefix,Ignore::Notice))
        {
            // Channel notice?
            if(isAChannel(parameterList.value(0)))
            {
                if (m_server->identifyMsg() && (trailing.length() > 1 && (trailing.at(0) == QLatin1Char('+') || trailing.at(0) == QLatin1Char('-'))))
                {
                    trailing.remove(0, 1);
                }

                m_server->appendServerMessageToChannel(parameterList.value(0), i18n("Notice"),
                        i18n("-%1 to %2- %3", sourceNick, parameterList.value(0), trailing), messageTags
                    );
            }
            // Private notice
            else
            {
                // Was this a CTCP reply?
                if (!trailing.isEmpty() && trailing.at(0) == QChar(0x01))
                {
                    // cut 0x01 bytes from trailing string
                    QString ctcp(trailing.mid(1,trailing.length()-2));
                    QString replyReason(ctcp.section(QLatin1Char(' '),0,0));
                    QString reply(ctcp.section(QLatin1Char(' '),1));

                    // pong reply, calculate turnaround time
                    if (replyReason.toLower()==QStringLiteral("ping"))
                    {
                        int dateArrived=QDateTime::currentDateTime().toSecsSinceEpoch();
                        int dateSent=reply.toInt();
                        int time = dateArrived-dateSent;
                        QString unit = i18np("second", "seconds", time);

                        m_server->appendMessageToFrontmost(i18n("CTCP"),
                            i18n("Received CTCP-PING reply from %1: %2 %3.",
                                 sourceNick, time, unit), messageTags
                            );
                    }
                    else if (replyReason.toLower() == QLatin1String("dcc"))
                    {
                        qCDebug(KONVERSATION_LOG) << reply;
                        QStringList dccList = reply.split(QLatin1Char(' '));

                        //all dcc notices we receive are rejects
                        if (dccList.count() >= 2 && dccList.first().toLower() == QLatin1String("reject"))
                        {
                            dccList.removeFirst();
                            if (dccList.count() >= 2 && dccList.first().toLower() == QLatin1String("send"))
                            {
                                dccList.removeFirst();
                                Q_EMIT rejectDccSendTransfer(sourceNick,dccList);
                            }
                            else if (dccList.first().toLower() == QLatin1String("chat"))
                            {
                                Q_EMIT rejectDccChat(sourceNick);
                            }
                        }
                    }
                    // all other ctcp replies get a general message
                    else
                    {
                        m_server->appendMessageToFrontmost(i18n("CTCP"),
                            i18n("Received CTCP-%1 reply from %2: %3.",
                                 replyReason, sourceNick, reply), messageTags
                            );
                    }
                }
                // No, so it was a normal notice
                else
                {

                    #if HAVE_QCA2
                    //Key exchange
                    if (trailing.startsWith(QLatin1String("DH1080_INIT ")))
                    {
                        m_server->appendMessageToFrontmost(i18n("Notice"), i18n("Received DH1080_INIT from %1", sourceNick), messageTags);
                        m_server->parseInitKeyX(sourceNick, trailing.mid(12));
                    }
                    else if (trailing.startsWith(QLatin1String("DH1080_FINISH ")))
                    {
                        m_server->appendMessageToFrontmost(i18n("Notice"), i18n("Received DH1080_FINISH from %1", sourceNick), messageTags);
                        m_server->parseFinishKeyX(sourceNick, trailing.mid(14));
                    }
                    else
                    {
                    #endif
                        m_server->appendMessageToFrontmost(i18n("Notice"), i18n("-%1- %2", sourceNick,
                            m_server->identifyMsg() ? trailing.mid(1) : trailing), messageTags);
                    #if HAVE_QCA2
                    }
                    #endif
                }
            }
        }
    }
    else if (command==QStringLiteral("join") && plHas(1))
    {
        QString channelName;
        QString account;
        QString realName;

        channelName = parameterList[0];

        if (m_server->capabilities() & Server::ExtendedJoin && plHas(3))
        {
            if (parameterList[1] != QLatin1String("*"))
                account = parameterList[1];

            realName = parameterList[2];
        }

        // Did we join the channel, or was it someone else?
        if (m_server->isNickname(sourceNick))
        {
            /*
                QString key;
                // TODO: Try to remember channel keys for autojoins and manual joins, so
                //       we can get %k to work

                if (channelName.contains(QLatin1Char(' '))) {
                    key = channelName.section(QLatin1Char(' '),1,1);
                    channelName = channelName.section(QLatin1Char(' '),0,0);
                }
            */

            // Join the channel
            Channel* channel = m_server->joinChannel(channelName, sourceHostmask, messageTags);

            // Upon JOIN we're going to receive some NAMES input from the server which
            // we need to be able to tell apart from manual invocations of /names
            setAutomaticRequest(QStringLiteral("NAMES"),channelName,true);

            channel->clearModeList();

            // Request modes for the channel
            m_server->queue(QStringLiteral("MODE ")+channelName, Server::LowPriority);

            // Initiate channel ban list
            channel->clearBanList();
            setAutomaticRequest(QStringLiteral("BANLIST"),channelName,true);
            m_server->queue(QStringLiteral("MODE ")+channelName+QStringLiteral(" +b"), Server::LowPriority);
        }
        else
        {
            Channel* channel = m_server->nickJoinsChannel(channelName, sourceNick, sourceHostmask, account, realName, messageTags);
            konv_app->notificationHandler()->join(channel, sourceNick);
        }
    }
    else if (command==QStringLiteral("kick") && plHas(2))
    {
        m_server->nickWasKickedFromChannel(parameterList.value(0), parameterList.value(1), sourceNick, trailing, messageTags);
    }
    else if (command==QStringLiteral("part") && plHas(1))
    {
        // A version of the PART line encountered on ircu: ":Nick!user@host PART :#channel"

        QString channel(parameterList.value(0));
        QString reason(parameterList.value(1));

        Channel* channelPtr = m_server->removeNickFromChannel(channel, sourceNick, reason, messageTags);

        if (sourceNick != m_server->getNickname())
        {
            konv_app->notificationHandler()->part(channelPtr, sourceNick);
        }
    }
    else if (command==QStringLiteral("quit") && plHas(1))
    {
        m_server->removeNickFromServer(sourceNick, trailing, messageTags);
        if (sourceNick != m_server->getNickname())
        {
            konv_app->notificationHandler()->quit(m_server->getStatusView(), sourceNick);
        }
    }
    else if (command==QStringLiteral("nick") && plHas(1))
    {
        QString newNick(parameterList.value(0)); // Message may not include ":" in front of the new nickname

        m_server->renameNick(sourceNick, newNick, messageTags);

        if (sourceNick != m_server->getNickname())
        {
            konv_app->notificationHandler()->nickChange(m_server->getStatusView(), sourceNick, newNick);
        }
    }
    else if (command==QStringLiteral("topic") && plHas(2))
    {
        m_server->setChannelTopic(sourceNick, parameterList.value(0), trailing, messageTags);
    }
    else if (command==QStringLiteral("mode") && plHas(2)) // mode #channel -/+ mmm params
    {
        parseModes(sourceNick, parameterList, messageTags);
        Channel* channel = m_server->getChannelByName(parameterList.value(0));
        konv_app->notificationHandler()->mode(channel, sourceNick, parameterList.value(0),
            QStringList(parameterList.mid(1)).join(QLatin1Char(' ')));
    }
    else if (command==QStringLiteral("invite") && plHas(2)) //:ejm!i=beezle@bas5-oshawa95-1176455927.dsl.bell.ca INVITE argnl :#sug4
    {
        if (!isIgnore(prefix, Ignore::Invite))
        {
            QString channel(trailing);

            m_server->appendMessageToFrontmost(i18n("Invite"),
                i18n("%1 invited you to channel %2.", sourceNick, channel), messageTags
                );
            Q_EMIT invitation(sourceNick, channel);
        }
    }
    else if (command == QLatin1String("away") && plHas(0))
    {
        NickInfoPtr nickInfo = m_server->getNickInfo(sourceNick);

        if (nickInfo)
        {
            if (!parameterList.isEmpty())
            {
                nickInfo->setAway(true);
                nickInfo->setAwayMessage(parameterList.first());
            }
            else
            {
                nickInfo->setAway(false);
                nickInfo->setAwayMessage(QString());
            }
        }
        else
        {
            qCDebug(KONVERSATION_LOG) << "Received away message for unknown nick," << sourceNick;
        }
    }
    else if (command == QLatin1String("account") && plHas(1))
    {
        NickInfoPtr nickInfo = m_server->getNickInfo(sourceNick);
        if (nickInfo) {
            const QString account = parameterList.at(0);
            nickInfo->setAccount(account == QLatin1Char('*') ? QString() : account);
        }
    }
    else if (command == QLatin1String("chghost") && plHas(2))
    {
        NickInfoPtr nickInfo = m_server->getNickInfo(sourceNick);

        if (nickInfo)
        {
            const QString user = parameterList.value(0);
            const QString host = parameterList.value(1);
            nickInfo->setHostmask(QStringLiteral("%1@%2").arg(user, host));
        }
    }
    else
    {
        qCDebug(KONVERSATION_LOG) << "unknown client command" << parameterList.count() << _plHad << _plWanted << command << parameterList.join(QLatin1Char(' '));
        m_server->appendMessageToFrontmost(command, parameterList.join(QLatin1Char(' ')), messageTags);
    }
}

void InputFilter::parseServerCommand(const QString &prefix, const QString &command, QStringList &parameterList, const QHash<QString, QString> &messageTags)
{
    bool isNumeric;
    int numeric = command.toInt(&isNumeric);

    Q_ASSERT(m_server);
    if (!m_server)
        return;

    if (!isNumeric)
    {
        if (command == QLatin1String("ping"))
        {
            QString text;
            text = (!trailing.isEmpty()) ? trailing : parameterList.join(QLatin1Char(' '));

            if (!trailing.isEmpty())
            {
                text = prefix + QLatin1String(" :") + text;
            }

            if (!text.startsWith(QLatin1Char(' ')))
            {
                text.prepend(QLatin1Char(' '));
            }

            // queue the reply to send it as soon as possible
            m_server->queue(QStringLiteral("PONG")+text, Server::HighPriority);

        }
        else if (command == QLatin1String("error :closing link:"))
        {
            qCDebug(KONVERSATION_LOG) << "link closed";
        }
        else if (command == QLatin1String("pong"))
        {
            // double check if we are in lag measuring mode since some servers fail to send
            // the LAG cookie back in PONG
            if (trailing.startsWith(QLatin1String("LAG")) || getLagMeasuring())
            {
                m_server->pongReceived();
            }
        }
        else if (command == QLatin1String("mode"))
        {
            parseModes(prefix, parameterList, messageTags);
        }
        else if (command == QLatin1String("notice"))
        {
            m_server->appendStatusMessage(i18n("Notice"), i18n("-%1- %2", prefix, trailing), messageTags);
        }
        else if (command == QLatin1String("kick") && plHas(3))
        {
            m_server->nickWasKickedFromChannel(parameterList.value(1), parameterList.value(2), prefix, trailing, messageTags);
        }
        else if (command == QLatin1String("privmsg"))
        {
            parsePrivMsg(prefix, parameterList, messageTags);
        }
        else if (command==QStringLiteral("cap") && plHas(3))
        {
            QString command = parameterList.value(1).toLower();

            if (command == QLatin1String("ack") || command == QLatin1String("nak"))
            {
                m_server->capReply();

                const QStringList capabilities = parameterList.value(2).split(QLatin1Char(' '), Qt::SkipEmptyParts);

                const QRegularExpression re(QStringLiteral("[a-z0-9]"), QRegularExpression::CaseInsensitiveOption);
                for (const QString& capability : capabilities) {
                    const int nameStart = capability.indexOf(re);
                    QString modifierString = capability.left(nameStart);
                    QString name = capability.mid(nameStart);

                    Server::CapModifiers modifiers = Server::NoModifiers;

                    if (modifierString.contains(QLatin1Char('-')))
                    {
                        modifiers = modifiers | Server::DisMod;
                        modifiers = modifiers ^ Server::NoModifiers;
                    }

                    if (modifierString.contains(QLatin1Char('=')))
                    {
                        modifiers = modifiers | Server::StickyMod;
                        modifiers = modifiers ^ Server::NoModifiers;
                    }

                    if (modifierString.contains(QLatin1Char('~')))
                    {
                        modifiers = modifiers | Server::AckMod;
                        modifiers = modifiers ^ Server::NoModifiers;
                    }

                    if (command == QLatin1String("ack"))
                        m_server->capAcknowledged(name, modifiers);
                    else
                        m_server->capDenied(name);
                }

                if(!m_server->capEndDelayed())
                {
                    m_server->capEndNegotiation();
                }
            }
            else if (command == QLatin1String("ls") || command == QLatin1String("list"))
            {
                m_server->appendStatusMessage(i18n("Capabilities"), trailing, messageTags);

                if (getAutomaticRequest(QStringLiteral("CAP LS"), QString()) != 0)
                {
                    if (parameterList.count() == 3)
                        setAutomaticRequest (QStringLiteral("CAP LS"), QString (), false);
                    m_server->capInitiateNegotiation (trailing);
                }
            }
            else if (command == QLatin1String("new"))
            {
                m_server->capInitiateNegotiation(trailing);
            }
            else if (command == QLatin1String("del"))
            {
                m_server->capDel(trailing);
            }
        }
        else if (command == QLatin1String("authenticate") && plHas(1))
        {
            if ((m_server->getLastAuthenticateCommand() == QLatin1String("PLAIN")
                || m_server->getLastAuthenticateCommand() == QLatin1String("EXTERNAL"))
                && parameterList.value(0) == QLatin1String("+"))
                m_server->registerWithServices();
        }
        // All yet unknown messages go into the frontmost window unaltered
        else
        {
            qCDebug(KONVERSATION_LOG) << "unknown server command" << command;
            m_server->appendMessageToFrontmost(command, parameterList.join(QLatin1Char(' ')), messageTags);
        }
    }
    else if (plHas(2)) //[0]==ourNick, [1] needs to be *something*
    {
        parseNumeric(prefix, numeric, parameterList, messageTags);
    } // end of numeric elseif
    else
    {
        qCDebug(KONVERSATION_LOG) << "unknown message format" << parameterList.count() << _plHad << _plWanted << command << parameterList.join(QLatin1Char(' '));
    }
} // end of server

void InputFilter::parseModes(const QString &sourceNick, const QStringList &parameterList, const QHash<QString, QString> &messageTags)
{
    const QString modestring=parameterList.value(1);

    if (!isAChannel(parameterList.value(0)))
    {
        QString message;
        if (parameterList.value(0) == m_server->getNickname())
        {
            if (sourceNick == m_server->getNickname())
            { //XXX someone might care about the potentially unnecessary plural here
                message = i18n("You have set personal modes: %1", modestring);
            }
            else
            { //XXX someone might care about the potentially unnecessary plural here
                message = i18n("%1 has changed your personal modes: %2", sourceNick, modestring);
            }
        }
        if (!message.isEmpty())
            m_server->appendStatusMessage(i18n("Mode"), message, messageTags);
        return;
    }

    bool plus=false;
    int parameterIndex=0;
    // List of modes that need a parameter (note exception with -k and -l)
    // Mode q is quiet on freenode and acts like b... if this is a channel mode on other
    //  networks then more logic is needed here. --MrGrim
    //FIXME: The assumptions being made here about which modes have parameters and
    // which don't strike me as very wrong, as some of these mode chars are used
    // both as user and channel modes (e.g. A and O) and whether they use a para-
    // meter can vary between those cases. This would seem to lead to trouble with
    // the list indices down there. --hein, Thu Aug 6 19:48:02 CEST 2009 / r1008015
    QString parameterModes = QStringLiteral("aAoOvhkbleIq");
    QString message = i18n("%1 sets mode: %2", sourceNick, modestring);

    for (const QChar m : modestring) {
        const char mode = m.toLatin1();
        QString parameter;

        // Check if this is a mode or a +/- qualifier
        if (mode=='+' || mode=='-')
        {
            plus=(mode=='+');
        }
        else
        {
            // Check if this was a parameter mode
            if (parameterModes.contains(QLatin1Char(mode)))
            {
                // Check if the mode actually wants a parameter. -k and -l do not!
                if (plus || (!plus && (mode!='k') && (mode!='l')))
                {
                    // Remember the mode parameter
                    parameter = parameterList.value(2+parameterIndex);
                    message += QLatin1Char(' ') + parameter;
                    // Switch to next parameter
                    ++parameterIndex;
                }
            }
            // Let the channel update its modes
            if (parameter.isEmpty())               // XXX Check this to ensure the braces are in the correct place
            {
                qCDebug(KONVERSATION_LOG)   << "in updateChannelMode.  sourceNick: '" << sourceNick << "'  parameterlist: '"
                    << parameterList.join(QLatin1String(", ")) << "'";
            }
            m_server->updateChannelMode(sourceNick, parameterList.value(0), mode, plus, parameter, messageTags);
        }
    } // endfor

    if (Preferences::self()->useLiteralModes())
    {
        m_server->appendCommandMessageToChannel(parameterList.value(0), i18n("Mode"), message, messageTags);
    }
}

// # & + and ! are *often*, but not necessarily, Channel identifiers. + and ! are non-RFC,
// so if a server doesn't offer 005 and supports + and ! channels, I think thats broken behaviour
// on their part - not ours. --Argonel
bool InputFilter::isAChannel(const QString &check) const
{
    if (check.isEmpty())
        return false;
    Q_ASSERT(m_server);
    // if we ever see the assert, we need the ternary
    return m_server? m_server->isAChannel(check) : bool(QStringLiteral("#&").contains(check.at(0)));
}

bool InputFilter::isIgnore(const QString &sender, Ignore::Type type) const
{
    bool doIgnore = false;

    QRegularExpression ignoreRe;
    ignoreRe.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    const auto ignoreList = Preferences::ignoreList();
    for (Ignore* item : ignoreList) {
        ignoreRe.setPattern(QRegularExpression::anchoredPattern(QRegularExpression::escape(
                                item->getName()).replace(QLatin1String("\\*"), QLatin1String("(.*)"))));

        if (ignoreRe.match(sender).hasMatch()) {
            if (item->getFlags() & type) {
                doIgnore = true;
            }

            if (item->getFlags() & Ignore::Exception) {
                return false;
            }
        }
    }

    return doIgnore;
}

void InputFilter::reset()
{
    m_automaticRequest.clear();
    m_whoRequestList.clear();
}

void InputFilter::setAutomaticRequest(const QString& command, const QString& name, bool yes)
{
    m_automaticRequest[command][name.toLower()] += (yes) ? 1 : -1;
    if(m_automaticRequest[command][name.toLower()]<0)
    {
        qCDebug(KONVERSATION_LOG)   << "( " << command << ", " << name
            << " ) was negative! Resetting!";
        m_automaticRequest[command][name.toLower()]=0;
    }
}

int InputFilter::getAutomaticRequest(const QString& command, const QString& name) const
{
    return m_automaticRequest[command][name.toLower()];
}

void InputFilter::addWhoRequest(const QString& name) { m_whoRequestList << name.toLower(); }

bool InputFilter::isWhoRequestUnderProcess(const QString& name) const { return m_whoRequestList.contains(name.toLower()); }

void InputFilter::setLagMeasuring(bool state) { m_lagMeasuring=state; }

bool InputFilter::getLagMeasuring() const     { return m_lagMeasuring; }

void InputFilter::parsePrivMsg(const QString& prefix, QStringList& parameterList, const QHash<QString, QString> &messageTags)
{
    int pos = prefix.indexOf(QLatin1Char('!'));
    QString source;
    QString sourceHostmask;
    QString message(trailing);

    if(pos > 0)
    {
        source = prefix.left(pos);
        sourceHostmask = prefix.mid(pos + 1);
    }
    else
    {
        source = prefix;
    }

    Application* konv_app = Application::instance();
    message = konv_app->doAutoreplace(message, false).first;

    const QRegularExpression regexp(QLatin1String("(^|[^\\d\\w])")
                                    + QRegularExpression::escape(m_server->loweredNickname())
                                    + QLatin1String("([^\\d\\w]|$)"),
                                    QRegularExpression::CaseInsensitiveOption);
    if(isAChannel(parameterList.value(0)))
    {
        if(!isIgnore(prefix, Ignore::Channel))
        {
            Channel* channel = m_server->getChannelByName(parameterList.value(0));
            if(channel)
            {
                QString label;

                if (m_server->getServerNickPrefixes().contains(parameterList.value(0).at(0)))
                {
                    label = parameterList.value(0);
                }

                channel->append(source, message, messageTags, label);

                if(source != m_server->getNickname())
                {
                    if(message.contains(regexp))
                    {
                        konv_app->notificationHandler()->nick(channel,
                                source, message);
                    }
                    else
                    {
                        konv_app->notificationHandler()->message(channel,
                                source, message);
                    }
                }
            }
        }
    }
    else
    {
        if(!isIgnore(prefix,Ignore::Query))
        {
            QString queryName = source;

            // Handle znc.in/self-message correctly
            if (source == m_server->getNickname())
                queryName = parameterList[0];

            NickInfoPtr nickinfo = m_server->obtainNickInfo(queryName);

            if (queryName == source)
                nickinfo->setHostmask(sourceHostmask);

            // Create a new query (server will check for dupes)
            Query* query = m_server->addQuery(nickinfo, false /*we didn't initiate this*/ );

            // send action to query
            query->appendQuery(source, message, messageTags);

            if(source != m_server->getNickname() && query)
            {
                if(message.contains(regexp))
                {
                    konv_app->notificationHandler()->nick(query,
                            source, message);
                }
                else
                {
                    konv_app->notificationHandler()->queryMessage(query,
                            source, message);
                }
            }
        }
    }
}

QHash<QString, QString> InputFilter::parseMessageTags(const QString &line, int *startOfMessage)
{
    int index = line.indexOf(QLatin1Char(' '));
    *startOfMessage = index + 1;
    const QStringList tags = line.mid(1, index - 1).split(QLatin1Char(';'));
    QHash<QString, QString> tagHash;

    for (const QString &tag : tags) {
        QStringList tagList = tag.split(QLatin1Char('='));
        tagHash.insert(tagList.first(), tagList.last());
    }

    return tagHash;
}

void InputFilter::parseNumeric(const QString &prefix, int command, QStringList &parameterList, const QHash<QString, QString> &messageTags)
{
    //:niven.freenode.net 353 argnl @ #konversation :@argonel psn @argnl bugbot pinotree CIA-13
    //QString m_serverAssignedNick(parameterList.takeFirst());
    QString m_serverAssignedNick(parameterList.first());

    switch (command)
    {
        case RPL_WELCOME:
        case RPL_YOURHOST:
        case RPL_CREATED:
        {
            if (plHas(0)) //make the script happy
            {
                if (command == RPL_WELCOME)
                {
                    QString host;

                    if (trailing.contains(QLatin1Char('@')))
                        host = trailing.section(QLatin1Char('@'), 1);

                    // re-set nickname, since the server may have truncated it
                    if (m_serverAssignedNick != m_server->getNickname())
                    {
                        m_server->renameNick(m_server->getNickname(), m_serverAssignedNick, messageTags);
                    }

                    // Send the welcome signal, so the server class knows we are connected properly
                    Q_EMIT welcome(host);
                    m_connecting = true;
                }
                m_server->appendStatusMessage(i18n("Welcome"), trailing, messageTags);
            }
            break;
        }
        case RPL_MYINFO:
        {
            if (plHas(5))
            {
                m_server->appendStatusMessage(i18n("Welcome"),
                    i18n("Server %1 (Version %2), User modes: %3, Channel modes: %4",
                     parameterList.value(1),
                     parameterList.value(2),
                     parameterList.value(3),
                     parameterList.value(4)),
                    messageTags
                    );

                const QString allowed = m_server->allowedChannelModes();
                QString newModes = parameterList.value(4);
                // attempt to merge the two
                for (const QChar a : allowed) {
                    if (!newModes.contains(a))
                        newModes.append(a);
                }
                m_server->setAllowedChannelModes(newModes);
            }
            break;
        }
        //case RPL_BOUNCE:   // RFC 1459 name, now seems to be obsoleted by ...
        case RPL_ISUPPORT:                    // ... DALnet RPL_ISUPPORT
        {
            if (plHas(0)) //make the script happy
            {
                m_server->appendStatusMessage(i18n("Support"), parameterList.join(QLatin1Char(' ')), messageTags);

            // The following behaviour is neither documented in RFC 1459 nor in 2810-2813
                // Nowadays, most ircds send server capabilities out via 005 (BOUNCE).
                // refer to http://www.irc.org/tech_docs/005.html for a kind of documentation.
                // More on http://www.irc.org/tech_docs/draft-brocklesby-irc-isupport-03.txt

                QStringList::const_iterator it = parameterList.constBegin();
                // don't want the user name
                ++it;
                for (; it != parameterList.constEnd(); ++it )
                {
                    QString property, value;
                    int pos;
                    if ((pos=(*it).indexOf( QLatin1Char('=') )) !=-1)
                    {
                        property = (*it).left(pos);
                        value = (*it).mid(pos+1);
                    }
                    else
                    {
                        property = *it;
                    }
                    if (property==QStringLiteral("PREFIX"))
                    {
                        pos = value.indexOf(QLatin1Char(')'),1);
                        if(pos==-1)
                        {
                            m_server->setPrefixes(QString(), value);
                            // XXX if ) isn't in the string, NOTHING should be there. anyone got a server
                            if (!value.isEmpty() || !property.isEmpty())
                                m_server->appendStatusMessage(QString(), QStringLiteral("XXX Server sent bad PREFIX in RPL_ISUPPORT, please report."), messageTags);
                        }
                        else
                        {
                            m_server->setPrefixes(value.mid(1, pos-1), value.mid(pos+1));
                        }
                    }
                    else if (property==QStringLiteral("CHANTYPES"))
                    {
                        m_server->setChannelTypes(value);
                    }
                    else if (property==QStringLiteral("MODES"))
                    {
                        if (!value.isEmpty())
                        {
                            bool ok = false;
                            // If a value is given, it must be numeric.
                            int modesCount = value.toInt(&ok, 10);
                            if(ok) m_server->setModesCount(modesCount);
                        }
                    }
                    else if (property == QLatin1String("CAPAB"))
                    {
                        // Disable as we don't use this for anything yet
                        //server->queue("CAPAB IDENTIFY-MSG");
                    }
                    else if (property == QLatin1String("CHANMODES"))
                    {
                        if(!value.isEmpty())
                        {
                            m_server->setChanModes(value);
                            const QString allowed = m_server->allowedChannelModes();
                            QString newModes = value.remove(QLatin1Char(','));
                            // attempt to merge the two
                            for (const QChar a : allowed) {
                                if (!newModes.contains(a))
                                    newModes.append(a);
                            }
                            m_server->setAllowedChannelModes(newModes);
                        }
                    }
                    else if (property == QLatin1String("TOPICLEN"))
                    {
                        if (!value.isEmpty())
                        {
                            bool ok =  false;
                            int topicLength = value.toInt(&ok);

                            if (ok)
                                m_server->setTopicLength(topicLength);
                        }
                    }
                    else if (property == QLatin1String("WHOX"))
                    {
                        m_server->setHasWHOX(true);
                     }
                    else
                    {
                        //qCDebug(KONVERSATION_LOG) << "Ignored server-capability: " << property << " with value '" << value << "'";
                    }
                }                                 // endfor
            }
            break;
        }
        case RPL_UMODEIS:
        {
            if (plHas(0))
            {
                // TODO check this one... I amputated  + ' '+trailing
                QString message = QStringLiteral("%1 %2").arg(i18n("Your personal modes are:"), parameterList.join(QLatin1Char(' ')).section(QLatin1Char(' '),1));
                m_server->appendMessageToFrontmost(QStringLiteral("Info"), message, messageTags);
            }
            break;
        }
        case RPL_CHANNELMODEIS:
        {
            if (plHas(2))
            {
                const QString modeString=parameterList.value(2); // TEST this one was a 2
                // This is the string the user will see
                QString modesAre;
                QString message = i18n("Channel modes: ") + modeString;
                int parameterCount=3;
                QHash<QChar,QString> channelModesHash = Konversation::getChannelModesHash();
                for (const QChar mode : modeString) {
                    QString parameter;

                    if (mode != QLatin1Char('+')) {
                        if(!modesAre.isEmpty())
                            modesAre+=QStringLiteral(", ");

                        if (mode == QLatin1Char('k')) {
                            parameter=parameterList.value(parameterCount++);
                            message += QLatin1Char(' ') + parameter;
                            modesAre+=i18n("password protected");
                        }
                        else if (mode == QLatin1Char('l')) {
                            parameter=parameterList.value(parameterCount++);
                            message += QLatin1Char(' ') + parameter;
                            modesAre+=i18np("limited to %1 user", "limited to %1 users", parameter.toInt());
                        }
                        else if (channelModesHash.contains(mode)) {
                            modesAre += channelModesHash.value(mode);
                        }
                        else
                        {
                            modesAre += mode;
                        }
                        m_server->updateChannelModeWidgets(parameterList.value(1), mode.toLatin1(), parameter);
                    }
                } // endfor
                if (!modesAre.isEmpty() && Preferences::self()->useLiteralModes())
                {
                    m_server->appendCommandMessageToChannel(parameterList.value(1), i18n("Mode"), message, messageTags);
                }
                else
                {
                    m_server->appendCommandMessageToChannel(parameterList.value(1), i18n("Mode"),
                        i18n("Channel modes: ") + modesAre, messageTags);
                }
            }
            break;
        }
        case RPL_CHANNELURLIS:
        {// :niven.freenode.net 328 argonel #channel :http://www.buggeroff.com/
            if (plHas(3))
            {
                m_server->appendCommandMessageToChannel(parameterList.value(1), i18n("URL"),
                    i18n("Channel URL: %1", trailing), messageTags);
            }
            break;
        }
        case RPL_CHANNELCREATED:
        {
            if (plHas(3))
            {
                QDateTime when;
                when.setMSecsSinceEpoch(static_cast<qint64>(parameterList.value(2).toInt()) * 1000);
                m_server->appendCommandMessageToChannel(parameterList.value(1), i18n("Created"),
                    i18n("This channel was created on %1.",
                        QLocale().toString(when, QLocale::ShortFormat)),
                    messageTags
                    );
            }
            break;
        }
        case RPL_WHOISACCOUNT:
        {
            if (plHas(2))
            {
                NickInfoPtr nickInfo = m_server->getNickInfo(parameterList.value(1));
                if (nickInfo)
                {
                    nickInfo->setIdentified(true);
                }
                // Display message only if this was not an automatic request.
                if (getAutomaticRequest(QStringLiteral("WHOIS"), parameterList.value(1)) == 0)
                {
                    m_server->appendMessageToFrontmost(i18n("Whois"), i18n("%1 is logged in as %2.", parameterList.value(1), parameterList.value(2)), messageTags);
                }
            }
            break;
        }
        //:niven.freenode.net 353 argnl @ #konversation :@argonel psn @argnl bugbot pinotree CIA-13
        case RPL_NAMREPLY:
        {
            if (plHas(4))
            {
                QStringList nickList;

                if (!trailing.isEmpty())
                {
                    nickList = trailing.split(QLatin1Char(' '), Qt::SkipEmptyParts);
                }
                else if (parameterList.count() > 3)
                {
                    nickList.reserve(parameterList.size() - 3);
                    for(int i = 3; i < parameterList.count(); i++) {
                        nickList.append(parameterList.value(i));
                    }
                }
                else
                {
                    qCDebug(KONVERSATION_LOG) << "Hmm seems something is broken... can't get to the names!";
                }

                // send list to channel
                m_server->queueNicks(parameterList.value(2), nickList); // TEST this was a 2

                // Display message only if this was not an automatic request.
                if (getAutomaticRequest(QStringLiteral("NAMES"), parameterList.value(2)) == 0)
                {
                    m_server->appendMessageToFrontmost(i18n("Names"), trailing, messageTags);
                }
            }
            break;
        }
        case RPL_ENDOFNAMES:
        {
            if (plHas(2))
            {
                if (getAutomaticRequest(QStringLiteral("NAMES"),parameterList.value(1)) != 0)
                {
                    // This code path was taken for the automatic NAMES input on JOIN, upcoming
                    // NAMES input for this channel will be manual invocations of /names
                    setAutomaticRequest(QStringLiteral("NAMES"), parameterList.value(1), false);
                }
                else
                {
                    m_server->appendMessageToFrontmost(i18n("Names"), i18n("End of NAMES list."), messageTags);
                }

                Q_EMIT endOfNames(parameterList.value(1));
            }
            break;
        }
        // Topic set messages
        case RPL_NOTOPIC:
        {
            if (plHas(2))
            {
                //this really has 3, but [2] is "No topic has been set"
                m_server->appendMessageToFrontmost(i18n("TOPIC"), i18n("The channel %1 has no topic set.", parameterList.value(1)), messageTags);
            }
            break;
        }
        case RPL_TOPIC:
        {
            if (plHas(3))
            {
                QString topic(trailing);

                // FIXME: This is an abuse of the automaticRequest system: We're
                // using it in an inverted manner, i.e. the automaticRequest is
                // set to true by a manual invocation of /topic. Bad bad bad -
                // needs rethinking of automaticRequest.
                if (getAutomaticRequest(QStringLiteral("TOPIC"), parameterList.value(1)) == 0)
                {
                    // Update channel window
                    m_server->setChannelTopic(parameterList.value(1), topic, messageTags);
                }
                else
                {
                    m_server->appendMessageToFrontmost(i18n("Topic"), i18n("The channel topic for %1 is: \"%2\"", parameterList.value(1), topic), messageTags);
                }
            }
            break;
        }
        case RPL_TOPICSETBY:
        {
            if (plHas(4))
            {
                // Inform user who set the topic and when
                QDateTime when;
                when.setMSecsSinceEpoch(static_cast<qint64>(parameterList.value(3).toInt()) * 1000);

                // See FIXME in RPL_TOPIC
                if (getAutomaticRequest(QStringLiteral("TOPIC"), parameterList.value(1)) == 0)
                {
                    m_server->appendCommandMessageToChannel(parameterList.value(1), i18n("Topic"),
                        i18n("The topic was set by %1 on %2.",
                        parameterList.value(2), QLocale().toString(when, QLocale::ShortFormat)),
                        messageTags,
                        false,
                        false);
                }
                else
                {
                    m_server->appendMessageToFrontmost(i18n("Topic"), i18n("The topic for %1 was set by %2 on %3.",
                        parameterList.value(1),
                        parameterList.value(2),
                        QLocale().toString(when, QLocale::ShortFormat)), messageTags,
                        false);
                    setAutomaticRequest(QStringLiteral("TOPIC"),parameterList.value(1), false);
                }
                Q_EMIT topicAuthor(parameterList.value(1), parameterList.value(2), when);
            }
            break;
        }
        case RPL_WHOISACTUALLY:
        {
            if (plHas(3))
            {
                // Display message only if this was not an automatic request.
                if (getAutomaticRequest(QStringLiteral("WHOIS"),parameterList.value(1)) == 0)
                {
                    m_server->appendMessageToFrontmost(i18n("Whois"), i18n("%1 is actually using the host %2.", parameterList.value(1), parameterList.value(2)), messageTags);
                }
            }
            break;
        }
        case ERR_NOSUCHNICK:
        {
            if (plHas(2))
            {
                // Display slightly different error message in case we performed a WHOIS for
                // IP resolve purposes, and clear it from the automaticRequest list
                if (getAutomaticRequest(QStringLiteral("DNS"), parameterList.value(1)) == 0)
                {
                    m_server->appendMessageToFrontmost(i18n("Error"), i18n("%1: No such nick/channel.", parameterList.value(1)), messageTags);
                }
                else if(getAutomaticRequest(QStringLiteral("WHOIS"), parameterList.value(1)) == 0) //Display message only if this was not an automatic request.
                {
                    m_server->appendMessageToFrontmost(i18n("Error"), i18n("No such nick: %1.", parameterList.value(1)), messageTags);
                    setAutomaticRequest(QStringLiteral("DNS"), parameterList.value(1), false);
                }
            }
            break;
        }
        case ERR_NOSUCHCHANNEL:
        {
            if (plHas(2))
            {
                // Display message only if this was not an automatic request.
                if (getAutomaticRequest(QStringLiteral("WHOIS"), parameterList.value(1)) == 0)
                {
                    m_server->appendMessageToFrontmost(i18n("Error"), i18n("%1: No such channel.", parameterList.value(1)), messageTags);
                }
            }
            break;
        }
        // Nick already on the server, so try another one
        case ERR_NICKNAMEINUSE:
        {
            if (plHas(1))
            {
                // if we are already connected, don't try tro find another nick ourselves
                if (m_server->isConnected()) // Show message
                {
                    m_server->appendMessageToFrontmost(i18n("Nick"), i18n("Nickname already in use, try a different one."), messageTags);
                }
                else // not connected yet, so try to find a nick that's not in use
                {
                    // Get the next nick from the list or ask for a new one
                    QString newNick = m_server->getNextNickname();

                    // The user chose to disconnect...
                    if (newNick.isNull())
                    {
                        if (m_server->isConnecting()) // ... or did they?
                            m_server->disconnectServer();
                         else // No they didn't!
                             m_server->appendMessageToFrontmost(i18n("Info"), i18n("The nickname %1 was already in use, but the connection failed before you responded.", m_server->getNickname()), messageTags);
                    }
                    else
                    {
                        // Update Server window
                        m_server->obtainNickInfo(m_server->getNickname()) ;
                        m_server->renameNick(m_server->getNickname(), newNick, messageTags);
                        // Show message
                        m_server->appendMessageToFrontmost(i18n("Nick"), i18n("Nickname already in use. Trying %1.", newNick), messageTags);
                        // Send nickchange request to the server
                        m_server->queue(QStringLiteral("NICK ")+newNick);
                    }
                }
            }
            break;
        }
        case ERR_ERRONEUSNICKNAME:
        {
            if (plHas(1))
            {
                if (m_server->isConnected())
                {                                 // We are already connected. Just print the error message
                    m_server->appendMessageToFrontmost(i18n("Nick"), trailing, messageTags);
                }
                else                              // Find a new nick as in ERR_NICKNAMEINUSE
                {
                    QString newNick = m_server->getNextNickname();

                    // The user chose to disconnect
                    if (newNick.isNull())
                    {
                        m_server->disconnectServer();
                    }
                    else
                    {
                        m_server->obtainNickInfo(m_server->getNickname()) ;
                        m_server->renameNick(m_server->getNickname(), newNick, messageTags);
                        m_server->appendMessageToFrontmost(i18n("Nick"), i18n("Erroneous nickname. Changing nick to %1.", newNick), messageTags);
                        m_server->queue(QStringLiteral("NICK ")+newNick);
                    }
                }
            }
            break;
        }
        case ERR_NOTONCHANNEL:
        {
            if (plHas(2))
            {
                m_server->appendMessageToFrontmost(i18n("Error"), i18n("You are not on %1.", parameterList.value(1)), messageTags);
                setAutomaticRequest(QStringLiteral("TOPIC"),parameterList.value(1), false);

            }
            break;
        }
        case RPL_MOTDSTART:
        {
            if (plHas(1))
            {
                if (!m_connecting || !Preferences::self()->skipMOTD())
                m_server->appendStatusMessage(i18n("MOTD"), i18n("Message of the day:"), messageTags);
            }
            break;
        }
        case RPL_MOTD:
        {
            if (plHas(2))
            {
                if (!m_connecting || !Preferences::self()->skipMOTD())
                    m_server->appendStatusMessage(i18n("MOTD"), trailing, messageTags);
            }
            break;
        }
        case RPL_ENDOFMOTD:
        {
            if (plHas(1))
            {
                if (!m_connecting || !Preferences::self()->skipMOTD())
                    m_server->appendStatusMessage(i18n("MOTD"), i18n("End of message of the day"), messageTags);

                if (m_connecting)
                    m_server->autoCommandsAndChannels();

                m_connecting = false;
            }
            break;
        }
        case ERR_NOMOTD:
        {
            if (plHas(1))
            {
                if (m_connecting)
                    m_server->autoCommandsAndChannels();

                m_connecting = false;
            }
            break;
        }
        case ERR_CHANOPRIVSNEEDED:
        {
            if (plHas(2))
            {
                m_server->appendMessageToFrontmost(i18n("Error"), i18n("You need to be a channel operator in %1 to do that.", parameterList.value(1)), messageTags);
            }
            break;
        }
        case RPL_YOUREOPER:
        {
            if (plHas(1))
            {
                m_server->appendMessageToFrontmost(i18n("Notice"), i18n("You are now an IRC operator on this server."), messageTags);
            }
            break;
        }
        case RPL_HOSTHIDDEN:
        {
            if (plHas(2))
            {
                m_server->appendStatusMessage(i18n("Info"), i18n("'%1' is now your hidden host (set by services).", parameterList.value(1)), messageTags);
            }
            break;
        }
        case RPL_GLOBALUSERS:                 // Current global users: 589 Max: 845
        {
            if (plHas(2))
            {
                QString current(trailing.section(QLatin1Char(' '),3));
                //QString max(trailing.section(QLatin1Char(' '),5,5));
                m_server->appendStatusMessage(i18n("Users"), i18n("Current users on the network: %1", current), messageTags);
            }
            break;
        }
        case RPL_LOCALUSERS:                  // Current local users: 589 Max: 845
        {
            if (plHas(2))
            {
                QString current(trailing.section(QLatin1Char(' '), 3));
                //QString max(trailing.section(QLatin1Char(' '),5,5));
                m_server->appendStatusMessage(i18n("Users"), i18n("Current users on %1: %2.", prefix, current), messageTags);
            }
            break;
        }
        case RPL_ISON:
        {
            if (plHas(2))
            {
                // Tell server to start the next notify timer round
                Q_EMIT notifyResponse(trailing);
            }
            break;
        }
        case RPL_AWAY:
        {
            if (plHas(3))
            {
                NickInfoPtr nickInfo = m_server->getNickInfo(parameterList.value(1));
                if (nickInfo)
                {
                    nickInfo->setAway(true);
                    if (nickInfo->getAwayMessage() != trailing) // FIXME i think this check should be in the setAwayMessage method
                    {
                        nickInfo->setAwayMessage(trailing);
                        // TEST this used to skip the automatic request handler below
                    }
                }

                if (getAutomaticRequest(QStringLiteral("WHOIS"), parameterList.value(1)) == 0)
                {
                    m_server->appendMessageToFrontmost(i18n("Away"),
                        i18n("%1 is away: %2", parameterList.value(1), trailing), messageTags
                        );
                }
            }
            break;
        }
        case RPL_INVITING:
        {
            if (plHas(3))
            {
                m_server->appendMessageToFrontmost(i18n("Invite"),
                        i18n("You invited %1 to channel %2.",
                        parameterList.value(1), parameterList.value(2)), messageTags
                    );
            }
            break;
        }
        //Sample WHOIS response
        //"/WHOIS psn"
        //[19:11] :zahn.freenode.net 311 PhantomsDad psn ~psn h106n2fls23o1068.bredband.comhem.se * :Peter Simonsson
        //[19:11] :zahn.freenode.net 319 PhantomsDad psn :#kde-devel #koffice
        //[19:11] :zahn.freenode.net 312 PhantomsDad psn irc.freenode.net :http://freenode.net/
        //[19:11] :zahn.freenode.net 301 PhantomsDad psn :away
        //[19:11] :zahn.freenode.net 320 PhantomsDad psn :is an identified user
        //[19:11] :zahn.freenode.net 317 PhantomsDad psn 4921 1074973024 :seconds idle, signon time
        //[19:11] :zahn.freenode.net 318 PhantomsDad psn :End of /WHOIS list.
        case RPL_WHOISUSER:
        {
            if (plHas(4))
            {
                NickInfoPtr nickInfo = m_server->getNickInfo(parameterList.value(1));
                if (nickInfo)
                {
                    nickInfo->setHostmask(i18n("%1@%2", parameterList.value(2), parameterList.value(3)));
                    nickInfo->setRealName(trailing);
                }
                // Display message only if this was not an automatic request.
                if (getAutomaticRequest(QStringLiteral("WHOIS"), parameterList.value(1)) == 0)
                {
                    // escape html tags
                    QString escapedRealName(trailing);

                    m_server->appendMessageToFrontmost(i18n("Whois"),
                        i18n("%1 is %2@%3 (%4)",
                            parameterList.value(1),
                            parameterList.value(2),
                            parameterList.value(3),
                            escapedRealName), messageTags, false);   // Don't parse any urls
                }
                else
                {
                    // This WHOIS was requested by Server for DNS resolve purposes; try to resolve the host
                    if (getAutomaticRequest(QStringLiteral("DNS"), parameterList.value(1)) != 0)
                    {
                        QHostInfo resolved = QHostInfo::fromName(parameterList.value(3));
                        if (resolved.error() == QHostInfo::NoError && !resolved.addresses().isEmpty())
                        {
                            QString ip = resolved.addresses().first().toString();
                            m_server->appendMessageToFrontmost(i18n("DNS"),
                                i18n("Resolved %1 (%2) to address: %3",
                                    parameterList.value(1),
                                    parameterList.value(3),
                                    ip), messageTags
                                );
                        }
                        else
                        {
                            m_server->appendMessageToFrontmost(i18n("Error"),
                                i18n("Unable to resolve address for %1 (%2)",
                                    parameterList.value(1),
                                    parameterList.value(3)), messageTags
                                );
                        }

                        // Clear this from the automaticRequest list so it works repeatedly
                        setAutomaticRequest(QStringLiteral("DNS"), parameterList.value(1), false);
                    }
                }
            }
            break;
        }
        // From a WHOIS.
        //[19:11] :zahn.freenode.net 320 PhantomsDad psn :is an identified user
        case RPL_WHOISIDENTIFY:
        case RPL_IDENTIFIED:
        {
            if (plHas(2))
            {
                NickInfoPtr nickInfo = m_server->getNickInfo(parameterList.value(1));
                if (nickInfo)
                {
                    nickInfo->setIdentified(true);
                }
                if (getAutomaticRequest(QStringLiteral("WHOIS"), parameterList.value(1)) == 0)
                {
                    // Prints "psn is an identified user"
                    //server->appendStatusMessage(i18n("Whois"),parameterList.join(" ").section(QLatin1Char(' '),1)+' '+trailing);
                    // The above line works fine, but can't be i18n'ised. So use the below instead.. I hope this is okay.
                    m_server->appendMessageToFrontmost(i18n("Whois"), i18n("%1 is an identified user.", parameterList.value(1)), messageTags);
                }
            }
            break;
        }
        case RPL_WHOISSECURE:
        {
            if (plHas(2))
            {
                if (getAutomaticRequest(QStringLiteral("WHOIS"), parameterList.value(1)) == 0)
                    m_server->appendMessageToFrontmost(i18n("Whois"), i18n("%1 is using a secure connection.", parameterList.value(1)), messageTags);
            }
            break;
        }
        // Sample WHO response
        //"/WHO #lounge"
        //[21:39] [352] #lounge jasmine bots.worldforge.org irc.worldforge.org jasmine H 0 jasmine
        //[21:39] [352] #lounge ~Nottingha worldforge.org irc.worldforge.org SherwoodSpirit H 0 Arboreal Entity
        case RPL_WHOSPCRPL:
        case RPL_WHOREPLY:
        {
            if (plHas(6))
            {
                NickInfoPtr nickInfo = m_server->getNickInfo(parameterList.value(5));
                                                // G=away G@=away,op G+=away,voice
                bool bAway = parameterList.value(6).toUpper().startsWith(QLatin1Char('G'));
                QString realName = trailing;

                if (realName.indexOf(QRegularExpression(QStringLiteral("\\d\\s"))) == 0)
                    realName = realName.mid (2);

                if (nickInfo)
                {
                    nickInfo->setHostmask(i18n("%1@%2", parameterList.value(2), parameterList.value(3)));
                    nickInfo->setRealName(realName);
                    nickInfo->setAway(bAway);
                    if(!bAway)
                    {
                        nickInfo->setAwayMessage(QString());
                    }

                    if(m_server->capabilities() & Server::WHOX && m_server->capabilities() & Server::ExtendedJoin)
                    {
                        nickInfo->setAccount(parameterList.value(8));
                    }
                }
                // Display message only if this was not an automatic request.
                if (!m_whoRequestList.isEmpty())     // for safe
                {
                    if (getAutomaticRequest(QStringLiteral("WHO"),m_whoRequestList.front())==0)
                    {
                        m_server->appendMessageToFrontmost(i18n("Who"),
                            i18n("%1 is %2@%3 (%4)%5", parameterList.value(5),
                                parameterList.value(2),
                                parameterList.value(3),
                                realName,
                                bAway?i18n(" (Away)"):QString()), messageTags,
                            false); // Don't parse as url
                    }
                }
            }
            break;
        }
        case RPL_ENDOFWHO:
        {
            if (plHas(2))
            {
                if (!m_whoRequestList.isEmpty())
                {
                    const QString param = parameterList.value(1).toLower();
                    // for safety
                    const int idx = m_whoRequestList.indexOf(param);
                    if (idx > -1)
                    {
                        if (getAutomaticRequest(QStringLiteral("WHO"), param) == 0)
                        {
                            m_server->appendMessageToFrontmost(i18n("Who"),
                                i18n("End of /WHO list for %1",
                                    parameterList.value(1)), messageTags);
                        }
                        else
                        {
                            setAutomaticRequest(QStringLiteral("WHO"), param, false);
                        }

                        m_whoRequestList.removeAt(idx);
                    }
                    else
                    {
                        // whoRequestList seems to be broken.
                        qCDebug(KONVERSATION_LOG) << "RPL_ENDOFWHO: malformed ENDOFWHO. retrieved: "
                            << parameterList.value(1) << " expected: " << m_whoRequestList.front();
                        m_whoRequestList.clear();
                    }
                }
                else
                {
                    qCDebug(KONVERSATION_LOG) << "RPL_ENDOFWHO: unexpected ENDOFWHO. retrieved: "
                        << parameterList.value(1);
                }

                Q_EMIT endOfWho(parameterList.value(1));
            }
            break;
        }
        case RPL_WHOISCHANNELS:
        {
            if (plHas(3))
            {
                QStringList userChannels,voiceChannels,opChannels,halfopChannels,ownerChannels,adminChannels;

                // get a list of all channels the user is in
                QStringList channelList=trailing.split(QLatin1Char(' '), Qt::SkipEmptyParts);
                channelList.sort();

                // split up the list in channels where they are operator / user / voice
                for (const QString& lookChannel : std::as_const(channelList)) {
                    if (lookChannel.startsWith(QLatin1Char('*')) || lookChannel.startsWith(QLatin1Char('&')))
                    {
                        adminChannels.append(lookChannel.mid(1));
                        m_server->setChannelNick(lookChannel.mid(1), parameterList.value(1), 16);
                    }
                                                // See bug #97354 part 2
                    else if((lookChannel.startsWith(QLatin1Char('!')) || lookChannel.startsWith(QLatin1Char('~'))) && m_server->isAChannel(lookChannel.mid(1)))
                    {
                        ownerChannels.append(lookChannel.mid(1));
                        m_server->setChannelNick(lookChannel.mid(1), parameterList.value(1), 8);
                    }
                                                // See bug #97354 part 1
                    else if (lookChannel.startsWith(QLatin1String("@+")))
                    {
                        opChannels.append(lookChannel.mid(2));
                        m_server->setChannelNick(lookChannel.mid(2), parameterList.value(1), 4);
                    }
                    else if (lookChannel.startsWith(QLatin1Char('@')))
                    {
                        opChannels.append(lookChannel.mid(1));
                        m_server->setChannelNick(lookChannel.mid(1), parameterList.value(1), 4);
                    }
                    else if (lookChannel.startsWith(QLatin1Char('%')))
                    {
                        halfopChannels.append(lookChannel.mid(1));
                        m_server->setChannelNick(lookChannel.mid(1), parameterList.value(1), 2);
                    }
                    else if (lookChannel.startsWith(QLatin1Char('+')))
                    {
                        voiceChannels.append(lookChannel.mid(1));
                        m_server->setChannelNick(lookChannel.mid(1), parameterList.value(1), 1);
                    }
                    else
                    {
                        userChannels.append(lookChannel);
                        m_server->setChannelNick(lookChannel, parameterList.value(1), 0);
                    }
                }                                 // endfor
                // Display message only if this was not an automatic request.
                if (getAutomaticRequest(QStringLiteral("WHOIS"), parameterList.value(1)) == 0)
                {
                    if (!userChannels.isEmpty()) {
                        m_server->appendMessageToFrontmost(i18n("Whois"),
                            i18n("%1 is a user on channels: %2",
                                parameterList.value(1),
                                userChannels.join(QLatin1Char(' '))), messageTags
                            );
                    }
                    if (!voiceChannels.isEmpty()) {
                        m_server->appendMessageToFrontmost(i18n("Whois"),
                            i18n("%1 has voice on channels: %2",
                                parameterList.value(1), voiceChannels.join(QLatin1Char(' '))), messageTags
                            );
                    }
                    if (!halfopChannels.isEmpty()) {
                        m_server->appendMessageToFrontmost(i18n("Whois"),
                            i18n("%1 is a halfop on channels: %2",
                                parameterList.value(1), halfopChannels.join(QLatin1Char(' '))), messageTags
                            );
                    }
                    if (!opChannels.isEmpty()) {
                        m_server->appendMessageToFrontmost(i18n("Whois"),
                            i18n("%1 is an operator on channels: %2",
                                parameterList.value(1), opChannels.join(QLatin1Char(' '))), messageTags
                            );
                    }
                    if (!ownerChannels.isEmpty()) {
                        m_server->appendMessageToFrontmost(i18n("Whois"),
                            i18n("%1 is owner of channels: %2",
                                parameterList.value(1), ownerChannels.join(QLatin1Char(' '))), messageTags
                            );
                    }
                    if (!adminChannels.isEmpty()) {
                        m_server->appendMessageToFrontmost(i18n("Whois"),
                            i18n("%1 is admin on channels: %2",
                                parameterList.value(1), adminChannels.join(QLatin1Char(' '))), messageTags
                            );
                    }
                }
            }
            break;
        }
        case RPL_WHOISSERVER:
        {
            if (plHas(4))
            {
                NickInfoPtr nickInfo = m_server->getNickInfo(parameterList.value(1));
                if (nickInfo)
                {
                    nickInfo->setNetServer(parameterList.value(2));
                    nickInfo->setNetServerInfo(trailing);
                    // Clear the away state on assumption that if nick is away, this message will be followed
                    // by a 301 RPL_AWAY message.  Not necessary a invalid assumption, but what can we do?
                    nickInfo->setAway(false);
                    nickInfo->setAwayMessage(QString());
                }
                // Display message only if this was not an automatic request.
                if (getAutomaticRequest(QStringLiteral("WHOIS"), parameterList.value(1)) == 0)
                {
                    m_server->appendMessageToFrontmost(i18n("Whois"),
                        i18n("%1 is online via %2 (%3).", parameterList.value(1),
                            parameterList.value(2), trailing), messageTags
                        );
                }
            }
            break;
        }
        case RPL_WHOISHELPER:
        {
            if (plHas(2))
            {
                // Display message only if this was not an automatic request.
                if (getAutomaticRequest(QStringLiteral("WHOIS"), parameterList.value(1)) == 0)
                {
                    m_server->appendMessageToFrontmost(i18n("Whois"),
                        i18n("%1 is available for help.",
                            parameterList.value(1)), messageTags
                        );
                }
            }
            break;
        }
        case RPL_WHOISOPERATOR:
        {
            if (plHas(2))
            {
                // Display message only if this was not an automatic request.
                if (getAutomaticRequest(QStringLiteral("WHOIS"), parameterList.value(1)) == 0)
                {
                    if (trailing.toLower().simplified().startsWith(QLatin1String("is an irc operator")))
                        m_server->appendMessageToFrontmost(i18n("Whois"), i18n("%1 is an IRC Operator.", parameterList.value(1)), messageTags);
                    else
                        m_server->appendMessageToFrontmost(i18n("Whois"), QStringLiteral("%1 %2").arg(parameterList.value(1), trailing), messageTags);
                }
            }
            break;
        }
        case RPL_WHOISIDLE:
        {
            if (plHas(3))
            {
                // get idle time in seconds
                bool ok = false;
                long seconds = parameterList.value(2).toLong(&ok);
                if (!ok) break;

                long minutes = seconds/60;
                long hours   = minutes/60;
                long days    = hours/24;

                QDateTime signonTime;
                uint signonTimestamp = parameterList.value(3).toUInt(&ok);

                if (ok && parameterList.count() == 5)
                    signonTime.setMSecsSinceEpoch(static_cast<qint64>(signonTimestamp) * 1000);

                if (!signonTime.isNull())
                {
                    NickInfoPtr nickInfo = m_server->getNickInfo(parameterList.value(1));

                    if (nickInfo)
                        nickInfo->setOnlineSince(signonTime);
                }

                // if idle time is longer than a day
                // Display message only if this was not an automatic request.
                if (getAutomaticRequest(QStringLiteral("WHOIS"), parameterList.value(1)) == 0)
                {
                    if (days)
                    {
                        const QString daysString = i18np("1 day", "%1 days", days);
                        const QString hoursString = i18np("1 hour", "%1 hours", (hours % 24));
                        const QString minutesString = i18np("1 minute", "%1 minutes", (minutes % 60));
                        const QString secondsString = i18np("1 second", "%1 seconds", (seconds % 60));

                        m_server->appendMessageToFrontmost(i18n("Whois"),
                            i18nc("%1 = name of person, %2 = (x days), %3 = (x hours), %4 = (x minutes), %5 = (x seconds)",
                                "%1 has been idle for %2, %3, %4, and %5.",
                                parameterList.value(1),
                                daysString, hoursString, minutesString, secondsString), messageTags);
                        // or longer than an hour
                    }
                    else if (hours)
                    {
                        const QString hoursString = i18np("1 hour", "%1 hours", hours);
                        const QString minutesString = i18np("1 minute", "%1 minutes", (minutes % 60));
                        const QString secondsString = i18np("1 second", "%1 seconds", (seconds % 60));
                        m_server->appendMessageToFrontmost(i18n("Whois"),
                            i18nc("%1 = name of person, %2 = (x hours), %3 = (x minutes), %4 = (x seconds)",
                                "%1 has been idle for %2, %3, and %4.", parameterList.value(1), hoursString,
                                minutesString, secondsString), messageTags);
                        // or longer than a minute
                    }
                    else if (minutes)
                    {
                        const QString minutesString = i18np("1 minute", "%1 minutes", minutes);
                        const QString secondsString = i18np("1 second", "%1 seconds", (seconds % 60));
                        m_server->appendMessageToFrontmost(i18n("Whois"),
                            i18nc("%1 = name of person, %2 = (x minutes), %3 = (x seconds)",
                                "%1 has been idle for %2 and %3.", parameterList.value(1), minutesString, secondsString), messageTags);
                        // or just some seconds
                    }
                    else
                    {
                        m_server->appendMessageToFrontmost(i18n("Whois"),
                        i18np("%2 has been idle for 1 second.", "%2 has been idle for %1 seconds.", seconds, parameterList.value(1)), messageTags);
                    }

                    if (!signonTime.isNull())
                    {
                        m_server->appendMessageToFrontmost(i18n("Whois"),
                            i18n("%1 has been online since %2.",
                            parameterList.value(1), QLocale().toString(signonTime, QLocale::ShortFormat)), messageTags);
                    }
                }
            }
            break;
        }
        case RPL_ENDOFWHOIS:
        {
            if (plHas(2))
            {
                /*FIXME why is the nickinfo line below commented out?
            //NickInfoPtr nickInfo = m_server->getNickInfo(parameterList.value(1));
                */
                // Display message only if this was not an automatic request.
                if (getAutomaticRequest(QStringLiteral("WHOIS"), parameterList.value(1)) == 0)
                {
                    m_server->appendMessageToFrontmost(i18n("Whois"), i18n("End of WHOIS list."), messageTags);
                }
                // was this an automatic request?
                if (getAutomaticRequest(QStringLiteral("WHOIS"), parameterList.value(1)) != 0)
                {
                    setAutomaticRequest(QStringLiteral("WHOIS"), parameterList.value(1), false);
                }
            }
            break;
        }
        case RPL_USERHOST:
        {
            if (plHas(2))
            {
                // iterate over all nick/masks in reply
                const QStringList uhosts = trailing.split(QLatin1Char(' '), Qt::SkipEmptyParts);

                for (const QString& uhost : uhosts) {
                    // extract nickname and hostmask from reply
                    QString nick(uhost.section(QLatin1Char('='),0,0));
                    QString mask(uhost.section(QLatin1Char('='),1));

                    // get away and IRC operator flags
                    bool away=(mask[0]==QLatin1Char('-'));
                    bool ircOp=(nick[nick.length()-1]==QLatin1Char('*'));

                    // cut flags from nick/hostmask
                    mask.remove(0, 1);
                    if (ircOp)
                    {
                        nick=nick.left(nick.length()-1);
                    }

                    // inform server of this user's data
                    Q_EMIT userhost(nick,mask,away,ircOp);

                    // display message only if this was no automatic request
                    if (getAutomaticRequest(QStringLiteral("USERHOST"),nick)==0)
                    {
                        m_server->appendMessageToFrontmost(i18n("Userhost"),
                            i18nc("%1 = nick, %2 = shows if nick is op, %3 = hostmask, %4 = shows away", "%1%2 is %3%4.",
                            nick,
                            (ircOp) ? i18n(" (IRC Operator)") : QString()
                            ,mask,
                            (away) ? i18n(" (away)") : QString()), messageTags);
                    }

                    // was this an automatic request?
                    if (getAutomaticRequest(QStringLiteral("USERHOST"),nick)!=0)
                    {
                        setAutomaticRequest(QStringLiteral("USERHOST"),nick,false);
                    }
                }                                 // for
            }
            break;
        }
        case RPL_LISTSTART:                   //FIXME This reply is obsolete!!!
        {
            if (plHas(0))
            {
                if (getAutomaticRequest(QStringLiteral("LIST"),QString())==0)
                {
                    m_server->appendMessageToFrontmost(i18n("List"), i18n("List of channels:"), messageTags);
                }
            }
            break;
        }
        case RPL_LIST:
        {
            if (plHas(3))
            {
                if (getAutomaticRequest(QStringLiteral("LIST"),QString())==0)
                {
                    QString message;
                    message=i18np("%2 (%1 user): %3", "%2 (%1 users): %3", parameterList.value(2).toInt(), parameterList.value(1), trailing);
                    m_server->appendMessageToFrontmost(i18n("List"), message, messageTags);
                }
                else                              // send them to /LIST window
                {
                    Q_EMIT addToChannelList(parameterList.value(1), parameterList.value(2).toInt(), trailing);
                }
            }
            break;
        }
        case RPL_LISTEND:
        {
            if (plHas(0))
            {
                // was this an automatic request?
                if (getAutomaticRequest(QStringLiteral("LIST"),QString())==0)
                {
                    m_server->appendMessageToFrontmost(i18n("List"), i18n("End of channel list."), messageTags);
                }
                else
                {
                    Q_EMIT endOfChannelList();
                    setAutomaticRequest(QStringLiteral("LIST"),QString(),false);
                }
            }
            break;
        }
        case RPL_NOWAWAY:
        {
            if (plHas(1))
            {
                NickInfoPtr nickInfo = m_server->getNickInfo(parameterList.value(0));
                if (nickInfo)
                {
                    nickInfo->setAway(true);
                }

                m_server->setAway(true, messageTags);
            }
            break;
        }
        case RPL_UNAWAY:
        {
            if (plHas(1))
            {
                NickInfoPtr nickInfo = m_server->getNickInfo(parameterList.value(0));

                if (nickInfo)
                {
                    nickInfo->setAway(false);
                    nickInfo->setAwayMessage(QString());
                }

                m_server->setAway(false, messageTags);
            }
            break;
        }
        case RPL_BANLIST:
        {
            //:calvino.freenode.net 367 argonel #konversation fooish!~a@example.com argonel!argkde4@konversation/developer/argonel 1269464382
            if (plHas(3))
            {
                if (getAutomaticRequest(QStringLiteral("BANLIST"), parameterList.value(1)))
                {
                    m_server->addBan(parameterList.value(1), parameterList.join(QLatin1Char(' ')).section(QLatin1Char(' '), 2, 4)); //<-- QString::Section handles out of bounds end parameter
                }
                else
                {
                    QDateTime when;
                    if (plHas(5))
                        when.setMSecsSinceEpoch(static_cast<qint64>(parameterList.value(4).toInt()) * 1000);
                    else
                        when = QDateTime::currentDateTime(); //use todays date instead of Jan 1 1970

                    QString setter(parameterList.value(3, i18nc("The server didn't respond with the identity of the ban creator, so we say unknown (in brackets to avoid confusion with a real nickname)", "(unknown)")).section(QLatin1Char('!'), 0, 0));

                    m_server->appendMessageToFrontmost(i18n("BanList:%1", parameterList.value(1)),
                                i18nc("BanList message: e.g. *!*@aol.com set by MrGrim on <date>", "%1 set by %2 on %3",
                                    parameterList.value(2), setter, QLocale().toString(when, QLocale::ShortFormat)), messageTags
                                );
                }
            }
            break;
        }
        case RPL_ENDOFBANLIST:
        {
            if (plHas(2))
            {
                if (getAutomaticRequest(QStringLiteral("BANLIST"), parameterList.value(1)))
                {
                    setAutomaticRequest(QStringLiteral("BANLIST"), parameterList.value(1), false);
                }
                else
                {
                    m_server->appendMessageToFrontmost(i18n("BanList:%1", parameterList.value(1)), i18n("End of Ban List."), messageTags);
                }
            }
            break;
        }
        case ERR_NOCHANMODES:
        {
            if (plHas(3))
            {
                ChatWindow *chatwindow = m_server->getChannelByName(parameterList.value(1));
                if (chatwindow)
                {
                    chatwindow->appendServerMessage(i18n("Channel"), trailing, messageTags);
                }
                else // We couldn't join the channel , so print the error. with [#channel] : <Error Message>
                {
                    m_server->appendMessageToFrontmost(i18n("Channel"), trailing, messageTags);
                }
            }
            break;
        }
        case ERR_NOSUCHSERVER:
        {
            if (plHas(2))
            {
                //Some servers don't know their name, so they return an error instead of the PING data
                if (getLagMeasuring() && trailing.startsWith(prefix))
                {
                    m_server->pongReceived();
                }
                else if (getAutomaticRequest(QStringLiteral("WHOIS"), parameterList.value(1)) != 0) //Inhibit message if this was an automatic request
                {
                    setAutomaticRequest(QStringLiteral("WHOIS"), parameterList.value(1), false);
                }
                else
                {
                    m_server->appendMessageToFrontmost(i18n("Error"), i18n("No such server: %1.", parameterList.value(1)), messageTags);
                }
            }
            break;
        }
        case ERR_UNAVAILRESOURCE:
        {
            if (plHas(2))
            {
                if (m_server->isConnected())
                    m_server->appendMessageToFrontmost(i18n("Error"), i18n("%1 is currently unavailable.", parameterList.value(1)), messageTags);
                else
                {
                    QString newNick = m_server->getNextNickname();

                    // The user chose to disconnect
                    if (newNick.isNull())
                        m_server->disconnectServer();
                    else
                    {
                        m_server->obtainNickInfo(m_server->getNickname()) ;
                        m_server->renameNick(m_server->getNickname(), newNick, messageTags);
                        m_server->appendMessageToFrontmost(i18n("Nick"),
                            i18n("Nickname %1 is unavailable. Trying %2.", parameterList.value(1), newNick), messageTags);
                        m_server->queue(QStringLiteral("NICK ")+newNick);
                    }
                }
            }
            break;
        }
        case RPL_HIGHCONNECTCOUNT:
        case RPL_LUSERCLIENT:
        case RPL_LUSEROP:
        case RPL_LUSERUNKNOWN:
        case RPL_LUSERCHANNELS:
        case RPL_LUSERME:
        { // TODO make sure this works, i amputated the "+ ' '+trailing"
            if (plHas(0))
            {
                m_server->appendStatusMessage(i18n("Users"), parameterList.join(QLatin1Char(' ')).section(QLatin1Char(' '),1), messageTags);
            }
            break;
        }
        case ERR_UNKNOWNCOMMAND:
        {
            if (plHas(2))
            {
                m_server->appendMessageToFrontmost(i18n("Error"), i18n("%1: Unknown command.", parameterList.value(1)), messageTags);
            }
            break;
        }
        case ERR_NOTREGISTERED:
        {
            if (plHas(0))
            {
                m_server->appendMessageToFrontmost(i18n("Error"), i18n("Not registered."), messageTags);
            }
            break;
        }
        case ERR_NEEDMOREPARAMS:
        {
            if (plHas(2))
            {
                m_server->appendMessageToFrontmost(i18n("Error"), i18n("%1: This command requires more parameters.", parameterList.value(1)), messageTags);
            }
            break;
        }
        case RPL_CAPAB: // Special freenode reply afaik
        {
            if (plHas(2))
            {
                // Disable as we don't use this for anything yet
                if (trailing.contains(QLatin1String("IDENTIFY-MSG")))
                {
                    m_server->enableIdentifyMsg(true);
                }
                else // TEST is this right? split the logic up in prep for slotization
                {
                    m_server->appendMessageToFrontmost(QString::number(command), parameterList.join(QLatin1Char(' ')).section(QLatin1Char(' '),1) + QLatin1Char(' ')+trailing, messageTags);
                }
            }
            break;
        }
        case ERR_BADCHANNELKEY:
        {
            if (plHas(2))
            {
                m_server->appendMessageToFrontmost(i18n("Error"), i18n("Cannot join %1: The channel is password-protected and either a wrong or no password was given.", parameterList.value(1)), messageTags);
            }
            break;
        }
        case RPL_LOGGEDIN:
        {
            if (plHas(3))
                m_server->appendStatusMessage(i18n("Info"), i18n("You are now logged in as %1.", parameterList.value(2)), messageTags);

            break;
        }
        case RPL_SASLSUCCESS:
        {
            if (plHas(2))
            {
                m_server->appendStatusMessage(i18n("Info"), i18n("SASL authentication successful."), messageTags);
                m_server->capEndNegotiation();

                NickInfoPtr nickInfo = m_server->getNickInfo(m_server->getNickname());
                if (nickInfo) nickInfo->setIdentified(true);
            }

            break;
        }
        case ERR_SASLFAIL:
        {
            if (plHas(2))
            {
                m_server->appendStatusMessage(i18n("Error"), i18n("SASL authentication attempt failed."), messageTags);
                m_server->capEndNegotiation();
            }

            break;
        }
        case ERR_SASLABORTED:
        {
            if (plHas(2))
                m_server->appendStatusMessage(i18n("Info"), i18n("SASL authentication aborted."), messageTags);

            break;
        }
        default:
        {
            // All yet unknown messages go into the frontmost window without the
            // preceding nickname
            qCDebug(KONVERSATION_LOG) << "unknown numeric" << parameterList.count() << _plHad << _plWanted << command << parameterList.join(QLatin1Char(' '));
            m_server->appendMessageToFrontmost(QString::number(command), parameterList.join(QLatin1Char(' ')), messageTags);
        }
    } // end of numeric switch
    if (!_plHad)
        qCDebug(KONVERSATION_LOG) << "numeric format error" << parameterList.count() << _plHad << _plWanted << command << parameterList.join(QLatin1Char(' '));
}

#include "moc_inputfilter.cpp"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
