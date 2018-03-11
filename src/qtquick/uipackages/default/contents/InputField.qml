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

import org.kde.kirigami 2.2 as Kirigami

import org.kde.konversation 1.0 as Konversation

/* WIPQTQUICK TODO Missing vs. ircinput.cpp - not all necessary needed anymore
 * ASCII BEL
 * ^U
 * Inline autoreplace
 * History browsing by mouse wheel
 * Text drops
 */

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

    function pasteFromClipboard() {
        paste(clipboard.clipboardText());
    }

    function pasteFromSelection() {
        paste(clipboard.selectionText());
    }

    function paste(text) {
        if (!text.length) {
            return;
        }

        completionPopup.close();
        inputFieldTextArea.resetCompletion();

        text = clipboard.simplifyPaste(text);

        var multiLine = false;

        if (text.indexOf("\n") != -1) {
            var firstIndex = text.indexOf("\n");
            var lastIndex = text.lastIndexOf("\n");

            // Emit the signal if there's a line break in the middle of the text.
            if (firstIndex > 0 && firstIndex != (text.length - 1)) {
                multiLine = true;
            }

            // Emit the signal if there's more than one line break in the text.
            if (firstIndex != lastIndex) {
                multiLine = true;
            }

            // Remove the \n from end of the line if there's only one \n.
            if (!multiLine) {
                text.remove("\n");
            }
        }

        if (multiLine) {
            // Prepend text currently in the input field.
            if (inputFieldTextArea.text.length) {
                text = inputFieldTextArea.text + "\n" + text;
            }

            text = clipboard.handlePaste(text);

            if (text.length && viewModel.currentView) {
                viewModel.currentView.textPasted(text);
                inputHistoryModel.append(viewModel.currentView, text);
                inputFieldTextArea.text = "";
            }
        } else {
            if (inputFieldTextArea.selectedText.length) {
                inputFieldTextArea.remove(inputFieldTextArea.selectionStart, inputFieldTextArea.selectionEnd);
                inputFieldTextArea.deselect();
            }

            inputFieldTextArea.insert(inputFieldTextArea.cursorPosition, text);
        }

        inputFieldTextArea.forceActiveFocus();
    }

    Connections {
        target: viewModel

        property QtObject lastView: null

        onCurrentViewChanged: {
            completionPopup.close();
            inputFieldTextArea.resetCompletion();

            if (!viewModel.currentView) {
                lastView = viewModel.currentView;

                return;
            }

            if (!konvUi.settings.sharedInputField) {
                if (lastView) {
                    if (inputFieldTextArea.historyCursor != -1) {
                        var idx = inputHistoryModel.index(inputHistoryModel.count - 1, 0);

                        if (inputHistoryModel.data(idx, Konversation.InputHistoryModel.Editing)) {
                            inputHistoryModel.remove(idx);
                        }
                    } else if (inputFieldTextArea.text.length) {
                        inputHistoryModel.append(lastView, inputFieldTextArea.text,
                            true, inputFieldTextArea.cursorPosition);
                        inputFieldTextArea.text = "";
                    }
                }

                inputHistoryModel.filterView = viewModel.currentView;

                if (inputHistoryModel.count) {
                    var idx = inputHistoryModel.index(inputHistoryModel.count - 1, 0);

                    if (inputHistoryModel.data(idx, Konversation.InputHistoryModel.Editing) === true) {
                        inputFieldTextArea.text = inputHistoryModel.data(idx);
                        inputFieldTextArea.cursorPosition = inputHistoryModel.data(idx,
                            Konversation.InputHistoryModel.CursorPosition);
                        inputHistoryModel.remove(idx);
                    }
                }

                lastView = viewModel.currentView;
            }
        }
    }

    Connections { // WIPQTQUICK TODO Less coupling.
        target: konvUi

        onSettingsModeChanged: {
            completionPopup.close();
            inputFieldTextArea.resetCompletion();
        }
    }

    QQC2.TextArea {
        id: inputFieldTextArea

        enabled: viewModel.currentView

        property bool upPressed: false
        property bool downPressed: false
        property int historyCursor: -1
        property bool historyResetLock: false

        property string lastCompletion: ""
        property int nextMatch: 0
        property bool completionResetLock: false

        background: null

        focus: true

        topPadding: Kirigami.Units.smallSpacing
        bottomPadding: Kirigami.Units.smallSpacing

        renderType: Text.NativeRendering

        font.pixelSize: largerFontSize

        verticalAlignment: Text.AlignVCenter

        wrapMode: konvUi.settings.constrictInputField ? TextEdit.NoWrap : TextEdit.Wrap

        onCursorPositionChanged: {
            resetHistory();
            resetCompletion();
        }

        onTextChanged: {
            resetHistory();
            resetCompletion();
        }

        function commit(modifiers) {
            var send = konvApp.doAutoreplace(text, true)[0];

            if (modifiers & Qt.ControlModifier && send.startsWith("/")) { // WIPQTQUICK TODO Access command char pref.
                viewModel.currentView.sendText("/" + send);
            } else {
                viewModel.currentView.sendText(send);
            }

            inputHistoryModel.append(viewModel.currentView, text);
            text = "";
        }

        function doHistory() {
            historyResetLock = true;

            if (upPressed) {
                if (inputHistoryModel.count) {
                    if (historyCursor == -1) {
                        historyCursor = inputHistoryModel.count - 1;

                        if (text) {
                            inputHistoryModel.append(viewModel.currentView, text,
                                true, cursorPosition);
                        }

                        text = inputHistoryModel.data(inputHistoryModel.index(historyCursor, 0));
                        cursorPosition = length;
                    } else if (historyCursor > 0) {
                        --historyCursor;
                        text = inputHistoryModel.data(inputHistoryModel.index(historyCursor, 0));
                        cursorPosition = length;
                    }
                }

                upPressed = false;
            } else if (downPressed) {
                if (historyCursor != -1) {
                    if (historyCursor < (inputHistoryModel.count - 1)) {
                        ++historyCursor;

                        var idx = inputHistoryModel.index(historyCursor, 0);
                        var editing = inputHistoryModel.data(idx, Konversation.InputHistoryModel.Editing);

                        if (historyCursor == (inputHistoryModel.count - 1) && editing) {
                            text = inputHistoryModel.data(idx);
                            cursorPosition = inputHistoryModel.data(idx,
                                Konversation.InputHistoryModel.CursorPosition);
                            inputHistoryModel.remove(idx);
                            historyCursor = -1;
                        } else {
                            text = inputHistoryModel.data(idx);
                            cursorPosition = length;
                        }
                    } else {
                        historyCursor = -1;
                        text = "";
                    }
                } else if (text) {
                    inputHistoryModel.append(viewModel.currentView, text);
                    text = "";
                }

                downPressed = false;
            }

            historyResetLock = false;
        }

        function resetHistory() {
            if (historyResetLock) {
                return;
            }

            historyCursor = -1;
        }

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

            insert(cursorPosition, lastCompletion);

            completionResetLock = false;
        }

        function insertNextMatch() {
            insertMatch(nextMatch);

            ++nextMatch;

            if (nextMatch == completer.matches.count) {
                nextMatch = 0;
            }
        }

        function cancelCompletion() {
            removeCompletion();
            insert(cursorPosition, completer.prefix);
        }

        function resetCompletion() {
            if (completionResetLock || completionPopup.currentIndex != -1) {
                return;
            }

            completer.prefix = "";
            nextMatch = 0;
        }

        Connections {
            target: completionPopup

            onCurrentIndexChanged: {
                if (!activeFocus && completionPopup.currentIndex != -1) {
                    inputFieldTextArea.insertMatch(completionPopup.currentIndex);
                }
            }

            onCancelled: inputFieldTextArea.cancelCompletion()
        }

        Timer {
            id: cursorMovementCheckTimer

            property int cursorPosition: -1

            repeat: false
            interval: 0

            onTriggered: {
                if (cursorPosition == inputFieldTextArea.cursorPosition) {
                    inputFieldTextArea.doHistory();
                }
            }

            function expectMovement() {
                cursorPosition = inputFieldTextArea.cursorPosition;
                restart();
            }
        }

        Keys.onUpPressed: {
            event.accepted = false;
            upPressed = true;
            cursorMovementCheckTimer.expectMovement();
        }

        Keys.onDownPressed: {
            event.accepted = false;
            downPressed = true;
            cursorMovementCheckTimer.expectMovement();
        }

        Keys.onTabPressed: {
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
        }

        Keys.onEnterPressed: {
            event.accepted = true;
            commit(event.modifiers);
        }

        Keys.onReturnPressed: {
            event.accepted = true;
            commit(event.modifiers);
        }

        Keys.onPressed: {
            if (event.matches(StandardKey.Paste)) {
                event.accepted = true;
                inputField.pasteFromClipboard();
            }
        }

        Component.onCompleted: forceActiveFocus()

        MouseArea {
            anchors.fill: parent

            acceptedButtons: Qt.MiddleButton

            onClicked: {
                mouse.accepted = true;
                inputField.pasteFromSelection();
            }
        }
    }
}