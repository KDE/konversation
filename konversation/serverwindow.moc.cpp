/****************************************************************************
** ServerWindow meta object code from reading C++ file 'serverwindow.h'
**
** Created: Tue Jun 25 05:13:38 2002
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "serverwindow.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 19)
#error "This file was generated using the moc from 3.0.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

const char *ServerWindow::className() const
{
    return "ServerWindow";
}

QMetaObject *ServerWindow::metaObj = 0;
static QMetaObjectCleanUp cleanUp_ServerWindow;

#ifndef QT_NO_TRANSLATION
QString ServerWindow::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "ServerWindow", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString ServerWindow::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "ServerWindow", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* ServerWindow::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = KMainWindow::staticMetaObject();
    static const QUParameter param_slot_0[] = {
	{ 0, &static_QUType_QString, 0, QUParameter::In }
    };
    static const QUMethod slot_0 = {"setNickname", 1, param_slot_0 };
    static const QUParameter param_slot_1[] = {
	{ "view", &static_QUType_ptr, "QWidget", QUParameter::In }
    };
    static const QUMethod slot_1 = {"newText", 1, param_slot_1 };
    static const QUParameter param_slot_2[] = {
	{ "view", &static_QUType_ptr, "QWidget", QUParameter::In }
    };
    static const QUMethod slot_2 = {"changedView", 1, param_slot_2 };
    static const QUParameter param_slot_3[] = {
	{ "text", &static_QUType_QString, 0, QUParameter::In }
    };
    static const QUMethod slot_3 = {"logText", 1, param_slot_3 };
    static const QUMethod slot_4 = {"statusTextEntered", 0, 0 };
    static const QUMethod slot_5 = {"addStatusView", 0, 0 };
    static const QUMethod slot_6 = {"showToolbar", 0, 0 };
    static const QUMethod slot_7 = {"openPreferences", 0, 0 };
    static const QUMethod slot_8 = {"quitProgram", 0, 0 };
    static const QUMethod slot_9 = {"openHilight", 0, 0 };
    static const QUParameter param_slot_10[] = {
	{ "newSize", &static_QUType_ptr, "QSize", QUParameter::In }
    };
    static const QUMethod slot_10 = {"closeHilight", 1, param_slot_10 };
    static const QUParameter param_slot_11[] = {
	{ "newList", &static_QUType_ptr, "QStringList", QUParameter::In }
    };
    static const QUMethod slot_11 = {"saveHilight", 1, param_slot_11 };
    static const QUMethod slot_12 = {"openIgnore", 0, 0 };
    static const QUParameter param_slot_13[] = {
	{ "newList", &static_QUType_ptr, "QPtrList<Ignore>", QUParameter::In }
    };
    static const QUMethod slot_13 = {"applyIgnore", 1, param_slot_13 };
    static const QUParameter param_slot_14[] = {
	{ "newSize", &static_QUType_ptr, "QSize", QUParameter::In }
    };
    static const QUMethod slot_14 = {"closeIgnore", 1, param_slot_14 };
    static const QUMethod slot_15 = {"openButtons", 0, 0 };
    static const QUParameter param_slot_16[] = {
	{ "newList", &static_QUType_ptr, "QStringList", QUParameter::In }
    };
    static const QUMethod slot_16 = {"applyButtons", 1, param_slot_16 };
    static const QUParameter param_slot_17[] = {
	{ "newSize", &static_QUType_ptr, "QSize", QUParameter::In }
    };
    static const QUMethod slot_17 = {"closeButtons", 1, param_slot_17 };
    static const QMetaData slot_tbl[] = {
	{ "setNickname(const QString&)", &slot_0, QMetaData::Public },
	{ "newText(QWidget*)", &slot_1, QMetaData::Public },
	{ "changedView(QWidget*)", &slot_2, QMetaData::Public },
	{ "logText(const QString&)", &slot_3, QMetaData::Public },
	{ "statusTextEntered()", &slot_4, QMetaData::Protected },
	{ "addStatusView()", &slot_5, QMetaData::Protected },
	{ "showToolbar()", &slot_6, QMetaData::Protected },
	{ "openPreferences()", &slot_7, QMetaData::Protected },
	{ "quitProgram()", &slot_8, QMetaData::Protected },
	{ "openHilight()", &slot_9, QMetaData::Protected },
	{ "closeHilight(QSize)", &slot_10, QMetaData::Protected },
	{ "saveHilight(QStringList)", &slot_11, QMetaData::Protected },
	{ "openIgnore()", &slot_12, QMetaData::Protected },
	{ "applyIgnore(QPtrList<Ignore>)", &slot_13, QMetaData::Protected },
	{ "closeIgnore(QSize)", &slot_14, QMetaData::Protected },
	{ "openButtons()", &slot_15, QMetaData::Protected },
	{ "applyButtons(QStringList)", &slot_16, QMetaData::Protected },
	{ "closeButtons(QSize)", &slot_17, QMetaData::Protected }
    };
    static const QUMethod signal_0 = {"prefsChanged", 0, 0 };
    static const QMetaData signal_tbl[] = {
	{ "prefsChanged()", &signal_0, QMetaData::Public }
    };
    metaObj = QMetaObject::new_metaobject(
	"ServerWindow", parentObject,
	slot_tbl, 18,
	signal_tbl, 1,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_ServerWindow.setMetaObject( metaObj );
    return metaObj;
}

void* ServerWindow::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "ServerWindow" ) ) return (ServerWindow*)this;
    return KMainWindow::qt_cast( clname );
}

// SIGNAL prefsChanged
void ServerWindow::prefsChanged()
{
    activate_signal( staticMetaObject()->signalOffset() + 0 );
}

bool ServerWindow::qt_invoke( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->slotOffset() ) {
    case 0: setNickname(static_QUType_QString.get(_o+1)); break;
    case 1: newText((QWidget*)static_QUType_ptr.get(_o+1)); break;
    case 2: changedView((QWidget*)static_QUType_ptr.get(_o+1)); break;
    case 3: logText(static_QUType_QString.get(_o+1)); break;
    case 4: statusTextEntered(); break;
    case 5: addStatusView(); break;
    case 6: showToolbar(); break;
    case 7: openPreferences(); break;
    case 8: quitProgram(); break;
    case 9: openHilight(); break;
    case 10: closeHilight(*((QSize*)static_QUType_ptr.get(_o+1))); break;
    case 11: saveHilight(*((QStringList*)static_QUType_ptr.get(_o+1))); break;
    case 12: openIgnore(); break;
    case 13: applyIgnore(*((QPtrList<Ignore>*)static_QUType_ptr.get(_o+1))); break;
    case 14: closeIgnore(*((QSize*)static_QUType_ptr.get(_o+1))); break;
    case 15: openButtons(); break;
    case 16: applyButtons(*((QStringList*)static_QUType_ptr.get(_o+1))); break;
    case 17: closeButtons(*((QSize*)static_QUType_ptr.get(_o+1))); break;
    default:
	return KMainWindow::qt_invoke( _id, _o );
    }
    return TRUE;
}

bool ServerWindow::qt_emit( int _id, QUObject* _o )
{
    switch ( _id - staticMetaObject()->signalOffset() ) {
    case 0: prefsChanged(); break;
    default:
	return KMainWindow::qt_emit(_id,_o);
    }
    return TRUE;
}
#ifndef QT_NO_PROPERTIES

bool ServerWindow::qt_property( int _id, int _f, QVariant* _v)
{
    return KMainWindow::qt_property( _id, _f, _v);
}
#endif // QT_NO_PROPERTIES
