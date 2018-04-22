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

import org.kde.kirigami 2.2 as Kirigami

Rectangle {
    id: handle

    anchors.verticalCenter: parent.verticalCenter

    width: Kirigami.Settings.isMobile ? 3 * Kirigami.Units.gridUnit : Kirigami.Units.gridUnit
    height: Kirigami.Settings.isMobile ? 6 * Kirigami.Units.gridUnit : 3 * Kirigami.Units.gridUnit

    property alias iconName: icon.source
    property alias iconSelected: icon.selected

    signal triggered

    Kirigami.Theme.colorSet: Kirigami.Theme.View
    color: Qt.darker(Kirigami.Theme.backgroundColor, 1 + konvUi.colorDeltaDarker)

    Kirigami.Icon {
        id: icon

        anchors.fill: parent
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent

        hoverEnabled: true

        onClicked: handle.triggered()
    }
}
