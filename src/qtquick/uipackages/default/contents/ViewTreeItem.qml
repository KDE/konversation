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

import QtQuick 2.0

import org.kde.kirigami 2.1 as Kirigami

Rectangle {
    height: text.font.pixelSize + Kirigami.Units.gridUnit
    width: viewTree.width // HACK Coupling to parent components is bad

    property int textMargin: 0

    color: model.IsFrontViewRole ? "#ececec" : "#fcfcfc"

    Text {
        id: text

        anchors.fill: parent
        anchors.leftMargin: textMargin

        text: model.display

        color: model.ColorRole != undefined ? model.ColorRole : "black"
        opacity: 0.7

        elide: Text.ElideRight

        verticalAlignment: Text.AlignVCenter

        Component.onCompleted: {
            font.pixelSize = font.pixelSize * 1.2;
        }
    }

    MouseArea {
        anchors.fill: parent

        onClicked: viewModel.showView(model.ChatWindowRole)
    }
}

