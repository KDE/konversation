/****************************************************************************
** Query meta object code from reading C++ file 'query.h'
**
** Created: Tue Jun 25 05:13:20 2002
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "query.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 19)
#error "This file was generated using the moc from 3.0.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

const char *Query::className() const
{
    return "Query";
}

QMetaObject *Query::metaObj = 0;
static QMetaObjectCleanUp cleanUp_Query;

#ifndef QT_NO_TRANSLATION
QString Query::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "Query", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString Query::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "Query", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* Query::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = ChatWindow::staticMetaObject();
    static const QUMethod slot_0 = {"queryTextEntered", 0, 0 };
    static const QUMethod slot_1 = {"newTextInView", 0, 0 };
    static const QUMethod slot_2 = {"close", 0, 0 };
    static const QMetaData slot_tbl[] = {
	{ "queryTextEntered()", &slot_0, QMetaData::Protected },
	{ "newTextInView()", &slot_1, QMetaData::Protected },
	{ "close()", &slot_2, QMetaData::Protected }
    };
    static const QUParameter param_signal_0[] = {
	{ "query", &static_QUType_ptr, "QWidget", QUParameter::In }
    };
    static const QUMethod signal_0 = {"newText", 1, param_signal_0 };
    static const QUParameter param_signal_1[] = {
	{ "query", &static_QUType_ptr, "Query", QUParameter::In }
    };
    static const QUMethod signal_1 = {"closed", 1, param_signal_1 };
    static const QMetaData signal_tbl[] = {
	{ "newText(QWidget*)", &signal_0, QMetaData::Public },
	{ "closed(Query*)", &signal_1, QMetaData::Public }
    };
    metaObj = QMetaObject::new_metaobject(
	"Query", parentObject,
	slot_tbl, 3,
	signal_tbl, 2,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_Query.setMetaObject( metaObj );
    return metaObj;
}

void* Query::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "Query" ) ) return (Query*)this;
    return ChatWindow::qt_cast( clname );
}

#include <qobjectdefs.h>
#include <qsignalslotimp.h>

// SIGNAL newText
void Query::newText( QWidget* t0 )
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

// SIGNAL closed
void Query::closed( Query* t0 )
{
    if ( signalsBlocked() )
	return;
    QConnectionList *clist = receivers( staticMetaObject()->signalOffset() + 1 );
    if ( !clist )
	return;
    QUObject o[2];
    static_QUType_ptr.set(o+1,t0);
    activate_signal( clist, o );
}

bool Query::qt_invoke( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->slotOffset() ) {
    case 0: queryTextEntered(); break;
    case 1: newTextInView(); break;
    case 2: close(); break;
    default:
	return ChatWindow::qt_invoke( _id, _o );
    }
    return TRUE;
}

bool Query::qt_emit( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->signalOffset() ) {
    case 0: newText((QWidget*)static_QUType_ptr.get(_o+1)); break;
    case 1: closed((Query*)static_QUType_ptr.get(_o+1)); break;
    default:
	return ChatWindow::qt_emit(_id,_o);
    }
    return TRUE;
}
#ifndef QT_NO_PROPERTIES

bool Query::qt_property( int _id, int _f, QVariant* _v)
{
    return ChatWindow::qt_property( _id, _f, _v);
}
#endif // QT_NO_PROPERTIES
