/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2006 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2006 John Tapsell <johnflux@gmail.com>
  Copyright (C) 2006-2008 Eike Hein <hein@kde.org>
  Copyright (C) 2010 Eli Mackenzie <argonel@gmail.com>
*/


#include "warnings_config.h"

#include <QHeaderView>


static const int WarningNameRole = Qt::UserRole + 100;

Warnings_Config::Warnings_Config(QWidget* parent, const char* name, Qt::WFlags fl)
    : QWidget(parent, fl)
{
    setObjectName(QString::fromLatin1(name));
    setupUi(this);

    dialogListView->header()->setClickable(false);
    dialogListView->header()->setMovable(false);

    loadSettings();

    connect(dialogListView, SIGNAL(itemChanged(QTreeWidgetItem*,int)), SIGNAL(modified()));
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
        emit modified();
    }
}

void Warnings_Config::saveSettings()
{
    KSharedConfigPtr config = KGlobal::config();
    KConfigGroup grp = config->group("Notification Messages");

    // prepare list
    QString warningsChecked;

    for (int i = 0; i < dialogListView->topLevelItemCount(); ++i)
    {
        QTreeWidgetItem *item = dialogListView->topLevelItem(i);
        const bool checked = item->checkState(0) == Qt::Checked;
        const QString warningName = item->data(0, WarningNameRole).toString();

        // save state of this item in hasChanged() list
        warningsChecked += checked ? "1" : "0";

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
            if (checked)
            {
                grp.writeEntry(warningName, "0");
            }
            else
            {
                // Let's keep the old state if we got one, or join if
                // there isn't an old state.
                QString state = grp.readEntry(warningName, QString());

                if (!state.isEmpty())
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
    // The context string in I18N_NOOP2_NOSTRIP must always be first, and now that it has
    // semantic markers it is always very long. So, if you want to understand a string
    // definition, you must repeatedly look at the context string. A macro replacement for it
    // is not possible as such recent and complicated inventions are not supported by the tools
    // used to generate the message files.

    static const struct DefinitionItem
    {
        const char *flagName;
        const char *context;
        const char *message;
    } warningDialogDefinitions[] = {
        { "Invitation",                                                                                 I18N_NOOP2_NOSTRIP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... a channel invitation is received"
        )},
        { "SaveLogfileNote",                                                                            I18N_NOOP2_NOSTRIP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... saving a log file would save only the visible portion"
        )},
        { "ClearLogfileQuestion",                                                                       I18N_NOOP2_NOSTRIP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... a log file is about to be deleted"
        )},
        { "CloseQueryAfterIgnore",                                                                      I18N_NOOP2_NOSTRIP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... an open query exists for a nickname that has just been marked as ignored"
        )},
        { "ReconnectWithDifferentServer",                                                               I18N_NOOP2_NOSTRIP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... switching a connection to a different server in the same network"
        )},
        { "ReuseExistingConnection",                                                                    I18N_NOOP2_NOSTRIP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... creating a new connection to an already connected network"
        )},
        { "QuitServerTab",                                                                              I18N_NOOP2_NOSTRIP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... closing a server tab"
        )},
        { "QuitChannelTab",                                                                             I18N_NOOP2_NOSTRIP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... closing a channel tab"
        )},
        { "QuitQueryTab",                                                                               I18N_NOOP2_NOSTRIP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... closing a query tab"
        )},
        { "QuitDCCChatTab",                                                                             I18N_NOOP2_NOSTRIP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... closing a DCC Chat tab"
        )},
        { "ChannelListNoServerSelected",                                                                I18N_NOOP2_NOSTRIP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... a source for the channel list cannot be determined from the current tab"
        )},
        { "HideMenuBarWarning",                                                                         I18N_NOOP2_NOSTRIP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... you have chosen to hide the menu bar"
        )},
        { "ChannelListWarning",                                                                         I18N_NOOP2_NOSTRIP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... a channel listing may cause disconnection due to the download size"
        )},
        { "LargePaste",                                                                                 I18N_NOOP2_NOSTRIP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... pasting large portions of text"
        )},
        { "systemtrayquitKonversation",                                                                 I18N_NOOP2_NOSTRIP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... quitting Konversation via the tray icon"
        )},
        { "IgnoreNick",                                                                                 I18N_NOOP2_NOSTRIP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... you have chosen to ignore a nickname"
        )},
        { "UnignoreNick",                                                                               I18N_NOOP2_NOSTRIP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... you have chosen to stop ignoring a nickname"
        )},
        { "QuitWithActiveDccTransfers",                                                                 I18N_NOOP2_NOSTRIP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... quitting Konversation while DCC file transfers are active"
        )},
        { "WarnEncodingConflict",                                                                       I18N_NOOP2_NOSTRIP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... some characters in a message are incompatible with the active encoding"
        )},
        { "HideOnCloseInfo",                                                                            I18N_NOOP2_NOSTRIP("@item:inlistbox Checkbox item, determines whether warning dialog is shown; concludes sentence \"Show a warning dialog when...\"",
          "... closing the window will minimize to the system tray"
        )}
    };
    static const int definitionsCount = sizeof(warningDialogDefinitions) / sizeof(warningDialogDefinitions[0]);

    dialogListView->clear();

    KSharedConfigPtr config = KGlobal::config();
    KConfigGroup grp =  config->group("Notification Messages");

    for (int i = 0; i < definitionsCount; ++i)
    {
        const QLatin1String flagName(warningDialogDefinitions[i].flagName);
        const char * const message(warningDialogDefinitions[i].message);
        const char * const ctx(warningDialogDefinitions[i].context);

        QTreeWidgetItem *item = new QTreeWidgetItem(dialogListView);
        item->setText(0, i18nc(ctx, message));
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setData(0, WarningNameRole, flagName);

        if (flagName == QLatin1String("LargePaste"))
        {
            item->setCheckState(0, grp.readEntry(flagName, QString()).isEmpty() ? Qt::Checked : Qt::Unchecked);
        }
        else if (flagName == QLatin1String("Invitation"))
        {
            item->setCheckState(0, grp.readEntry(flagName, QString()) == "0" ? Qt::Checked : Qt::Unchecked);
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
QString Warnings_Config::currentWarningsChecked()
{
    // prepare list
    QString newList;

    // get first checklist item
    for (int i = 0; i < dialogListView->topLevelItemCount(); ++i)
    {
        newList += dialogListView->topLevelItem(i)->checkState(0) == Qt::Checked ? "1" : "0";
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

#include "warnings_config.moc"
