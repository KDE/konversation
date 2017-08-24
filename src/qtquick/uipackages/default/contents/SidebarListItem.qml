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
import org.kde.konversation.uicomponents 1.0 as KUIC

Rectangle {
    id: item

    height: text.font.pixelSize + Kirigami.Units.gridUnit

    property int textMargin: 0
    property bool isActive: {
        if ("ChatWindowRole" in model) {
            return (model.ChatWindowRole == viewModel.currentView);
        } else {
            return (index == ListView.view.currentIndex);
        }
    }

    property alias text: text.text

    signal triggered(var value)

    color: isActive ? Kirigami.Theme.highlightColor : KUIC.ExtraColors.spotColor

    Text {
        id: text

        anchors.fill: parent
        anchors.leftMargin: textMargin

        font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 1.2

        color: {
            if (isActive) {
                return Kirigami.Theme.highlightedTextColor;
            }

            return ("ColorRole" in model && model.ColorRole != undefined
                ? model.ColorRole : KUIC.ExtraColors.spotTextColor);
        }

        elide: Text.ElideRight

        verticalAlignment: Text.AlignVCenter
    }

    MouseArea {
        anchors.fill: parent

        onClicked: {
            if ("ChatWindowRole" in model) {
                item.triggered(model.ChatWindowRole);
            } else {
                item.triggered(index);
            }
        }
    }
}

