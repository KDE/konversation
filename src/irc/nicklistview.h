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

#include <k3listview.h>
#include <KMenu>

class QActionGroup;
class QMenu;
class KAction;
class QContextMenuEvent;
class QTimer;

class NickListView : public K3ListView
{
    Q_OBJECT

        public:
        NickListView(QWidget* parent, Channel *chan);
        ~NickListView();

        /** Call when the icons have been changed.
         */
        void refresh();
        void setWhatsThis();

        virtual void setSorting(int column, bool ascending);

    public slots:
        /** When this is called, resort is guaranteed to be called within a hard-coded time (a few seconds).
         *  This prevents lots of calls to resort.
         */
        void startResortTimer();

        /** Resort the listview.
         *  It is better to call startResortTimer() which will resort with a minimum of a
         *  1 second delay.
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
    void contextMenuEvent(QContextMenuEvent* ce);
        virtual bool acceptDrag (QDropEvent* event) const;
        void insertAssociationSubMenu();
        void updateActions();
        KMenu* popup;
        KMenu* modes;
        KMenu* kickban;
        KMenu* addressbook;
        Channel *channel;
        QTimer *m_resortTimer;

        int m_column;
        bool m_ascending;

        private:
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
