
#ifndef WARNINGS_CONFIG_H
#define WARNINGS_CONFIG_H

#include "warnings_preferencesui.h"

class KListView;
class QListViewItem;

class Warnings_Config : public Warnings_ConfigUI
{
    Q_OBJECT

public:
    Warnings_Config( QWidget* parent = 0, const char* name = 0, WFlags fl = 0 );
    ~Warnings_Config();
    QString flagNames;

public slots:
    virtual void languageChange();
    virtual void saveSettings();
    virtual void updateWidgets();
signals:
    void modified();
};

#endif // WARNINGS_CONFIG_H
