/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2003 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2004 Peter Simonsson <psn@linux.se>
*/

#include "scriptlauncher.h"
#include "channel.h"
#include "application.h"
#include "server.h"

#include <QFileInfo>

#include <KProcess>
#include <QStandardPaths>

#include "konvi_qdbus.h"

ScriptLauncher::ScriptLauncher(QObject* parent) : QObject(parent)
{
    qputenv("KONVERSATION_LANG", QLocale().name().toLatin1());
    if (!qEnvironmentVariableIsSet("KONVERSATION_DBUS_BIN"))
        qputenv("KONVERSATION_DBUS_BIN", QDBUS_BIN);

    QStringList pythonPath(QProcessEnvironment::systemEnvironment().value(QStringLiteral("PYTHONPATH")).split(QLatin1Char(':'), Qt::SkipEmptyParts));
    pythonPath << QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("konversation/scripting_support/python"),
        QStandardPaths::LocateDirectory);
    qputenv("PYTHONPATH", pythonPath.join(QLatin1Char(':')).toLocal8Bit());
}

ScriptLauncher::~ScriptLauncher()
{
}

QString ScriptLauncher::scriptPath(const QString& script)
{
    return QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("konversation/scripts/") + script);
}

void ScriptLauncher::launchScript(int connectionId, const QString& target, const QString &parameter)
{
    // send the script all the information it will need
    QStringList parameterList = parameter.split(QLatin1Char(' '));

    // find script path (could be installed for all users in $KDEDIR/share/apps/ or
    // for one user alone in $HOME/.kde/share/apps/)
    QString script(parameterList.takeFirst());
    QString path = scriptPath(script);

    parameterList.prepend(target);
    parameterList.prepend(QString::number(connectionId));

    QFileInfo fileInfo(path);

    KProcess proc;
    proc.setWorkingDirectory(fileInfo.path());
    proc.setProgram(path, parameterList);

    if (proc.startDetached() == 0)
    {
        if (!fileInfo.exists())
           Q_EMIT scriptNotFound(script);
        else
           Q_EMIT scriptExecutionError(script);
    }
}

#include "moc_scriptlauncher.cpp"
