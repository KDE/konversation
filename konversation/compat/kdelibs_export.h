/*
   This file is part of the KDE libraries
   Copyright (C) 2004 Jaroslaw Staniek <js@iidea.pl>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef _KDELIBS_EXPORT_H
#define _KDELIBS_EXPORT_H

/* needed for KDE_EXPORT macros */
#include <kdemacros.h>

/* needed, because e.g. Q_OS_UNIX is so frequently used */
#include <qglobal.h>

#ifdef Q_WS_WIN
#include <kdelibs_export_win.h>

#else /* Q_OS_UNIX */

/* export statements for unix */
#define KDECORE_EXPORT KDE_EXPORT
#define KDEUI_EXPORT KDE_EXPORT
#define KDEFX_EXPORT KDE_EXPORT
#define KDEPRINT_EXPORT KDE_EXPORT
#define KIO_EXPORT KDE_EXPORT
#define DCOP_EXPORT KDE_EXPORT
#define KPARTS_EXPORT KDE_EXPORT
#define KTEXTEDITOR_EXPORT KDE_EXPORT
#define KABC_EXPORT KDE_EXPORT
#define KDESU_EXPORT KDE_EXPORT
#define KVCARD_EXPORT KDE_EXPORT
#define KRESOURCES_EXPORT KDE_EXPORT
#define KSTYLE_EXPORT KDE_EXPORT
#define KHTML_EXPORT KDE_EXPORT
#define KMDI_EXPORT KDE_EXPORT
#define KUTILS_EXPORT KDE_EXPORT
#define KATEPARTINTERFACES_EXPORT KDE_EXPORT
#define KATEPART_EXPORT KDE_EXPORT
#define KMID_EXPORT KDE_EXPORT
#define KIMPROXY_EXPORT KDE_EXPORT
#define ARTS_EXPORT KDE_EXPORT

#define KPATH_SEPARATOR ':'

#ifndef O_BINARY
#define O_BINARY 0 /* for open() */
#endif

#endif

#endif /*_KDELIBS_H*/

/* workaround for kdecore: stupid moc's grammar doesn't accept two macros 
   between 'class' keyword and <classname>: */
#ifdef KDE_DEPRECATED
# ifndef KDECORE_EXPORT_DEPRECATED
#  define KDECORE_EXPORT_DEPRECATED KDE_DEPRECATED KDECORE_EXPORT
# endif
# ifndef KIO_EXPORT_DEPRECATED
#  define KIO_EXPORT_DEPRECATED KDE_DEPRECATED KIO_EXPORT
# endif
# ifndef KDEUI_EXPORT_DEPRECATED
#  define KDEUI_EXPORT_DEPRECATED KDE_DEPRECATED KDEUI_EXPORT
# endif
# ifndef KABC_EXPORT_DEPRECATED
#  define KABC_EXPORT_DEPRECATED KDE_DEPRECATED KABC_EXPORT
# endif
#endif
/* (let's add KDE****_EXPORT_DEPRECATED for other libraries if it's needed) */
