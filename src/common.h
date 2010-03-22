/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2004 Peter Simonsson <psn@linux.se>
  Copyright (C) 2006-2008 Eike Hein <hein@kde.org>
*/

#ifndef COMMON_H
#define COMMON_H

#include <QByteArray>
#include <QHash>

class QString;
class QPixmap;

namespace Konversation
{
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
        QString htmlText;
    };

    QString removeIrcMarkup(const QString& text);
    QString doVarExpansion(const QString& text);
    QString replaceFormattingCodes(const QString& text);

    QList<QPair<int, int> > getUrlRanges(const QString& text);
    QString tagUrls(const QString& text, const QString& fromNick, bool useCustomColor = true);
    TextUrlData extractUrlData(const QString& text, const QString& fromNick, bool doUrlRanges,
        bool doHyperlinks, bool useCustomHyperlinkColor);
    bool isUrl(const QString& text);

    QPixmap overlayPixmaps(const QPixmap &under, const QPixmap &over);
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
