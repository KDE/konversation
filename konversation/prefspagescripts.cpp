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
	myPreferences = passedPreferences;

	scriptsList = new KListView(parentFrame);
	scriptsList->addColumn(i18n("Name"));
	scriptEditor = new KEdit(parentFrame);
	scriptsList->setItemsRenameable(true);
  mainBox = new QHBoxLayout(passedParent);
	mainBox->addWidget(scriptsList);
	mainBox->addWidget(scriptEditor);

	scriptsDirectory = new QDir(QString(myPreferences->getLogPath() + "/scripts"));
	scriptFiles = scriptsDirectory->entryList(QDir::Files);

	scriptsList->setRenameable(0, true);

  for(QStringList::Iterator it = scriptFiles.begin(); it != scriptFiles.end(); ++it)
	{
		scriptsListEntry = new KListViewItem(scriptsList, *it);
	}

	connect(scriptsList, SIGNAL(executed(QListViewItem*)), this, SLOT(openScriptFile(QListViewItem*)));
	//connect(this, SIGNAL(fileRenamed(QListViewItem*)), this, SLOT(openScriptFile(QListViewItem*)));
	connect(scriptsList, SIGNAL(itemRenamed(QListViewItem*)), this, SLOT(renameScriptFile(QListViewItem*)));

}
PrefsPageScripts::~PrefsPageScripts()
{
}

void PrefsPageScripts::openScriptFile(QListViewItem *passedSelectedItem)
{
	kdDebug() << "001\n";

	QListViewItem *selectedItem = passedSelectedItem;

	scriptFilePath = QString(myPreferences->getLogPath() + "/scripts");
	scriptFilePath.append("/");
	scriptFilePath.append(selectedItem->text(0));

	openedScriptFile.setName(scriptFilePath);
  oldScriptFileName = openedScriptFile.name();

	if(openedScriptFile.open(IO_ReadWrite))
	{
		scriptEditor->clear();
		script = new QTextStream(&openedScriptFile);
		scriptEditor->insertText(script);
		openedScriptFile.close();
	}
}

void PrefsPageScripts::renameScriptFile(QListViewItem *passedItem)
{
	QListViewItem *renameItem = passedItem;
	
	kdDebug() << "002\n";

	openedScriptFile.close();
	scriptsDirectory->rename(oldScriptFileName, renameItem->text(0));
	emit fileRenamed(renameItem);
}

#include "prefspagescripts.moc"
