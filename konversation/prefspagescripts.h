/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspagescripts.h  -  Provides a user interface to customize login scripts
  begin:     Tue Sep 24 2002
  copyright: (C) 2002 by Matthias Gierlings
  email:     gismore@users.sourceforge.net

  $Id$
*/


#ifndef PREFSPAGESCRIPTS_H
#define PREFSPAGESCRIPTS_H

#include <klistview.h>
#include <keditcl.h>

#include <qlayout.h>
#include <qdir.h>
#include <qstringlist.h>
#include <qtextstream.h>

#include "prefspage.h"

/*
  @author Matthias Gierlings
*/

class PrefsPageScripts : public PrefsPage
{
  Q_OBJECT

  public:
	  PrefsPageScripts(QFrame *passedParent, Preferences *passedPreferences);
	  ~PrefsPageScripts();

	protected:
    QBoxLayout      *mainBox;
		KListView       *scriptsList;
		KListViewItem   *scriptsListEntry;
		KEdit           *scriptEditor;
    QDir						*scriptsDirectory;
		QString					scriptFilePath;
		QStringList			scriptFiles;
		char						streamBuffer[512];
		QTextStream			*script;

	public slots:
		//void openScriptFile();
		//void renameScriptFile();
};

#endif
