/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Copyright (C) 2005 Peter Simonsson <psn@linux.se>
  Copyright (C) 2008 Modestas Vainius <modestas@vainius.eu>
*/

/*
  Based on kopeteemoticons.cpp (as of KDE 4.2) - Kopete Preferences Container-Class

  Copyright (C) 2002-2005 by the Kopete developers <kopete-devel@kde.org>
  Copyright (C) 2002 Stefan Gehn <metz@gehn.net>
  Copyright (C) 2002-2006 Olivier Goffart <ogoffart@kde.org>
  Copyright (C) 2005 Engin Aydogan <engin@bzzzt.biz>
*/

#ifndef KONVERSATIONEMOTICONS_H
#define KONVERSATIONEMOTICONS_H

#include <kemoticons.h>

namespace Konversation {

    class Emoticons
    {
    public:
        /**
         * The emoticons container-class by default is a singleton object.
         * Use this method to retrieve the instance.
         */
        static KEmoticons *self();

        static QString parseEmoticons(const QString &text, KEmoticonsTheme::ParseMode mode = KEmoticonsTheme::DefaultParse, const QStringList &exclude = QStringList());
    };

} //END namespace Konversation

#endif
