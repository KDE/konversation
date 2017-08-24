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

MouseArea {
    id: handle

    property var target: null
    property int defaultWidth: -1
    property int range: (Kirigami.Units.gridUnit * 3)

    property int _lastX: -1

    signal newWidth(int newWidth)

    anchors.top: parent.top
    anchors.bottom: parent.bottom

    width: Kirigami.Units.devicePixelRatio * 2

    cursorShape: Qt.SplitHCursor

    onPressed: {
        _lastX = mouseX;
    }

    onPositionChanged: {
        if (!target || defaultWidth == -1) {
            return;
        }

        if (mouse.x > _lastX) {
            handle.newWidth(Math.min((defaultWidth + range),
                target.width + (mouse.x - _lastX)));
        } else if (mouse.x < _lastX) {
            handle.newWidth(Math.max((defaultWidth - range),
                target.width - (_lastX - mouse.x)));
        }
    }
}
