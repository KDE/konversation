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
import org.kde.konversation.uicomponents 1.0 as KUIC

Rectangle {
    id: item

    height: text.font.pixelSize + Kirigami.Units.gridUnit

    property color backgroundColor: Kirigami.Theme.backgroundColor
    property int textMargin: 0
    property bool isActive: {
        if ("ViewRole" in model) {
            return (model.ViewRole == viewModel.currentView);
        } else {
            return (index == ListView.view.currentIndex);
        }
    }

    property alias text: text.text

    signal clicked(var value, var mouse)
    signal doubleClicked(var value)

    color: isActive ? Kirigami.Theme.highlightColor : backgroundColor

    Text {
        id: text

        anchors.fill: parent
        anchors.leftMargin: textMargin

        renderType: Text.NativeRendering
        color: {
            if (isActive) {
                return Kirigami.Theme.highlightedTextColor;
            }

            return ("ColorRole" in model && model.ColorRole != undefined
                ? model.ColorRole : Kirigami.Theme.textColor);
        }

        font.pixelSize: konvUi.listItemFontSize

        elide: Text.ElideRight

        verticalAlignment: Text.AlignVCenter
    }

    MouseArea {
        anchors.fill: parent

        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onPressed: {
            mouse.accepted = true;

            if ("ViewRole" in model) {
                item.clicked(model.ViewRole, mouse);
            } else {
                item.clicked(index, mouse);
            }
        }

        onDoubleClicked: {
            if ("ViewRole" in model) {
                item.doubleClicked(model.ViewRole);
            } else {
                item.doubleClicked(index);
            }
        }
    }
}

