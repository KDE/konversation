/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefsdialog.h  -  description
  begin:     Sun Feb 10 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef PREFSDIALOG_H
#define PREFSDIALOG_H

#include <qpushbutton.h>
#include <qtabwidget.h>

#include <kdialogbase.h>
#include <klistview.h>

#include "preferences.h"

/*
 *@author Dario Abatianni
*/

class PrefsDialog : public KDialogBase
{
  Q_OBJECT

  public:
    PrefsDialog(Preferences* preferences,bool noServer);
    ~PrefsDialog();

  signals:
    void connectToServer(int);
    void prefsChanged();
    void closed();

  protected slots:
    void connectClicked();
    void newServer();
    void editServer();
    void removeServer();
    void serverSelected(QListViewItem* item);
    void serverDoubleClicked(QListViewItem* item);
    void updateServer(const QString&,const QString&,const QString&,const QString&,const QString&,const QString&);
    void updateServerProperty(QListViewItem*,const QString&,int);
    void realNameChanged(const QString& newRealName);
    void loginChanged(const QString& newlogin);
    void nick0Changed(const QString& newNick);
    void nick1Changed(const QString& newNick);
    void nick2Changed(const QString& newNick);
    void nick3Changed(const QString& newNick);

    void slotOk();
    void slotApply();
    void slotCancel();

  protected:
    QTabWidget* prefsTabs;
    QPushButton* connectButton;
    QPushButton* newServerButton;
    QPushButton* editServerButton;
    QPushButton* removeServerButton;
    KListView* serverListView;
    Preferences* preferences;

    void setPreferences(Preferences* newPrefs) { preferences=newPrefs; };
};

#endif
