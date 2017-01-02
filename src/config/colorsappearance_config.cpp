#include "colorsappearance_config.h"

#include <QIcon>
#include <QInputDialog>
#include <QStandardPaths>
#include <QDebug>
#include <QSettings>
#include <QDir>

ColorsAppearance_Config::ColorsAppearance_Config(QWidget *parent, const char *name) : QWidget(parent)
{
    setObjectName(name);
    setupUi(this);

    saveThemeButton->setIcon(QIcon::fromTheme("document-save"));
    connect(saveThemeButton, &QToolButton::clicked, this, &ColorsAppearance_Config::saveTheme);

    loadThemes();
    connect(themeCombo, SIGNAL(activated(int)), this, SLOT(applyTheme(int)));
}

void ColorsAppearance_Config::saveTheme()
{
    bool ok = false;
    QString themeName = QInputDialog::getText(this, i18n("Save Color Theme"), i18n("Theme name:"),
                                              QLineEdit::Normal, themeCombo->currentText(), &ok);

    if(ok)
    {
        QString themePath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
                QStringLiteral("/konversation/colorthemes/");
        QString file = themePath + themeName + QStringLiteral (".theme");
        QSettings colorSettings(file, QSettings::IniFormat);
        colorSettings.setValue("KonversationTheme/Name", themeName);

        colorSettings.beginGroup("CustomColors");
        colorSettings.setValue("Action", kcfg_ActionMessageColor->color().name());
        colorSettings.setValue("ChannelMessage", kcfg_ChannelMessageColor->color().name());
        colorSettings.setValue("Hyperlink", kcfg_HyperlinkColor->color().name());
        colorSettings.setValue("ServerMessage", kcfg_ServerMessageColor->color().name());
        colorSettings.setValue("Background", kcfg_TextViewBackgroundColor->color().name());
        colorSettings.setValue("Backlog", kcfg_BacklogMessageColor->color().name());
        colorSettings.setValue("CommandMessage", kcfg_CommandMessageColor->color().name());
        colorSettings.setValue("QueryMessage", kcfg_QueryMessageColor->color().name());
        colorSettings.setValue("Timestamp", kcfg_TimeColor->color().name());
        colorSettings.setValue("AlternateBackground", kcfg_AlternateBackgroundColor->color().name());
        colorSettings.setValue("InputCustomColors", kcfg_InputFieldsBackgroundColor->isChecked());
        colorSettings.endGroup();

        colorSettings.beginGroup("NickColors");
        colorSettings.setValue("Enabled", kcfg_UseColoredNicks->isChecked());
        colorSettings.setValue("NickColor0", kcfg_NickColor0->color().name());
        colorSettings.setValue("NickColor1", kcfg_NickColor1->color().name());
        colorSettings.setValue("NickColor2", kcfg_NickColor2->color().name());
        colorSettings.setValue("NickColor3", kcfg_NickColor3->color().name());
        colorSettings.setValue("NickColor4", kcfg_NickColor4->color().name());
        colorSettings.setValue("NickColor5", kcfg_NickColor5->color().name());
        colorSettings.setValue("NickColor6", kcfg_NickColor6->color().name());
        colorSettings.setValue("NickColor7", kcfg_NickColor7->color().name());
        colorSettings.setValue("OwnNick", kcfg_NickColor8->color().name());
        colorSettings.endGroup();

        colorSettings.beginGroup("IrcColors");
        colorSettings.setValue("Enabled", kcfg_AllowColorCodes->isChecked());
        colorSettings.setValue("IRCColor0", kcfg_IrcColorCode0->color().name());
        colorSettings.setValue("IRCColor1", kcfg_IrcColorCode1->color().name());
        colorSettings.setValue("IRCColor2", kcfg_IrcColorCode2->color().name());
        colorSettings.setValue("IRCColor3", kcfg_IrcColorCode3->color().name());
        colorSettings.setValue("IRCColor4", kcfg_IrcColorCode4->color().name());
        colorSettings.setValue("IRCColor5", kcfg_IrcColorCode5->color().name());
        colorSettings.setValue("IRCColor6", kcfg_IrcColorCode6->color().name());
        colorSettings.setValue("IRCColor7", kcfg_IrcColorCode7->color().name());
        colorSettings.setValue("IRCColor8", kcfg_IrcColorCode8->color().name());
        colorSettings.setValue("IRCColor9", kcfg_IrcColorCode9->color().name());
        colorSettings.setValue("IRCColor10", kcfg_IrcColorCode10->color().name());
        colorSettings.setValue("IRCColor11", kcfg_IrcColorCode11->color().name());
        colorSettings.setValue("IRCColor12", kcfg_IrcColorCode12->color().name());
        colorSettings.setValue("IRCColor13", kcfg_IrcColorCode13->color().name());
        colorSettings.setValue("IRCColor14", kcfg_IrcColorCode14->color().name());
        colorSettings.setValue("IRCColor15", kcfg_IrcColorCode15->color().name());
        colorSettings.endGroup();


        //Update the theme combo box
        int index = themeCombo->findText(themeName);

        if (index < 0)
        {
            themeCombo->addItem(themeName, file);
            themeCombo->setCurrentText(themeName);
        }
        else
        {
            themeCombo->setItemData(index, file);
        }
    }
}

void ColorsAppearance_Config::loadThemes()
{
    QStringList paths = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation,
                                                  "konversation/colorthemes/",
                                                  QStandardPaths::LocateDirectory);
    themeCombo->clear();
    QDir::home();

    foreach(const QString& path, paths)
    {
        QDir dir(path, QStringLiteral("*.theme"), QDir::LocaleAware, QDir::Files);

        foreach(const QFileInfo& fileInfo, dir.entryInfoList())
        {
            if(fileInfo.exists())
            {
                QSettings theme(fileInfo.absoluteFilePath(), QSettings::IniFormat);
                QString themeName = theme.value("KonversationTheme/Name").toString();

                if(!themeName.isEmpty())
                    themeCombo->addItem(themeName, fileInfo.absoluteFilePath());
            }
        }
    }
}

void ColorsAppearance_Config::applyTheme(int index)
{
    if (index < 0)
        return;

    QString themePath = themeCombo->itemData(index).toString();
    QSettings colorSettings(themePath, QSettings::IniFormat);

    colorSettings.beginGroup("CustomColors");
    kcfg_ActionMessageColor->setColor(colorSettings.value("Action").toString());
    kcfg_ChannelMessageColor->setColor(colorSettings.value("ChannelMessage").toString());
    kcfg_HyperlinkColor->setColor(colorSettings.value("Hyperlink").toString());
    kcfg_ServerMessageColor->setColor(colorSettings.value("ServerMessage").toString());
    kcfg_TextViewBackgroundColor->setColor(colorSettings.value("Background").toString());
    kcfg_BacklogMessageColor->setColor(colorSettings.value("Backlog").toString());
    kcfg_CommandMessageColor->setColor(colorSettings.value("CommandMessage").toString());
    kcfg_QueryMessageColor->setColor(colorSettings.value("QueryMessage").toString());
    kcfg_TimeColor->setColor(colorSettings.value("Timestamp").toString());
    kcfg_AlternateBackgroundColor->setColor(colorSettings.value("AlternateBackground").toString());
    kcfg_InputFieldsBackgroundColor->setChecked(colorSettings.value("InputCustomColors").toBool());
    colorSettings.endGroup();

    colorSettings.beginGroup("NickColors");
    kcfg_UseColoredNicks->setChecked(colorSettings.value("Enabled").toBool());
    kcfg_NickColor0->setColor(colorSettings.value("NickColor0").toString());
    kcfg_NickColor1->setColor(colorSettings.value("NickColor1").toString());
    kcfg_NickColor2->setColor(colorSettings.value("NickColor2").toString());
    kcfg_NickColor3->setColor(colorSettings.value("NickColor3").toString());
    kcfg_NickColor4->setColor(colorSettings.value("NickColor4").toString());
    kcfg_NickColor5->setColor(colorSettings.value("NickColor5").toString());
    kcfg_NickColor6->setColor(colorSettings.value("NickColor6").toString());
    kcfg_NickColor7->setColor(colorSettings.value("NickColor7").toString());
    kcfg_NickColor8->setColor(colorSettings.value("OwnNick").toString());
    colorSettings.endGroup();

    colorSettings.beginGroup("IrcColors");
    kcfg_AllowColorCodes->setChecked(colorSettings.value("Enabled").toBool());
    kcfg_IrcColorCode0->setColor(colorSettings.value("IRCColor0").toString());
    kcfg_IrcColorCode1->setColor(colorSettings.value("IRCColor1").toString());
    kcfg_IrcColorCode2->setColor(colorSettings.value("IRCColor2").toString());
    kcfg_IrcColorCode3->setColor(colorSettings.value("IRCColor3").toString());
    kcfg_IrcColorCode4->setColor(colorSettings.value("IRCColor4").toString());
    kcfg_IrcColorCode5->setColor(colorSettings.value("IRCColor5").toString());
    kcfg_IrcColorCode6->setColor(colorSettings.value("IRCColor6").toString());
    kcfg_IrcColorCode7->setColor(colorSettings.value("IRCColor7").toString());
    kcfg_IrcColorCode8->setColor(colorSettings.value("IRCColor8").toString());
    kcfg_IrcColorCode9->setColor(colorSettings.value("IRCColor9").toString());
    kcfg_IrcColorCode10->setColor(colorSettings.value("IRCColor10").toString());
    kcfg_IrcColorCode11->setColor(colorSettings.value("IRCColor11").toString());
    kcfg_IrcColorCode12->setColor(colorSettings.value("IRCColor12").toString());
    kcfg_IrcColorCode13->setColor(colorSettings.value("IRCColor13").toString());
    kcfg_IrcColorCode14->setColor(colorSettings.value("IRCColor14").toString());
    kcfg_IrcColorCode15->setColor(colorSettings.value("IRCColor15").toString());
    colorSettings.endGroup();
}
