/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Shows the list of channels
  begin:     Die Apr 29 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include "channellistpanel.h"
#include <QGridLayout>
#include "channel.h"
#include "channellistviewitem.h"
#include "server.h"
#include "common.h"

#include <q3grid.h>
#include <qlabel.h>
#include <qspinbox.h>
#include <qpushbutton.h>
#include <q3hgroupbox.h>
#include <qregexp.h>
#include <qcheckbox.h>
#include <qtimer.h>

#include <krun.h>
#include <k3listview.h>
#include <klineedit.h>
#include <klocale.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kmenu.h>
#include <kdeversion.h>
#include <KMenu>
#include <kvbox.h>

ChannelListPanel::ChannelListPanel(QWidget* parent) : ChatWindow(parent)
{
    setType(ChatWindow::ChannelList);
    setName(i18n("Channel List"));

    m_oldSortColumn = 0;

    setNumChannels(0);
    setNumUsers(0);
    setVisibleChannels(0);
    setVisibleUsers(0);

    setMinUsers(0);
    setMaxUsers(0);

    setChannelTarget(true);
    setTopicTarget(false);
    setRegExp(false);

    filterTextChanged(QString());

    QGroupBox* filterGroup=new QGroupBox(i18n("Filter Settings"),this);
    QGridLayout* mainGrid=new QGridLayout( filterGroup );
    mainGrid->setSpacing(spacing());

    QLabel* minLabel=new QLabel(i18n("Minimum users:"));
    mainGrid->addWidget(minLabel,  0, 0 );
    QLabel* maxLabel=new QLabel(i18n("Maximum users:"));
    mainGrid->addWidget( maxLabel, 1, 0 );
    QSpinBox* minUsersSpin = new QSpinBox;
    minUsersSpin->setMinimum(0);
    minUsersSpin->setMaximum(9999);
    minUsersSpin->setObjectName("min_users_spin");
    minUsersSpin->setWhatsThis(i18n("You can limit the channel list to those channels with a minimum number of users here. Choosing 0 disables this criterion."));
    mainGrid->addWidget( minUsersSpin, 0, 1 );
    QSpinBox* maxUsersSpin = new QSpinBox;
    mainGrid->addWidget( maxUsersSpin, 1, 1 );
    maxUsersSpin->setMinimum(0);
    maxUsersSpin->setMaximum(9999);
    maxUsersSpin->setObjectName("max_users_spin");
    maxUsersSpin->setWhatsThis(i18n("You can limit the channel list to those channels with a maximum number of users here. Choosing 0 disables this criterion."));
    minUsersSpin->setValue(getMinUsers());
    maxUsersSpin->setValue(getMaxUsers());
    minLabel->setBuddy(minUsersSpin);
    maxLabel->setBuddy(maxUsersSpin);

    QLabel* patternLabel=new QLabel(i18n("Filter pattern:"));
    new QLabel(i18n("Filter target:" ));
    mainGrid->addWidget( patternLabel, 0, 2 );
    filterInput=new KLineEdit;
    filterInput->setObjectName("channel_list_filter_input");
    filterInput->setWhatsThis(i18n("Enter a filter string here."));
    filterInput->setText(getFilterText());
    mainGrid->addWidget( filterInput, 0, 3 );


    KHBox* targetBox=new KHBox;
    targetBox->setSpacing(spacing());

    channelFilter = new QCheckBox(i18n("Channel"), targetBox);
    channelFilter->setObjectName("filter_target_channel_check");
    topicFilter = new QCheckBox(i18n("Topic"), targetBox);
    topicFilter->setObjectName("filter_target_topic_check");
    regexpCheck = new QCheckBox(i18n("Regular expression"), targetBox);
    regexpCheck->setObjectName("regexp_check");
    applyFilter = new QPushButton(i18n("Apply Filter"), targetBox);
    applyFilter->setObjectName("apply_filter_button");
    applyFilter->setWhatsThis(i18n("Click here to retrieve the list of channels from the server and apply the filter."));
    mainGrid->addWidget( targetBox, 1, 3 );
    channelFilter->setChecked(getChannelTarget());
    topicFilter->setChecked(getTopicTarget());
    regexpCheck->setChecked(getRegExp());

    targetBox->setStretchFactor(topicFilter,10);

    channelListView=new K3ListView(this);
    channelListView->setObjectName("channel_list_view");

    channelListView->setWhatsThis(i18n("The filtered list of channels is displayed here. Notice that if you do not use regular expressions, Konversation lists any channel whose name contains the filter string you entered. The channel name does not have to start with the string you entered.\n\nSelect a channel you want to join by clicking on it. Right click on the channel to get a list of all web addresses mentioned in the channel's topic."));
    channelListView->addColumn(i18n("Channel Name"));
    channelListView->addColumn(i18n("Users"));
    channelListView->addColumn(i18n("Channel Topic"));
    channelListView->setAllColumnsShowFocus(true);
    channelListView->setResizeMode( K3ListView::LastColumn );
    channelListView->setSortColumn(-1); //Disable sorting

    KHBox* statsBox=new KHBox(this);
    statsBox->setSpacing(spacing());

    QLabel* channelsLabel=new QLabel(statsBox);
    QLabel* usersLabel=new QLabel(statsBox);

    statsBox->setStretchFactor(usersLabel,10);

    KHBox* actionBox=new KHBox(this);
    actionBox->setSpacing(spacing());

    refreshListButton = new QPushButton(i18n("Refresh List"), actionBox);
    refreshListButton->setObjectName("refresh_list_button");
    QPushButton* saveListButton=new QPushButton(i18n("Save List..."), actionBox);
    saveListButton->setObjectName("save_list_button");
    joinChannelButton = new QPushButton(i18n("Join Channel"), actionBox);
    joinChannelButton->setObjectName("join_channel_button");
    joinChannelButton->setWhatsThis(i18n("Click here to join the channel. A new tab is created for the channel."));

    connect(&updateTimer,SIGNAL (timeout()),this,SLOT (updateDisplay()));

    // double click on channel entry joins the channel
    connect(channelListView,SIGNAL (doubleClicked(Q3ListViewItem*)),
        this,SLOT (joinChannelClicked()) );

    connect(channelListView,SIGNAL (contextMenu (K3ListView*, Q3ListViewItem*, const QPoint&) ),
        this, SLOT (contextMenu (K3ListView*, Q3ListViewItem*, const QPoint&)) );

    connect(minUsersSpin,SIGNAL (valueChanged(int)),this,SLOT(setMinUsers(int)) );
    connect(maxUsersSpin,SIGNAL (valueChanged(int)),this,SLOT(setMaxUsers(int)) );
    connect(this,SIGNAL (adjustMinValue(int)),minUsersSpin,SLOT (setValue(int)) );
    connect(this,SIGNAL (adjustMaxValue(int)),maxUsersSpin,SLOT (setValue(int)) );

    connect(filterInput,SIGNAL (textChanged(const QString&)),this,SLOT (filterTextChanged(const QString&)) );
    connect(filterInput,SIGNAL (returnPressed()),this,SLOT (applyFilterClicked()) );

    connect(channelFilter,SIGNAL (clicked()),this,SLOT (channelTargetClicked()) );
    connect(topicFilter,SIGNAL (clicked()),this,SLOT (topicTargetClicked()) );
    connect(regexpCheck,SIGNAL (clicked()),this,SLOT (regExpClicked()) );

    connect(applyFilter,SIGNAL (clicked()),this,SLOT (applyFilterClicked()) );

    connect(refreshListButton,SIGNAL (clicked()),this,SLOT (refreshList()) );
    connect(saveListButton,SIGNAL (clicked()),this,SLOT (saveList()) );
    connect(joinChannelButton,SIGNAL (clicked()),this,SLOT (joinChannelClicked()) );

    connect(this,SIGNAL (updateNumUsers(const QString&)),usersLabel,SLOT (setText(const QString&)) );
    connect(this,SIGNAL (updateNumChannels(const QString&)),channelsLabel,SLOT (setText(const QString&)) );

    updateUsersChannels();
}

ChannelListPanel::~ChannelListPanel()
{
}

void ChannelListPanel::refreshList()
{
    channelListView->clear();

    setNumChannels(0);
    setNumUsers(0);
    setVisibleChannels(0);
    setVisibleUsers(0);

    updateUsersChannels();

    /* No good idea: If the server is "temporary loaded" they stay disabled :-(
      applyFilter->setEnabled(false);
      refreshListButton->setEnabled(false); */

    emit refreshChannelList();
}

void ChannelListPanel::saveList()
{
    // Ask user for file name
    QString fileName=KFileDialog::getSaveFileName(
        QString(),
        QString(),
        this,
        i18n("Save Channel List"));

    if(!fileName.isEmpty())
    {
        // first find the longest channel name and nick number for clean table layouting
        int maxChannelWidth=0;
        int maxNicksWidth=0;

        Q3ListViewItem* item = channelListView->firstChild();
        while(item)
        {
            if(item->isVisible())
            {
                if(item->text(0).length()>maxChannelWidth)
                {
                    maxChannelWidth = item->text(0).length();
                }

                if(item->text(1).length()>maxNicksWidth)
                {
                    maxNicksWidth = item->text(1).length();
                }
            }

            item = item->nextSibling();
        }

        // now save the list to disk
        QFile listFile(fileName);
        listFile.open(QIODevice::WriteOnly);
        // wrap the file into a stream
        QTextStream stream(&listFile);

        QString header(i18n("Konversation Channel List: %1 - %2\n\n",
            m_server->getServerName(),
            QDateTime::currentDateTime().toString()));

        // send header to stream
        stream << header;

        item = channelListView->firstChild();

        while(item)
        {
            if(item->isVisible())
            {
                QString channelName;
                channelName.fill(' ',maxChannelWidth);
                channelName.replace(0,item->text(0).length(),item->text(0));

                QString nicksPad;
                nicksPad.fill(' ',maxNicksWidth);
                QString nicksNum(nicksPad+item->text(1));
                nicksNum=nicksNum.right(maxNicksWidth);

                QString line(channelName+' '+nicksNum+' '+item->text(2)+'\n');

                // send final line to stream
                stream << line;
            }

            item = item->nextSibling();
        }

        listFile.close();
    }
}

void ChannelListPanel::joinChannelClicked()
{
    Q3ListViewItem* item=channelListView->selectedItem();
    if(item)
    {
        emit joinChannel(item->text(0));
    }
}

void ChannelListPanel::addToChannelList(const QString& channel,int users,const QString& topic)
{
    pendingChannels.append(channel + ' ' + QString::number(users)
            + ' ' + Konversation::removeIrcMarkup(topic));

    // set internal numbers of channels and users, display will be updated by a timer
    setNumChannels(getNumChannels()+1);
    setNumUsers(getNumUsers()+users);

    if(!updateTimer.isActive())
    {
        updateTimer.start(10);

        if(channelListView->sortColumn() != -1)
            m_oldSortColumn = channelListView->sortColumn();

        channelListView->setSortColumn(-1); //Disable sorting
    }
}

void ChannelListPanel::updateDisplay()
{
    if(!pendingChannels.isEmpty())
    {
        // fetch next channel line
        QString channelLine = pendingChannels.first();
        // split it up into the single parts we need
        QString channel = channelLine.section(' ',0,0);
        QString users = channelLine.section(' ',1,1);
        QString topic = channelLine.section(' ',2);
        ChannelListViewItem* item = new ChannelListViewItem(channelListView, channel, users, topic);
        applyFilterToItem(item);
        pendingChannels.pop_front();
    }

    if(pendingChannels.isEmpty())
    {
        updateTimer.stop();
        updateUsersChannels();
        channelListView->setSortColumn(m_oldSortColumn); //Disable sorting
        applyFilter->setEnabled(true);
        refreshListButton->setEnabled(true);
    }
}

void ChannelListPanel::filterTextChanged(const QString& newText)
{
    filterText=newText;
}

int ChannelListPanel::getNumChannels()
{
  return numChannels;
}

int ChannelListPanel::getNumUsers()
{
  return numUsers;
}

void ChannelListPanel::setNumChannels(int num)
{
  numChannels=num;
}

void ChannelListPanel::setNumUsers(int num)
{
  numUsers=num;
}

int ChannelListPanel::getVisibleChannels()
{
  return visibleChannels;
}

int ChannelListPanel::getVisibleUsers()
{
  return visibleUsers;
}

void ChannelListPanel::setVisibleChannels(int num)
{
  visibleChannels=num;
}

void ChannelListPanel::setVisibleUsers(int num)
{
  visibleUsers=num;
}

int ChannelListPanel::getMinUsers()
{
  return minUsers;
}

int ChannelListPanel::getMaxUsers()
{
  return maxUsers;
}

bool ChannelListPanel::getChannelTarget()
{
  return channelTarget;
}

bool ChannelListPanel::getTopicTarget()
{
  return topicTarget;
}

bool ChannelListPanel::getRegExp()
{
  return regExp;
}

const QString& ChannelListPanel::getFilterText()
{
  return filterText;
}

void ChannelListPanel::setMinUsers(int num)
{
    minUsers=num;
}

void ChannelListPanel::setMaxUsers(int num)
{
    maxUsers=num;
}

void ChannelListPanel::setChannelTarget(bool state)
{
  channelTarget=state;
}

void ChannelListPanel::setTopicTarget(bool state)
{
  topicTarget=state;
}

void ChannelListPanel::setRegExp(bool state)
{
  regExp=state;
}

void ChannelListPanel::channelTargetClicked()
{
  setChannelTarget(channelFilter->checkState()==Qt::Checked);
}

void ChannelListPanel::topicTargetClicked()
{
  setTopicTarget(topicFilter->checkState()==Qt::Checked);
}

void ChannelListPanel::regExpClicked()
{
  setRegExp(regexpCheck->checkState()==Qt::Checked);
}

void ChannelListPanel::applyFilterToItem(Q3ListViewItem* item)
{
    bool visible=true;

    if(getMinUsers() || getMaxUsers())
    {
        if(item->text(1).toInt()<getMinUsers() || (getMaxUsers()>=getMinUsers() &&
            item->text(1).toInt()>getMaxUsers()))
            visible=false;
    }

    if(!getFilterText().isEmpty())
    {
        if(getChannelTarget())
        {
            if(!item->text(0).contains(QRegExp(getFilterText(), Qt::CaseInsensitive, (getRegExp()) ? QRegExp::FixedString : QRegExp::Wildcard))) visible=false;
        }

        if(getTopicTarget())
        {
            if(!item->text(2).contains(QRegExp(getFilterText(), Qt::CaseInsensitive, (getRegExp()) ? QRegExp::FixedString : QRegExp::Wildcard))) visible=false;
        }
    }

    item->setVisible(visible);
    if(visible)
    {
        setVisibleUsers(getVisibleUsers()+item->text(1).toInt());
        setVisibleChannels(getVisibleChannels()+1);
    }
}

void ChannelListPanel::applyFilterClicked()
{
    if(!getNumChannels())
    {
        refreshList();
        return;
    }
    else
    {
        Q3ListViewItem* item = channelListView->firstChild();

        setVisibleChannels(0);
        setVisibleUsers(0);

        while(item)
        {
            applyFilterToItem(item);
            item = item->nextSibling();
        }

        updateUsersChannels();
    }
}

void ChannelListPanel::updateUsersChannels()
{
    emit updateNumChannels(i18n("Channels: %1 (%2 shown)", getNumChannels(), getVisibleChannels()));
    emit updateNumUsers(i18n("Non-unique users: %1 (%2 shown)", getNumUsers(), getVisibleUsers()));
}

bool ChannelListPanel::closeYourself()
{
    // make the server delete us so server can reset the pointer to us
    m_server->closeChannelListPanel();
    return true;
}

void ChannelListPanel::childAdjustFocus()
{
}

void ChannelListPanel::contextMenu (K3ListView* /* l */, Q3ListViewItem* i, const QPoint& p)
{
    if(!i) return;

    KMenu* showURLmenu = new KMenu(this);
    showURLmenu->addTitle( i18n("Open URL") );
    QString filteredLine(i->text(2));

    QRegExp pattern("((http://|https://|ftp://|nntp://|news://|gopher://|www\\.|ftp\\.)"
    // IP Address
        "([0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}|"
    // Decimal IP address
        "[0-9]{1,12}|"
    // Standard host name
        "[a-z0-9][\\.@%a-z0-9_-]+\\.[a-z]{2,}"
    // Port number, path to document
        ")(:[0-9]{1,5})?(/[^)>\"'\\s]*)?|"
    // eDonkey2000 links need special treatment
        "ed2k://\\|([^|]+\\|){4})");

    pattern.setCaseSensitivity(Qt::CaseInsensitive);

    int pos=0;
    while(static_cast<int>(pos) < filteredLine.length())
    {
        if (pattern.indexIn(filteredLine, pos) != -1)
        {
            // Remember where we found the url
            pos=pattern.pos();

            // Extract url
            QString url=pattern.capturedTexts()[0];
            QString href(url);

            // clean up href for browser
            if(href.startsWith(QLatin1String("www."))) href="http://"+href;
            else if(href.startsWith(QLatin1String("ftp."))) href="ftp://"+href;

            // Replace all spaces with %20 in href
            href.replace(' ', "%20");
            href.replace('&', "&&");

            // next search begins right after the link
            pos+=url.length();

            // tell the program that we have found a new url
            QAction* action = new QAction(showURLmenu);
            action->setText(href);
            showURLmenu->addAction(action);
            connect(action, SIGNAL(triggered()), this, SLOT(openURL()));
        }
        else
        {
            pos++;
        }
    }

    if (showURLmenu->actions().count()==1)
    {
        showURLmenu->addAction(i18n("<<No URL found>>"))->setEnabled(false);
    }

    showURLmenu->exec(p);

    delete showURLmenu;
}

void ChannelListPanel::openURL()
{
    const QAction* action = static_cast<const QAction*>(sender());

    if (action)
        new KRun(KUrl(action->text().replace("&&","&")), this);
}

void ChannelListPanel::appendInputText(const QString& text, bool fromCursor)
{
    Q_UNUSED(fromCursor);
    filterInput->setText(filterInput->text() + text);
}

//Used to disable functions when not connected
void ChannelListPanel::serverOnline(bool online)
{
    refreshListButton->setEnabled(online);
    applyFilter->setEnabled(online);
    joinChannelButton->setEnabled(online);
}

void ChannelListPanel::emitUpdateInfo()
{
    QString info;
    info = i18n("Channel List for %1", m_server->getDisplayName());
    emit updateInfo(info);
}

void ChannelListPanel::setFilter(const QString& filter)
{
    filterInput->setText(filter);
}

#include "channellistpanel.moc"
