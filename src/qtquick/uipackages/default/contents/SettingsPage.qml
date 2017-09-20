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

Item {
    id: settingsPage

    property alias title: pageTitle.text

    Column {
        anchors.fill: parent
        anchors.leftMargin: Kirigami.Units.gridUnit

        spacing: Kirigami.Units.gridUnit

        Kirigami.Heading {
            id: pageTitle

            level: 1
        }

        QQC2.Button {
            text: "Show Legacy Menu Bar"

            checkable: true

            onCheckedChanged: konvUi.showMenuBar(checked)
        }

        QQC2.Button {
            text: "Open Legacy Config Dialog"
            onClicked: konvUi.openLegacyConfigDialog()
        }

        QQC2.Label {
            text: "Note: You can press F10 at any time to switch between the old and new UI!"
        }

        QQC2.CheckBox {
            text: "Don't show popup for user name completion (Tab completion)"

            checked: konvUi.settings.hideCompletionPopup

            onCheckedChanged: {
                konvUi.settings.hideCompletionPopup = checked;
            }
        }
    }
}
