/****************************************************************************
** MyColorCombo meta object code from reading C++ file 'mycolorcombo.h'
**
** Created: Tue Sep 24 13:46:51 2002
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "mycolorcombo.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 19)
#error "This file was generated using the moc from 3.0.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#include <qvariant.h>
const char *MyColorCombo::className() const
{
    return "MyColorCombo";
}

QMetaObject *MyColorCombo::metaObj = 0;
static QMetaObjectCleanUp cleanUp_MyColorCombo;

#ifndef QT_NO_TRANSLATION
QString MyColorCombo::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "MyColorCombo", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString MyColorCombo::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "MyColorCombo", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* MyColorCombo::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = QComboBox::staticMetaObject();
    static const QUParameter param_slot_0[] = {
	{ "index", &static_QUType_int, 0, QUParameter::In }
    };
    static const QUMethod slot_0 = {"slotActivated", 1, param_slot_0 };
    static const QUParameter param_slot_1[] = {
	{ "index", &static_QUType_int, 0, QUParameter::In }
    };
    static const QUMethod slot_1 = {"slotHighlighted", 1, param_slot_1 };
    static const QMetaData slot_tbl[] = {
	{ "slotActivated(int)", &slot_0, QMetaData::Private },
	{ "slotHighlighted(int)", &slot_1, QMetaData::Private }
    };
    static const QUParameter param_signal_0[] = {
	{ "col", &static_QUType_ptr, "const QColor&", QUParameter::In }
    };
    static const QUMethod signal_0 = {"activated", 1, param_signal_0 };
    static const QUParameter param_signal_1[] = {
	{ "col", &static_QUType_ptr, "const QColor&", QUParameter::In }
    };
    static const QUMethod signal_1 = {"highlighted", 1, param_signal_1 };
    static const QMetaData signal_tbl[] = {
	{ "activated(const QColor&)", &signal_0, QMetaData::Public },
	{ "highlighted(const QColor&)", &signal_1, QMetaData::Public }
    };
#ifndef QT_NO_PROPERTIES
    static const QMetaProperty props_tbl[1] = {
 	{ "QColor","color", 259, &MyColorCombo::metaObj, 0, -1 }
    };
#endif // QT_NO_PROPERTIES
    metaObj = QMetaObject::new_metaobject(
	"MyColorCombo", parentObject,
	slot_tbl, 2,
	signal_tbl, 2,
#ifndef QT_NO_PROPERTIES
	props_tbl, 1,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_MyColorCombo.setMetaObject( metaObj );
    return metaObj;
}

void* MyColorCombo::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "MyColorCombo" ) ) return (MyColorCombo*)this;
    return QComboBox::qt_cast( clname );
}

#include <qobjectdefs.h>
#include <qsignalslotimp.h>

// SIGNAL activated
void MyColorCombo::activated( const QColor& t0 )
{
    if ( signalsBlocked() )
	return;
    QConnectionList *clist = receivers( staticMetaObject()->signalOffset() + 0 );
    if ( !clist )
	return;
    QUObject o[2];
    static_QUType_ptr.set(o+1,&t0);
    activate_signal( clist, o );
}

// SIGNAL highlighted
void MyColorCombo::highlighted( const QColor& t0 )
{
    if ( signalsBlocked() )
	return;
    QConnectionList *clist = receivers( staticMetaObject()->signalOffset() + 1 );
    if ( !clist )
	return;
    QUObject o[2];
    static_QUType_ptr.set(o+1,&t0);
    activate_signal( clist, o );
}

bool MyColorCombo::qt_invoke( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->slotOffset() ) {
    case 0: slotActivated(static_QUType_int.get(_o+1)); break;
    case 1: slotHighlighted(static_QUType_int.get(_o+1)); break;
    default:
	return QComboBox::qt_invoke( _id, _o );
    }
    return TRUE;
}

bool MyColorCombo::qt_emit( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->signalOffset() ) {
    case 0: activated(*((QColor*)static_QUType_ptr.get(_o+1))); break;
    case 1: highlighted(*((QColor*)static_QUType_ptr.get(_o+1))); break;
    default:
	return QComboBox::qt_emit(_id,_o);
    }
    return TRUE;
}
#ifndef QT_NO_PROPERTIES

bool MyColorCombo::qt_property( int _id, int _f, QVariant* _v)
{
    switch ( _id - staticMetaObject()->propertyOffset() ) {
    case 0: switch( _f ) {
	case 0: setColor(_v->asColor()); break;
	case 1: *_v = QVariant( color() ); break;
	case 3: case 4: case 5: break;
	default: return FALSE;
    } break;
    default:
	return QComboBox::qt_property( _id, _f, _v );
    }
    return TRUE;
}
#endif // QT_NO_PROPERTIES
