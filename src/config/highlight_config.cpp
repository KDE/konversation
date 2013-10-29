/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2006 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2006 John Tapsell <johnflux@gmail.com>
*/

#include "highlight_config.h"
#include "highlightviewitem.h"
#include "application.h"
#include "sound.h"
#include "preferences.h"

#include <QDir>
#include <KStandardDirs>
#include <KFileDialog>



#include <KGlobal>
#include <kparts/componentfactory.h>
#include <kregexpeditorinterface.h>


Highlight_Config::Highlight_Config(QWidget* parent, const char* name)
: QWidget(parent)
{
    setObjectName(name);
    setupUi(this);

    // reset flag to defined state (used to block signals when just selecting a new item)
    newItemSelected = false;

    loadSettings();

    soundPlayBtn->setIcon(KIcon("media-playback-start"));
    soundURL->setWhatsThis(i18n("Select Sound File"));

    // This code was copied from KNotifyWidget::openSoundDialog() (knotifydialog.cpp) [it's under LGPL v2]
    // find the first "sound"-resource that contains files
    QStringList soundDirs = KGlobal::dirs()->findDirs("data", "konversation/sounds");
    soundDirs += KGlobal::dirs()->resourceDirs( "sound" );

    if (!soundDirs.isEmpty())
    {
        KUrl url;
        QDir dir;
        dir.setFilter( QDir::Files | QDir::Readable );
        QStringList::ConstIterator it = soundDirs.constBegin();
        while ( it != soundDirs.constEnd() )
        {
            dir = *it;
            if ( dir.isReadable() && dir.count() > 2 )
            {
                url.setPath( *it );
                soundURL->fileDialog()->setUrl( url );
                break;
            }
            ++it;
        }
    }
    // End copy

    connect(highlightListView, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT (highlightSelected(QTreeWidgetItem*)));
    connect(patternInput, SIGNAL(textChanged(QString)), this, SLOT (patternChanged(QString)));
    connect(enableNotificationsCheckbox, SIGNAL(toggled(bool)), this, SLOT(notifyModeChanged(bool)));
    connect(patternButton, SIGNAL(clicked()), this, SLOT(regExpButtonClicked()));
    connect(patternColor, SIGNAL(changed(QColor)), this, SLOT (colorChanged(QColor)));
    connect(soundURL, SIGNAL(textChanged(QString)), this, SLOT(soundURLChanged(QString)));
    connect(soundPlayBtn, SIGNAL(clicked()), this, SLOT(playSound()));
    connect(autoTextInput, SIGNAL(textChanged(QString)), this, SLOT (autoTextChanged(QString)));
    connect(chatWindowsInput, SIGNAL(textChanged(QString)), this, SLOT (chatWindowsChanged(QString)));
    connect(newButton, SIGNAL(clicked()), this, SLOT (addHighlight()));
    connect(removeButton, SIGNAL(clicked()), this, SLOT(removeHighlight()));

    updateButtons();
}

Highlight_Config::~Highlight_Config()
{
}

void Highlight_Config::restorePageToDefaults()
{
    if (highlightListView->topLevelItemCount() != 0)
    {
        highlightListView->clear();
        emit modified();
    }
}

void Highlight_Config::loadSettings()
{
    highlightListView->clear();
    foreach (Highlight* currentHighlight, Preferences::highlightList())
    {
        HighlightViewItem *item = new HighlightViewItem(highlightListView,currentHighlight);
        item->setFlags(item->flags() &~ Qt::ItemIsDropEnabled);
    }

    highlightListView->setCurrentItem(highlightListView->topLevelItem(0));

    // remember current list for hasChanged()
    m_oldHighlightList=currentHighlightList();
}

bool Highlight_Config::hasChanged()
{
    return (m_oldHighlightList!=currentHighlightList());
}

// Slots:

void Highlight_Config::highlightSelected(QTreeWidgetItem* item)
{
    // check if there was a widget selected at all
    if (item)
    {
        // make a highlight item out of the generic qlistviewitem
        HighlightViewItem* highlightItem = static_cast<HighlightViewItem*>(item);

        // tell all now emitted signals that we just clicked on a new item, so they should
        // not emit the modified() signal.
        newItemSelected = true;
        patternColor->setColor(highlightItem->getColor());
        patternInput->setText(highlightItem->getPattern());
        enableNotificationsCheckbox->setChecked(highlightItem->getNotify());
        soundURL->setUrl(highlightItem->getSoundURL().prettyUrl());
        autoTextInput->setText(highlightItem->getAutoText());
        chatWindowsInput->setText(highlightItem->getChatWindows());
        // all signals will now emit the modified() signal again
        newItemSelected = false;
        // remember to enable all edit widgets
    }
    updateButtons();
}

void Highlight_Config::updateButtons()
{
    bool enabled = highlightListView->currentItem() != NULL;
    // is the kregexpeditor installed?
    bool installed = false;
    // it does not make sense to port / enable this since KRegExpEditor is in a very bad shape. just keep this
    // code here because it will probably help at a later point to port it when KRegExpEditor is again usable.
    // 2009-02-06, uwolfer
#if 0
    !KTrader::self()->query("KRegExpEditor/KRegExpEditor").isEmpty();
#endif
    // enable or disable edit widgets
    patternLabel->setEnabled(enabled);
    patternInput->setEnabled(enabled);
    patternButton->setEnabled(enabled && installed);
    colorLabel->setEnabled(enabled);
    patternColor->setEnabled(enabled);
    enableNotificationsLabel->setEnabled(enabled);
    enableNotificationsCheckbox->setEnabled(enabled);
    soundURL->setEnabled(enabled);
    soundLabel->setEnabled(enabled);
    soundPlayBtn->setEnabled(enabled);
    autoTextLabel->setEnabled(enabled);
    autoTextInput->setEnabled(enabled);
    chatWindowsLabel->setEnabled(enabled);
    chatWindowsInput->setEnabled(enabled);

    if (installed)
    {
        patternButton->setStatusTip(i18n("Click to run Regular Expression Editor (KRegExpEditor)"));
    }
    else
    {
        patternButton->setVisible(false);
        patternButton->setStatusTip(i18n("The Regular Expression Editor (KRegExpEditor) is not installed"));
    }
}

void Highlight_Config::patternChanged(const QString& newPattern)
{
    HighlightViewItem* item = static_cast<HighlightViewItem*>(highlightListView->currentItem());

    if (!newItemSelected && item)
    {
        item->setPattern(newPattern);
        emit modified();
    }
}

void Highlight_Config::notifyModeChanged(bool enabled)
{
    HighlightViewItem* item = static_cast<HighlightViewItem*>(highlightListView->currentItem());

    if (!newItemSelected && item)
    {
        item->setNotify(enabled);
        emit modified();
    }
}

void Highlight_Config::regExpButtonClicked()
{
    // see note above about KRegExpEditor
#if 0
    QDialog *editorDialog = KParts::ComponentFactory::createInstanceFromQuery<QDialog>("KRegExpEditor/KRegExpEditor");
    if (editorDialog)
    {
        // kdeutils was installed, so the dialog was found.  Fetch the editor interface.
        KRegExpEditorInterface *reEditor = static_cast<KRegExpEditorInterface *>(editorDialog->qt_cast("KRegExpEditorInterface"));
        Q_ASSERT(reEditor); // This should not fail!// now use the editor.
        reEditor->setRegExp(patternInput->text());
        int dlgResult = editorDialog->exec();
        if (dlgResult == QDialog::Accepted)
        {
            QString re = reEditor->regExp();
            patternInput->setText(re);
            HighlightViewItem* item = static_cast<HighlightViewItem*>(highlightListView->currentItem());
            if (item)
            {
                item->setPattern(re);
            }
        }
        delete editorDialog;
    }
#endif
}

void Highlight_Config::colorChanged(const QColor& newColor)
{
    HighlightViewItem* item = static_cast<HighlightViewItem*>(highlightListView->currentItem());

    if (!newItemSelected && item)
    {
        item->setColor(newColor);
        emit modified();
    }
}

void Highlight_Config::soundURLChanged(const QString& newURL)
{
    HighlightViewItem* item = static_cast<HighlightViewItem*>(highlightListView->currentItem());

    if (!newItemSelected && item)
    {
        item->setSoundURL(KUrl(newURL));
        emit modified();
    }
}

void Highlight_Config::autoTextChanged(const QString& newText)
{
    HighlightViewItem* item = static_cast<HighlightViewItem*>(highlightListView->currentItem());

    if (!newItemSelected && item)
    {
        item->setAutoText(newText);
        emit modified();
    }
}

void Highlight_Config::chatWindowsChanged(const QString& newChatWindows)
{
    HighlightViewItem* item = static_cast<HighlightViewItem*>(highlightListView->currentItem());

    if (!newItemSelected && item)
    {
        item->setChatWindows(newChatWindows);
        emit modified();
    }
}

void Highlight_Config::addHighlight()
{
    Highlight* newHighlight = new Highlight(i18n("New"), false, QColor("#ff0000"), KUrl(), QString(), QString(), true);

    HighlightViewItem* item = new HighlightViewItem(highlightListView, newHighlight);
    item->setFlags(item->flags() &~ Qt::ItemIsDropEnabled);
    highlightListView->setCurrentItem(item);
    patternInput->setFocus();
    patternInput->selectAll();
    emit modified();
}

void Highlight_Config::removeHighlight()
{
    HighlightViewItem* item = static_cast<HighlightViewItem*>(highlightListView->currentItem());

    if (item)
    {
        delete item;

        item = static_cast<HighlightViewItem*>(highlightListView->currentItem());

        if (item)
        {
            highlightListView->setCurrentItem(item);
        }

        emit modified();
    }
    updateButtons();
}

QList<Highlight*> Highlight_Config::getHighlightList()
{
    QList<Highlight*> newList;

    HighlightViewItem* item = static_cast<HighlightViewItem*>(highlightListView->topLevelItem(0));
    while (item)
    {
        newList.append(new Highlight(item->getPattern(), item->getRegExp(), item->getColor(),
            item->getSoundURL(), item->getAutoText(), item->getChatWindows(), item->getNotify()));
        item = static_cast<HighlightViewItem*>(highlightListView->itemBelow(item));
    }

    return newList;
}

QStringList Highlight_Config::currentHighlightList()
{
    QStringList newList;

    HighlightViewItem* item = static_cast<HighlightViewItem*>(highlightListView->topLevelItem(0));
    while (item)
    {
        newList.append(item->getPattern() + QString(item->getRegExp()) + item->getColor().name() +
            item->getSoundURL().url() + item->getAutoText() + item->getChatWindows() + QString::number(item->getNotify()));
        item = static_cast<HighlightViewItem*>(highlightListView->itemBelow(item));
    }

    return newList;
}

void Highlight_Config::playSound()
{
    Application *konvApp = static_cast<Application *>(KApplication::kApplication());
    konvApp->sound()->play(KUrl(soundURL->url()));
}

void Highlight_Config::saveSettings()
{
    KSharedConfigPtr config = KGlobal::config();

    // Write all highlight entries
    QList<Highlight*> hiList = getHighlightList();
    int i = 0;
    foreach (Highlight* hl, hiList)
    {
        KConfigGroup grp = config->group(QString("Highlight%1").arg(i));
        grp.writeEntry("Pattern", hl->getPattern());
        grp.writeEntry("RegExp", hl->getRegExp());
        grp.writeEntry("Color", hl->getColor());
        grp.writePathEntry("Sound", hl->getSoundURL().prettyUrl());
        grp.writeEntry("AutoText", hl->getAutoText());
        grp.writeEntry("ChatWindows", hl->getChatWindows());
        grp.writeEntry("Notify", hl->getNotify());
        i++;
    }

    Preferences::setHighlightList(hiList);

    // Remove unused entries...
    while (config->hasGroup(QString("Highlight%1").arg(i)))
    {
        config->deleteGroup(QString("Highlight%1").arg(i));
        i++;
    }

    // remember current list for hasChanged()
    m_oldHighlightList=currentHighlightList();
}

#include "highlight_config.moc"
