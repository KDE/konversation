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
import QtQuick.Controls 2.3 as QQC2

import org.kde.kirigami 2.2 as Kirigami
import org.kde.kquickcontrolsaddons 2.0 as KQuickControlsAddons

import org.kde.konversation 1.0 as Konversation
import org.kde.konversation.uicomponents 1.0 as KUIC

Kirigami.ApplicationWindow {
    id: konvUi

    readonly property int defaultSidebarWidth: Kirigami.Units.gridUnit * 11
    readonly property int defaultContextDrawerWidth: Kirigami.Units.gridUnit * 17
    property int sidebarWidth: defaultSidebarWidth
    readonly property int largerFontSize: Kirigami.Theme.defaultFont.pixelSize * 1.1
    readonly property QtObject largerFontMetrics: largerFontMetrics
    readonly property int listItemFontSize: Kirigami.Theme.defaultFont.pixelSize * 1.2
    readonly property real colorDeltaLighter: 0.08
    readonly property real colorDeltaDarker: 0.02
    readonly property int footerHeight: largerFontSize + (Kirigami.Units.smallSpacing * 6)

    property var settings: settingsObj

    property Item sidebarStackView: null
    property Item contentStackView: null
    property Item contentFooterStackView: null
    property Item inputField: null

    property bool settingsMode: false
    property Item settingsModeButtons: null

    signal openServerList
    signal openIdentities
    signal openLegacyConfigDialog
    signal showLegacyMainWindow
    signal quitApp

    signal endNotification

    signal setStatusBarTempText(string text)
    signal clearStatusBarTempText

    pageStack.defaultColumnWidth: sidebarWidth
    pageStack.initialPage: [sidebarComponent, contentComponent]
    pageStack.separatorVisible: false
    pageStack.globalToolBar.style: Kirigami.ApplicationHeaderStyle.None

    pageStack.interactive: false

    onActiveChanged: {
        if (active) {
            endNotification();
        }
    }

    Repeater {
        model: shortcutsModel

        delegate: Item {
                Shortcut {
                context: Qt.ApplicationShortcut
                sequence: model.KeySequence
                onActivated: shortcutsModel.trigger(index)
                }
        }
    }

    Shortcut {
        context: Qt.ApplicationShortcut
        sequence: "F10"
        onActivated: showLegacyMainWindow()
    }

    TextMetrics {
        id: largerFontMetrics

        font.pixelSize: largerFontSize
        text: "M"
    }

    Settings {
        id: settingsObj
    }

    KQuickControlsAddons.EventGenerator {
        id: eventGenerator
    }

    contextDrawer: Kirigami.OverlayDrawer {
        id: contextDrawer

        width: defaultContextDrawerWidth
        edge: Qt.RightEdge

        modal: !drawerPinButton.checked
        handleVisible: drawerOpen

        drawerOpen: false

        Kirigami.Theme.colorSet: contextDrawer.modal ? Kirigami.Theme.View : Kirigami.Theme.Complementary
        Kirigami.Theme.inherit: contextDrawer.modal ? true : false

        background: Rectangle { color: Kirigami.Theme.backgroundColor }

        onDrawerOpenChanged: {
            if (drawerOpen) {
                userList.forceActiveFocus();
                userList.currentIndex = -1;
            } else if (konvUi.inputField) {
                inputField.forceActiveFocus();
            }
        }

        leftPadding: 0
        rightPadding: 0
        topPadding: 0
        bottomPadding: 0

        MouseArea {
            anchors.fill: parent

            onClicked: userList.forceActiveFocus()
        }

        Rectangle {
            id: topicArea

            visible: viewModel.currentView && viewModel.currentView.name != ""

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: (visible ?
                (Math.max(viewName.height, drawerPinButton.height)
                    + (topicLabel.visible ? topicLabel.contentHeight + (Kirigami.Units.smallSpacing * 4) : 0))
                : 0)

            Kirigami.Theme.colorSet: Kirigami.Theme.Complementary
            Kirigami.Theme.inherit: false

            color: Qt.lighter(Kirigami.Theme.backgroundColor, 1 + konvUi.colorDeltaLighter)

            Kirigami.Heading {
                id: viewName

                anchors.left: parent.left
                anchors.leftMargin: (Kirigami.Units.smallSpacing * 2)
                anchors.right: parent.right
                anchors.rightMargin: drawerPinButton.width

                level: 2

                elide: Text.ElideRight

                color: Kirigami.Theme.textColor
                opacity: 1.0 // Override

                text: viewModel.currentView ? viewModel.currentView.name : ""
            }

            Rectangle {
                id: drawerPinButton

                anchors.right: parent.right

                width: footerHeight
                height: width

                property bool checked: false

                color: checked ? Kirigami.Theme.highlightColor : topicArea.color

                Kirigami.Icon {
                    anchors.centerIn: parent

                    width: parent.width - (Kirigami.Units.smallSpacing * 4)
                    height: width

                    source: "window-pin"
                }

                MouseArea {
                    anchors.fill: parent

                    onClicked: parent.checked = !parent.checked
                }
            }

            TopicLabel {
                id: topicLabel

                visible: viewModel.currentView && viewModel.currentView.description != ""

                x: (Kirigami.Units.smallSpacing * 2)
                y: viewName.height + (Kirigami.Units.smallSpacing * 2)

                width: parent.width - (Kirigami.Units.smallSpacing * 4)
            }
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

            Kirigami.Theme.colorSet: Kirigami.Theme.Complementary
            Kirigami.Theme.inherit: false

            property Item viewListView: null

            MouseArea {
                anchors.fill: parent

                onClicked: {
                    if (viewListView) {
                        viewListView.forceActiveFocus();
                    }
                }
            }

            QQC2.StackView {
                id: sidebarStackView

                anchors.fill: parent

                background: Rectangle { color: Kirigami.Theme.backgroundColor }

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

                Component.onCompleted: {
                    sidebarStackView.push("ViewSwitcher.qml");
                    konvUi.sidebarStackView = sidebarStackView;
                }
            }


            Component {
                id: settingsTreeComponent

                QQC2.ScrollView {
                    id: settingsPageSwitcher

                    property alias currentIndex: settingsPageList.currentIndex

                    ListView {
                        id: settingsPageList

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
                            width: settingsPageList.width

                            text: name
                            textMarginLeft: Kirigami.Units.gridUnit

                            onIsActiveChanged: {
                                if (isActive && konvUi.contentStackView.depth == 1) {
                                    konvUi.contentStackView.push("SettingsPage.qml", {"title": name});
                                    //konvUi.settingsModeButtons.enabled = true;
                                }
                            }

                            onClicked: {
                                settingsPageList.forceActiveFocus();
                                settingsPageList.currentIndex = index;
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

                color: Qt.lighter(Kirigami.Theme.backgroundColor, 1 + konvUi.colorDeltaLighter)

                onTriggered: pageStack.currentIndex = 1
            }

            footer: Rectangle {
                id: sidebarFooter

                width: parent.width
                height: footerHeight

                color: Qt.lighter(Kirigami.Theme.backgroundColor, 1 + konvUi.colorDeltaLighter)

                Rectangle {
                    id: settingsModeToggleButton

                    width: sidebarFooter.height
                    height: width

                    property bool checked: false

                    color: checked ? Kirigami.Theme.highlightColor : sidebarFooter.color

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

                            konvUi.inputField.forceActiveFocus();
                        }
                    }

                    Kirigami.Icon {
                        anchors.centerIn: parent

                        width: parent.width - (Kirigami.Units.smallSpacing * 4)
                        height: width

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

            Kirigami.Theme.colorSet: Kirigami.Theme.View
            Kirigami.Theme.inherit: false

            onWidthChanged: {
                konvUi.pageStack.currentIndex = 1;
            }

            Rectangle {
                anchors.fill: parent

                color: Kirigami.Theme.backgroundColor
            }

            CompletionPopup { // WIPQTQUICK TODO Lazy-load.
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

                background: Rectangle { color: Qt.darker(Kirigami.Theme.backgroundColor, 1 + konvUi.colorDeltaDarker) }

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
