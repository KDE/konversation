/****************************************************************************
** Preferences meta object code from reading C++ file 'preferences.h'
**
** Created: Tue Jun 25 05:30:22 2002
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "preferences.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 19)
#error "This file was generated using the moc from 3.0.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

const char *Preferences::className() const
{
    return "Preferences";
}

QMetaObject *Preferences::metaObj = 0;
static QMetaObjectCleanUp cleanUp_Preferences;

#ifndef QT_NO_TRANSLATION
QString Preferences::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "Preferences", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString Preferences::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "Preferences", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* Preferences::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = QObject::staticMetaObject();
    static const QUMethod slot_0 = {"openPrefsDialog", 0, 0 };
    static const QUParameter param_slot_1[] = {
	{ "number", &static_QUType_int, 0, QUParameter::In }
    };
    static const QUMethod slot_1 = {"connectToServer", 1, param_slot_1 };
    static const QUMethod slot_2 = {"saveOptions", 0, 0 };
    static const QUMethod slot_3 = {"closePrefsDialog", 0, 0 };
    static const QUMethod slot_4 = {"clearPrefsDialog", 0, 0 };
    static const QMetaData slot_tbl[] = {
	{ "openPrefsDialog()", &slot_0, QMetaData::Public },
	{ "connectToServer(int)", &slot_1, QMetaData::Protected },
	{ "saveOptions()", &slot_2, QMetaData::Protected },
	{ "closePrefsDialog()", &slot_3, QMetaData::Protected },
	{ "clearPrefsDialog()", &slot_4, QMetaData::Protected }
    };
    static const QUParameter param_signal_0[] = {
	{ "number", &static_QUType_int, 0, QUParameter::In }
    };
    static const QUMethod signal_0 = {"requestServerConnection", 1, param_signal_0 };
    static const QUMethod signal_1 = {"requestSaveOptions", 0, 0 };
    static const QMetaData signal_tbl[] = {
	{ "requestServerConnection(int)", &signal_0, QMetaData::Public },
	{ "requestSaveOptions()", &signal_1, QMetaData::Public }
    };
    metaObj = QMetaObject::new_metaobject(
	"Preferences", parentObject,
	slot_tbl, 5,
	signal_tbl, 2,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_Preferences.setMetaObject( metaObj );
    return metaObj;
}

void* Preferences::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "Preferences" ) ) return (Preferences*)this;
    return QObject::qt_cast( clname );
}

// SIGNAL requestServerConnection
void Preferences::requestServerConnection( int t0 )
{
    activate_signal( staticMetaObject()->signalOffset() + 0, t0 );
}

// SIGNAL requestSaveOptions
void Preferences::requestSaveOptions()
{
    activate_signal( staticMetaObject()->signalOffset() + 1 );
}

bool Preferences::qt_invoke( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->slotOffset() ) {
    case 0: openPrefsDialog(); break;
    case 1: connectToServer(static_QUType_int.get(_o+1)); break;
    case 2: saveOptions(); break;
    case 3: closePrefsDialog(); break;
    case 4: clearPrefsDialog(); break;
    default:
	return QObject::qt_invoke( _id, _o );
    }
    return TRUE;
}

bool Preferences::qt_emit( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->signalOffset() ) {
    case 0: requestServerConnection(static_QUType_int.get(_o+1)); break;
    case 1: requestSaveOptions(); break;
    default:
	return QObject::qt_emit(_id,_o);
    }
    return TRUE;
}
#ifndef QT_NO_PROPERTIES

bool Preferences::qt_property( int _id, int _f, QVariant* _v)
{
    return QObject::qt_property( _id, _f, _v);
}
#endif // QT_NO_PROPERTIES
