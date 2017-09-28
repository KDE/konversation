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

import org.kde.kirigami 2.1 as Kirigami

import org.kde.konversation 1.0 as Konversation
import org.kde.konversation.uicomponents 1.0 as KUIC

Kirigami.ApplicationWindow {
    id: konvUi

    readonly property int defaultSidebarWidth: Kirigami.Units.gridUnit * 11
    readonly property int defaultContextDrawerWidth: Kirigami.Units.gridUnit * 17
    property int sidebarWidth: defaultSidebarWidth
    readonly property int largerFontSize: Kirigami.Theme.defaultFont.pixelSize * 1.1
    readonly property int listItemFontSize: Kirigami.Theme.defaultFont.pixelSize * 1.2
    readonly property int footerHeight: largerFontSize + (Kirigami.Units.smallSpacing * 6)

    property var settings: settingsObj

    property Item sidebarStackView: null
    property Item contentStackView: null
    property Item contentFooterStackView: null
    property Item inputField: null

    property bool settingsMode: false
    property Item settingsModeButtons: null

    signal openLegacyConfigDialog
    signal showMenuBar(bool show)

    signal setStatusBarTempText(string text)
    signal clearStatusBarTempText

    pageStack.defaultColumnWidth: sidebarWidth
    pageStack.initialPage: [sidebarComponent, contentComponent]
    pageStack.separatorVisible: false

    TextMetrics {
        id: largerFontMetrics

        font.pixelSize: largerFontSize
        text: "M"
    }

    Settings {
        id: settingsObj
    }

    contextDrawer: Kirigami.OverlayDrawer {
        id: contextDrawer

        width: defaultContextDrawerWidth
        edge: Qt.RightEdge

        modal: true
        handleVisible: drawerOpen

        drawerOpen: false

        background: Rectangle { color: Kirigami.Theme.viewBackgroundColor }

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

            visible: viewModel.currentView && viewModel.currentView.description != ""

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

        UserList {
            id: userList

            anchors.top: topicArea.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            model: contextDrawer.drawerOpen ? userModel : null
        }

        KUIC.HorizontalDragHandle {
            id: contextDrawerDragHandle

            anchors.left: parent.left

            target: contextDrawer
            defaultWidth: defaultContextDrawerWidth

            onNewWidth: contextDrawer.width = width
        }
    }

    KUIC.HorizontalDragHandle {
        id: sidebarDragHandle

        visible: pageStack.wideMode

        x: sidebarWidth - (width / 2)

        target: sidebarStackView
        defaultWidth: defaultSidebarWidth

        onNewWidth: konvUi.sidebarWidth = width
    }

    Component {
        id: sidebarComponent

        KUIC.Page {
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

                Component.onCompleted: konvUi.sidebarStackView = sidebarStackView
            }

            Component {
                id: viewTreeComponent

                QQC2.ScrollView {
                    ListView {
                        id: viewTreeList

                        clip: true

                        model: viewModel

                        function showView(view) {
                            viewModel.showView(view);

                            if (!konvUi.pageStack.wideMode) {
                                konvUi.pageStack.currentIndex = 1;
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

                                onClicked: {
                                    viewTreeList.forceActiveFocus();

                                    if (mouse.button == Qt.RightButton) {
                                        viewModel.showViewContextMenu(viewModel.index(index, 0),
                                            mapToGlobal(mouse.x, mouse.y));
                                    } else {
                                        viewTreeList.showView(value);
                                    }
                                }
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

                                    onClicked: {
                                        viewTreeList.forceActiveFocus();

                                        if (mouse.button == Qt.RightButton) {
                                            viewModel.showViewContextMenu(viewModel.index(index, 0,
                                                subLevelEntries.rootIndex),
                                                mapToGlobal(mouse.x, mouse.y));
                                        } else {
                                            viewTreeList.showView(value);
                                        }
                                    }
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

                        Component.onCompleted: sidebar.viewTreeList = viewTreeList
                    }
                }
            }

            Component {
                id: settingsTreeComponent

                QQC2.ScrollView {
                    id: viewTree

                    property alias currentIndex: settingsTreeList.currentIndex

                    ListView {
                        id: settingsTreeList

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
                                if (isActive && konvUi.contentStackView.depth == 1) {
                                    konvUi.contentStackView.push("SettingsPage.qml", {"title": name});
                                    //konvUi.settingsModeButtons.enabled = true;
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

            PageHandle {
                id: sidebarRightPaginationHandle

                anchors.right: parent.right

                visible: !konvUi.pageStack.wideMode

                iconName: "go-previous"
                iconSelected: true

                color: KUIC.ExtraColors.alternateSpotColor

                onTriggered: pageStack.currentIndex = 1
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
                        konvUi.settingsMode = checked;

                        if (checked) {
                            sidebarStackView.push(settingsTreeComponent);
                            konvUi.contentFooterStackView.push("SettingsModeButtons.qml", {"enabled": false});
                        } else {
                            sidebarStackView.pop();

                            if (konvUi.contentStackView.depth == 2) {
                                konvUi.contentStackView.pop();
                                konvUi.contentFooterStackView.pop();
                            }

                            konvUi.showMenuBar(false);

                            konvUi.inputField.forceActiveFocus();
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
        }
    }

    Component {
        id: contentComponent

        KUIC.Page {
            id: content

            leftPadding: 0
            rightPadding: 0
            topPadding: 0
            bottomPadding: 0

            onWidthChanged: {
                konvUi.pageStack.currentIndex = 1;
            }

            Rectangle {
                anchors.fill: parent

                color: Kirigami.Theme.viewBackgroundColor
            }

            CompletionPopup {
                id: completionPopup

                anchors.bottom: parent.bottom

                width: parent.width

                Component.onCompleted: konvUi.inputField.completionPopup = completionPopup
            }

            QQC2.StackView {
                id: contentStackView

                anchors.fill: parent
                anchors.bottomMargin: Kirigami.Units.smallSpacing

                pushEnter: null
                pushExit: null
                popEnter: null
                popExit: null

                Component.onCompleted: {
                    contentStackView.push("TextView.qml");
                    konvUi.contentStackView = contentStackView;
                }
            }

            footer: QQC2.StackView {
                id: contentFooterStackView

                height: ((settingsMode || konvUi.settings.constrictInputField || !konvUi.inputField)
                    ? footerHeight : inputField.height)

                background: Rectangle { color: Qt.darker(Kirigami.Theme.viewBackgroundColor, 1.02) }

                initialItem: inputFieldComponent

                pushEnter: Transition {
                    XAnimator {
                        from: contentFooterStackView.width
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
                    XAnimator {
                        from: 0
                        to: contentFooterStackView.width
                        duration: Kirigami.Units.longDuration * 2
                        easing.type: Easing.OutCubic
                    }
                }

                Component {
                    id: inputFieldComponent

                    InputField {
                        id: inputField

                        Component.onCompleted: konvUi.inputField = inputField
                    }
                }

                Component.onCompleted: konvUi.contentFooterStackView = contentFooterStackView
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

                visible: (!konvUi.settingsMode
                    && viewModel.currentView
                    && !contextDrawer.drawerOpen)

                iconName: "go-previous"

                onTriggered: contextDrawer.drawerOpen ? contextDrawer.close() : contextDrawer.open()
            }
        }
    }
}
