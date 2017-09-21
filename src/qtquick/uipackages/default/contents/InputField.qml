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

QQC2.ScrollView {
    id: inputField

    height: (konvUi.settings.constrictInputField
        ? parent.height : Math.max(inputFieldTextArea.implicitHeight, konvUi.footerHeight))

    property Item completionPopup: null

    QQC2.ScrollBar.horizontal: null
    QQC2.ScrollBar.vertical: null

    function textForward(text) {
        inputFieldTextArea.forceActiveFocus();
        inputFieldTextArea.insert(inputFieldTextArea.cursorPosition, text);
    }

    Connections {
        target: viewModel

        onCurrentViewChanged: {
            completionPopup.close();
            inputFieldTextArea.resetCompletion();
        }
    }

    Connections {
        target: konvUi

        onSettingsModeChanged: {
            completionPopup.close();
            inputFieldTextArea.resetCompletion();
        }
    }

    QQC2.TextArea {
        id: inputFieldTextArea

        enabled: viewModel.currentView

        property string lastCompletion: ""
        property int nextMatch: 0
        property bool completionResetLock: false

        background: null

        topPadding: Kirigami.Units.smallSpacing
        bottomPadding: Kirigami.Units.smallSpacing

        renderType: Text.NativeRendering

        font.pixelSize: largerFontSize

        verticalAlignment: Text.AlignVCenter

        wrapMode: konvUi.settings.constrictInputField ? TextEdit.NoWrap : TextEdit.Wrap

        function removeCompletion() {
            if (!lastCompletion.length) {
                return;
            }

            remove(cursorPosition - lastCompletion.length,
                (cursorPosition - lastCompletion.length) + lastCompletion.length);
        }

        function insertMatch(match) {
            completionResetLock = true;

            removeCompletion();

            lastCompletion = completer.matches.at(match);

            if (cursorPosition > 0) { // WIPQTQUICK TODO KConfigXT has different suffixes for each case
                lastCompletion += konvUi.settings.completionSuffix;
            }

            insert(cursorPosition, lastCompletion)

            completionResetLock = false;
        }

        function insertNextMatch() {
            insertMatch(nextMatch);

            ++nextMatch;

            if (nextMatch == completer.matches.count) {
                nextMatch = 0;
            }
        }

        function resetCompletion() {
            if (completionResetLock || completionPopup.currentIndex != -1) {
                return;
            }

            completer.prefix = "";
            nextMatch = 0;
        }

        onCursorPositionChanged: resetCompletion()
        onTextChanged: resetCompletion()

        Connections {
            target: completionPopup

            onCurrentIndexChanged: {
                if (!activeFocus) {
                    inputFieldTextArea.insertMatch(completionPopup.currentIndex);
                }
            }

            onCancelled: completionPopup.removeCompletion()
        }

        Keys.onPressed: {
            if (event.key == Qt.Key_Tab) {
                event.accepted = true;

                if (!text.length && lastCompletion.length) {
                    text = lastCompletion;
                    cursorPosition = cursorPosition + lastCompletion.length;
                } else if (cursorPosition > 0) {
                    if (completer.matches.count) {
                        insertNextMatch();
                    } else {
                        // WIPQTQUICK TODO There's faster ways than splitting all words.
                        var prefix = getText(0, cursorPosition).split(/[\s]+/).pop();

                        if (prefix.length) {
                            completer.prefix = prefix;

                            if (completer.matches.count) {
                                lastCompletion = prefix;

                                if (konvUi.settings.hideCompletionPopup || completer.matches.count == 1) {
                                    insertNextMatch();
                                } else {
                                    completionPopup.open();
                                }
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

        Component.onCompleted: forceActiveFocus()
    }
}
