/****************************************************************************
** QuickButton meta object code from reading C++ file 'quickbutton.h'
**
** Created: Tue Jun 25 05:13:44 2002
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "quickbutton.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 19)
#error "This file was generated using the moc from 3.0.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

const char *QuickButton::className() const
{
    return "QuickButton";
}

QMetaObject *QuickButton::metaObj = 0;
static QMetaObjectCleanUp cleanUp_QuickButton;

#ifndef QT_NO_TRANSLATION
QString QuickButton::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "QuickButton", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString QuickButton::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "QuickButton", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* QuickButton::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = QPushButton::staticMetaObject();
    static const QUMethod slot_0 = {"wasClicked", 0, 0 };
    static const QMetaData slot_tbl[] = {
	{ "wasClicked()", &slot_0, QMetaData::Public }
    };
    static const QUParameter param_signal_0[] = {
	{ 0, &static_QUType_int, 0, QUParameter::In }
    };
    static const QUMethod signal_0 = {"clicked", 1, param_signal_0 };
    static const QMetaData signal_tbl[] = {
	{ "clicked(int)", &signal_0, QMetaData::Public }
    };
    metaObj = QMetaObject::new_metaobject(
	"QuickButton", parentObject,
	slot_tbl, 1,
	signal_tbl, 1,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_QuickButton.setMetaObject( metaObj );
    return metaObj;
}

void* QuickButton::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "QuickButton" ) ) return (QuickButton*)this;
    return QPushButton::qt_cast( clname );
}

// SIGNAL clicked
void QuickButton::clicked( int t0 )
{
    activate_signal( staticMetaObject()->signalOffset() + 0, t0 );
}

bool QuickButton::qt_invoke( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->slotOffset() ) {
    case 0: wasClicked(); break;
    default:
	return QPushButton::qt_invoke( _id, _o );
    }
    return TRUE;
}

bool QuickButton::qt_emit( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->signalOffset() ) {
    case 0: clicked(static_QUType_int.get(_o+1)); break;
    default:
	return QPushButton::qt_emit(_id,_o);
    }
    return TRUE;
}
#ifndef QT_NO_PROPERTIES

bool QuickButton::qt_property( int _id, int _f, QVariant* _v)
{
    return QPushButton::qt_property( _id, _f, _v);
}
#endif // QT_NO_PROPERTIES
