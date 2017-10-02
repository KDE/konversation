/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2017 Eike Hein <hein@kde.org>
*/

#ifndef QCLIPBOARDWRAPPER_H
#define QCLIPBOARDWRAPPER_H

#include <QObject>

class QClipboardWrapper : public QObject
{
    Q_OBJECT

    public:
        explicit QClipboardWrapper(QObject *parent = 0);
        ~QClipboardWrapper();

        Q_INVOKABLE QString clipboardText() const;
        Q_INVOKABLE void setClipboardText(const QString &text);

        Q_INVOKABLE QString selectionText() const;
        Q_INVOKABLE void setSelectionText(const QString &text);

        Q_INVOKABLE QString simplifyPaste(const QString &text);

        Q_INVOKABLE QString handlePaste(const QString &text);
};

#endif
