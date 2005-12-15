
#ifndef IGNORE_CONFIG_H
#define IGNORE_CONFIG_H

#include "ignore_preferencesui.h"

class Ignore_Config : public Ignore_ConfigUI
{
    Q_OBJECT

public:
    Ignore_Config( QWidget* parent = 0, const char* name = 0, WFlags fl = 0 );
    ~Ignore_Config();
    QString flagNames;

public slots:
    virtual void languageChange();
    virtual void saveSettings();
    virtual void updateWidgets();
protected slots:
    virtual void newIgnore();
    virtual void removeIgnore();
signals:
    void modified();
};

#endif // IGNORE_CONFIG_H
