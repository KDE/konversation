#ifndef KONVERSATION_IRCCHARSETS_H
#define KONVERSATION_IRCCHARSETS_H


#include <QHash>
#include <QStringList>
#include <QTextCodec>

namespace Konversation
{

    struct IRCCharsets
    {
        static IRCCharsets *self();
        QString encodingForLocale() const { return QStringLiteral("UTF-8"); }
        bool isValidEncoding(const QString& ) const { return true; }
        QTextCodec* codecForName(const QString& ) const { return QTextCodec::codecForName("UTF-8"); }
    };
}

#endif //KONVERSATION_IRCCHARSETS_H

