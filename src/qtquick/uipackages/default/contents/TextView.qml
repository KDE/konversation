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

    KUIC.ListView {
        id: textViewList

        anchors.bottom: parent.bottom

        width: parent.width
        height: Math.min(contentItem.height, parent.height)

        visible: !konvUi.settingsMode

        QQC2.ScrollBar.vertical: QQC2.ScrollBar {}

        readonly property int msgWidth: width - QQC2.ScrollBar.vertical.width

        model: messageModel
        delegate: msgComponent

        onHeightChanged: positionViewAtEnd()

        onCountChanged: {
            currentIndex = (count - 1);
            positionViewAtEnd();
        }

        Component {
            id: msgComponent

            Loader {
                id: msg

                width: ListView.view.msgWidth
                height: (active ? (konvUi.largerFontSize + messageText.height + Kirigami.Units.gridUnit)
                    : messageText.height)

                readonly property int avatarSize: konvUi.largerFontSize * 3.6
                property var authorSize: Qt.point(0, 0)

                readonly property bool showTimeStamp: !model.TimeStampMatchesPrecedingMessage
                property Item timeStamp: null

                active: !model.AuthorMatchesPrecedingMessage
                sourceComponent: metabitsComponent

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
                    id: timeStampComponent

                    Text {
                        id: timeStamp

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
                        color: "grey"

                        text: model.TimeStamp
                    }
                }

                Component {
                    id: metabitsComponent

                    Item {
                        anchors.fill: parent

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
                                anchors.margins: Kirigami.Units.smallSpacing

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

                    text: (model.Type == Konversation.MessageModel.ActionMessage
                        ? actionWrap(model.display) : model.display)

                    function actionWrap(text) {
                        return "<i>" + model.Author + "&nbsp;" + text + "</i>";
                    }

                    onLinkActivated: konvApp.openUrl(link)
                }
            }
        }
    }

}
