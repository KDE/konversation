/*
  Copyright (C) 2017 by Eike Hein <hein@kde.org>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License or (at your option) version 3 or any later version
  accepted by the membership of KDE e.V. (or its successor appro-
  ved by the membership of KDE e.V.), which shall act as a proxy
  defined in Section 14 of version 3 of the license.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see http://www.gnu.org/licenses/.
*/

import QtQuick 2.9

import QtQuick.Controls 1.4
import QtQml.Models 2.2

import org.kde.plasma.core 2.1 as PlasmaCore

Item {
    Rectangle { // HACK
        width: viewTree.width

        anchors.top: viewTree.top
        anchors.bottom: viewTree.bottom

        color: "#fcfcfc"
    }

    ScrollView {
        id: viewTree

        width: 350

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.bottomMargin: inputField.height + inputFieldBorder.height

        frameVisible: false

        horizontalScrollBarPolicy: Qt.ScrollBarAlwaysOff

        Column {
            Repeater {
                id: topLevelEntries

                model: viewModel

                delegate: Column {
                    ViewTreeItem { textMargin: 15 /* HACK */ }

                    DelegateModel {
                        id: subLevelEntries

                        model: viewModel
                        rootIndex: modelIndex(index)

                        delegate: ViewTreeItem { textMargin: 55 /* HACK */ }
                    }

                    Column { Repeater { model: subLevelEntries } }
                }
            }
        }
    }

    Rectangle { // HACK
        id: optionsArea

        width: viewTree.width
        height: inputField.height

        anchors.left: parent.left
        anchors.bottom: parent.bottom

        color: "grey"

        opacity: 0.06
    }

    ComboBox {
        anchors.fill: optionsArea
        anchors.leftMargin: optionsArea.height

        editable: true

        model: [viewModel.currentNick]

        onAccepted: viewModel.setCurrentNick(currentText)
    }

    Text {
        id: optionsButton

        width: optionsArea.height
        height: width

        anchors.left: optionsArea.left
        anchors.top: optionsArea.top
        anchors.topMargin: -3

        color: optionsMouseArea.containsMouse ? "blue" : "black"

        text: "⚙"

        opacity: 0.6

        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter

        Component.onCompleted: {
            font.pixelSize = Math.ceil((optionsArea.height/5) * 4);
        }
    }

    MouseArea {
        id: optionsMouseArea

        anchors.fill: optionsButton

        hoverEnabled: true

        onClicked: {}
    }

    Rectangle {
        id: viewTreeRightBorder

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: viewTree.right

        width: 2

        color: "grey"

        opacity: 0.5
    }

    Rectangle {
        id: viewTreeBottomBorder

        anchors.left: parent.left
        anchors.right: viewTree.right
        anchors.top: viewTree.bottom

        height: 1

        color: "grey"

        opacity: 0.5
    }

    Rectangle {
        id: inputFieldBorder

        anchors.left: viewTreeRightBorder.right
        anchors.right: parent.right
        anchors.bottom: inputField.top

        height: 1

        color: "grey"

        opacity: 0.5
    }

    TextArea {
        id: inputField

        height: font.pixelSize + 40 // HACK

        anchors.left: viewTreeRightBorder.right
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        focus: true

        frameVisible: false

        verticalAlignment: Text.AlignVCenter
        textMargin: 10 // HACK

        wrapMode: TextEdit.NoWrap

        Keys.onPressed: {
            if (text != "" && (event.key == Qt.Key_Enter || event.key == Qt.Key_Return)) {
                event.accepted = true;
                viewModel.sendTextToFrontView(text);
                text = "";
            }
        }

        Component.onCompleted: {
            font.pixelSize = font.pixelSize * 1.1;
        }
    }

    PlasmaCore.SortFilterModel {
        id: filteredMessageModel

        sourceModel: messageModel

        filterRole: "ViewId"
        filterString: viewModel.currentViewId
    }

    Rectangle {
        id: topicArea

        visible: viewModel.currentTopic != ""

        anchors.top: parent.top
        anchors.left: viewTreeRightBorder.right
        anchors.right: parent.right
        height: topic.contentHeight + 20

        color: "#fcfcfc"

        Text {
            id: topic

            x: 10
            y: 10

            width: parent.width - 20

            text: viewModel.currentTopic
            textFormat: Text.StyledText

            wrapMode: Text.WordWrap

            onLinkActivated: Qt.openUrlExternally(link)

            Component.onCompleted: {
                font.pixelSize = font.pixelSize * 1.1;
            }
        }
    }

    Rectangle {
        id: topicBorder

        visible: viewModel.currentTopic != ""

        anchors.left: viewTreeRightBorder.right
        anchors.right: parent.right
        anchors.top: topicArea.bottom

        height: visible ? 1 : 0

        color: "grey"

        opacity: 0.5
    }

    ScrollView {
        id: textArea

        anchors.top: topicBorder.bottom
        anchors.left: viewTreeRightBorder.right
        anchors.right: parent.right
        anchors.bottom: inputFieldBorder.top

        ListView {
            model: filteredMessageModel

            delegate: Message {}

            ListView.onAdd: positionViewAtEnd()
        }
    }

    Rectangle {
        id: fakeNickListUncollapseThumb

        width: 15
        height: 80

        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter

        color: "#ececec"

        Text {
            anchors.fill: parent

            font.weight: Font.Bold

            color: "grey"
            opacity: 0.6

            text: "◀"

            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter

            Component.onCompleted: {
                font.pixelSize = parent.width - 4;
            }
        }
    }
}
