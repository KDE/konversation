/****************************************************************************
** QuickButtonsDialog meta object code from reading C++ file 'quickbuttonsdialog.h'
**
** Created: Tue Jun 25 05:13:35 2002
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "quickbuttonsdialog.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 19)
#error "This file was generated using the moc from 3.0.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

const char *QuickButtonsDialog::className() const
{
    return "QuickButtonsDialog";
}

QMetaObject *QuickButtonsDialog::metaObj = 0;
static QMetaObjectCleanUp cleanUp_QuickButtonsDialog;

#ifndef QT_NO_TRANSLATION
QString QuickButtonsDialog::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "QuickButtonsDialog", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString QuickButtonsDialog::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "QuickButtonsDialog", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* QuickButtonsDialog::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = KDialogBase::staticMetaObject();
    static const QUMethod slot_0 = {"slotOk", 0, 0 };
    static const QUMethod slot_1 = {"slotApply", 0, 0 };
    static const QUMethod slot_2 = {"slotCancel", 0, 0 };
    static const QMetaData slot_tbl[] = {
	{ "slotOk()", &slot_0, QMetaData::Protected },
	{ "slotApply()", &slot_1, QMetaData::Protected },
	{ "slotCancel()", &slot_2, QMetaData::Protected }
    };
    static const QUParameter param_signal_0[] = {
	{ "newList", &static_QUType_ptr, "QStringList", QUParameter::In }
    };
    static const QUMethod signal_0 = {"applyClicked", 1, param_signal_0 };
    static const QUParameter param_signal_1[] = {
	{ "newButtonsSize", &static_QUType_ptr, "QSize", QUParameter::In }
    };
    static const QUMethod signal_1 = {"cancelClicked", 1, param_signal_1 };
    static const QMetaData signal_tbl[] = {
	{ "applyClicked(QStringList)", &signal_0, QMetaData::Protected },
	{ "cancelClicked(QSize)", &signal_1, QMetaData::Protected }
    };
    metaObj = QMetaObject::new_metaobject(
	"QuickButtonsDialog", parentObject,
	slot_tbl, 3,
	signal_tbl, 2,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_QuickButtonsDialog.setMetaObject( metaObj );
    return metaObj;
}

void* QuickButtonsDialog::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "QuickButtonsDialog" ) ) return (QuickButtonsDialog*)this;
    return KDialogBase::qt_cast( clname );
}

#include <qobjectdefs.h>
#include <qsignalslotimp.h>

// SIGNAL applyClicked
void QuickButtonsDialog::applyClicked( QStringList t0 )
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

// SIGNAL cancelClicked
void QuickButtonsDialog::cancelClicked( QSize t0 )
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

bool QuickButtonsDialog::qt_invoke( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->slotOffset() ) {
    case 0: slotOk(); break;
    case 1: slotApply(); break;
    case 2: slotCancel(); break;
    default:
	return KDialogBase::qt_invoke( _id, _o );
    }
    return TRUE;
}

bool QuickButtonsDialog::qt_emit( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->signalOffset() ) {
    case 0: applyClicked(*((QStringList*)static_QUType_ptr.get(_o+1))); break;
    case 1: cancelClicked(*((QSize*)static_QUType_ptr.get(_o+1))); break;
    default:
	return KDialogBase::qt_emit(_id,_o);
    }
    return TRUE;
}
#ifndef QT_NO_PROPERTIES

bool QuickButtonsDialog::qt_property( int _id, int _f, QVariant* _v)
{
    return KDialogBase::qt_property( _id, _f, _v);
}
#endif // QT_NO_PROPERTIES
