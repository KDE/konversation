/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

*/

/*
  Based on kopeteemoticons.cpp (as of KDE 4.2) - Kopete Preferences Container-Class

  Copyright (C) 2002-2005 by the Kopete developers <kopete-devel@kde.org>
  Copyright (C) 2002 Stefan Gehn <metz@gehn.net>
  Copyright (C) 2002-2006 Olivier Goffart <ogoffart@kde.org>
  Copyright (C) 2005 Engin Aydogan <engin@bzzzt.biz>
  Copyright (C) 2005 Peter Simonsson <psn@linux.se>
  Copyright (C) 2008 Modestas Vainius <modestas@vainius.eu>
*/

#include "emoticons.h"
#include "config/preferences.h"

namespace Konversation
{

    K_GLOBAL_STATIC(KEmoticons, s_self)

    KEmoticons *Emoticons::self()
    {
        return s_self;
    }

    QString Emoticons::parseEmoticons(const QString &text, KEmoticonsTheme::ParseMode mode, const QStringList &exclude)
    {
        // Disable emoticons support until IRCView supports them
        if (Preferences::self()->enableEmotIcons())
        {
            return Konversation::Emoticons::self()->theme().parseEmoticons(text, mode, exclude);
        }
        else 
        {
            return text;
        }
    }
}
