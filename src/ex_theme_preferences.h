/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  (C) 2004,2005 by İsmail Dönmez
*/

#ifndef PREFSPAGETHEMES_H
#define PREFSPAGETHEMES_H

#include "theme_preferences.h"

class QStringList;

class Theme_Config_Ext : public Theme_Config
{
  Q_OBJECT;
    public:
  Theme_Config_Ext(QWidget* parent, const char* name);
	~Theme_Config_Ext();

    protected slots:
	void applyPreferences();
        void updatePreview(int id);
        void updateButtons();
        void installTheme();
        void removeTheme();

    private:
        QStringList m_dirs;
        QString m_oldTheme;

        void updateList();
};
#endif
