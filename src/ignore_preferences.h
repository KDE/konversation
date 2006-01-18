
#ifndef IGNORE_CONFIG_H
#define IGNORE_CONFIG_H

#include "ignore_preferencesui.h"
#include "konvisettingspage.h"
#include <qptrlist.h>


class Ignore;
class Ignore_Config : public Ignore_ConfigUI, public KonviSettingsPage
{
    Q_OBJECT

public:
    Ignore_Config( QWidget* parent = 0, const char* name = 0, WFlags fl = 0 );
    ~Ignore_Config();
    QString flagNames;

    virtual void restorePageToDefaults();
    virtual void saveSettings();
    virtual void loadSettings();

    virtual bool hasChanged() { return false; }; // FIXME

private:
    QPtrList<Ignore> getIgnoreList();
    void updateEnabledness();

public slots:
    virtual void languageChange();

protected slots:
    void newIgnore();
    void removeIgnore();
    void flagCheckboxChanged();
    void select(QListViewItem* item);
    void removeAllIgnore();
signals:
    void modified();
};

#endif // IGNORE_CONFIG_H
