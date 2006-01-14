#ifndef IRCVIEW_H
#define IRCVIEW_H

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  the text widget used for all text based panels
  begin:     Sun Jan 20 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <qmap.h>

#include <ktextbrowser.h>

#include "common.h"

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
            Bookmark
        };

        void setChatWin(ChatWindow* chatWin);

        bool search(const QString& pattern, bool caseSensitive,
            bool wholeWords, bool forward, bool fromCursor);
        bool searchNext();

        QColor highlightColor() { return m_highlightColor; }

        signals:
        // Notify container of new text and highlight state
        void updateTabNotification(Konversation::TabNotifyType type);
        void gotFocus();                          // So we can set focus to input line
        void textToLog(const QString& text);
        void sendFile();
        void extendedPopup(int id);
        void autoText(const QString& text);
        void textPasted(bool useSelection);
        void popupCommand(int);
        void filesDropped(const QStrList&);
        void doSearch();

    public slots:
        void append(const QString& nick, const QString& message);
        void appendRaw(const QString& message, bool suppressTimestamps=false, bool self = false);
        void appendQuery(const QString& nick, const QString& message);
        void appendAction(const QString& nick, const QString& message);
        void appendServerMessage(const QString& type, const QString& message);
        void appendCommandMessage(const QString& command, const QString& message, bool important,
            bool parseURL=true, bool self=false);
        void appendBacklogMessage(const QString& firstColumn, const QString& message);
        void search();
        void searchAgain();

        virtual void removeSelectedText(int selNum=0);

        virtual void scrollToBottom();            // Overwritten for internal reasons

        // Clears context nick
        void clearContextNick();

        /// Updates the scrollbar position
        void updateScrollBarPos();

    protected slots:
        void highlightedSlot(const QString& link);
        void urlClickSlot(const QString &url);

    protected:
        void urlClickSlot(const QString &url, bool newTab);
        QString filter(const QString& line, const QString& defaultColor, const QString& who=NULL,
            bool doHighlight=true, bool parseURL=true, bool self=false);
        void doAppend(const QString& line, bool important=true, bool self=false);
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
        void setupQueryPopupMenu();

        QChar::Direction basicDirection(const QString &string);

        /// Returns a formated timestamp if timestamps are enabled else it returns QString::null
        QString timeStamp();

        // used by search function
        int m_findParagraph;
        int m_findIndex;

        // decide if we should place the scrollbar at the bottom on show()
        bool m_resetScrollbar;

        QString m_autoTextToSend;
        Konversation::TabNotifyType m_tabNotification;
        bool m_copyUrlMenu;
        QString m_urlToCopy;

        QString m_buffer;
        Server* m_server;
        QPopupMenu* m_popup;

        KPopupMenu* m_nickPopup;
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
        QMap<QString,QString> m_colorMap;

        QString m_currentNick;
        bool m_isOnNick;
        bool m_mousePressed;
        QString m_urlToDrag;
        QPoint m_pressPosition;
        int m_popupId;

        ChatWindow* m_chatWin;
};
#endif
