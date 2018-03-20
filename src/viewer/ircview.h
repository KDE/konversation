/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2002 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2005-2016 Peter Simonsson <peter.simonsson@gmail.com>
  Copyright (C) 2006-2008 Eike Hein <hein@kde.org>
*/

#ifndef IRCVIEW_H
#define IRCVIEW_H

#include "common.h"
#include "irccontextmenus.h"

#include <QAbstractTextDocumentLayout>
#include <QFontDatabase>

#include <QTextBrowser>
#include <QUrl>


class Server;
class ChatWindow;
struct Burr;

class IrcViewMarkerLine: public QObject, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)

    public:
        IrcViewMarkerLine(QObject *p) : QObject(p), QTextObjectInterface() {}
        ~IrcViewMarkerLine() {}
        void drawObject(QPainter *painter, const QRectF &rect, QTextDocument *doc, int posInDocument, const QTextFormat &format) Q_DECL_OVERRIDE;
        QSizeF intrinsicSize(QTextDocument *doc, int posInDocument, const QTextFormat &format) Q_DECL_OVERRIDE;
};

/// Helper struct which remembers the openHtmlTags, the fore and
/// background color, if the reverse char is set and the defaultcolor (for reverse)
/// while the ircrichtext -> html generation is in progress
struct TextHtmlData
{
    TextHtmlData()
        : reverse(false)
    {
    }

    QList<QString> openHtmlTags;
    QString lastFgColor;
    QString lastBgColor;
    bool reverse;
    QString defaultColor;
};

class IRCView : public QTextBrowser
{
    Q_OBJECT

    public:
        explicit IRCView(QWidget* parent);
        ~IRCView();

        //! this function is proper given it is not nessary for the ircview to have a server for DCC.
        void setServer(Server* server);
        //! FIXME assumes the IRCView looks at a chatwin
        void setChatWin(ChatWindow* chatWin);

        bool search(const QString& pattern, QTextDocument::FindFlags flags, bool fromCursor);
        bool searchNext(bool reversed = false);

        //! FIXME maybe we should create some sort of palette of our own?
        QColor highlightColor() { return m_highlightColor; }

        void updateAppearance();

        void setContextMenuOptions(IrcContextMenus::MenuOptions options, bool on);

    Q_SIGNALS:
        void gotFocus(); // So we can set focus to input line
        void textToLog(const QString& text); ///< send the to the log file
        void sendFile(); ///< a command for a target to which we can DCC send
        void autoText(const QString& text); ///< helper for autotext-on-highlight
        void textPasted(bool useSelection); ///< middle button with no m_copyUrlMenu
        void urlsDropped(const QList<QUrl>& urls);
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

    public Q_SLOTS:
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

    protected:
        QMimeData* createMimeDataFromSelection() const Q_DECL_OVERRIDE;
        void dragEnterEvent(QDragEnterEvent* e) Q_DECL_OVERRIDE;
        void dragMoveEvent(QDragMoveEvent* e) Q_DECL_OVERRIDE;
        void dropEvent(QDropEvent* e) Q_DECL_OVERRIDE;

    private:
        /// The internal mechanics of inserting a line.
        /// Clears m_rememberLineDirtyBit.
        void appendRememberLine();

        /// Create a remember line and insert it.
        /// @return - Pointer to the Burr that was inserted into the block
        Burr* appendLine(ObjectFormats=MarkerLine);

        /// Convenience method - forget the position of the remember line and markers.
        void wipeLineParagraphs();

        /// Convenience method - is the last block any sort of line, or a specific line?
        /// @param select - default value is -1, meaning "is any kind of line"
        bool lastBlockIsLine(int select=-1);

        /// Causes a block to stop being a marker.
        void voidLineBlock(QTextBlock rem);

        /// Shortcut to get an object format of the desired type
        QTextCharFormat getFormat(ObjectFormats);

    public Q_SLOTS:
        // Doesn't have to be a slot, but what the hay.
        /// Called *only* from ~Burr(), by QTextBlockData::free
        void blockDeleted(Burr* b);

    private Q_SLOTS:
        /** Called every time a change occurs to the document.
         *
         * Used to infer the clearing of the entire document,
         * because Trolltech removed virtual from the method
         * that would indicate authoritatively.
         */
        void cullMarkedLine(int, int, int);

    private: //marker/remember line data
        Burr *m_rememberLine, *m_lastMarkerLine;
        bool m_rememberLineDirtyBit; ///< the next append needs a remember line
        IrcViewMarkerLine markerFormatObject; ///< a QTextObjectInterface

    //// Other stuff
    public Q_SLOTS:
        //! FIXME enum { Raw, Query, Query+Action, Channel+Action, Server Message, Command Message, Backlog message } this looks more like a tuple
        void append(const QString& nick, const QString& message, const QHash<QString, QString> &messageTags = QHash<QString, QString>(), const QString& label = QString());
        void appendRaw(const QString& message, bool self = false);
        void appendLog(const QString& message);

        void appendQuery(const QString& nick, const QString& message, const QHash<QString, QString> &messageTags, bool inChannel = false);
        void appendQueryAction(const QString& nick, const QString& message, const QHash<QString, QString> &messageTags);
    protected:
        //! FIXME why is this protected, and all alone down there?
        void appendAction(const QString& nick, const QString& message, const QHash<QString, QString> &messageTags);

        /// Appends a new line without any scrollback or notification checks
        void doRawAppend(const QString& newLine, bool rtl);

    public Q_SLOTS:
        void appendChannelAction(const QString& nick, const QString& message, const QHash<QString, QString> &messageTags);

        void appendServerMessage(const QString& type, const QString& message, const QHash<QString, QString> &messageTags = QHash<QString, QString>(), bool parseURL = true);
        void appendCommandMessage(const QString& command, const QString& message, const QHash<QString, QString> &messageTags, bool parseURL=true, bool self=false);
        void appendBacklogMessage(const QString& firstColumn, const QString& message);

    protected:
        void doAppend(const QString& line, bool rtl, bool self=false);

    public Q_SLOTS:
        /// Emits the doSearch signal.
        void findText();
        /// Emits the doSearchNext signal.
        void findNextText();
        /// Emits the doSearchPrevious signal.
        void findPreviousText();

        void increaseFontSize();
        void decreaseFontSize();
        void resetFontSize();

    protected Q_SLOTS:
        void highlightedSlot(const QString& link);
        void anchorClicked(const QUrl& url);

    protected:
        void openLink(const QUrl &url);

        QString filter(const QString& line, const QString& defaultColor, const QString& who=QString(), bool doHighlight=true, bool parseURL=true, bool self=false, QChar::Direction* direction = 0);

        void replaceDecoration(QString& line, char decoration, char replacement);

    private:

        /// Returns a string where all irc-richtext chars are replaced with proper
        /// html tags and all urls are parsed if parseURL is true
        inline QString ircTextToHtml(const QString& text, bool parseURL, const QString& defaultColor, const QString& whoSent, bool closeAllTags = true, QChar::Direction* direction = 0);

        /// Returns a string that closes all open html tags to <parm>tag</parm>
        /// The closed tag is removed from opentagList in data
        inline QString closeToTagString(TextHtmlData* data, const QString& tag);

        /// Returns a html open span line with given backgroundcolor style
        inline QString spanColorOpenTag(const QString& bgColor);

        /// Returns a html open font line with given foregroundcolor
        inline QString fontColorOpenTag(const QString& fgColor);

        /// Insert a string that closes as open html tags to <parm>tag</parm> and reopen the remaining ones.
        /// For the next example I will use [b] as boldchar and [i] as italic char
        /// If are currently working on text like
        /// "aa<b>bb<i>cc[b]dd[i]ee"
        /// it would generate for the next [b], "</i></b><i>".
        /// <i> is reopened as it is still relevant
        /// Returns the Length of the inserted String
        inline int defaultHtmlReplace(QString& htmlText, TextHtmlData* data, int pos, const QString& tag);

        /// Returns a string that opens all tags starting from index <parm>from</parm>
        inline QString openTags(TextHtmlData* data, int from = 0);

        /// Returns a string that closes all open tags
        /// but does not remove them from the opentaglist in data
        inline QString closeTags(TextHtmlData* data);

        /// This function looks in <parm>codes</parm> which tags it open/closes
        /// and appends/removes them from opentagList in <parm>data</parm>.
        /// This way we avoid pointless empty tags after the url like "<b></b>"
        /// The returned string consists of all codes that this function could not deal with,
        /// which is the best case empty.
        QString removeDuplicateCodes(const QString& codes, TextHtmlData* data, bool allowColors);

        /// Helperfunction for removeDuplicateCodes, for dealing with simple irc richtext
        /// chars as bold, italic, underline and strikethrou.
        /// The default behaivor is to look if the <parm>tag</parm> is already in the
        /// opentagList in <parm>data</parm> and remove it if in case if is in, or
        /// append it in case it is not.
        inline void defaultRemoveDuplicateHandling(TextHtmlData* data, const QString& tag);

        /// Changes the ranges in <parm>urlRanges</parm>, that are found in
        /// <parm>strippedText</parm>, to match in <parm>richText</parm>.
        /// This is needed for cases were the url is tainted by ircrichtext chars
        inline void adjustUrlRanges(QList< QPair< int, int > >& urlRanges, const QStringList& fixedUrls, QString& richtext, const QString& strippedText);

        /// Parses the colors in <parm>text</parm> starting from <parm>start</parm>
        /// and returns them in the given fg and bg string, as well as information
        /// if the values are valid
        inline QString getColors(const QString& text, int start, QString& _fgColor, QString& _bgColor, bool* invalidFgVal, bool* invalidBgValue);

    protected:
        void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
        void mouseReleaseEvent(QMouseEvent* ev) Q_DECL_OVERRIDE;
        void mousePressEvent(QMouseEvent* ev) Q_DECL_OVERRIDE;
        void mouseMoveEvent(QMouseEvent* ev) Q_DECL_OVERRIDE;
        void keyPressEvent(QKeyEvent* ev) Q_DECL_OVERRIDE;
        void contextMenuEvent(QContextMenuEvent* ev) Q_DECL_OVERRIDE;
        void wheelEvent(QWheelEvent* ev) Q_DECL_OVERRIDE;

        QChar::Direction basicDirection(const QString &string);

        /// Returns true if the timestamp string is RTL, otherwise false
        bool dateRtlDirection();

        /// Format the line by adding needed bidi marks
        QString formatFinalLine(bool rtl, QString lineColor, QString label, QString nickLine, QString nickStar, QString text);

        /// Returns a formated timestamp if timestamps are enabled else it returns QString::null
        QString timeStamp(QHash<QString, QString> messageTags, bool rtl);

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

        //used in ::filter
        QString m_autoTextToSend;

        //TODO FIXME light this on fire and send it sailing down an uncharted river riddled with arrows
        Konversation::TabNotifyType m_tabNotification;

        Server* m_server; //! FIXME assumes we have a server

        //// RTL hack
        static QChar LRM;
        static QChar RLM;
        static QChar LRE;
        static QChar RLE;
        static QChar RLO;
        static QChar LRO;
        static QChar PDF;

        IrcContextMenus::MenuOptions m_contextMenuOptions;
        QString m_currentNick;
        QString m_currentChannel;
        QString m_urlToCopy; ///< the URL we might be about to copy
        bool m_isOnNick; ///< context menu click hit a nickname
        bool m_isOnChannel; ///< context menu click hit a channel
        bool m_mousePressedOnUrl; ///< currently processing a mouse press
        QPoint m_mousePressPosition; ///< x,y of the click, relative to the GPS location of tip of Phantom's left ear
        QString m_dragUrl; ///< we took a stab at whatever was clicked on, may or may not actually be a URL

        //! TODO FIXME i'll bite. why do we have this in here?
        QFontDatabase m_fontDataBase;
        int m_fontSizeDelta;

        ChatWindow* m_chatWin;
        friend class IRCStyleSheet;
};
#endif
