/****************************************************************************
** Channel meta object code from reading C++ file 'channel.h'
**
** Created: Tue Jun 25 05:13:51 2002
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "channel.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 19)
#error "This file was generated using the moc from 3.0.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

const char *Channel::className() const
{
    return "Channel";
}

QMetaObject *Channel::metaObj = 0;
static QMetaObjectCleanUp cleanUp_Channel;

#ifndef QT_NO_TRANSLATION
QString Channel::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "Channel", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString Channel::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "Channel", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* Channel::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = ChatWindow::staticMetaObject();
    static const QUParameter param_slot_0[] = {
	{ 0, &static_QUType_QString, 0, QUParameter::In }
    };
    static const QUMethod slot_0 = {"setNickname", 1, param_slot_0 };
    static const QUMethod slot_1 = {"channelTextEntered", 0, 0 };
    static const QUParameter param_slot_2[] = {
	{ "line", &static_QUType_QString, 0, QUParameter::In }
    };
    static const QUMethod slot_2 = {"sendChannelText", 1, param_slot_2 };
    static const QUMethod slot_3 = {"newTextInView", 0, 0 };
    static const QUParameter param_slot_4[] = {
	{ "url", &static_QUType_QString, 0, QUParameter::In }
    };
    static const QUMethod slot_4 = {"urlCatcher", 1, param_slot_4 };
    static const QUMethod slot_5 = {"completeNick", 0, 0 };
    static const QUParameter param_slot_6[] = {
	{ "id", &static_QUType_int, 0, QUParameter::In }
    };
    static const QUMethod slot_6 = {"quickButtonClicked", 1, param_slot_6 };
    static const QUParameter param_slot_7[] = {
	{ "id", &static_QUType_int, 0, QUParameter::In }
    };
    static const QUMethod slot_7 = {"popupCommand", 1, param_slot_7 };
    static const QMetaData slot_tbl[] = {
	{ "setNickname(const QString&)", &slot_0, QMetaData::Public },
	{ "channelTextEntered()", &slot_1, QMetaData::Public },
	{ "sendChannelText(const QString&)", &slot_2, QMetaData::Public },
	{ "newTextInView()", &slot_3, QMetaData::Public },
	{ "urlCatcher(QString)", &slot_4, QMetaData::Public },
	{ "completeNick()", &slot_5, QMetaData::Protected },
	{ "quickButtonClicked(int)", &slot_6, QMetaData::Protected },
	{ "popupCommand(int)", &slot_7, QMetaData::Protected }
    };
    static const QUParameter param_signal_0[] = {
	{ "channel", &static_QUType_ptr, "QWidget", QUParameter::In }
    };
    static const QUMethod signal_0 = {"newText", 1, param_signal_0 };
    static const QMetaData signal_tbl[] = {
	{ "newText(QWidget*)", &signal_0, QMetaData::Public }
    };
    metaObj = QMetaObject::new_metaobject(
	"Channel", parentObject,
	slot_tbl, 8,
	signal_tbl, 1,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_Channel.setMetaObject( metaObj );
    return metaObj;
}

void* Channel::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "Channel" ) ) return (Channel*)this;
    return ChatWindow::qt_cast( clname );
}

#include <qobjectdefs.h>
#include <qsignalslotimp.h>

// SIGNAL newText
void Channel::newText( QWidget* t0 )
{
    if ( signalsBlocked() )
	return;
    QConnectionList *clist = receivers( staticMetaObject()->signalOffset() + 0 );
    if ( !clist )
	return;
    QUObject o[2];
    static_QUType_ptr.set(o+1,t0);
    activate_signal( clist, o );
}

bool Channel::qt_invoke( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->slotOffset() ) {
    case 0: setNickname(static_QUType_QString.get(_o+1)); break;
    case 1: channelTextEntered(); break;
    case 2: sendChannelText(static_QUType_QString.get(_o+1)); break;
    case 3: newTextInView(); break;
    case 4: urlCatcher(static_QUType_QString.get(_o+1)); break;
    case 5: completeNick(); break;
    case 6: quickButtonClicked(static_QUType_int.get(_o+1)); break;
    case 7: popupCommand(static_QUType_int.get(_o+1)); break;
    default:
	return ChatWindow::qt_invoke( _id, _o );
    }
    return TRUE;
}

bool Channel::qt_emit( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->signalOffset() ) {
    case 0: newText((QWidget*)static_QUType_ptr.get(_o+1)); break;
    default:
	return ChatWindow::qt_emit(_id,_o);
    }
    return TRUE;
}
#ifndef QT_NO_PROPERTIES

bool Channel::qt_property( int _id, int _f, QVariant* _v)
{
    return ChatWindow::qt_property( _id, _f, _v);
}
#endif // QT_NO_PROPERTIES
