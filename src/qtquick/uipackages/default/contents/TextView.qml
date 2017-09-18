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

import QtQuick.Controls 2.2 as QQC2

import org.kde.kirigami 2.1 as Kirigami
import org.kde.kquickcontrolsaddons 2.0 as KQuickControlsAddons

import org.kde.konversation 1.0 as Konversation
import org.kde.konversation.uicomponents 1.0 as KUIC

Item {
    id: textView

    QQC2.ScrollView {
        anchors.fill: parent

        ListView {
            id: textListView

            anchors.bottom: parent.bottom

            width: parent.width
            height: parent.height

            visible: !konvUi.settingsMode

            QQC2.ScrollBar.vertical: QQC2.ScrollBar {}

            property bool scrollUp: false
            property bool scrollDown: false

            readonly property int msgWidth: width - QQC2.ScrollBar.vertical.width

            model: messageModel
            delegate: msgComponent

            function scrollToEnd() {
                if (messageModel.hasSelection
                    || (mouseOverlay.inlineSelectionItem
                    && mouseOverlay.inlineSelectionItem.hasSelectedText)) {
                    return;
                }

                var newIndex = (count - 1);
                positionViewAtEnd();
                currentIndex = newIndex;
            }

            function cancelAutoScroll() {
                scrollUp = false;
                scrollDown = false;
            }

            onContentYChanged: {
                if (contentY == 0) {
                    scrollUp = false;
                }

                if (contentY == contentItem.height - height) {
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
                    contentY = contentItem.height - height;
                } else {
                    contentY = contentY;
                    smoothY.enabled = false;
                }
            }

            Behavior on contentX { id: smoothX; enabled: false; SmoothedAnimation { velocity: 500 } }
            Behavior on contentY { id: smoothY; enabled: false; SmoothedAnimation { velocity: 500 } }

            Connections {
                target: textListView.contentItem

                onHeightChanged: {
                    if (textListView.contentItem.height <= textView.height) {
                        textListView.height = textListView.contentItem.height;
                    } else {
                        textListView.height = textView.height;
                    }
                }
            }

            Connections {
                target: messageModel

                onRowsInserted: scrollDownTimer.restart()
                onRowsRemoved: scrollDownTimer.restart()
                onModelReset: scrollDownTimer.restart()
            }

            Timer {
                id: scrollDownTimer

                interval: 0
                repeat: false

                onTriggered: textListView.scrollToEnd()
            }

            Component {
                id: msgComponent

                Loader {
                    id: msg

                    width: ListView.view.msgWidth
                    height: (active ? (Math.max(avatarSize, konvUi.largerFontSize + messageText.height + Kirigami.Units.gridUnit))
                        : messageText.height)

                    property int row: index

                    readonly property int avatarSize: konvUi.largerFontSize * 3.3
                    property var authorSize: Qt.point(0, 0)

                    property int contentWidth: {
                        var width = 0;;

                        if (timeStamp) {
                            width = Math.max(timeStamp.x + timeStamp.width,
                                messageText.x + messageText.contentWidth);
                        } else {
                            width = Math.max(messageText.x + messageText.contentWidth,
                                avatarSize + Kirigami.Units.gridUnit + authorSize.x);
                        }

                        return Math.min(textView.width, width);
                    }

                    property bool selected: model.Selected === true
                    property bool hasSelectedText: (inlineSelectionTextItem
                        && inlineSelectionTextItem.selectedText.length)

                    property bool allowInlineSelection: (mouseOverlay.inlineSelectionItem == msg
                        && !mouseOverlay.tapSelecting)

                    readonly property bool showTimeStamp: !model.TimeStampMatchesPrecedingMessage

                    property Item timeStamp: null
                    property Item messageTextArea: messageText
                    property Item selectedBackgroundItem: null
                    property Item inlineSelectionTextItem: null

                    active: !model.AuthorMatchesPrecedingMessage
                    sourceComponent: metabitsComponent

                    onSelectedChanged: {
                        if (selected && !selectedBackgroundItem) {
                            selectedBackgroundItem = selectedBackgroundItemComponent.createObject(msg);
                        } else if (!selected && selectedBackgroundItem) {
                            selectedBackgroundItem.destroy();
                            selectedBackgroundItem = null;
                        }
                    }

                    onShowTimeStampChanged: {
                        if (!showTimeStamp) {
                            if (timeStamp) {
                                timeStamp.destroy();
                                timeStamp = null;
                            }
                        } else {
                            timeStamp = timeStampComponent.createObject(msg);
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

                    Component {
                        id: selectedBackgroundItemComponent

                        Rectangle {
                            anchors.top: parent.top
                            anchors.topMargin: msg.active ? (Kirigami.Units.gridUnit / 2) : 0

                            x: messageText.x

                            height: (messageText.y + messageText.contentHeight) - anchors.topMargin
                            width: msg.contentWidth - x

                            z: 0

                            color: Kirigami.Theme.highlightColor
                        }
                    }

                    Component {
                        id: timeStampComponent

                        Text {
                            id: timeStamp

                            z: 2

                            readonly property bool collides: (messageText.x
                                + messageText.implicitWidth
                                + Kirigami.Units.smallSpacing + width > parent.width)
                            readonly property int margin: Kirigami.Units.gridUnit / 2

                            x: messageText.x + margin + (active ? authorSize.x : messageText.contentWidth)

                            y: {
                                if (!active) {
                                    return messageText.y + ((largerFontMetrics.height / 2) - (height / 2));
                                } else {
                                    return (Kirigami.Units.gridUnit / 2) + ((authorSize.y / 2) - (height / 2));
                                }
                            }

                            renderType: Text.NativeRendering
                            color: selected ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.disabledTextColor

                            text: model.TimeStamp
                        }
                    }

                    Component {
                        id: metabitsComponent

                        Item {
                            anchors.fill: parent

                            z: 1

                            Rectangle {
                                id: avatar

                                x: Kirigami.Units.gridUnit / 2
                                y: Kirigami.Units.gridUnit / 2

                                width: avatarSize
                                height: avatarSize

                                color: model.NickColor

                                radius: width * 0.5

                                Text {
                                    anchors.fill: parent
                                    anchors.margins: Kirigami.Units.devicePixelRatio * 5

                                    renderType: Text.QtRendering
                                    color: "white"

                                    font.weight: Font.Bold
                                    font.pointSize: 100
                                    minimumPointSize: theme.defaultFont.pointSize
                                    fontSizeMode: Text.Fit

                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter

                                    text: {
                                        // WIPQTQUICK HACK TODO Probably doesn't work with non-latin1.
                                        var match = model.Author.match(/([a-zA-Z])([a-zA-Z])/);
                                        var abbrev = match[1].toUpperCase();

                                        if (match.length > 2) {
                                            abbrev += match[2].toLowerCase();
                                        }

                                        return abbrev;
                                    }
                                }
                            }

                            Text {
                                id: author

                                y: Kirigami.Units.gridUnit / 2

                                anchors.left: parent.left
                                anchors.leftMargin: avatarSize + Kirigami.Units.gridUnit

                                renderType: Text.NativeRendering
                                color: selected ? Kirigami.Theme.highlightedTextColor : model.NickColor

                                font.weight: Font.Bold
                                font.pixelSize: konvUi.largerFontSize

                                text: model.Author

                                onWidthChanged: msg.authorSize = Qt.point(width, height)
                            }
                        }
                    }

                    Component {
                        id: inlineSelectionTextItemComponent

                        Item {
                            id: inlineSelectionText

                            anchors.fill: messageText

                            z: 1

                            property Item textArea: textArea
                            property alias selectedText: textArea.selectedText

                            Connections {
                                target: mouseOverlay

                                onClearInlineSelectedText: {
                                    inlineSelectionText.destroy();
                                    msg.inlineSelectionTextItem = null;
                                }

                                onTapSelectingChanged: {
                                    if (!mouseOverlay.tapSelecting) {
                                        inlineSelectionText.destroy();
                                        msg.inlineSelectionTextItem = null;
                                    }
                                }
                            }

                            Connections {
                                target: mouseOverlay.inlineSelectionItem

                                enabled: mouseOverlay.inlineSelectionItem != msg

                                onHasSelectedTextChanged: {
                                    inlineSelectionText.destroy();
                                    msg.inlineSelectionTextItem = null;
                                }
                            }

                            QQC2.TextArea {
                                id: textArea

                                anchors.fill: parent

                                background: Item {}

                                leftPadding: 0
                                rightPadding: 0
                                topPadding: 0
                                bottomPadding: 0

                                // Init from messageText.
                                renderType: messageText.renderType
                                textFormat: Text.RichText
                                font: messageText.font
                                wrapMode: messageText.wrapMode
                                color: messageText.color
                                text: messageText.text

                                readOnly: true

                                selectByMouse: true
                                persistentSelection: true

                                onSelectedTextChanged: {
                                    if (!selectedText.length
                                        && !msg.allowInlineSelection) {
                                        inlineSelectionText.destroy();
                                        msg.inlineSelectionTextItem = null;
                                    }
                                }

                                Component.onCompleted: {
                                    forceActiveFocus();
                                }
                            }
                        }
                    }

                    Text {
                        id: messageText

                        opacity: allowInlineSelection ? 0.0 : 1.0

                        z: 1

                        anchors.left: parent.left
                        anchors.leftMargin: avatarSize + Kirigami.Units.gridUnit
                        anchors.right: parent.right
                        anchors.rightMargin: (timeStamp && timeStamp.collides
                            ? timeStamp.margin + timeStamp.width : 0)
                        anchors.bottom: parent.bottom

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
                }
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

        property var rowsToSelect: []
        property int pressedRow: -1
        property bool tapSelecting: false

        property Item inlineSelectionItem: null

        property string hoveredLink: ""
        property string pressedLink: ""

        signal clearInlineSelectedText

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

        function processClick(x, y) {
            var item = itemAt(x, y);

            // Adding a gridUnit for bigger finger targets.
            if (item
                && x >= item.messageTextArea.x
                && x <= (item.contentWidth + Kirigami.Units.gridUnit)) {

                if (rowsToSelect.length && konvUi.shiftPressed) {
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

                var messageTextPos = mapToItem(item.messageTextArea, mouse.x, mouse.y);
                hoveredLink = item.messageTextArea.linkAt(messageTextPos.x, messageTextPos.y);

                if (!mouseOverlay.tapSelecting && hoveredLink) {
                    cursorShape = Qt.PointingHandCursor;

                    return;
                }

                if (mouse.x >= item.messageTextArea.x && mouse.x <= item.contentWidth) {
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

            if (tapSelecting) {
                processClick(mouse.x, mouse.y);
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
                konvApp.openUrl(hoveredLink);
            }

            pressedRow = -1;
            pressedLink = "";
            pressAndHoldTimer.stop();
            textListView.cancelAutoScroll();
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
                mouseOverlay.processClick(mouseOverlay.mouseX, mouseOverlay.mouseY);
            }
        }

        KQuickControlsAddons.EventGenerator {
            id: eventGenerator
        }
    }
}
