/****************************************************************************
** EditServerDialog meta object code from reading C++ file 'editserverdialog.h'
**
** Created: Tue Jun 25 05:13:31 2002
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "editserverdialog.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 19)
#error "This file was generated using the moc from 3.0.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

const char *EditServerDialog::className() const
{
    return "EditServerDialog";
}

QMetaObject *EditServerDialog::metaObj = 0;
static QMetaObjectCleanUp cleanUp_EditServerDialog;

#ifndef QT_NO_TRANSLATION
QString EditServerDialog::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "EditServerDialog", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString EditServerDialog::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "EditServerDialog", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* EditServerDialog::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = KDialogBase::staticMetaObject();
    static const QUMethod slot_0 = {"slotOk", 0, 0 };
    static const QMetaData slot_tbl[] = {
	{ "slotOk()", &slot_0, QMetaData::Protected }
    };
    static const QUParameter param_signal_0[] = {
	{ "group", &static_QUType_QString, 0, QUParameter::In },
	{ "serverName", &static_QUType_QString, 0, QUParameter::In },
	{ "port", &static_QUType_QString, 0, QUParameter::In },
	{ "serverKey", &static_QUType_QString, 0, QUParameter::In },
	{ "channelName", &static_QUType_QString, 0, QUParameter::In },
	{ "channelKey", &static_QUType_QString, 0, QUParameter::In }
    };
    static const QUMethod signal_0 = {"serverChanged", 6, param_signal_0 };
    static const QMetaData signal_tbl[] = {
	{ "serverChanged(const QString&,const QString&,const QString&,const QString&,const QString&,const QString&)", &signal_0, QMetaData::Public }
    };
    metaObj = QMetaObject::new_metaobject(
	"EditServerDialog", parentObject,
	slot_tbl, 1,
	signal_tbl, 1,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_EditServerDialog.setMetaObject( metaObj );
    return metaObj;
}

void* EditServerDialog::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "EditServerDialog" ) ) return (EditServerDialog*)this;
    return KDialogBase::qt_cast( clname );
}

#include <qobjectdefs.h>
#include <qsignalslotimp.h>

// SIGNAL serverChanged
void EditServerDialog::serverChanged( const QString& t0, const QString& t1, const QString& t2, const QString& t3, const QString& t4, const QString& t5 )
{
    if ( signalsBlocked() )
	return;
    QConnectionList *clist = receivers( staticMetaObject()->signalOffset() + 0 );
    if ( !clist )
	return;
    QUObject o[7];
    static_QUType_QString.set(o+1,t0);
    static_QUType_QString.set(o+2,t1);
    static_QUType_QString.set(o+3,t2);
    static_QUType_QString.set(o+4,t3);
    static_QUType_QString.set(o+5,t4);
    static_QUType_QString.set(o+6,t5);
    activate_signal( clist, o );
}

bool EditServerDialog::qt_invoke( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->slotOffset() ) {
    case 0: slotOk(); break;
    default:
	return KDialogBase::qt_invoke( _id, _o );
    }
    return TRUE;
}

bool EditServerDialog::qt_emit( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->signalOffset() ) {
    case 0: serverChanged(static_QUType_QString.get(_o+1),static_QUType_QString.get(_o+2),static_QUType_QString.get(_o+3),static_QUType_QString.get(_o+4),static_QUType_QString.get(_o+5),static_QUType_QString.get(_o+6)); break;
    default:
	return KDialogBase::qt_emit(_id,_o);
    }
    return TRUE;
}
#ifndef QT_NO_PROPERTIES

bool EditServerDialog::qt_property( int _id, int _f, QVariant* _v)
{
    return KDialogBase::qt_property( _id, _f, _v);
}
#endif // QT_NO_PROPERTIES
