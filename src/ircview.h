#ifndef IRCVIEW_H
#define IRCVIEW_H

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2002 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2005-2006 Peter Simonsson <psn@linux.se>
  Copyright (C) 2006-2007 Eike Hein <hein@kde.org>
*/

#include "common.h"

#include <qmap.h>
#include <qfontdatabase.h>

#include <ktextbrowser.h>


class QPixmap;
class QStrList;
class QDropEvent;
class QDragEnterEvent;
class QEvent;

class KPopupMenu;

class Server;
class ChatWindow;
class SearchBar;

class IRCView : public KTextBrowser
{
    Q_OBJECT

    public:
        IRCView(QWidget* parent,Server* newServer);
        ~IRCView();

        void clear();
        void setViewBackground(const QColor& backgroundColor, const QString& pixmapName);
        void setServer(Server* server);

        // Returns the current nick under context menu.
        const QString& getContextNick() const;

        void updateStyleSheet();

        QPopupMenu* getPopup() const;

        enum PopupIDs
        {
            Copy,CopyUrl,SelectAll,
            Search,
            SendFile,
            Bookmark,
            SaveAs
        };

        void setChatWin(ChatWindow* chatWin);

        bool search(const QString& pattern, bool caseSensitive,
            bool wholeWords, bool forward, bool fromCursor);
        bool searchNext(bool reversed = false);

        QColor highlightColor() { return m_highlightColor; }
        QString currentChannel() { return m_currentChannel; }

        void setNickAndChannelContextMenusEnabled(bool enable);

        bool hasLines();


    signals:
        // Notify container of new text and highlight state
        void gotFocus();                          // So we can set focus to input line
        void textToLog(const QString& text);
        void sendFile();
        void extendedPopup(int id);
        void autoText(const QString& text);
        void textPasted(bool useSelection);
        void popupCommand(int);
        void filesDropped(const QStrList&);
        void doSearch();

        void setStatusBarTempText(const QString&);
        void clearStatusBarTempText();

    public slots:
        void insertRememberLine();
        void cancelRememberLine();
        void insertMarkerLine();
        void clearLines();

        void append(const QString& nick, const QString& message);
        void appendRaw(const QString& message, bool suppressTimestamps=false, bool self = false);
        void appendQuery(const QString& nick, const QString& message, bool inChannel = false);
        void appendAction(const QString& nick, const QString& message);
        void appendServerMessage(const QString& type, const QString& message, bool parseURL = true);
        void appendCommandMessage(const QString& command, const QString& message, bool important,
            bool parseURL=true, bool self=false);
        void appendBacklogMessage(const QString& firstColumn, const QString& message);
        void search();
        void searchAgain();

        void setCurrentChannel(const QString& channel) { m_currentChannel = channel; }

        virtual void removeSelectedText(int selNum=0);

        virtual void scrollToBottom();            // Overwritten for internal reasons

        // Clears context nick
        void clearContextNick();

        // Updates the scrollbar position
        void updateScrollBarPos();

    protected slots:
        void highlightedSlot(const QString& link);
        void saveLinkAs(const QString& url);

    protected:
        void openLink(const QString &url, bool newTab=false);
        QString filter(const QString& line, const QString& defaultColor, const QString& who=NULL,
        bool doHighlight=true, bool parseURL=true, bool self=false);
        void doAppend(const QString& line, bool self=false);
        void replaceDecoration(QString& line,char decoration,char replacement);
        virtual void contentsDragMoveEvent(QDragMoveEvent* e);
        virtual void contentsDropEvent(QDropEvent* e);
        virtual void contentsMouseReleaseEvent(QMouseEvent* ev);
        virtual void contentsMousePressEvent(QMouseEvent* ev);
        virtual void contentsMouseMoveEvent(QMouseEvent* ev);
        virtual void contentsContextMenuEvent(QContextMenuEvent* ev);

        virtual void keyPressEvent(QKeyEvent* e);
        virtual void resizeEvent(QResizeEvent* e);

        void hideEvent(QHideEvent* event);
        void showEvent(QShowEvent* event);

        bool contextMenu(QContextMenuEvent* ce);

        void setupNickPopupMenu();
        void updateNickMenuEntries(QPopupMenu* popup, const QString& nickname);
        void setupQueryPopupMenu();
        void setupChannelPopupMenu();

        QChar::Direction basicDirection(const QString &string);

        /// Returns a formated timestamp if timestamps are enabled else it returns QString::null
        QString timeStamp();

        /// Returns a formated nick string
        QString createNickLine(const QString& nick, bool encapsulateNick = true, bool privMsg = false);

        // used by search function
        int m_findParagraph;
        int m_findIndex;

        void appendLine(const QString& color);
        void appendRememberLine();

        void updateLineParagraphs(int numRemoved);
        void wipeLineParagraphs();

        int m_rememberLineParagraph;
        bool m_rememberLineDirtyBit;

        QValueList<int> m_markerLineParagraphs;

        // This is set to what we last sent status text to the statusbar.  Empty if we have sent clearStatusBarTempText() string
        QString m_lastStatusText;
        // decide if we should place the scrollbar at the bottom on show()
        bool m_resetScrollbar;

        QString m_autoTextToSend;
        Konversation::TabNotifyType m_tabNotification;
        bool m_copyUrlMenu;
        QString m_urlToCopy;

        QString m_buffer;
        Server* m_server;
        QPopupMenu* m_popup;
        int toggleMenuBarSeparator;
        KPopupMenu* m_nickPopup;
        KPopupMenu* m_channelPopup;
        KPopupMenu* m_modes;
        KPopupMenu* m_kickban;

        static QChar LRM;
        static QChar RLM;
        static QChar LRE;
        static QChar RLE;
        static QChar RLO;
        static QChar LRO;
        static QChar PDF;

        bool m_caseSensitive;
        bool m_wholeWords;
        bool m_forward;
        bool m_fromCursor;
        QString m_pattern;
        QColor m_highlightColor;

        uint m_offset;
        QStringList m_colorList;

        QString m_highlightedURL;   // the URL we're currently hovering on with the mouse
        QString m_currentNick;
        QString m_currentChannel;
        bool m_isOnNick;
        bool m_isOnChannel;
        bool m_mousePressed;
        QString m_urlToDrag;
        QPoint m_pressPosition;
        int m_nickPopupId;
        int m_channelPopupId;

        QFontDatabase m_fontDataBase;

        ChatWindow* m_chatWin;
        friend class IRCStyleSheet;
};
#endif
