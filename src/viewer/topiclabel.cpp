/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2004, 2009 Peter Simonsson <peter.simonsson@gmail.com>
    SPDX-FileCopyrightText: 2006-2008 Eike Hein <hein@kde.org>
*/

#include "topiclabel.h"
#include "application.h"

#include <QClipboard>
#include <QDrag>
#include <QResizeEvent>
#include <QTextDocument>
#include <QMimeData>

namespace Konversation
{
    TopicLabel::TopicLabel(QWidget *parent)
        : QLabel(parent)
    {
        setWordWrap(true);
        setFocusPolicy(Qt::ClickFocus);
        setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        setTextInteractionFlags(Qt::TextBrowserInteraction);

        m_mousePressedOnUrl = false;
        m_isOnChannel = false;
        m_server = nullptr;

        connect(this, &TopicLabel::linkActivated, this, &TopicLabel::openLink);
        connect(this, &TopicLabel::linkHovered, this, &TopicLabel::highlightedSlot);
    }

    TopicLabel::~TopicLabel()
    {
    }

    QSize TopicLabel::minimumSizeHint() const
    {
        int minHeight = fontMetrics().ascent() + fontMetrics().descent();
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
       Q_EMIT clearStatusBarTempText();
       m_lastStatusText.clear();
       QLabel::leaveEvent(e);
    }

    void TopicLabel::openLink(const QString& link)
    {
        if (!link.isEmpty())
        {
            if (link.startsWith(QLatin1Char('#')))
            {
                if (m_server && m_server->isConnected())
                {
                    QString channel(QUrl::fromEncoded(link.toUtf8()).fragment(QUrl::FullyDecoded));
                    m_server->sendJoinCommand(channel);
                }
                else {
                // NOTE openUrl doesn't support these channel "links", this unhandled case is supported by clearing the topic on disconnect
                }
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
        if (m_mousePressedOnUrl && (m_mousePressPosition - ev->pos()).manhattanLength() > QApplication::startDragDistance())
        {
            setSelection(0, 0);

            QPointer<QDrag> drag = new QDrag(this);
            auto* mimeData = new QMimeData;

            QUrl url(m_dragUrl);

            mimeData->setUrls(QList<QUrl> { url });

            drag->setMimeData(mimeData);

            const QString iconName = KIO::iconNameForUrl(url);
            const QPixmap pixmap = QIcon::fromTheme(iconName, QIcon::fromTheme(QStringLiteral("application-octet-stream"))).pixmap(32);
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

    void TopicLabel::clear()
    {
        m_fullText.clear();
        updateSqueezedText();
    }

    void TopicLabel::updateSqueezedText()
    {
        setToolTip(QString());

        if (m_fullText.isEmpty())
        {
            QLabel::clear();

            return;
        }

        QString text = m_fullText;
        // text.replace("&", "&amp;"). Not needed as we do it in tagUrls
        text.replace(QLatin1Char('<'), QLatin1String("\x0blt;")). // tagUrls will replace \x0b with &
            replace(QLatin1Char('>'), QLatin1String("\x0bgt;"));

        text = tagUrls(text, m_channelName);

        if(height() < ((fontMetrics().ascent() + fontMetrics().descent()) * 2))
        {
            text = rPixelSqueeze(text, width() - 10);
            setWordWrap(false);
        }
        else
        {
            setWordWrap(true);
        }

        setToolTip(QLatin1String("<qt>") + m_fullText.toHtmlEscaped() + QLatin1String("</qt>"));
        QLabel::setText(QLatin1String("<qt>") + text + QLatin1String("</qt>"));
    }

    void TopicLabel::resizeEvent(QResizeEvent* ev)
    {
        QLabel::resizeEvent(ev);
        updateSqueezedText();
    }

    QString TopicLabel::rPixelSqueeze(const QString& text, int maxPixels) const
    {
        int tw = textWidth(text);

        if(tw > maxPixels)
        {
            QString tmp = text;
            int em = fontMetrics().maxWidth();
            maxPixels -= fontMetrics().horizontalAdvance(QStringLiteral("..."));
            int len, delta;

            // On some MacOS system, maxWidth may return 0
            if (em == 0) {
                for (QChar c : text) {
                    em = qMax(em, fontMetrics().horizontalAdvance(c));
                }
            }

            while((tw > maxPixels) && !tmp.isEmpty())
            {
                len = tmp.length();
                delta = (tw - maxPixels) / em;
                delta = qBound(1, delta, len);

                tmp.remove(len - delta, delta);
                tw = textWidth(tmp);
            }

            return tmp.append(QLatin1String("..."));
        }

        return text;
    }

    int TopicLabel::textWidth(const QString& text) const
    {
        QTextDocument document;
        document.setDefaultFont(font());
        document.setHtml(QLatin1String("<qt>") + text + QLatin1String("</qt>"));

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

            if (link.startsWith(QLatin1Char('#')))
            {
                m_isOnChannel = true;
                m_currentChannel = QUrl::fromEncoded(link.toUtf8()).fragment(QUrl::FullyDecoded);

                Q_EMIT setStatusBarTempText(i18n("Join the channel %1", m_currentChannel));
            }
            else
            {
                m_currentUrl = link;

                Q_EMIT setStatusBarTempText(link);

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
            Q_EMIT clearStatusBarTempText();
        }
    }

    QString TopicLabel::tagUrls(const QString& text, const QString& sender)
    {
        QString htmlText(removeIrcMarkup(text));
        QString link(QStringLiteral("<a href=\"#%1\">%2</a>"));

        QString original;
        TextChannelData channelData = extractChannelData(htmlText);

        for (int i = channelData.channelRanges.count()-1; i >= 0 ; --i)
        {
            const QPair <int, int>& range = channelData.channelRanges.at(i);
            original = htmlText.mid(range.first, range.second);
            htmlText.replace(range.first, range.second, link.arg(channelData.fixedChannels.at(i), original));
        }

        link = QStringLiteral("<a href=\"%1\">%2</a>");
        TextUrlData urlData = extractUrlData(htmlText);
        for (int i = urlData.urlRanges.count()-1; i >= 0 ; --i)
        {
            const QPair <int, int>& range = urlData.urlRanges.at(i);

            int pos = range.first;
            // check if the matched text is already replaced as a channel
            if (htmlText.lastIndexOf(QLatin1String("<a"), pos ) > htmlText.lastIndexOf(QLatin1String("</a>"), pos))
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
        htmlText.replace(QLatin1Char('&'), QLatin1String("&amp;"));
        htmlText.replace(QLatin1Char('\x0b'), QLatin1String("&"));
        return htmlText;
    }
}

#include "moc_topiclabel.cpp"
