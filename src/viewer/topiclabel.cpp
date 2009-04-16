/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2004, 2009 Peter Simonsson <peter.simonsson@gmail.com>
  Copyright (C) 2006-2008 Eike Hein <hein@kde.org>
*/

#include "topiclabel.h"
#include "application.h" ////// header renamed
#include "connectionmanager.h"
#include "server.h"
#include "common.h"
#include "channel.h"

#include <QClipboard>
#include <QContextMenuEvent>
#include <QResizeEvent>
#include <QMenu>
#include <QEvent>
#include <QTextDocument>

#include <krun.h>
#include <kprocess.h>
#include <kshell.h>
#include <kstringhandler.h>
#include <kglobal.h>
#include <kdebug.h>
#include <kmenu.h>
#include <kiconloader.h>
#include <kbookmarkmanager.h>
#include <kbookmarkdialog.h>
#include <klocale.h>


namespace Konversation
{

    TopicLabel::TopicLabel(QWidget *parent, const char *name)
        : QLabel(parent)
    {
        setObjectName(name);
        setWordWrap(true);
        setFocusPolicy(Qt::ClickFocus);
        setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        setTextInteractionFlags(Qt::TextBrowserInteraction);

        m_isOnChannel = false;
        m_copyUrlMenu = false;
        m_server = NULL;

        connect(this, SIGNAL(linkActivated(const QString&)), this, SLOT(openLink (const QString&)));
        connect(this, SIGNAL(linkHovered(const QString&)), this, SLOT(highlightedSlot(const QString&)));
    }

    TopicLabel::~TopicLabel()
    {
    }

    QSize TopicLabel::minimumSizeHint() const
    {
        int minHeight = fontMetrics().lineSpacing() + fontMetrics().descent();
        return QSize(0, minHeight);
    }

    QSize TopicLabel::sizeHint() const
    {
        return minimumSizeHint();
    }

    void TopicLabel::setServer(Server* server)
    {
        m_server = server;
    }

    void TopicLabel::leaveEvent(QEvent* e)
    {
       emit clearStatusBarTempText();
       m_lastStatusText.clear();
       QLabel::leaveEvent(e);
    }

    void TopicLabel::openLink(const QString& link)
    {
        if (!link.isEmpty())
        {
            if (link.startsWith("irc://"))
            {
                KonversationApplication* konvApp = static_cast<KonversationApplication*>(kapp);
                konvApp->getConnectionManager()->connectTo(Konversation::SilentlyReuseConnection, link);
            }
            else if (link.startsWith('#') && m_server && m_server->isConnected())
            {
                QString channel(link);
                channel.replace("##","#");
                m_server->sendJoinCommand(channel);
            }
            // Always use KDE default mailer.
            else if (!Preferences::self()->useCustomBrowser() || link.toLower().startsWith("mailto:"))
            {
                new KRun(KUrl(link), this);
            }
            else
            {
                QString cmd = Preferences::webBrowserCmd();
                cmd.replace("%u",KUrl(link).url());
                KProcess *proc = new KProcess;
                QStringList cmdAndArgs = KShell::splitArgs(cmd);
                *proc << cmdAndArgs;
                proc->startDetached();
                delete proc;
            }
        }
    }

    void TopicLabel::contextMenuEvent(QContextMenuEvent* ev)
    {
        bool block = contextMenu(ev);

        if(!block)
        {
            QLabel::contextMenuEvent(ev);
        }
    }

    bool TopicLabel::contextMenu(QContextMenuEvent* ce)
    {
        QMenu* menu = new QMenu(this);
        bool actionsAdded = false;

        if (m_isOnChannel)
        {
            m_isOnChannel = false;

            if (!m_currentChannel.isEmpty())
            {
                menu->addAction(i18n("&Join"), this, SLOT(joinChannel()));
                menu->addAction(i18n("Get &user list"), this, SLOT (getChannelUserList()));
                menu->addAction(i18n("Get &topic"), this, SLOT(getChannelTopic()));
                actionsAdded = true;
            }
        }
        else
        {
            if (m_copyUrlMenu)
            {
                menu->addAction(KIcon("edit-copy"), i18n("Copy URL to Clipboard"), this, SLOT (copyUrl()));
                menu->addAction(KIcon("bookmark-new"), i18n("Add to Bookmarks"), this, SLOT (bookmarkUrl()));
                actionsAdded = true;
            }
        }

        if (actionsAdded)
            menu->exec(ce->globalPos());

        delete menu;

        return actionsAdded;
    }

    void TopicLabel::setText(const QString& text)
    {
        m_fullText = text;
        updateSqueezedText();
    }

    void TopicLabel::updateSqueezedText()
    {
        setToolTip(QString());

        if (m_fullText.isEmpty())
        {
            QLabel::setText(QString());

            return;
        }

        QString text = m_fullText;
        // text.replace("&", "&amp;"). Not needed as we do it in tagURLs
        text.replace('<', "\x0blt;"). // tagUrls will replace \x0b with &
            replace('>', "\x0bgt;");
        text = tagURLs(text, "", false);

        if(height() < (fontMetrics().lineSpacing() * 2))
        {
            text = rPixelSqueeze(text, width() - 10);
            setWordWrap(false);
        }
        else
        {
            setWordWrap(true);
        }

        setToolTip("<qt>" + Qt::escape(m_fullText) + "</qt>");
        QLabel::setText("<qt>" + text + "</qt>");
    }

    void TopicLabel::resizeEvent(QResizeEvent* ev)
    {
        QLabel::resizeEvent(ev);
        updateSqueezedText();
    }

    QString TopicLabel::rPixelSqueeze(const QString& text, int maxPixels)
    {
        int tw = textWidth(text);

        if(tw > maxPixels)
        {
            QString tmp = text;
            int em = fontMetrics().maxWidth();
            maxPixels -= fontMetrics().width("...");
            int len, delta;

            while((tw > maxPixels) && !tmp.isEmpty())
            {
                len = tmp.length();
                delta = (tw - maxPixels) / em;
                delta = qBound(1, delta, len);

                tmp.remove(len - delta, delta);
                tw = textWidth(tmp);
            }

            return tmp.append("...");
        }

        return text;
    }

    int TopicLabel::textWidth(const QString& text)
    {
        QTextDocument document(this);
        document.setDefaultFont(font());
        document.setHtml("<qt>" + text + "</qt>");

        return document.size().toSize().width();
    }

    void TopicLabel::highlightedSlot(const QString& link)
    {
        //we just saw this a second ago.  no need to reemit.
        if (link == m_lastStatusText && !link.isEmpty())
            return;

        if (link.isEmpty())
        {
            if (!m_lastStatusText.isEmpty())
            {
                emit clearStatusBarTempText();
                m_lastStatusText.clear();
            }
        } else {
            m_lastStatusText = link;
        }

        if (!link.startsWith('#'))
        {
            m_isOnChannel = false;

            if (!link.isEmpty()) {
                //link therefore != m_lastStatusText  so emit with this new text
                emit setStatusBarTempText(link);
            }
            if (link.isEmpty() && m_copyUrlMenu)
            {
                m_copyUrlMenu = false;
            }
            else if (!link.isEmpty() && !m_copyUrlMenu)
            {
                m_copyUrlMenu = true;
                m_urlToCopy = link;
            }
        }
        else if (link.startsWith("##"))
        {
            m_currentChannel = link.mid(1);
            m_isOnChannel = true;
            emit setStatusBarTempText(i18n("Join the channel %1", m_currentChannel));
        }
    }

    void TopicLabel::copyUrl()
    {
        if (m_urlToCopy.isEmpty())
            return;

        QClipboard *cb = QApplication::clipboard();
        cb->setText(m_urlToCopy,QClipboard::Selection);
        cb->setText(m_urlToCopy,QClipboard::Clipboard);
    }

    void TopicLabel::bookmarkUrl()
    {
        if (m_urlToCopy.isEmpty())
            return;

        KBookmarkManager* bm = KBookmarkManager::userBookmarksManager();
        KBookmarkDialog* dialog = new KBookmarkDialog(bm, this);
        dialog->addBookmark(m_urlToCopy, m_urlToCopy);
        delete dialog;
    }

    void TopicLabel::joinChannel()
    {
        if(m_currentChannel.isEmpty() || !m_server || !m_server->isConnected())
            return;

        m_server->sendJoinCommand(m_currentChannel);
    }

    void TopicLabel::getChannelUserList()
    {
        if(m_currentChannel.isEmpty() || !m_server || !m_server->isConnected())
            return;

        m_server->queue("NAMES " + m_currentChannel, Server::LowPriority);
    }

    void TopicLabel::getChannelTopic()
    {
        if(m_currentChannel.isEmpty() || !m_server || !m_server->isConnected())
            return;

        m_server->requestTopic(m_currentChannel);
    }
}

#include "topiclabel.moc"
