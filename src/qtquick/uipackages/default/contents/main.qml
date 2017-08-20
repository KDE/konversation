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

Kirigami.ApplicationItem {
    id: appItem

    Component {
        id: contextDrawerComponent

        Kirigami.ContextDrawer {
            width: Kirigami.Units.gridUnit * 15

            QQC2.ScrollView {
                id: userList

                anchors.fill: parent

                clip: true
                background: Rectangle { color: "#fcfcfc" }

                Column {
                    Repeater {
                        id: topLevelEntries

                        model: viewModel.currentUsersModel

                        delegate: ViewTreeItem { textMargin: Kirigami.Units.gridUnit }
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        // HACK QQC2.Popup (that's what Kirigami.ContextDrawer is) can't find
        // the QQuickWidget's internal QQuickWindow if it's instanciated along-
        // side the Kirigami.ApplicationItem immediately, so we do it later. I
        // consider this a Qt bug.
        Qt.callLater(setupContextDrawer);
    }

    function setupContextDrawer() {
        contextDrawer = contextDrawerComponent.createObject(appItem, {'parent' : appItem });
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
            anchors.bottom: optionsArea.top
            anchors.bottomMargin: viewTreeBottomBorder.height

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
            id: optionsArea

            width: viewTree.width
            height: inputField.height

            anchors.left: parent.left
            anchors.bottom: parent.bottom

            color: "grey"

            opacity: 0.06
        }

        QQC1.ComboBox {
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

        Rectangle {
            id: viewTreeRightBorder

            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: viewTree.right

            width: Math.max(1.5, Kirigami.Units.devicePixelRatio)

            color: "#c1c3c4"
        }

        Rectangle {
            id: viewTreeBottomBorder

            anchors.left: parent.left
            anchors.right: viewTree.right
            anchors.top: viewTree.bottom

            height: Math.max(1, Kirigami.Units.devicePixelRatio / 2)

            color: "#c1c3c4"
        }

        Rectangle {
            id: inputFieldBorder

            anchors.left: viewTreeRightBorder.right
            anchors.right: parent.right
            anchors.rightMargin: contextDrawer && contextDrawer.visible ? Kirigami.Units.devicePixelRatio : 0
            anchors.bottom: inputField.top

            height: Math.max(1, Kirigami.Units.devicePixelRatio / 2)

            color: "#c1c3c4"
        }

        QQC2.TextArea { // HACK Causes warning: 'unknown: file:///home/eike/devel/install/lib64/qml/QtQuick/Controls.2/org.kde.desktop/TextArea.qml:45: ReferenceError: Window is not defined'
            id: inputField

            height: font.pixelSize + (Kirigami.Units.smallSpacing * 6)

            anchors.left: viewTreeRightBorder.right
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            focus: true

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

        Rectangle {
            id: topicArea

            visible: viewModel.currentTopic != ""

            anchors.top: parent.top
            anchors.left: viewTreeRightBorder.right
            anchors.right: parent.right
            height: visible ? topic.contentHeight + (Kirigami.Units.smallSpacing * 4) : 0

            color: "#fcfcfc"

            Text {
                id: topic

                x: (Kirigami.Units.smallSpacing * 2)
                y: (Kirigami.Units.smallSpacing * 2)

                width: parent.width - Kirigami.Units.gridUnit

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
            anchors.rightMargin: contextDrawer && contextDrawer.visible ? Kirigami.Units.devicePixelRatio : 0
            anchors.top: topicArea.bottom

            height: visible ? Math.max(1, Kirigami.Units.devicePixelRatio / 2) : 0

            color: "#c1c3c4"
        }

        QQC2.ScrollView {
            id: textArea

            anchors.top: topicBorder.bottom
            anchors.left: viewTreeRightBorder.right
            anchors.right: parent.right
            anchors.bottom: inputFieldBorder.top

            background: Rectangle { color: "white" }

            ListView {
                model: messageModel

                delegate: Message {}

                ListView.onAdd: positionViewAtEnd()
            }
        }

        Rectangle {
            id: fakeNickListUncollapseThumb

            width: Kirigami.Units.gridUnit / 2
            height: Kirigami.Units.gridUnit * 3

            anchors.right: parent.right
            anchors.rightMargin: contextDrawer && contextDrawer.visible ? Kirigami.Units.devicePixelRatio : 0
            anchors.verticalCenter: parent.verticalCenter

            color: contextDrawer && contextDrawer.visible ? "#c1c3c4" : "#ececec"

            Text {
                anchors.fill: parent

                font.weight: Font.Bold

                color: fakeNickListUncollapseThumbMouseArea.containsMouse ? Kirigami.Theme.buttonHoverColor : "black"
                opacity: 0.6

                text: contextDrawer && contextDrawer.visible ? "▶" : "◀"

                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter

                Component.onCompleted: {
                    font.pixelSize = parent.width - (Kirigami.Units.devicePixelRatio * 2);
                }
            }

            MouseArea {
                id: fakeNickListUncollapseThumbMouseArea

                anchors.fill: parent

                hoverEnabled: true

                onClicked: contextDrawer && contextDrawer.visible ? contextDrawer.close() : contextDrawer.open()
            }
        }
    }
}
