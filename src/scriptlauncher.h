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

#include <QObject>

#ifndef SCRIPTLAUNCHER_H
#define SCRIPTLAUNCHER_H



class ScriptLauncher : public QObject
{
    Q_OBJECT

    public:
        explicit ScriptLauncher(QObject* parent);
        ~ScriptLauncher();

        static QString scriptPath(const QString& script);

    signals:
        void scriptNotFound(const QString& name);
        void scriptExecutionError(const QString& name);

    public slots:
        void launchScript(int connectionId, const QString& target, const QString& parameter);
};
#endif
