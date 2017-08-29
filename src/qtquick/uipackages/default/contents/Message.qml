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

import org.kde.kirigami 2.1 as Kirigami

import org.kde.konversation 1.0 as Konversation

Item {
    id: msg

    width: ListView.view.width
    height: (metabitsLoader.active
        ? (konvApp.largerFontSize + messageText.height + Kirigami.Units.gridUnit)
        : messageText.height)

    readonly property int avatarSize: nick.height * 2

    property bool selectable: false
    property Item selectableText: null

    readonly property bool linkHovered: selectableText && selectableText.hoveredLink != ""

    onSelectableChanged: {
        if (selectable) {
            selectableText = selectableTextComponent.createObject(msg);
            selectableText.forceActiveFocus();
        } else if (selectableText) {
            selectableText.destroy();
        }
    }

    Text { // WIPQTQUICK TODO Only outside loader to set avatar height.
        id: nick

        visible: metabitsLoader.active

        y: Kirigami.Units.gridUnit / 2

        anchors.left: parent.left
        anchors.leftMargin: avatarSize + Kirigami.Units.gridUnit

        renderType: Text.NativeRendering
        color: model.NickColor

        font.weight: Font.Bold
        font.pixelSize: konvApp.largerFontSize

        text: model.Author
    }

    Loader {
        id: metabitsLoader

        active: !model.AuthorMatchesPrecedingMessage

        anchors.fill: parent

        sourceComponent: metabitsComponent
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
                id: timeStamp

                y: Kirigami.Units.gridUnit / 2

                height: nick.height

                anchors.left: parent.left
                anchors.leftMargin: (avatarSize
                    + Kirigami.Units.gridUnit
                    + nick.width
                    + (Kirigami.Units.gridUnit / 2))

                renderType: Text.NativeRendering
                color: "grey"

                text: model.TimeStamp
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    Text {
        id: messageText

        visible: !selectable

        anchors.left: parent.left
        anchors.leftMargin: avatarSize + Kirigami.Units.gridUnit
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        renderType: Text.NativeRendering
        textFormat: Text.RichText

        font.pixelSize: konvApp.largerFontSize

        wrapMode: Text.WordWrap

        text: {
            var t = model.display;

            if (model.Type == Konversation.MessageModel.ActionMessage) {
                t = "<i>" + model.Nick + "&nbsp;" + t + "</i>";
            }

            if (metabitsLoader.active) {
                return t;
            } else if (!model.TimeStampMatchesPrecedingMessage) {
                return t + "&nbsp;&nbsp;<font color=\"grey\">" + model.TimeStamp + "</font>";
            }

            return t;
        }

        onLinkActivated: Qt.openUrlExternally(link)
    }

    Component {
        id: selectableTextComponent

        TextEdit {
            anchors.fill: messageText

            readOnly: true
            selectByMouse: true
            persistentSelection: true

            renderType: Text.NativeRendering
            textFormat: Text.RichText

            font.pixelSize: konvApp.largerFontSize

            wrapMode: Text.WordWrap

            text: messageText.text

            onLinkActivated: Qt.openUrlExternally(link)
        }
    }
}

