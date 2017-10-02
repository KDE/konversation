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
#include "application.h"
#include "common.h"
#include "pasteeditor.h"

#include <KMessageBox>


#include <QClipboard>
#include <QApplication>

QClipboardWrapper::QClipboardWrapper(QObject *parent)
    : QObject(parent)
{
}

QClipboardWrapper::~QClipboardWrapper()
{
}

QString QClipboardWrapper::clipboardText() const
{
    return QGuiApplication::clipboard()->text(QClipboard::Clipboard);
}

void QClipboardWrapper::setClipboardText(const QString &text)
{
    QGuiApplication::clipboard()->setText(text, QClipboard::Clipboard);
}

QString QClipboardWrapper::selectionText() const
{
    return QGuiApplication::clipboard()->text(QClipboard::Selection);
}

void QClipboardWrapper::setSelectionText(const QString &text)
{
    QGuiApplication::clipboard()->setText(text, QClipboard::Selection);
}

QString QClipboardWrapper::simplifyPaste(const QString &text)
{
    QString pasteText(text);

    Konversation::sterilizeUnicode(pasteText);

    // Replace \r with \n to make xterm pastes happy.
    pasteText.replace('\r','\n');

    // Remove blank lines.
    while(pasteText.contains("\n\n")) {
        pasteText.replace("\n\n","\n");
    }

    QRegExp reTopSpace("^ *\n");
    while(pasteText.contains(reTopSpace)) {
        pasteText.remove(reTopSpace);
    }

    QRegExp reBottomSpace("\n *$");
    while(pasteText.contains(reBottomSpace)) {
        pasteText.remove(reBottomSpace);
    }

    return pasteText;
}

QString QClipboardWrapper::handlePaste(const QString &text)
{
    int doPaste = KMessageBox::Yes;

    const int lines = text.count('\n');

    if (text.length() > 256 || lines)
    {
        const QString bytesString = i18np("1 byte", "%1 bytes", text.length());
        const QString linesString = i18np("1 line", "%1 lines", lines + 1);

        doPaste = KMessageBox::warningYesNoCancel
            (QApplication::activeWindow(),
            i18nc(
            "%1 is, for instance, '200 bytes'.  %2 is, for instance, '7 lines'.  Both are localised (see the two previous messages).",
            "<qt>You are attempting to paste a large portion of text (%1 or %2) into "
            "the chat. This can cause connection resets or flood kills. "
            "Do you really want to continue?</qt>", bytesString, linesString),
            i18n("Large Paste Warning"),
            KGuiItem(i18n("Paste")),
            KGuiItem(i18n("&Edit...")),
            KStandardGuiItem::cancel(),
            QString("LargePaste"),
            KMessageBox::Dangerous);
    }

    if (doPaste == KMessageBox::No) {
        QString ret(PasteEditor::edit(QApplication::activeWindow(), text));

        if (ret.isEmpty()) {
            return ret;
        }

        ret = Application::instance()->doAutoreplace(ret, true)[0].toString();

        Konversation::sterilizeUnicode(ret);

        return ret;
    } else if (doPaste == KMessageBox::Yes) {
        QString ret(Application::instance()->doAutoreplace(text, true)[0].toString());

        Konversation::sterilizeUnicode(ret);

        return ret;
    }

    return QString();
}
