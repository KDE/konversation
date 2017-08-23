/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2017 Eike Hein <hein@kde.org>
*/

import QtQuick 2.0

import org.kde.kirigami 2.1 as Kirigami

Rectangle {
    height: text.font.pixelSize + Kirigami.Units.gridUnit
    width: userList.width // HACK Coupling to parent components is bad

    property int textMargin: 0

    Text {
        id: text

        anchors.fill: parent
        anchors.leftMargin: textMargin

        text: model.display

        color: "black" // HACK
        opacity: 0.7

        elide: Text.ElideRight

        verticalAlignment: Text.AlignVCenter

        Component.onCompleted: {
            font.pixelSize = font.pixelSize * 1.2;
        }
    }
}

