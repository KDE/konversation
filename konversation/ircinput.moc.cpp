/****************************************************************************
** IRCInput meta object code from reading C++ file 'ircinput.h'
**
** Created: Tue Jun 25 05:13:18 2002
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "ircinput.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 19)
#error "This file was generated using the moc from 3.0.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

const char *IRCInput::className() const
{
    return "IRCInput";
}

QMetaObject *IRCInput::metaObj = 0;
static QMetaObjectCleanUp cleanUp_IRCInput;

#ifndef QT_NO_TRANSLATION
QString IRCInput::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "IRCInput", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString IRCInput::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "IRCInput", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* IRCInput::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = QLineEdit::staticMetaObject();
    static const QUParameter param_slot_0[] = {
	{ "up", &static_QUType_bool, 0, QUParameter::In }
    };
    static const QUMethod slot_0 = {"getHistory", 1, param_slot_0 };
    static const QMetaData slot_tbl[] = {
	{ "getHistory(bool)", &slot_0, QMetaData::Protected }
    };
    static const QUMethod signal_0 = {"nickCompletion", 0, 0 };
    static const QUParameter param_signal_1[] = {
	{ "up", &static_QUType_bool, 0, QUParameter::In }
    };
    static const QUMethod signal_1 = {"history", 1, param_signal_1 };
    static const QUMethod signal_2 = {"pageUp", 0, 0 };
    static const QUMethod signal_3 = {"pageDown", 0, 0 };
    static const QMetaData signal_tbl[] = {
	{ "nickCompletion()", &signal_0, QMetaData::Public },
	{ "history(bool)", &signal_1, QMetaData::Public },
	{ "pageUp()", &signal_2, QMetaData::Public },
	{ "pageDown()", &signal_3, QMetaData::Public }
    };
    metaObj = QMetaObject::new_metaobject(
	"IRCInput", parentObject,
	slot_tbl, 1,
	signal_tbl, 4,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_IRCInput.setMetaObject( metaObj );
    return metaObj;
}

void* IRCInput::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "IRCInput" ) ) return (IRCInput*)this;
    return QLineEdit::qt_cast( clname );
}

// SIGNAL nickCompletion
void IRCInput::nickCompletion()
{
    activate_signal( staticMetaObject()->signalOffset() + 0 );
}

// SIGNAL history
void IRCInput::history( bool t0 )
{
    activate_signal_bool( staticMetaObject()->signalOffset() + 1, t0 );
}

// SIGNAL pageUp
void IRCInput::pageUp()
{
    activate_signal( staticMetaObject()->signalOffset() + 2 );
}

// SIGNAL pageDown
void IRCInput::pageDown()
{
    activate_signal( staticMetaObject()->signalOffset() + 3 );
}

bool IRCInput::qt_invoke( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->slotOffset() ) {
    case 0: getHistory(static_QUType_bool.get(_o+1)); break;
    default:
	return QLineEdit::qt_invoke( _id, _o );
    }
    return TRUE;
}

bool IRCInput::qt_emit( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->signalOffset() ) {
    case 0: nickCompletion(); break;
    case 1: history(static_QUType_bool.get(_o+1)); break;
    case 2: pageUp(); break;
    case 3: pageDown(); break;
    default:
	return QLineEdit::qt_emit(_id,_o);
    }
    return TRUE;
}
#ifndef QT_NO_PROPERTIES

bool IRCInput::qt_property( int _id, int _f, QVariant* _v)
{
    return QLineEdit::qt_property( _id, _f, _v);
}
#endif // QT_NO_PROPERTIES
