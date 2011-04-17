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
This module provides i18n support for Konversation scripts written in Python,
via gettext.

Call init() to install the standard _() function in the builtins namespace and
use it to mark your translatable strings. Optionally provide the 'domain'
keyword argument to specify a gettext .mo file installed in one of the
directories KDE considers for this type of resource.

This module is considered EXPERIMENTAL at this time and not part of the public,
stable scripting inteface.

"""

import gettext
import os
import subprocess


def init(domain='konversation'):

    """
    Initializes gettext and installs the _() function in the builtins namespace.

    Third-party scripts may want to provide the 'domain' keyword argument to
    specify the name of their own gettext .mo file, installed in one of the
    directories that KDE considers for this type of resource.

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

    t.install()

def current_language():

    """
    Returns the language code for the language currently used by Konversation's
    user interface (e.g. en_US).

    """

    return os.environ['KONVERSATION_LANG']


def locale_directories():

    """
    Returns the list of directory paths searched for gettext .mo files, using
    KDE's normal lookup logic for this type of resource."

    """

    dirs = subprocess.Popen('kde4-config --path locale',
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE,
                            shell=True).communicate()

    return dirs[0].rstrip().decode().split(':')
