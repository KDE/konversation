# -*- coding: utf-8 -*-
#
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
#
# SPDX-FileCopyrightText: 2011 Eike Hein


"""
This module provides several functions and attributes for use in interacting
with Konversation's D-Bus API.

The 'connection' and 'target' attributes are fetched from the arguments given
to your script by Konversation when you import this module.

Modify the 'default_message_prefix' attribute if you want to prefix all messages
sent through the say/info/error functions with a common string rather than pass
a prefix with each call (but doing so overrides this fallback attribute).

This module is considered EXPERIMENTAL at this time and not part of the public,
stable scripting interface.

"""

import subprocess
import sys
import os

__all__ = ('info', 'error', 'say', 'dbus_command')

# Functions

def info(message, prefix=None):

    """Shows an info message in the active tab in Konversation."""

    _dispatch('info', _prefix(message, prefix))

def error(message, prefix=None, exit=False):

    """Shows an error message in the active tab in Konversation."""

    _dispatch('error', _prefix(message, prefix))

    if exit:
        sys.exit(1)

def say(message, prefix=None):

    """
    Instructs Konversation to send a message to the destination (a channel or
    nickname in a particular connection context) the importing script was given
    as arguments.

    """

    _dispatch('say', connection, target, _prefix(message, prefix))

def _prefix(message, prefix):

    """Prefix message, using the argument or falling back to the global."""

    if prefix is None:
        prefix = default_message_prefix

    return prefix + message

def _dispatch(*args):

    """Dispatch to Konversation's D-Bus API."""

    subprocess.call(_dbus_command + args)


# Attributes

try:
    connection = sys.argv[1]
    target = sys.argv[2]
except IndexError:
    connection = None
    target = None

default_message_prefix = ''

dbus_command = os.environ.get("KONVERSATION_DBUS_BIN", "qdbus")

_dbus_command = (dbus_command, 'org.kde.konversation', '/irc')
