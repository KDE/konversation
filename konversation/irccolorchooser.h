/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  irccolorchooser.h - dialog used to add irc colors to your messages
  begin:     Wed 9 July 2003
  copyright: (C) 2003 by Peter Simonsson
  email:     psn@linux.se
*/

#ifndef IRCCOLORCHOOSER_H
#define IRCCOLORCHOOSER_H

#include <kdialogbase.h>

class IRCColorChooserUI;
class KComboBox;
class Preferences;

class IRCColorChooser : public KDialogBase
{
  Q_OBJECT
  public:
    IRCColorChooser(QWidget* parent, Preferences* p, const char* name = 0);
    QString color();

  protected slots:
    void updatePreview();

  protected:
    void initColors(KComboBox* combo);

  protected:
    IRCColorChooserUI* m_view;
    Preferences* m_preferences;
};

#endif
