/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2006 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2006 John Tapsell <johnflux@gmail.com>
    SPDX-FileCopyrightText: 2006-2008 Eike Hein <hein@kde.org>
    SPDX-FileCopyrightText: 2010 Eli Mackenzie <argonel@gmail.com>
*/


#include "warnings_config.h"

#include <QHeaderView>

#include <ki18n_version.h>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KLazyLocalizedString>
#undef I18NC_NOOP
#define I18NC_NOOP kli18nc

static const int WarningNameRole = Qt::UserRole + 100;

Warnings_Config::Warnings_Config(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, fl)
{
    setObjectName(QString::fromLatin1(name));
    setupUi(this);

    dialogListView->header()->setSectionsClickable(false);
    dialogListView->header()->setSectionsMovable(false);

    loadSettings();

    connect(dialogListView, &QTreeWidget::itemChanged, this, &Warnings_Config::modified);
}

Warnings_Config::~Warnings_Config()
{
}

void Warnings_Config::restorePageToDefaults()
{
    bool changed=false;
    for (int i = 0; i < dialogListView->topLevelItemCount(); ++i)
    {
        QTreeWidgetItem *item = dialogListView->topLevelItem(i);
        if (item->checkState(0) == Qt::Unchecked)
        {
            item->setCheckState(0, Qt::Checked);
            changed=true;
        }
    }
    if(changed)
    {
        Q_EMIT modified();
    }
}

void Warnings_Config::saveSettings()
{
    KSharedConfigPtr config = KSharedConfig::openConfig();
    KConfigGroup grp = config->group(QStringLiteral("Notification Messages"));

    // prepare list
    QString warningsChecked;

    for (int i = 0; i < dialogListView->topLevelItemCount(); ++i)
    {
        QTreeWidgetItem *item = dialogListView->topLevelItem(i);
        const bool checked = item->checkState(0) == Qt::Checked;
        const QString warningName = item->data(0, WarningNameRole).toString();

        // save state of this item in hasChanged() list
        warningsChecked += checked ? QLatin1Char('1') : QLatin1Char('0');

        if (warningName == QLatin1String("LargePaste"))
        {
            if (checked)
            {
                grp.deleteEntry(warningName);
            }
            else
            {
                // Let's keep the old state if we got one.
                QString state = grp.readEntry(warningName, QString());

                if (!state.isEmpty())
                    grp.writeEntry(warningName, state);
                else
                    grp.writeEntry(warningName, "true");
            }
        }
        else if (warningName == QLatin1String("Invitation"))
        {
            // 0 == always ask
            // 1 == always join
            // 2 == always ignore

            if (checked)
            {
                grp.writeEntry(warningName, "0");
            }
            else
            {
                // We have two cases here, new unchecked and old already unchecked
                // If we already ignore the joining, keep it "2"
                // else newly unchecked, always join "1"
                QString state = grp.readEntry(warningName, QString());

                if (state == QLatin1Char('2'))
                    grp.writeEntry(warningName, state);
                else
                    grp.writeEntry(warningName, "1");
            }
        }
        else
        {
            grp.writeEntry(warningName, checked ? "1" : "0");
        }
    }

    // remember checkbox state for hasChanged()
    m_oldWarningsChecked=warningsChecked;
}

void Warnings_Config::loadSettings()
{
    // This table is very wide, on purpose, so that the nauseatingly constant context string is
    // out of the way. The problem is this:
    // The context string in I18NC_NOOP must always be first, and now that it has
    // semantic markers it is always very long. So, if you want to understand a string
    // definition, you must repeatedly look at the context string. A macro replacement for it
    // is not possible as such recent and complicated inventions are not supported by the tools
    // used to generate the message files.

    static const struct DefinitionItem
    {
        const char *flagName;
        const KLazyLocalizedString message;
    } warningDialogDefinitions[] = {
        { "Invitation",                                                                                 I18NC_NOOP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... a channel invitation is received"
        )},
        { "SaveLogfileNote",                                                                            I18NC_NOOP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... saving a log file would save only the visible portion"
        )},
        { "ClearLogfileQuestion",                                                                       I18NC_NOOP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... a log file is about to be deleted"
        )},
        { "CloseQueryAfterIgnore",                                                                      I18NC_NOOP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... an open query exists for a nickname that has just been marked as ignored"
        )},
        { "ReconnectWithDifferentServer",                                                               I18NC_NOOP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... switching a connection to a different server in the same network"
        )},
        { "ReuseExistingConnection",                                                                    I18NC_NOOP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... creating a new connection to an already connected network"
        )},
        { "QuitServerTab",                                                                              I18NC_NOOP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... closing a server tab"
        )},
        { "QuitChannelTab",                                                                             I18NC_NOOP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... closing a channel tab"
        )},
        { "QuitQueryTab",                                                                               I18NC_NOOP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... closing a query tab"
        )},
        { "QuitDCCChatTab",                                                                             I18NC_NOOP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... closing a DCC Chat tab"
        )},
        { "ChannelListNoServerSelected",                                                                I18NC_NOOP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... a source for the channel list cannot be determined from the current tab"
        )},
        { "HideMenuBarWarning",                                                                         I18NC_NOOP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... you have chosen to hide the menu bar"
        )},
        { "ChannelListWarning",                                                                         I18NC_NOOP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... a channel listing may cause disconnection due to the download size"
        )},
        { "LargePaste",                                                                                 I18NC_NOOP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... pasting large portions of text"
        )},
        { "systemtrayquitKonversation",                                                                 I18NC_NOOP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... quitting Konversation via the tray icon"
        )},
        { "IgnoreNick",                                                                                 I18NC_NOOP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... you have chosen to ignore a nickname"
        )},
        { "UnignoreNick",                                                                               I18NC_NOOP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... you have chosen to stop ignoring a nickname"
        )},
        { "QuitWithActiveDccTransfers",                                                                 I18NC_NOOP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... quitting Konversation while DCC file transfers are active"
        )},
        { "WarnEncodingConflict",                                                                       I18NC_NOOP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... some characters in a message are incompatible with the active encoding"
        )},
        { "HideOnCloseInfo",                                                                            I18NC_NOOP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... closing the window will minimize to the system tray"
        )}
    };

    dialogListView->clear();

    KSharedConfigPtr config = KSharedConfig::openConfig();
    KConfigGroup grp =  config->group(QStringLiteral("Notification Messages"));

    for (const auto& warningDialogDefinition : warningDialogDefinitions) {
        const QLatin1String flagName(warningDialogDefinition.flagName);
        const QString message = warningDialogDefinition.message.toString();

        auto *item = new QTreeWidgetItem(dialogListView);
        item->setText(0, message);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setData(0, WarningNameRole, flagName);

        if (flagName == QLatin1String("LargePaste"))
        {
            item->setCheckState(0, grp.readEntry(flagName, QString()).isEmpty() ? Qt::Checked : Qt::Unchecked);
        }
        else if (flagName == QLatin1String("Invitation"))
        {
            item->setCheckState(0, grp.readEntry(flagName, QStringLiteral("0")) == QLatin1Char('0') ? Qt::Checked : Qt::Unchecked);
        }
        else
        {
            item->setCheckState(0, grp.readEntry(flagName, true) ? Qt::Checked : Qt::Unchecked);
        }
    }

    dialogListView->sortItems(0, Qt::AscendingOrder);

    // remember checkbox state for hasChanged()
    m_oldWarningsChecked=currentWarningsChecked();
}

// get a list of checked/unchecked items for hasChanged()
QString Warnings_Config::currentWarningsChecked() const
{
    // prepare list
    QString newList;

    // get first checklist item
    for (int i = 0; i < dialogListView->topLevelItemCount(); ++i)
    {
        newList += (dialogListView->topLevelItem(i)->checkState(0) == Qt::Checked) ? QLatin1Char('1') : QLatin1Char('0');
    }
    // return list
    return newList;
}

bool Warnings_Config::hasChanged()
{
    return(m_oldWarningsChecked!=currentWarningsChecked());
}

// Sets the strings of the subwidgets using the current language.
void Warnings_Config::languageChange()
{
    loadSettings();
}

#include "moc_warnings_config.cpp"
