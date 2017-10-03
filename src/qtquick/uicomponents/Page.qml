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

Kirigami.Page {
    Keys.onPressed: {
        if (!konvUi.inputField) {
            return;
        }

        // WIPQTQUICK TODO Evaluating text is not good enough, needs real key event fwd
        // to make things like deadkeys work.
        if (event.text != "" && !konvUi.inputField.activeFocus) {
            event.accept = true;
            konvUi.inputField.textForward(event.text);
        } else if (event.matches(StandardKey.Paste)) {
            event.accepted = true;
            konvUi.inputField.pasteFromClipboard();
        }
    }
}
