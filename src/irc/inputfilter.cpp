/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

/*
  Copyright (C) 2002 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2004 Peter Simonsson <psn@linux.se>
  Copyright (C) 2006-2008 Eike Hein <hein@kde.org>
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
#include <config-konversation.h>

#include <QStringList>
#include <QDateTime>
#include <QRegExp>
#include <QLocale>


InputFilter::InputFilter()
    : m_server(0),
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

/*
Prefix parsePrefix(QString prefix)
{
    //3 possibilities - bare nickname, nickname and a host with optional ident, or a server if there is no punct, its a nickname
    //there must be at least 1 character before a symbol or its some kind of screwed up nickname
    Prefix pre;
    int id=prefix.indexOf("!");
    int ad=prefix.indexOf("@");
    if (prefix.indexOf(".") > 0 && id==-1 && ad==-1 ) // it is a server
    {
        pre.isServer=true;
        pre.host=prefix;
    }
    else
    {
        if (ad >0)
        {
            pre.host=prefix.mid(ad+1);
            prefix.truncate(ad);
            if (id > 0 && id < ad)
            {
                pre.ident=prefix.mid(id+1);
                prefix.truncate(id);
            }
        }
        //else // consider all of it a nickname
        pre.nickname=prefix;
    }
    return pre;
}
*/

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
    QString prefix;
    int start=0;
    int end(posOrLen(' ', line));

    if (line[0]==QLatin1Char(':'))
    {
        start=end+1;
        prefix=line.mid(1, end-1); //skips the colon and does not include the trailing space
        end=posOrLen(' ', line, start);
    }

    //even though the standard is UPPER CASE, someone when through a great deal of trouble to make this lower case...
    QString command = QString(line.mid(start, end-start)).toLower();
    start=end+1;

    int trailing=line.indexOf(QStringLiteral(" :"), end);
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
        parseServerCommand(prefix, command, parameterList);
    }
    else
    {
        parseClientCommand(prefix, command, parameterList);
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
        qDebug() << "plhad" << count << "wanted" << x;
    return _plHad;
}

void InputFilter::parseClientCommand(const QString &prefix, const QString &command, QStringList &parameterList)
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
    if (parameterList.isEmpty())
        return;

    //PRIVMSG #channel :message
    if (command == QStringLiteral("privmsg") && plHas(2))
    {
        bool isChan = isAChannel(parameterList.value(0));
        // CTCP message?
        if (m_server->identifyMsg() && (trailing.length() > 1 && (trailing.at(0) == QLatin1Char('+') || trailing.at(0) == QLatin1Char('-'))))
        {
            trailing = trailing.mid(1);
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
            if (ctcpCommand == QStringLiteral("action") && isChan)
            {
                if (!isIgnore(prefix, Ignore::Channel))
                {
                    Channel* channel = m_server->getChannelByName( parameterList.value(0) );

                    if (!channel) {
                        qCritical() << "Didn't find the channel " << parameterList.value(0) << endl;
                        return;
                    }

                    channel->appendAction(sourceNick, ctcpArgument);

                    if (sourceNick != m_server->getNickname())
                    {
                        if (hasArg && ctcpArgument.toLower().contains(QRegExp(QStringLiteral("(^|[^\\d\\w])")
                            + QRegExp::escape(m_server->loweredNickname())
                            + QStringLiteral("([^\\d\\w]|$)"))))
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
            else if (ctcpCommand == QStringLiteral("action") && !isChan)
            {
                // Check if we ignore queries from this nick
                if (!isIgnore(prefix, Ignore::Query))
                {
                    NickInfoPtr nickinfo = m_server->obtainNickInfo(sourceNick);
                    nickinfo->setHostmask(sourceHostmask);

                    // create new query (server will check for dupes)
                    Query* query = m_server->addQuery(nickinfo, false /* we didn't initiate this*/ );

                    // send action to query
                    query->appendAction(sourceNick, ctcpArgument);

                    if (sourceNick != m_server->getNickname() && query)
                        konv_app->notificationHandler()->queryMessage(query, sourceNick, ctcpArgument);
                }
            }

            // Answer ping requests
            else if (ctcpCommand == QStringLiteral("ping") && hasArg)
            {
                if (!isIgnore(prefix,Ignore::CTCP))
                {
                    if (isChan)
                    {
                        m_server->appendMessageToFrontmost(i18n("CTCP"),
                            i18n("Received CTCP-PING request from %1 to channel %2, sending answer.",
                                 sourceNick, parameterList.value(0))
                            );
                    }
                    else
                    {
                        m_server->appendMessageToFrontmost(i18n("CTCP"),
                            i18n("Received CTCP-%1 request from %2, sending answer.",
                                 QStringLiteral("PING"), sourceNick)
                            );
                    }
                    m_server->ctcpReply(sourceNick, QString(QStringLiteral("PING %1")).arg(ctcpArgument));
                }
            }

            // Maybe it was a version request, so act appropriately
            else if (ctcpCommand == QStringLiteral("version"))
            {
                if(!isIgnore(prefix,Ignore::CTCP))
                {
                    if (isChan)
                    {
                        m_server->appendMessageToFrontmost(i18n("CTCP"),
                            i18n("Received Version request from %1 to channel %2.",
                                 sourceNick, parameterList.value(0))
                            );
                    }
                    else
                    {
                        m_server->appendMessageToFrontmost(i18n("CTCP"),
                            i18n("Received Version request from %1.",
                                 sourceNick)
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
                        reply = QString(QStringLiteral("Konversation %1 (C) 2002-2015 by the Konversation team"))
                            .arg(QStringLiteral(KONVI_VERSION));
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
                    dccArgumentList += dccArguments.split(QLatin1Char(' '), QString::SkipEmptyParts);

                    if (dccType==QStringLiteral("send"))
                    {
                        if (dccArgumentList.count()==4)
                        {
                            // incoming file
                            konv_app->notificationHandler()->dccIncoming(m_server->getStatusView(), sourceNick);
                            emit addDccGet(sourceNick,dccArgumentList);
                        }
                        else if (dccArgumentList.count() >= 5)
                        {
                            if (dccArgumentList[dccArgumentList.size() - 3] == QStringLiteral("0"))
                            {
                                // incoming file (Reverse DCC)
                                konv_app->notificationHandler()->dccIncoming(m_server->getStatusView(), sourceNick);
                                emit addDccGet(sourceNick,dccArgumentList);
                            }
                            else
                            {
                                // the receiver accepted the offer for Reverse DCC
                                emit startReverseDccSendTransfer(sourceNick,dccArgumentList);
                            }
                        }
                        else
                        {
                            m_server->appendMessageToFrontmost(i18n("DCC"),
                                i18n("Received invalid DCC SEND request from %1.",
                                     sourceNick)
                                );
                        }
                    }
                    else if (dccType==QStringLiteral("accept"))
                    {
                        // resume request was accepted
                        if (dccArgumentList.count() >= 3)
                        {
                            emit resumeDccGetTransfer(sourceNick,dccArgumentList);
                        }
                        else
                        {
                            m_server->appendMessageToFrontmost(i18n("DCC"),
                                i18n("Received invalid DCC ACCEPT request from %1.",
                                     sourceNick)
                                );
                        }
                    }
                    // Remote client wants our sent file resumed
                    else if (dccType==QStringLiteral("resume"))
                    {
                        if (dccArgumentList.count() >= 3)
                        {
                            emit resumeDccSendTransfer(sourceNick,dccArgumentList);
                        }
                        else
                        {
                            m_server->appendMessageToFrontmost(i18n("DCC"),
                                i18n("Received invalid DCC RESUME request from %1.",
                                     sourceNick)
                                );
                        }
                    }
                    else if (dccType==QStringLiteral("chat"))
                    {
                        if (dccArgumentList.count() == 3)
                        {
                            // incoming chat
                            emit addDccChat(sourceNick,dccArgumentList);
                        }
                        else if (dccArgumentList.count() == 4)
                        {
                            if (dccArgumentList[dccArgumentList.size() - 2] == QStringLiteral("0"))
                            {
                                // incoming chat (Reverse DCC)
                                emit addDccChat(sourceNick,dccArgumentList);
                            }
                            else
                            {
                                // the receiver accepted the offer for Reverse DCC chat
                                emit startReverseDccChat(sourceNick,dccArgumentList);
                            }
                        }
                        else
                        {
                            m_server->appendMessageToFrontmost(i18n("DCC"),
                                                             i18n("Received invalid DCC CHAT request from %1.",
                                                             sourceNick)
                                );
                        }
                    }
                    else
                    {
                        m_server->appendMessageToFrontmost(i18n("DCC"),
                            i18n("Unknown DCC command %1 received from %2.",
                                 ctcpArgument, sourceNick)
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
                            QString::fromLatin1("CLIENTINFO"), sourceNick)
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
                            QString::fromLatin1("TIME"), sourceNick)
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
                                 ctcp, sourceNick, parameterList.value(0))
                            );
                    else
                        m_server->appendMessageToFrontmost(i18n("CTCP"),
                            i18n("Received unknown CTCP-%1 request from %2.",
                                 ctcp, sourceNick)
                            );
                }
            }
        }
        // No CTCP, so it's an ordinary channel or query message
        else
        {
            parsePrivMsg(prefix, parameterList);
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
                    trailing = trailing.mid(1);
                }

                m_server->appendServerMessageToChannel(parameterList.value(0), i18n("Notice"),
                        i18n("-%1 to %2- %3", sourceNick, parameterList.value(0), trailing)
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
                        int dateArrived=QDateTime::currentDateTime().toTime_t();
                        int dateSent=reply.toInt();
                        int time = dateArrived-dateSent;
                        QString unit = i18np("second", "seconds", time);

                        m_server->appendMessageToFrontmost(i18n("CTCP"),
                            i18n("Received CTCP-PING reply from %1: %2 %3.",
                                 sourceNick, time, unit)
                            );
                    }
                    else if (replyReason.toLower() == QStringLiteral("dcc"))
                    {
                        qDebug() << reply;
                        QStringList dccList = reply.split(QLatin1Char(' '));

                        //all dcc notices we receive are rejects
                        if (dccList.count() >= 2 && dccList.first().toLower() == QStringLiteral("reject"))
                        {
                            dccList.removeFirst();
                            if (dccList.count() >= 2 && dccList.first().toLower() == QStringLiteral("send"))
                            {
                                dccList.removeFirst();
                                emit rejectDccSendTransfer(sourceNick,dccList);
                            }
                            else if (dccList.first().toLower() == QStringLiteral("chat"))
                            {
                                emit rejectDccChat(sourceNick);
                            }
                        }
                    }
                    // all other ctcp replies get a general message
                    else
                    {
                        m_server->appendMessageToFrontmost(i18n("CTCP"),
                            i18n("Received CTCP-%1 reply from %2: %3.",
                                 replyReason, sourceNick, reply)
                            );
                    }
                }
                // No, so it was a normal notice
                else
                {

                    #ifdef HAVE_QCA2
                    //Key exchange
                    if (trailing.startsWith(QLatin1String("DH1080_INIT ")))
                    {
                        m_server->appendMessageToFrontmost(i18n("Notice"), i18n("Received DH1080_INIT from %1", sourceNick));
                        m_server->parseInitKeyX(sourceNick, trailing.mid(12));
                    }
                    else if (trailing.startsWith(QLatin1String("DH1080_FINISH ")))
                    {
                        m_server->appendMessageToFrontmost(i18n("Notice"), i18n("Received DH1080_FINISH from %1", sourceNick));
                        m_server->parseFinishKeyX(sourceNick, trailing.mid(14));
                    }
                    else
                    {
                    #endif
                        m_server->appendMessageToFrontmost(i18n("Notice"), i18n("-%1- %2", sourceNick,
                            m_server->identifyMsg() ? trailing.mid(1) : trailing));
                    #ifdef HAVE_QCA2
                    }
                    #endif
                }
            }
        }
    }
    else if (command==QStringLiteral("join") && plHas(1))
    {
        QString channelName(trailing);
        // Sometimes JOIN comes without ":" in front of the channel name

        // Did we join the channel, or was it someone else?
        if (m_server->isNickname(sourceNick))
        {
            /*
                QString key;
                // TODO: Try to remember channel keys for autojoins and manual joins, so
                //       we can get %k to work

                if(channelName.contains(' '))
                {
                    key=channelName.section(' ',1,1);
                    channelName=channelName.section(' ',0,0);
                }
            */

            // Join the channel
            Channel* channel = m_server->joinChannel(channelName, sourceHostmask);

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
            Channel* channel = m_server->nickJoinsChannel(channelName, sourceNick, sourceHostmask);
            konv_app->notificationHandler()->join(channel, sourceNick);
        }
    }
    else if (command==QStringLiteral("kick") && plHas(2))
    {
        m_server->nickWasKickedFromChannel(parameterList.value(0), parameterList.value(1), sourceNick, trailing);
    }
    else if (command==QStringLiteral("part") && plHas(1))
    {
        // A version of the PART line encountered on ircu: ":Nick!user@host PART :#channel"

        QString channel(parameterList.value(0));
        QString reason(parameterList.value(1));

        Channel* channelPtr = m_server->removeNickFromChannel(channel, sourceNick, reason);

        if (sourceNick != m_server->getNickname())
        {
            konv_app->notificationHandler()->part(channelPtr, sourceNick);
        }
    }
    else if (command==QStringLiteral("quit") && plHas(1))
    {
        m_server->removeNickFromServer(sourceNick, trailing);
        if (sourceNick != m_server->getNickname())
        {
            konv_app->notificationHandler()->quit(m_server->getStatusView(), sourceNick);
        }
    }
    else if (command==QStringLiteral("nick") && plHas(1))
    {
        QString newNick(parameterList.value(0)); // Message may not include ":" in front of the new nickname

        m_server->renameNick(sourceNick, newNick);

        if (sourceNick != m_server->getNickname())
        {
            konv_app->notificationHandler()->nickChange(m_server->getStatusView(), sourceNick, newNick);
        }
    }
    else if (command==QStringLiteral("topic") && plHas(2))
    {
        m_server->setChannelTopic(sourceNick, parameterList.value(0), trailing);
    }
    else if (command==QStringLiteral("mode") && plHas(2)) // mode #channel -/+ mmm params
    {
        parseModes(sourceNick, parameterList);
        Channel* channel = m_server->getChannelByName(parameterList.value(0));
        konv_app->notificationHandler()->mode(channel, sourceNick, parameterList.value(0),
            QStringList(parameterList.mid(1)).join (QStringLiteral(" ")));
    }
    else if (command==QStringLiteral("invite") && plHas(2)) //:ejm!i=beezle@bas5-oshawa95-1176455927.dsl.bell.ca INVITE argnl :#sug4
    {
        if (!isIgnore(prefix, Ignore::Invite))
        {
            QString channel(trailing);

            m_server->appendMessageToFrontmost(i18n("Invite"),
                i18n("%1 invited you to channel %2.", sourceNick, channel)
                );
            emit invitation(sourceNick, channel);
        }
    }
    else
    {
        qDebug() << "unknown client command" << parameterList.count() << _plHad << _plWanted << command << parameterList.join(QStringLiteral(" "));
        m_server->appendMessageToFrontmost(command, parameterList.join(QStringLiteral(" ")));
    }
}

void InputFilter::parseServerCommand(const QString &prefix, const QString &command, QStringList &parameterList)
{
    bool isNumeric;
    int numeric = command.toInt(&isNumeric);

    Q_ASSERT(m_server);
    if (!m_server)
        return;

    if (!isNumeric)
    {
        if (command == QStringLiteral("ping"))
        {
            QString text;
            text = (!trailing.isEmpty()) ? trailing : parameterList.join(QStringLiteral(" "));

            if (!trailing.isEmpty())
            {
                text = prefix + QStringLiteral(" :") + text;
            }

            if (!text.startsWith(QLatin1Char(' ')))
            {
                text.prepend(QLatin1Char(' '));
            }

            // queue the reply to send it as soon as possible
            m_server->queue(QStringLiteral("PONG")+text, Server::HighPriority);

        }
        else if (command == QStringLiteral("error :closing link:"))
        {
            qDebug() << "link closed";
        }
        else if (command == QStringLiteral("pong"))
        {
            // double check if we are in lag measuring mode since some servers fail to send
            // the LAG cookie back in PONG
            if (trailing.startsWith(QLatin1String("LAG")) || getLagMeasuring())
            {
                m_server->pongReceived();
            }
        }
        else if (command == QStringLiteral("mode"))
        {
            parseModes(prefix, parameterList);
        }
        else if (command == QStringLiteral("notice"))
        {
            m_server->appendStatusMessage(i18n("Notice"), i18n("-%1- %2", prefix, trailing));
        }
        else if (command == QStringLiteral("kick") && plHas(3))
        {
            m_server->nickWasKickedFromChannel(parameterList.value(1), parameterList.value(2), prefix, trailing);
        }
        else if (command == QStringLiteral("privmsg"))
        {
            parsePrivMsg(prefix, parameterList);
        }
        else if (command==QStringLiteral("cap") && plHas(3))
        {
            QString command = parameterList.value(1).toLower();

            if (command == QStringLiteral("ack") || command == QStringLiteral("nak"))
            {
                m_server->capReply();

                QStringList capabilities = parameterList.value(2).split(QLatin1Char(' '), QString::SkipEmptyParts);

                foreach(const QString& capability, capabilities)
                {
                    int nameStart = capability.indexOf(QRegExp(QStringLiteral("[a-z0-9"), Qt::CaseInsensitive));
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

                    if (command == QStringLiteral("ack"))
                        m_server->capAcknowledged(name, modifiers);
                    else
                        m_server->capDenied(name);
                }
            }
        }
        else if (command == QStringLiteral("authenticate") && plHas(1))
        {
            if (m_server->getLastAuthenticateCommand() == QStringLiteral("PLAIN") && parameterList.value(0) == QStringLiteral("+"))
                m_server->registerWithServices();
        }
        // All yet unknown messages go into the frontmost window unaltered
        else
        {
            qDebug() << "unknown server command" << command;
            m_server->appendMessageToFrontmost(command, parameterList.join(QStringLiteral(" ")));
        }
    }
    else if (plHas(2)) //[0]==ourNick, [1] needs to be *something*
    {
        //:niven.freenode.net 353 argnl @ #konversation :@argonel psn @argnl bugbot pinotree CIA-13
        //QString m_serverAssignedNick(parameterList.takeFirst());
        QString m_serverAssignedNick(parameterList.first());

        switch (numeric)
        {
            case RPL_WELCOME:
            case RPL_YOURHOST:
            case RPL_CREATED:
            {
                if (plHas(0)) //make the script happy
                {
                    if (numeric == RPL_WELCOME)
                    {
                        QString host;

                        if (trailing.contains(QStringLiteral("@")))
                            host = trailing.section(QLatin1Char('@'), 1);

                        // re-set nickname, since the server may have truncated it
                        if (m_serverAssignedNick != m_server->getNickname())
                        {
                            m_server->renameNick(m_server->getNickname(), m_serverAssignedNick);
                        }

                        // Send the welcome signal, so the server class knows we are connected properly
                        emit welcome(host);
                        m_connecting = true;
                    }
                    m_server->appendStatusMessage(i18n("Welcome"), trailing);
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
                         parameterList.value(4))
                        );

                    QString allowed = m_server->allowedChannelModes();
                    QString newModes = parameterList.value(4);
                    if(!allowed.isEmpty()) //attempt to merge the two
                    {
                        for(int i=0; i < allowed.length(); i++)
                        {
                            if(!newModes.contains(allowed.at(i)))
                                newModes.append(allowed.at(i));
                        }
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
                    m_server->appendStatusMessage(i18n("Support"), parameterList.join(QStringLiteral(" ")));

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
                                if (value.length() || property.length())
                                    m_server->appendStatusMessage(QString(), QStringLiteral("XXX Server sent bad PREFIX in RPL_ISUPPORT, please report."));
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
                        else if (property == QStringLiteral("CAPAB"))
                        {
                            // Disable as we don't use this for anything yet
                            //server->queue("CAPAB IDENTIFY-MSG");
                        }
                        else if (property == QStringLiteral("CHANMODES"))
                        {
                            if(!value.isEmpty())
                            {
                                m_server->setChanModes(value);
                                QString allowed = m_server->allowedChannelModes();
                                QString newModes = value.remove(QLatin1Char(','));
                                if(!allowed.isEmpty()) //attempt to merge the two
                                {
                                    for(int i=0; i < allowed.length(); i++)
                                    {
                                        if(!newModes.contains(allowed.at(i)))
                                            newModes.append(allowed.at(i));
                                    }
                                }
                                m_server->setAllowedChannelModes(newModes);
                            }
                        }
                        else if (property == QStringLiteral("TOPICLEN"))
                        {
                            if (!value.isEmpty())
                            {
                                bool ok =  false;
                                int topicLength = value.toInt(&ok);

                                if (ok)
                                    m_server->setTopicLength(topicLength);
                            }
                        }
                        else
                        {
                            //qDebug() << "Ignored server-capability: " << property << " with value '" << value << "'";
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
                    QString message=QString(QStringLiteral("%1 %2")).arg(i18n("Your personal modes are:")).arg(parameterList.join(QLatin1Char(' ')).section(QLatin1Char(' '),1));
                    m_server->appendMessageToFrontmost(QStringLiteral("Info"), message);
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
                    for (int index=0;index<modeString.length();index++)
                    {
                        QString parameter;
                        char mode(modeString[index].toLatin1());
                        if(mode!='+')
                        {
                            if(!modesAre.isEmpty())
                                modesAre+=QStringLiteral(", ");

                            if(mode=='k')
                            {
                                parameter=parameterList.value(parameterCount++);
                                message += QLatin1Char(' ') + parameter;
                                modesAre+=i18n("password protected");
                            }
                            else if(mode=='l')
                            {
                                parameter=parameterList.value(parameterCount++);
                                message += QLatin1Char(' ') + parameter;
                                modesAre+=i18np("limited to %1 user", "limited to %1 users", parameter.toInt());
                            }
                            else if(channelModesHash.contains(QLatin1Char(mode)))
                            {
                                modesAre+=channelModesHash.value(QLatin1Char(mode));
                            }
                            else
                            {
                                modesAre+=QLatin1Char(mode);
                            }
                            m_server->updateChannelModeWidgets(parameterList.value(1), mode, parameter);
                        }
                    } // endfor
                    if (!modesAre.isEmpty() && Preferences::self()->useLiteralModes())
                    {
                        m_server->appendCommandMessageToChannel(parameterList.value(1), i18n("Mode"), message);
                    }
                    else
                    {
                        m_server->appendCommandMessageToChannel(parameterList.value(1), i18n("Mode"),
                            i18n("Channel modes: ") + modesAre);
                    }
                }
                break;
            }
            case RPL_CHANNELURLIS:
            {// :niven.freenode.net 328 argonel #channel :http://www.buggeroff.com/
                if (plHas(3))
                {
                    m_server->appendCommandMessageToChannel(parameterList.value(1), i18n("URL"),
                        i18n("Channel URL: %1", trailing));
                }
                break;
            }
            case RPL_CHANNELCREATED:
            {
                if (plHas(3))
                {
                    QDateTime when;
                    when.setTime_t(parameterList.value(2).toUInt());
                    m_server->appendCommandMessageToChannel(parameterList.value(1), i18n("Created"),
                        i18n("This channel was created on %1.",
                            QLocale().toString(when, QLocale::ShortFormat))
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
                        m_server->appendMessageToFrontmost(i18n("Whois"), i18n("%1 is logged in as %2.", parameterList.value(1), parameterList.value(2)) );
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
                        nickList = trailing.split(QLatin1Char(' '), QString::SkipEmptyParts);
                    }
                    else if (parameterList.count() > 3)
                    {
                        for(int i = 3; i < parameterList.count(); i++) {
                            nickList.append(parameterList.value(i));
                        }
                    }
                    else
                    {
                        qDebug() << "Hmm seems something is broken... can't get to the names!";
                    }

                    // send list to channel
                    m_server->queueNicks(parameterList.value(2), nickList); // TEST this was a 2

                    // Display message only if this was not an automatic request.
                    if (getAutomaticRequest(QStringLiteral("NAMES"), parameterList.value(2)) == 0)
                    {
                        m_server->appendMessageToFrontmost(i18n("Names"), trailing);
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
                        m_server->appendMessageToFrontmost(i18n("Names"), i18n("End of NAMES list."));
                    }

                    emit endOfNames(parameterList.value(1));
                }
                break;
            }
            // Topic set messages
            case RPL_NOTOPIC:
            {
                if (plHas(2))
                {
                    //this really has 3, but [2] is "No topic has been set"
                    m_server->appendMessageToFrontmost(i18n("TOPIC"), i18n("The channel %1 has no topic set.", parameterList.value(1)));
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
                        m_server->setChannelTopic(parameterList.value(1), topic);
                    }
                    else
                    {
                        m_server->appendMessageToFrontmost(i18n("Topic"), i18n("The channel topic for %1 is: \"%2\"", parameterList.value(1), topic));
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
                    when.setTime_t(parameterList.value(3).toUInt());

                    // See FIXME in RPL_TOPIC
                    if (getAutomaticRequest(QStringLiteral("TOPIC"), parameterList.value(1)) == 0)
                    {
                        m_server->appendCommandMessageToChannel(parameterList.value(1), i18n("Topic"),
                            i18n("The topic was set by %1 on %2.",
                            parameterList.value(2), QLocale().toString(when, QLocale::ShortFormat)),
                            false,
                            false);
                    }
                    else
                    {
                        m_server->appendMessageToFrontmost(i18n("Topic"), i18n("The topic for %1 was set by %2 on %3.",
                            parameterList.value(1),
                            parameterList.value(2),
                            QLocale().toString(when, QLocale::ShortFormat)),
                            false);
                        setAutomaticRequest(QStringLiteral("TOPIC"),parameterList.value(1), false);
                    }
                    emit topicAuthor(parameterList.value(1), parameterList.value(2), when);
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
                        m_server->appendMessageToFrontmost(i18n("Whois"), i18n("%1 is actually using the host %2.", parameterList.value(1), parameterList.value(2)));
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
                        m_server->appendMessageToFrontmost(i18n("Error"), i18n("%1: No such nick/channel.", parameterList.value(1)));
                    }
                    else if(getAutomaticRequest(QStringLiteral("WHOIS"), parameterList.value(1)) == 0) //Display message only if this was not an automatic request.
                    {
                        m_server->appendMessageToFrontmost(i18n("Error"), i18n("No such nick: %1.", parameterList.value(1)));
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
                        m_server->appendMessageToFrontmost(i18n("Error"), i18n("%1: No such channel.", parameterList.value(1)));
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
                        m_server->appendMessageToFrontmost(i18n("Nick"), i18n("Nickname already in use, try a different one."));
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
                                 m_server->appendMessageToFrontmost(i18n("Info"), i18n("The nickname %1 was already in use, but the connection failed before you responded.", m_server->getNickname()));
                        }
                        else
                        {
                            // Update Server window
                            m_server->obtainNickInfo(m_server->getNickname()) ;
                            m_server->renameNick(m_server->getNickname(), newNick);
                            // Show message
                            m_server->appendMessageToFrontmost(i18n("Nick"), i18n("Nickname already in use. Trying %1.", newNick));
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
                        m_server->appendMessageToFrontmost(i18n("Nick"), trailing);
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
                            m_server->renameNick(m_server->getNickname(), newNick);
                            m_server->appendMessageToFrontmost(i18n("Nick"), i18n("Erroneus nickname. Changing nick to %1.", newNick));
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
                    m_server->appendMessageToFrontmost(i18n("Error"), i18n("You are not on %1.", parameterList.value(1)));
                    setAutomaticRequest(QStringLiteral("TOPIC"),parameterList.value(1), false);

                }
                break;
            }
            case RPL_MOTDSTART:
            {
                if (plHas(1))
                {
                    if (!m_connecting || !Preferences::self()->skipMOTD())
                    m_server->appendStatusMessage(i18n("MOTD"), i18n("Message of the day:"));
                }
                break;
            }
            case RPL_MOTD:
            {
                if (plHas(2))
                {
                    if (!m_connecting || !Preferences::self()->skipMOTD())
                        m_server->appendStatusMessage(i18n("MOTD"), trailing);
                }
                break;
            }
            case RPL_ENDOFMOTD:
            {
                if (plHas(1))
                {
                    if (!m_connecting || !Preferences::self()->skipMOTD())
                        m_server->appendStatusMessage(i18n("MOTD"), i18n("End of message of the day"));

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
                    m_server->appendMessageToFrontmost(i18n("Error"), i18n("You need to be a channel operator in %1 to do that.", parameterList.value(1)));
                }
                break;
            }
            case RPL_YOUREOPER:
            {
                if (plHas(1))
                {
                    m_server->appendMessageToFrontmost(i18n("Notice"), i18n("You are now an IRC operator on this server."));
                }
                break;
            }
            case RPL_HOSTHIDDEN:
            {
                if (plHas(2))
                {
                    m_server->appendStatusMessage(i18n("Info"), i18n("'%1' is now your hidden host (set by services).", parameterList.value(1)));
                }
                break;
            }
            case RPL_GLOBALUSERS:                 // Current global users: 589 Max: 845
            {
                if (plHas(2))
                {
                    QString current(trailing.section(QLatin1Char(' '),3));
                    //QString max(trailing.section(' ',5,5));
                    m_server->appendStatusMessage(i18n("Users"), i18n("Current users on the network: %1", current));
                }
                break;
            }
            case RPL_LOCALUSERS:                  // Current local users: 589 Max: 845
            {
                if (plHas(2))
                {
                    QString current(trailing.section(QLatin1Char(' '), 3));
                    //QString max(trailing.section(' ',5,5));
                    m_server->appendStatusMessage(i18n("Users"), i18n("Current users on %1: %2.", prefix, current));
                }
                break;
            }
            case RPL_ISON:
            {
                if (plHas(2))
                {
                    // Tell server to start the next notify timer round
                    emit notifyResponse(trailing);
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
                            i18n("%1 is away: %2", parameterList.value(1), trailing)
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
                            parameterList.value(1), parameterList.value(2))
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
                        escapedRealName.replace(QLatin1Char('<'), QStringLiteral("&lt;")).replace(QLatin1Char('>'), QStringLiteral("&gt;"));
                        m_server->appendMessageToFrontmost(i18n("Whois"),
                            i18n("%1 is %2@%3 (%4)",
                                parameterList.value(1),
                                parameterList.value(2),
                                parameterList.value(3),
                                escapedRealName), false);   // Don't parse any urls
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
                                        ip)
                                    );
                            }
                            else
                            {
                                m_server->appendMessageToFrontmost(i18n("Error"),
                                    i18n("Unable to resolve address for %1 (%2)",
                                        parameterList.value(1),
                                        parameterList.value(3))
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
                        //server->appendStatusMessage(i18n("Whois"),parameterList.join(" ").section(' ',1)+' '+trailing);
                        // The above line works fine, but can't be i18n'ised. So use the below instead.. I hope this is okay.
                        m_server->appendMessageToFrontmost(i18n("Whois"), i18n("%1 is an identified user.", parameterList.value(1)));
                    }
                }
                break;
            }
            case RPL_WHOISSECURE:
            {
                if (plHas(2))
                {
                    if (getAutomaticRequest(QStringLiteral("WHOIS"), parameterList.value(1)) == 0)
                        m_server->appendMessageToFrontmost(i18n("Whois"), i18n("%1 is using a secure connection.", parameterList.value(1)));
                }
                break;
            }
            // Sample WHO response
            //"/WHO #lounge"
            //[21:39] [352] #lounge jasmine bots.worldforge.org irc.worldforge.org jasmine H 0 jasmine
            //[21:39] [352] #lounge ~Nottingha worldforge.org irc.worldforge.org SherwoodSpirit H 0 Arboreal Entity
            case RPL_WHOREPLY:
            {
                if (plHas(6))
                {
                    NickInfoPtr nickInfo = m_server->getNickInfo(parameterList.value(5));
                                                    // G=away G@=away,op G+=away,voice
                    bool bAway = parameterList.value(6).toUpper().startsWith(QLatin1Char('G'));
                    if (nickInfo)
                    {
                        nickInfo->setHostmask(i18n("%1@%2", parameterList.value(2), parameterList.value(3)));
                                                    //Strip off the "0 "
                        nickInfo->setRealName(trailing.section(QLatin1Char(' '), 1));
                        nickInfo->setAway(bAway);
                        if(!bAway)
                        {
                            nickInfo->setAwayMessage(QString());
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
                                    trailing.section(QLatin1Char(' '), 1),
                                    bAway?i18n(" (Away)"):QString()),
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
                                        parameterList.value(1)));
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
                            qDebug() << "RPL_ENDOFWHO: malformed ENDOFWHO. retrieved: "
                                << parameterList.value(1) << " expected: " << m_whoRequestList.front();
                            m_whoRequestList.clear();
                        }
                    }
                    else
                    {
                        qDebug() << "RPL_ENDOFWHO: unexpected ENDOFWHO. retrieved: "
                            << parameterList.value(1);
                    }

                    emit endOfWho(parameterList.value(1));
                }
                break;
            }
            case RPL_WHOISCHANNELS:
            {
                if (plHas(3))
                {
                    QStringList userChannels,voiceChannels,opChannels,halfopChannels,ownerChannels,adminChannels;

                    // get a list of all channels the user is in
                    QStringList channelList=trailing.split(QLatin1Char(' '), QString::SkipEmptyParts);
                    channelList.sort();

                    // split up the list in channels where they are operator / user / voice
                    for (int index=0; index < channelList.count(); index++)
                    {
                        QString lookChannel=channelList[index];
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
                        else if (lookChannel.startsWith(QStringLiteral("@+")))
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
                        if (userChannels.count())
                        {
                            m_server->appendMessageToFrontmost(i18n("Whois"),
                                i18n("%1 is a user on channels: %2",
                                    parameterList.value(1),
                                    userChannels.join(QStringLiteral(" ")))
                                );
                        }
                        if (voiceChannels.count())
                        {
                            m_server->appendMessageToFrontmost(i18n("Whois"),
                                i18n("%1 has voice on channels: %2",
                                    parameterList.value(1), voiceChannels.join(QStringLiteral(" ")))
                                );
                        }
                        if (halfopChannels.count())
                        {
                            m_server->appendMessageToFrontmost(i18n("Whois"),
                                i18n("%1 is a halfop on channels: %2",
                                    parameterList.value(1), halfopChannels.join(QStringLiteral(" ")))
                                );
                        }
                        if (opChannels.count())
                        {
                            m_server->appendMessageToFrontmost(i18n("Whois"),
                                i18n("%1 is an operator on channels: %2",
                                    parameterList.value(1), opChannels.join(QStringLiteral(" ")))
                                );
                        }
                        if (ownerChannels.count())
                        {
                            m_server->appendMessageToFrontmost(i18n("Whois"),
                                i18n("%1 is owner of channels: %2",
                                    parameterList.value(1), ownerChannels.join(QStringLiteral(" ")))
                                );
                        }
                        if (adminChannels.count())
                        {
                            m_server->appendMessageToFrontmost(i18n("Whois"),
                                i18n("%1 is admin on channels: %2",
                                    parameterList.value(1), adminChannels.join(QStringLiteral(" ")))
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
                                parameterList.value(2), trailing)
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
                                parameterList.value(1))
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
                            m_server->appendMessageToFrontmost(i18n("Whois"), i18n("%1 is an IRC Operator.", parameterList.value(1)));
                        else
                            m_server->appendMessageToFrontmost(i18n("Whois"), QString(QStringLiteral("%1 %2")).arg(parameterList.value(1)).arg(trailing));
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
                        signonTime.setTime_t(signonTimestamp);

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
                                    daysString, hoursString, minutesString, secondsString));
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
                                    minutesString, secondsString));
                            // or longer than a minute
                        }
                        else if (minutes)
                        {
                            const QString minutesString = i18np("1 minute", "%1 minutes", minutes);
                            const QString secondsString = i18np("1 second", "%1 seconds", (seconds % 60));
                            m_server->appendMessageToFrontmost(i18n("Whois"),
                                i18nc("%1 = name of person, %2 = (x minutes), %3 = (x seconds)",
                                    "%1 has been idle for %2 and %3.", parameterList.value(1), minutesString, secondsString));
                            // or just some seconds
                        }
                        else
                        {
                            m_server->appendMessageToFrontmost(i18n("Whois"),
                            i18np("%2 has been idle for 1 second.", "%2 has been idle for %1 seconds.", seconds, parameterList.value(1)));
                        }

                        if (!signonTime.isNull())
                        {
                            m_server->appendMessageToFrontmost(i18n("Whois"),
                                i18n("%1 has been online since %2.",
                                parameterList.value(1), QLocale().toString(signonTime, QLocale::ShortFormat)));
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
                        m_server->appendMessageToFrontmost(i18n("Whois"), i18n("End of WHOIS list."));
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
                    QStringList uhosts=trailing.split(QLatin1Char(' '), QString::SkipEmptyParts);

                    for (int index=0;index<uhosts.count();index++)
                    {
                        // extract nickname and hostmask from reply
                        QString nick(uhosts[index].section(QLatin1Char('='),0,0));
                        QString mask(uhosts[index].section(QLatin1Char('='),1));

                        // get away and IRC operator flags
                        bool away=(mask[0]==QLatin1Char('-'));
                        bool ircOp=(nick[nick.length()-1]==QLatin1Char('*'));

                        // cut flags from nick/hostmask
                        mask=mask.mid(1);
                        if (ircOp)
                        {
                            nick=nick.left(nick.length()-1);
                        }

                        // inform server of this user's data
                        emit userhost(nick,mask,away,ircOp);

                        // display message only if this was no automatic request
                        if (getAutomaticRequest(QStringLiteral("USERHOST"),nick)==0)
                        {
                            m_server->appendMessageToFrontmost(i18n("Userhost"),
                                i18nc("%1 = nick, %2 = shows if nick is op, %3 = hostmask, %4 = shows away", "%1%2 is %3%4.",
                                nick,
                                (ircOp) ? i18n(" (IRC Operator)") : QString()
                                ,mask,
                                (away) ? i18n(" (away)") : QString()));
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
                        m_server->appendMessageToFrontmost(i18n("List"), i18n("List of channels:"));
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
                        m_server->appendMessageToFrontmost(i18n("List"), message);
                    }
                    else                              // send them to /LIST window
                    {
                        emit addToChannelList(parameterList.value(1), parameterList.value(2).toInt(), trailing);
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
                        m_server->appendMessageToFrontmost(i18n("List"), i18n("End of channel list."));
                    }
                    else
                    {
                        emit endOfChannelList();
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

                    m_server->setAway(true);
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

                    m_server->setAway(false);
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
                        m_server->addBan(parameterList.value(1), parameterList.join(QStringLiteral(" ")).section(QLatin1Char(' '), 2, 4)); //<-- QString::Section handles out of bounds end parameter
                    }
                    else
                    {
                        QDateTime when;
                        if (plHas(5))
                            when.setTime_t(parameterList.value(4).toUInt());
                        else
                            when = QDateTime::currentDateTime(); //use todays date instead of Jan 1 1970

                        QString setter(parameterList.value(3, i18nc("The server didn't respond with the identity of the ban creator, so we say unknown (in brackets to avoid confusion with a real nickname)", "(unknown)")).section(QLatin1Char('!'), 0, 0));

                        m_server->appendMessageToFrontmost(i18n("BanList:%1", parameterList.value(1)),
                                    i18nc("BanList message: e.g. *!*@aol.com set by MrGrim on <date>", "%1 set by %2 on %3",
                                        parameterList.value(2), setter, QLocale().toString(when, QLocale::ShortFormat))
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
                        m_server->appendMessageToFrontmost(i18n("BanList:%1", parameterList.value(1)), i18n("End of Ban List."));
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
                        chatwindow->appendServerMessage(i18n("Channel"), trailing);
                    }
                    else // We couldn't join the channel , so print the error. with [#channel] : <Error Message>
                    {
                        m_server->appendMessageToFrontmost(i18n("Channel"), trailing);
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
                        m_server->appendMessageToFrontmost(i18n("Error"), i18n("No such server: %1.", parameterList.value(1)));
                    }
                }
                break;
            }
            case ERR_UNAVAILRESOURCE:
            {
                if (plHas(2))
                {
                    if (m_server->isConnected())
                        m_server->appendMessageToFrontmost(i18n("Error"), i18n("%1 is currently unavailable.", parameterList.value(1)));
                    else
                    {
                        QString newNick = m_server->getNextNickname();

                        // The user chose to disconnect
                        if (newNick.isNull())
                            m_server->disconnectServer();
                        else
                        {
                            m_server->obtainNickInfo(m_server->getNickname()) ;
                            m_server->renameNick(m_server->getNickname(), newNick);
                            m_server->appendMessageToFrontmost(i18n("Nick"),
                                i18n("Nickname %1 is unavailable. Trying %2.", parameterList.value(1), newNick));
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
                    m_server->appendStatusMessage(i18n("Users"), parameterList.join(QStringLiteral(" ")).section(QLatin1Char(' '),1));
                }
                break;
            }
            case ERR_UNKNOWNCOMMAND:
            {
                if (plHas(2))
                {
                    m_server->appendMessageToFrontmost(i18n("Error"), i18n("%1: Unknown command.", parameterList.value(1)));
                }
                break;
            }
            case ERR_NOTREGISTERED:
            {
                if (plHas(0))
                {
                    m_server->appendMessageToFrontmost(i18n("Error"), i18n("Not registered."));
                }
                break;
            }
            case ERR_NEEDMOREPARAMS:
            {
                if (plHas(2))
                {
                    m_server->appendMessageToFrontmost(i18n("Error"), i18n("%1: This command requires more parameters.", parameterList.value(1)));
                }
                break;
            }
            case RPL_CAPAB: // Special freenode reply afaik
            {
                if (plHas(2))
                {
                    // Disable as we don't use this for anything yet
                    if (trailing.contains(QStringLiteral("IDENTIFY-MSG")))
                    {
                        m_server->enableIdentifyMsg(true);
                    }
                    else // TEST is this right? split the logic up in prep for slotization
                    {
                        m_server->appendMessageToFrontmost(command, parameterList.join(QStringLiteral(" ")).section(QLatin1Char(' '),1) + QLatin1Char(' ')+trailing);
                    }
                }
                break;
            }
            case ERR_BADCHANNELKEY:
            {
                if (plHas(2))
                {
                    m_server->appendMessageToFrontmost(i18n("Error"), i18n("Cannot join %1: The channel is password-protected and either a wrong or no password was given.", parameterList.value(1)));
                }
                break;
            }
            case RPL_LOGGEDIN:
            {
                if (plHas(3))
                    m_server->appendStatusMessage(i18n("Info"), i18n("You are now logged in as %1.", parameterList.value(2)));

                break;
            }
            case RPL_SASLSUCCESS:
            {
                if (plHas(2))
                {
                    m_server->appendStatusMessage(i18n("Info"), i18n("SASL authentication successful."));
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
                    m_server->appendStatusMessage(i18n("Error"), i18n("SASL authentication attempt failed."));
                    m_server->capEndNegotiation();
                }

                break;
            }
            case ERR_SASLABORTED:
            {
                if (plHas(2))
                    m_server->appendStatusMessage(i18n("Info"), i18n("SASL authentication aborted."));

                break;
            }
            default:
            {
                // All yet unknown messages go into the frontmost window without the
                // preceding nickname
                qDebug() << "unknown numeric" << parameterList.count() << _plHad << _plWanted << command << parameterList.join(QStringLiteral(" "));
                m_server->appendMessageToFrontmost(command, parameterList.join(QStringLiteral(" ")));
            }
        } // end of numeric switch
        if (!_plHad)
            qDebug() << "numeric format error" << parameterList.count() << _plHad << _plWanted << command << parameterList.join(QStringLiteral(" "));
    } // end of numeric elseif
    else
    {
        qDebug() << "unknown message format" << parameterList.count() << _plHad << _plWanted << command << parameterList.join(QStringLiteral(" "));
    }
} // end of server

void InputFilter::parseModes(const QString &sourceNick, const QStringList &parameterList)
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
            m_server->appendStatusMessage(i18n("Mode"), message);
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

    for (int index=0;index<modestring.length();index++)
    {
        unsigned char mode(modestring[index].toLatin1());
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
                qDebug()   << "in updateChannelMode.  sourceNick: '" << sourceNick << "'  parameterlist: '"
                    << parameterList.join(QStringLiteral(", ")) << "'";
            }
            m_server->updateChannelMode(sourceNick, parameterList.value(0), mode, plus, parameter);
        }
    } // endfor

    if (Preferences::self()->useLiteralModes())
    {
        m_server->appendCommandMessageToChannel(parameterList.value(0), i18n("Mode"), message);
    }
}

// # & + and ! are *often*, but not necessarily, Channel identifiers. + and ! are non-RFC,
// so if a server doesn't offer 005 and supports + and ! channels, I think thats broken behaviour
// on their part - not ours. --Argonel
bool InputFilter::isAChannel(const QString &check)
{
    if (check.isEmpty())
        return false;
    Q_ASSERT(m_server);
    // if we ever see the assert, we need the ternary
    return m_server? m_server->isAChannel(check) : bool(QString(QStringLiteral("#&")).contains(check.at(0)));
}

bool InputFilter::isIgnore(const QString &sender, Ignore::Type type)
{
    bool doIgnore = false;

    foreach (Ignore* item, Preferences::ignoreList())
    {
        QRegExp ignoreItem(QRegExp::escape(item->getName()).replace(QStringLiteral("\\*"), QStringLiteral("(.*)")), Qt::CaseInsensitive);
        if (ignoreItem.exactMatch(sender) && (item->getFlags() & type))
            doIgnore = true;
        if (ignoreItem.exactMatch(sender) && (item->getFlags() & Ignore::Exception))
            return false;
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
        qDebug()   << "( " << command << ", " << name
            << " ) was negative! Resetting!";
        m_automaticRequest[command][name.toLower()]=0;
    }
}

int InputFilter::getAutomaticRequest(const QString& command, const QString& name)
{
    return m_automaticRequest[command][name.toLower()];
}

void InputFilter::addWhoRequest(const QString& name) { m_whoRequestList << name.toLower(); }

bool InputFilter::isWhoRequestUnderProcess(const QString& name) { return (m_whoRequestList.contains(name.toLower())>0); }

void InputFilter::setLagMeasuring(bool state) { m_lagMeasuring=state; }

bool InputFilter::getLagMeasuring()           { return m_lagMeasuring; }

void InputFilter::parsePrivMsg(const QString& prefix, QStringList& parameterList)
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

    if(isAChannel(parameterList.value(0)))
    {
        if(!isIgnore(prefix, Ignore::Channel))
        {
            Channel* channel = m_server->getChannelByName(parameterList.value(0));
            if(channel)
            {
                channel->append(source, message);

                if(source != m_server->getNickname())
                {
                    QRegExp regexp(QStringLiteral("(^|[^\\d\\w])") +
                            QRegExp::escape(m_server->loweredNickname()) +
                            QStringLiteral("([^\\d\\w]|$)"));
                    regexp.setCaseSensitivity(Qt::CaseInsensitive);
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
            NickInfoPtr nickinfo = m_server->obtainNickInfo(source);
            nickinfo->setHostmask(sourceHostmask);

            // Create a new query (server will check for dupes)
            Query* query = m_server->addQuery(nickinfo, false /*we didn't initiate this*/ );

            // send action to query
            query->appendQuery(source, message);

            if(source != m_server->getNickname() && query)
            {
                QRegExp regexp(QStringLiteral("(^|[^\\d\\w])") +
                        QRegExp::escape(m_server->loweredNickname()) +
                        QStringLiteral("([^\\d\\w]|$)"));
                regexp.setCaseSensitivity(Qt::CaseInsensitive);
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



// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
