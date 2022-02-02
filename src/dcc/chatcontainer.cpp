/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#include "chatcontainer.h"

#include <preferences.h>
#include <ircinput.h>
#include <irccharsets.h>
#include <application.h>
#include <ircviewbox.h>
#include <topiclabel.h>
#include <ircview.h>
#include <notificationhandler.h>
#include "whiteboard.h"
#include "konversation_log.h"

#include <KLocalizedString>
#include <KMessageBox>
#include <KStandardGuiItem>

#include <QSplitter>

namespace Konversation
{
    namespace DCC
    {
        ChatContainer::ChatContainer(QWidget *parent, Chat *chat)
            : ChatWindow(parent),
              m_chat(chat),
              m_whiteBoard(nullptr)
        {
            setType(ChatWindow::DccChat);
            //dcc chat, not used here
            setServer(nullptr);
            setChannelEncodingSupported(true);
            setPartnerNick(m_chat->partnerNick());

            m_headerSplitter = new QSplitter(Qt::Vertical, this);
            m_topicLabel = new Konversation::TopicLabel(m_headerSplitter);
            m_headerSplitter->setStretchFactor(m_headerSplitter->indexOf(m_topicLabel), 0);

            // setup layout
            if (m_chat->extension() == Chat::Whiteboard)
            {
                auto* chatSplitter = new QSplitter(Qt::Vertical);

                m_whiteBoard = new WhiteBoard(chatSplitter);
                connect(m_whiteBoard, &WhiteBoard::rawWhiteBoardCommand, m_chat, &Chat::sendRawLine);
                connect(m_chat, &Chat::connected, m_whiteBoard, &WhiteBoard::connected);
                //chatSplitter->setStretchFactor(chatSplitter->indexOf(paintLabel), 1);

                auto *ircViewBox = new IRCViewBox(chatSplitter);
                //chatSplitter->setStretchFactor(chatSplitter->indexOf(ircViewBox), 1);
                setTextView(ircViewBox->ircView());

                m_headerSplitter->addWidget(chatSplitter);
            }
            else //(m_chat->extension() == Chat::SimpleChat || m_chat->extension() == Chat::Unknown)
            {
                auto *ircViewBox = new IRCViewBox(m_headerSplitter);
                m_headerSplitter->setStretchFactor(m_headerSplitter->indexOf(ircViewBox), 1);
                setTextView(ircViewBox->ircView());
            }

            m_inputBar = new IRCInput(this);
            getTextView()->installEventFilter(m_inputBar);
            m_inputBar->setReadOnly(true);

            connect(m_chat, &Chat::receivedRawLine, this, &ChatContainer::receivedLine);
            connect(m_chat, &Chat::statusChanged, this, &ChatContainer::chatStatusChanged);
            connect(m_chat, &Chat::upnpError, this, &ChatContainer::upnpError);

            connect(m_inputBar, &IRCInput::submit, this, &ChatContainer::textEntered);
            connect(m_inputBar, &IRCInput::textPasted, this, &ChatContainer::textPasted);

            connect(getTextView(), &IRCView::textPasted, m_inputBar, &IRCInput::paste);
            connect(getTextView(), &IRCView::gotFocus, m_inputBar, QOverload<>::of(&IRCInput::setFocus));
            connect(getTextView(), &IRCView::autoText, this, &ChatContainer::textPasted);

            updateAppearance();
        }

        ChatContainer::~ChatContainer()
        {
            if (m_chat)
            {
                //Do not delete the chat, its the transfermanagers job
                disconnect(m_chat, nullptr, nullptr, nullptr);
                m_chat->close();
                m_chat->removedFromView();
                m_chat = nullptr;
            }
        }

        void ChatContainer::chatStatusChanged(Chat *chat, Konversation::DCC::Chat::Status newstatus, Konversation::DCC::Chat::Status oldstatus)
        {
            Q_UNUSED(oldstatus)
            Q_UNUSED(chat)

            Q_ASSERT(newstatus != oldstatus);
            Q_ASSERT(m_chat == chat);

            switch (newstatus)
            {
                case Chat::WaitingRemote:
                    m_topicLabel->setText(i18nc("%1=extension like Chat or Whiteboard, %2=partnerNick, %3=port",
                                                "DCC %1 with %2 on port %3.",
                                                m_chat->localizedExtensionString(), m_chat->partnerNick(), QString::number(m_chat->ownPort())));
                    getTextView()->appendServerMessage(i18n("DCC"), m_chat->statusDetails());
                    break;

                case Chat::Connecting:
                    m_topicLabel->setText(i18nc("%1=extension like Chat or Whiteboard, %2 = nickname, %3 = IP, %4 = port",
                                                "DCC %1 with %2 on %3:%4.",
                                                m_chat->localizedExtensionString(), m_chat->partnerNick(), m_chat->partnerIp(), QString::number(m_chat->partnerPort())));
                    getTextView()->appendServerMessage(i18n("DCC"), m_chat->statusDetails());
                    break;

                case Chat::Chatting:
                    getTextView()->appendServerMessage(i18n("DCC"), m_chat->statusDetails());
                    m_inputBar->setReadOnly(false);
                    // KTextEdit::setReadOnly(true) from the ChatContainer constructor fucked up the palette.
                    m_inputBar->updateAppearance();
                    break;
                case Chat::Failed:
                default:
                    getTextView()->appendServerMessage(i18n("DCC"), m_chat->statusDetails());
                    m_inputBar->setReadOnly(true);
                    break;
            }

        }

        void ChatContainer::upnpError(const QString &errorMessage)
        {
            getTextView()->appendServerMessage(i18nc("Universal Plug and Play", "UPnP"), errorMessage);
        }

        void ChatContainer::setPartnerNick (const QString &nick)
        {
            ChatWindow::setName(QLatin1Char('-') + nick + QLatin1Char('-'));
            ChatWindow::setLogfileName(QLatin1Char('-') + nick + QLatin1Char('-'));
            m_chat->setPartnerNick(nick);
        }

        QString ChatContainer::ownNick() const
        {
            return m_chat->ownNick();
        }

        bool ChatContainer::canBeFrontView() const
        {
            return true;
        }

        void ChatContainer::childAdjustFocus()
        {
            m_inputBar->setFocus();
        }

        bool ChatContainer::closeYourself(bool askForConfirmation)
        {
            // if chat is already closed don't ask
            if (m_chat->status() >= Chat::Closed)
            {
                deleteLater();
                return true;
            }

            int result = KMessageBox::Continue;
            if (askForConfirmation)
            {
                result = KMessageBox::warningContinueCancel(this,
                                                            i18nc("%1=extension like Chat or Whiteboard, %2=partnerNick",
                                                                  "Do you want to close your DCC %1 with %2?",
                                                                  m_chat->localizedExtensionString(), m_chat->partnerNick()),
                                                            i18nc("%1=extension like Chat or Whiteboard",
                                                                  "Close DCC %1", m_chat->localizedExtensionString()),
                                                            KStandardGuiItem::close(),
                                                            KStandardGuiItem::cancel(),
                                                            QStringLiteral("QuitDCCChatTab"));

                if (result == KMessageBox::Continue)
                {
                    deleteLater();
                    return true;
                }
            }
            return false;
        }

        void ChatContainer::emitUpdateInfo()
        {
            //qCDebug(KONVERSATION_LOG) << __FUNCTION__;
            QString info;
            if (m_chat && m_chat->partnerNick() == m_chat->ownNick())
                info = i18n("Talking to yourself");
            else if (m_chat)
                info = m_chat->ownNick();
            else
                info = getName();

            Q_EMIT updateInfo(info);
        }

        QString ChatContainer::getChannelEncoding() const
        {
            return m_chat->getEncoding();
        }

        QString ChatContainer::getChannelEncodingDefaultDesc() const
        {
            return i18nc("%1=Encoding","Default ( %1 )", Konversation::IRCCharsets::self()->encodingForLocale());
        }

        bool ChatContainer::searchView() const
        {
            return true;
        }

        void ChatContainer::setChannelEncoding(const QString &encoding)
        {
            m_chat->setEncoding(encoding);
        }

        void ChatContainer::textEntered()
        {
            const QString &line = sterilizeUnicode(m_inputBar->toPlainText());

            if (line.isEmpty())
            {
                return;
            }

            const QString &cc = Preferences::self()->commandChar();
            if (line.startsWith(cc))
            {
                const QString cmd = line.section(QLatin1Char(' '), 0, 0).toLower();
                qCDebug(KONVERSATION_LOG) << "cmd" << cmd;
                if (cmd == cc + QLatin1String("clear")) {
                    textView->clear();
                }
                else if (cmd == cc + QLatin1String("me")) {
                    const QString toSend = line.section(QLatin1Char(' '), 1);
                    //qCDebug(KONVERSATION_LOG) << "toSend" << toSend;
                    if (toSend.isEmpty())
                    {
                        getTextView()->appendServerMessage(i18n("Usage"), i18n("Usage: %1ME text", Preferences::self()->commandChar()));
                    }
                    else
                    {
                        m_chat->sendAction(toSend);
                        appendAction(m_chat->ownNick(), toSend);
                    }
                }
                else if (cmd == cc + QLatin1String("close")) {
                    closeYourself(false);
                }
                else
                {
                    getTextView()->appendServerMessage(i18n("Error"), i18n("Unknown command."));
                }
            }
            else
            {
                textPasted(line);
            }
            m_inputBar->clear();
        }

        void ChatContainer::textPasted(const QString &text)
        {
            if (text.isEmpty())
            {
                return;
            }

            const QStringList lines = sterilizeUnicode(text.split(QLatin1Char('\n'), Qt::SkipEmptyParts));

            for (const QString &line : lines) {
                m_chat->sendText(line);
                getTextView()->append(m_chat->ownNick(), line);
            }
        }

        void ChatContainer::receivedLine(const QString &_line)
        {
            const QString line(sterilizeUnicode(_line));
            if (line.startsWith(QLatin1Char('\x01'))) {
                // cut out the CTCP command
                const QString ctcp = line.mid(1, line.indexOf(1, 1) - 1);

                const QString ctcpCommand = ctcp.section(QLatin1Char(' '), 0, 0);
                QString ctcpArgument = ctcp.section(QLatin1Char(' '), 1);
                OutputFilter::replaceAliases(ctcpArgument);

                if (ctcpCommand.toLower() == QLatin1String("action"))
                {
                    appendAction(m_chat->partnerNick(), ctcpArgument);
                }
                else if (m_chat->extension() == Chat::Whiteboard && m_whiteBoard && WhiteBoard::whiteboardCommands().contains(ctcpCommand.toUpper()))
                {
                    m_whiteBoard->receivedWhiteBoardLine(ctcp);
                }
                else
                {
                    appendServerMessage(i18n("CTCP"), i18n("Received unknown CTCP-%1 request from %2", ctcp, m_chat->partnerNick()));
                }
            }
            else
            {
                getTextView()->append(m_chat->partnerNick(), line);

                const QRegularExpression regexp(QLatin1String("(^|[^\\d\\w])")
                                                + QRegularExpression::escape(m_chat->ownNick())
                                                + QLatin1String("([^\\d\\w]|$)"),
                                                QRegularExpression::CaseInsensitiveOption);

                if(line.contains(regexp))
                {
                    Application::instance()->notificationHandler()->nick(this,
                            m_chat->partnerNick(), line);
                }
                else
                {
                    Application::instance()->notificationHandler()->message(this,
                            m_chat->partnerNick(), line);
                }
            }
        }
    }
}
