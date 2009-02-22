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
  Copyright (C) 2006-2008 Eike Hein <hein@kde.org>
*/

/** New IRCViewer.

    Sadly, the k3 and q3 controls are useless for porting. I've chosen to use QPlainTextEdit for now,
    as it's API fairly close to what we'll end up with.
 */


#include "common.h"

#include <qmap.h>
#include <qfontdatabase.h>
#include <QList>

#include <KTextBrowser>

class QDropEvent;
class KUrl;

class KMenu;

class Server;
class ChatWindow;

class IRCView : public KTextBrowser
{
    Q_OBJECT

    public:
        IRCView(QWidget* parent,Server* newServer);
        ~IRCView();

        //void clear();
        //! Some people apparently want the text in the view to be doublespaced :/
        void enableParagraphSpacing();

        //! this function is proper given it is not nessary for the ircview to have a server for DCC.
        void setServer(Server* server);
        //! FIXME assumes the IRCView looks at a chatwin
        void setChatWin(ChatWindow* chatWin);

        // Returns the current nick under context menu.
        const QString& getContextNick() const;

        //!Obtain the context menu popup in order to add things to it
        KMenu* getPopup() const;


        bool search(const QString& pattern, bool caseSensitive, bool wholeWords, bool forward, bool fromCursor);
        bool searchNext(bool reversed = false);

        //! FIXME maybe we should create some sort of palette of our own?
        QColor highlightColor() { return m_highlightColor; }
        void setViewBackground(const QColor& backgroundColor, const KUrl& url);

        QString currentChannel() { return m_currentChannel; }

        void setNickAndChannelContextMenusEnabled(bool enable);

        bool hasLines(); ///< are there any remember lines?


    signals:
        void gotFocus(); // So we can set focus to input line
        void textToLog(const QString& text); ///< send the to the log file
        void sendFile(); ///< a command for a target to which we can DCC send
        void extendedPopup(int id); ///< this is for the query/nickname popup
        void autoText(const QString& text); ///< helper for autotext-on-highlight
        void textPasted(bool useSelection); ///< middle button with no m_copyUrlMenu
        void popupCommand(int); ///< wired to all of the popup menus
        void filesDropped(const QStringList&); ///< Q3UriDrag::decode valid in contentsDropEvent
        void doSearch(); ///< this is a

        void setStatusBarTempText(const QString&); //! these two look like mixins to me
        void clearStatusBarTempText();//! these two look like mixins to me

    public slots:
        //! FIXME  mixin?
        void insertRememberLine();
        void cancelRememberLine();
        void insertMarkerLine();
        void clearLines();

        //! FIXME enum { Raw, Query, Query+Action, Channel+Action, Server Message, Command Message, Backlog message } this looks more like a tuple
        void append(const QString& nick, const QString& message);
        void appendRaw(const QString& message, bool suppressTimestamps=false, bool self = false);

        void appendQuery(const QString& nick, const QString& message, bool inChannel = false);
        void appendQueryAction(const QString& nick, const QString& message);
    protected:
        //! FIXME why is this protected, and all alone down there?
        void appendAction(const QString& nick, const QString& message);

    public slots:
        void appendChannelAction(const QString& nick, const QString& message);

        void appendServerMessage(const QString& type, const QString& message, bool parseURL = true);
        void appendCommandMessage(const QString& command, const QString& message, bool important, bool parseURL=true, bool self=false);
        void appendBacklogMessage(const QString& firstColumn, const QString& message);

    protected:
        void doAppend(const QString& line, bool self=false);
        void appendLine(const QString& color);
        void appendRememberLine();


    public slots:
        void search(); ///! TODO FIXME this is a dangerous overload
        void searchAgain();

        //! FIXME eh? what is this?
        void setCurrentChannel(const QString& channel) { m_currentChannel = channel; }

        /// Overwritten so the scrollview remains not freaked out
        //virtual void removeSelectedText(int selNum=0);
        //! TODO FIXME meta, derived
        //virtual void scrollToBottom();            // Overwritten for internal reasons

        // Clears context nick
        //! The name of this method is unclear enough that it needs documentation, but clear enough that the documentation just needs to be a repeat of the name. Thanks for playing, but get some kills next time.
        void clearContextNick();

        // Updates the scrollbar position
        //! Again. Really? Two in a row? Couldn't be more inventive, whoever you are? Come on, show some personality. Let your vocabulary loose! Because right now its looking like you don't know what you're taking about.
        //void updateScrollBarPos();

    protected slots:
        void highlightedSlot(const QString& link);
        void saveLinkAs();
        void anchorClicked(const QUrl& url);
    void copyUrl();
    void slotBookmark();
    protected:
        void openLink(const QString &url, bool newTab=false);

        QString filter(const QString& line, const QString& defaultColor, const QString& who=NULL, bool doHighlight=true, bool parseURL=true, bool self=false);

        void replaceDecoration(QString& line,char decoration,char replacement);

        //virtual void contentsDragMoveEvent(QDragMoveEvent* e);
        //virtual void contentsDropEvent(QDropEvent* e);
        virtual void mouseReleaseEvent(QMouseEvent* ev);
        virtual void mousePressEvent(QMouseEvent* ev);
        virtual void mouseMoveEvent(QMouseEvent* ev);
        virtual void contextMenuEvent(QContextMenuEvent* ev);

        //virtual void keyPressEvent(QKeyEvent* e);
        //virtual void resizeEvent(QResizeEvent* e);

        //void hideEvent(QHideEvent* event);
        //void showEvent(QShowEvent* event);

        bool contextMenu(QContextMenuEvent* ce);

        void setupNickPopupMenu();
        void updateNickMenuEntries(KMenu* popup, const QString& nickname);
        void setupQueryPopupMenu();
        void setupChannelPopupMenu();

        QChar::Direction basicDirection(const QString &string);

        /// Returns a formated timestamp if timestamps are enabled else it returns QString::null
        QString timeStamp();

        /// Returns a formated nick string
        //! FIXME formatted in what way?
        QString createNickLine(const QString& nick, bool encapsulateNick = true, bool privMsg = false);



        //// Search
        QTextDocument::FindFlags m_searchFlags;
        bool m_forward;
        QString m_pattern;

        //used in ::filter
        QColor m_highlightColor;

        //// Remember line
        void updateLineParagraphs(int numRemoved);
        void wipeLineParagraphs();
        int m_rememberLineParagraph;
        bool m_rememberLineDirtyBit;
        QList<int> m_markerLineParagraphs;


        QString m_lastStatusText; //last sent status text to the statusbar. Is empty after clearStatusBarTempText()

        bool m_resetScrollbar; ///< decide if we should place the scrollbar at the bottom on show()

        //used in ::filter
        QString m_autoTextToSend;

        //TODO FIXME light this on fire and send it sailing down an uncharted river riddled with arrows
        Konversation::TabNotifyType m_tabNotification;


        //QString m_buffer; ///< our text
        Server* m_server; //! FIXME assumes we have a server

        //// Popup menus
        KMenu* m_popup; ///< text area context menu
        QAction* copyUrlMenuSeparator;
    QAction * m_copyUrlClipBoard;
    QAction * m_bookmark;
    QAction * m_saveUrl;
        bool m_copyUrlMenu; ///<the menu we're popping up, is it for copying URI?
        QString m_highlightedURL;   // the URL we're currently hovering on with the mouse
        QTextCharFormat m_fmtUnderMouse;
        KMenu* m_nickPopup; ///<menu to show when context-click on a nickname
        KMenu* m_channelPopup; ///<menu to show when context-click on a channel
        KMenu* m_modes; ///< the submenu for mode changes on a nickname menu
        KMenu* m_kickban; ///< the submenu for kicks/bans on a nickname menu
        int m_nickPopupId; ///< context menu
        int m_channelPopupId; ///< context menu


        QString m_urlToCopy; ///< the URL we might be about to copy

        //// RTL hack
        static QChar LRM;
        static QChar RLM;
        static QChar LRE;
        static QChar RLE;
        static QChar RLO;
        static QChar LRO;
        static QChar PDF;

        //// Nickname colorization
        uint m_offset;
        QStringList m_colorList;

        QString m_currentNick;
        QString m_currentChannel;
        bool m_isOnNick; ///< context menu click hit a nickname
        bool m_isOnChannel; ///< context menu click hit a channel
        bool m_mousePressed; ///< currently processing a mouse press
        QString m_urlToDrag; ///< we took a stab at whatever was clicked on, may or may not actually be a URL
        QPoint m_pressPosition; ///< x,y of the click, relative to the GPS location of tip of Phantom's left ear

        //! TODO FIXME i'll bite. why do we have this in here?
        QFontDatabase m_fontDataBase;

        ChatWindow* m_chatWin;
        friend class IRCStyleSheet;
};
#endif
