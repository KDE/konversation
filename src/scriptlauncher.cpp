/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2003 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2004 Peter Simonsson <psn@linux.se>
*/

#include "scriptlauncher.h"
#include "channel.h"
#include "application.h"
#include "server.h"

#include <qstringlist.h>
#include <qfileinfo.h>
#include <qprocess.h>

#include <kdebug.h>
#include <kstandarddirs.h>


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
    // send the script all the information it will need
    QStringList parameterList = parameter.split(' ');
    // find script path (could be installed for all users in $KDEDIR/share/apps/ or
    // for one user alone in $HOME/.kde/share/apps/
    QString script(parameterList.takeFirst());
    QString scriptPath(KStandardDirs::locate("data", "konversation/scripts/" + script));
    parameterList.prepend(target);
    parameterList.prepend(QString::number(m_server->connectionId()));
    QFileInfo fileInfo(scriptPath);
    if (!QProcess::startDetached(scriptPath, parameterList, fileInfo.path()))
    {
        if(!fileInfo.exists())
           emit scriptNotFound(script);
        else
           emit scriptExecutionError(script);
    }
}

#include "scriptlauncher.moc"
