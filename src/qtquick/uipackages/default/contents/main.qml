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
    id: konvApp

    property int defaultSidebarWidth: Kirigami.Units.gridUnit * 11
    property int sidebarWidth: defaultSidebarWidth
    property int footerHeight: ((Kirigami.Theme.defaultFont.pixelSize * 1.1)
        + (Kirigami.Units.smallSpacing * 6))

    pageStack.defaultColumnWidth: sidebarWidth
    pageStack.initialPage: [sidebarComponent, contentComponent]
    // pageStack.separatorVisible: false TODO Needs https://phabricator.kde.org/D7509

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
                text: viewModel.currentView ? viewModel.currentView.name : ""

                color: KUIC.ExtraColors.alternateSpotTextColor
                opacity: 1.0 // Override
            }

            Text {
                id: topic

                x: (Kirigami.Units.smallSpacing * 2)
                y: channelName.height + (Kirigami.Units.smallSpacing * 2)

                width: parent.width - (Kirigami.Units.smallSpacing * 4)

                text: viewModel.currentView ? viewModel.currentView.description : ""
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

            KUIC.ScrollHelper {
                flickable: userList
                anchors.fill: userList
            }
        }
    }

    Component {
        id: sidebarComponent

        Kirigami.Page {
            leftPadding: 0
            rightPadding: 0
            topPadding: 0
            bottomPadding: 0

            QQC2.StackView {
                anchors.fill: parent

                initialItem: viewTreeComponent
            }

            Component {
                id: viewTreeComponent

                QQC2.ScrollView {
                    id: viewTree

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
            }

            MouseArea {
                id: sidebarSplitterHandle

                property int lastX: -1

                enabled: konvApp.pageStack.wideMode

                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.right: parent.right

                width: Kirigami.Units.devicePixelRatio * 2

                cursorShape: Qt.SplitHCursor

                onPressed: {
                    lastX = mouseX;
                }

                onPositionChanged: {
                    if (mouse.x > lastX) {
                        konvApp.sidebarWidth = Math.min((konvApp.defaultSidebarWidth
                            + (Kirigami.Units.gridUnit * 3)),
                            sidebarWidth + (mouse.x - lastX));
                    } else if (mouse.x < lastX) {
                        konvApp.sidebarWidth = Math.max((konvApp.defaultSidebarWidth
                            - (Kirigami.Units.gridUnit * 3)),
                            sidebarWidth - (lastX - mouse.x));
                    }
                }
            }

            EdgeHandle {
                id: sidebarRightPaginationHandle

                anchors.right: parent.right

                visible: !konvApp.pageStack.wideMode

                iconName: "go-previous"
                iconSelected: true

                color: KUIC.ExtraColors.alternateSpotColor

                onTriggered: {
                    pageStack.currentIndex = 1;
                    console.log(viewModel.currentView, viewModel.currentView.description);
                    console.log(viewModel.currentServer, viewModel.currentServer.nickname);
                }
            }

            footer: Rectangle {
                id: sidebarFooter

                width: parent.width
                height: footerHeight

                color: KUIC.ExtraColors.alternateSpotColor

                Kirigami.Icon {
                    id: optionsButton

                    anchors.verticalCenter: parent.verticalCenter

                    width: sidebarFooter.height
                    height: sidebarFooter.height - (Kirigami.Units.smallSpacing * 2)

                    selected: true

                    source: "application-menu"
                }

                MouseArea {
                    id: optionsMouseArea

                    anchors.fill: optionsButton

                    hoverEnabled: true

                    onClicked: {}
                }

                QQC1.ComboBox {
                    anchors.fill: sidebarFooter
                    anchors.leftMargin: optionsButton.width

                    visible: viewModel.currentServer

                    editable: true

                    model: viewModel.currentServer ? [viewModel.currentServer.nickname] : []

                    onAccepted: {
                        return; // WIPQTQUICK TODO Server::setNickname does something weird
                        if (viewModel.currentServer) {
                            viewModel.currentServer.setNickname(currentText);
                        }
                    }
                }
            }
        }
    }

    Component {
        id: contentComponent

        Kirigami.Page {
            leftPadding: 0
            rightPadding: 0
            topPadding: 0
            bottomPadding: 0

            onWidthChanged: {
                konvApp.pageStack.currentIndex = 1;
            }

            QQC2.ScrollView {
                id: textArea

                anchors.fill: parent

                background: Rectangle { color: Kirigami.Theme.viewBackgroundColor }

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

            footer: QQC2.TextArea { // HACK Causes warning: 'unknown: file:///home/eike/devel/install/lib64/qml/QtQuick/Controls.2/org.kde.desktop/TextArea.qml:45: ReferenceError: Window is not defined'
                id: inputField

                height: footerHeight

                focus: true

                background: Rectangle { color: Qt.darker(Kirigami.Theme.viewBackgroundColor, 1.02) }

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

            EdgeHandle {
                id: contentLeftPaginationHandle

                anchors.left: parent.left

                visible: !pageStack.wideMode

                iconName: "go-next"

                onTriggered: pageStack.currentIndex = 0
            }

            EdgeHandle {
                id: contentRightPaginationHandle

                anchors.right: parent.right

                visible: viewModel.currentView && viewModel.currentView.description != "" && !contextDrawer.drawerOpen

                iconName: "go-previous"

                onTriggered: contextDrawer.drawerOpen ? contextDrawer.close() : contextDrawer.open()
            }
        }
    }
}
