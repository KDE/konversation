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

#ifndef KONVICONFIGDIALOG_H
#define KONVICONFIGDIALOG_H

#include <q3asciidict.h>

#include <kpagedialog.h>


class KConfig;
class KConfigSkeleton;
class KConfigDialogManager;

class KonviConfigDialog : public KPageDialog
{
    Q_OBJECT

    signals:
    void widgetModified();

    void settingsChanged();

    void settingsChanged(const char *dialogName);

    void sigUpdateWidgets();

    public:
        KonviConfigDialog( QWidget *parent, const char *name,
                           KConfigSkeleton *config,
                           KPageDialog::FaceType  dialogType = List,
                           QFlags<KDialog::ButtonCode> dialogButtons = Default|Ok|Apply|Cancel|Help,
                           ButtonCode defaultButton = Ok,
                           bool modal=false );

        ~KonviConfigDialog();

        KPageWidgetItem *addPage( QWidget *page, KPageWidgetItem *group,
                      const QString &pixmapName,
                      const QString &header=QString(),
                      bool manage=true );

        KPageWidgetItem *addPage( QWidget *page, KConfigSkeleton *config,
                      KPageWidgetItem *group,
                      const QString &pixmapName,
                      const QString &header=QString() );

        static KonviConfigDialog* exists( const char* name );

        static bool showDialog( const char* name );

        virtual void show();

        int lastAddedIndex();

    protected slots:
        virtual void updateSettings();

        virtual void updateWidgets();

        virtual void updateWidgetsDefault();

    protected:
        virtual bool hasChanged() { return false; }

        virtual bool isDefault() { return true; }

    protected slots:
        void updateButtons();

        void settingsChangedSlot();

    private:
        KPageWidgetItem *addPageInternal(QWidget *page, KPageWidgetItem *group,
                             const QString &pixmapName, const QString &header);

        void setupManagerConnections(KConfigDialogManager *manager);

    private:
        static Q3AsciiDict<KonviConfigDialog> openDialogs;

        class KConfigDialogPrivate;

        KConfigDialogPrivate *d;

        int m_lastAddedIndex;
};
#endif //KONVICONFIGDIALOG_H
