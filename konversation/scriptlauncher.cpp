/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  scriptlauncher.cpp  -  Launches shell scripts
  begin:     Mit Mär 12 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <qstringlist.h>

#include <kdebug.h>
#include <kstddirs.h>
#include <kprocess.h>
#include <dcopclient.h>

#include "scriptlauncher.h"
#include "konversationapplication.h"

ScriptLauncher::ScriptLauncher(const QString& newServer,const QString& newTarget)
{
  server=newServer;
  target=newTarget;
}

ScriptLauncher::~ScriptLauncher()
{
}

void ScriptLauncher::launchScript(const QString &parameter)
{
  KStandardDirs kstddir;
  QString scriptPath(kstddir.saveLocation("data","konversation/scripts"));
  KProcess process;

  // TODO: This is in the making
  // send the script all the information it will need
  QStringList parameterList=QStringList::split(' ',parameter);

  process << scriptPath+"/"+parameterList[0]  // script path / name
          << kapp->dcopClient()->appId()      // our dcop port
          << server                           // the server we are connected to
          << target;                          // the target where the call came from

  // send remaining parameters to the script
  for(unsigned int index=1;index<parameterList.count();index++)
    process << parameterList[index];

  process.setWorkingDirectory(scriptPath);
  if(process.start()==false) kdDebug() << "exec() error" << endl;
  process.detach(); // to free the script's stdin
  kdDebug() << "Script running." << endl;
}

#include "scriptlauncher.moc"

