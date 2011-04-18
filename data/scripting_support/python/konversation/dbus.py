#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License or (at your option) version 3 or any later version
# accepted by the membership of KDE e.V. (or its successor appro-
# ved by the membership of KDE e.V.), which shall act as a proxy
# defined in Section 14 of version 3 of the license.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see http://www.gnu.org/licenses/.
#
# Copyright (C) 2011 Eike Hein


"""
This module provides several functions and attributes for use in interacting
with Konversation's D-Bus API.

The 'connection' and 'target' attributes are fetched from the arguments given
to your script by Konversation when you import this module.

Modify the 'defaultMessagePrefix' attribute if you want to prefix all messages
sent through the say/info/error functions with a common string.

This module is considered EXPERIMENTAL at this time and not part of the public,
stable scripting inteface.

"""

import subprocess
import sys


__all__ = ('abort_if_standalone', 'info', 'error', 'say')


# Functions

def info(message):

    """Shows an info message in the active tab in Konversation."""

    _dispatch('info', defaultMessagePrefix + message)

def error(message):

    """Shows an error message in the active tab in Konversation."""

    _dispatch('error', defaultMessagePrefix + message)

def say(message):

    """
    Instructs Konversation to send a message to the destination (a channel or
    nickname in a particular connection context) the importing script was given
    as arguments.

    """

    _dispatch('say', connection, target, defaultMessagePrefix + message)

def _dispatch(*args):

    """Dispatch to Konversation's D-Bus API."""

    subprocess.Popen(_dbus_command + args).communicate()


# Attributes

try:
    connection = sys.argv[1]
    target = sys.argv[2]
except IndexError:
    connection = None
    target = None

defaultMessagePrefix = ''

_dbus_command = ('qdbus', 'org.kde.konversation', '/irc')
