/****************************************************************************
** LedTab meta object code from reading C++ file 'ledtab.h'
**
** Created: Tue Jun 25 05:13:04 2002
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "ledtab.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 19)
#error "This file was generated using the moc from 3.0.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

const char *LedTab::className() const
{
    return "LedTab";
}

QMetaObject *LedTab::metaObj = 0;
static QMetaObjectCleanUp cleanUp_LedTab;

#ifndef QT_NO_TRANSLATION
QString LedTab::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "LedTab", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString LedTab::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "LedTab", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* LedTab::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = QObject::staticMetaObject();
    static const QUMethod slot_0 = {"blinkTimeout", 0, 0 };
    static const QMetaData slot_tbl[] = {
	{ "blinkTimeout()", &slot_0, QMetaData::Protected }
    };
    static const QUMethod signal_0 = {"repaintTab", 0, 0 };
    static const QMetaData signal_tbl[] = {
	{ "repaintTab()", &signal_0, QMetaData::Protected }
    };
    metaObj = QMetaObject::new_metaobject(
	"LedTab", parentObject,
	slot_tbl, 1,
	signal_tbl, 1,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_LedTab.setMetaObject( metaObj );
    return metaObj;
}

void* LedTab::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "LedTab" ) ) return (LedTab*)this;
    if ( !qstrcmp( clname, "QTab" ) ) return (QTab*)this;
    return QObject::qt_cast( clname );
}

// SIGNAL repaintTab
void LedTab::repaintTab()
{
    activate_signal( staticMetaObject()->signalOffset() + 0 );
}

bool LedTab::qt_invoke( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->slotOffset() ) {
    case 0: blinkTimeout(); break;
    default:
	return QObject::qt_invoke( _id, _o );
    }
    return TRUE;
}

bool LedTab::qt_emit( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->signalOffset() ) {
    case 0: repaintTab(); break;
    default:
	return QObject::qt_emit(_id,_o);
    }
    return TRUE;
}
#ifndef QT_NO_PROPERTIES

bool LedTab::qt_property( int _id, int _f, QVariant* _v)
{
    return QObject::qt_property( _id, _f, _v);
}
#endif // QT_NO_PROPERTIES
