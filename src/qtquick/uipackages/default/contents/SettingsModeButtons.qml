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

Row {
    id: settingsModeButtons

    property Item okButton: null
    property Item applyButton: null
    property Item cancelButton: null

    spacing: Kirigami.Units.smallSpacing
    rightPadding: Kirigami.Units.largeSpacing / 2

    layoutDirection: Qt.RightToLeft // WIPQTQUICK TODO Test RTL mode (layouts, animation directions)

    QQC2.Button {
        id: okButton

        anchors.verticalCenter: parent.verticalCenter

        enabled: parent.enabled

        text: "Ok" // WIPQTQUICK TODO i18n

        Component.onOnCompleted: settingsButtons.okButton = okButton
    }

    QQC2.Button {
        id: applyButton

        anchors.verticalCenter: parent.verticalCenter

        enabled: parent.enabled

        text: "Apply" // WIPQTQUICK TODO i18n

        Component.onOnCompleted: settingsButtons.applyButton = applyButton
    }

    QQC2.Button {
        id: cancelButton

        anchors.verticalCenter: parent.verticalCenter

        enabled: parent.enabled

        text: "Cancel" // WIPQTQUICK TODO i18n

        Component.onOnCompleted: settingsButtons.cancelButton = cancelButton
    }

    Component.onCompleted: konvApp.settingsModeButtons = settingsModeButtons
}
