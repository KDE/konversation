/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspagehighlight.h  -  Preferences panel for highlight patterns
  begin:     Die Jun 10 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#ifndef PREFSPAGEHIGHLIGHT_H
#define PREFSPAGEHIGHLIGHT_H

#include "prefspage.h"

/*
  @author Dario Abatianni
*/
class PrefsPageHighlight : public PrefsPage
{
  Q_OBJECT

  public:
    PrefsPageHighlight(QFrame* newParent,Preferences* newPreferences);
    ~PrefsPageHighlight();
};

#endif
