/****************************************************************************
** IgnoreCheckBox meta object code from reading C++ file 'ignorecheckbox.h'
**
** Created: Tue Jun 25 05:13:56 2002
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "ignorecheckbox.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 19)
#error "This file was generated using the moc from 3.0.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

const char *IgnoreCheckBox::className() const
{
    return "IgnoreCheckBox";
}

QMetaObject *IgnoreCheckBox::metaObj = 0;
static QMetaObjectCleanUp cleanUp_IgnoreCheckBox;

#ifndef QT_NO_TRANSLATION
QString IgnoreCheckBox::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "IgnoreCheckBox", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString IgnoreCheckBox::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "IgnoreCheckBox", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* IgnoreCheckBox::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = QCheckBox::staticMetaObject();
    static const QUMethod slot_0 = {"wasClicked", 0, 0 };
    static const QMetaData slot_tbl[] = {
	{ "wasClicked()", &slot_0, QMetaData::Protected }
    };
    static const QUParameter param_signal_0[] = {
	{ "flag", &static_QUType_int, 0, QUParameter::In },
	{ "active", &static_QUType_bool, 0, QUParameter::In }
    };
    static const QUMethod signal_0 = {"flagChanged", 2, param_signal_0 };
    static const QMetaData signal_tbl[] = {
	{ "flagChanged(int,bool)", &signal_0, QMetaData::Public }
    };
    metaObj = QMetaObject::new_metaobject(
	"IgnoreCheckBox", parentObject,
	slot_tbl, 1,
	signal_tbl, 1,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_IgnoreCheckBox.setMetaObject( metaObj );
    return metaObj;
}

void* IgnoreCheckBox::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "IgnoreCheckBox" ) ) return (IgnoreCheckBox*)this;
    return QCheckBox::qt_cast( clname );
}

#include <qobjectdefs.h>
#include <qsignalslotimp.h>

// SIGNAL flagChanged
void IgnoreCheckBox::flagChanged( int t0, bool t1 )
{
    if ( signalsBlocked() )
	return;
    QConnectionList *clist = receivers( staticMetaObject()->signalOffset() + 0 );
    if ( !clist )
	return;
    QUObject o[3];
    static_QUType_int.set(o+1,t0);
    static_QUType_bool.set(o+2,t1);
    activate_signal( clist, o );
}

bool IgnoreCheckBox::qt_invoke( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->slotOffset() ) {
    case 0: wasClicked(); break;
    default:
	return QCheckBox::qt_invoke( _id, _o );
    }
    return TRUE;
}

bool IgnoreCheckBox::qt_emit( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->signalOffset() ) {
    case 0: flagChanged(static_QUType_int.get(_o+1),static_QUType_bool.get(_o+2)); break;
    default:
	return QCheckBox::qt_emit(_id,_o);
    }
    return TRUE;
}
#ifndef QT_NO_PROPERTIES

bool IgnoreCheckBox::qt_property( int _id, int _f, QVariant* _v)
{
    return QCheckBox::qt_property( _id, _f, _v);
}
#endif // QT_NO_PROPERTIES
