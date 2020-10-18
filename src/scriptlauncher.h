/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2003 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2004 Peter Simonsson <psn@linux.se>
*/

#include <QObject>

#ifndef SCRIPTLAUNCHER_H
#define SCRIPTLAUNCHER_H

class ScriptLauncher : public QObject
{
    Q_OBJECT

    public:
        explicit ScriptLauncher(QObject* parent);
        ~ScriptLauncher() override;

        static QString scriptPath(const QString& script);

    public Q_SLOTS:
        void launchScript(int connectionId, const QString& target, const QString& parameter);

    Q_SIGNALS:
        void scriptNotFound(const QString& name);
        void scriptExecutionError(const QString& name);

    private:
        Q_DISABLE_COPY(ScriptLauncher)
};

#endif
