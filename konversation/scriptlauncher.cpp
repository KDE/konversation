/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Launches shell scripts
  begin:     Mit Mär 12 2003
  copyright: (C) 2003 by Dario Abatianni
             (C) 2004 by Peter Simonsson
  email:     eisfuchs@tigress.com
*/

#include <qstringlist.h>
#include <qfile.h>
#include <qfileinfo.h>

#include <kdebug.h>
#include <kstandarddirs.h>
#include <kprocess.h>
#include <dcopclient.h>

#include "channel.h"
#include "scriptlauncher.h"
#include "konversationapplication.h"
#include "server.h"

ScriptLauncher::ScriptLauncher(Server* server)
  : QObject(server)
{
  m_server = server;
}

ScriptLauncher::~ScriptLauncher()
{
}

void ScriptLauncher::launchScript(const QString& target, const QString &parameter)
{
  KStandardDirs kstddir;
//  QString scriptPath(kstddir.saveLocation("data",QString("konversation/scripts")));
  KProcess process;

  // send the script all the information it will need
  QStringList parameterList=QStringList::split(' ',parameter);

  // find script path (could be installed for all users in $KDEDIR/share/apps/ or
  // for one user alone in $HOME/.kde/share/apps/
  QString scriptPath(kstddir.findResource("data","konversation/scripts/"+parameterList[0]));

  process << scriptPath                    // script path and name
          << kapp->dcopClient()->appId()   // our dcop port
          << m_server->getServerName()     // the server we are connected to
          << target;                       // the target where the call came from

  // send remaining parameters to the script
  for(unsigned int index=1;index<parameterList.count();index++)
    process << parameterList[index];

  QFileInfo fileInfo(scriptPath);

  process.setWorkingDirectory(fileInfo.dirPath());
  if(process.start()==false)
  {
    QFile file(parameterList[0]);
    if(!file.exists()) emit scriptNotFound(file.name());
    else emit scriptExecutionError(file.name());
  }

  // to free the script's stdin, otherwise backticks won't work
  process.detach();
}

#include "scriptlauncher.moc"

