/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2017 Eike Hein <hein@kde.org>
*/

import QtQuick 2.7

import QtQuick.Controls 2.2 as QQC2

QQC2.TextField {
    id: inputField

    enabled: viewModel.currentView

    property string lastCompletion: ""
    property int nextMatch: 0
    property bool completionResetLock: false

    background: null

    renderType: Text.NativeRendering

    font.pixelSize: largerFontSize

    verticalAlignment: Text.AlignVCenter

    wrapMode: TextEdit.NoWrap

    function insertNextMatch() {
        lastCompletion = completer.matches.at(nextMatch);

        completionResetLock = true;
        insert(cursorPosition, lastCompletion)
        completionResetLock = false;

        ++nextMatch;

        if (nextMatch == completer.matches.rowCount()) {
            nextMatch = 0;
        }
    }

    function resetCompletion() {
        completer.prefix = "";
        nextMatch = 0;
    }

    onFocusChanged: {
        if (!focus) {
            resetCompletion();
        }
    }

    onCursorPositionChanged: {
        if (!completionResetLock) {
            resetCompletion();
        }
    }

    onTextChanged: {
        if (!completionResetLock) {
            resetCompletion();
        }
    }

    Keys.onPressed: {
        if (event.key == Qt.Key_Tab) {
            event.accepted = true;

            if (!text.length && lastCompletion.length) {
                text = lastCompletion;
                cursorPosition = cursorPosition + lastCompletion.length;
            } else if (cursorPosition > 0) {
                if (completer.matches.rowCount()) {
                    completionResetLock = true;
                    remove(cursorPosition - lastCompletion.length,
                        (cursorPosition - lastCompletion.length) + lastCompletion.length);
                    completionResetLock = false;

                    insertNextMatch();
                } else {
                    // WIPQTQUICK TODO There's faster ways than splitting all words.
                    var prefix = getText(0, cursorPosition).split(/[\s]+/).pop();

                    if (prefix.length) {
                        completer.prefix = prefix;

                        if (completer.matches.rowCount()) {
                            completionResetLock = true;
                            remove(cursorPosition - completer.prefix.length,
                                (cursorPosition - completer.prefix.length) + completer.prefix.length);
                            completionResetLock = false;

                            insertNextMatch();
                        }
                    }
                }
            }

            return;
        } else if (text != "") {
            resetCompletion(); // WIPQTQUICK Possibly excessive.

            // WIPQTQUICK TODO Evaluating text is not good enough, needs real key event fwd
            // to make things like deadkeys work
            if (event.key == Qt.Key_Enter || event.key == Qt.Key_Return) {
                event.accepted = true;
                viewModel.currentView.sendText(text);
                text = "";
            }
        }
    }

    function textForward(text) {
        forceActiveFocus();
        insert(length, text);
        cursorPosition = length;
    }

    Component.onCompleted: {
        konvUi.inputField = inputField;
        forceActiveFocus();
    }
}
