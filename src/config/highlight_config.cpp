/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2006 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2006 John Tapsell <johnflux@gmail.com>
*/

#include "highlight_config.h"
#include "highlightviewitem.h"
#include "application.h"
#include "sound.h"
#include "preferences.h"

#include <QDir>
#include <KSharedConfig>
#include <QStandardPaths>


Highlight_Config::Highlight_Config(QWidget* parent, const QString& name)
: QWidget(parent)
{
    setObjectName(name);
    setupUi(this);

    // reset flag to defined state (used to block signals when just selecting a new item)
    newItemSelected = false;

    loadSettings();

    soundPlayBtn->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-start")));
    soundURL->setWhatsThis(i18n("Select Sound File"));
    soundURL->setMimeTypeFilters({QStringLiteral("audio/x-wav"), QStringLiteral("audio/x-mp3"), QStringLiteral("application/ogg"), QStringLiteral("audio/x-adpcm")});

    // This code was copied from KNotifyWidget::openSoundDialog() (knotifydialog.cpp) [it's under LGPL v2]
    // find the first "sound"-resource that contains files
    QStringList soundDirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("konversation/sounds"));
    soundDirs += QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("sounds"), QStandardPaths::LocateDirectory);

    if (!soundDirs.isEmpty())
    {
        QDir dir;
        dir.setFilter( QDir::Files | QDir::Readable );
        for (const QString& soundDir : std::as_const(soundDirs)) {
            dir.setPath(soundDir);
            if ( dir.isReadable() && dir.count() > 2 )
            {
                soundURL->setStartDir(QUrl::fromLocalFile(soundDir));
                break;
            }
        }
    }
    // End copy

    connect(highlightListView, &QTreeWidget::currentItemChanged, this, &Highlight_Config::highlightSelected);
    connect(highlightListView, &QTreeWidget::itemChanged, this, &Highlight_Config::modified);
    connect(highlightListView, &HighlightTreeWidget::itemDropped, this, &Highlight_Config::modified);
    connect(patternInput, &KLineEdit::textChanged, this, &Highlight_Config::patternChanged);
    connect(enableNotificationsCheckbox, &QCheckBox::toggled, this, &Highlight_Config::notifyModeChanged);
    connect(patternColor, &KColorButton::changed, this, &Highlight_Config::colorChanged);
    connect(soundURL, &KUrlRequester::textChanged, this, &Highlight_Config::soundURLChanged);
    connect(soundPlayBtn, &QToolButton::clicked, this, &Highlight_Config::playSound);
    connect(autoTextInput, &KLineEdit::textChanged, this, &Highlight_Config::autoTextChanged);
    connect(chatWindowsInput, &KLineEdit::textChanged, this, &Highlight_Config::chatWindowsChanged);
    connect(newButton, &QPushButton::clicked, this, &Highlight_Config::addHighlight);
    connect(removeButton, &QPushButton::clicked, this, &Highlight_Config::removeHighlight);

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
        Q_EMIT modified();
    }
}

void Highlight_Config::loadSettings()
{
    highlightListView->clear();
    const auto highlightList = Preferences::highlightList();
    for (Highlight* currentHighlight : highlightList) {
        auto *item = new HighlightViewItem(highlightListView,currentHighlight);
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
        auto* highlightItem = dynamic_cast<HighlightViewItem*>(item);

        // tell all now emitted signals that we just clicked on a new item, so they should
        // not emit the modified() signal.
        newItemSelected = true;
        patternColor->setColor(highlightItem->getColor());
        patternInput->setText(highlightItem->getPattern());
        enableNotificationsCheckbox->setChecked(highlightItem->getNotify());
        soundURL->setUrl(highlightItem->getSoundURL());
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
    bool enabled = highlightListView->currentItem() != nullptr;
    // enable or disable edit widgets
    patternLabel->setEnabled(enabled);
    patternInput->setEnabled(enabled);
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
}

void Highlight_Config::patternChanged(const QString& newPattern)
{
    auto* item = dynamic_cast<HighlightViewItem*>(highlightListView->currentItem());

    if (!newItemSelected && item)
    {
        item->setPattern(newPattern);
        Q_EMIT modified();
    }
}

void Highlight_Config::notifyModeChanged(bool enabled)
{
    auto* item = dynamic_cast<HighlightViewItem*>(highlightListView->currentItem());

    if (!newItemSelected && item)
    {
        item->setNotify(enabled);
        Q_EMIT modified();
    }
}

void Highlight_Config::colorChanged(const QColor& newColor)
{
    auto* item = dynamic_cast<HighlightViewItem*>(highlightListView->currentItem());

    if (!newItemSelected && item)
    {
        item->setColor(newColor);
        Q_EMIT modified();
    }
}

void Highlight_Config::soundURLChanged()
{
    auto* item = dynamic_cast<HighlightViewItem*>(highlightListView->currentItem());

    if (!newItemSelected && item)
    {
        item->setSoundURL(soundURL->url());
        Q_EMIT modified();
    }
}

void Highlight_Config::autoTextChanged(const QString& newText)
{
    auto* item = dynamic_cast<HighlightViewItem*>(highlightListView->currentItem());

    if (!newItemSelected && item)
    {
        item->setAutoText(newText);
        Q_EMIT modified();
    }
}

void Highlight_Config::chatWindowsChanged(const QString& newChatWindows)
{
    auto* item = dynamic_cast<HighlightViewItem*>(highlightListView->currentItem());

    if (!newItemSelected && item)
    {
        item->setChatWindows(newChatWindows);
        Q_EMIT modified();
    }
}

void Highlight_Config::addHighlight()
{
    auto* newHighlight = new Highlight(i18n("New"), false, QColor("#ff0000"), QUrl(), QString(), QString(), true);

    auto* item = new HighlightViewItem(highlightListView, newHighlight);
    item->setFlags(item->flags() &~ Qt::ItemIsDropEnabled);
    highlightListView->setCurrentItem(item);
    patternInput->setFocus();
    patternInput->selectAll();
    Q_EMIT modified();
}

void Highlight_Config::removeHighlight()
{
    auto* item = dynamic_cast<HighlightViewItem*>(highlightListView->currentItem());

    if (item)
    {
        delete item;

        item = dynamic_cast<HighlightViewItem*>(highlightListView->currentItem());

        if (item)
        {
            highlightListView->setCurrentItem(item);
        }

        Q_EMIT modified();
    }
    updateButtons();
}

QList<Highlight*> Highlight_Config::getHighlightList() const
{
    QList<Highlight*> newList;

    auto* item = dynamic_cast<HighlightViewItem*>(highlightListView->topLevelItem(0));
    while (item)
    {
        newList.append(new Highlight(item->getPattern(), item->getRegExp(), item->getColor(),
            item->getSoundURL(), item->getAutoText(), item->getChatWindows(), item->getNotify()));
        item = dynamic_cast<HighlightViewItem*>(highlightListView->itemBelow(item));
    }

    return newList;
}

QStringList Highlight_Config::currentHighlightList() const
{
    QStringList newList;

    auto* item = dynamic_cast<HighlightViewItem*>(highlightListView->topLevelItem(0));
    while (item)
    {
        newList.append(item->getPattern() + QLatin1Char(item->getRegExp() ? '1' : '0') + item->getColor().name() +
            item->getSoundURL().url() + item->getAutoText() + item->getChatWindows() + QString::number(item->getNotify()));
        item = dynamic_cast<HighlightViewItem*>(highlightListView->itemBelow(item));
    }

    return newList;
}

void Highlight_Config::playSound()
{
    Application *konvApp = Application::instance();
    konvApp->sound()->play(soundURL->url());
}

void Highlight_Config::saveSettings()
{
    KSharedConfigPtr config = KSharedConfig::openConfig();

    // Write all highlight entries
    const QList<Highlight*> hiList = getHighlightList();
    int i = 0;
    for (Highlight* hl : hiList) {
        KConfigGroup grp = config->group(QStringLiteral("Highlight%1").arg(i));
        grp.writeEntry("Pattern", hl->getPattern());
        grp.writeEntry("RegExp", hl->getRegExp());
        grp.writeEntry("Color", hl->getColor());
        grp.writePathEntry("Sound", hl->getSoundURL().url());
        grp.writeEntry("AutoText", hl->getAutoText());
        grp.writeEntry("ChatWindows", hl->getChatWindows());
        grp.writeEntry("Notify", hl->getNotify());
        i++;
    }

    Preferences::setHighlightList(hiList);

    // Remove unused entries...
    while (config->hasGroup(QStringLiteral("Highlight%1").arg(i)))
    {
        config->deleteGroup(QStringLiteral("Highlight%1").arg(i));
        i++;
    }

    // remember current list for hasChanged()
    m_oldHighlightList=currentHighlightList();
}

#include "moc_highlight_config.cpp"
