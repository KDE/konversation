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

#ifndef IRCVIEW_H
#define IRCVIEW_H

#include "common.h"

#include <QFontDatabase>
#include <QList>

#include <KTextBrowser>

class Server;
class ChatWindow;

class QDropEvent;

class KUrl;
class KToggleAction;
class KMenu;

#include <QAbstractTextDocumentLayout>

class IrcViewMarkerLine: public QObject, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)

    public:
        IrcViewMarkerLine(QObject *p) : QObject(p), QTextObjectInterface() {}
        ~IrcViewMarkerLine() {}
        virtual void drawObject(QPainter *painter, const QRectF &rect, QTextDocument *doc, int posInDocument, const QTextFormat &format);
        virtual QSizeF intrinsicSize(QTextDocument *doc, int posInDocument, const QTextFormat &format);
};


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

        void updateAppearance();

        QString currentChannel() { return m_currentChannel; }

        void setNickAndChannelContextMenusEnabled(bool enable);


    signals:
        void gotFocus(); // So we can set focus to input line
        void textToLog(const QString& text); ///< send the to the log file
        void sendFile(); ///< a command for a target to which we can DCC send
        void extendedPopup(int id); ///< this is for the query/nickname popup
        void autoText(const QString& text); ///< helper for autotext-on-highlight
        void textPasted(bool useSelection); ///< middle button with no m_copyUrlMenu
        void popupCommand(int); ///< wired to all of the popup menus
        void filesDropped(const QStringList&); ///< Q3UriDrag::decode valid in contentsDropEvent
        void doSearch(); /// Emitted when a search should be started
        void doSearchNext(); /// Emitted when there's a request to go to the next search result.
        void doSearchPrevious(); /// Emitted when there's a request to go to the previous search result.

        void setStatusBarTempText(const QString&); //! these two look like mixins to me
        void clearStatusBarTempText();//! these two look like mixins to me


    //// Marker lines
    public:
        /// Are there any markers or a remember lines in the view?
        ///Is used internally now.
        bool hasLines();

        /// QTextBlockFormat states for setUserState.
        enum BlockStates { None = -1, BlockIsMarker = 1, BlockIsRemember = 2 };

        /// QTextCharFormat object types.
        enum ObjectFormats { MarkerLine = QTextFormat::UserObject, RememberLine};

    public slots:
        /// Inserts a marker line.
        /// Does not disturb m_rememberLineDirtyBit.
        void insertMarkerLine();

        /// Insert a remember line now, or when text is appended. Sets the m_rememberLineDirtyBit
        /// unless configured to add a remember line at any time.
        void insertRememberLine();

        /// Prevents the next append from inserting a remember line.
        ///Simply clears m_rememberLineDirtyBit.
        void cancelRememberLine();

        /// Remove all of the marker lines, and the remember line.
        /// Does not effect m_rememberLineDirtyBit.
        void clearLines();

    private:
        /// The internal mechanics of inserting a line.
        /// Clears m_rememberLineDirtyBit.
        void appendRememberLine();

        /// Create a remember line and insert it.
        void appendLine(ObjectFormats=MarkerLine);

        /// Forget the position of the remember line and markers.
        void wipeLineParagraphs();

        /// Convenience method - is the last block any sort of line, or a specific line?
        /// @param select - default value is -1, meaning "is any kind of line"
        bool lastBlockIsLine(int select=-1);

        /// Causes a block to stop being a marker.
        void voidLineBlock(QTextBlock rem);

        /// Shortcut to get an object format of the desired type
        QTextCharFormat getFormat(ObjectFormats);

    private slots:
        /** Called to see if a marker is queued up for deletion. Only triggers if
            "where" is the beginning and there was nothing added.
        */
        void cullMarkedLine(int, int, int);

    private: //marker/remember line data
        bool m_nextCullIsMarker; ///< the next time a cull occurs, it'll be a marker
        QList<QTextBlock> m_markers; ///< what blocks are markers?
        int m_rememberLinePosition; ///< position of remember line in m_markers
        bool m_rememberLineDirtyBit; ///< the next append needs a remember line
        IrcViewMarkerLine markerFormatObject; ///< a QTextObjectInterface

    //// Other stuff
    public slots:
        //! FIXME enum { Raw, Query, Query+Action, Channel+Action, Server Message, Command Message, Backlog message } this looks more like a tuple
        void append(const QString& nick, const QString& message);
        void appendRaw(const QString& message, bool suppressTimestamps=false, bool self = false);
        void appendLog(const QString& message);

        void appendQuery(const QString& nick, const QString& message, bool inChannel = false);
        void appendQueryAction(const QString& nick, const QString& message);
    protected:
        //! FIXME why is this protected, and all alone down there?
        void appendAction(const QString& nick, const QString& message);

        /// Appends a new line without any scrollback or notification checks
        void doRawAppend(const QString& newLine, bool rtl);

    public slots:
        void appendChannelAction(const QString& nick, const QString& message);

        void appendServerMessage(const QString& type, const QString& message, bool parseURL = true);
        void appendCommandMessage(const QString& command, const QString& message, bool important, bool parseURL=true, bool self=false);
        void appendBacklogMessage(const QString& firstColumn, const QString& message);

    protected:
        void doAppend(const QString& line, bool rtl, bool self=false);

        void updateNickMenuEntries(const QString& nickname);

    public slots:
        /// Emits the doSeach signal.
        void findText();
        /// Emits the doSeachNext signal.
        void findNextText();
        /// Emits the doSeachPrevious signal.
        void findPreviousText();

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
        void handleContextActions();

    protected:
        void openLink(const QString &url, bool newTab=false);

        QString filter(const QString& line, const QString& defaultColor, const QString& who=NULL, bool doHighlight=true, bool parseURL=true, bool self=false);

        void replaceDecoration(QString& line,char decoration,char replacement);

        virtual void resizeEvent(QResizeEvent *event);
        virtual void mouseReleaseEvent(QMouseEvent* ev);
        virtual void mousePressEvent(QMouseEvent* ev);
        virtual void mouseMoveEvent(QMouseEvent* ev);
        virtual void contextMenuEvent(QContextMenuEvent* ev);

        bool contextMenu(QContextMenuEvent* ce);

        void setupNickPopupMenu(bool isQuery);
        void updateNickMenuEntries(KMenu* popup, const QString& nickname);
        void setupChannelPopupMenu();

        QChar::Direction basicDirection(const QString &string);

        /// Returns a formated timestamp if timestamps are enabled else it returns QString::null
        QString timeStamp();

        /// Returns a formated nick string
        //! FIXME formatted in what way?
        QString createNickLine(const QString& nick, const QString& defaultColor,
            bool encapsulateNick = true, bool privMsg = false);



        //// Search
        QTextDocument::FindFlags m_searchFlags;
        bool m_forward;
        QString m_pattern;

        //used in ::filter
        QColor m_highlightColor;


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
        QAction* m_copyUrlClipBoard;
        QAction* m_bookmark;
        QAction* m_saveUrl;
        KToggleAction* m_ignoreAction;
        QAction* m_addNotifyAction;
        bool m_copyUrlMenu; ///<the menu we're popping up, is it for copying URI?
        KMenu* m_nickPopup; ///<menu to show when context-click on a nickname
        KMenu* m_channelPopup; ///<menu to show when context-click on a channel


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
