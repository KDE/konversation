//
// C++ Interface: quickbuttonsconfigcontroller
//
// Description:
//
//
// Author: Dario Abatianni <eisfuchs@tigress.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef QUICKBUTTONSCONFIGCONTROLLER_H
#define QUICKBUTTONSCONFIGCONTROLLER_H

#include <qobject.h>

class QuickButtons_Config;
class QListViewItem;

/**
  @author Dario Abatianni <eisfuchs@tigress.com>
*/

class QuickButtonsConfigController : public QObject
{
  Q_OBJECT

  public:
    QuickButtonsConfigController(QuickButtons_Config* quickButtonsPage,QObject *parent = 0, const char *name = 0);
    ~QuickButtonsConfigController();

    void saveSettings();

  signals:
    void modified();

  protected slots:
    void entrySelected(QListViewItem* quickButtonEntry);
    void nameChanged(const QString& newName);
    void actionChanged(const QString& newAction);

  protected:
    void populateQuickButtonsList();
    QuickButtons_Config* m_quickButtonsPage;
    bool newItemSelected;
};

#endif
