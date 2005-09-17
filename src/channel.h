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

        void setName(const QString& newName);
        void setKey(const QString& newKey);
        QString getKey();

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

        void setPendingNicks(bool state);
        bool getPendingNicks();

        void adjustNicks(int value);
        void adjustOps(int value);
        virtual void emitUpdateInfo();

        virtual void setServer(Server* newServer);
        virtual void setIdentity(const Identity *newIdentity);

        /** get the channel topic history, including the current topic at the top,
         * and descending by age.
         * Each topic may or may not have the author that set it at the start of the string,
         * like:  "<author> topic"
         *
         * @return a list of topics this channel used to have, current at the top.
         */
        QStringList getTopicHistory();
        /** get the current channel topic
         *
         * The topic may or may not have the author that set it at the start of the string,
         * like:  "<author> topic"
         *
         * The internal variable topicAuthorUnknown stores whether the "<author>" bit is there or not.
         *
         * */
        QString getTopic();
        void setTopic(const QString& topic);
                                                  // Overloaded
        void setTopic(const QString& nickname,const QString& topic);
        void setTopicAuthor(const QString& author);

        /** get the list of modes that this channel has - e.g. {+l,+s,-m}
         *  @return All the modes that this channel has
         */
        QStringList getModeList() const { return m_modeList; }
        /** Outputs a message on the channel, and modifies the mode for a ChannelNick.
         *  @param sourceNick The server or the nick of the person that made the mode change.
         *  @param mode The mode that is changing one of v,h,o,a for voice halfop op admin
         *  @param plus True if the mode is being granted, false if it's being taken away.
         *  @param parameter This depends on what the mode change is.  In most cases it is the nickname of the person that is being given voice/op/admin etc.  See the code.
         */
        void updateMode(QString sourceNick, char mode, bool plus, const QString &parameter);
        void updateModeWidgets(char mode, bool plus, const QString &parameter);
        void updateQuickButtons(const QStringList &newButtonList);
        void updateFonts();
        void updateStyleSheet();

        virtual QString getTextInLine();
        virtual bool closeYourself();
        virtual bool canBeFrontView();
        virtual bool searchView();

        bool allowNotifications() { return m_allowNotifications; }

        ChannelNickList getSelectedChannelNicks();
        QStringList getSelectedNickList();

        virtual void setChannelEncoding(const QString& encoding);
        virtual QString getChannelEncoding();
        virtual QString getChannelEncodingDefaultDesc();

        NickListView* getNickListView() const { return nicknameListView; }

        int numberOfNicks() const { return nicks; }
        int numberOfOps() const { return ops; }

        virtual bool eventFilter(QObject* watched, QEvent* e);

        signals:
        void sendFile();
        void topicHistoryChanged();
        void modesChanged();
        void splitterMoved(Channel*);

    public slots:
        void setNickname(const QString& newNickname);
        void channelTextEntered();
        void channelPassthroughCommand();
        void sendChannelText(const QString& line);
        void showQuickButtons(bool show);
        void showModeButtons(bool show);
        void appendInputText(const QString& s);
        virtual void indicateAway(bool show);
        void showTopic(bool show);
        void scheduleAutoWho();
        void setShowNicknameBox(bool show);
        void showNicknameList(bool show);

        void setAllowNotifications(bool allow) { m_allowNotifications = allow; }

        void updateSplitters(Channel* channel);

    protected slots:
        void purgeNicks();
        void completeNick();
        void filesDropped(QDropEvent* e);
        void endCompleteNick();
        void quickButtonClicked(const QString& definition);
        void modeButtonClicked(int id,bool on);
        void channelLimitChanged();

        void popupChannelCommand(int id);         // Will be connected to IRCView::popupCommand()
        void popupCommand(int id);                // Will be connected to NickListView::popupCommand()
        void doubleClickCommand(QListViewItem*);  // Will be connected to NickListView::doubleClicked()
        // Dialogs
        void changeNickname(const QString& newNickname);
        void showOptionsDialog();
        // connected to IRCInput::textPasted() - used to handle large/multiline pastings
        void textPasted(const QString& text);
        // connected to IRCInput::sendFile()
        void sendFileMenu();
        void autoUserhost();
        void autoUserhostChanged(bool state);
        void autoWho();
        void nicknameComboboxChanged();

        void processPendingNicks();
        virtual void serverOnline(bool online);
        /** Enable/disable the mode buttons depending on whether you are op or not.
         */
        void refreshModeButtons();

        ///Request a delayed nicklist sorting
        void requestNickListSort();

        ///Sort the nicklist
        void sortNickList();

        virtual bool areIRCColorsSupported() {return true; }
        virtual bool isInsertCharacterSupported() { return true; }

    protected:
        void showEvent(QShowEvent* event);
        void initializeSplitters();
        // use with caution! does not check for duplicates
        void fastAddNickname(ChannelNickPtr channelnick);
        /** Called from ChatWindow adjustFocus */
        virtual void childAdjustFocus();

        void cycleChannel();

        int nicks;
        int ops;

        // True if nick context menu is executed from IRCView
        bool channelCommand;

        // are there still nicks to be added by /names reply?
        bool pendingNicks;

        // to take care of redraw problem if hidden
        bool quickButtonsChanged;
        bool quickButtonsState;
        bool modeButtonsChanged;
        bool modeButtonsState;
        bool splitterChanged;
        bool awayChanged;
        bool awayState;

        bool topicAuthorUnknown;

        unsigned int completionPosition;

        QSplitter* m_horizSplitter;
        QSplitter* m_vertSplitter;
        QWidget* topicWidget;
        QToolButton* m_topicButton;
        Konversation::TopicLabel* topicLine;
        QStringList m_topicHistory;
        QHBox* modeBox;

        QString key;

        ModeButton* modeT;
        ModeButton* modeN;
        ModeButton* modeS;
        ModeButton* modeI;
        ModeButton* modeP;
        ModeButton* modeM;
        ModeButton* modeK;
        ModeButton* modeL;

        KLineEdit* limit;

        NickListView* nicknameListView;
        QColor abgCache;                         // caches the alternate background color
        QHBox* commandLineBox;
        QVBox* nickListButtons;
        QComboBox* nicknameCombobox;
        QString oldNick;
        QLabel* awayLabel;
        QGrid* buttonsGrid;
        IRCInput* channelInput;

        NickChangeDialog* nickChangeDialog;
        NickList nicknameList;
        QPtrList<QuickButton> buttonList;
        QTimer userhostTimer;

        bool m_firstAutoWhoDone;
        QTimer m_whoTimer;                        // for continuous auto /WHO

        bool m_allowNotifications;

        QValueList<QStringList> m_pendingChannelNickLists;
        int m_opsToAdd;
        uint m_currentIndex;

        QTimer* m_processingTimer;
        QTimer* m_delayedSortTimer;

        QStringList m_modeList;
        ChannelNickPtr m_ownChannelNick;
};
#endif
