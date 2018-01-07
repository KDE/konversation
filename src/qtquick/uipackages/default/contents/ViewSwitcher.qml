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

import org.kde.kirigami 2.2 as Kirigami

import org.kde.konversation 1.0

Item {
    id: viewSwitcher

    readonly property string activityColor: "#008000" // WIPQTQUICK TODO Hard-coded green because Breeze' positiveTextColor is ugly.
    readonly property string mentionColor: "red" // WIPQTQUICK TODO Hard-coded red because Breeze' negativeTextColor is ugly.

    QQC2.ScrollView {
        anchors.fill: parent

        ListView {
            id: viewListView

            QQC2.ScrollBar.vertical: QQC2.ScrollBar { id: verticalScrollbar }

            clip: true

            model: viewListModel
            cacheBuffer: Math.max(1000 * Kirigami.Units.devicePixelRatio, viewSwitcher.height)

            function showView(view) {
                viewModel.showView(view);

                if (!konvUi.pageStack.wideMode) {
                    if (konvUi.inputField) {
                        konvUi.inputField.forceActiveFocus();
                    }

                    konvUi.pageStack.currentIndex = 1;
                }
            }

            onContentYChanged: Qt.callLater(updateOffViewportOverlay);

            function updateOffViewportOverlay() {
                var topActivity = false;
                var topMention = false;
                var bottomActivity = false;
                var bottomMention = false;

                if (visibleArea.heightRatio == 1.0) {
                    return;
                }

                var delegates = new Array();

                // WIPQTQUICK HACK Skip over non-delegate children.
                for (var i = 0; i < contentItem.children.length; ++i) {
                    var item = contentItem.children[i];

                    if (item.row != undefined) {
                       delegates.push(item);
                    }
                }

                delegates.sort(function(a, b) {
                    return ((a.row < b.row) ? -1 : ((a.row > b.row) ? 1 : 0));
                });

                var mapped = mapToItem(contentItem, 0, 0);
                var firstIndex = Math.max(0, indexAt(0, mapped.y));

                if (firstIndex) {
                    for (var i = 0; i < firstIndex; ++i) {
                        var item = delegates[i];

                        if (item.unreadMentions) {
                            topMention = true;
                            break;
                        } else if (item.hasActivity) {
                            topActivity = true;
                        }
                    }
                }

                mapped = mapToItem(contentItem, 0, height - 1);
                var lastIndex = indexAt(0, mapped.y);

                if (lastIndex != -1 && lastIndex < (count - 1)) {
                    for (var i = lastIndex + 1; i < count; ++i) {
                        var item = delegates[i];

                        if (item.unreadMentions) {
                            bottomMention = true;
                            break;
                        } else if (item.hasActivity) {
                            bottomActivity = true;
                        }
                    }
                }

                if (topMention) {
                    topOverlay.mention = true;
                    topOverlay.visible = true;
                } else if (topActivity) {
                    topOverlay.mention = false;
                    topOverlay.visible = true;
                } else {
                    topOverlay.visible = false;
                }

                if (bottomMention) {
                    bottomOverlay.mention = true;
                    bottomOverlay.visible = true;
                } else if (bottomActivity) {
                    bottomOverlay.mention = false;
                    bottomOverlay.visible = true;
                } else {
                    bottomOverlay.visible = false;
                }
            }

            delegate: ListItem {
                id: viewListItem

                width: viewListView.width

                readonly property int row: index
                readonly property bool hasActivity: model.HasActivity
                readonly property int unreadMentions: model.ViewRole.unreadMentions
                property int connectionState: model.ViewRole ? model.ViewRole.server.connectionState : Konversation.NeverConnected
                property Item unreadMentionsCounter: null
                property Item loadingBar: null


                text: model.display
                textMarginLeft: model.IsChild ? Kirigami.Units.gridUnit * 2 : Kirigami.Units.gridUnit
                textMarginRight: unreadMentionsCounter ? (width - unreadMentionsCounter.x) + Kirigami.Units.smallSpacing : 0

                textColor: model.HasActivity ? viewSwitcher.activityColor : Kirigami.Theme.textColor

                onTextColorChanged: Qt.callLater(ListView.view.updateOffViewportOverlay)

                onUnreadMentionsChanged: {
                    if (!unreadMentions && unreadMentionsCounter) {
                        unreadMentionsCounter.destroy();
                        unreadMentionsCounter = null;
                    } else if (!unreadMentionsCounter) {
                        unreadMentionsCounter = unreadMentionsCounterComponent.createObject(viewListItem);
                    }

                    Qt.callLater(ListView.view.updateOffViewportOverlay);
                }

                onConnectionStateChanged: {
                    if (!loadingBar && connectionState == Konversation.Connecting) {
                        loadingBar = loadingBarComponent.createObject(viewListItem);
                    } else if (loadingBar) {
                        loadingBar.destroy();
                        loadingBar = null;
                    }
                }

                onClicked: {
                    viewListView.forceActiveFocus();

                    if (mouse.button == Qt.RightButton) {
                        viewModel.showViewContextMenu(viewModel.indexForView(model.ViewRole),
                            mapToGlobal(mouse.x, mouse.y));
                    } else {
                        viewListView.showView(value);
                    }
                }

                Component {
                    id: loadingBarComponent

                    QQC2.BusyIndicator {
                        id: loadingAnimator
                        running: true
                        anchors.right: parent.right
                        // we assume there won't be intersection with showing busy indicator and unread count
                        anchors.rightMargin: verticalScrollbar.visible ? verticalScrollbar.width : Kirigami.Units.smallSpacing
                        anchors.verticalCenter: parent.verticalCenter
                        height: Kirigami.Units.iconSizes.smallMedium
                        width: Kirigami.Units.iconSizes.smallMedium
                    }
                }

                Component {
                    id: unreadMentionsCounterComponent

                    Rectangle {
                        id: unreadMentionsCounter

                        anchors.right: parent.right
                        anchors.rightMargin: verticalScrollbar.visible ? verticalScrollbar.width : Kirigami.Units.smallSpacing
                        anchors.verticalCenter: parent.verticalCenter

                        height: Math.floor(unreadMentionsCounterText.paintedHeight)
                        width: Math.floor(unreadMentionsCounterText.paintedWidth) + Kirigami.Units.smallSpacing

                        color: viewSwitcher.mentionColor

                        radius: (Kirigami.Units.devicePixelRatio * 2)

                        Text {
                            id: unreadMentionsCounterText

                            anchors.centerIn: parent

                            renderType: Text.NativeRendering
                            textFormat: Text.PlainText
                            color: "white"

                            font.pixelSize: konvUi.largerFontSize
                            font.weight: Font.Bold
                            font.hintingPreference: Font.PreferFullHinting

                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter

                            text: viewListItem.unreadMentions
                        }
                    }
                }
            }

            Keys.onUpPressed: {
                event.accept = true;
                viewModel.showPreviousView();
            }

            Keys.onDownPressed: {
                event.accept = true;
                viewModel.showNextView();
            }

            Component.onCompleted: sidebar.viewListView = viewListView
        }
    }

    Rectangle {
        id: topOverlay

        visible: false

        anchors.top: parent.top

        width: parent.width
        height: Math.ceil(Kirigami.Units.devicePixelRatio)

        property bool mention: false

        color: mention ? viewSwitcher.mentionColor : viewSwitcher.activityColor
    }

    Rectangle {
        id: bottomOverlay

        visible: false

        anchors.bottom: parent.bottom

        width: parent.width
        height: Math.ceil(Kirigami.Units.devicePixelRatio)

        property bool mention: false

        color: mention ? viewSwitcher.mentionColor : viewSwitcher.activityColor
    }
}
