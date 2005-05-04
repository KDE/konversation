/* This file is part of the KDE libraries
    Copyright (c) 2002-2003 KDE Team

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#ifndef _KDE_MACROS_H_
#define _KDE_MACROS_H_

#define KDE_EXPORT
#define KDE_NO_EXPORT


#ifdef __GNUC__
#define KDE_PACKED __attribute__((__packed__))
#else
#define KDE_PACKED
#endif

/**
 * The KDE_DEPRECATED macro can be used to trigger compile-time warnings
 * with gcc >= 3.2 when deprecated functions are used.
 *
 * For non-inline functions, the macro gets inserted at the very end of the
 * function declaration, right before the semicolon:
 *
 * \code
 * DeprecatedConstructor() KDE_DEPRECATED;
 * void deprecatedFunctionA() KDE_DEPRECATED;
 * int deprecatedFunctionB() const KDE_DEPRECATED;
 * \endcode
 *
 * Functions which are implemented inline are handled differently: for them,
 * the KDE_DEPRECATED macro is inserted at the front, right before the return
 * type, but after "static" or "virtual":
 *
 * \code
 * KDE_DEPRECATED void deprecatedInlineFunctionA() { .. }
 * virtual KDE_DEPRECATED int deprecatedInlineFunctionB() { .. }
 * static KDE_DEPRECATED bool deprecatedInlineFunctionC() { .. }
 * \end
 *
 * You can also mark whole structs or classes as deprecated, by inserting the
 * KDE_DEPRECATED macro after the struct/class keyword, but before the
 * name of the struct/class:
 *
 * \code
 * class KDE_DEPRECATED DeprecatedClass { };
 * struct KDE_DEPRECATED DeprecatedStruct { };
 * \endcode
 *
 * \note
 * It does not make much sense to use the KDE_DEPRECATED keyword for a Qt signal;
 * this is because usually get called by the class which they belong to,
 * and one'd assume that a class author doesn't use deprecated methods of his
 * own class. The only exception to this are signals which are connected to
 * other signals; they get invoked from moc-generated code. In any case, 
 * printing a warning message in either case is not useful.
 * For slots, it can make sense (since slots can be invoked directly) but be
 * aware that if the slots get triggered by a signal, the will get called from
 * moc code as well and thus the warnings are useless.
 *
 * \par
 * Also note that it is not possible to use KDE_DEPRECATED for classes which
 * use the k_dcop keyword (to indicate a DCOP interface declaration); this is
 * because the dcopidl program would choke on the unexpected declaration
 * syntax.
 */

#ifndef KDE_DEPRECATED
#if __GNUC__ - 0 > 3 || (__GNUC__ - 0 == 3 && __GNUC_MINOR__ - 0 >= 2)
# define KDE_DEPRECATED __attribute__ ((deprecated))
#else
# define KDE_DEPRECATED
#endif
#endif

/**
 * The KDE_ISLIKELY macro tags a boolean expression as likely to evaluate to
 * 'true'. When used in an if ( ) statement, it gives a hint to the compiler
 * that the following codeblock is likely to get executed. Providing this
 * information helps the compiler to optimize the code for better performance.
 * Using the macro has an insignificant code size or runtime memory footprint impact.
 * The code semantics is not affected.
 *
 * \note
 * Providing wrong information ( like marking a condition that almost never
 * passes as 'likely' ) will cause a significant runtime slowdown. Therefore only
 * use it for cases where you can be sure about the odds of the expression to pass
 * in all cases ( independent from e.g. user configuration ).
 *
 * \par
 * The KDE_ISUNLIKELY macro tags an expression as unlikely evaluating to 'true'. 
 *
 * \note
 * Do NOT use ( !KDE_ISLIKELY(foo) ) as an replacement for KDE_ISUNLIKELY !
 *
 * \code
 * if ( KDE_ISUNLIKELY( testsomething() ) )
 *     abort();     // assume its unlikely that the application aborts
 * \endcode
 */
#if __GNUC__ - 0 >= 3
# define KDE_ISLIKELY( x )    __builtin_expect(!!(x),1)
# define KDE_ISUNLIKELY( x )  __builtin_expect(!!(x),0)
#else
# define KDE_ISLIKELY( x )   ( x )
# define KDE_ISUNLIKELY( x )  ( x )
#endif

#endif // _KDE_MACROS_H_
