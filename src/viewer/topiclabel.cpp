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

#include <q3simplerichtext.h>
#include <qclipboard.h>
//Added by qt3to4:
#include <QContextMenuEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <Q3PopupMenu>
#include <QEvent>
#include <q3textedit.h>
#include <krun.h>
#include <k3process.h>
#include <kshell.h>
#include <k3urldrag.h>
#include <kstringhandler.h>
#include <kglobal.h>
#include <kdebug.h>
#include <kmenu.h>
#include <kiconloader.h>
#include <kbookmarkmanager.h>
#include <kdeversion.h>
#include <QTextDocument>


namespace Konversation
{

    TopicLabel::TopicLabel(QWidget *parent, const char *name)
        : QLabel(parent)
    {
        setObjectName(name);
        setWordWrap(true);
        setFocusPolicy(Qt::ClickFocus);

        m_isOnChannel = false;
        m_copyUrlMenu = false;
        mousePressed=false;

        m_popup = new Q3PopupMenu(this,"topiclabel_context_menu");
        m_popup->insertItem(SmallIconSet("editcopy"),i18n("&Copy"),Copy);
        m_popup->insertItem(i18n("Select All"),SelectAll);

        setupChannelPopupMenu();

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

    void TopicLabel::leaveEvent(QEvent*)
    {
       emit clearStatusBarTempText();
       m_lastStatusText = QString();
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
            else if (link.startsWith("#") && m_server && m_server->isConnected())
            {
                QString channel(link);
                channel.replace("##","#");
                m_server->sendJoinCommand(channel);
            }
            // Always use KDE default mailer.
            else if (!Preferences::useCustomBrowser() || link.toLower().startsWith("mailto:"))
            {
                new KRun(KUrl::fromPathOrUrl(link), this);
            }
            else
            {
                QString cmd = Preferences::webBrowserCmd();
                cmd.replace("%u",KUrl::fromPathOrUrl(link).url());
                K3Process *proc = new K3Process;
                QStringList cmdAndArgs = KShell::splitArgs(cmd);
                *proc << cmdAndArgs;
                //      This code will also work, but starts an extra shell process.
                //      kDebug() << "IRCView::linkClickSlot(): cmd = " << cmd << endl;
                //      *proc << cmd;
                //      proc->setUseShell(true);
                proc->start(K3Process::DontCare);
                delete proc;
            }
        }
    }

//     void TopicLabel::contentsContextMenuEvent(QContextMenuEvent* ev)
//     {
//         bool block = contextMenu(ev);
//
//         if(!block)
//         {
//             QLabel::contentsContextMenuEvent(ev);
//         }
//     }

//     bool TopicLabel::contextMenu(QContextMenuEvent* ce)
//     {
//         if (m_isOnChannel)
//         {
//             m_channelPopup->exec(ce->globalPos());
//             m_isOnChannel = false;
//         }
//         else
//         {
//             m_popup->setItemEnabled(Copy,(hasSelectedText()));
//
//             int r = m_popup->exec(ce->globalPos());
//
//             switch(r)
//             {
//                 case -1:
//                     // dummy. -1 means, no entry selected. we don't want -1to go in default, so
//                     // we catch it here
//                     break;
//                 case Copy:
//                     copy();
//                     break;
//                 case CopyUrl:
//                 {
//                     QClipboard *cb = KApplication::kApplication()->clipboard();
//                     cb->setText(m_urlToCopy,QClipboard::Selection);
//                     cb->setText(m_urlToCopy,QClipboard::Clipboard);
//                     break;
//                 }
//                 case SelectAll:
//                     selectAll();
//                     break;
//                 case Bookmark:
//                 {
//                     KBookmarkManager* bm = KBookmarkManager::userBookmarksManager();
//                     KBookmarkGroup bg = bm->addBookmarkDialog(m_urlToCopy, QString());
//                     bm->save();
//                     bm->emitChanged(bg);
//                     break;
//                 }
//                 default:
//                     break;
//             }
//         }
//         return true;
//     }

    void TopicLabel::setupChannelPopupMenu()
    {
/*        m_channelPopup = new KMenu(this,"channel_context_menu");
        m_channelPopupId = m_channelPopup->insertTitle(m_currentChannel);
        m_channelPopup->insertItem(i18n("&Join"),Konversation::Join);
        m_channelPopup->insertItem(i18n("Get &user list"),Konversation::Names);
        m_channelPopup->insertItem(i18n("Get &topic"),Konversation::Topic);

        connect(m_channelPopup, SIGNAL(activated(int)), this, SIGNAL(popupCommand(int)));*/
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
            QLabel::setText(QString::null);

            return;
        }

        QString text = m_fullText;
        // text.replace("&", "&amp;"). Not needed as we do it in tagURLs
        text.replace("<", "\x0blt;"). // tagUrls will replace \x0b with &
            replace(">", "\x0bgt;");
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

    QString TopicLabel::rPixelSqueeze(const QString& text, uint maxPixels)
    {
        uint tw = textWidth(text);

        if(tw > maxPixels)
        {
            QString tmp = text;
            const uint em = fontMetrics().maxWidth();
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

    uint TopicLabel::textWidth(const QString& text)
    {
        Q3SimpleRichText richText("<qt>" + text + "</qt>", font());
        richText.setWidth(fontMetrics().width(text));

        return richText.widthUsed();
    }

    void TopicLabel::highlightedSlot(const QString& _link)
    {
        QString link = KUrl::fromPathOrUrl(_link).url();
        //we just saw this a second ago.  no need to reemit.
        if (link == m_lastStatusText && !link.isEmpty())
            return;

        // remember current URL to overcome link clicking problems in QTextBrowser
        m_highlightedURL = link;

        if (link.isEmpty())
        {
            if (!m_lastStatusText.isEmpty())
            {
                emit clearStatusBarTempText();
                m_lastStatusText = QString();
            }
        } else {
            m_lastStatusText = link;
        }

        if (!link.startsWith("#"))
        {
            m_isOnChannel = false;

            if (!link.isEmpty()) {
                //link therefore != m_lastStatusText  so emit with this new text
                emit setStatusBarTempText(link);
            }
            if (link.isEmpty() && m_copyUrlMenu)
            {
                m_popup->removeItem(CopyUrl);
                m_popup->removeItem(Bookmark);
                m_copyUrlMenu = false;
            }
            else if (!link.isEmpty() && !m_copyUrlMenu)
            {
                m_popup->insertItem(SmallIcon("editcopy"), i18n("Copy URL to Clipboard"), CopyUrl, 0);
                m_popup->insertItem(SmallIcon("bookmark"), i18n("Add to Bookmarks"), Bookmark, 1);
                m_copyUrlMenu = true;
                m_urlToCopy = link;
            }
        }
        else if (link.startsWith("##"))
        {
            m_currentChannel = link.mid(1);

            emit currentChannelChanged(m_currentChannel);

            QString prettyId = m_currentChannel;

            if (prettyId.length()>15)
            {
                prettyId.truncate(15);
                prettyId.append("...");
            }

//             m_channelPopup->changeTitle(m_channelPopupId,prettyId);
            m_isOnChannel = true;
            emit setStatusBarTempText(i18n("Join the channel %1", m_currentChannel));
        }
    }
}

// #include "./viewer/topiclabel.moc"
