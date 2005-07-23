#ifndef SERVER_H
#define SERVER_H

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  server.h  -  Server Class to handle connection to the IRC server
  begin:     Sun Jan 20 2002
  copyright: (C) 2002,2003,2004 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <qtimer.h>
#include <qdict.h>

#include <qdeepcopy.h>

#include <ksharedptr.h>
#include <kprocess.h>
#include <ksocketbase.h>
#include <kbufferedsocket.h>
#include <kstreamsocket.h>

#include "channelnick.h"
#include "inputfilter.h"
#include "outputfilter.h"
#include "dcctransfer.h"
#include "nickinfo.h"
#include "sslsocket.h"
#include "serversettings.h"
#include "servergroupsettings.h"

class Channel;
class Query;
class StatusPanel;
class Identity;
class KonversationMainWindow;
class RawLog;
class ChannelListPanel;
class ScriptLauncher;
class ServerISON;
class QStrList;

using namespace KNetwork;

class Server : public QObject
{
  Q_OBJECT

  public:
    typedef enum {
      SSDisconnected,
      SSConnecting,
      SSConnected
    } State;
    /** Constructor used for connecting to a known server.
     *  Read in the prefrences to get all the details about the server.
     */	  
    Server(KonversationMainWindow* mainWindow, int number);

    /** Constructor used for a 'fast connect' to a server.
     *  The details are passed in.  Used for example when the user does "/server irc.somewhere.net"
     */
    Server(KonversationMainWindow* mainWindow,const QString& hostName,const QString& port,
	   const QString& channel,const QString& nick, QString password, const bool& useSSL=FALSE);
    ~Server();

    QString getServerName() const;
    QString getServerGroup() const;

    Konversation::ServerGroupSettingsPtr serverGroupSettings() const { return m_serverGroup; }

    IdentityPtr getIdentity() const;

    bool getUseSSL() const;
    QString getSSLInfo() const;

    int getPort() const;
    int getLag() const;

    bool getAutoJoin() const;
    void setAutoJoin(bool on);
    
    /** This returns true when we have a socket connection.
     *	Not necessarily 'online' and ready for commands.
     *  @see connected()
     */
    bool isConnected() const;
    bool isConnecting() const;

    QString getAutoJoinChannel() const;
    void setAutoJoinChannel(const QString &channel);

    QString getAutoJoinChannelKey() const;
    void setAutoJoinChannelKey(const QString &key);

    void setDeliberateQuit(bool on);
    bool getDeliberateQuit() const;

    QString getNextNickname();
    
    void setIrcName(const QString &newIrcName);
    QString getIrcName() const;

    void addPendingNickList(const QString& channelName,const QStringList& nickList);
    void addHostmaskToNick(const QString &sourceNick, const QString &sourceHostmask);
    Channel* nickJoinsChannel(const QString &channelName, const QString &nickname, const QString &hostmask);
    void renameNick(const QString &nickname,const QString &newNick);
    Channel* removeNickFromChannel(const QString &channelName, const QString &nickname, const QString &reason, bool quit=false);
    void nickWasKickedFromChannel(const QString &channelName, const QString &nickname, const QString &kicker, const QString &reason);
    void removeNickFromServer(const QString &nickname, const QString &reason);
    void noMorePendingNicks(const QString& channel);

    void setChannelTypes(const QString &types);
    QString getChannelTypes() const;
    
    // extended user modes support
    void setPrefixes(const QString &modes, const QString& prefixes);
    void mangleNicknameWithModes(QString &nickname,bool& isAdmin,bool& isOwner,bool &isOp,
                                 bool& isHalfop,bool &hasVoice);

    bool isAChannel(const QString &channel) const;
    bool isNickname(const QString& compare) const;

    QString getNickname() const;
    QString loweredNickname() const;

    InputFilter* getInputFilter();
    Konversation::OutputFilter* getOutputFilter();

    void joinChannel(const QString& name, const QString& hostmask);
    void removeChannel(Channel* channel);
    void appendServerMessageToChannel(const QString& channel, const QString& type, const QString& message);
    void appendCommandMessageToChannel(const QString& channel, const QString& command, const QString& message);
    void appendStatusMessage(const QString& type,const QString& message);
    void appendMessageToFrontmost(const QString& type,const QString& message);
    

    void dcopRaw(const QString& command);
    void dcopSay(const QString& target,const QString& command);
    void dcopInfo(const QString& string);
    void ctcpReply(const QString& receiver, const QString& text);

    void setChannelTopic(const QString& channel, const QString& topic);
    void setChannelTopic(const QString& nickname, const QString& channel, const QString& topic); // Overloaded
    void updateChannelMode(const QString& nick, const QString& channel, char mode, bool plus, const QString& parameter);
    void updateChannelModeWidgets(const QString& channel, char mode, const QString& parameter);
    void updateFonts();
    void setShowQuickButtons(bool state);
    void setShowModeButtons(bool state);
    void setShowTopic(bool state);
    void setShowNicknameBox(bool state);

    Channel* getChannelByName(const QString& name);
    class Query* getQueryByName(const QString& name);
    QString parseWildcards(const QString& toParse, const QString& nickname, const QString& channelName, const QString &channelKey, const QStringList &nickList, const QString& parameter);
    QString parseWildcards(const QString& toParse, const QString& nickname, const QString& channelName, const QString &channelKey, const QString& nick, const QString& parameter);
    
    QString getAutoJoinCommand() const;
    
    void sendURIs(const QStrList& uris, const QString& nick);

    void notifyAction(const QString& nick);
    ChannelListPanel* getChannelListPanel() const;

    StatusPanel* getStatusView() const { return statusView; }

    /** This returns true when we are 'online' - ready to take commands, join channels and so on.
     */
    bool connected() const;
    QString getIp(bool followDccSetting = false);
    QString getNumericalIp(bool followDccSetting = false);
    /**
     * Returns true if the given nickname is known to be online.
     * @param nickname      The nickname.  Case insensitive.
     * @return              True if the nickname is known to be online by the server.
     * Note that a nick that is not in any of the joined channels and is not on the
     * notify list, and has not initiated a query with you, may well be online,
     * but server doesn't know if it is or not, in which case False is returned.
     */
    bool isNickOnline(const QString &nickname);
    /** Given a nickname, returns NickInfo object.
     *  @param nickname    The desired nickname.  Case insensitive.
     *  @return            Pointer to the nickinfo for this nickname if one exists.
     *                     0 if not known to be online.
     *
     *  A NickInfo pointer will only be returned if the nickname is known to the Konvi
     *  Server object.  A nick will be known if:
     *  - It is in one of the server's channels user has joined.
     *  - It is on the notify list and is known to be online.
     *  - The nick initiated a query with the user.
     *  A NickInfo is destroyed when it is offline.
     */
    NickInfoPtr getNickInfo(const QString& nickname);
    /** Given a nickname, returns an existing NickInfo object, or creates a new NickInfo object.
     *  Guaranteed to return a nickinfo.
     *  @param nickname    The desired nickname.  Case sensitive.
     *  @return            Pointer to the found or created NickInfo object. 
     */
    NickInfoPtr obtainNickInfo(const QString& nickname);
    /** Returns a list of all the NickInfos that are online and known to the server.
     * Caller should not modify the list.
     * A nick will be known if:
     *  - It is in one of the server's channels user has joined.
     *  - It is on the notify list and is known to be online.
     *  - The nick initiated a query with the user.
     *
     * @return A QMap of KSharedPtrs to NickInfos indexed by lowercase nickname.
     */
    const NickInfoMap* getAllNicks();
    /** Returns the list of members for a channel in the joinedChannels list.
     *  A joinedChannel is one that you are in, as opposed to a channel that you aren't in,
     *  but one of your watched nicks is in.
     *  Code that calls this must not modify the list.
     *  @param channelName Name of desired channel.  Case insensitive.
     *  @return            A map of all the nicks in the channel.
     *                     0 if channel is not in the joinedChannels list. 
     */
    const ChannelNickMap *getJoinedChannelMembers(const QString& channelName) const;
    /** Returns the list of members for a channel in the unjoinedChannels list.
     *  An unjoinedChannel is a channel you aren't in.  As such, this is only going to return
     *  nicks that you know are in that channel because a /whois has been done against them.
     *  This could be done automatically if they are on the watch list.
     *  Code that calls this must not modify the list.
     *  @param channelName Name of desired channel.  Case insensitive.
     *  @return            A map of only the nicks that we know that are in the channel.
     *                     0 if channel is not in the unjoinedChannels list.
     */
    const ChannelNickMap *getUnjoinedChannelMembers(const QString& channelName) const;
    /** Searches the Joined and Unjoined lists for the given channel and returns the member list.
     *  Code that calls this must not modify the list.
     *  @param channelName Name of desired channel.  Case insensitive.
     *  @return            A map of nicks in that channel.  0 if channel is not in either list.
     *
     *  @see getJoinedChannelMembers(const QString& channelName)
     *  @see getUnjoinedChannelMembers(const QString& channelName)
     */
    const ChannelNickMap *getChannelMembers(const QString& channelName) const;
    /** Returns a list of all the joined channels that a nick is in.
     *  @param nickname    The desired nickname.  Case insensitive.
     *  @return            A list of joined channels the nick is in.  Empty if none.
     */
    QStringList getNickJoinedChannels(const QString& nickname);
    /** Returns a list of all the channels (joined or unjoined) that a nick is in.
     *  @param nickname    The desired nickname.  Case insensitive.
     *  @return            A list of channels the nick is in.  Empty if none.
     *
     *  A nick will not appear in the Unjoined channels list unless a WHOIS
     *  has been performed on it.
     */
    QStringList getNickChannels(const QString& nickname);
    /** Returns pointer to the ChannelNick (mode and pointer to NickInfo) for a
     *  given channel and nickname.
     *  @param channelName The desired channel name.  Case insensitive.
     *  @param nickname    The desired nickname.  Case insensitive.
     *  @return            Pointer to ChannelNick structure containing a pointer
     *                     to the NickInfo and the mode of the nick in the channel.
     *                     0 if not found.
     */
    ChannelNickPtr getChannelNick(const QString& channelName, const QString& nickname);
    /** Updates a nickname in a channel.  If not on the joined or unjoined lists, and nick
     *  is in the watch list, adds the channel and nick to the unjoinedChannels list.
     *  If mode != 99, sets the mode for the nick in the channel.
     *  Returns the NickInfo object if nick is on any lists, otherwise 0.
     *  @param channelName The channel name.  Case sensitive.
     *  @param nickname    The nickname.  Case sensitive.
     *  @param mode        Bit mask containing the modes the nick has in the channel,
     *                     or 99 if not known.  See channelnick.cpp for bit definitions.
     */
    ChannelNickPtr setChannelNick(const QString& channelName, const QString& nickname, unsigned int mode = 99);
    /**
     * Given the nickname of nick that is offline (or at least not known to be online),
     * returns the addressbook entry (if any) for the nick.
     * @param nickname       Desired nickname.  Case insensitive.
     * @return               Addressbook entry of the nick or empty if not found.
     */
    KABC::Addressee getOfflineNickAddressee(QString& nickname);
    
    /**
     * Returns a QPtrList of all channels
     */
    QPtrList<Channel> getChannelList() const { return channelList; }
    
    /** Returns the time we have been away for.
     *  If we are not away, returns 00:00:00
     */
    QString awayTime() const;
    /** Does the _server_ think we are away.  Note that if we went auto-away when not connected to the server, this may
     *  return false.
     */
    bool isAway() const;
    /** Put the server in autoaway.  This means that when there is mouse activity, we will set to available again
     *  @see isAway
     */
    void setAutoAway();

    void emitChannelNickChanged(const ChannelNickPtr channelNick);
    void emitNickInfoChanged(const NickInfoPtr nickInfo);
    
    /**
    * Returns a list of all the nicks on the user watch list plus nicks in the addressbook.
    */
    QStringList getWatchList();
    QString getWatchListString();
    /**
    * Return true if the given nickname is on the watch list.
    */
    bool isWatchedNick(const QString& nickname);
    /**
    * Returns a list of all the nicks on the watch list that are not in joined
    * channels.  ISON command is sent for these nicks.
    */
    QStringList getISONList();
    QString getISONListString();
    
    KonversationMainWindow* getMainWindow() const;

    /** Adds a nickname to the joinedChannels list.
     *  Creates new NickInfo if necessary.
     *  If needed, moves the channel from the unjoined list to the joined list.
     *  If needed, moves the nickname from the Offline to Online lists.
     *  If mode != 99 sets the mode for this nick in this channel.
     *  @param channelName The channel name.  Case sensitive.
     *  @param nickname    The nickname.  Case sensitive.
     *  @return            The NickInfo for the nickname.
     */
    ChannelNickPtr addNickToJoinedChannelsList(const QString& channelName, const QString& nickname);

    void setAllowedChannelModes(const QString& modes) { m_allowedChannelModes = modes; }
    QString allowedChannelModes() const { return m_allowedChannelModes; }

    void registerWithServices();
    
    // Blowfish stuff
    QCString getKeyForRecepient(const QString& recepient) const;
    void setKeyForRecepient(const QString& recepient, const QCString& key);
    
  signals:
    void nicknameChanged(const QString&);
    void serverLag(Server* server,int msec); /// will be connected to KonversationMainWindow::updateLag()
    void tooLongLag(Server* server, int msec); /// will be connected to KonversationMainWindow::updateLag()
    void resetLag();
    void nicksNowOnline(Server* server,const QStringList& list,bool changed); /// Will be emitted when new 303 came in
    void addDccPanel(); /// will be connected to MainWindow::addDccPanel()
    void addKonsolePanel(); /// will be connected to MainWindow::addKonsolePanel()
    void closeDccPanel(); /// will be connected to MainWindow::closeDccPanel()
    void deleted(Server* myself); /// will be connected to KonversationApplication::removeServer()
    void awayState(bool away); /// will be connected to any user input panel;
    void multiServerCommand(const QString& command, const QString& parameter);
    
    /// Emitted when the server gains/loses connection.
    void serverOnline(bool state); /// will be connected to all server dependant tabs
    
    /// Note that these signals haven't been implemented yet.
    /// Fires when the information in a NickInfo object changes.
    void nickInfoChanged(Server* server, const NickInfoPtr nickInfo);
    /// Fires when the mode of a nick in a channel changes.
    void channelNickChanged(Server* server, const ChannelNickPtr channelNick);
    /// Fires when a nick leaves or joins a channel.  Based on joined flag, receiver could
    /// call getJoinedChannelMembers or getUnjoinedChannelMembers, or just
    /// getChannelMembers to get a list of all the nicks now in the channel.
    /// parted indicates whether the nick joined or left the channel.
    void channelMembersChanged(Server* server, const QString& channelName, bool joined, bool parted, const QString& nickname);
    /// Fires when a channel is moved to/from the Joinied/Unjoined lists.
    /// joined indicates which list it is now on.  Note that if joined is False, it is
    /// possible the channel does not exist in any list anymore.
    void channelJoinedOrUnjoined(Server* server, const QString& channelName, bool joined);
    /// Fires when a nick on the watch list goes online or offline.
    void watchedNickChanged(Server* server, const QString& nickname, bool online);
    ///Fires when the user switches his state to away and has enabled "Insert Remember Line on away" in his identity.
    void awayInsertRememberLine();
    void sslInitFailure();
    void sslConnected(Server* server);

    void connectionChangedState(Server* server, Server::State state);

  public slots:
    void lookupFinished();
    void connectToIRCServer();
    void queue(const QString &buffer);
    void queueList(const QStringList &buffer);
    void queueAt(uint pos,const QString& buffer);
    void setNickname(const QString &newNickname);
    /** This is called when we want to open a new query, or focus an existing one.
     *  @param nickInfo The nickinfo we want to open the query to.  Must exist.
     *  @param weinitiated This is whether we initiated this - did we do /query, or somebody else sending us a message.
     *  @return A pointer to a new or already-existing query.  Guaranteed to be non-null
     */
    class Query *addQuery(const NickInfoPtr & nickInfo, bool weinitiated);
    void closeQuery(const QString &name);
    void closeChannel(const QString &name);
    void quitServer();
    void requestKonsolePanel();
    void requestDccPanel();
    void requestDccChat(const QString& nickname);
    void requestCloseDccPanel();
    void requestBan(const QStringList& users,const QString& channel,const QString& option);
    void requestUnban(const QString& mask,const QString& channel);

    void addDccSend(const QString &recipient,KURL fileURL, const QString &altFileName = QString::null, uint fileSize = 0);
    void removeQuery(class Query *query);
    void startNotifyTimer(int msec=0);
    void sendJoinCommand(const QString& channelName, const QString& password = QString::null);
    void requestChannelList();
    void requestWhois(const QString& nickname);
    void requestWho(const QString& channel);
    void requestUserhost(const QString& nicks);
    void addRawLog(bool show);
    void closeRawLog();
    void addChannelListPanel();
    void addToChannelList(const QString& channel, int users, const QString& topic);
    void closeChannelListPanel();
    void updateChannelQuickButtons();
    void sendMultiServerCommand(const QString& command, const QString& parameter);
    void executeMultiServerCommand(const QString& command, const QString& parameter);
    void reconnect();
    void connectToNewServer(const QString& server, const QString& port, const QString& password);
    void showSSLDialog();
    void sendToAllChannels(const QString& text);
    void notifyTimeout();

  protected slots:

    void preShellCommandExited(KProcess*);
    void ircServerConnectionSuccess();
    void startAwayTimer();
    void lockSending();
    void unlockSending();
    void incoming();
    void processIncomingData();
    void send();
    /**
     *Because KBufferedSocket has no closed(int) signal we use this slot to call broken(0)
     */
    void closed();
    void broken(int state);
    /** This is connected to the SSLSocket failed.
     * @param reason The reason why this failed.  This is already translated, ready to show the user.
     */
    void sslError(QString reason);
    void notifyCheckTimeout();
    void connectionEstablished(const QString& ownHost);
    void notifyResponse(const QString& nicksOnline);
    void addDccGet(const QString& sourceNick,const QStringList& dccArguments);
    void requestDccSend();                           // -> to outputFilter, dccPanel
    void requestDccSend(const QString& recipient);   // -> to outputFilter
    void resumeDccGetTransfer(const QString& sourceNick,const QStringList& dccArguments);  // -> to inputFilter
    void resumeDccSendTransfer(const QString& sourceNick,const QStringList& dccArguments); // -> to inputFilter
    void dccSendRequest(const QString& recipient,const QString& fileName,const QString& address,const QString& port,unsigned long size);
    void dccResumeGetRequest(const QString& sender,const QString& fileName,const QString& port,KIO::filesize_t startAt);
    void dccGetDone(const QString& fileName, DccTransfer::DccStatus status, const QString &errorMessage);
    void dccSendDone(const QString& fileName, DccTransfer::DccStatus status, const QString &errorMessage);
    void dccStatusChanged(const DccTransfer* item);
    void away();
    void unAway();
    void scriptNotFound(const QString& name);
    void scriptExecutionError(const QString& name);
    void userhost(const QString& nick,const QString& hostmask,bool away,bool ircOp);
    void setTopicAuthor(const QString& channel,const QString& author);
    void endOfWho(const QString& target);
    void invitation(const QString& nick,const QString& channel);
    void sendToAllChannelsAndQueries(const QString& text);
    void gotOwnResolvedHostByWelcome(KResolverResults res);
    void gotOwnResolvedHostByUserhost(KResolverResults res);

  protected:
    // constants
    static const int BUFFER_LEN=513;
    
    /// Initialize the class
    void init(KonversationMainWindow* mainWindow, const QString& nick, const QString& channel);
    
    /// Initialize the timers
    void initTimers();
    
    /// Connect to the signals used in this class.
    void connectSignals();

    void setMainWindow(KonversationMainWindow* newMainWindow);

    
    void startNotifyCheckTimer();

    void autoRejoinChannels();
    
    /** Adds a nickname to the unjoinedChannels list.
     *  Creates new NickInfo if necessary.
     *  If needed, moves the channel from the joined list to the unjoined list.
     *  If needed, moves the nickname from the Offline to the Online list.
     *  If mode != 99 sets the mode for this nick in this channel.
     *  @param channelName The channel name.  Case sensitive.
     *  @param nickname    The nickname.  Case sensitive.
     *  @return            The NickInfo for the nickname.
     */
    ChannelNickPtr addNickToUnjoinedChannelsList(const QString& channelName, const QString& nickname);
    /**
     * If not already online, changes a nick to the online state by creating
     * a NickInfo for it and emits various signals and messages for it.
     * This method should only be called for nicks on the watch list.
     * @param nickname           The nickname that is online.
     * @return                   Pointer to NickInfo for nick.
     */
    NickInfoPtr setWatchedNickOnline(const QString& nickname);
    /**
    * If nickname is no longer on any channel list, or the query list, delete it altogether.
    * Call this routine only if the nick is not on the notify list or is on the notify
    * list but is known to be offline.
    * @param nickname           The nickname to be deleted.  Case insensitive.
    * @return                   True if the nickname is deleted.
    */
    bool deleteNickIfUnlisted(QString &nickname);
    /**
     * If not already offline, changes a nick to the offline state.
     * Removes it from all channels on the joined and unjoined lists.
     * If the nick is in the watch list, and went offline, emits a signal,
     * posts a Notify message, and posts a KNotify.
     * If the nick is in the addressbook, and went offline, informs addressbook of change.
     * If the nick goes offline, the NickInfo is deleted.
     *
     * @param nickname     The nickname.  Case sensitive.
     * @return             True if the nick was online.
     */
    bool setNickOffline(const QString& nickname);
    /** Remove nickname from a channel (on joined or unjoined lists).
     *  @param channelName The channel name.  Case insensitive.
     *  @param nickname    The nickname.  Case insensitive.
     */
    void removeChannelNick(const QString& channelName, const QString& nickname);
    /** Remove channel from the joined list.
     *  Nicknames in the channel are added to the unjoined list if they are in the watch list.
     *  @param channelName The channel name.  Case insensitive.
     */
    void removeJoinedChannel(const QString& channelName);
    /** Renames a nickname in all NickInfo lists.
     *  @param nickInfo    Pointer to existing NickInfo object.
     *  @param newname     New nickname for the nick.  Case sensitive.
     */
    void renameNickInfo(NickInfoPtr nickInfo, const QString& newname);

    /** Called in the server constructor if the preferences are set to run a command on a new server instance.
     *  This sets up the kprocess, runs it, and connects the signals to call preShellCommandExited when done. */
    void doPreShellCommand();
    
    unsigned int completeQueryPosition;
    unsigned int tryNickNumber;
    unsigned int reconnectCounter;

    QString bot;
    QString botPassword;

    // TODO roll these into a QMap.
    QString serverNickPrefixes;     // Prefixes used by the server to indicate a mode
    QString serverNickPrefixModes;  // if supplied: modes related to those prefixes
    QString channelPrefixes;        // prefixes that indicate channel names. defaults to RFC1459 "#&"

    bool autoJoin;
    bool autoRejoin;
    bool autoReconnect;
    bool deliberateQuit;

    QStringList connectCommands;

    QString autoJoinChannel;
    QString autoJoinChannelKey;

    KonversationMainWindow* mainWindow;

    KNetwork::KStreamSocket* m_socket;
    bool         m_tryReconnect;

    QTimer reconnectTimer;
    QTimer incomingTimer;
    QTimer outgoingTimer;
    QTimer unlockTimer;      // timeout waiting for server to send initial messages

    int timerInterval;       // flood protection

    QTimer notifyTimer;
    QTimer notifyCheckTimer; // Checks if the ISON reply needs too long
    QTime notifySent;
    QStringList notifyCache; // List of users found with ISON
    int checkTime;           // Time elapsed while waiting for server 303 response
    int currentLag;

    QString ircName;
    QCString inputBufferIncomplete;
    QStringList inputBuffer;
    QStringList outputBuffer;
    QString nickname;
    QString m_loweredNickname;
    QString ownIpByUserhost;  // RPL_USERHOST
    QString ownIpByWelcome;  // RPL_WELCOME
    QString lastDccDir;

    QPtrList<Channel> channelList;
    QPtrList<Query> queryList;

    InputFilter inputFilter;
    Konversation::OutputFilter* outputFilter;

    StatusPanel* statusView;
    RawLog* rawLog;
    ChannelListPanel* channelListPanel;

    bool m_isAway;
    bool m_isAutoAway; ///Note that this may be true, but m_isAway is false, if we go auto-away when disconnected.
    bool alreadyConnected;
    bool rejoinChannels;
    bool sendUnlocked;
    bool connecting;

    QString nonAwayNick;

    int m_awayTime;
    
    ScriptLauncher* m_scriptLauncher;

    KProcess preShellCommand;
    
  private:
    /// Helper object to construct ISON (notify) list and map offline nicks to
    /// addressbook.
    ServerISON* m_serverISON;
    /// All nicks known to this server.  Note this is NOT a list of all nicks on the server.
    /// Any nick appearing in this list is online, but may not necessarily appear in
    /// any of the joined or unjoined channel lists because a WHOIS has not yet been
    /// performed on the nick.
    NickInfoMap m_allNicks;
    /// List of membership lists for joined channels.  A "joined" channel is a channel
    /// that user has joined, i.e., a tab appears for the channel in the main window.
    ChannelMembershipMap m_joinedChannels;
    /// List of membership lists for unjoined channels.  These come from WHOIS responses.
    /// Note that this is NOT a list of all channels on the server, just those we are
    /// interested in because of nicks in the Nick Watch List.
    ChannelMembershipMap m_unjoinedChannels;
    /// List of nicks in Queries.
    NickInfoMap m_queryNicks;
    
    Konversation::ServerGroupSettingsPtr m_serverGroup;
    unsigned int m_currentServerIndex;

    QString m_allowedChannelModes;
    
    // Blowfish key map
    QMap<QString,QCString> keyMap;
};

#endif
