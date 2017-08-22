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

import QtQuick 2.7

import QtQml.Models 2.2
import QtQuick.Controls 1.4 as QQC1
import QtQuick.Controls 2.2 as QQC2

import org.kde.kirigami 2.1 as Kirigami

Kirigami.ApplicationWindow {
    id: appItem

    contextDrawer: Kirigami.OverlayDrawer {
        width: Kirigami.Units.gridUnit * 15
        edge: Qt.RightEdge

        modal: true
        handleVisible: drawerOpen
        drawerOpen: false

        leftPadding: 0
        rightPadding: 0
        topPadding: 0
        bottomPadding: 0

        Rectangle {
            id: topicArea

            visible: viewModel.currentTopic != ""

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: visible ? topic.contentHeight + (Kirigami.Units.smallSpacing * 4) : 0

            Text {
                id: topic

                x: (Kirigami.Units.smallSpacing * 2)
                y: (Kirigami.Units.smallSpacing * 2)

                width: parent.width - (Kirigami.Units.smallSpacing * 4)

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

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: topicArea.bottom

            height: visible ? Math.max(1, Kirigami.Units.devicePixelRatio / 2) : 0

            color: "#c1c3c4"
        }

        QQC2.ScrollView {
            id: userList

            anchors.top: topicBorder.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            clip: true
            background: Rectangle { color: "#fcfcfc" }

            Column {
                Repeater {

                    model: viewModel.currentUsersModel

                    delegate: UserListItem { textMargin: Kirigami.Units.gridUnit }
                }
            }
        }
    }

    pageStack.initialPage: Kirigami.Page {
        leftPadding: 0
        rightPadding: 0
        topPadding: 0
        bottomPadding: 0

        Rectangle {
            width: viewTree.width

            anchors.top: viewTree.top
            anchors.bottom: viewTree.bottom

            color: "#fcfcfc"
        }

        QQC2.ScrollView {
            id: viewTree

            width: Kirigami.Units.gridUnit * 11

            anchors.top: parent.top
            anchors.bottom: parent.bottom

            clip: true
            background: Rectangle { color: "#fcfcfc" }

            Column {
                Repeater {
                    id: topLevelEntries

                    model: viewModel

                    delegate: Column {
                        ViewTreeItem { textMargin: Kirigami.Units.gridUnit }

                        DelegateModel {
                            id: subLevelEntries

                            model: viewModel
                            rootIndex: modelIndex(index)

                            delegate: ViewTreeItem { textMargin: Kirigami.Units.gridUnit * 2}
                        }

                        Column { Repeater { model: subLevelEntries } }
                    }
                }
            }
        }

        Rectangle {
            id: viewTreeRightBorder

            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: viewTree.right

            width: Math.max(1.5, Kirigami.Units.devicePixelRatio)

            color: "#c1c3c4"
        }

        QQC2.ScrollView {
            id: textArea

            anchors.left: viewTreeRightBorder.right
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom

            background: Rectangle { color: "white" }

            ListView {
                model: messageModel

                delegate: Message {}

                ListView.onAdd: positionViewAtEnd()
            }
        }

        footer: Item {
            id: footer

            height: inputField.height + footerBorder.height

            Rectangle {
                id: footerBorder

                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right

                height: Math.max(1, Kirigami.Units.devicePixelRatio / 2)

                color: "#c1c3c4"
            }

            Rectangle {
                id: optionsArea

                width: viewTree.width
                height: inputField.height

                anchors.top: footerBorder.bottom
                anchors.left: parent.left

                color: "#e8e9ea"

                Text {
                    id: optionsButton

                    width: optionsArea.height
                    height: width

                    anchors.left: optionsArea.left
                    anchors.top: optionsArea.top

                    opacity: 0.6

                    color: optionsMouseArea.containsMouse ? Kirigami.Theme.buttonHoverColor : "black"

                    font.weight: Font.Bold
                    font.pointSize: 100
                    minimumPointSize: theme.defaultFont.pointSize
                    fontSizeMode: Text.Fit

                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter

                    text: "⚙"
                }

                MouseArea {
                    id: optionsMouseArea

                    anchors.fill: optionsButton

                    hoverEnabled: true

                    onClicked: {}
                }

                QQC1.ComboBox {
                    anchors.fill: optionsArea
                    anchors.leftMargin: optionsButton.width

                    editable: true

                    model: [viewModel.currentNick]

                    onAccepted: viewModel.setCurrentNick(currentText)
                }
            }

            Rectangle {
                id: optionsAreaBorder

                anchors.top: footerBorder.bottom
                anchors.bottom: parent.bottom
                anchors.left: optionsArea.right

                width: Math.max(1.5, Kirigami.Units.devicePixelRatio)

                color: "#c1c3c4"
            }

            QQC2.TextArea { // HACK Causes warning: 'unknown: file:///home/eike/devel/install/lib64/qml/QtQuick/Controls.2/org.kde.desktop/TextArea.qml:45: ReferenceError: Window is not defined'
                id: inputField

                height: font.pixelSize + (Kirigami.Units.smallSpacing * 6)

                anchors.left: optionsAreaBorder.right
                anchors.right: parent.right
                anchors.top: footerBorder.bottom
                anchors.bottom: parent.bottom

                focus: true

                clip: true
                background: Rectangle {}

                verticalAlignment: Text.AlignVCenter

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
        }

        Rectangle {
            id: contextDrawerHandle

            width: Kirigami.Units.gridUnit / 2
            height: Kirigami.Units.gridUnit * 3

            visible: viewModel.currentTopic != "" && !contextDrawer.drawerOpen

            anchors.right: parent.right
            anchors.rightMargin: 0 // contextDrawer.drawerOpen ? contextDrawer.width + Kirigami.Units.devicePixelRatio : 0
            anchors.verticalCenter: parent.verticalCenter

            color: contextDrawer.drawerOpen ? "#c1c3c4" : "#ececec"

            Text {
                anchors.fill: parent

                font.weight: Font.Bold

                color: contextDrawerHandleMouseArea.containsMouse ? Kirigami.Theme.buttonHoverColor : "black"
                opacity: 0.6

                text: "◀" // contextDrawer.drawerOpen ? "▶" : "◀"

                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter

                Component.onCompleted: {
                    font.pixelSize = parent.width - (Kirigami.Units.devicePixelRatio * 2);
                }
            }

            MouseArea {
                id: contextDrawerHandleMouseArea

                anchors.fill: parent

                hoverEnabled: true

                onClicked: contextDrawer.drawerOpen ? contextDrawer.close() : contextDrawer.open()
            }
        }
    }
}
