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

        m_mousePressedOnUrl = false;
        m_isOnChannel = false;
        m_server = NULL;

        connect(this, SIGNAL(linkActivated(QString)), this, SLOT(openLink(QString)));
        connect(this, SIGNAL(linkHovered(QString)), this, SLOT(highlightedSlot(QString)));
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
        }
        else
        {
#if !(QT_VERSION >= QT_VERSION_CHECK(4, 7, 0))
            if (m_contextMenuOptions.testFlag(IrcContextMenus::ShowLinkActions))
            {
                IrcContextMenus::linkMenu(ev->globalPos(), m_currentUrl);
            }
            else
                QLabel::contextMenuEvent(ev);
#else
            int contextMenuActionId = IrcContextMenus::textMenu(ev->globalPos(), m_contextMenuOptions,
                m_server, selectedText(), m_currentUrl);

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

        resetLinkHighlightState();
    }

    void TopicLabel::mousePressEvent(QMouseEvent* ev)
    {
        if (ev->button() == Qt::LeftButton)
        {
            if (!m_currentUrl.isEmpty())
            {
                m_mousePressedOnUrl = true;
                m_mousePressPosition = ev->pos();

                // We need to keep a copy of the current URL because by the time
                // the Manhatten length is reached and the drag is initiated the
                // cursor may have left the link, causing m_currentUrl to be
                // cleared.
                m_dragUrl = m_currentUrl;
            }
        }

        QLabel::mousePressEvent(ev);
    }

    void TopicLabel::mouseReleaseEvent(QMouseEvent *ev)
    {
        if (ev->button() == Qt::LeftButton)
        {
            m_mousePressedOnUrl = false;
        }

        QLabel::mouseReleaseEvent(ev);
    }

    void TopicLabel::mouseMoveEvent(QMouseEvent* ev)
    {
        if (m_mousePressedOnUrl && (m_mousePressPosition - ev->pos()).manhattanLength() > KApplication::startDragDistance())
        {
#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
            setSelection(0, 0);
#endif

            QPointer<QDrag> drag = new QDrag(this);
            QMimeData* mimeData = new QMimeData;

            KUrl url(m_dragUrl);
            url.populateMimeData(mimeData);

            drag->setMimeData(mimeData);

            QPixmap pixmap = KIO::pixmapForUrl(url, 0, KIconLoader::Desktop, KIconLoader::SizeMedium);
            drag->setPixmap(pixmap);

            drag->exec();

            m_mousePressedOnUrl = false;
            m_dragUrl.clear();

            return;
        }

        QLabel::mouseMoveEvent(ev);
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
        if (link.isEmpty())
            resetLinkHighlightState();
        else
        {
            // We just saw this link, no need to do the work again.
            if (link == m_lastStatusText)
                return;

            m_lastStatusText = link;

            if (link.startsWith(QLatin1String("##")))
            {
                m_isOnChannel = true;
                m_currentChannel = link.mid(1);

                emit setStatusBarTempText(i18n("Join the channel %1", m_currentChannel));
            }
            else
            {
                m_currentUrl = link;

                emit setStatusBarTempText(link);

                setContextMenuOptions(IrcContextMenus::ShowLinkActions, true);
            }
        }
    }

    void TopicLabel::resetLinkHighlightState()
    {
        m_currentUrl.clear();
        m_currentChannel.clear();
        m_isOnChannel = false;

        setContextMenuOptions(IrcContextMenus::ShowLinkActions, false);

        if (!m_lastStatusText.isEmpty())
        {
            m_lastStatusText.clear();
            emit clearStatusBarTempText();
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
