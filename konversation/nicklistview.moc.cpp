/****************************************************************************
** NickListView meta object code from reading C++ file 'nicklistview.h'
**
** Created: Tue Jun 25 05:14:02 2002
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "nicklistview.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 19)
#error "This file was generated using the moc from 3.0.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

const char *NickListView::className() const
{
    return "NickListView";
}

QMetaObject *NickListView::metaObj = 0;
static QMetaObjectCleanUp cleanUp_NickListView;

#ifndef QT_NO_TRANSLATION
QString NickListView::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "NickListView", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString NickListView::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "NickListView", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* NickListView::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = KListView::staticMetaObject();
    static const QUParameter param_signal_0[] = {
	{ "id", &static_QUType_int, 0, QUParameter::In }
    };
    static const QUMethod signal_0 = {"popupCommand", 1, param_signal_0 };
    static const QMetaData signal_tbl[] = {
	{ "popupCommand(int)", &signal_0, QMetaData::Public }
    };
    metaObj = QMetaObject::new_metaobject(
	"NickListView", parentObject,
	0, 0,
	signal_tbl, 1,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_NickListView.setMetaObject( metaObj );
    return metaObj;
}

void* NickListView::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "NickListView" ) ) return (NickListView*)this;
    return KListView::qt_cast( clname );
}

// SIGNAL popupCommand
void NickListView::popupCommand( int t0 )
{
    activate_signal( staticMetaObject()->signalOffset() + 0, t0 );
}

bool NickListView::qt_invoke( int _id, QUObject* _o )
{
    return KListView::qt_invoke(_id,_o);
}

bool NickListView::qt_emit( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->signalOffset() ) {
    case 0: popupCommand(static_QUType_int.get(_o+1)); break;
    default:
	return KListView::qt_emit(_id,_o);
    }
    return TRUE;
}
#ifndef QT_NO_PROPERTIES

bool NickListView::qt_property( int _id, int _f, QVariant* _v)
{
    return KListView::qt_property( _id, _f, _v);
}
#endif // QT_NO_PROPERTIES
