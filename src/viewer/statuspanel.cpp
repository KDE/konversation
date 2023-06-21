/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2003 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2006-2008 Eike Hein <hein@kde.org>
*/

#include "statuspanel.h"
#include "application.h"
#include "ircinput.h"
#include "ircview.h"
#include "ircviewbox.h"
#include "awaylabel.h"
#include "server.h"

#include <KComboBox>
#include <KMessageBox>

#include <QLineEdit>
#include <QHBoxLayout>

using namespace Konversation;

StatusPanel::StatusPanel(QWidget* parent) : ChatWindow(parent)
{
    setType(ChatWindow::Status);

    setChannelEncodingSupported(true);

    awayChanged=false;
    awayState=false;

    // set up text view, will automatically take care of logging
    auto* ircBox = new IRCViewBox(this); // Server will be set later in setServer()
    setTextView(ircBox->ircView());

    auto* commandLineBox=new QWidget(this);
    auto* commandLineBoxLayout = new QHBoxLayout(commandLineBox);
    commandLineBoxLayout->setSpacing(spacing());
    commandLineBoxLayout->setContentsMargins(0, 0, 0, 0);

    nicknameCombobox = new KComboBox(commandLineBox);
    commandLineBoxLayout->addWidget(nicknameCombobox);
    nicknameCombobox->setEditable(true);
    nicknameCombobox->setSizeAdjustPolicy(KComboBox::AdjustToContents);
    QLineEdit* nicknameComboboxLineEdit = nicknameCombobox->lineEdit();
    nicknameComboboxLineEdit->setClearButtonEnabled(false);
    nicknameCombobox->setWhatsThis(i18n("<qt><p>This shows your current nick, and any alternatives you have set up.  If you select or type in a different nickname, then a request will be sent to the IRC server to change your nick.  If the server allows it, the new nickname will be selected.  If you type in a new nickname, you need to press 'Enter' at the end.</p><p>You can edit the alternative nicknames from the <em>Identities</em> option in the <em>Settings</em> menu.</p></qt>"));

    awayLabel=new AwayLabel(commandLineBox);
    commandLineBoxLayout->addWidget(awayLabel);
    awayLabel->hide();

    m_inputBar=new IRCInput(commandLineBox);
    commandLineBoxLayout->addWidget(m_inputBar);

    getTextView()->installEventFilter(m_inputBar);
    m_inputBar->installEventFilter(this);

    connect(getTextView(), &IRCView::gotFocus, m_inputBar, QOverload<>::of(&IRCInput::setFocus));

    connect(getTextView(),&IRCView::sendFile,this,&StatusPanel::sendFileMenu );
    connect(getTextView(), &IRCView::autoText, this, &StatusPanel::sendText);

    connect(m_inputBar, &IRCInput::submit, this, &StatusPanel::statusTextEntered);
    connect(m_inputBar, &IRCInput::textPasted, this, &StatusPanel::textPasted);
    connect(getTextView(), &IRCView::textPasted, m_inputBar, &IRCInput::paste);

    connect(nicknameCombobox, QOverload<int>::of(&KComboBox::activated),
            this, &StatusPanel::nicknameComboboxChanged);
    Q_ASSERT(nicknameCombobox->lineEdit());       //it should be editable.  if we design it so it isn't, remove these lines.
    if(nicknameCombobox->lineEdit())
        connect(nicknameCombobox->lineEdit(), &QLineEdit::editingFinished,
                this, &StatusPanel::nicknameComboboxChanged);

    updateAppearance();
}

StatusPanel::~StatusPanel()
{
}

void StatusPanel::cycle()
{
    if (m_server) m_server->cycle();
}

void StatusPanel::setNickname(const QString& newNickname)
{
    nicknameCombobox->setCurrentIndex(nicknameCombobox->findText(newNickname));
}

void StatusPanel::childAdjustFocus()
{
    m_inputBar->setFocus();
}

void StatusPanel::sendText(const QString& sendLine)
{
    // create a work copy
    QString outputAll(sendLine);

    // replace aliases and wildcards
    OutputFilter::replaceAliases(outputAll, this);

    // Send all strings, one after another
    const QStringList outList = outputAll.split(QLatin1Char('\n'));
    for (const QString& output : outList) {
        // encoding stuff is done in Server()
        Konversation::OutputFilterResult result = m_server->getOutputFilter()->parse(m_server->getNickname(), output, QString(), this);

        if(!result.output.isEmpty())
        {
            if(result.type == Konversation::PrivateMessage) msgHelper(result.typeString, result.output);
            else appendServerMessage(result.typeString, result.output);
        }
        m_server->queue(result.toServer);
    } // for
}

void StatusPanel::statusTextEntered()
{
    QString line = sterilizeUnicode(m_inputBar->toPlainText());

    m_inputBar->clear();

    if (!line.isEmpty()) sendText(line);
}

void StatusPanel::textPasted(const QString& text)
{
    if(m_server)
    {
        const QStringList multiline = text.split(QLatin1Char('\n'));
        for (QString line : multiline) {
            QString cChar(Preferences::self()->commandChar());
            // make sure that lines starting with command char get escaped
            if(line.startsWith(cChar)) line=cChar+line;
            sendText(line);
        }
    }
}

void StatusPanel::updateAppearance()
{
    if (Preferences::self()->showNicknameBox())
    {
        if (Preferences::self()->customTextFont())
            nicknameCombobox->setFont(Preferences::self()->textFont());
        else
            nicknameCombobox->setFont(QFontDatabase::systemFont(QFontDatabase::GeneralFont));

        nicknameCombobox->show();
    }
    else
        nicknameCombobox->hide();

    ChatWindow::updateAppearance();
}

void StatusPanel::setName(const QString& newName)
{
    ChatWindow::setName(newName);
    setLogfileName(newName.toLower());
}

void StatusPanel::updateName()
{
    QString newName = getServer()->getDisplayName();
    setName(newName);
    setLogfileName(newName.toLower());
}

void StatusPanel::sendFileMenu()
{
    Q_EMIT sendFile();
}

void StatusPanel::indicateAway(bool show)
{
    // QT does not redraw the label properly when they are not on screen
    // while getting hidden, so we remember the "soon to be" state here.
    if(isHidden())
    {
        awayChanged=true;
        awayState=show;
    }
    else
    {
        if(show)
            awayLabel->show();
        else
            awayLabel->hide();
    }
}

// fix Qt's broken behavior on hidden QListView pages
void StatusPanel::showEvent(QShowEvent*)
{
    if(awayChanged)
    {
        awayChanged=false;
        indicateAway(awayState);
    }
}

bool StatusPanel::canBeFrontView() const  { return true; }
bool StatusPanel::searchView() const      { return true; }

void StatusPanel::setNotificationsEnabled(bool enable)
{
    if (m_server->getServerGroup()) m_server->getServerGroup()->setNotificationsEnabled(enable);

    m_notificationsEnabled = enable;
}

bool StatusPanel::closeYourself(bool confirm)
{
    int result;

    if (confirm && !m_server->isConnected())
    {
        result = KMessageBox::warningContinueCancel(
                this,
                i18n("Do you really want to close '%1'?\n\nAll associated tabs will be closed as well.",getName()),
                i18n("Close Tab"),
                KStandardGuiItem::close(),
                KStandardGuiItem::cancel(),
                QStringLiteral("QuitServerTab"));
    }
    else
    {
        result = KMessageBox::warningContinueCancel(
            this,
            i18n("Do you want to disconnect from '%1'?\n\nAll associated tabs will be closed as well.",m_server->getServerName()),
            i18n("Disconnect From Server"),
            KGuiItem(i18n("Disconnect"), QStringLiteral("network-disconnect")),
            KStandardGuiItem::cancel(),
            QStringLiteral("QuitServerTab"));
    }

    if (result==KMessageBox::Continue)
    {
        if (m_server->getServerGroup()) m_server->getServerGroup()->setNotificationsEnabled(notificationsEnabled());
        m_server->quitServer();
        // This will delete the status view as well.
        m_server->deleteLater();
        m_server = nullptr;
        return true;
    }
    else
    {
        m_recreationScheduled = false;

        m_server->abortScheduledRecreation();
    }

    return false;
}

void StatusPanel::nicknameComboboxChanged()
{
    QString newNick=nicknameCombobox->currentText();
    oldNick=m_server->getNickname();
    if (oldNick != newNick)
    {
        nicknameCombobox->setCurrentIndex(nicknameCombobox->findText(oldNick));
        m_server->queue(QLatin1String("NICK ") + newNick);
    }
    // return focus to input line
    m_inputBar->setFocus();
}

void StatusPanel::changeNickname(const QString& newNickname)
{
    m_server->queue(QLatin1String("NICK ") + newNickname);
}

void StatusPanel::emitUpdateInfo()
{
    Q_EMIT updateInfo(getServer()->getDisplayName());
}
                                                  // virtual
void StatusPanel::setChannelEncoding(const QString& encoding)
{
    if(m_server->getServerGroup())
        Preferences::setChannelEncoding(m_server->getServerGroup()->id(), QStringLiteral(":server"), encoding);
    else
        Preferences::setChannelEncoding(m_server->getDisplayName(), QStringLiteral(":server"), encoding);
}

QString StatusPanel::getChannelEncoding() const        // virtual
{
    if(m_server->getServerGroup())
        return Preferences::channelEncoding(m_server->getServerGroup()->id(), QStringLiteral(":server"));
    return Preferences::channelEncoding(m_server->getDisplayName(), QStringLiteral(":server"));
}

                                                  // virtual
QString StatusPanel::getChannelEncodingDefaultDesc() const
{
    return i18n("Identity Default ( %1 )", getServer()->getIdentity()->getCodecName());
}

//Used to disable functions when not connected
void StatusPanel::serverOnline(bool online)
{
    //m_inputBar->setEnabled(online);
    nicknameCombobox->setEnabled(online);
}

void StatusPanel::setServer(Server* server)
{
    ChatWindow::setServer(server);
    nicknameCombobox->setModel(m_server->nickListModel());
    connect(awayLabel, &AwayLabel::unaway, m_server, &Server::requestUnaway);
    connect(awayLabel, &AwayLabel::awayMessageChanged, m_server, &Server::requestAway);
}

#include "moc_statuspanel.cpp"
