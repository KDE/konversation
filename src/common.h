/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2004 Peter Simonsson <psn@linux.se>
    SPDX-FileCopyrightText: 2006-2008 Eike Hein <hein@kde.org>
*/

#ifndef COMMON_H
#define COMMON_H

#include <QByteArray>
#include <QHash>
#include <QRegularExpression>
#include <QStringList>

class QString;

namespace Konversation
{
    static QRegularExpression ircMarkupsRegExp(QStringLiteral("[\\o{0000}-\\o{0037}]"));

    static QRegularExpression colorRegExp(QStringLiteral("(\x03(([0-9]{1,2})(,([0-9]{1,2}))?)?|\x0f)|\x02|\x09|\x11|\x13|\x15|\x16|\x1d|\x1e|\x1f"));

    static QRegularExpression urlPattern(QStringLiteral("\\b((?:(?:([a-z][\\w\\.-]+:/{1,3})|www\\d{0,3}[.]|[a-z0-9.\\-]+[.][a-z]{2,4}/)(?:[^\\s()<>]+|\\(([^\\s()<>]+|(\\([^\\s()<>]+\\)))*\\))+(?:\\(([^\\s()<>]+|(\\([^\\s()<>]+\\)))*\\)|\\}\\]|[^\\s`!()\\[\\]{};:'\".,<>?%1%2%3%4%5%6])|[a-z0-9.\\-+_]+@[a-z0-9.\\-]+[.][a-z]{1,5}[^\\s/`!()\\[\\]{};:'\".,<>?%1%2%3%4%5%6]))").arg(QChar(0x00AB)).arg(QChar(0x00BB)).arg(QChar(0x201C)).arg(QChar(0x201D)).arg(QChar(0x2018)).arg(QChar(0x2019)));

    static QRegularExpression chanExp(QStringLiteral("(^|\\s|^\"|\\s\"|,|'|\\(|\\:|!|@|%|\\+)(#[^,\\s;\\)\\:\\/\\(\\<\\>]*[^.,\\s;\\)\\:\\/\\(\"\''\\<\\>?%1%2%3%4%5%6])").arg(QChar(0x00AB)).arg(QChar(0x00BB)).arg(QChar(0x201C)).arg(QChar(0x201D)).arg(QChar(0x2018)).arg(QChar(0x2019)));

    enum TabNotifyType
    {
        tnfNick,
        tnfHighlight,
        tnfPrivate,
        tnfNormal,
        tnfSystem,
        tnfControl,
        tnfNone
    };

    enum ConnectionState
    {
        SSNeverConnected,
        SSDeliberatelyDisconnected,
        SSInvoluntarilyDisconnected,
        SSScheduledToConnect,
        SSConnecting,
        SSConnected
    };

    enum ConnectionFlag
    {
        SilentlyReuseConnection,
        PromptToReuseConnection,
        CreateNewConnection
    };

    struct TextUrlData
    {
        QList<QPair<int, int> > urlRanges;
        QStringList fixedUrls;
    };

    struct TextChannelData
    {
        QList<QPair<int, int> > channelRanges;
        QStringList fixedChannels;
    };

    QString removeIrcMarkup(const QString& text);
    QString doVarExpansion(const QString& text);
    QString replaceFormattingCodes(const QString& text);
    QString replaceIRCMarkups(const QString& text);
    inline bool hasIRCMarkups(const QString& text)
    {
        return text.contains(ircMarkupsRegExp);
    }

    QList<QPair<int, int> > getUrlRanges(const QString& text);
    QList<QPair<int, int> > getChannelRanges(const QString& text);
    TextUrlData extractUrlData(const QString& string, bool doUrlFixup = true);
    TextChannelData extractChannelData(const QString& text, bool doChannelFixup = true);
    bool isUrl(const QString& text);
    QString extractColorCodes(const QString& text);

    bool isUtf8(const QByteArray& text);
    uint colorForNick(const QString& nickname);

    static QHash<QChar,QString> m_modesHash;
    QHash<QChar,QString> getChannelModesHash();

    QString sterilizeUnicode(const QString& s);
    QString& sterilizeUnicode(QString& s);
    QStringList& sterilizeUnicode(QStringList& list);
    QStringList sterilizeUnicode(const QStringList& inVal);
}
#endif
