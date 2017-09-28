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

QtObject {
    property bool constrictInputField: false // WIPQTQUICK HACK Redundant with KConfigXT (but inverted)

    property bool sharedInputField: false

    property string completionSuffix: " " // WIPQTQUICK HACK Redundant with KConfigXT (but merged from two)
    property bool hideCompletionPopup: false

}
