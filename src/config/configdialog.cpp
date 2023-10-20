/*
    SPDX-License-Identifier: LGPL-2.0-or-later

    SPDX-FileCopyrightText: 2003 Benjamin C Meyer (ben+kdelibs at meyerhome dot net)
    SPDX-FileCopyrightText: 2003 Waldo Bastian <bastian@kde.org>
    SPDX-FileCopyrightText: 2004 Michael Brade <brade@kde.org>

    Forked from KConfigDialog from KF5::ConfigWidgets to add tree support.
    SPDX-FileCopyrightText: 2014 Eike Hein
*/
#include "configdialog.h"

#include <KConfigDialogManager>
#include <KCoreConfigSkeleton>
#include <KHelpClient>

#include <QApplication>
#include <QScreen>
#include <QDialogButtonBox>
#include <QIcon>
#include <QPushButton>
#include <QMap>
#include <QScrollArea>
#include <QVBoxLayout>

#include <algorithm>

class ConfigDialog::ConfigDialogPrivate
{
public:
    ConfigDialogPrivate(ConfigDialog *q, const QString &name, KCoreConfigSkeleton *config)
        : q(q), shown(false), manager(nullptr)
    {
        q->setObjectName(name);
        q->setWindowTitle(tr("Configure"));
        q->setFaceType(List);

        if (!name.isEmpty()) {
            openDialogs.insert(name, q);
        } else {
            const QString genericName = QString::asprintf("SettingsDialog-%p", static_cast<void *>(q));
            openDialogs.insert(genericName, q);
            q->setObjectName(genericName);
        }

        QDialogButtonBox *buttonBox = q->buttonBox();
        buttonBox->setStandardButtons(QDialogButtonBox::RestoreDefaults
                                      | QDialogButtonBox::Ok
                                      | QDialogButtonBox::Apply
                                      | QDialogButtonBox::Cancel
                                      | QDialogButtonBox::Help);
        connect(buttonBox->button(QDialogButtonBox::Ok), &QAbstractButton::clicked, q, &ConfigDialog::updateSettings);
        connect(buttonBox->button(QDialogButtonBox::Apply), &QAbstractButton::clicked, q, &ConfigDialog::updateSettings);
        connect(buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked,
                q, [this]() { _k_updateButtons(); });
        connect(buttonBox->button(QDialogButtonBox::Cancel), &QAbstractButton::clicked, q, &ConfigDialog::updateWidgets);
        connect(buttonBox->button(QDialogButtonBox::RestoreDefaults), &QAbstractButton::clicked, q, &ConfigDialog::updateWidgetsDefault);
        connect(buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked,
                q, [this]() { _k_updateButtons(); });
        connect(buttonBox->button(QDialogButtonBox::Help), &QAbstractButton::clicked, q, &ConfigDialog::showHelp);

        connect(q, &KPageDialog::pageRemoved, q, &ConfigDialog::onPageRemoved);

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
        return nullptr;
    }

    KPageWidgetItem *item = d->addPageInternal(nullptr, page, itemName, pixmapName, header);
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
        return nullptr;
    }

    Q_ASSERT(page);
    if (!page) {
        return nullptr;
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
        return nullptr;
    }

    KPageWidgetItem *item = d->addPageInternal(nullptr, page, itemName, pixmapName, header);
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
    auto *frame = new QWidget(q);
    auto *boxLayout = new QVBoxLayout(frame);
    boxLayout->setContentsMargins(0, 0, 0, 0);

    auto *scroll = new QScrollArea(q);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setWidget(page);
    scroll->setWidgetResizable(true);
    scroll->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding );

    boxLayout->addWidget(scroll);
    auto *item = new KPageWidgetItem(frame, itemName);
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
    ConfigDialog::connect(manager, QOverload<>::of(&KConfigDialogManager::settingsChanged),
                          q, [this]() { _k_settingsChangedSlot(); });
    ConfigDialog::connect(manager, &KConfigDialogManager::widgetModified,
                          q, [this]() { _k_updateButtons(); });

    QDialogButtonBox *buttonBox = q->buttonBox();
    ConfigDialog::connect(buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, manager, &KConfigDialogManager::updateSettings);
    ConfigDialog::connect(buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, manager, &KConfigDialogManager::updateSettings);
    ConfigDialog::connect(buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, manager, &KConfigDialogManager::updateWidgets);
    ConfigDialog::connect(buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, manager, &KConfigDialogManager::updateWidgetsDefault);
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
    return nullptr;
}

bool ConfigDialog::showDialog(const QString &name)
{
    ConfigDialog *dialog = exists(name);
    if (dialog) {
        dialog->show();
    }
    return (dialog != nullptr);
}

void ConfigDialog::ConfigDialogPrivate::_k_updateButtons()
{
    static bool only_once = false;
    if (only_once) {
        return;
    }
    only_once = true;

    const bool has_changed = manager->hasChanged() || q->hasChanged() ||
            std::any_of(managerForPage.cbegin(), managerForPage.cend(),
                        [](KConfigDialogManager* manager) { return manager->hasChanged(); });

    setApplyButtonEnabled(has_changed);

    const bool is_default = manager->isDefault() && q->isDefault() &&
            std::all_of(managerForPage.cbegin(), managerForPage.cend(),
                        [](KConfigDialogManager* manager) { return manager->isDefault(); });

    setRestoreDefaultsButtonEnabled(!is_default);

    Q_EMIT q->widgetModified();
    only_once = false;
}

void ConfigDialog::ConfigDialogPrivate::_k_settingsChangedSlot()
{
    // Update the buttons
    _k_updateButtons();
    Q_EMIT q->settingsChanged(q->objectName());
}

void ConfigDialog::showEvent(QShowEvent *e)
{
    if (!d->shown) {
        updateWidgets();
        d->manager->updateWidgets();
        for (KConfigDialogManager* manager : std::as_const(d->managerForPage)) {
            manager->updateWidgets();
        }

        const bool has_changed = d->manager->hasChanged() || hasChanged() ||
            std::any_of(d->managerForPage.cbegin(), d->managerForPage.cend(),
                        [](KConfigDialogManager* manager) { return manager->hasChanged(); });

        d->setApplyButtonEnabled(has_changed);

        const bool is_default = d->manager->isDefault() && isDefault() &&
            std::all_of(d->managerForPage.cbegin(), d->managerForPage.cend(),
                        [](KConfigDialogManager* manager) { return manager->isDefault(); });

        d->setRestoreDefaultsButtonEnabled(!is_default);

        d->shown = true;
    }
    const QSize & availableSize = screen()->availableGeometry().size();
    this->setMaximumSize(availableSize);
    KPageDialog::showEvent(e);
}

void ConfigDialog::moveEvent(QMoveEvent *e)
{
    const QSize & availableSize = screen()->availableGeometry().size();
    this->setMaximumSize(availableSize);
    KPageDialog::moveEvent(e);
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
