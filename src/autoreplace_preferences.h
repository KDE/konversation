//
// C++ Interface: autoreplaceconfigcontroller
//
// Description:
//
//
// Author: Dario Abatianni <eisfuchs@tigress.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef AUTOREPLACECONFIG_H
#define AUTOREPLACECONFIG_H

#include "autoreplace_preferencesui.h"
#include "konvisettingspage.h"

/**
  @author Dario Abatianni <eisfuchs@tigress.com>
*/

class Autoreplace_Config : public Autoreplace_ConfigUI, public KonviSettingsPage
{
  Q_OBJECT

  public:
    Autoreplace_Config(QWidget* parent, const char* name=NULL);
    ~Autoreplace_Config();

    virtual void saveSettings();
    virtual void loadSettings();
    virtual void restorePageToDefaults();

    virtual bool hasChanged();

  signals:
    void modified();

  protected slots:
    void entrySelected(QListViewItem* autoreplaceEntry);
    void directionChanged(int newDirection);
    void patternChanged(const QString& newPattern);
    void replacementChanged(const QString& newReplacement);
    void addEntry();
    void removeEntry();

  protected:
    void setAutoreplaceListView(const QStringList &autoreplaceList);

    bool m_newItemSelected;

    QStringList m_oldAutoreplaceList;
    QStringList currentAutoreplaceList();
};

#endif
