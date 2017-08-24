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

    property string user: model.Nick
    property int row: index
    property int avatarSize: nick.height * 2

    onRowChanged: metabitsLoader.active = showMetabits()

    function showMetabits() {
        if (row == (messageModel.rowCount() - 1)) {
            return true;
        }

        var prevNick = messageModel.data(messageModel.index(row + 1, 0),
            Konversation.MessageModel.Nick);

        return (prevNick != model.Nick);
    }

    Text { // WIPQTQUICK TODO Only outside loader to set avatar height
        id: nick

        visible: metabitsLoader.active

        y: Kirigami.Units.gridUnit / 2

        anchors.left: parent.left
        anchors.leftMargin: avatarSize + Kirigami.Units.gridUnit

        text: model.Nick

        font.weight: Font.Bold
        font.pixelSize: konvApp.largerFontSize
        color: model.NickColor
    }

    Loader {
        id: metabitsLoader

        active: false

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

                    color: "white"

                    font.weight: Font.Bold
                    font.pointSize: 100
                    minimumPointSize: theme.defaultFont.pointSize
                    fontSizeMode: Text.Fit

                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter

                    text: model.Nick.match(/[a-zA-Z]/).pop().toUpperCase() // HACK
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

                text: model.TimeStamp
                color: "grey"

                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    Text {
        id: messageText

        anchors.left: parent.left
        anchors.leftMargin: avatarSize + Kirigami.Units.gridUnit
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        text: {
            if (metabitsLoader.active) {
                return model.display;
            } else {
                var prevTimeStamp = messageModel.data(messageModel.index(row + 1, 0),
            Konversation.MessageModel.TimeStamp);

                if (model.TimeStamp != prevTimeStamp) {
                    return model.display + "&nbsp;&nbsp;<font color=\"grey\">" + model.TimeStamp + "</font>";
                }
            }

            return model.display;
        }

        textFormat: Text.StyledText

        font.pixelSize: konvApp.largerFontSize

        wrapMode: Text.WordWrap

        onLinkActivated: Qt.openUrlExternally(link)
    }

    Component.onCompleted: metabitsLoader.active = showMetabits()
}

