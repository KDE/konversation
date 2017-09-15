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

            readonly property int msgWidth: width - QQC2.ScrollBar.vertical.width

            model: messageModel
            delegate: msgComponent

            function scrollToEnd() {
                var newIndex = (count - 1);
                positionViewAtEnd();
                currentIndex = newIndex;
            }

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

                    property bool selected: model.Selected === true

                    readonly property bool showTimeStamp: !model.TimeStampMatchesPrecedingMessage
                    property Item timeStamp: null

                    property Item messageTextArea: messageText

                    property Item selectedBackgroundItem: null

                    active: !model.AuthorMatchesPrecedingMessage
                    sourceComponent: metabitsComponent

                    onSelectedChanged: {
                        if (selected && !selectedBackgroundItem) {
                            selectedBackgroundItem = selectedBackgroundItemComponent.createObject(msg);
                        } else if (!selected && selectedBackgroundItem) {
                            selectedBackgroundItem.destroy();
                        }
                    }

                    onShowTimeStampChanged: {
                        if (!showTimeStamp) {
                            if (timeStamp) {
                                timeStamp.destroy();
                            }
                        } else {
                            timeStamp = timeStampComponent.createObject(msg);
                        }
                    }

                    Component {
                        id: selectedBackgroundItemComponent

                        Rectangle {
                            anchors.fill: parent
                            anchors.topMargin: msg.active ? (Kirigami.Units.gridUnit / 2) : 0

                            z: 0

                            color: Kirigami.Theme.highlightColor
                        }
                    }

                    Component {
                        id: timeStampComponent

                        Text {
                            id: timeStamp

                            z: 1

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
                            color: Kirigami.Theme.disabledTextColor

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
                                color: model.NickColor

                                font.weight: Font.Bold
                                font.pixelSize: konvUi.largerFontSize

                                text: model.Author

                                onWidthChanged: msg.authorSize = Qt.point(width, height)
                            }
                        }
                    }

                    Text {
                        id: messageText

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

                        onLinkActivated: konvApp.openUrl(link)
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
        anchors.fill: parent

        onClicked: {
            mouse.accepted = false;

            var cPos = mapToItem(textListView.contentItem, mouse.x, mouse.y);
            var item = textListView.itemAt(cPos.x, cPos.y);

            if (item) {
                messageModel.toggleSelected(item.row);
            }

            if (messageModel.hasSelection()) {
                textListView.forceActiveFocus();
            }
        }
    }
}
