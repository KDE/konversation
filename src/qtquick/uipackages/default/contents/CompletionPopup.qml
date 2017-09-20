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

import org.kde.kirigami 2.1 as Kirigami

Loader {
    id: completionPopup

    height: Math.min(parent.height, _itemHeight * Math.min(completer.matches.count, 7))

    z: active ? 1 : 0

    readonly property int currentIndex: active ? item.currentIndex : -1
    readonly property int _itemHeight: konvUi.listItemFontSize + Kirigami.Units.gridUnit

    active: false

    sourceComponent: popupComponent

    function open() {
        active = true;
    }

    function close() {
        konvUi.inputField.forceActiveFocus();
        active = false;
    }

    Component {
        id: popupComponent

        QQC2.ScrollView {
            property alias currentIndex: completionList.currentIndex

            ListView {
                id: completionList

                onHeightChanged: positionViewAtIndex(currentIndex, ListView.Contain)

                onActiveFocusChanged: {
                    if (!activeFocus) {
                        completionPopup.close();
                    }
                }

                clip: true

                currentIndex: 0
                onCurrentIndexChanged: positionViewAtIndex(currentIndex, ListView.Contain)

                model: completer.matches

                delegate: ListItem {
                    width: completionList.width
                    height: completionPopup._itemHeight

                    backgroundColor: Qt.darker(Kirigami.Theme.viewBackgroundColor, 1.016)

                    text: model.display
                    textMargin: Kirigami.Units.gridUnit

                    onClicked: completionList.currentIndex = index
                }

                function prev() {
                    if (currentIndex == 0 && count > 1) {
                        currentIndex = (count - 1);
                    } else {
                        decrementCurrentIndex();
                    }
                }

                function next() {
                    if (currentIndex == (count - 1)) {
                        currentIndex = 0;
                    } else {
                        incrementCurrentIndex();
                    }
                }

                Keys.onUpPressed: {
                    event.accept = true;
                    prev();
                }

                Keys.onDownPressed: {
                    event.accept = true;
                    next();
                }

                Keys.onTabPressed: {
                    event.accept = true;
                    next();
                }

                Keys.onPressed: {
                    // WIPQTQUICK TODO Evaluating text is not good enough, needs real key event fwd
                    // to make things like deadkeys work
                    if (event.text != "" && konvUi.inputField && !konvUi.inputField.activeFocus) {
                        event.accept = true;
                        konvUi.inputField.textForward(event.text);
                    }
                }

                Component.onCompleted: forceActiveFocus()
            }
        }
    }
}
