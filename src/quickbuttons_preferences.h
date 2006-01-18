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
#ifndef QUICKBUTTONSCONFIG_H
#define QUICKBUTTONSCONFIG_H

#include "quickbuttons_preferencesui.h"
#include "konvisettingspage.h"

/**
  @author Dario Abatianni <eisfuchs@tigress.com>
*/

class QuickButtons_Config : public QuickButtons_ConfigUI, public KonviSettingsPage
{
  Q_OBJECT

  public:
    QuickButtons_Config(QWidget* parent, const char* name=NULL);
    ~QuickButtons_Config();

    virtual void saveSettings();
    virtual void loadSettings();
    virtual void restorePageToDefaults();

    virtual bool hasChanged();

  signals:
    void modified();

  protected slots:
    void entrySelected(QListViewItem* quickButtonEntry);
    void nameChanged(const QString& newName);
    void actionChanged(const QString& newAction);

  private:
    void setButtonsListView(const QStringList &buttonList);

  protected:
    bool m_newItemSelected;
    QStringList m_oldButtonList;

    QStringList currentButtonList();
};

#endif
