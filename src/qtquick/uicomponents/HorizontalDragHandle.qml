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
    id: dragHandle

    anchors.top: parent.top
    anchors.bottom: parent.bottom

    width: Kirigami.Units.devicePixelRatio * 2

    property var target: null

    property int defaultWidth: -1
    property int dragRange: (Kirigami.Units.gridUnit * 5)

    property bool _configured: target != null && defaultWidth != -1
    property int _pressX: -1
    property int _pressTargetWidth: -1

    signal newWidth(int width)

    cursorShape: Qt.SplitHCursor

    onPressed: {
        if (!_configured) {
            return;
        }

        _pressX = mapToGlobal(mouse.x, 0).x;
        _pressTargetWidth = target.width;
    }

    onPressedChanged: {
        if (target && "interactive" in target) {
            target.interactive = !pressed;
        }
    }

    onPositionChanged: {
        if (!_configured) {
            return;
        }

        var mappedX = mapToGlobal(mouse.x, 0).x;

        if ("edge" in target && target.edge == Qt.RightEdge) {
            if (mappedX > _pressX) {
                target.width = Math.max((defaultWidth - dragRange),
                    _pressTargetWidth - (mappedX - _pressX));
            } else if (mappedX < _pressX) {
                target.width = Math.min((defaultWidth + dragRange),
                    _pressTargetWidth + (_pressX - mappedX));
            }
        } else {
            if (mappedX > _pressX) {
                dragHandle.newWidth(Math.min((defaultWidth + dragRange),
                    _pressTargetWidth + (mappedX - _pressX)));
            } else if (mappedX < _pressX) {
                dragHandle.newWidth(Math.max((defaultWidth - dragRange),
                    _pressTargetWidth - (_pressX - mappedX)));
            }
        }
    }
}
