/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2017 Eike Hein <hein@kde.org>
*/

#include "qclipboardwrapper.h"

#include <QClipboard>
#include <QGuiApplication>

QClipboardWrapper::QClipboardWrapper(QObject *parent)
    : QObject(parent)
{
}

QClipboardWrapper::~QClipboardWrapper()
{
}

void QClipboardWrapper::setClipboardText(const QString &text)
{
    QGuiApplication::clipboard()->setText(text, QClipboard::Clipboard);
}
