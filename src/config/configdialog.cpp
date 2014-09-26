/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2003 Benjamin C Meyer (ben+kdelibs at meyerhome dot net)
 *  Copyright (C) 2003 Waldo Bastian <bastian@kde.org>
 *  Copyright (C) 2004 Michael Brade <brade@kde.org>
 *
 *  Forked from KConfigDialog from KF5::ConfigWidgets to add tree support.
 *  Copyright (C) 2014 Eike Hein
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */
#include "configdialog.h"

#include <kconfigdialogmanager.h>
#include <kcoreconfigskeleton.h>
#include <kpagewidgetmodel.h>
#include <khelpclient.h>

#include <QDialogButtonBox>
#include <QIcon>
#include <QLayout>
#include <QPushButton>
#include <QtCore/QMap>
#include <QCoreApplication>
#include <QDesktopServices>

class ConfigDialog::ConfigDialogPrivate
{
public:
    ConfigDialogPrivate(ConfigDialog *q, const QString &name, KCoreConfigSkeleton *config)
        : q(q), shown(false), manager(0)
    {
        q->setObjectName(name);
        q->setWindowTitle(tr("Configure"));
        q->setFaceType(List);

        if (!name.isEmpty()) {
            openDialogs.insert(name, q);
        } else {
            QString genericName;
            genericName.sprintf("SettingsDialog-%p", static_cast<void *>(q));
            openDialogs.insert(genericName, q);
            q->setObjectName(genericName);
        }

        QDialogButtonBox *buttonBox = q->buttonBox();
        buttonBox->setStandardButtons(QDialogButtonBox::RestoreDefaults
                                      | QDialogButtonBox::Ok
                                      | QDialogButtonBox::Apply
                                      | QDialogButtonBox::Cancel
                                      | QDialogButtonBox::Help);
        connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), q, SLOT(updateSettings()));
        connect(buttonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked()), q, SLOT(updateSettings()));
        connect(buttonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked()), q, SLOT(_k_updateButtons()));
        connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), q, SLOT(updateWidgets()));
        connect(buttonBox->button(QDialogButtonBox::RestoreDefaults), SIGNAL(clicked()), q, SLOT(updateWidgetsDefault()));
        connect(buttonBox->button(QDialogButtonBox::RestoreDefaults), SIGNAL(clicked()), q, SLOT(_k_updateButtons()));
        connect(buttonBox->button(QDialogButtonBox::Help), SIGNAL(clicked()), q, SLOT(showHelp()));

        connect(q, SIGNAL(pageRemoved(KPageWidgetItem*)), q, SLOT(onPageRemoved(KPageWidgetItem*)));

        manager = new KConfigDialogManager(q, config);
        setupManagerConnections(manager);

        setApplyButtonEnabled(false);
    }

    KPageWidgetItem *addPageInternal(KPageWidgetItem *parent, QWidget *page, const QString &itemName,
                                     const QString &pixmapName, const QString &header);

    void setupManagerConnections(KConfigDialogManager *manager);
    void setApplyButtonEnabled(bool enabled);
    void setRestoreDefaultsButtonEnabled(bool enabled);

    void _k_updateButtons();
    void _k_settingsChangedSlot();

    ConfigDialog *q;
    QString mAnchor;
    QString mHelpApp;
    bool shown;
    KConfigDialogManager *manager;
    QMap<QWidget *, KConfigDialogManager *> managerForPage;

    /**
      * The list of existing dialogs.
     */
    static QHash<QString, ConfigDialog *> openDialogs;
};

QHash<QString, ConfigDialog *> ConfigDialog::ConfigDialogPrivate::openDialogs;

ConfigDialog::ConfigDialog(QWidget *parent, const QString &name,
                             KCoreConfigSkeleton *config) :
    KPageDialog(parent),
    d(new ConfigDialogPrivate(this, name, config))
{
}

ConfigDialog::~ConfigDialog()
{
    ConfigDialogPrivate::openDialogs.remove(objectName());
    delete d;
}

KPageWidgetItem *ConfigDialog::addPage(QWidget *page,
                                        const QString &itemName,
                                        const QString &pixmapName,
                                        const QString &header,
                                        bool manage)
{
    Q_ASSERT(page);
    if (!page) {
        return 0;
    }

    KPageWidgetItem *item = d->addPageInternal(0, page, itemName, pixmapName, header);
    if (manage) {
        d->manager->addWidget(page);
    }

    if (d->shown && manage) {
        // update the default button if the dialog is shown
        QPushButton *defaultButton = buttonBox()->button(QDialogButtonBox::RestoreDefaults);
        if (defaultButton) {
            bool is_default = defaultButton->isEnabled() && d->manager->isDefault();
            defaultButton->setEnabled(!is_default);
        }
    }
    return item;
}

KPageWidgetItem *ConfigDialog::konviAddSubPage(KPageWidgetItem *parent,
                                               QWidget *page,
                                               const QString &itemName,
                                               const QString &pixmapName,
                                               const QString &header,
                                               bool manage)
{
    Q_ASSERT(parent);
    if (!parent) {
        return 0;
    }

    Q_ASSERT(page);
    if (!page) {
        return 0;
    }

    KPageWidgetItem *item = d->addPageInternal(parent, page, itemName, pixmapName, header);
    if (manage) {
        d->manager->addWidget(page);
    }

    if (d->shown && manage) {
        // update the default button if the dialog is shown
        QPushButton *defaultButton = buttonBox()->button(QDialogButtonBox::RestoreDefaults);
        if (defaultButton) {
            bool is_default = defaultButton->isEnabled() && d->manager->isDefault();
            defaultButton->setEnabled(!is_default);
        }
    }
    return item;
}

KPageWidgetItem *ConfigDialog::addPage(QWidget *page,
                                        KCoreConfigSkeleton *config,
                                        const QString &itemName,
                                        const QString &pixmapName,
                                        const QString &header)
{
    Q_ASSERT(page);
    if (!page) {
        return 0;
    }

    KPageWidgetItem *item = d->addPageInternal(0, page, itemName, pixmapName, header);
    d->managerForPage[page] = new KConfigDialogManager(page, config);
    d->setupManagerConnections(d->managerForPage[page]);

    if (d->shown) {
        // update the default button if the dialog is shown
        QPushButton *defaultButton = buttonBox()->button(QDialogButtonBox::RestoreDefaults);
        if (defaultButton) {
            bool is_default = defaultButton->isEnabled() && d->managerForPage[page]->isDefault();
            defaultButton->setEnabled(!is_default);
        }
    }
    return item;
}

KPageWidgetItem *ConfigDialog::ConfigDialogPrivate::addPageInternal(KPageWidgetItem *parent,
                                                                    QWidget *page,
                                                                    const QString &itemName,
                                                                    const QString &pixmapName,
                                                                    const QString &header)
{
    QWidget *frame = new QWidget(q);
    QVBoxLayout *boxLayout = new QVBoxLayout(frame);
    boxLayout->setMargin(0);

    boxLayout->addWidget(page);
    KPageWidgetItem *item = new KPageWidgetItem(frame, itemName);
    item->setHeader(header);
    if (!pixmapName.isEmpty()) {
        item->setIcon(QIcon::fromTheme(pixmapName));
    }

    if (parent) {
        q->KPageDialog::addSubPage(parent, item);
    } else {
        q->KPageDialog::addPage(item);
    }

    return item;
}

void ConfigDialog::ConfigDialogPrivate::setupManagerConnections(KConfigDialogManager *manager)
{
    q->connect(manager, SIGNAL(settingsChanged()), q, SLOT(_k_settingsChangedSlot()));
    q->connect(manager, SIGNAL(widgetModified()), q, SLOT(_k_updateButtons()));

    QDialogButtonBox *buttonBox = q->buttonBox();
    q->connect(buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, manager, &KConfigDialogManager::updateSettings);
    q->connect(buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, manager, &KConfigDialogManager::updateSettings);
    q->connect(buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, manager, &KConfigDialogManager::updateWidgets);
    q->connect(buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, manager, &KConfigDialogManager::updateWidgetsDefault);
}

void ConfigDialog::ConfigDialogPrivate::setApplyButtonEnabled(bool enabled)
{
    QPushButton *applyButton = q->buttonBox()->button(QDialogButtonBox::Apply);
    if (applyButton) {
        applyButton->setEnabled(enabled);
    }
}

void ConfigDialog::ConfigDialogPrivate::setRestoreDefaultsButtonEnabled(bool enabled)
{
    QPushButton *restoreDefaultsButton = q->buttonBox()->button(QDialogButtonBox::RestoreDefaults);
    if (restoreDefaultsButton) {
        restoreDefaultsButton->setEnabled(enabled);
    }
}

void ConfigDialog::onPageRemoved(KPageWidgetItem *item)
{
    QMap<QWidget *, KConfigDialogManager *>::iterator j = d->managerForPage.begin();
    while (j != d->managerForPage.end()) {
        // there is a manager for this page, so remove it
        if (item->widget()->isAncestorOf(j.key())) {
            KConfigDialogManager *manager = j.value();
            d->managerForPage.erase(j);
            delete manager;
            d->_k_updateButtons();
            break;
        }
        ++j;
    }
}

ConfigDialog *ConfigDialog::exists(const QString &name)
{
    QHash<QString, ConfigDialog *>::const_iterator it = ConfigDialogPrivate::openDialogs.constFind(name);
    if (it != ConfigDialogPrivate::openDialogs.constEnd()) {
        return *it;
    }
    return 0;
}

bool ConfigDialog::showDialog(const QString &name)
{
    ConfigDialog *dialog = exists(name);
    if (dialog) {
        dialog->show();
    }
    return (dialog != NULL);
}

void ConfigDialog::ConfigDialogPrivate::_k_updateButtons()
{
    static bool only_once = false;
    if (only_once) {
        return;
    }
    only_once = true;

    QMap<QWidget *, KConfigDialogManager *>::iterator it;

    bool has_changed = manager->hasChanged() || q->hasChanged();
    for (it = managerForPage.begin();
            it != managerForPage.end() && !has_changed;
            ++it) {
        has_changed |= (*it)->hasChanged();
    }

    setApplyButtonEnabled(has_changed);

    bool is_default = manager->isDefault() && q->isDefault();
    for (it = managerForPage.begin();
            it != managerForPage.end() && is_default;
            ++it) {
        is_default &= (*it)->isDefault();
    }

    setRestoreDefaultsButtonEnabled(!is_default);

    emit q->widgetModified();
    only_once = false;
}

void ConfigDialog::ConfigDialogPrivate::_k_settingsChangedSlot()
{
    // Update the buttons
    _k_updateButtons();
    emit q->settingsChanged(q->objectName());
}

void ConfigDialog::showEvent(QShowEvent *e)
{
    if (!d->shown) {
        QMap<QWidget *, KConfigDialogManager *>::iterator it;

        updateWidgets();
        d->manager->updateWidgets();
        for (it = d->managerForPage.begin(); it != d->managerForPage.end(); ++it) {
            (*it)->updateWidgets();
        }

        bool has_changed = d->manager->hasChanged() || hasChanged();
        for (it = d->managerForPage.begin();
                it != d->managerForPage.end() && !has_changed;
                ++it) {
            has_changed |= (*it)->hasChanged();
        }

        d->setApplyButtonEnabled(has_changed);

        bool is_default = d->manager->isDefault() && isDefault();
        for (it = d->managerForPage.begin();
                it != d->managerForPage.end() && is_default;
                ++it) {
            is_default &= (*it)->isDefault();
        }

        d->setRestoreDefaultsButtonEnabled(!is_default);

        d->shown = true;
    }
    KPageDialog::showEvent(e);
}

void ConfigDialog::updateSettings()
{
}

void ConfigDialog::updateWidgets()
{
}

void ConfigDialog::updateWidgetsDefault()
{
}

bool ConfigDialog::hasChanged()
{
    return false;
}

bool ConfigDialog::isDefault()
{
    return true;
}

void ConfigDialog::updateButtons()
{
    d->_k_updateButtons();
}

void ConfigDialog::settingsChangedSlot()
{
    d->_k_settingsChangedSlot();
}

void ConfigDialog::setHelp(const QString &anchor, const QString &appname)
{
    d->mAnchor  = anchor;
    d->mHelpApp = appname;
}

void ConfigDialog::showHelp()
{
    KHelpClient::invokeHelp(d->mAnchor, d->mHelpApp);
}

#include "moc_configdialog.cpp"
