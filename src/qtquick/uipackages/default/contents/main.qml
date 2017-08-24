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

import QtQml.Models 2.2
import QtQuick.Controls 1.4 as QQC1
import QtQuick.Controls 2.2 as QQC2
import QtQuick.Layouts 1.0

import org.kde.kirigami 2.1 as Kirigami
import org.kde.konversation.uicomponents 1.0 as KUIC

Kirigami.ApplicationWindow {
    id: appItem

    contextDrawer: Kirigami.OverlayDrawer {
        width: Kirigami.Units.gridUnit * 17
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
            height: visible ? channelName.height + topic.contentHeight + (Kirigami.Units.smallSpacing * 4): 0

            color: KUIC.ExtraColors.spotColor

            Kirigami.Heading {
                id: channelName

                x: (Kirigami.Units.smallSpacing * 2)

                level: 2
                text: viewTree.currentViewName

                color: KUIC.ExtraColors.alternateSpotTextColor
                opacity: 1.0 // Override
            }

            Text {
                id: topic

                x: (Kirigami.Units.smallSpacing * 2)
                y: channelName.height + (Kirigami.Units.smallSpacing * 2)

                width: parent.width - (Kirigami.Units.smallSpacing * 4)

                text: viewModel.currentTopic
                textFormat: Text.StyledText

                color: KUIC.ExtraColors.spotTextColor

                wrapMode: Text.WordWrap

                onLinkActivated: Qt.openUrlExternally(link)

                Component.onCompleted: {
                    font.pixelSize = font.pixelSize * 1.1;
                }
            }
        }

        ListView {
            id: userList

            anchors.top: topicArea.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            clip: true

            model: viewModel.currentUsersModel

            delegate: UserListItem { textMargin: Kirigami.Units.gridUnit }

            //ScrollBar.vertical: QQC2.ScrollBar {}

            KUIC.ScrollHelper {
                flickable: userList
                anchors.fill: userList
            }
        }
    }

    pageStack.initialPage: Kirigami.Page {
        leftPadding: 0
        rightPadding: 0
        topPadding: 0
        bottomPadding: 0

        QQC1.SplitView {
            anchors.fill: parent

            handleDelegate: Item {}

            QQC2.ScrollView {
                id: viewTree

                Layout.minimumWidth: Kirigami.Units.gridUnit * 8
                Layout.maximumWidth: Kirigami.Units.gridUnit * 20

                Layout.fillHeight: true

                width: Kirigami.Units.gridUnit * 11

                property string currentViewName: ""

                clip: true
                background: Rectangle { color: KUIC.ExtraColors.spotColor }

                ListView {
                    id: viewTreeList

                    anchors.fill: parent

                    clip: true

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

                    KUIC.ScrollHelper {
                        flickable: viewTreeList
                        anchors.fill: viewTreeList
                    }
                }
            }

            QQC2.ScrollView {
                id: textArea

                Layout.fillWidth: true
                Layout.fillHeight: true

                background: Rectangle { color: KUIC.ExtraColors.alternateSpotTextColor }

                ListView {
                    id: textAreaList

                    verticalLayoutDirection: ListView.BottomToTop

                    model: messageModel

                    delegate: Message {}

                    ListView.onAdd: positionViewAtEnd()

                    KUIC.ScrollHelper {
                        flickable: textAreaList
                        anchors.fill: textAreaList
                    }
                }
            }
        }

        footer: Item {
            id: footer

            height: inputField.height

            Rectangle {
                id: optionsArea

                width: viewTree.width
                height: inputField.height

                anchors.top: parent.top
                anchors.left: parent.left

                color: KUIC.ExtraColors.alternateSpotColor

                Text {
                    id: optionsButton

                    width: optionsArea.height
                    height: width

                    anchors.left: optionsArea.left
                    anchors.top: optionsArea.top

                    color: optionsMouseArea.containsMouse ? Kirigami.Theme.highlightColor : "white"

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

            QQC2.TextArea { // HACK Causes warning: 'unknown: file:///home/eike/devel/install/lib64/qml/QtQuick/Controls.2/org.kde.desktop/TextArea.qml:45: ReferenceError: Window is not defined'
                id: inputField

                height: font.pixelSize + (Kirigami.Units.smallSpacing * 6)

                anchors.left: optionsArea.right
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom

                focus: true

                clip: true
                background: Rectangle { color: Qt.darker(Kirigami.Theme.viewBackgroundColor, 1.025) }

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

            width: Kirigami.Units.gridUnit
            height: Kirigami.Units.gridUnit * 3

            visible: viewModel.currentTopic != "" && !contextDrawer.drawerOpen

            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter

            color: Kirigami.Theme.buttonBackgroundColor


            Kirigami.Icon {
                anchors.fill: parent

                selected: contextDrawerHandleMouseArea

                source: "go-previous"
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
