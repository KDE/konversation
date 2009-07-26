/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Channel Nick List, including context menu
  begin:     Fre Jun 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef NICKLISTVIEW_H
#define NICKLISTVIEW_H

#include "channel.h"
#include "images.h"
#include "common.h"
#include "application.h" // for PopupIDs...

#include <KMenu>

#include <QTreeWidget>

class QActionGroup;
class QMenu;
class QContextMenuEvent;
class QTimer;

class KAction;


class NickListView : public QTreeWidget
{
    Q_OBJECT

        public:
        NickListView(QWidget* parent, Channel *chan);
        ~NickListView();

        /** Call when the icons have been changed.
         */
        void refresh();
        void setWhatsThis();

        void setSortingEnabled(bool enable); // WARNING: originally non-virtual
        void sortByColumn(int column, Qt::SortOrder order); // WARNING: originally non-virtual
        void fastSetSortingEnabled(bool value);
        int findLowerBound(const QTreeWidgetItem& item) const;
        void executeDelayedItemsLayout();

        static int getMinimumRowHeight();

        // A helper class to disable sorting while in scope
        class NoSorting
        {
            private:
                NickListView *w;
                bool enabled;
            public:
                NoSorting(NickListView *w) : w(w), enabled(w->isSortingEnabled())
                {
                    if (enabled) w->fastSetSortingEnabled(false);
                };
                ~NoSorting()
                {
                    if (enabled) w->fastSetSortingEnabled(true);
                }
        };

    public slots:
        /** Resort the listview. CAUTION: this might be CPU intensive
         */
        void resort();

        signals:
        /* Will be connected to Channel::popupCommand(int) */
        void popupCommand(int id);

        protected slots:
        void slotActionTriggered(QAction* action);

    protected:
        //! Reimplemented for dynamic tooltips
        virtual bool event(QEvent *ev);
        virtual void contextMenuEvent(QContextMenuEvent* ce);

        // Drag & Drop support
        virtual QStringList mimeTypes () const;
        bool canDecodeMime(QDropEvent const *event) const;
        virtual bool dropMimeData(QTreeWidgetItem *parent, int index, const QMimeData *data, Qt::DropAction action);
        virtual void dragEnterEvent(QDragEnterEvent *event);
        virtual void dragMoveEvent(QDragMoveEvent *event);

        void insertAssociationSubMenu();
        void updateActions();
        KMenu* popup;
        KMenu* modes;
        KMenu* kickban;
        KMenu* addressbook;
        Channel *channel;
        QTimer *m_resortTimer;

    private:
        static int s_minimumRowHeight;
        static void updateMinimumRowHeight();

        // TODO use a more specific enum for just our actions?
        KAction* createAction(QMenu* menu, const QString& text, Konversation::PopupIDs);

        QActionGroup* m_actionGroup;
        KAction* m_whoisAction;
        KAction* m_versionAction;
        KAction* m_pingAction;
        KAction* m_giveOpAction;
        KAction* m_takeOpAction;
        KAction* m_giveHalfOpAction;
        KAction* m_takeHalfOpAction;
        KAction* m_giveVoiceAction;
        KAction* m_takeVoiceAction;
        KAction* m_ignoreAction;
        KAction* m_unIgnoreAction;
        KAction* m_kickAction;
        KAction* m_kickBanAction;
        KAction* m_banNickAction;
        KAction* m_banHostAction;
        KAction* m_banDomainAction;
        KAction* m_banUserHostAction;
        KAction* m_banUserDomainAction;
        KAction* m_kickBanHostAction;
        KAction* m_kickBanDomainAction;
        KAction* m_kickBanUserHostAction;
        KAction* m_kickBanUserDomainAction;
        KAction* m_addNotifyAction;
        KAction* m_sendMailAction;
        KAction* m_AddressbookNewAction;
        KAction* m_AddressbookChangeAction;
        KAction* m_AddressbookEditAction;
        KAction* m_AddressbookDeleteAction;
        KAction* m_openQueryAction;
        KAction* m_startDccChatAction;
        KAction* m_dccSendAction;
};
#endif
