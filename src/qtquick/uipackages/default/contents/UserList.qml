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

import org.kde.konversation 1.0 as Konversation
import org.kde.konversation.uicomponents 1.0 as KUIC

QQC2.ScrollView {
    id: userList

    property alias currentIndex: userListView.currentIndex
    property alias model: userListView.model

    ListView {
        id: userListView

        onHeightChanged: {
            if (currentIndex != -1) {
                positionViewAtIndex(currentIndex, ListView.Contain);
            }
        }

        property int anchorIndex: 0

        focus: true
        clip: true

        currentIndex: -1
        onCurrentIndexChanged: positionViewAtIndex(currentIndex, ListView.Contain)

        onModelChanged: currentIndex = -1

        delegate: ListItem {
            width: userListView.width

            isActive: (model.Selected === true)

            text: model.display
            textMarginLeft: Kirigami.Units.gridUnit

            function openQuery() {
                viewModel.currentServer.addQuery(model.display);
                contextDrawer.close();
            }

            onClicked: {
                userListView.forceActiveFocus();

                if (mouse.button == Qt.RightButton) {
                    if (!model.Selected) {
                        userListView.currentIndex = index;
                        userModel.clearAndSelect([index]);
                    }

                    contextMenus.nickMenu(mapToGlobal(mouse.x, mouse.y),
                        Konversation.IrcContextMenus.ShowChannelActions,
                        viewModel.currentServer,
                        userModel.selectedNames,
                        viewModel.currentView.name);

                    return;
                } else {
                    if (mouse.modifiers & Qt.ControlModifier) {
                        userModel.toggleSelected(index);

                        return;
                    } else if ((mouse.modifiers & Qt.ShiftModifier) && userModel.hasSelection) {
                        var selection = [];
                        var start = Math.min(userListView.currentIndex, index);
                        var end = Math.max(userListView.currentIndex, index);

                        for (var i = start; i <= end; ++i) {
                            selection.push(i);
                        }

                        userModel.clearAndSelect(selection);

                        return;
                    }

                    userListView.currentIndex = index;
                    userModel.clearAndSelect([index]);
                }
            }

            onDoubleClicked: openQuery();

            Keys.onEnterPressed: {
                event.accept = true;
                openQuery();
            }

            Keys.onReturnPressed: {
                event.accept = true;
                openQuery();
            }
        }

        Keys.onUpPressed: {
            event.accept = true;

            if (currentIndex == -1) {
                currentIndex = 0;
                return;
            }

            decrementCurrentIndex();

            if (event.modifiers & Qt.ShiftModifier) {
                userModel.setRangeSelected(anchorIndex, currentIndex);
            } else {
                userModel.clearAndSelect([currentIndex]);
            }
        }

        Keys.onDownPressed: {
            event.accept = true;

            if (currentIndex == -1) {
                currentIndex = 0;
                return;
            }

            incrementCurrentIndex();

            if (event.modifiers & Qt.ShiftModifier) {
                userModel.setRangeSelected(anchorIndex, currentIndex)
            } else {
                userModel.clearAndSelect([currentIndex]);
            }
        }

        Keys.onPressed: {
            if (event.matches(StandardKey.SelectAll)) {
                event.accepted = true;
                userModel.selectAll();
            } else if (event.key == Qt.Key_Shift) {
                if (currentIndex != -1) {
                    anchorIndex = currentIndex;
                }
            }
        }

        Keys.onReleased: {
            if (event.key == Qt.Key_Shift) {
                if (currentIndex != -1) {
                    anchorIndex = 0;
                }
            }
        }
    }
}
