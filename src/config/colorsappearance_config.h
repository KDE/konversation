#ifndef COLORSAPPEARANCE_CONFIG_H
#define COLORSAPPEARANCE_CONFIG_H

#include "ui_colorsappearance_config.h"

#include <QWidget>

class ColorsAppearance_Config : public QWidget, private Ui::ColorsAppearance_Config
{
    Q_OBJECT
public:
    explicit ColorsAppearance_Config(QWidget *parent, const char *name = 0);

private slots:
    void saveTheme();
    void applyTheme(int index);

private:
    void loadThemes();
};

#endif // COLORSAPPEARANCE_CONFIG_H
