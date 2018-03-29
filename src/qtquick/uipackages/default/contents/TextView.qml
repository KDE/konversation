/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2017-2018 Eike Hein <hein@kde.org>
*/

import QtQuick 2.7

import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2 as QQC2

import org.kde.kirigami 2.2 as Kirigami
import org.kde.kquickcontrolsaddons 2.0 as KQuickControlsAddons

import org.kde.konversation 1.0 as Konversation
import org.kde.konversation.uicomponents 1.0 as KUIC

/* WIPQTQUICK TODO Missing vs. ircview.cpp - not all necessary needed anymore
 * Background images
 * Bidi stuff
 * Search
 * Remember lines
 * Drop handling
 * Start text selection by press and move outside of text areas
 * Text selection by keyboard
 * Finish auto-scrolldown
 * Scroll by keyboard
 * Selecting by double and tripple click
 * Text/file drops
 */

Item {
    id: textView

    QQC2.ScrollView {
        anchors.fill: parent

        anchors.topMargin: Math.max(0, textView.height - textListView.contentHeight)

        ListView {
            id: textListView

            anchors.bottom: parent.bottom

            width: parent.width

            visible: !konvUi.settingsMode

            QQC2.ScrollBar.vertical: QQC2.ScrollBar { id: verticalScrollbar }

            property bool scrollUp: false
            property bool scrollDown: false
            property bool scrollToEnd: true
            property bool userScrolling: false

            readonly property int msgWidth: width - verticalScrollbar.width

            model: messageModel

            Component {
                id: avatarComponent

                Rectangle {
                    id: avatar

                    width: parent.avatarSize
                    height: parent.avatarSize

                    Layout.column: 0
                    Layout.rowSpan: parent.sectionLeader ? 2 : 1
                    Layout.topMargin: Kirigami.Units.smallSpacing
                    Layout.leftMargin: parent.columnSpacing
                    Layout.alignment: Qt.AlignTop
                    Layout.maximumWidth: width

                    color: parent.m.NickColor

                    radius: width * 0.5

                    Text {
                        anchors.fill: parent
                        anchors.margins: Kirigami.Units.devicePixelRatio * 5

                        renderType: Text.QtRendering
                        color: "white"

                        font.weight: Font.Bold
                        font.pointSize: 100
                        minimumPointSize: Kirigami.Theme.defaultFont.pointSize
                        fontSizeMode: Text.Fit

                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter

                        text: {
                            // WIPQTQUICK HACK TODO Probably doesn't work with non-latin1.
                            var match = parent.parent.m.Author.match(/([a-zA-Z])([a-zA-Z])/);
                            var abbrev = match[1].toUpperCase();

                            if (match.length > 2) {
                                abbrev += match[2].toLowerCase();
                            }

                            return abbrev;
                        }
                    }
                }
            }

            Component {
                id: sectionHeaderComponent

                Text {
                    id: author

                    Layout.row: 0
                    Layout.column: 1
                    Layout.columnSpan: parent.timeStamp ? 2 : 1
                    Layout.topMargin: Kirigami.Units.smallSpacing

                    renderType: Text.NativeRendering
                    textFormat: Text.StyledText

                    font.weight: Font.Bold
                    font.pixelSize: konvUi.largerFontSize

                    color: parent.parent.selected ? Kirigami.Theme.highlightedTextColor : parent.m.NickColor

                    text: parent.m.Author

                    Component.onCompleted: parent.parent.authorTextArea = author
                }
            }

            Component {
                id: timeStampComponent

                Item {
                    Layout.row: parent.sectionLeader ? 1 : 0
                    Layout.column: 2
                    Layout.minimumWidth: width
                    Layout.alignment: Qt.AlignTop
                    Layout.topMargin: parent.sectionLeader ? Kirigami.Units.smallSpacing / 2 : 0

                    width: text.implicitWidth
                    height: text.implicitHeight

                    Text {
                        id: text

                        x: parent.parent.timeStampOffset
                        width: parent.width

                        renderType: Text.NativeRendering

                        color: parent.parent.selected ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.disabledTextColor

                        text: parent.parent.m.TimeStamp
                    }
                }
            }

            Component {
                id: selectedBackgroundItemComponent

                Rectangle {
                    anchors.fill: parent
                    anchors.topMargin: parent.authorTextArea ? parent.authorTextArea.y : 0
                    anchors.bottomMargin: parent.msgTextArea.Layout.bottomMargin
                    anchors.leftMargin: parent.msgTextArea.x
                    anchors.rightMargin: parent.width - parent.contentWidth

                    z: 0

                    color: Kirigami.Theme.highlightColor
                }
            }

            Component {
                id: inlineSelectionTextItemComponent

                Item {
                    id: inlineSelectionText

                    x: parent.msgTextArea.x
                    y: parent.msgTextArea.y

                    width: parent.msgTextArea.width
                    height: parent.msgTextArea.height

                    z: 1

                    property Item textArea: textArea
                    property alias selectedText: textArea.selectedText

                    Connections {
                        target: mouseOverlay

                        onClearInlineSelectedText: {
                            inlineSelectionText.destroy();
                            parent.inlineSelectionTextItem = null;
                        }

                        onTapSelectingChanged: {
                            if (!mouseOverlay.tapSelecting) {
                                inlineSelectionText.destroy();
                                parent.inlineSelectionTextItem = null;
                            }
                        }
                    }

                    Connections {
                        target: mouseOverlay.inlineSelectionItem

                        enabled: mouseOverlay.inlineSelectionItem != parent

                        onHasSelectedTextChanged: {
                            inlineSelectionText.destroy();
                            parent.inlineSelectionTextItem = null;
                        }
                    }

                    QQC2.TextArea {
                        id: textArea

                        anchors.fill: parent

                        background: null

                        leftPadding: 0
                        rightPadding: 0
                        topPadding: 0
                        bottomPadding: 0

                        // Init from parent.msgTextArea.
                        renderType: parent.parent.msgTextArea.renderType
                        textFormat: Text.RichText
                        font: parent.parent.msgTextArea.font
                        wrapMode: parent.parent.msgTextArea.wrapMode
                        color: parent.parent.msgTextArea.color
                        text: parent.parent.msgTextArea.text

                        readOnly: true

                        selectByMouse: true
                        persistentSelection: true

                        onSelectedTextChanged: {
                            if (!selectedText.length
                                && !parent.parent.allowInlineSelection) {
                                inlineSelectionText.destroy();
                                parent.parent.inlineSelectionTextItem = null;
                            }
                        }

                        Component.onCompleted: {
                            forceActiveFocus();
                        }
                    }
                }
            }

            delegate: Item {
                id: msg

                width: ListView.view.msgWidth
                implicitWidth: width
                height: msgLayout.implicitHeight

                readonly property int row: index

                readonly property bool selected: model.Selected === true
                readonly property bool hasSelectedText: (inlineSelectionTextItem
                    && inlineSelectionTextItem.selectedText.length)
                readonly property string selectedText: hasSelectedText ? inlineSelectionTextItem.selectedText : ""
                readonly property bool allowInlineSelection: (mouseOverlay.inlineSelectionItem == msg
                    && !mouseOverlay.tapSelecting)

                readonly property int contentWidth: Math.max(msgLayout.timeStamp ? msgLayout.timeStamp.x + msgLayout.timeStamp.width + msgLayout.timeStampOffset
                    : msgText.x + msgText.contentWidth, authorTextArea ? authorTextArea.x + authorTextArea.width : 0)

                readonly property Item msgTextArea: msgText
                property Item authorTextArea: null
                property Item selectedBackgroundItem: null
                property Item inlineSelectionTextItem: null

                onSelectedChanged: {
                    if (selected && !selectedBackgroundItem) {
                        selectedBackgroundItem = selectedBackgroundItemComponent.createObject(msg);
                    } else if (!selected && selectedBackgroundItem) {
                        selectedBackgroundItem.destroy();
                        selectedBackgroundItem = null;
                    }
                }

                onAllowInlineSelectionChanged: {
                    if (allowInlineSelection && !inlineSelectionTextItem) {
                        inlineSelectionTextItem = inlineSelectionTextItemComponent.createObject(msg);
                    } else if (!allowInlineSelection
                        && inlineSelectionTextItem
                        && !inlineSelectionTextItem.selectedText.length) {
                        inlineSelectionTextItem.destroy();
                        inlineSelectionTextItem = null;
                    }
                }

                GridLayout {
                    id: msgLayout

                    width: parent.width

                    z: 1

                    readonly property var m: model

                    readonly property bool sectionLeader: !model.AuthorMatchesPrecedingMessage

                    readonly property int avatarSize: (konvUi.largerFontMetrics.height * 2) + Kirigami.Units.smallSpacing
                    property Item avatar: null

                    property Item sectionHeader: null

                    readonly property bool showTimeStamp: !model.TimeStampMatchesPrecedingMessage
                    property Item timeStamp: null
                    readonly property int timeStampOffset: -Math.max(msgText.width - msgText.implicitWidth,
                        msgText.width - msgText.contentWidth)

                    rowSpacing: 0
                    columnSpacing: Kirigami.Units.smallSpacing

                    onSectionLeaderChanged: checkDeco()
                    onShowTimeStampChanged: checkTimeStamp()

                    function checkDeco() {
                        if (!sectionLeader) {
                            if (avatar) {
                                avatar.destroy();
                                avatar = null;
                            }

                            if (sectionHeader) {
                                sectionHeader.destroy();
                                sectionHeader = null;
                            }
                        } else if (!sectionHeader || !avatar) {
                            avatar = avatarComponent.createObject(msgLayout);
                            sectionHeader = sectionHeaderComponent.createObject(msgLayout);
                        }
                    }

                    function checkTimeStamp() {
                        if (!showTimeStamp) {
                            if (timeStamp) {
                                timeStamp.destroy();
                                timeStamp = null;
                            }
                        } else if (!timeStamp) {
                            timeStamp = timeStampComponent.createObject(msgLayout);
                        }
                    }

                    Text {
                        id: msgText

                        opacity: allowInlineSelection ? 0.0 : 1.0

                        Layout.row: parent.sectionLeader ? 1 : 0
                        Layout.column: 1
                        Layout.topMargin: parent.sectionLeader ? Kirigami.Units.smallSpacing / 2 : 0
                        Layout.bottomMargin: index == (textListView.count - 1) ? 0 : Kirigami.Units.smallSpacing
                        Layout.leftMargin: parent.avatar ? 0 : parent.avatarSize + (parent.columnSpacing * 2)
                        Layout.alignment: Qt.AlignTop
                        Layout.fillWidth: true

                        renderType: Text.NativeRendering
                        textFormat: Text.StyledText

                        font.pixelSize: konvUi.largerFontSize

                        wrapMode: Text.Wrap

                        color: selected ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor

                        text: (model.Type == Konversation.MessageModel.ActionMessage
                            ? actionWrap(model.display) : model.display)

                        function actionWrap(text) {
                            return "<i>" + model.Author + "&nbsp;" + text + "</i>";
                        }
                    }

                    Component.onCompleted: {
                        checkDeco();
                        checkTimeStamp();
                    }
                }
            }

            add: Transition {
                SequentialAnimation {
                    ScriptAction { script: Qt.callLater(textListView.checkScrollToEnd) }
                }
            }

            function isScrolledToEnd() {
                // This is absolutely terrible, but due to the intricacies
                // of how ListView manages the size of its contentItem,
                // there is no better way to determine that the view has
                // been visually scrolled to its end (actually: we go "to
                // the last message" here, as it's what we semantically
                // care about). Trust me. No, atYEnd doesn't work. No,
                // various calculations involving contentItem.height, originY,
                // contentY and the list view height don't work. This works.
                var mapped = mapToItem(contentItem, 0, height);
                return (indexAt(0, mapped.y) == (count - 1));
            }

            function checkScrollToEnd() {
                if (!userScrolling && scrollToEnd) {
                    positionViewAtEnd();
                }
            }

            function cancelAutoScroll() {
                scrollUp = false;
                scrollDown = false;
            }

            onMovementStarted: userScrolling = true
            onMovementEnded: userScrolling = false

            onUserScrollingChanged: {
                if (!userScrolling) {
                    scrollToEnd = isScrolledToEnd();
                }
            }

            onContentYChanged: {
                if (contentY == 0) {
                    scrollUp = false;
                }

                if (contentY == (contentItem.height - height) + originY) {
                    scrollDown = false;
                }
            }

            onScrollUpChanged: {
                if (scrollUp && visibleArea.heightRatio < 1.0) {
                    smoothY.enabled = true;
                    contentY = 0;
                } else {
                    contentY = contentY;
                    smoothY.enabled = false;
                }
            }

            onScrollDownChanged: {
                if (scrollDown && visibleArea.heightRatio < 1.0) {
                    smoothY.enabled = true;
                    contentY = (contentItem.height - height) + originY;
                } else {
                    contentY = contentY;
                    smoothY.enabled = false;
                }
            }

            Behavior on contentX { id: smoothX; enabled: false; SmoothedAnimation { velocity: 500 } }
            Behavior on contentY { id: smoothY; enabled: false; SmoothedAnimation { velocity: 500 } }

            Connections {
                target: messageModel

                onFilterViewChanged: textListView.scrollToEnd = true
            }

            Connections {
                target: textView

                onHeightChanged: textListView.checkScrollToEnd()
            }

            Connections {
                target: textListView.contentItem

                onHeightChanged: textListView.checkScrollToEnd()
            }

            Keys.onPressed: {
                if (event.matches(StandardKey.Copy) || event.matches(StandardKey.Cut)) {
                    event.accepted = true;
                    messageModel.copySelectionToClipboard();
                }
            }
        }
    }

    MouseArea {
        id: mouseOverlay

        anchors.fill: parent
        anchors.rightMargin: Math.ceil(verticalScrollbar.width)

        property var rowsToSelect: []
        property int pressedRow: -1
        property bool tapSelecting: false

        property Item inlineSelectionItem: null

        property string hoveredLink: ""
        property string pressedLink: ""

        signal clearInlineSelectedText

        acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton

        hoverEnabled: true

        function itemAt(x, y) {
            var cPos = mapToItem(textListView.contentItem, x, y);
            return textListView.itemAt(cPos.x, cPos.y);
        }

        function toggleSelected(row) {
            var index = rowsToSelect.indexOf(row);

            if (index != -1) {
                var selection = rowsToSelect;
                selection.splice(index, 1);
                rowsToSelect = selection;
            } else {
                var selection = rowsToSelect;
                selection.push(row);
                selection.sort();
                rowsToSelect = selection;
            }
        }

        function processClick(x, y, shiftPressed) {
            var item = itemAt(x, y);

            // Adding a gridUnit for bigger finger targets.
            if (item
                && x >= item.msgTextArea.x
                && x <= (item.contentWidth + Kirigami.Units.gridUnit)) {

                if (rowsToSelect.length && shiftPressed) {
                    var start = Math.min(rowsToSelect[0], item.row);
                    var end = Math.max(rowsToSelect[0], item.row);
                    var selection = [];

                    for (var i = start; i <= end; ++i) {
                        selection.push(i);
                    }

                    rowsToSelect = selection;
                } else {
                    toggleSelected(item.row);
                }
            } else {
                rowsToSelect = [];
            }
        }

        onRowsToSelectChanged: messageModel.clearAndSelect(rowsToSelect)

        onContainsMouseChanged: {
            if (!containsMouse) {
                pressAndHoldTimer.stop();
                hoveredLink = "";
                pressedLink = "";

                if (inlineSelectionItem
                    && !inlineSelectionItem.hasSelectedText) {
                    inlineSelectionItem = null;
                }
            }
        }

        onPositionChanged: {
            mouse.accepted = false;

            var item = itemAt(mouse.x, mouse.y);

            if (item) {
                if (pressedRow != -1) {
                    // Trigger auto-scroll.
                    // WIPQTQUICK TODO: The selection should be updated even without
                    // pointer movement while autoscrolling.
                    textListView.scrollUp = (mouse.y <= 0 && textListView.contentY > 0);
                    textListView.scrollDown = (mouse.y >= textListView.height
                        && textListView.contentY < textListView.contentItem.height - textListView.height);

                    if (item.row != pressedRow && pressed) {
                        var start = Math.min(pressedRow, item.row);
                        var end = Math.max(pressedRow, item.row);
                        var selection = [];

                        for (var i = start; i <= end; ++i) {
                            selection.push(i);
                        }

                        rowsToSelect = selection;

                        pressAndHoldTimer.stop();
                    } else if (rowsToSelect.length) {
                        rowsToSelect = [item.row];
                    }
                }

                var messageTextPos = mapToItem(item.msgTextArea, mouse.x, mouse.y);
                var authorTextPos = item.active ? mapToItem(item.authorTextArea, mouse.x, mouse.y) : null;

                if (item.active && item.authorTextArea.contains(authorTextPos)) {
                    hoveredLink = "#" +  item.authorTextArea.text;
                } else if (item.msgTextArea.contains(messageTextPos)) {
                    hoveredLink = item.msgTextArea.linkAt(messageTextPos.x, messageTextPos.y)
                } else {
                    hoveredLink = "";
                }

                if (!mouseOverlay.tapSelecting && hoveredLink) {
                    cursorShape = Qt.PointingHandCursor;

                    return;
                }

                if (mouse.x >= item.msgTextArea.x && mouse.x <= item.contentWidth) {
                    cursorShape = Qt.IBeamCursor;

                    if (!messageModel.hasSelection && !tapSelecting) {
                        inlineSelectionItem = item;

                        eventGenerator.sendMouseEvent(inlineSelectionItem.inlineSelectionTextItem.textArea,
                            KQuickControlsAddons.EventGenerator.MouseMove,
                            messageTextPos.x,
                            messageTextPos.y,
                            Qt.LeftButton,
                            Qt.LeftButton,
                            0);
                    }

                    return;
                }
            } else {
                pressAndHoldTimer.stop();

                if (inlineSelectionItem && !inlineSelectionItem.hasSelectedText) {
                    inlineSelectionItem = null;
                }

                if (pressedRow != -1) {
                    if (mouse.y < 0) {
                        var topRow = itemAt(0, 0).row;
                        var start = Math.min(pressedRow, topRow);
                        var end = Math.max(pressedRow, topRow);
                        var selection = [];

                        for (var i = start; i <= end; ++i) {
                            selection.push(i);
                        }

                        rowsToSelect = selection;
                    } else if (mouse.y > height) {
                        var bottomRow = itemAt(0, height - 1).row;
                        var start = Math.min(pressedRow, bottomRow);
                        var end = Math.max(pressedRow, bottomRow);
                        var selection = [];

                        for (var i = start; i <= end; ++i) {
                            selection.push(i);
                        }

                        rowsToSelect = selection;
                    }
                }
            }

            cursorShape = Qt.ArrowCursor;
        }

        onPressed: {
            mouse.accepted = true;

            textListView.forceActiveFocus();

            pressedLink = hoveredLink;

            if (mouse.button == Qt.RightButton) {
                var options = Konversation.IrcContextMenus.ShowTitle;

                if (viewModel.currentView) {
                    options = options | viewModel.currentView.contextMenuOptions;

                    if (hoveredLink && !hoveredLink.startsWith("#")) {
                        options = options | Konversation.IrcContextMenus.ShowLinkActions;
                    }
                }

                if (hoveredLink.startsWith("##")) {
                    contextMenus.channelMenu(mapToGlobal(mouse.x, mouse.y),
                        viewModel.currentServer,
                        hoveredLink.slice(1));
                } else if (hoveredLink.startsWith("#")) {
                    contextMenus.nickMenu(mapToGlobal(mouse.x, mouse.y),
                        options,
                        viewModel.currentServer,
                        [hoveredLink.slice(1)],
                        viewModel.name);
                } else {
                    var actionId = contextMenus.textMenu(mapToGlobal(mouse.x, mouse.y),
                        options,
                        viewModel.currentServer,
                        (inlineSelectionItem && inlineSelectionItem.hasSelectedText)
                            ? inlineSelectionItem.selectedText : "",
                        hoveredLink,
                        (viewModel.currentView
                            && (viewModel.currentView.contextMenuOptions & Konversation.IrcContextMenus.ShowNickActions)
                            ? viewModel.currentView.name : ""));

                    if (actionId == Konversation.IrcContextMenus.TextCopy) {
                        if (inlineSelectionItem && inlineSelectionItem.hasSelectedText) {
                            clipboard.setClipboardText(inlineSelectionItem.selectedText);
                        } else if (messageModel.hasSelection) {
                            messageModel.copySelectionToClipboard();
                        }
                    } else if (actionId == Konversation.IrcContextMenus.TextSelectAll) {
                        messageModel.selectAll();
                    } else if (viewModel.currentView
                        && (viewModel.currentView.contextMenuOptions & Konversation.IrcContextMenus.ShowNickActions)) {
                        contextMenus.processNickAction(actionId,
                            viewModel.currentServer,
                            [viewModel.currentView.name],
                            "");
                    }
                }

                return;
            }

            if (tapSelecting) {
                processClick(mouse.x, mouse.y, (mouse.modifiers & Qt.ShiftModifier));
            } else {
                rowsToSelect = [];

                var item = itemAt(mouse.x, mouse.y);

                if (item && cursorShape == Qt.IBeamCursor) {
                    pressedRow = item.row;

                    if (inlineSelectionItem) {
                        var mPos = mapToItem(inlineSelectionItem.inlineSelectionTextItem.textArea,
                            mouse.x,
                            mouse.y);
                        eventGenerator.sendMouseEvent(inlineSelectionItem.inlineSelectionTextItem.textArea,
                            KQuickControlsAddons.EventGenerator.MouseButtonPress,
                            mPos.x,
                            mPos.y,
                            Qt.LeftButton,
                            Qt.LeftButton,
                            0);

                        if (!inlineSelectionItem.hasSelectedText) {
                            pressAndHoldTimer.restart();
                        }
                    }
                } else {
                    pressedRow = -1;
                    inlineSelectionItem = null;
                    clearInlineSelectedText();
                    pressAndHoldTimer.stop();
                }
            }
        }

        onReleased: {
            if (mouse.button == Qt.RightButton) {
                return;
            }

            if (inlineSelectionItem) {
                var mPos = mapToItem(inlineSelectionItem.inlineSelectionTextItem.textArea,
                    mouse.x,
                    mouse.y);
                eventGenerator.sendMouseEvent(inlineSelectionItem.inlineSelectionTextItem.textArea,
                    KQuickControlsAddons.EventGenerator.MouseButtonRelease,
                    mPos.x,
                    mPos.y,
                    Qt.LeftButton,
                    Qt.LeftButton,
                    0);
            }

            if (!tapSelecting && hoveredLink && hoveredLink === pressedLink) {
                // WIPQTQUICK TODO These replaces are copied from legacy IrcView
                // code and really need reviewing if they're still necessary ...
                // HACK Replace " " with %20 for channelnames, NOTE there can't be 2 channelnames in one link
                var link = hoveredLink.replace(" ", "%20");
                // HACK Handle pipe as toString doesn't seem to decode that correctly
                link = link.replace("%7C", "|");
                // HACK Handle ` as toString doesn't seem to decode that correctly
                link = link.replace("%60", "`");

                if (hoveredLink.startsWith("##")) {
                    viewModel.currentServer.sendJoinCommand(link.slice(1));
                } else if (hoveredLink.startsWith("#")) {
                    viewModel.currentServer.addQuery(link.replace("#", ""));
                } else {
                    konvApp.openUrl(link);
                }
            }

            pressedRow = -1;
            pressedLink = "";
            pressAndHoldTimer.stop();
            textListView.cancelAutoScroll();
        }

        onClicked: {
            if (mouse.button == Qt.MiddleButton && konvUi.inputField) { // WIPQTQUICK TODO Less coupling please, use signal-slot.
                mouse.accepted = true;
                konvUi.inputField.pasteFromSelection();
            }
        }

        onHoveredLinkChanged: {
            if (hoveredLink) {
                if (hoveredLink.startsWith("##")) {
                    // WIPQTQUICK TODO These replaces are copied from legacy IrcView
                    // code and really need reviewing if they're still necessary ...
                    // Replace spaces with %20 for channel links.
                    konvUi.setStatusBarTempText(i18n("Join the channel %1",
                        hoveredLink.slice(1).replace(" ", "%20")));
                } else if (hoveredLink.startsWith("#")) {
                    konvUi.setStatusBarTempText(i18n("Open a query with %1",
                        hoveredLink.slice(1)));
                } else {
                    konvUi.setStatusBarTempText(hoveredLink);
                }
            } else {
                konvUi.clearStatusBarTempText();
            }
        }

        Connections {
            target: messageModel

            onHasSelectionChanged: {
                if (!messageModel.hasSelection) {
                    mouseOverlay.tapSelecting = false;
                } else {
                    mouseOverlay.inlineSelectionItem = null;
                    mouseOverlay.clearInlineSelectedText();
                }
            }
        }

        Timer {
            id: pressAndHoldTimer

            interval: mouseOverlay.pressAndHoldInterval
            repeat: false

            onTriggered: {
                if (mouseOverlay.inlineSelectionItem
                    && mouseOverlay.inlineSelectionItem.hasSelectedText) {
                    return;
                }

                if (messageModel.hasSelection) {
                    return;
                }

                mouseOverlay.tapSelecting = true;
                mouseOverlay.processClick(mouseOverlay.mouseX, mouseOverlay.mouseY, null);
            }
        }
    }
}
