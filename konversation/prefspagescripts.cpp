/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspagescripts.cpp  -  Provides a user interface to customize login scripts
  begin:     Tue Sep 24 2002
  copyright: (C) 2002 by Matthias Gierlings
  email:     gismore@users.sourceforge.net

  $Id$
*/

#include <qfile.h>

#include <kdebug.h>

#include "prefspagescripts.h"

PrefsPageScripts::PrefsPageScripts(QFrame *passedParent, Preferences *passedPreferences)
                 :PrefsPage(passedParent, passedPreferences)
{
	scriptsList = new KListView(parentFrame);
	scriptsList->addColumn(i18n("Name"));
	scriptEditor = new KEdit(parentFrame);
	
  mainBox = new QHBoxLayout(passedParent);
	mainBox->addWidget(scriptsList);
	mainBox->addWidget(scriptEditor);

	//set scripts directory, save all file names into a QStringList
	scriptsDirectory = new QDir("/home/gismore/applications/konversation/scripts/");
	scriptFiles = scriptsDirectory->entryList(QDir::Files);

	for(QStringList::Iterator it = scriptFiles.begin(); it != scriptFiles.end(); ++it)
	{
		scriptsListEntry = new KListViewItem(scriptsList, *it);
	}
	scriptFilePath = scriptsDirectory->path();
	scriptFilePath.append("bla");

	QFile	openedScriptFile(scriptFilePath);

	if(openedScriptFile.open(IO_ReadOnly))
	{
		script = new QTextStream(&openedScriptFile);
		scriptEditor->insertText(&(script->readRawBytes(streamBuffer, 6)));
		for(int i = 1; i < 6; ++i)
		{
			kdDebug() << streamBuffer[i] << endl;
		}
	}
}
PrefsPageScripts::~PrefsPageScripts()
{
}

#include "prefspagescripts.moc"
