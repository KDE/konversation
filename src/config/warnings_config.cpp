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
*/


#include "warnings_config.h"

#include <QHeaderView>


static const int WarningNameRole = Qt::UserRole + 100;

Warnings_Config::Warnings_Config( QWidget* parent, const char* name, Qt::WFlags fl )
    : QWidget(parent, fl)
{
  setObjectName(QString::fromLatin1(name));
  setupUi(this);

  dialogListView->header()->setClickable(false);
  dialogListView->header()->setMovable(false);

  loadSettings();

  connect(dialogListView, SIGNAL(itemChanged(QTreeWidgetItem *, int)), this, SIGNAL(modified()));
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
    if (item->checkState(0) == Qt::Unchecked) {
      item->setCheckState(0, Qt::Checked);
      changed=true;
    }
  }
  if(changed) {
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

    if (warningName == "LargePaste" || warningName == "Invitation")
    {
        if (checked)
        {
            grp.writeEntry(warningName, 1);
        }
        else
        {
            QString state = grp.readEntry(warningName, QString());

            if (!state.isEmpty() && (state == "yes" || state == "no"))
                grp.writeEntry(warningName, state);
            else
                grp.writeEntry(warningName, "yes");
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
  QStringList dialogDefinitions;
  QString flagNames = "Invitation,SaveLogfileNote,ClearLogfileQuestion,CloseQueryAfterIgnore,ReconnectWithDifferentServer,ReuseExistingConnection,QuitServerTab,QuitChannelTab,QuitQueryTab,ChannelListNoServerSelected,HideMenuBarWarning,ChannelListWarning,LargePaste,systemtrayquitKonversation,IgnoreNick,UnignoreNick,QuitWithActiveDccTransfers";
  dialogDefinitions.append(i18n("Automatically join channel on invite"));
  dialogDefinitions.append(i18n("Notice that saving logfiles will save whole file"));
  dialogDefinitions.append(i18n("Ask before deleting logfile contents"));
  dialogDefinitions.append(i18n("Ask about closing queries after ignoring the nickname"));
  dialogDefinitions.append(i18n("Ask before switching a connection to a network to a different server"));
  dialogDefinitions.append(i18n("Ask before creating another connection to the same network or server"));
  dialogDefinitions.append(i18n("Close server tab"));
  dialogDefinitions.append(i18n("Close channel tab"));
  dialogDefinitions.append(i18n("Close query tab"));
  dialogDefinitions.append(i18n("The channel list can only be opened from server-aware tabs"));
  dialogDefinitions.append(i18n("Warning on hiding the main window menu"));
  dialogDefinitions.append(i18n("Warning on high traffic with channel list"));
  dialogDefinitions.append(i18n("Warning on pasting large portions of text"));
  dialogDefinitions.append(i18n("Warning on quitting Konversation"));
  dialogDefinitions.append(i18n("Ignore"));
  dialogDefinitions.append(i18n("Unignore"));
  dialogDefinitions.append(i18n("Warn before quitting with active DCC file transfers"));
  QTreeWidgetItem *item;
  dialogListView->clear();

  KSharedConfigPtr config = KGlobal::config();
  KConfigGroup grp =  config->group("Notification Messages");
  QString flagName;
  for (int i = 0; i < dialogDefinitions.count(); ++i)
  {
    item = new QTreeWidgetItem(dialogListView);
    item->setText(0, dialogDefinitions[i]);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    flagName = flagNames.section(',',i,i);
    item->setData(0, WarningNameRole, flagName);

    if (flagName == "LargePaste" || flagName == "Invitation")
    {
        QString state = grp.readEntry(flagName, QString());

        if (state == "yes" || state == "no")
            item->setCheckState(0, Qt::Unchecked);
        else
            item->setCheckState(0, Qt::Checked);
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

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void Warnings_Config::languageChange()
{
  loadSettings();
}

#include "warnings_config.moc"
