#ifndef KONV_DCOP_H
#define KONV_DCOP_H

#include <qobject.h>
#include <dcopobject.h>
#include <qptrlist.h>

#include "konviface.h"

class KonvDCOP : public QObject, virtual public KonvIface
{
  Q_OBJECT

  public:
    KonvDCOP();
    QPtrList<IRCEvent> registered_events;

    bool isIgnore (int serverid, const QString &hostmask, Ignore::Type type);
    bool isIgnore (int serverid, const QString &hostmask, int type);
    QString getNickname (int serverid);

  signals:
    void dcopSay(const QString& server,const QString& target,const QString& command);
    void dcopInfo(const QString& string);
    void dcopInsertRememberLine();
    void dcopConnectToServer(const QString& url, int port, const QString& channel, const QString& password);
    void dcopRaw(const QString& server, const QString& command);
    void dcopMultiServerRaw(const QString& command);

  public slots:
    int registerEventHook(const QString& type,const QString& criteria,const QString& app,const QString& object,const QString& signal);
    void unregisterEventHook (int id);

    void setAway(const QString &awaymessage);
    void setBack();
    void sayToAll(const QString &message);
    void actionToAll(const QString &message);
    void raw(const QString& server,const QString& command);
    void say(const QString& server,const QString& target,const QString& command);
    void info(const QString& string);
    void debug(const QString& string);
    void error(const QString& string);
    void insertRememberLine();
    void connectToServer(const QString& url, int port, const QString& channel, const QString& password);

  protected:
    int hookId;
};

class KonvIdentDCOP : public QObject, virtual public KonvIdentityIface
{
  Q_OBJECT

  public:
  KonvIdentDCOP();

  void setrealName(const QString &identity, const QString& name);
  QString getrealName(const QString &identity);
  void setIdent(const QString &identity, const QString& ident);
  QString getIdent(const QString &identity);

  void setNickname(const QString &identity, int index,const QString& nick);
  QString getNickname(const QString &identity, int index);

  void setBot(const QString &identity, const QString& bot);
  QString getBot(const QString &identity);
  void setPassword(const QString &identity, const QString& password);
  QString getPassword(const QString &identity);

  void setNicknameList(const QString &identity, const QStringList& newList);
  QStringList getNicknameList(const QString &identity);

  void setPartReason(const QString &identity, const QString& reason);
  QString getPartReason(const QString &identity);
  void setKickReason(const QString &identity, const QString& reason);
  QString getKickReason(const QString &identity);

  void setShowAwayMessage(const QString &identity, bool state);
  bool getShowAwayMessage(const QString &identity);

  void setAwayMessage(const QString &identity, const QString& message);
  QString getAwayMessage(const QString &identity);
  void setReturnMessage(const QString &identity, const QString& message);
  QString getReturnMessage(const QString &identity);

};

class KonvPrefsDCOP : public QObject, virtual public KonvPreferencesIface
{
  Q_OBJECT

  public:
  KonvPrefsDCOP ();

  bool getAutoReconnect();
  void setAutoReconnect(bool state);
  bool getAutoRejoin();
  void setAutoRejoin(bool state);
  bool getBeep();
  void setBeep(bool state);
  void setLog(bool state);
  bool getLog();
  void setLowerLog(bool state);
  bool getLowerLog();
  void setLogFollowsNick(bool state);
  bool getLogFollowsNick();
  void setLogPath(QString path);
  QString getLogPath();
  void setDccAddPartner(bool state);
  bool getDccAddPartner();
  void setDccCreateFolder(bool state);
  bool getDccCreateFolder();
  void setDccAutoGet(bool state);
  bool getDccAutoGet();
  void setDccAutoResume(bool state);
  bool getDccAutoResume();
  void setDccBufferSize(unsigned long size);
  unsigned long getDccBufferSize();
  void setDccPath(QString path);
  QString getDccPath();
  void setDccMethodToGetOwnIp(int methodId);
  int getDccMethodToGetOwnIp();
  void setDccSpecificOwnIp(const QString& ip);
  QString getDccSpecificOwnIp();
  void setDccSpecificSendPorts(bool state);
  bool getDccSpecificSendPorts();
  void setDccSendPortsFirst(unsigned long port);
  unsigned long getDccSendPortsFirst();
  void setDccSendPortsLast(unsigned long port);
  unsigned long getDccSendPortsLast();
  void setDccSpecificChatPorts(bool state);
  bool getDccSpecificChatPorts();
  void setDccChatPortsFirst(unsigned long port);
  unsigned long getDccChatPortsFirst();
  void setDccChatPortsLast(unsigned long port);
  unsigned long getDccChatPortsLast();
  void setDccFastSend(bool state);
  bool getDccFastSend();
  void setDccSendTimeout(int sec);
  int getDccSendTimeout();
  
  void setBlinkingTabs(bool blink);
  bool getBlinkingTabs();
  void setBringToFront(bool state);
  bool getBringToFront();
  void setCloseButtonsOnTabs(bool state);
  bool getCloseButtonsOnTabs();
  int getNotifyDelay();
  void setNotifyDelay(int delay);
  bool getUseNotify();
  void setUseNotify(bool use);
  QMap<QString, QStringList> getNotifyList();
  QStringList getNotifyListByGroup(QString groupName);
  QString getNotifyStringByGroup(QString groupName);
  void setNotifyList(QMap<QString, QStringList> newList);
  bool addNotify(QString groupName, QString newPattern);
  bool removeNotify(QString groupName, QString pattern);
  void addIgnore(QString newIgnore);
  void clearIgnoreList();
  //QPtrList<Ignore> getIgnoreList();
  void setColor(QString colorName,QString color);
  void setIgnoreList(QPtrList<Ignore> newList);
  void setNickCompleteSuffixStart(QString suffix);
  void setNickCompleteSuffixMiddle(QString suffix);
  QString getNickCompleteSuffixStart();
  QString getNickCompleteSuffixMiddle();
  void setOSDUsage(bool state);
  bool getOSDUsage();
  void setOSDOffsetX(int offset);
  int getOSDOffsetX();
  void setOSDOffsetY(int offset);
  int getOSDOffsetY();
  void setOSDAlignment(int alignment);
  int getOSDAlignment();
};

#endif
