/****************************************************************************
** HighLightBox meta object code from reading C++ file 'highlightbox.h'
**
** Created: Tue Jun 25 05:13:46 2002
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "highlightbox.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 19)
#error "This file was generated using the moc from 3.0.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

const char *HighLightBox::className() const
{
    return "HighLightBox";
}

QMetaObject *HighLightBox::metaObj = 0;
static QMetaObjectCleanUp cleanUp_HighLightBox;

#ifndef QT_NO_TRANSLATION
QString HighLightBox::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "HighLightBox", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString HighLightBox::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "HighLightBox", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* HighLightBox::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = QVBox::staticMetaObject();
    static const QUParameter param_slot_0[] = {
	{ "selectedItem", &static_QUType_ptr, "QListBoxItem", QUParameter::In }
    };
    static const QUMethod slot_0 = {"updateInputLine", 1, param_slot_0 };
    static const QUParameter param_slot_1[] = {
	{ "inputText", &static_QUType_QString, 0, QUParameter::In }
    };
    static const QUMethod slot_1 = {"updateListItem", 1, param_slot_1 };
    static const QUMethod slot_2 = {"addItemToList", 0, 0 };
    static const QUMethod slot_3 = {"removeItemFromList", 0, 0 };
    static const QUMethod slot_4 = {"closeWindowDiscardingChanges", 0, 0 };
    static const QUMethod slot_5 = {"closeWindowReturnChanges", 0, 0 };
    static const QMetaData slot_tbl[] = {
	{ "updateInputLine(QListBoxItem*)", &slot_0, QMetaData::Protected },
	{ "updateListItem(const QString&)", &slot_1, QMetaData::Protected },
	{ "addItemToList()", &slot_2, QMetaData::Protected },
	{ "removeItemFromList()", &slot_3, QMetaData::Protected },
	{ "closeWindowDiscardingChanges()", &slot_4, QMetaData::Public },
	{ "closeWindowReturnChanges()", &slot_5, QMetaData::Public }
    };
    static const QUParameter param_signal_0[] = {
	{ "HighLightList", &static_QUType_ptr, "QStringList", QUParameter::In }
    };
    static const QUMethod signal_0 = {"highLightListChange", 1, param_signal_0 };
    static const QUParameter param_signal_1[] = {
	{ 0, &static_QUType_ptr, "QSize", QUParameter::In }
    };
    static const QUMethod signal_1 = {"highLightListClose", 1, param_signal_1 };
    static const QMetaData signal_tbl[] = {
	{ "highLightListChange(QStringList)", &signal_0, QMetaData::Public },
	{ "highLightListClose(QSize)", &signal_1, QMetaData::Public }
    };
    metaObj = QMetaObject::new_metaobject(
	"HighLightBox", parentObject,
	slot_tbl, 6,
	signal_tbl, 2,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_HighLightBox.setMetaObject( metaObj );
    return metaObj;
}

void* HighLightBox::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "HighLightBox" ) ) return (HighLightBox*)this;
    return QVBox::qt_cast( clname );
}

#include <qobjectdefs.h>
#include <qsignalslotimp.h>

// SIGNAL highLightListChange
void HighLightBox::highLightListChange( QStringList t0 )
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

// SIGNAL highLightListClose
void HighLightBox::highLightListClose( QSize t0 )
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

bool HighLightBox::qt_invoke( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->slotOffset() ) {
    case 0: updateInputLine((QListBoxItem*)static_QUType_ptr.get(_o+1)); break;
    case 1: updateListItem(static_QUType_QString.get(_o+1)); break;
    case 2: addItemToList(); break;
    case 3: removeItemFromList(); break;
    case 4: closeWindowDiscardingChanges(); break;
    case 5: closeWindowReturnChanges(); break;
    default:
	return QVBox::qt_invoke( _id, _o );
    }
    return TRUE;
}

bool HighLightBox::qt_emit( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->signalOffset() ) {
    case 0: highLightListChange(*((QStringList*)static_QUType_ptr.get(_o+1))); break;
    case 1: highLightListClose(*((QSize*)static_QUType_ptr.get(_o+1))); break;
    default:
	return QVBox::qt_emit(_id,_o);
    }
    return TRUE;
}
#ifndef QT_NO_PROPERTIES

bool HighLightBox::qt_property( int _id, int _f, QVariant* _v)
{
    return QVBox::qt_property( _id, _f, _v);
}
#endif // QT_NO_PROPERTIES
