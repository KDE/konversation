/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  scriptlauncher.h  -  Launches shell scripts
  begin:     Mit M�r 12 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <qobject.h>

#ifndef SCRIPTLAUNCHER_H
#define SCRIPTLAUNCHER_H

/*
  @author Dario Abatianni
*/

class ScriptLauncher : public QObject
{
  Q_OBJECT

  public:
    ScriptLauncher();
    ~ScriptLauncher();

    void setServerName(const QString& newServerName);
    void setTargetName(const QString& newName);

  signals:
    void scriptNotFound(const QString& name);
    void scriptExecutionError(const QString& name);

  public slots:
    void launchScript(const QString& parameter);

  protected:
    QString server;
    QString target;
};

#endif
