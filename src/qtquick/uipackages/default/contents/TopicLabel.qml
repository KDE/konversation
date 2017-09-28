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

import QtQuick.Controls 2.2 as QQC2

import org.kde.kirigami 2.1 as Kirigami
import org.kde.kquickcontrolsaddons 2.0 as KQuickControlsAddons

import org.kde.konversation 1.0 as Konversation
import org.kde.konversation.uicomponents 1.0 as KUIC

QQC2.TextArea {
    id: topicLabel

    readonly property string preText: "<html><head><style> a { color: " + KUIC.ExtraColors.spotTextColor + "; }</style><body>"
    readonly property string postText: "</body></html>"

    background: null

    focus: true

    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0

    readOnly: true

    selectByMouse: true

    renderType: Text.NativeRendering
    textFormat: Text.RichText

    font.pixelSize: konvUi.largerFontSize

    color: KUIC.ExtraColors.spotTextColor

    text: viewModel.currentView ? preText + viewModel.currentView.description + postText : ""

    wrapMode: Text.WordWrap

    MouseArea {
        anchors.fill: parent

        property string hoveredLink: ""
        property string pressedLink: ""

        acceptedButtons: Qt.LeftButton | Qt.RightButton

        hoverEnabled: true
        preventStealing: true

        onContainsMouseChanged: {
            if (!containsMouse) {
                hoveredLink = "";
                pressedLink = "";
            }
        }

        onPositionChanged: {
            mouse.accepted = true;

            eventGenerator.sendMouseEvent(parent,
                KQuickControlsAddons.EventGenerator.MouseMove,
                mouse.x,
                mouse.y,
                Qt.LeftButton,
                Qt.LeftButton,
                0);

            hoveredLink = parent.linkAt(mouse.x, mouse.y)

            if (hoveredLink) {
                cursorShape = Qt.PointingHandCursor;

                return;
            }

            cursorShape = Qt.IBeamCursor;
        }

        onPressed: {
            mouse.accepted = true;

            parent.forceActiveFocus();

            pressedLink = hoveredLink;

            if (mouse.button == Qt.RightButton) {
                if (hoveredLink.startsWith("##")) {
                    contextMenus.channelMenu(mapToGlobal(mouse.x, mouse.y),
                        viewModel.currentServer,
                        hoveredLink.slice(1));
                } else {
                    var options = Konversation.IrcContextMenus.NoOptions;

                    if (hoveredLink && !hoveredLink.startsWith("##")) {
                        options = Konversation.IrcContextMenus.ShowLinkActions;
                    }

                    var actionId = contextMenus.textMenu(mapToGlobal(mouse.x, mouse.y),
                        options,
                        viewModel.currentServer,
                        parent.selectedText,
                        hoveredLink,
                        "");

                    if (actionId == Konversation.IrcContextMenus.TextCopy && parent.selectedText) {
                        clipboard.setClipboardText(parent.selectedText);
                    } else if (actionId == Konversation.IrcContextMenus.TextSelectAll) {
                        parent.selectAll();
                    }
                }

                return;
            }

            eventGenerator.sendMouseEvent(parent,
                KQuickControlsAddons.EventGenerator.MouseButtonPress,
                mouse.x,
                mouse.y,
                Qt.LeftButton,
                Qt.LeftButton,
                0);
        }

        onReleased: {
            if (mouse.button == Qt.RightButton) {
                return;
            }

            eventGenerator.sendMouseEvent(parent,
                KQuickControlsAddons.EventGenerator.MouseButtonRelease,
                mouse.x,
                mouse.y,
                Qt.LeftButton,
                Qt.LeftButton,
                0);

            if (hoveredLink && hoveredLink === pressedLink) {
                // WIPQTQUICK TODO These replaces are copied from legacy IrcView
                // code and really need reviewing if they're still necessary ...
                // HACK Replace " " with %20 for channelnames, NOTE there can't be 2 channelnames in one link
                var link = hoveredLink.replace(" ", "%20");
                // HACK Handle pipe as toString doesn't seem to decode that correctly
                link = link.replace("%7C", "|");
                // HACK Handle ` as toString doesn't seem to decode that correctly
                link = link.replace("%60", "`");

                if (hoveredLink.startsWith("##")) {
                    viewModel.currentServer.sendJoinCommand(link.slice(1));
                } else {
                    konvApp.openUrl(link);
                }
            }

            pressedLink = "";
        }

        onHoveredLinkChanged: {
            if (hoveredLink) {
                if (hoveredLink.startsWith("##")) {
                    // WIPQTQUICK TODO These replaces are copied from legacy IrcView
                    // code and really need reviewing if they're still necessary ...
                    // Replace spaces with %20 for channel links.
                    konvUi.setStatusBarTempText(i18n("Join the channel %1",
                        hoveredLink.slice(1).replace(" ", "%20")));
                } else {
                    konvUi.setStatusBarTempText(hoveredLink);
                }
            } else {
                konvUi.clearStatusBarTempText();
            }
        }
    }
}
