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

  Original copyright by:

  Copyright (c) 2002      by Stefan Gehn            <metz@gehn.net>
  Copyright (c) 2002-2006 by Olivier Goffart        <ogoffart@kde.org>
  Copyright (c) 2005      by Engin AYDOGAN          <engin@bzzzt.biz>

  Kopete    (c) 2002-2005 by the Kopete developers  <kopete-devel@kde.org>
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
        static QList<KEmoticonsTheme::Token> tokenize(const QString &message, KEmoticonsTheme::ParseMode mode = KEmoticonsTheme::DefaultParse);

    };

} //END namespace Kopete

#endif
