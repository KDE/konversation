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
    height: text.font.pixelSize + Kirigami.Units.gridUnit
    width: viewTree.width // HACK Coupling to parent components is bad

    property int textMargin: 0
    property bool isFrontView: model.IsFrontViewRole

    onIsFrontViewChanged: { // HACK While we don't have a backend prop for it
        viewTree.currentViewName = model.display;
    }

    color: isFrontView ? Kirigami.Theme.highlightColor : KUIC.ExtraColors.spotColor

    Text {
        id: text

        anchors.fill: parent
        anchors.leftMargin: textMargin

        text: model.display

        color: {
            if (isFrontView) {
                return Kirigami.Theme.highlightedTextColor;
            }

            return (model.ColorRole != undefined ? model.ColorRole : KUIC.ExtraColors.spotTextColor);
        }

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

