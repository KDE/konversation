/****************************************************************************
** IRCView meta object code from reading C++ file 'ircview.h'
**
** Created: Tue Jun 25 05:13:11 2002
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "ircview.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 19)
#error "This file was generated using the moc from 3.0.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

const char *IRCView::className() const
{
    return "IRCView";
}

QMetaObject *IRCView::metaObj = 0;
static QMetaObjectCleanUp cleanUp_IRCView;

#ifndef QT_NO_TRANSLATION
QString IRCView::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "IRCView", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString IRCView::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "IRCView", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* IRCView::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = KTextBrowser::staticMetaObject();
    static const QUParameter param_slot_0[] = {
	{ "nick", &static_QUType_charstar, 0, QUParameter::In },
	{ "message", &static_QUType_charstar, 0, QUParameter::In }
    };
    static const QUMethod slot_0 = {"append", 2, param_slot_0 };
    static const QUParameter param_slot_1[] = {
	{ "nick", &static_QUType_charstar, 0, QUParameter::In },
	{ "message", &static_QUType_charstar, 0, QUParameter::In }
    };
    static const QUMethod slot_1 = {"appendQuery", 2, param_slot_1 };
    static const QUParameter param_slot_2[] = {
	{ "nick", &static_QUType_charstar, 0, QUParameter::In },
	{ "message", &static_QUType_charstar, 0, QUParameter::In }
    };
    static const QUMethod slot_2 = {"appendAction", 2, param_slot_2 };
    static const QUParameter param_slot_3[] = {
	{ "type", &static_QUType_charstar, 0, QUParameter::In },
	{ "message", &static_QUType_charstar, 0, QUParameter::In }
    };
    static const QUMethod slot_3 = {"appendServerMessage", 2, param_slot_3 };
    static const QUParameter param_slot_4[] = {
	{ "command", &static_QUType_charstar, 0, QUParameter::In },
	{ "message", &static_QUType_charstar, 0, QUParameter::In }
    };
    static const QUMethod slot_4 = {"appendCommandMessage", 2, param_slot_4 };
    static const QUParameter param_slot_5[] = {
	{ "firstColumn", &static_QUType_charstar, 0, QUParameter::In },
	{ "message", &static_QUType_charstar, 0, QUParameter::In }
    };
    static const QUMethod slot_5 = {"appendBacklogMessage", 2, param_slot_5 };
    static const QMetaData slot_tbl[] = {
	{ "append(const char*,const char*)", &slot_0, QMetaData::Public },
	{ "appendQuery(const char*,const char*)", &slot_1, QMetaData::Public },
	{ "appendAction(const char*,const char*)", &slot_2, QMetaData::Public },
	{ "appendServerMessage(const char*,const char*)", &slot_3, QMetaData::Public },
	{ "appendCommandMessage(const char*,const char*)", &slot_4, QMetaData::Public },
	{ "appendBacklogMessage(const char*,const char*)", &slot_5, QMetaData::Public }
    };
    static const QUMethod signal_0 = {"newText", 0, 0 };
    static const QUParameter param_signal_1[] = {
	{ "url", &static_QUType_QString, 0, QUParameter::In }
    };
    static const QUMethod signal_1 = {"newURL", 1, param_signal_1 };
    static const QUMethod signal_2 = {"gotFocus", 0, 0 };
    static const QUParameter param_signal_3[] = {
	{ "text", &static_QUType_QString, 0, QUParameter::In }
    };
    static const QUMethod signal_3 = {"textToLog", 1, param_signal_3 };
    static const QMetaData signal_tbl[] = {
	{ "newText()", &signal_0, QMetaData::Public },
	{ "newURL(QString)", &signal_1, QMetaData::Public },
	{ "gotFocus()", &signal_2, QMetaData::Public },
	{ "textToLog(const QString&)", &signal_3, QMetaData::Public }
    };
    metaObj = QMetaObject::new_metaobject(
	"IRCView", parentObject,
	slot_tbl, 6,
	signal_tbl, 4,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_IRCView.setMetaObject( metaObj );
    return metaObj;
}

void* IRCView::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "IRCView" ) ) return (IRCView*)this;
    return KTextBrowser::qt_cast( clname );
}

// SIGNAL newText
void IRCView::newText()
{
    activate_signal( staticMetaObject()->signalOffset() + 0 );
}

// SIGNAL newURL
void IRCView::newURL( QString t0 )
{
    activate_signal( staticMetaObject()->signalOffset() + 1, t0 );
}

// SIGNAL gotFocus
void IRCView::gotFocus()
{
    activate_signal( staticMetaObject()->signalOffset() + 2 );
}

// SIGNAL textToLog
void IRCView::textToLog( const QString& t0 )
{
    activate_signal( staticMetaObject()->signalOffset() + 3, t0 );
}

bool IRCView::qt_invoke( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->slotOffset() ) {
    case 0: append(static_QUType_charstar.get(_o+1),static_QUType_charstar.get(_o+2)); break;
    case 1: appendQuery(static_QUType_charstar.get(_o+1),static_QUType_charstar.get(_o+2)); break;
    case 2: appendAction(static_QUType_charstar.get(_o+1),static_QUType_charstar.get(_o+2)); break;
    case 3: appendServerMessage(static_QUType_charstar.get(_o+1),static_QUType_charstar.get(_o+2)); break;
    case 4: appendCommandMessage(static_QUType_charstar.get(_o+1),static_QUType_charstar.get(_o+2)); break;
    case 5: appendBacklogMessage(static_QUType_charstar.get(_o+1),static_QUType_charstar.get(_o+2)); break;
    default:
	return KTextBrowser::qt_invoke( _id, _o );
    }
    return TRUE;
}

bool IRCView::qt_emit( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->signalOffset() ) {
    case 0: newText(); break;
    case 1: newURL(static_QUType_QString.get(_o+1)); break;
    case 2: gotFocus(); break;
    case 3: textToLog(static_QUType_QString.get(_o+1)); break;
    default:
	return KTextBrowser::qt_emit(_id,_o);
    }
    return TRUE;
}
#ifndef QT_NO_PROPERTIES

bool IRCView::qt_property( int _id, int _f, QVariant* _v)
{
    return KTextBrowser::qt_property( _id, _f, _v);
}
#endif // QT_NO_PROPERTIES
