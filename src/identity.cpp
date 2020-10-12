/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2003 Dario Abatianni <eisfuchs@tigress.com>
*/

#include "identity.h"
#include "irccharsets.h"

#include <QTextCodec>


int Identity::s_availableId = 0;

Identity::Identity() : QSharedData()
{
    m_id = s_availableId;
    s_availableId++;

    init();
}

Identity::Identity(int id) : QSharedData()
{
    if (id < 0)
    {
        m_id = s_availableId;
        s_availableId++;
    }
    else
    {
        m_id = id;
    }

    init();
}

Identity::Identity(const Identity& original) : QSharedData()
{
    copy(original);
    m_id = original.id();
}

Identity::~Identity()
{
}

void Identity::init()
{
    setAuthType(QStringLiteral("saslplain"));
    setNickservNickname(QStringLiteral("nickserv"));
    setNickservCommand(QStringLiteral("identify"));

    setCodecName(Konversation::IRCCharsets::self()->encodingForLocale());

    setInsertRememberLineOnAway(false);

    setQuitReason(QStringLiteral("Konversation terminated!"));
    setPartReason(QStringLiteral("Konversation terminated!"));
    setKickReason(QStringLiteral("User terminated!"));

    setAwayMessage(QStringLiteral("Gone away for now"));

    setRunAwayCommands(false);

    setAutomaticAway(false);
    setAwayInactivity(10);
    setAutomaticUnaway(false);
}

void Identity::copy(const Identity& original)
{
    setName(original.getName());
    setRealName(original.getRealName());
    setIdent(original.getIdent());
    setNicknameList(original.getNicknameList());
    setAuthType(original.getAuthType());
    setAuthPassword(original.getAuthPassword());
    setNickservNickname(original.getNickservNickname());
    setNickservCommand(original.getNickservCommand());
    setSaslAccount(original.getSaslAccount());
    setPemClientCertFile(original.getPemClientCertFile());
    setQuitReason(original.getQuitReason());
    setPartReason(original.getPartReason());
    setKickReason(original.getKickReason());
    setInsertRememberLineOnAway(original.getInsertRememberLineOnAway());
    setRunAwayCommands(original.getRunAwayCommands());
    setAwayCommand(original.getAwayCommand());
    setAwayMessage(original.getAwayMessage());
    setAwayNickname(original.getAwayNickname());
    setReturnCommand(original.getReturnCommand());
    setAutomaticAway(original.getAutomaticAway());
    setAwayInactivity(original.getAwayInactivity());
    setAutomaticUnaway(original.getAutomaticUnaway());
    setShellCommand(original.getShellCommand());
    setCodecName(original.getCodecName());
}

void Identity::setName(const QString& newName)          { name=newName; }
QString Identity::getName() const                       { return name; }

void Identity::setRealName(const QString& name)         { realName=name; }
QString Identity::getRealName() const                   { return realName; }
void Identity::setIdent(const QString& newIdent)        { ident=newIdent; }
QString Identity::getIdent() const                      { return ident; }

void Identity::setNickname(uint index,const QString& newName) { nicknameList[index]=newName; }

QString Identity::getNickname(int index) const
{
  return nicknameList.value(index);
}

void Identity::setAuthType(const QString& authType)     { m_authType = authType; }
QString Identity::getAuthType() const                   { return m_authType; }
void Identity::setAuthPassword(const QString& authPassword) { m_authPassword = authPassword; }
QString Identity::getAuthPassword() const                   { return m_authPassword; }
void Identity::setNickservNickname(const QString& nickservNickname) { m_nickservNickname = nickservNickname; }
QString Identity::getNickservNickname() const                       { return m_nickservNickname; }
void Identity::setNickservCommand(const QString& nickservCommand) { m_nickservCommand = nickservCommand; }
QString Identity::getNickservCommand() const                      { return m_nickservCommand; }
void Identity::setSaslAccount(const QString& saslAccount) { m_saslAccount = saslAccount; }
QString Identity::getSaslAccount() const                  { return m_saslAccount; }
void Identity::setPemClientCertFile(const QUrl &url)      { m_pemClientCertFile = url; }
QUrl Identity::getPemClientCertFile() const               { return m_pemClientCertFile; }

void Identity::setQuitReason(const QString& reason)     { quitReason=reason; }
QString Identity::getQuitReason() const                 { return quitReason; }
void Identity::setPartReason(const QString& reason)     { partReason=reason; }
QString Identity::getPartReason() const                 { return partReason; }
void Identity::setKickReason(const QString& reason)     { kickReason=reason; }
QString Identity::getKickReason() const                 { return kickReason; }

void Identity::setInsertRememberLineOnAway(bool state) { insertRememberLineOnAway = state; }
bool Identity::getInsertRememberLineOnAway() const { return insertRememberLineOnAway; }

void Identity::setRunAwayCommands(bool run)             { runAwayCommands = run; }
bool Identity::getRunAwayCommands() const               { return runAwayCommands; }
void Identity::setAwayCommand(const QString& command)   { awayCommand = command; }
QString Identity::getAwayCommand() const                { return awayCommand; }
void Identity::setReturnCommand(const QString& command) { returnCommand = command; }
QString Identity::getReturnCommand() const              { return returnCommand; }

void Identity::setAutomaticAway(bool automaticAway)     { m_automaticAway = automaticAway; }
bool Identity::getAutomaticAway() const                 { return m_automaticAway; }
void Identity::setAwayInactivity(int awayInactivity)    { m_awayInactivity = awayInactivity; }
int Identity::getAwayInactivity() const                 { return m_awayInactivity; }
void Identity::setAutomaticUnaway(bool automaticUnaway) { m_automaticUnaway = automaticUnaway; }
bool Identity::getAutomaticUnaway() const               { return m_automaticUnaway; }

void Identity::setNicknameList(const QStringList& newList)
{
    nicknameList.clear();
    nicknameList = newList;
}

QStringList Identity::getNicknameList() const           { return nicknameList; }

QString Identity::getShellCommand() const { return m_shellCommand;}
void Identity::setShellCommand(const QString& command) { m_shellCommand=command;}

QTextCodec* Identity::getCodec() const                  { return m_codec; }
QString Identity::getCodecName() const                  { return m_codecName; }
void Identity::setCodecName(const QString &newCodecName)
{
    // NOTE: codecName should be based on KCharsets::availableEncodingNames() / descriptiveEncodingNames()
    // We can get a QTextCodec from QString based on them, but can't do the reverse of that.

    // never set an empty or borked codec!
    QString codecName = newCodecName;
    if (!Konversation::IRCCharsets::self()->isValidEncoding(codecName))
        codecName = Konversation::IRCCharsets::self()->encodingForLocale();

    m_codecName = codecName;
    m_codec = Konversation::IRCCharsets::self()->codecForName(codecName);

    if (!m_codec) {
        setCodecName(QStringLiteral("UTF-8"));
    }
}

void Identity::setAwayMessage(const QString& message)   { awayMessage = message; }
QString Identity::getAwayMessage() const                { return awayMessage; }

void Identity::setAwayNickname(const QString& nickname) { awayNickname = nickname; }
QString Identity::getAwayNickname() const               { return awayNickname; }
