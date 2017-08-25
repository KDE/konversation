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

    property bool settingsMode: false

    property int defaultSidebarWidth: Kirigami.Units.gridUnit * 11
    property int defaultContextDrawerWidth: Kirigami.Units.gridUnit * 17
    property int sidebarWidth: defaultSidebarWidth
    property int largerFontSize: Kirigami.Theme.defaultFont.pixelSize * 1.1
    property int footerHeight: largerFontSize + (Kirigami.Units.smallSpacing * 6)

    property Item sidebarStackView
    property Item contentStackView
    property Item inputField: null

    signal openLegacyConfigDialog
    signal showMenuBar(bool show)

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

        onDrawerOpenChanged: {
            if (drawerOpen) {
                userList.forceActiveFocus();
                userList.currentIndex = -1;
            }
        }

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

        MouseArea {
            anchors.fill: parent

            onClicked: userList.forceActiveFocus()
        }

        QQC2.ScrollView {
            anchors.top: topicArea.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            background: Rectangle { color: Kirigami.Theme.viewBackgroundColor }

            KUIC.ListView {
                id: userList

                visible: viewModel.currentView && "userModel" in viewModel.currentView

                clip: true

                currentIndex: -1
                onCurrentIndexChanged: positionViewAtIndex(currentIndex, ListView.Contain)

                model: visible ? viewModel.currentView.userModel : null

                onModelChanged: currentIndex = -1

                delegate: ListItem {
                    width: userList.width

                    text: model.display
                    textMargin: Kirigami.Units.gridUnit

                    function openQuery() {
                        viewModel.currentServer.addQuery(model.display);
                        contextDrawer.close();
                    }

                    onClicked: {
                        userList.forceActiveFocus();
                        userList.currentIndex = index;
                    }

                    onDoubleClicked: openQuery();

                    Keys.onEnterPressed: {
                        event.accept = true;
                        openQuery();
                    }

                    Keys.onReturnPressed: {
                        event.accept = true;
                        openQuery();
                    }
                }

                Keys.onUpPressed: {
                    event.accept = true;

                    if (currentIndex == -1) {
                        currentIndex = 0;
                        return;
                    }

                    decrementCurrentIndex();
                }

                Keys.onDownPressed: {
                    event.accept = true;

                    if (currentIndex == -1) {
                        currentIndex = 0;
                        return;
                    }

                    incrementCurrentIndex();
                }
            }

            Keys.onPressed: {
                // WIPQTQUICK TODO Evaluating text is not good enough, needs real key event fwd
                // to make things like deadkeys work
                if (event.text != "" && inputField && !inputField.activeFocus) {
                    contextDrawer.close();
                    event.accept = true;
                    inputField.textForward(event.text);
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

            property Item viewTreeList: null

            MouseArea {
                anchors.fill: parent

                onClicked: {
                    if (viewTreeList) {
                        viewTreeList.forceActiveFocus();
                    }
                }
            }

            QQC2.StackView {
                id: sidebarStackView

                anchors.fill: parent

                background: Rectangle { color: KUIC.ExtraColors.spotColor }

                initialItem: viewTreeComponent

                onBusyChanged: {
                    if (!busy && depth == 2) {
                        currentItem.currentIndex = 0;
                        currentItem.forceActiveFocus();
                    }
                }

                pushEnter: Transition {
                    YAnimator {
                        from: sidebarStackView.height
                        to: 0
                        duration: Kirigami.Units.longDuration * 2
                        easing.type: Easing.OutCubic
                    }
                }

                pushExit: Transition {
                    OpacityAnimator {
                        from: 1.0
                        to: 0.0
                        duration: Kirigami.Units.longDuration * 2
                    }
                }

                popEnter: Transition {
                    OpacityAnimator {
                        from: 0.0
                        to: 1.0
                        duration: Kirigami.Units.longDuration * 2
                    }
                }

                popExit: Transition {
                    YAnimator {
                        from: 0
                        to: sidebarStackView.height
                        duration: Kirigami.Units.longDuration * 2
                        easing.type: Easing.OutCubic
                    }
                }

                Component.onCompleted: konvApp.sidebarStackView = sidebarStackView
            }

            Component {
                id: viewTreeComponent

                QQC2.ScrollView {
                    id: viewTree

                    KUIC.ListView {
                        id: viewTreeList

                        anchors.fill: parent

                        clip: true

                        model: viewModel

                        function showView(index, view) {
                            viewTreeList.forceActiveFocus();
                            viewModel.showView(view);

                            if (!konvApp.pageStack.wideMode) {
                                konvApp.pageStack.currentIndex = 1;
                            }
                        }

                        delegate: Column {
                            property int topLevelIndex: index

                            ListItem {
                                width: viewTreeList.width

                                textColor: KUIC.ExtraColors.spotTextColor
                                backgroundColor: KUIC.ExtraColors.spotColor

                                text: model.display
                                textMargin: Kirigami.Units.gridUnit

                                onClicked: viewTreeList.showView(topLevelIndex, value)
                            }

                            DelegateModel {
                                id: subLevelEntries

                                model: viewModel
                                rootIndex: modelIndex(index)

                                delegate: ListItem {
                                    width: viewTreeList.width

                                    textColor: KUIC.ExtraColors.spotTextColor
                                    backgroundColor: KUIC.ExtraColors.spotColor

                                    text: model.display
                                    textMargin: Kirigami.Units.gridUnit * 2

                                    onClicked: viewTreeList.showView(topLevelIndex, value)
                                }
                            }

                            Column { Repeater { model: subLevelEntries } }
                        }

                        Keys.onUpPressed: {
                            event.accept = true;
                            viewModel.showPreviousView();
                        }

                        Keys.onDownPressed: {
                            event.accept = true;
                            viewModel.showNextView();
                        }

                        Component.onCompleted: {
                            sidebar.viewTreeList = viewTreeList;
                        }
                    }
                }
            }

            Component {
                id: settingsTreeComponent

                QQC2.ScrollView {
                    id: viewTree

                    property alias currentIndex: settingsTreeList.currentIndex

                    KUIC.ListView {
                        id: settingsTreeList

                        anchors.fill: parent

                        focus: true

                        clip: true

                        currentIndex: -1

                        onCurrentIndexChanged: positionViewAtIndex(currentIndex, ListView.Contain)

                        model: ListModel {
                            ListElement { name: "Dummy 1" }
                            ListElement { name: "Dummy 2" }
                            ListElement { name: "Dummy 3" }
                        }

                        delegate: ListItem {
                            width: settingsTreeList.width

                            textColor: KUIC.ExtraColors.spotTextColor
                            backgroundColor: KUIC.ExtraColors.spotColor

                            text: name
                            textMargin: Kirigami.Units.gridUnit

                            onIsActiveChanged: {
                                if (isActive && konvApp.contentStackView.depth == 1) {
                                    konvApp.contentStackView.push("SettingsPage.qml", {"title": name});
                                }
                            }

                            onClicked: {
                                settingsTreeList.forceActiveFocus();
                                settingsTreeList.currentIndex = index;
                            }
                        }

                        Keys.onUpPressed: {
                            event.accept = true;

                            if (currentIndex == -1) {
                                currentIndex = 0;
                                return;
                            }

                            decrementCurrentIndex();
                        }

                        Keys.onDownPressed: {
                            event.accept = true;

                            if (currentIndex == -1) {
                                currentIndex = 0;
                                return;
                            }

                            incrementCurrentIndex();
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

                Rectangle {
                    id: settingsModeToggleButton

                    width: sidebarFooter.height
                    height: width

                    property bool checked: false

                    color: checked ? Kirigami.Theme.highlightColor: KUIC.ExtraColors.alternateSpotColor

                    onCheckedChanged: {
                        konvApp.settingsMode = checked;

                        if (checked) {
                            sidebarStackView.push(settingsTreeComponent);
                        } else {
                            sidebarStackView.pop();

                            if (konvApp.contentStackView.depth == 2) {
                                konvApp.contentStackView.pop();
                            }

                            konvApp.showMenuBar(false);

                            inputField.forceActiveFocus();
                        }
                    }

                    Kirigami.Icon {
                        anchors.centerIn: parent

                        width: parent.width - (Kirigami.Units.smallSpacing * 4)
                        height: width

                        selected: true

                        source: "application-menu"
                    }

                    MouseArea {
                        anchors.fill: parent

                        onClicked: parent.checked = !parent.checked
                    }
                }

                QQC1.ComboBox {
                    anchors.fill: sidebarFooter
                    anchors.leftMargin: settingsModeToggleButton.width

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

            Rectangle {
                anchors.fill: parent

                color: Kirigami.Theme.viewBackgroundColor
            }

            QQC2.StackView {
                id: contentStackView

                anchors.fill: parent
                anchors.bottomMargin: Kirigami.Units.smallSpacing

                initialItem: textViewComponent

                pushEnter: null
                pushExit: null
                popEnter: null
                popExit: null

                Component.onCompleted: konvApp.contentStackView = contentStackView
            }

            Component {
                id: textViewComponent

                QQC2.ScrollView {
                    id: textView

                    background: Rectangle { color: Kirigami.Theme.viewBackgroundColor } // WIPQTQUICK Needed?

                    KUIC.ListView {
                        id: textViewList

                        verticalLayoutDirection: ListView.BottomToTop

                        model: messageModel

                        delegate: Message {}

                        ListView.onAdd: positionViewAtEnd()
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

                visible: (!konvApp.settingsMode
                    && (viewModel.currentView && viewModel.currentView.description != "")
                    && !contextDrawer.drawerOpen)

                iconName: "go-previous"

                onTriggered: contextDrawer.drawerOpen ? contextDrawer.close() : contextDrawer.open()
            }
        }
    }
}
