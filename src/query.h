/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  begin:     Mon Jan 28 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef QUERY_H
#define QUERY_H

#include <qstring.h>

#include "chatwindow.h"
#include "nickinfo.h"

/*
  @author Dario Abatianni
*/

/* TODO: Idle counter to close query after XXX minutes of inactivity */
/* TODO: Use /USERHOST to check if queries are still valid */

class QLineEdit;
class QCheckBox;
class QLabel;
class QStrList;

class IRCInput;

class Query : public ChatWindow
{
    Q_OBJECT

    public:
        Query(QWidget* parent);
        ~Query();

        /** This will always be called soon after this object is created.
         *  @param nickInfo A nickinfo that must exist.
         */
        void setNickInfo(const NickInfoPtr & nickInfo);
        /** It seems that this does _not_ guaranttee to return non null.
         *  The problem is when you open a query to someone, then the go offline.
         *  This should be fixed maybe?  I don't know.
         */
        NickInfoPtr getNickInfo();
        virtual QString getTextInLine();
        virtual bool closeYourself();
        virtual bool canBeFrontView();
        virtual bool searchView();

        virtual void setChannelEncoding(const QString& encoding);
        virtual QString getChannelEncoding();
        virtual QString getChannelEncodingDefaultDesc();
        virtual void emitUpdateInfo();

        virtual bool areIRCColorsSupported() {return true; }
        virtual bool isInsertCharacterSupported() { return true; }

        /** call this when you see a nick quit from the server.
         *  @param reason The quit reason given by that user.
         */
        void quitNick(const QString& reason);

    signals:
        void sendFile(const QString& recipient);
        void updateQueryChrome(ChatWindow*, const QString&);

    public slots:
        void sendQueryText(const QString& text);
        void appendInputText(const QString& s);
        virtual void indicateAway(bool show);
        void updateAppearance();
        virtual void lostFocus();

    protected slots:
        void queryTextEntered();
        void queryPassthroughCommand();
        void sendFileMenu();
        void filesDropped(const QStrList& files);
        // connected to IRCInput::textPasted() - used to handle large/multiline pastes
        void textPasted(const QString& text);
        void popup(int id);
        void nickInfoChanged();

    protected:
        void setName(const QString& newName);
        void showEvent(QShowEvent* event);
        /** Called from ChatWindow adjustFocus */
        virtual void childAdjustFocus();

        bool awayChanged;
        bool awayState;

        QString queryName;
        QString buffer;

        QLabel* queryHostmask;
        QLabel* addresseeimage;
        QLabel* addresseelogoimage;
        QLabel* awayLabel;
        IRCInput* queryInput;
        NickInfoPtr m_nickInfo;
};
#endif
