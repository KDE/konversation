/*
  Copyright (C) 2017 by Eike Hein <hein@kde.org>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License or (at your option) version 3 or any later version
  accepted by the membership of KDE e.V. (or its successor appro-
  ved by the membership of KDE e.V.), which shall act as a proxy
  defined in Section 14 of version 3 of the license.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see http://www.gnu.org/licenses/.
*/

import QtQuick 2.7

Item {
    width: parent.width
    height: nick.height + messageText.height + 25

    Rectangle {
        id: avatar

        x: 15

        width: height
        height: (nick.height * 2)

        anchors.verticalCenter: parent.verticalCenter

        color: model.NickColor

        radius: width * 0.5

        Text {
            anchors.fill: parent

            color: "white"
            text: model.Nick.match(/[a-zA-Z]/).pop().toUpperCase() // HACK
            font.weight: Font.Bold

            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter

            Component.onCompleted: {
                font.pixelSize = font.pixelSize * 2.7;
            }
        }
    }

    Text {
        id: nick

        y: 8

        anchors.left: avatar.right
        anchors.leftMargin: 15

        text: model.Nick
        color: model.NickColor
        font.weight: Font.Bold

        Component.onCompleted: {
            font.pixelSize = font.pixelSize * 1.1;
        }
    }

    Text {
        id: timeStamp

        y: 8

        height: nick.height

        anchors.left: nick.right
        anchors.leftMargin: 10

        text: model.TimeStamp
        color: "grey"

        verticalAlignment: Text.AlignVCenter
    }

    Text {
        id: messageText

        anchors.left: avatar.right
        anchors.leftMargin: 15
        anchors.right: parent.right
        anchors.top: nick.bottom
        anchors.topMargin: 3

        text: model.display
        textFormat: Text.StyledText

        wrapMode: Text.WordWrap

        onLinkActivated: Qt.openUrlExternally(link)

        Component.onCompleted: {
            font.pixelSize = font.pixelSize * 1.1;
        }
    }
}

