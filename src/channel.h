/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
    channel.h  -  The class that controls a channel
    begin:     Wed Jan 23 2002
    copyright: (C) 2002 by Dario Abatianni
               (C) 2004 by Peter Simonsson <psn@linux.se>
    email:     eisfuchs@tigress.com
*/

#ifndef CHANNEL_H
#define CHANNEL_H

#include <qtimer.h>
#include <qstring.h>
#include "server.h"
#include "chatwindow.h"
#include "channelnick.h"

/*
  @author Dario Abatianni
*/

class QPushButton;
class QCheckBox;
class QLabel;
class QTimer;
class QListViewItem;
class QHBox;
class QStringList;
class QSplitter;
class QGrid;
class QComboBox;
class QDropEvent;
class QToolButton;

class KLineEdit;

class Nick;
class NickListView;
class QuickButton;
class ModeButton;
class IRCInput;
class NickChangeDialog;

namespace Konversation
{
    class TopicLabel;
    class ChannelOptionsDialog;
}

class NickList : public QPtrList<Nick>
{
    public:
        QString completeNick(const QString& pattern, bool& complete, QStringList& found,
            bool skipNonAlfaNum, bool caseSensitive, const QString& ownNick);
    protected:
        virtual int compareItems(QPtrCollection::Item item1, QPtrCollection::Item item2);
};

class Channel : public ChatWindow
{
    Q_OBJECT

    public:
        Channel(QWidget* parent);
        ~Channel();
//META
        virtual bool canBeFrontView();
        virtual bool searchView();

    protected slots:
        //FIXME: i doubt these ought to be slots
        virtual bool areIRCColorsSupported() { return true; }
        virtual bool isInsertCharacterSupported() { return true; }

//General administrative stuff
    public:
        void setName(const QString& newName);
        void setKey(const QString& newKey);
        QString getKey();

        virtual void setServer(Server* newServer);
        virtual void setIdentity(const Identity *newIdentity);

//Unsure of future placement and/or continued existence of these members
        int numberOfNicks() const { return nicks; }
        int numberOfOps() const { return ops; }
        virtual void setChannelEncoding(const QString& encoding);
        virtual QString getChannelEncoding();
        virtual QString getChannelEncodingDefaultDesc();

    protected:
        // use with caution! does not check for duplicates
        void fastAddNickname(ChannelNickPtr channelnick);


    public slots:
        void setNickname(const QString& newNickname);
        void scheduleAutoWho();
        void setAutoUserhost(bool state);


    protected slots:
        void autoUserhost();
        void autoWho();
        virtual void serverOnline(bool online);


//Nicklist
    public:
        ChannelNickPtr getOwnChannelNick();
        ChannelNickPtr getChannelNick(const QString &ircnick);

        void joinNickname(ChannelNickPtr channelNick);
        void removeNick(ChannelNickPtr channelNick, const QString &reason, bool quit);
        void kickNick(ChannelNickPtr channelNick, const ChannelNick &kicker, const QString &reason);
        void addNickname(ChannelNickPtr channelNick);
        void nickRenamed(const QString &oldNick, const NickInfo& channelnick);
        void addPendingNickList(const QStringList& pendingChannelNickList);
        Nick *getNickByName(const QString& lookname);
        QPtrList<Nick> getNickList();

        void adjustNicks(int value);
        void adjustOps(int value);
        virtual void emitUpdateInfo();

    protected slots:
        void purgeNicks();
        void processPendingNicks();

//Topic
    public:
        /** Get the current channel topic.
         *
         * The topic may or may not have the author that set it at the start of the string,
         * like:  "<author> topic"
         *
         * The internal variable topicAuthorUnknown stores whether the "<author>" bit is there or not.
         *
         * */
        QString getTopic();
        /** Get the channel topic history sorted in reverse chronological order.
         *
         * Each topic may or may not have the author that set it at the start of the string,
         * like:  "<author> topic"
         *
         * @return a list of topics this channel used to have, current at the top.
         */
        QStringList getTopicHistory();

        void setTopic(const QString& topic);
        void setTopic(const QString& nickname, const QString& topic);
        void setTopicAuthor(const QString& author);

    signals:
        void topicHistoryChanged();


//Modes
//TODO: the only representation of the channel limit is held in the GUI

    public:
        /// Internal - Empty the modelist
        void clearModeList();
        /// Get the list of modes that this channel has - e.g. {+l,+s,-m}
        //TODO: does this method return a list of all modes, all modes that have been changed, or all modes that are +?
        QStringList getModeList() const { return m_modeList; }

        /** Outputs a message on the channel, and modifies the mode for a ChannelNick.
         *  @param sourceNick The server or the nick of the person that made the mode change.
         *  @param mode The mode that is changing one of v,h,o,a for voice halfop op admin
         *  @param plus True if the mode is being granted, false if it's being taken away.
         *  @param parameter This depends on what the mode change is.  In most cases it is the nickname of the person that is being given voice/op/admin etc.  See the code.
         */
        void updateMode(QString sourceNick, char mode, bool plus, const QString &parameter);

    signals:
        void modesChanged();

//Bans
    public:

        void addBan(const QString& ban);
        void removeBan(const QString& ban);

        void clearBanList();
        QStringList getBanList() const { return m_BanList; }

    signals:
        void banAdded(const QString& newban);
        void banRemoved(const QString& newban);
        void banListCleared();

//Generic GUI
    public:
        virtual bool eventFilter(QObject* watched, QEvent* e);

//Specific GUI
    public:
        void updateModeWidgets(char mode, bool plus, const QString &parameter);
        void updateQuickButtons(const QStringList &newButtonList);

        /// Thunks to ircview->updateStyleSheet
        void updateStyleSheet();

        /// Get the contents of the input line.
        virtual QString getTextInLine();
        /// Sounds suspiciously like a destructor..
        virtual bool closeYourself();

        ///TODO: kill this, it has been reimplemented at the ChatWindow level
        bool allowNotifications() { return m_allowNotifications; }

        ChannelNickList getSelectedChannelNicks();
        ///TODO: this looks like a half-arsed overload.
        QStringList getSelectedNickList();

        NickListView* getNickListView() const { return nicknameListView; }

    signals:
        void sendFile();

    public slots:
        void channelTextEntered();
        void channelPassthroughCommand();
        void sendChannelText(const QString& line);

        void showQuickButtons(bool show);
        void showModeButtons(bool show);

        void appendInputText(const QString& s);
        virtual void indicateAway(bool show);
        void showTopic(bool show);
        void showNicknameBox(bool show);
        void showNicknameList(bool show);

        void setAllowNotifications(bool allow) { m_allowNotifications = allow; }

        void updateAppearance();
        virtual void lostFocus();

    protected slots:
        void completeNick(); ///< I guess this is a GUI function, might be nice to have at DCOP level though --argonel
        void endCompleteNick();
        void filesDropped(QDropEvent* e);
        void quickButtonClicked(const QString& definition);
        void modeButtonClicked(int id,bool on);
        void channelLimitChanged();

        void popupChannelCommand(int id);         ///< Connected to IRCView::popupCommand()
        void popupCommand(int id);                ///< Connected to NickListView::popupCommand()
        void doubleClickCommand(QListViewItem*);  ///< Connected to NickListView::doubleClicked()
        // Dialogs
        void changeNickname(const QString& newNickname);
        void showOptionsDialog();

        void textPasted(const QString& text); ///< connected to IRCInput::textPasted() - used to handle large/multiline pastings

        void sendFileMenu(); ///< connected to IRCInput::sendFile()
        void nicknameComboboxChanged();
        /// Enable/disable the mode buttons depending on whether you are op or not.
        void refreshModeButtons();

//only the GUI cares about sorted nicklists
        ///Request a delayed nicklist sorting
        void requestNickListSort();
        ///Sort the nicklist
        void sortNickList();

    protected:
        void showEvent(QShowEvent* event);
        void hideEvent(QHideEvent* event);
        void initializeSplitters();
        /// Called from ChatWindow adjustFocus
        virtual void childAdjustFocus();
        /// Close the channel then come back in
        void cycleChannel(); ///< TODO this is definately implemented and hooked incorrectly.

        bool channelCommand;///< True if nick context menu is executed from IRCView

        // to take care of redraw problem if hidden
        bool quickButtonsChanged;
        bool quickButtonsState;
        bool modeButtonsChanged;
        bool modeButtonsState;
        bool awayChanged;
        bool awayState;
        bool splitterChanged;
        bool topicSplitterHidden;
        bool channelSplitterHidden;

        unsigned int completionPosition;

        QSplitter* m_horizSplitter;
        QSplitter* m_vertSplitter;
        QWidget* topicWidget;
        QToolButton* m_topicButton;
        Konversation::TopicLabel* topicLine;

        //TODO: abstract these
        QHBox* modeBox;
        ModeButton* modeT;
        ModeButton* modeN;
        ModeButton* modeS;
        ModeButton* modeI;
        ModeButton* modeP;
        ModeButton* modeM;
        ModeButton* modeK;
        ModeButton* modeL;

        KLineEdit* limit; //TODO: this GUI element is the only storage for the mode

        NickListView* nicknameListView;
        QColor abgCache; ///< Caches the alternate background color for the nicklist
        QHBox* commandLineBox;
        QVBox* nickListButtons;
        QGrid* buttonsGrid;
        QComboBox* nicknameCombobox;
        QString oldNick; ///< GUI
        QLabel* awayLabel;
        IRCInput* channelInput;

        NickChangeDialog* nickChangeDialog;
        QPtrList<QuickButton> buttonList;

//Members from here to end are not GUI
        NickList nicknameList;
        QTimer userhostTimer;

        QStringList m_topicHistory;
        QStringList m_BanList;
        bool topicAuthorUnknown; ///< Stores whether the "<author>" bit is there or not.

        QString key;

        bool m_firstAutoWhoDone;
        QTimer m_whoTimer; ///< For continuous auto /WHO

        QValueList<QStringList> m_pendingChannelNickLists;
        int m_opsToAdd;
        uint m_currentIndex;

        QTimer* m_processingTimer;
        QTimer* m_delayedSortTimer;

        QStringList m_modeList;
        ChannelNickPtr m_ownChannelNick;

        bool pendingNicks; ///< are there still nicks to be added by /names reply?
        int nicks; ///< How many nicks on the channel
        int ops; ///< How many ops on the channel

        bool m_allowNotifications; ///<TODO: remove this, its been implemented on the chatwindow object

        Konversation::ChannelOptionsDialog *m_optionsDialog;
};
#endif
