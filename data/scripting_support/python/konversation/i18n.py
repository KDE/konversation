# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
#
# SPDX-FileCopyrightText: 2011 Eike Hein


"""
This module provides i18n support for Konversation scripts written in Python,
via gettext.

Call the init() function to install the various functions that may be used to
mark up translatable strings in the builtins namespace, and optionally specify
a gettext .mo filename to use. See the function's documentation for more
details.

This module is considered EXPERIMENTAL at this time and not part of the public,
stable scripting interface.

"""

import gettext
import os
import subprocess
from . import dbus

try:
    # Python 2.x.
    import __builtin__ as builtins
except ImportError:
    # Python 3.x.
    import builtins


def init(domain='konversation'):

    """
    Initializes gettext and installs the following functions in the builtins
    namespace:

    i18n(message[, arg1, arg2, ...])
    i18np(singular, plural, num[, arg1, arg2, ...])
    i18nc(context, message[, arg1, arg2, ...])
    i18ncp(context, singular, plural, num[, arg1, arg2, ...])

    Third-party scripts may want to provide the 'domain' keyword argument to
    specify the name of their own gettext .mo file, which has to be installed
    in one of the directories that KDE considers for this type of resource.

    Keyword arguments:
    domain -- The gettext domain to use (default 'konversation').

    """

    dirs = locale_directories()

    for dir in dirs:
        try:
            t = gettext.translation(domain=domain,
                                    localedir=dir,
                                    languages=(current_language(),),
                                    fallback=False)
        except IOError:
            if dir is dirs[-1]:
                t = gettext.NullTranslations()

            continue
        else:
            break

    def i18n(msg, *args):
        return _insert_args(t.gettext(msg), args)

    def i18np(smsg, pmsg, num, *args):
        return _insert_args(t.ngettext(smsg, pmsg, num), args)

    # Context is part of the gettext msgid. Sadly Python's gettext API currently
    # doesn't have a convenient way to hand over both context and message, hence
    # the trick using string formatting here. Python bug 2504 indicates the API
    # may be extended to support context in Python 3.3.
    s = '{0}\x04{1}'

    def i18nc(ctxt, msg, *args):
        translated = t.gettext(s.format(ctxt, msg))
        
        if '\x04' in translated:
            translated = msg

        return _insert_args(translated, args)

    def i18ncp(ctxt, smsg, pmsg, num, *args):
        translated = t.ngettext(s.format(ctxt, smsg), s.format(ctxt, pmsg), num)

        if '\x04' in translated:
            # "If no message catalog is found msgid1 is returned if n == 1, otherwise msgid2."
            # (Source: GNU gettext v0.18 docs, 11.2.6 Additional functions for plural forms)
            translated = smsg if num == 1 else pmsg

        return _insert_args(translated, args)

    for func in (i18n, i18np, i18nc, i18ncp):
        builtins.__dict__[func.__name__] = func

def _insert_args(msg, args):
    for i in range(len(args)):
        msg = msg.replace('%' + str(i + 1), str(args[i]))

    return msg

def current_language():

    """
    Returns the language code for the language currently used by Konversation's
    user interface (e.g. en_US).

    """

    return os.environ['KONVERSATION_LANG']

def locale_directories():

    """
    Returns the list of directory paths searched for gettext .mo files, using
    KDE's normal lookup logic for this type of resource.

    """

    dirs = b''

    try:
        dirs = subprocess.check_output(('qtpaths6', '--locate-dirs', 'GenericDataLocation', 'locale'))
    except (OSError, subprocess.CalledProcessError):
        dbus.error("A problem occurred while looking for directories containing translation files. "
                   "The output of this script will not be translated.")

    return dirs.rstrip().decode().split(':')
