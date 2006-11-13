/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2003 Benjamin C Meyer (ben+kdelibs at meyerhome dot net)
 *  Copyright (C) 2003 Waldo Bastian <bastian@kde.org>
 *  Copyright (C) 2004 Michael Brade <brade@kde.org>
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

/*
 *  KConfigDialog derivative allowing for a multi-level hierarchical TreeList.
 *  Differences from KConfigDialog:
 *  - Use QStringList instead of QString for the item name(s) in addPage and
 *    addPageInternal, thus calling the respective KDialogBase methods which
 *    allow specifying a path from which the TreeList hierarchy is constructed.
 *  - Use 16x16 icons in the TreeList.
 *  - Fill a new int m_lastAddedIndex with the pageIndex() of a new page added
 *    with addPageInternal, and offer a public interface int lastAddedIndex().
 *  See the KConfigDialog reference for detailed documentation.
 *
 *  begin:     Nov 22 2005
 *  copyright: (C) 2005-2006 by Eike Hein, KConfigDialog developers
 *  email:     hein@kde.org
 */

#include "konviconfigdialog.h"

#include <kconfigskeleton.h>
#include <kconfigdialogmanager.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kdebug.h>

#include <qlayout.h>
#include <qvbox.h>
#include <qmap.h>

QAsciiDict<KonviConfigDialog> KonviConfigDialog::openDialogs;

// This class is here purly so we don't break binary compatibility down the road.
class KonviConfigDialog::KConfigDialogPrivate
{
    public:
        KConfigDialogPrivate(KDialogBase::DialogType t) : shown(false), type(t), manager(0) { }

        bool shown;
        KDialogBase::DialogType type;
        KConfigDialogManager *manager;
        QMap<QWidget *, KConfigDialogManager *> managerForPage;
};

KonviConfigDialog::KonviConfigDialog( QWidget *parent, const char *name,
                                      KConfigSkeleton *config,
                                      DialogType dialogType,
                                      int dialogButtons,
                                      ButtonCode defaultButton,
                                      bool modal ) :
                   KDialogBase( dialogType, Qt::WStyle_DialogBorder,
                                parent, name, modal, i18n("Configure"), dialogButtons, defaultButton ),
                                d(new KConfigDialogPrivate(dialogType))
{
    if ( name )
    {
        openDialogs.insert(name, this);
    }
    else
    {
        QCString genericName;
        genericName.sprintf("SettingsDialog-%p", this);
        openDialogs.insert(genericName, this);
        setName(genericName);
    }

    connect(this, SIGNAL(okClicked()), this, SLOT(updateSettings()));
    connect(this, SIGNAL(applyClicked()), this, SLOT(updateSettings()));
    connect(this, SIGNAL(applyClicked()), this, SLOT(updateButtons()));
    connect(this, SIGNAL(defaultClicked()), this, SLOT(updateWidgetsDefault()));
    connect(this, SIGNAL(defaultClicked()), this, SLOT(updateButtons()));

    d->manager = new KConfigDialogManager(this, config);
    setupManagerConnections(d->manager);

    enableButton(Apply, false);
}

KonviConfigDialog::~KonviConfigDialog()
{
    openDialogs.remove(name());
    delete d;
}

void KonviConfigDialog::addPage(QWidget *page,
                                const QStringList &items,
                                const QString &pixmapName,
                                const QString &header,
                                bool manage)
{
    addPageInternal(page, items, pixmapName, header);
    if(manage)
        d->manager->addWidget(page);
}

void KonviConfigDialog::addPage(QWidget *page,
                                KConfigSkeleton *config,
                                const QStringList &items,
                                const QString &pixmapName,
                                const QString &header)
{
    addPageInternal(page, items, pixmapName, header);
    d->managerForPage[page] = new KConfigDialogManager(page, config);
    setupManagerConnections(d->managerForPage[page]);
}

void KonviConfigDialog::addPageInternal(QWidget *page,
                                        const QStringList &items,
                                        const QString &pixmapName,
                                        const QString &header)
{
    if(d->shown)
    {
        kdDebug(240) << "KonviConfigDialog::addPage: can not add a page after the dialog has been shown.";
        return;
    }
    switch(d->type)
    {
        case TreeList:
        case IconList:
        case Tabbed:
        {
            QVBox *frame = addVBoxPage(items, header, SmallIcon(pixmapName, 16));
            frame->setSpacing( 0 );
            frame->setMargin( 0 );
            page->reparent(((QWidget*)frame), 0, QPoint());
            m_lastAddedIndex = pageIndex(frame);
        }
        break;

        case Swallow:
        {
            page->reparent(this, 0, QPoint());
            setMainWidget(page);
        }
        break;

        case Plain:
        {
            QFrame *main = plainPage();
            QVBoxLayout *topLayout = new QVBoxLayout( main, 0, 0 );
            page->reparent(((QWidget*)main), 0, QPoint());
            topLayout->addWidget( page );
        }
        break;

        default:
            kdDebug(240) << "KonviConfigDialog::addpage: unknown type.";
    }
}

void KonviConfigDialog::setupManagerConnections(KConfigDialogManager *manager)
{
    connect(manager, SIGNAL(settingsChanged()), this, SLOT(settingsChangedSlot()));
    connect(manager, SIGNAL(widgetModified()), this, SLOT(updateButtons()));

    connect(this, SIGNAL(okClicked()), manager, SLOT(updateSettings()));
    connect(this, SIGNAL(applyClicked()), manager, SLOT(updateSettings()));
    connect(this, SIGNAL(defaultClicked()), manager, SLOT(updateWidgetsDefault()));
}

KonviConfigDialog* KonviConfigDialog::exists(const char* name)
{
    return openDialogs.find(name);
}

bool KonviConfigDialog::showDialog(const char* name)
{
    KonviConfigDialog *dialog = exists(name);
    if(dialog)
        dialog->show();
    return (dialog != NULL);
}

void KonviConfigDialog::updateButtons()
{
    static bool only_once = false;
    if (only_once) return;
    only_once = true;

    QMap<QWidget *, KConfigDialogManager *>::iterator it;

    bool has_changed = d->manager->hasChanged() || hasChanged();
    for (it = d->managerForPage.begin();
        it != d->managerForPage.end() && !has_changed;
        ++it)
    {
        has_changed |= (*it)->hasChanged();
    }

    enableButton(Apply, has_changed);

    bool is_default = d->manager->isDefault() && isDefault();
    for (it = d->managerForPage.begin();
        it != d->managerForPage.end() && is_default;
        ++it)
    {
        is_default &= (*it)->isDefault();
    }

    enableButton(Default, !is_default);

    emit widgetModified();
    only_once = false;
}

void KonviConfigDialog::settingsChangedSlot()
{
    // Update the buttons
    updateButtons();
    emit settingsChanged();
    emit settingsChanged(name());
}

void KonviConfigDialog::show()
{
    QMap<QWidget *, KConfigDialogManager *>::iterator it;

    updateWidgets();
    d->manager->updateWidgets();
    for (it = d->managerForPage.begin(); it != d->managerForPage.end(); ++it)
        (*it)->updateWidgets();

    bool has_changed = d->manager->hasChanged() || hasChanged();
    for (it = d->managerForPage.begin();
        it != d->managerForPage.end() && !has_changed;
        ++it)
    {
        has_changed |= (*it)->hasChanged();
    }

    enableButton(Apply, has_changed);

    bool is_default = d->manager->isDefault() && isDefault();
    for (it = d->managerForPage.begin();
        it != d->managerForPage.end() && is_default;
        ++it)
    {
        is_default &= (*it)->isDefault();
    }

    enableButton(Default, !is_default);
    d->shown = true;
    KDialogBase::show();
}

int KonviConfigDialog::lastAddedIndex()
{
    return m_lastAddedIndex;
}

void KonviConfigDialog::updateSettings()
{
}

void KonviConfigDialog::updateWidgets()
{
}

void KonviConfigDialog::updateWidgetsDefault()
{
}

