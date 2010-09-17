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
#include "application.h"

#include <QClipboard>
#include <QResizeEvent>
#include <QTextCursor>
#include <QTextDocument>


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

    void TopicLabel::setChannelName(const QString& channel)
    {
        m_channelName = channel;
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
            if (link.startsWith('#') && m_server && m_server->isConnected())
            {
                QString channel(link);
                channel.replace("##","#");
                m_server->sendJoinCommand(channel);
            }
            // Always use KDE default mailer.
            else
                Application::openUrl(link);
        }
    }

    void TopicLabel::setContextMenuOptions(IrcContextMenus::MenuOptions options, bool on)
    {
        if (on)
            m_contextMenuOptions |= options;
        else
            m_contextMenuOptions &= ~options;
    }

    void TopicLabel::contextMenuEvent(QContextMenuEvent* ev)
    {
        if (m_isOnChannel && m_server)
        {
            IrcContextMenus::channelMenu(ev->globalPos(), m_server, m_currentChannel);

            m_isOnChannel = false;

            return;
        }

#if !(QT_VERSION >= QT_VERSION_CHECK(4, 7, 0))
        if (m_contextMenuOptions.testFlag(IrcContextMenus::ShowLinkActions))
        {
            IrcContextMenus::linkMenu(ev->globalPos(), m_urlToCopy);

            return;
        }

        QLabel::contextMenuEvent(ev);
#else
        int contextMenuActionId = IrcContextMenus::textMenu(ev->globalPos(), m_contextMenuOptions,
            m_server, selectedText(), m_urlToCopy);

        switch (contextMenuActionId)
        {
            case -1:
                break;
            case IrcContextMenus::TextCopy:
            {
                QClipboard* clipboard = qApp->clipboard();
                clipboard->setText(selectedText(), QClipboard::Clipboard);

                break;
            }
            case IrcContextMenus::TextSelectAll:
            {
                QTextDocument doc;
                doc.setHtml(text());

                setSelection(0, doc.toPlainText().length());

                break;
            }
            default:
                break;
        }
#endif
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
        // text.replace("&", "&amp;"). Not needed as we do it in tagUrls
        text.replace('<', "\x0blt;"). // tagUrls will replace \x0b with &
            replace('>', "\x0bgt;");

        text = tagUrls(text, m_channelName);

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
        }
        else
            m_lastStatusText = link;

        if (!link.startsWith('#'))
        {
            m_isOnChannel = false;

            if (!link.isEmpty()) {
                //link therefore != m_lastStatusText  so emit with this new text
                emit setStatusBarTempText(link);
            }
            if (link.isEmpty() && m_contextMenuOptions.testFlag(IrcContextMenus::ShowLinkActions))
                setContextMenuOptions(IrcContextMenus::ShowLinkActions, false);
            else if (!link.isEmpty() && !m_contextMenuOptions.testFlag(IrcContextMenus::ShowLinkActions))
            {
                m_urlToCopy = link;
                setContextMenuOptions(IrcContextMenus::ShowLinkActions, true);
            }
        }
        else if (link.startsWith(QLatin1String("##")))
        {
            m_currentChannel = link.mid(1);
            m_isOnChannel = true;
            emit setStatusBarTempText(i18n("Join the channel %1", m_currentChannel));
        }
    }

    QString TopicLabel::tagUrls(const QString& text, const QString& sender)
    {
        QString htmlText(removeIrcMarkup(text));
        QString link("<a href=\"#%1\">%2</a>");

        QString original;
        TextChannelData channelData = extractChannelData(htmlText);

        for (int i = channelData.channelRanges.count()-1; i >= 0 ; --i)
        {
            const QPair <int, int>& range = channelData.channelRanges.at(i);
            original = htmlText.mid(range.first, range.second);
            htmlText.replace(range.first, range.second, link.arg(channelData.fixedChannels.at(i), original));
        }

        link = "<a href=\"%1\">%2</a>";
        TextUrlData urlData = extractUrlData(htmlText);
        for (int i = urlData.urlRanges.count()-1; i >= 0 ; --i)
        {
            const QPair <int, int>& range = urlData.urlRanges.at(i);

            int pos = range.first;
            // check if the matched text is already replaced as a channel
            if (htmlText.lastIndexOf("<a", pos ) > htmlText.lastIndexOf("</a>", pos))
            {
                continue;
            }
            original = htmlText.mid(range.first, range.second);
            QString fixedUrl = urlData.fixedUrls.at(i);
            htmlText.replace(range.first, range.second, link.arg(fixedUrl, original));

            QMetaObject::invokeMethod(Application::instance(), "storeUrl", Qt::QueuedConnection,
                Q_ARG(QString, sender), Q_ARG(QString, fixedUrl), Q_ARG(QDateTime, QDateTime::currentDateTime()));
        }

        // Change & to &amp; to prevent html entities to do strange things to the text
        htmlText.replace('&', "&amp;");
        htmlText.replace("\x0b", "&");
        return htmlText;
    }
}

#include "topiclabel.moc"
