/****************************************************************************
** ChatWindow meta object code from reading C++ file 'chatwindow.h'
**
** Created: Tue Jun 25 05:13:29 2002
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "chatwindow.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 19)
#error "This file was generated using the moc from 3.0.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

const char *ChatWindow::className() const
{
    return "ChatWindow";
}

QMetaObject *ChatWindow::metaObj = 0;
static QMetaObjectCleanUp cleanUp_ChatWindow;

#ifndef QT_NO_TRANSLATION
QString ChatWindow::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "ChatWindow", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString ChatWindow::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "ChatWindow", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* ChatWindow::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = QObject::staticMetaObject();
    static const QUParameter param_slot_0[] = {
	{ "text", &static_QUType_QString, 0, QUParameter::In }
    };
    static const QUMethod slot_0 = {"logText", 1, param_slot_0 };
    static const QMetaData slot_tbl[] = {
	{ "logText(const QString&)", &slot_0, QMetaData::Public }
    };
    metaObj = QMetaObject::new_metaobject(
	"ChatWindow", parentObject,
	slot_tbl, 1,
	0, 0,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_ChatWindow.setMetaObject( metaObj );
    return metaObj;
}

void* ChatWindow::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "ChatWindow" ) ) return (ChatWindow*)this;
    return QObject::qt_cast( clname );
}

bool ChatWindow::qt_invoke( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->slotOffset() ) {
    case 0: logText(static_QUType_QString.get(_o+1)); break;
    default:
	return QObject::qt_invoke( _id, _o );
    }
    return TRUE;
}

bool ChatWindow::qt_emit( int _id, QUObject* _o )
{
    return QObject::qt_emit(_id,_o);
}
#ifndef QT_NO_PROPERTIES

bool ChatWindow::qt_property( int _id, int _f, QVariant* _v)
{
    return QObject::qt_property( _id, _f, _v);
}
#endif // QT_NO_PROPERTIES
