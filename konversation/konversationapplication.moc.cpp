/****************************************************************************
** KonversationApplication meta object code from reading C++ file 'konversationapplication.h'
**
** Created: Tue Jun 25 05:13:06 2002
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "konversationapplication.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 19)
#error "This file was generated using the moc from 3.0.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

const char *KonversationApplication::className() const
{
    return "KonversationApplication";
}

QMetaObject *KonversationApplication::metaObj = 0;
static QMetaObjectCleanUp cleanUp_KonversationApplication;

#ifndef QT_NO_TRANSLATION
QString KonversationApplication::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "KonversationApplication", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString KonversationApplication::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "KonversationApplication", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* KonversationApplication::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = KApplication::staticMetaObject();
    static const QUParameter param_slot_0[] = {
	{ "number", &static_QUType_int, 0, QUParameter::In }
    };
    static const QUMethod slot_0 = {"connectToServer", 1, param_slot_0 };
    static const QUParameter param_slot_1[] = {
	{ "number", &static_QUType_int, 0, QUParameter::In }
    };
    static const QUMethod slot_1 = {"connectToAnotherServer", 1, param_slot_1 };
    static const QUMethod slot_2 = {"readOptions", 0, 0 };
    static const QUMethod slot_3 = {"saveOptions", 0, 0 };
    static const QUMethod slot_4 = {"quitKonversation", 0, 0 };
    static const QMetaData slot_tbl[] = {
	{ "connectToServer(int)", &slot_0, QMetaData::Public },
	{ "connectToAnotherServer(int)", &slot_1, QMetaData::Public },
	{ "readOptions()", &slot_2, QMetaData::Public },
	{ "saveOptions()", &slot_3, QMetaData::Public },
	{ "quitKonversation()", &slot_4, QMetaData::Public }
    };
    metaObj = QMetaObject::new_metaobject(
	"KonversationApplication", parentObject,
	slot_tbl, 5,
	0, 0,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_KonversationApplication.setMetaObject( metaObj );
    return metaObj;
}

void* KonversationApplication::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "KonversationApplication" ) ) return (KonversationApplication*)this;
    return KApplication::qt_cast( clname );
}

bool KonversationApplication::qt_invoke( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->slotOffset() ) {
    case 0: connectToServer(static_QUType_int.get(_o+1)); break;
    case 1: connectToAnotherServer(static_QUType_int.get(_o+1)); break;
    case 2: readOptions(); break;
    case 3: saveOptions(); break;
    case 4: quitKonversation(); break;
    default:
	return KApplication::qt_invoke( _id, _o );
    }
    return TRUE;
}

bool KonversationApplication::qt_emit( int _id, QUObject* _o )
{
    return KApplication::qt_emit(_id,_o);
}
#ifndef QT_NO_PROPERTIES

bool KonversationApplication::qt_property( int _id, int _f, QVariant* _v)
{
    return KApplication::qt_property( _id, _f, _v);
}
#endif // QT_NO_PROPERTIES
