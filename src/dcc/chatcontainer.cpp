/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#include "chatcontainer.h"

#include <preferences.h>
#include <ircinput.h>
#include <irccharsets.h>
#include <application.h>
#include <ircviewbox.h>
#include <topiclabel.h>
#include <ircview.h>
#include "whiteboard.h"

#include <QSplitter>

#include <KLocalizedString>
#include <KActionCollection>
#include <KMessageBox>
#include <KStandardGuiItem>

namespace Konversation
{
    namespace DCC
    {
        ChatContainer::ChatContainer(QWidget *parent, Chat *chat)
            : ChatWindow(parent),
              m_chat(chat),
              m_whiteBoard(0)
        {
            setType(ChatWindow::DccChat);
            //dcc chat, not used here
            setServer(0);
            setChannelEncodingSupported(true);
            setPartnerNick(m_chat->partnerNick());

            m_headerSplitter = new QSplitter(Qt::Vertical, this);
            m_topicLabel = new Konversation::TopicLabel(m_headerSplitter);
            m_headerSplitter->setStretchFactor(m_headerSplitter->indexOf(m_topicLabel), 0);

            // setup layout
            if (m_chat->extension() == Chat::Whiteboard)
            {
                QSplitter* chatSplitter = new QSplitter(Qt::Vertical);

                m_whiteBoard = new WhiteBoard(chatSplitter);
                connect(m_whiteBoard, SIGNAL(rawWhiteBoardCommand(QString)),
                        m_chat, SLOT(sendRawLine(QString)));
                connect(m_chat, SIGNAL(connected()), m_whiteBoard, SLOT(connected()));
                //chatSplitter->setStretchFactor(chatSplitter->indexOf(paintLabel), 1);

                IRCViewBox *ircViewBox = new IRCViewBox(chatSplitter);
                //chatSplitter->setStretchFactor(chatSplitter->indexOf(ircViewBox), 1);
                setTextView(ircViewBox->ircView());

                m_headerSplitter->addWidget(chatSplitter);
            }
            else //(m_chat->extension() == Chat::SimpleChat || m_chat->extension() == Chat::Unknown)
            {
                IRCViewBox *ircViewBox = new IRCViewBox(m_headerSplitter);
                m_headerSplitter->setStretchFactor(m_headerSplitter->indexOf(ircViewBox), 1);
                setTextView(ircViewBox->ircView());
            }

            m_inputBar = new IRCInput(this);
            getTextView()->installEventFilter(m_inputBar);
            m_inputBar->setReadOnly(true);

            connect(m_chat, SIGNAL(receivedRawLine(QString)), this, SLOT(receivedLine(QString)));
            connect(m_chat, SIGNAL(statusChanged(Konversation::DCC::Chat*,Konversation::DCC::Chat::Status,Konversation::DCC::Chat::Status)),
                    this, SLOT(chatStatusChanged(Konversation::DCC::Chat*,Konversation::DCC::Chat::Status,Konversation::DCC::Chat::Status)));
            connect(m_chat, SIGNAL(upnpError(QString)), this, SLOT(upnpError(QString)));

            connect(m_inputBar, SIGNAL(submit()), this, SLOT(textEntered()));
            connect(m_inputBar, SIGNAL(textPasted(QString)), this, SLOT(textPasted(QString)));

            connect(getTextView(), SIGNAL(textPasted(bool)), m_inputBar, SLOT(paste(bool)));
            connect(getTextView(), SIGNAL(gotFocus()), m_inputBar, SLOT(setFocus()));
            connect(getTextView(), SIGNAL(autoText(QString)), this, SLOT(textPasted(QString)));

            updateAppearance();
        }

        ChatContainer::~ChatContainer()
        {
            if (m_chat)
            {
                //Do not delete the chat, its the transfermanagers job
                disconnect(m_chat, 0, 0, 0);
                m_chat->close();
                m_chat->removedFromView();
                m_chat = 0;
            }
        }

        void ChatContainer::chatStatusChanged(Chat *chat, Konversation::DCC::Chat::Status newstatus, Konversation::DCC::Chat::Status oldstatus)
        {
            Q_UNUSED(oldstatus);
            Q_UNUSED(chat);

            Q_ASSERT(newstatus != oldstatus);
            Q_ASSERT(m_chat == chat);

            switch (newstatus)
            {
                case Chat::WaitingRemote:
                    m_topicLabel->setText(i18nc("%1=extension like Chat or Whiteboard, %2=partnerNick, %3=port",
                                                "DCC %1 with %2 on port <numid>%3</numid>.",
                                                m_chat->localizedExtensionString(), m_chat->partnerNick(), m_chat->ownPort()));
                    getTextView()->appendServerMessage(i18n("DCC"), m_chat->statusDetails());
                    break;

                case Chat::Connecting:
                    m_topicLabel->setText(i18nc("%1=extension like Chat or Whiteboard, %2 = nickname, %3 = IP, %4 = port",
                                                "DCC %1 with %2 on %3:<numid>%4</numid>.",
                                                m_chat->localizedExtensionString(), m_chat->partnerNick(), m_chat->partnerIp(), m_chat->partnerPort()));
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
            ChatWindow::setName('-' + nick + '-');
            ChatWindow::setLogfileName('-' + nick + '-');
            m_chat->setPartnerNick(nick);
        }

        QString ChatContainer::ownNick() const
        {
            return m_chat->ownNick();
        }

        bool ChatContainer::canBeFrontView()
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
                                                            "QuitDCCChatTab");

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
            //kDebug();
            QString info;
            if (m_chat && m_chat->partnerNick() == m_chat->ownNick())
                info = i18n("Talking to yourself");
            else if (m_chat)
                info = m_chat->ownNick();
            else
                info = getName();

            emit updateInfo(info);
        }

        QString ChatContainer::getChannelEncoding()
        {
            return m_chat->getEncoding();
        }

        QString ChatContainer::getChannelEncodingDefaultDesc()
        {
            return i18nc("%1=Encoding","Default ( %1 )", Konversation::IRCCharsets::self()->encodingForLocale());
        }

        bool ChatContainer::searchView()
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
                QString cmd = line.section(' ', 0, 0).toLower();
                kDebug() << "cmd" << cmd;
                if (cmd == cc + "clear")
                {
                    textView->clear();
                }
                else if (cmd == cc + "me")
                {
                    QString toSend = line.section(' ', 1);
                    //kDebug() << "toSend" << toSend;
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
                else if (cmd == cc + "close")
                {
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

            const QStringList lines = sterilizeUnicode(text.split('\n', QString::SkipEmptyParts));

            foreach (const QString &line, lines)
            {
                m_chat->sendText(line);
                getTextView()->append(m_chat->ownNick(), line);
            }
        }

        void ChatContainer::receivedLine(const QString &_line)
        {
            const QString line(sterilizeUnicode(_line));
            if (line.startsWith('\x01'))
            {
                // cut out the CTCP command
                const QString ctcp = line.mid(1, line.indexOf(1, 1) - 1);

                const QString ctcpCommand = ctcp.section(' ', 0, 0);
                QString ctcpArgument(ctcp.section(' ', 1));
                OutputFilter::replaceAliases(ctcpArgument);

                if (ctcpCommand.toLower() == "action")
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
            }
        }
    }
}
