/****************************************************************************
** PrefsDialog meta object code from reading C++ file 'prefsdialog.h'
**
** Created: Tue Jun 25 05:13:59 2002
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "prefsdialog.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 19)
#error "This file was generated using the moc from 3.0.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

const char *PrefsDialog::className() const
{
    return "PrefsDialog";
}

QMetaObject *PrefsDialog::metaObj = 0;
static QMetaObjectCleanUp cleanUp_PrefsDialog;

#ifndef QT_NO_TRANSLATION
QString PrefsDialog::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "PrefsDialog", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString PrefsDialog::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "PrefsDialog", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* PrefsDialog::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = KDialogBase::staticMetaObject();
    static const QUMethod slot_0 = {"connectClicked", 0, 0 };
    static const QUMethod slot_1 = {"editServer", 0, 0 };
    static const QUParameter param_slot_2[] = {
	{ "item", &static_QUType_ptr, "QListViewItem", QUParameter::In }
    };
    static const QUMethod slot_2 = {"serverSelected", 1, param_slot_2 };
    static const QUParameter param_slot_3[] = {
	{ "item", &static_QUType_ptr, "QListViewItem", QUParameter::In }
    };
    static const QUMethod slot_3 = {"serverDoubleClicked", 1, param_slot_3 };
    static const QUParameter param_slot_4[] = {
	{ 0, &static_QUType_QString, 0, QUParameter::In },
	{ 0, &static_QUType_QString, 0, QUParameter::In },
	{ 0, &static_QUType_QString, 0, QUParameter::In },
	{ 0, &static_QUType_QString, 0, QUParameter::In },
	{ 0, &static_QUType_QString, 0, QUParameter::In },
	{ 0, &static_QUType_QString, 0, QUParameter::In }
    };
    static const QUMethod slot_4 = {"updateServer", 6, param_slot_4 };
    static const QUParameter param_slot_5[] = {
	{ 0, &static_QUType_ptr, "QListViewItem", QUParameter::In },
	{ 0, &static_QUType_QString, 0, QUParameter::In },
	{ 0, &static_QUType_int, 0, QUParameter::In }
    };
    static const QUMethod slot_5 = {"updateServerProperty", 3, param_slot_5 };
    static const QUParameter param_slot_6[] = {
	{ "newRealName", &static_QUType_QString, 0, QUParameter::In }
    };
    static const QUMethod slot_6 = {"realNameChanged", 1, param_slot_6 };
    static const QUParameter param_slot_7[] = {
	{ "newlogin", &static_QUType_QString, 0, QUParameter::In }
    };
    static const QUMethod slot_7 = {"loginChanged", 1, param_slot_7 };
    static const QUParameter param_slot_8[] = {
	{ "newNick", &static_QUType_QString, 0, QUParameter::In }
    };
    static const QUMethod slot_8 = {"nick0Changed", 1, param_slot_8 };
    static const QUParameter param_slot_9[] = {
	{ "newNick", &static_QUType_QString, 0, QUParameter::In }
    };
    static const QUMethod slot_9 = {"nick1Changed", 1, param_slot_9 };
    static const QUParameter param_slot_10[] = {
	{ "newNick", &static_QUType_QString, 0, QUParameter::In }
    };
    static const QUMethod slot_10 = {"nick2Changed", 1, param_slot_10 };
    static const QUParameter param_slot_11[] = {
	{ "newNick", &static_QUType_QString, 0, QUParameter::In }
    };
    static const QUMethod slot_11 = {"nick3Changed", 1, param_slot_11 };
    static const QUMethod slot_12 = {"slotOk", 0, 0 };
    static const QUMethod slot_13 = {"slotApply", 0, 0 };
    static const QUMethod slot_14 = {"slotCancel", 0, 0 };
    static const QMetaData slot_tbl[] = {
	{ "connectClicked()", &slot_0, QMetaData::Protected },
	{ "editServer()", &slot_1, QMetaData::Protected },
	{ "serverSelected(QListViewItem*)", &slot_2, QMetaData::Protected },
	{ "serverDoubleClicked(QListViewItem*)", &slot_3, QMetaData::Protected },
	{ "updateServer(const QString&,const QString&,const QString&,const QString&,const QString&,const QString&)", &slot_4, QMetaData::Protected },
	{ "updateServerProperty(QListViewItem*,const QString&,int)", &slot_5, QMetaData::Protected },
	{ "realNameChanged(const QString&)", &slot_6, QMetaData::Protected },
	{ "loginChanged(const QString&)", &slot_7, QMetaData::Protected },
	{ "nick0Changed(const QString&)", &slot_8, QMetaData::Protected },
	{ "nick1Changed(const QString&)", &slot_9, QMetaData::Protected },
	{ "nick2Changed(const QString&)", &slot_10, QMetaData::Protected },
	{ "nick3Changed(const QString&)", &slot_11, QMetaData::Protected },
	{ "slotOk()", &slot_12, QMetaData::Protected },
	{ "slotApply()", &slot_13, QMetaData::Protected },
	{ "slotCancel()", &slot_14, QMetaData::Protected }
    };
    static const QUParameter param_signal_0[] = {
	{ 0, &static_QUType_int, 0, QUParameter::In }
    };
    static const QUMethod signal_0 = {"connectToServer", 1, param_signal_0 };
    static const QUMethod signal_1 = {"prefsChanged", 0, 0 };
    static const QUMethod signal_2 = {"closed", 0, 0 };
    static const QMetaData signal_tbl[] = {
	{ "connectToServer(int)", &signal_0, QMetaData::Public },
	{ "prefsChanged()", &signal_1, QMetaData::Public },
	{ "closed()", &signal_2, QMetaData::Public }
    };
    metaObj = QMetaObject::new_metaobject(
	"PrefsDialog", parentObject,
	slot_tbl, 15,
	signal_tbl, 3,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_PrefsDialog.setMetaObject( metaObj );
    return metaObj;
}

void* PrefsDialog::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "PrefsDialog" ) ) return (PrefsDialog*)this;
    return KDialogBase::qt_cast( clname );
}

// SIGNAL connectToServer
void PrefsDialog::connectToServer( int t0 )
{
    activate_signal( staticMetaObject()->signalOffset() + 0, t0 );
}

// SIGNAL prefsChanged
void PrefsDialog::prefsChanged()
{
    activate_signal( staticMetaObject()->signalOffset() + 1 );
}

// SIGNAL closed
void PrefsDialog::closed()
{
    activate_signal( staticMetaObject()->signalOffset() + 2 );
}

bool PrefsDialog::qt_invoke( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->slotOffset() ) {
    case 0: connectClicked(); break;
    case 1: editServer(); break;
    case 2: serverSelected((QListViewItem*)static_QUType_ptr.get(_o+1)); break;
    case 3: serverDoubleClicked((QListViewItem*)static_QUType_ptr.get(_o+1)); break;
    case 4: updateServer(static_QUType_QString.get(_o+1),static_QUType_QString.get(_o+2),static_QUType_QString.get(_o+3),static_QUType_QString.get(_o+4),static_QUType_QString.get(_o+5),static_QUType_QString.get(_o+6)); break;
    case 5: updateServerProperty((QListViewItem*)static_QUType_ptr.get(_o+1),static_QUType_QString.get(_o+2),static_QUType_int.get(_o+3)); break;
    case 6: realNameChanged(static_QUType_QString.get(_o+1)); break;
    case 7: loginChanged(static_QUType_QString.get(_o+1)); break;
    case 8: nick0Changed(static_QUType_QString.get(_o+1)); break;
    case 9: nick1Changed(static_QUType_QString.get(_o+1)); break;
    case 10: nick2Changed(static_QUType_QString.get(_o+1)); break;
    case 11: nick3Changed(static_QUType_QString.get(_o+1)); break;
    case 12: slotOk(); break;
    case 13: slotApply(); break;
    case 14: slotCancel(); break;
    default:
	return KDialogBase::qt_invoke( _id, _o );
    }
    return TRUE;
}

bool PrefsDialog::qt_emit( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->signalOffset() ) {
    case 0: connectToServer(static_QUType_int.get(_o+1)); break;
    case 1: prefsChanged(); break;
    case 2: closed(); break;
    default:
	return KDialogBase::qt_emit(_id,_o);
    }
    return TRUE;
}
#ifndef QT_NO_PROPERTIES

bool PrefsDialog::qt_property( int _id, int _f, QVariant* _v)
{
    return KDialogBase::qt_property( _id, _f, _v);
}
#endif // QT_NO_PROPERTIES
