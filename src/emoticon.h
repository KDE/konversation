/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2005 Peter Simonsson <psn@linux.se>
*/

#ifndef KONVERSATIONEMOTICON_H
#define KONVERSATIONEMOTICON_H

#include <qstring.h>
#include <qmap.h>


class QFontMetrics;

namespace Konversation
{

    typedef QMap<QString, QString> EmotIconMap;

    class EmotIcon
    {
        public:
            ~EmotIcon();
            static EmotIcon* self();

            static void changeTheme(const QString& themeName);
            static QString filter(const QString& txt, const QFontMetrics& fm);

        protected:
            EmotIcon();
            static EmotIcon* s_self;

            static QString findIcon(const QString& filename);

        private:
            QString m_themeName;
            EmotIconMap m_emotIconMap;
    };

}
#endif
