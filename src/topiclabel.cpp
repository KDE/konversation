/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2004 Peter Simonsson <psn@linux.se>
  Copyright (C) 2006 Eike Hein <sho@eikehein.com>
*/

#include "topiclabel.h"

#include <qsimplerichtext.h>
#include <qtooltip.h>
#include <qclipboard.h>

#include <krun.h>
#include <kprocess.h>
#include <kshell.h>
#include <kurldrag.h>
#include <kstringhandler.h>
#include <kglobal.h>
#include <kdebug.h>
#include <kpopupmenu.h>
#include <kiconloader.h>
#include <kbookmarkmanager.h>

#include "konversationapplication.h"
#include "server.h"
#include "common.h"
#include "channel.h"

#include <kdeversion.h>

namespace Konversation
{

    TopicLabel::TopicLabel(QWidget *parent, const char *name)
        : KActiveLabel(parent, name)
    {
        setWrapPolicy(QTextEdit::AtWordOrDocumentBoundary);
        setFocusPolicy(QWidget::ClickFocus);

        m_isOnChannel = false;
        m_copyUrlMenu = false;
        mousePressed=false;

        m_popup = new QPopupMenu(this,"topiclabel_context_menu");
        m_popup->insertItem(SmallIconSet("editcopy"),i18n("&Copy"),Copy);
        m_popup->insertItem(i18n("Select All"),SelectAll);

        setupChannelPopupMenu();

        connect(this, SIGNAL(highlighted(const QString&)), this, SLOT(highlightedSlot(const QString&)));
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
        int minHeight = fontMetrics().lineSpacing() + fontMetrics().descent();
        return QSize(0, minHeight);
    }

    void TopicLabel::setServer(Server* server)
    {
        m_server = server;
    }

    void TopicLabel::contentsMousePressEvent(QMouseEvent *e)
    {
        if (e->button()==QMouseEvent::LeftButton)
        {
            pressPosition=e->pos();
            // Hack to counter the fact that we're given an decoded url
            urlToDrag = KURL::fromPathOrURL(anchorAt(pressPosition)).url();
            if (!urlToDrag.isNull())
            {
                mousePressed=true;
                return;
            }
        }
        KActiveLabel::contentsMousePressEvent(e);
    }

    void TopicLabel::contentsMouseReleaseEvent(QMouseEvent *e)
    {
        if (e->button()==QMouseEvent::LeftButton)
        {
            if (mousePressed) openLink(urlToDrag);
            mousePressed=false;
        }
        KActiveLabel::contentsMouseReleaseEvent(e);
    }

    void TopicLabel::contentsMouseMoveEvent(QMouseEvent *e)
    {
        if (mousePressed && (pressPosition-e->pos()).manhattanLength() > QApplication::startDragDistance())
        {
            mousePressed=false;
            removeSelection();
            KURL ux = KURL::fromPathOrURL(urlToDrag);
            if (urlToDrag.startsWith("##")) ux=QString("irc://%1:%2/%3").arg(m_server->getServerName()).
                    arg(m_server->getPort()).arg(urlToDrag.mid(2));
            KURLDrag* u=new KURLDrag(ux,viewport());
            u->drag();
        }
        KActiveLabel::contentsMouseMoveEvent(e);
    }

    void TopicLabel::leaveEvent(QEvent*)
    {
       emit clearStatusBarTempText();
       m_lastStatusText = QString::null;
    }

    void TopicLabel::openLink(const QString& link)
    {
        if (!link.isEmpty())
        {
            if (link.startsWith("#") && m_server && m_server->isConnected())
            {
                QString channel(link);
                channel.replace("##","#");
                m_server->sendJoinCommand(channel);
            }
            // Always use KDE default mailer.
            else if (!Preferences::useCustomBrowser() || link.lower().startsWith("mailto:"))
            {
                new KRun(KURL::fromPathOrURL(link));
            }
            else
            {
                QString cmd = Preferences::webBrowserCmd();
                cmd.replace("%u",KURL::fromPathOrURL(link).url());
                KProcess *proc = new KProcess;
                QStringList cmdAndArgs = KShell::splitArgs(cmd);
                *proc << cmdAndArgs;
                //      This code will also work, but starts an extra shell process.
                //      kdDebug() << "IRCView::linkClickSlot(): cmd = " << cmd << endl;
                //      *proc << cmd;
                //      proc->setUseShell(true);
                proc->start(KProcess::DontCare);
                delete proc;
            }
        }
    }

    void TopicLabel::contentsContextMenuEvent(QContextMenuEvent* ev)
    {
        bool block = contextMenu(ev);

        if(!block)
        {
            KActiveLabel::contentsContextMenuEvent(ev);
        }
    }

    bool TopicLabel::contextMenu(QContextMenuEvent* ce)
    {
        if (m_isOnChannel)
        {
            m_channelPopup->exec(ce->globalPos());
            m_isOnChannel = false;
        }
        else
        {
            m_popup->setItemEnabled(Copy,(hasSelectedText()));

            int r = m_popup->exec(ce->globalPos());

            switch(r)
            {
                case -1:
                    // dummy. -1 means, no entry selected. we don't want -1to go in default, so
                    // we catch it here
                    break;
                case Copy:
                    copy();
                    break;
                case CopyUrl:
                {
                    QClipboard *cb = KApplication::kApplication()->clipboard();
                    cb->setText(m_urlToCopy,QClipboard::Selection);
                    cb->setText(m_urlToCopy,QClipboard::Clipboard);
                    break;
                }
                case SelectAll:
                    selectAll();
                    break;
                case Bookmark:
                {
                    KBookmarkManager* bm = KBookmarkManager::userBookmarksManager();
                    KBookmarkGroup bg = bm->addBookmarkDialog(m_urlToCopy, QString::null);
                    bm->save();
                    bm->emitChanged(bg);
                    break;
                }
                default:
                    break;
            }
        }
        return true;
    }

    void TopicLabel::setupChannelPopupMenu()
    {
        m_channelPopup = new KPopupMenu(this,"channel_context_menu");
        m_channelPopupId = m_channelPopup->insertTitle(m_currentChannel);
        m_channelPopup->insertItem(i18n("&Join"),Konversation::Join);
        m_channelPopup->insertItem(i18n("Get &user list"),Konversation::Names);
        m_channelPopup->insertItem(i18n("Get &topic"),Konversation::Topic);

        connect(m_channelPopup, SIGNAL(activated(int)), this, SIGNAL(popupCommand(int)));
    }

    void TopicLabel::setText(const QString& text)
    {
        m_fullText = text;
        updateSqueezedText();
    }

    void TopicLabel::updateSqueezedText()
    {
        QFontMetrics fm(currentFont());
        QString text = m_fullText;
        // text.replace("&", "&amp;"). Not needed as we do it in tagURLs
        text.replace("<", "\x0blt;"). // tagUrls will replace \x0b with &
            replace(">", "\x0bgt;");
        text = tagURLs(text, "", false);

        QToolTip::remove(this);

        if(height() < (fm.lineSpacing() * 2))
        {
            text = rPixelSqueeze(text, visibleWidth() - 10);
            setWordWrap(NoWrap);
            QToolTip::add(this, "<qt>" + QStyleSheet::escape(m_fullText) + "</qt>");
        }
        else
        {
            setWordWrap(WidgetWidth);

            if(height() < contentsHeight())
            {
                QToolTip::add(this, "<qt>" + QStyleSheet::escape(m_fullText) + "</qt>");
            }
        }

        KActiveLabel::setText("<qt>" + text + "</qt>");
    }

    void TopicLabel::resizeEvent(QResizeEvent* ev)
    {
        KActiveLabel::resizeEvent(ev);
        updateSqueezedText();
    }

    QString TopicLabel::rPixelSqueeze(const QString& text, uint maxPixels)
    {
        QFontMetrics fm(currentFont());
        uint tw = textWidth(text, fm);

        if(tw > maxPixels)
        {
            QString tmp = text;
            const uint em = fm.maxWidth();
            maxPixels -= fm.width("...");
            int len, delta;

            while((tw > maxPixels) && !tmp.isEmpty())
            {
                len = tmp.length();
                delta = (tw - maxPixels) / em;
                delta = kClamp(delta, 1, len);

                tmp.remove(len - delta, delta);
                tw = textWidth(tmp, fm);
            }

            return tmp.append("...");
        }

        return text;
    }

    uint TopicLabel::textWidth(const QString& text, const QFontMetrics& fm)
    {
        QSimpleRichText richText("<qt>" + text + "</qt>", currentFont());
        richText.setWidth(fm.width(text));

        return richText.widthUsed();
    }

    void TopicLabel::highlightedSlot(const QString& _link)
    {
        QString link = KURL::fromPathOrURL(_link).url();
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
                m_lastStatusText = QString::null;
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
            else if(!link.isEmpty() && !m_copyUrlMenu)
            {
                m_popup->insertItem(i18n("Copy URL to Clipboard"),CopyUrl,1);
                m_popup->insertItem(i18n("Add to Bookmarks"),Bookmark,2);
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

            m_channelPopup->changeTitle(m_channelPopupId,prettyId);
            m_isOnChannel = true;
            emit setStatusBarTempText(i18n("Join the channel %1").arg(m_currentChannel));
        }
    }
}

#include "topiclabel.moc"
