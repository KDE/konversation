/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2003 Benjamin C Meyer (ben+kdelibs at meyerhome dot net)
 *  Copyright (C) 2003 Waldo Bastian <bastian@kde.org>
 *  Copyright (C) 2004 Michael Brade <brade@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

/*
 *  This is an abstract class that every settings page should inherit from.
 *
 *  begin:     Jan 13 2006
 *  copyright: (C) 2005 by John Tapsell, Konversation Developers
 *  email:     sho@eikehein.com
 */

#ifndef KONVISETTINGSPAGE_H
#define KONVISETTINGSPAGE_H

class KonviSettingsPage
{
  public:
    virtual void restorePageToDefaults() = 0;  // function called when the user klicks "Default"
    virtual void saveSettings() = 0;           // function called when the user klicks "Ok" or "Apply"
    virtual void loadSettings() = 0;           // function called when the user opens the page

    virtual bool hasChanged() = 0;             // is to return if any non-KConfigXT settings have changed
};

#endif


