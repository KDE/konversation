#ifndef KONV_IFACE_H
#define KONV_IFACE_H

#include <qobject.h>
#include <qstringlist.h>

#include <dcopobject.h>

#include "ignore.h"

class KonvIface : virtual public DCOPObject
{
    K_DCOP

  k_dcop:
    virtual void setAway(const QString &awaymessage) = 0;
    virtual void setAutoAway() = 0;
    virtual void setBack() = 0;
    virtual void sayToAll(const QString &message) = 0;
    virtual void actionToAll(const QString &message) = 0;

    virtual void raw(const QString& server,const QString& command) = 0;
    virtual void say(const QString& server,const QString& target,const QString& command) = 0;
    virtual void info(const QString& string) = 0;
    virtual void debug(const QString& string) = 0;
    virtual void error(const QString& string) = 0;
    virtual void insertRememberLine() = 0;
    virtual void connectToServer(const QString& url, int port, const QString& channel, const QString& password) = 0;
    virtual int registerEventHook (const QString &type, const QString &criteria, const QString &app, const QString &object, const QString &signal) = 0;
    virtual void unregisterEventHook (int id) = 0;
    virtual QString getNickname (const QString &serverid) = 0;
    virtual QStringList listServers() = 0;
    virtual QStringList listConnectedServers() = 0;
};

class KonvIdentityIface : virtual public DCOPObject
{
    K_DCOP
  k_dcop:

    virtual void setrealName(const QString &identity, const QString& name) = 0;
    virtual QString getrealName(const QString &identity) = 0;
    virtual void setIdent(const QString &identity, const QString& ident) = 0;
    virtual QString getIdent(const QString &identity) = 0;

    virtual void setNickname(const QString &identity, int index,const QString& nick) = 0;
    virtual QString getNickname(const QString &identity, int index) = 0;

    virtual void setBot(const QString &identity, const QString& bot) = 0;
    virtual QString getBot(const QString &identity) = 0;
    virtual void setPassword(const QString &identity, const QString& password) = 0;
    virtual QString getPassword(const QString &identity) = 0;

    virtual void setNicknameList(const QString &identity, const QStringList& newList) = 0;
    virtual QStringList getNicknameList(const QString &identity) = 0;

    virtual void setPartReason(const QString &identity, const QString& reason) = 0;
    virtual QString getPartReason(const QString &identity) = 0;
    virtual void setKickReason(const QString &identity, const QString& reason) = 0;
    virtual QString getKickReason(const QString &identity) = 0;

    virtual void setShowAwayMessage(const QString &identity, bool state) = 0;
    virtual bool getShowAwayMessage(const QString &identity) = 0;

    virtual void setAwayMessage(const QString &identity, const QString& message) = 0;
    virtual QString getAwayMessage(const QString &identity) = 0;
    virtual void setReturnMessage(const QString &identity, const QString& message) = 0;
    virtual QString getReturnMessage(const QString &identity) = 0;
};

class KonvPreferencesIface : virtual public DCOPObject
{
    K_DCOP
  k_dcop:

    virtual bool getAutoReconnect() = 0;
    virtual void setAutoReconnect(bool state) = 0;
    virtual bool getAutoRejoin() = 0;
    virtual void setAutoRejoin(bool state) = 0;
    virtual bool getBeep() = 0;
    virtual void setBeep(bool state) = 0;
    virtual void setLog(bool state) = 0;
    virtual bool getLog() = 0;
    virtual void setLowerLog(bool state) = 0;
    virtual bool getLowerLog() = 0;
    virtual void setLogFollowsNick(bool state) = 0;
    virtual bool getLogFollowsNick() = 0;
    virtual void setLogPath(QString path) = 0;
    virtual QString getLogPath() = 0;
    virtual void setDccAddPartner(bool state) = 0;
    virtual bool getDccAddPartner() = 0;
    virtual void setDccCreateFolder(bool state) = 0;
    virtual bool getDccCreateFolder() = 0;
    virtual void setDccMethodToGetOwnIp(int methodId) = 0;
    virtual int getDccMethodToGetOwnIp() = 0;
    virtual void setDccSpecificOwnIp(const QString& ip) = 0;
    virtual QString getDccSpecificOwnIp() = 0;
    virtual void setDccSpecificSendPorts(bool state) = 0;
    virtual bool getDccSpecificSendPorts() = 0;
    virtual void setDccSendPortsFirst(unsigned long port) = 0;
    virtual unsigned long getDccSendPortsFirst() = 0;
    virtual void setDccSendPortsLast(unsigned long port) = 0;
    virtual unsigned long getDccSendPortsLast() = 0;
    virtual void setDccSpecificChatPorts(bool state) = 0;
    virtual bool getDccSpecificChatPorts() = 0;
    virtual void setDccChatPortsFirst(unsigned long port) = 0;
    virtual unsigned long getDccChatPortsFirst() = 0;
    virtual void setDccChatPortsLast(unsigned long port) = 0;
    virtual unsigned long getDccChatPortsLast() = 0;
    virtual void setDccAutoGet(bool state) = 0;
    virtual bool getDccAutoGet() = 0;
    virtual void setDccAutoResume(bool state) = 0;
    virtual bool getDccAutoResume() = 0;
    virtual void setDccBufferSize(unsigned long size) = 0;
    virtual unsigned long getDccBufferSize() = 0;
    virtual void setDccPath(QString path) = 0;
    virtual QString getDccPath() = 0;
    virtual void setDccFastSend(bool state) = 0;
    virtual bool getDccFastSend() = 0;
    virtual void setDccSendTimeout(int sec) = 0;
    virtual int getDccSendTimeout() = 0;
    virtual void setBlinkingTabs(bool blink) = 0;
    virtual bool getBlinkingTabs() = 0;
    virtual void setBringToFront(bool state) = 0;
    virtual bool getBringToFront() = 0;
    virtual void setCloseButtonsOnTabs(bool state) = 0;
    virtual bool getCloseButtonsOnTabs() = 0;
    virtual int getNotifyDelay() = 0;
    virtual void setNotifyDelay(int delay) = 0;
    virtual bool getUseNotify() = 0;
    virtual void setUseNotify(bool use) = 0;
    virtual QMap<QString, QStringList> getNotifyList() = 0;
    virtual QStringList getNotifyListByGroup(QString groupName) = 0;
    virtual QString getNotifyStringByGroup(QString groupName) = 0;
    virtual void setNotifyList(QMap<QString, QStringList> newList) = 0;
    virtual bool addNotify(QString groupName, QString newPattern) = 0;
    virtual bool removeNotify(QString groupName, QString pattern) = 0;
    virtual void addIgnore(QString newIgnore) = 0;
    virtual void clearIgnoreList() = 0;
    //QPtrList<Ignore> getIgnoreList() = 0;
    virtual void setIgnoreList(QPtrList<Ignore> newList) = 0;
    virtual void setColor(QString colorName,QString color) = 0;

    virtual void setNickCompleteSuffixStart(QString suffix) = 0;
    virtual void setNickCompleteSuffixMiddle(QString suffix) = 0;
    virtual QString getNickCompleteSuffixStart() = 0;
    virtual QString getNickCompleteSuffixMiddle() = 0;
    virtual void setOSDUsage(bool state) =0;
    virtual bool getOSDUsage() = 0;
    virtual void setOSDOffsetX(int offset) = 0;
    virtual int getOSDOffsetX() = 0;
    virtual void setOSDOffsetY(int offset) = 0;
    virtual int getOSDOffsetY() = 0;
    virtual void setOSDAlignment(int alignment) = 0;
    virtual int getOSDAlignment() = 0;
    

};

#endif
