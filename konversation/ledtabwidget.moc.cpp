/****************************************************************************
** LedTabWidget meta object code from reading C++ file 'ledtabwidget.h'
**
** Created: Tue Jun 25 05:13:58 2002
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "ledtabwidget.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 19)
#error "This file was generated using the moc from 3.0.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

const char *LedTabWidget::className() const
{
    return "LedTabWidget";
}

QMetaObject *LedTabWidget::metaObj = 0;
static QMetaObjectCleanUp cleanUp_LedTabWidget;

#ifndef QT_NO_TRANSLATION
QString LedTabWidget::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "LedTabWidget", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString LedTabWidget::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "LedTabWidget", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* LedTabWidget::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = QTabWidget::staticMetaObject();
    static const QUParameter param_slot_0[] = {
	{ "id", &static_QUType_int, 0, QUParameter::In }
    };
    static const QUMethod slot_0 = {"tabSelected", 1, param_slot_0 };
    static const QMetaData slot_tbl[] = {
	{ "tabSelected(int)", &slot_0, QMetaData::Protected }
    };
    static const QUParameter param_signal_0[] = {
	{ "view", &static_QUType_ptr, "QWidget", QUParameter::In }
    };
    static const QUMethod signal_0 = {"currentChanged", 1, param_signal_0 };
    static const QMetaData signal_tbl[] = {
	{ "currentChanged(QWidget*)", &signal_0, QMetaData::Public }
    };
    metaObj = QMetaObject::new_metaobject(
	"LedTabWidget", parentObject,
	slot_tbl, 1,
	signal_tbl, 1,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_LedTabWidget.setMetaObject( metaObj );
    return metaObj;
}

void* LedTabWidget::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "LedTabWidget" ) ) return (LedTabWidget*)this;
    return QTabWidget::qt_cast( clname );
}

#include <qobjectdefs.h>
#include <qsignalslotimp.h>

// SIGNAL currentChanged
void LedTabWidget::currentChanged( QWidget* t0 )
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

bool LedTabWidget::qt_invoke( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->slotOffset() ) {
    case 0: tabSelected(static_QUType_int.get(_o+1)); break;
    default:
	return QTabWidget::qt_invoke( _id, _o );
    }
    return TRUE;
}

bool LedTabWidget::qt_emit( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->signalOffset() ) {
    case 0: currentChanged((QWidget*)static_QUType_ptr.get(_o+1)); break;
    default:
	return QTabWidget::qt_emit(_id,_o);
    }
    return TRUE;
}
#ifndef QT_NO_PROPERTIES

bool LedTabWidget::qt_property( int _id, int _f, QVariant* _v)
{
    return QTabWidget::qt_property( _id, _f, _v);
}
#endif // QT_NO_PROPERTIES
