/****************************************************************************
** Server meta object code from reading C++ file 'server.h'
**
** Created: Tue Jun 25 05:13:13 2002
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "server.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 19)
#error "This file was generated using the moc from 3.0.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

const char *Server::className() const
{
    return "Server";
}

QMetaObject *Server::metaObj = 0;
static QMetaObjectCleanUp cleanUp_Server;

#ifndef QT_NO_TRANSLATION
QString Server::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "Server", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString Server::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "Server", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* Server::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = QObject::staticMetaObject();
    static const QUParameter param_slot_0[] = {
	{ "buffer", &static_QUType_QString, 0, QUParameter::In }
    };
    static const QUMethod slot_0 = {"queue", 1, param_slot_0 };
    static const QUParameter param_slot_1[] = {
	{ "newNickname", &static_QUType_QString, 0, QUParameter::In }
    };
    static const QUMethod slot_1 = {"setNickname", 1, param_slot_1 };
    static const QUParameter param_slot_2[] = {
	{ "nickname", &static_QUType_QString, 0, QUParameter::In },
	{ "hostmask", &static_QUType_QString, 0, QUParameter::In }
    };
    static const QUMethod slot_2 = {"addQuery", 2, param_slot_2 };
    static const QUParameter param_slot_3[] = {
	{ "query", &static_QUType_ptr, "Query", QUParameter::In }
    };
    static const QUMethod slot_3 = {"removeQuery", 1, param_slot_3 };
    static const QUParameter param_slot_4[] = {
	{ 0, &static_QUType_ptr, "KSocket", QUParameter::In }
    };
    static const QUMethod slot_4 = {"incoming", 1, param_slot_4 };
    static const QUMethod slot_5 = {"processIncomingData", 0, 0 };
    static const QUParameter param_slot_6[] = {
	{ 0, &static_QUType_ptr, "KSocket", QUParameter::In }
    };
    static const QUMethod slot_6 = {"send", 1, param_slot_6 };
    static const QUParameter param_slot_7[] = {
	{ 0, &static_QUType_ptr, "KSocket", QUParameter::In }
    };
    static const QUMethod slot_7 = {"broken", 1, param_slot_7 };
    static const QMetaData slot_tbl[] = {
	{ "queue(const QString&)", &slot_0, QMetaData::Public },
	{ "setNickname(const QString&)", &slot_1, QMetaData::Public },
	{ "addQuery(const QString&,const QString&)", &slot_2, QMetaData::Public },
	{ "removeQuery(Query*)", &slot_3, QMetaData::Public },
	{ "incoming(KSocket*)", &slot_4, QMetaData::Protected },
	{ "processIncomingData()", &slot_5, QMetaData::Protected },
	{ "send(KSocket*)", &slot_6, QMetaData::Protected },
	{ "broken(KSocket*)", &slot_7, QMetaData::Protected }
    };
    static const QUParameter param_signal_0[] = {
	{ 0, &static_QUType_QString, 0, QUParameter::In }
    };
    static const QUMethod signal_0 = {"nicknameChanged", 1, param_signal_0 };
    static const QMetaData signal_tbl[] = {
	{ "nicknameChanged(const QString&)", &signal_0, QMetaData::Public }
    };
    metaObj = QMetaObject::new_metaobject(
	"Server", parentObject,
	slot_tbl, 8,
	signal_tbl, 1,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_Server.setMetaObject( metaObj );
    return metaObj;
}

void* Server::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "Server" ) ) return (Server*)this;
    return QObject::qt_cast( clname );
}

// SIGNAL nicknameChanged
void Server::nicknameChanged( const QString& t0 )
{
    activate_signal( staticMetaObject()->signalOffset() + 0, t0 );
}

bool Server::qt_invoke( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->slotOffset() ) {
    case 0: queue(static_QUType_QString.get(_o+1)); break;
    case 1: setNickname(static_QUType_QString.get(_o+1)); break;
    case 2: addQuery(static_QUType_QString.get(_o+1),static_QUType_QString.get(_o+2)); break;
    case 3: removeQuery((Query*)static_QUType_ptr.get(_o+1)); break;
    case 4: incoming((KSocket*)static_QUType_ptr.get(_o+1)); break;
    case 5: processIncomingData(); break;
    case 6: send((KSocket*)static_QUType_ptr.get(_o+1)); break;
    case 7: broken((KSocket*)static_QUType_ptr.get(_o+1)); break;
    default:
	return QObject::qt_invoke( _id, _o );
    }
    return TRUE;
}

bool Server::qt_emit( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->signalOffset() ) {
    case 0: nicknameChanged(static_QUType_QString.get(_o+1)); break;
    default:
	return QObject::qt_emit(_id,_o);
    }
    return TRUE;
}
#ifndef QT_NO_PROPERTIES

bool Server::qt_property( int _id, int _f, QVariant* _v)
{
    return QObject::qt_property( _id, _f, _v);
}
#endif // QT_NO_PROPERTIES
