/****************************************************************************
** OutputFilter meta object code from reading C++ file 'outputfilter.h'
**
** Created: Tue Jun 25 05:13:26 2002
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "outputfilter.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 19)
#error "This file was generated using the moc from 3.0.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

const char *OutputFilter::className() const
{
    return "OutputFilter";
}

QMetaObject *OutputFilter::metaObj = 0;
static QMetaObjectCleanUp cleanUp_OutputFilter;

#ifndef QT_NO_TRANSLATION
QString OutputFilter::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "OutputFilter", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString OutputFilter::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "OutputFilter", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* OutputFilter::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = QObject::staticMetaObject();
    static const QUParameter param_signal_0[] = {
	{ "nick", &static_QUType_QString, 0, QUParameter::In },
	{ "hostmask", &static_QUType_QString, 0, QUParameter::In }
    };
    static const QUMethod signal_0 = {"openQuery", 2, param_signal_0 };
    static const QMetaData signal_tbl[] = {
	{ "openQuery(const QString&,const QString&)", &signal_0, QMetaData::Protected }
    };
    metaObj = QMetaObject::new_metaobject(
	"OutputFilter", parentObject,
	0, 0,
	signal_tbl, 1,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_OutputFilter.setMetaObject( metaObj );
    return metaObj;
}

void* OutputFilter::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "OutputFilter" ) ) return (OutputFilter*)this;
    return QObject::qt_cast( clname );
}

#include <qobjectdefs.h>
#include <qsignalslotimp.h>

// SIGNAL openQuery
void OutputFilter::openQuery( const QString& t0, const QString& t1 )
{
    if ( signalsBlocked() )
	return;
    QConnectionList *clist = receivers( staticMetaObject()->signalOffset() + 0 );
    if ( !clist )
	return;
    QUObject o[3];
    static_QUType_QString.set(o+1,t0);
    static_QUType_QString.set(o+2,t1);
    activate_signal( clist, o );
}

bool OutputFilter::qt_invoke( int _id, QUObject* _o )
{
    return QObject::qt_invoke(_id,_o);
}

bool OutputFilter::qt_emit( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->signalOffset() ) {
    case 0: openQuery(static_QUType_QString.get(_o+1),static_QUType_QString.get(_o+2)); break;
    default:
	return QObject::qt_emit(_id,_o);
    }
    return TRUE;
}
#ifndef QT_NO_PROPERTIES

bool OutputFilter::qt_property( int _id, int _f, QVariant* _v)
{
    return QObject::qt_property( _id, _f, _v);
}
#endif // QT_NO_PROPERTIES
