
#ifndef WARNINGS_CONFIG_H
#define WARNINGS_CONFIG_H

#include "warnings_preferencesui.h"
#include "konvisettingspage.h"

class KListView;
class QListViewItem;

class Warnings_Config : public Warnings_ConfigUI, public KonviSettingsPage
{
    Q_OBJECT

public:
    Warnings_Config( QWidget* parent = 0, const char* name = 0, WFlags fl = 0 );
    ~Warnings_Config();

    virtual void restorePageToDefaults();
    virtual void saveSettings();
    virtual void loadSettings();

    virtual bool hasChanged();

public slots:
    virtual void languageChange();

protected:
    QString currentWarningsChecked(); // for hasChanged()

    QString m_oldWarningsChecked;     // for hasChanged()

signals:
    void modified();
};

#endif // WARNINGS_CONFIG_H
