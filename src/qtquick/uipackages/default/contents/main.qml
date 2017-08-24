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
    property int defaultContextDrawerWidth: Kirigami.Units.gridUnit * 17
    property int sidebarWidth: defaultSidebarWidth
    property int largerFontSize: Kirigami.Theme.defaultFont.pixelSize * 1.1
    property int footerHeight: largerFontSize + (Kirigami.Units.smallSpacing * 6)

    property Item inputField: null

    pageStack.defaultColumnWidth: sidebarWidth
    pageStack.initialPage: [sidebarComponent, contentComponent]
    // pageStack.separatorVisible: false TODO Needs https://phabricator.kde.org/D7509

    contextDrawer: Kirigami.OverlayDrawer {
        id: contextDrawer

        width: defaultContextDrawerWidth
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

                font.pixelSize: largerFontSize
                color: KUIC.ExtraColors.spotTextColor

                wrapMode: Text.WordWrap

                onLinkActivated: Qt.openUrlExternally(link)
            }
        }

        QQC2.ScrollView {
            anchors.top: topicArea.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            background: Rectangle { color: Kirigami.Theme.viewBackgroundColor }

            ListView {
                id: userList

                visible: viewModel.currentView && "userModel" in viewModel.currentView

                clip: true

                model: visible ? viewModel.currentView.userModel : null

                delegate: UserListItem { textMargin: Kirigami.Units.gridUnit }

                KUIC.ScrollHelper {
                    flickable: userList
                    anchors.fill: userList
                }
            }
        }
    }

    Component {
        id: sidebarComponent

        Kirigami.Page {
            id: sidebar

            leftPadding: 0
            rightPadding: 0
            topPadding: 0
            bottomPadding: 0

            MouseArea {
                anchors.fill: parent

                onClicked: {
                    viewTreeList.forceActiveFocus();
                }
            }

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

                        function showView(index, view) {
                            viewModel.showView(view);
                            viewTreeList.forceActiveFocus();
                            viewTreeList.positionViewAtIndex(index, ListView.Visible);

                            if (!konvApp.pageStack.wideMode) {
                                konvApp.pageStack.currentIndex = 1;
                            }
                        }

                        delegate: Column {
                            property int topLevelIndex: index

                            ViewTreeItem {
                                width: viewTreeList.width

                                textMargin: Kirigami.Units.gridUnit

                                onTriggered: viewTreeList.showView(topLevelIndex, view)
                            }

                            DelegateModel {
                                id: subLevelEntries

                                model: viewModel
                                rootIndex: modelIndex(index)

                                delegate: ViewTreeItem {
                                    width: viewTreeList.width

                                    textMargin: Kirigami.Units.gridUnit * 2

                                    onTriggered: viewTreeList.showView(topLevelIndex, view)
                                }
                            }

                            Column { Repeater { model: subLevelEntries } }
                        }

                        KUIC.ScrollHelper {
                            flickable: viewTreeList
                            anchors.fill: viewTreeList
                        }

                        Keys.onUpPressed: {
                            event.accept = true;
                            viewModel.showPreviousView();
                        }

                        Keys.onDownPressed: {
                            event.accept = true;
                            viewModel.showNextView();
                        }
                    }
                }
            }

            DragHandle {
                enabled: konvApp.pageStack.wideMode

                anchors.right: parent.right

                defaultWidth: konvApp.defaultSidebarWidth

                target: sidebar

                onNewWidth: konvApp.sidebarWidth = newWidth
            }

            PageHandle {
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

            Keys.onPressed: {
                // WIPQTQUICK TODO Evaluating text is not good enough, needs real key event fwd
                // to make things like deadkeys work
                if (event.text != "" && inputField && !inputField.activeFocus) {
                    event.accept = true;
                    inputField.textForward(event.text);
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

                enabled: viewModel.currentView

                background: Rectangle { color: Qt.darker(Kirigami.Theme.viewBackgroundColor, 1.02) }

                font.pixelSize: largerFontSize

                verticalAlignment: Text.AlignVCenter

                wrapMode: TextEdit.NoWrap

                Keys.onPressed: {
                    // WIPQTQUICK TODO Evaluating text is not good enough, needs real key event fwd
                    // to make things like deadkeys work
                    if (text != "" && (event.key == Qt.Key_Enter || event.key == Qt.Key_Return)) {
                        event.accepted = true;
                        viewModel.currentView.sendText(text);
                        text = "";
                    }
                }

                function textForward(text) {
                    forceActiveFocus();
                    insert(length, text);
                    cursorPosition = length;
                }

                Component.onCompleted: {
                    konvApp.inputField = inputField;
                    forceActiveFocus();
                }
            }

            PageHandle {
                id: contentLeftPaginationHandle

                anchors.left: parent.left

                visible: !pageStack.wideMode

                iconName: "go-next"

                onTriggered: pageStack.currentIndex = 0
            }

            PageHandle {
                id: contentRightPaginationHandle

                anchors.right: parent.right

                visible: viewModel.currentView && viewModel.currentView.description != "" && !contextDrawer.drawerOpen

                iconName: "go-previous"

                onTriggered: contextDrawer.drawerOpen ? contextDrawer.close() : contextDrawer.open()
            }
        }
    }
}
