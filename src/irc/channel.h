/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2002 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2004-2006, 2009 Peter Simonsson <peter.simonsson@gmail.com>
  Copyright (C) 2006-2008 Eike Hein <hein@kde.org>
*/

#ifndef CHANNEL_H
#define CHANNEL_H

#include <config-konversation.h>

#include "server.h"
#include "chatwindow.h"
#include "channelnick.h"

#ifdef HAVE_QCA2
#include "cipher.h"
#endif

#include <QTimer>
#include <QString>


class QLabel;
class QTimer;
class QTreeWidgetItem;
class QStringList;
class QSplitter;
class QToolButton;

class KLineEdit;
class KComboBox;

class AwayLabel;
class NickListView;
class Nick;
class QuickButton;
class ModeButton;
class IRCInput;
class TopicHistoryModel;

namespace Konversation
{
    class TopicLabel;
    class ChannelOptionsDialog;
}

class NickList : public QList<Nick*>
{
    public:
        NickList();

        QString completeNick(const QString& pattern, bool& complete, QStringList& found,
                             bool skipNonAlfaNum, bool caseSensitive);

        bool containsNick(const QString& nickname);

};

class Channel : public ChatWindow
{
    Q_OBJECT

    friend class Nick;

    public:
        explicit Channel(QWidget* parent, const QString& name);
        ~Channel();
//META

        virtual bool canBeFrontView();
        virtual bool searchView();

        virtual void append(const QString& nickname,const QString& message);
        virtual void appendAction(const QString& nickname,const QString& message);
        void nickActive(const QString& nickname);
        #ifdef HAVE_QCA2
        Konversation::Cipher* getCipher();
        #endif
//General administrative stuff
    public:
        void setName(const QString& newName);
        QString getPassword();

        const Konversation::ChannelSettings channelSettings();

        QString getPassword() const;

        virtual void setServer(Server* newServer);

        void setEncryptedOutput(bool);

        bool joined() const { return m_joined; }
        bool rejoinable();
//Unsure of future placement and/or continued existence of these members
        int numberOfNicks() const { return nicks; }
        int numberOfOps() const { return ops; }
        virtual void setChannelEncoding(const QString& encoding);
        virtual QString getChannelEncoding();
        virtual QString getChannelEncodingDefaultDesc();

        virtual bool log();

    protected:
        // use with caution! does not check for duplicates
        void fastAddNickname(ChannelNickPtr channelnick, Nick *nick=0);
        void setActive(bool active);
        void repositionNick(Nick *nick);
        bool shouldShowEvent(ChannelNickPtr channelNick);

    public Q_SLOTS:
        void setNickname(const QString& newNickname);
        void scheduleAutoWho(int msec = -1);
        void setAutoUserhost(bool state);
        void rejoin();

    protected Q_SLOTS:
        void autoUserhost();
        void autoWho();
        void updateAutoWho();
        void fadeActivity();
        virtual void serverOnline(bool online);
        void delayedSortNickList();


//Nicklist
    public:
        void flushNickQueue();

        ChannelNickPtr getOwnChannelNick() const;
        ChannelNickPtr getChannelNick(const QString &ircnick) const;

        void joinNickname(ChannelNickPtr channelNick);
        void removeNick(ChannelNickPtr channelNick, const QString &reason, bool quit);
        void kickNick(ChannelNickPtr channelNick, const QString &kicker, const QString &reason);
        void addNickname(ChannelNickPtr channelNick);
        void nickRenamed(const QString &oldNick, const NickInfo& channelnick);
        void queueNicks(const QStringList& nicknameList);
        void endOfNames();
        Nick *getNickByName(const QString& lookname) const;
        NickList getNickList() const { return nicknameList; }

        void adjustNicks(int value);
        void adjustOps(int value);
        virtual void emitUpdateInfo();

        void resizeNicknameListViewColumns();

    protected Q_SLOTS:
        void purgeNicks();
        void processQueuedNicks(bool flush = false);

        void updateNickInfos();
        void updateChannelNicks(const QString& channel);
//Topic
    public:
        QString getTopic();
        TopicHistoryModel* getTopicHistory() { return m_topicHistory; };

        void setTopic(const QString& text);
        void setTopic(const QString& nickname, const QString& text);
        void setTopicAuthor(const QString& author, QDateTime timestamp);

    Q_SIGNALS:
        void joined(Channel* channel);


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
        void updateMode(const QString& sourceNick, char mode, bool plus, const QString &parameter);

    Q_SIGNALS:
        void modesChanged();

//Bans
    public:

        void addBan(const QString& ban);
        void removeBan(const QString& ban);

        void clearBanList();
        QStringList getBanList() const { return m_BanList; }

    Q_SIGNALS:
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

        /// Sounds suspiciously like a destructor..
        virtual bool closeYourself(bool askForConfirmation=true);

        bool autoJoin();

        QStringList getSelectedNickList();

        NickListView* getNickListView() const { return nicknameListView; }

        Konversation::ChannelSettings channelSettings() const;

    Q_SIGNALS:
        void sendFile();

    public Q_SLOTS:
        void updateAppearance();
        void channelTextEntered();
        void channelPassthroughCommand();
        void sendText(const QString& line);
        void showOptionsDialog();
        void showQuickButtons(bool show);
        void showModeButtons(bool show);

        virtual void indicateAway(bool show);
        void showTopic(bool show);
        void showNicknameBox(bool show);
        void showNicknameList(bool show);

        void setAutoJoin(bool autojoin);

        void connectionStateChanged(Server*, Konversation::ConnectionState);

    protected Q_SLOTS:
        void completeNick(); ///< I guess this is a GUI function, might be nice to have at DCOP level though --argonel
        void endCompleteNick();
        void quickButtonClicked(const QString& definition);
        void modeButtonClicked(int id,bool on);
        void channelLimitChanged();

        void doubleClickCommand(QTreeWidgetItem *item,int column);  ///< Connected to NickListView::itemDoubleClicked()
        // Dialogs
        void changeNickname(const QString& newNickname);

        void textPasted(const QString& text); ///< connected to IRCInput::textPasted() - used to handle large/multiline pastings

        void sendFileMenu(); ///< connected to IRCInput::sendFile()
        void nicknameComboboxChanged();
        /// Enable/disable the mode buttons depending on whether you are op or not.
        void refreshModeButtons();

//only the GUI cares about sorted nicklists
        ///Request a delayed nicklist sorting
        void requestNickListSort();
        ///Sort the nicklist
        void sortNickList(bool delayed=false);

        void nicknameListViewTextChanged(int textChangedFlags);
    protected:
        void showEvent(QShowEvent* event);
        void syncSplitters();
        /// Called from ChatWindow adjustFocus
        virtual void childAdjustFocus();

        // to take care of redraw problem if hidden
        bool quickButtonsChanged;
        bool quickButtonsState;
        bool modeButtonsChanged;
        bool modeButtonsState;
        bool awayChanged;
        bool awayState;
        bool splittersInitialized;
        bool topicSplitterHidden;
        bool channelSplitterHidden;

        int completionPosition;

        QSplitter* m_horizSplitter;
        QSplitter* m_vertSplitter;
        QWidget* topicWidget;
        QToolButton* m_topicButton;
        Konversation::TopicLabel* topicLine;

        //TODO: abstract these
        QFrame* modeBox;
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
        QFrame* commandLineBox;
        QFrame* nickListButtons;
        QWidget* m_buttonsGrid;
        KComboBox* nicknameCombobox;
        QString oldNick; ///< GUI
        AwayLabel* awayLabel;
        QLabel* cipherLabel;

        QList<QuickButton*> buttonList;

//Members from here to end are not GUI
        bool m_joined;
        NickList nicknameList;
        QTimer userhostTimer;
        int m_nicknameListViewTextChanged;
        QHash<QString, Nick*> m_nicknameNickHash;

        TopicHistoryModel* m_topicHistory;
        QStringList m_BanList;

        QTimer m_whoTimer; ///< For continuous auto /WHO
        QTime  m_whoTimerStarted;

        QTimer m_fadeActivityTimer; ///< For the smoothing function used in activity sorting

        QStringList m_nickQueue;
        int m_processedNicksCount;
        int m_processedOpsCount;
        bool m_initialNamesReceived;

        QTimer* m_delayedSortTimer;
        int m_delayedSortTrigger;

        QStringList m_modeList;
        ChannelNickPtr m_ownChannelNick;

        bool pendingNicks; ///< are there still nicks to be added by /names reply?
        int nicks; ///< How many nicks on the channel
        int ops; ///< How many ops on the channel

        Konversation::ChannelOptionsDialog *m_optionsDialog;
        #ifdef HAVE_QCA2
        Konversation::Cipher *m_cipher;
        #endif
};
#endif
