/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Shows the list of channels

  Copyright (C) 2003 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2009 Travis McHenry <wordsizzle@gmail.com>
*/

#include "channellistpanel.h"
#include "channel.h"
#include "server.h"
#include "common.h"

#include <KRun>
#include <KFileDialog>
#include <KMessageBox>
#include <KMenu>

ChannelListItem::ChannelListItem( QTreeWidget * tree, QStringList & strings) : QTreeWidgetItem (tree,strings)
{
}

ChannelListItem::ChannelListItem( QTreeWidgetItem * parent, QStringList & strings) : QTreeWidgetItem (parent,strings)
{
}

bool ChannelListItem::operator<(const QTreeWidgetItem &other) const
{
    int column = treeWidget()->sortColumn();
    if (column==1)
    {
        if (text(1).toInt() >= other.text(1).toInt())
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    return text(column) < other.text(column);
}
ChannelListPanel::ChannelListPanel(QWidget* parent) : ChatWindow(parent)
{
    setType(ChatWindow::ChannelList);
    setName(i18n("Channel List"));

    m_oldSortColumn = 0;
    m_numUsers = 0;
    m_numChannels = 0;
    m_visibleUsers = 0;
    m_visibleChannels = 0;

    //UI Setup
    setupUi(this);
    m_bufferLbl->setVisible(false);
    m_buffer->setVisible(false);

    connect(&m_updateTimer, SIGNAL(timeout()), this, SLOT(updateDisplay()));
    // double click on channel entry joins the channel
    connect(m_channelListView, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
            this, SLOT(joinChannelClicked()) );

    connect(m_filterLine, SIGNAL(returnPressed()), this, SLOT(applyFilterClicked()) );

    connect(m_applyBtn, SIGNAL(clicked()), this, SLOT(applyFilterClicked()) );

    connect(m_refreshListBtn, SIGNAL(clicked()), this, SLOT(refreshList()) );
    connect(m_saveListBtn, SIGNAL(clicked()), this, SLOT(saveList()) );
    connect(m_joinChanBtn, SIGNAL(clicked()), this, SLOT(joinChannelClicked()) );

    connect(m_channelListView, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(contextMenu()) );

    updateUsersChannels();
}

ChannelListPanel::~ChannelListPanel()
{
}

void ChannelListPanel::refreshList()
{
    m_channelListView->clear();
    m_numUsers = 0;
    m_numChannels = 0;
    m_visibleUsers = 0;
    m_visibleChannels = 0;

    updateUsersChannels();

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

    if (!fileName.isEmpty())
    {
        // first find the longest channel name and nick number for clean table layouting
        int maxChannelWidth=0;
        int maxNicksWidth=0;

        QTreeWidgetItemIterator it(m_channelListView);
        while (*it)
        {
            if (!(*it)->isHidden())
            {
                if ((*it)->text(0).length()>maxChannelWidth)
                {
                    maxChannelWidth = (*it)->text(0).length();
                }

                if ((*it)->text(1).length()>maxNicksWidth)
                {
                    maxNicksWidth = (*it)->text(1).length();
                }
            }
        ++it;
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

        QTreeWidgetItemIterator it2(m_channelListView);

        while (*it2)
        {
            if (!(*it2)->isHidden())
            {
                QString channelName;
                channelName.fill(' ',maxChannelWidth);
                channelName.replace(0,(*it2)->text(0).length(),(*it2)->text(0));

                QString nicksPad;
                nicksPad.fill(' ',maxNicksWidth);
                QString nicksNum(nicksPad+(*it2)->text(1));
                nicksNum=nicksNum.right(maxNicksWidth);

                QString line(channelName+' '+nicksNum+' '+(*it2)->text(2)+'\n');

                // send final line to stream
                stream << line;
            }
            ++it2;
        }

        listFile.close();
    }
}

void ChannelListPanel::joinChannelClicked()
{
    if(!m_channelListView->selectedItems().isEmpty())
        emit joinChannel(m_channelListView->selectedItems().first()->text(0));
}

void ChannelListPanel::addToChannelList(const QString& channel,int users,const QString& topic)
{
    m_pendingChannels.append(QStringList() << channel << QString::number(users)
                                           << Konversation::removeIrcMarkup(topic));

    // set internal numbers of channels and users, display will be updated by a timer
    ++m_numChannels;
    m_numUsers += users;

    m_buffer->setMaximum(m_numChannels);

    if (!m_updateTimer.isActive())
    {
        m_updateTimer.start(10);

        if(m_channelListView->sortColumn() != -1)
        {
            m_oldSortColumn = m_channelListView->sortColumn();
            m_oldSortOrder = m_channelListView->header()->sortIndicatorOrder();
        }

        m_applyBtn->setEnabled(false);
        m_refreshListBtn->setEnabled(false);
        m_statsLabel->setText(QString());
        m_bufferLbl->setVisible(true);
        m_buffer->setVisible(true);
        m_channelListView->setSortingEnabled(false); //Disable sorting
    }
}

void ChannelListPanel::updateDisplay()
{
    if (!m_pendingChannels.isEmpty())
    {
        // fetch next channel line
        QStringList channelLine = m_pendingChannels.first();

        QTreeWidgetItem* item = new ChannelListItem(m_channelListView, channelLine);
        applyFilterToItem(item);
        m_pendingChannels.pop_front();
        m_buffer->setValue(m_pendingChannels.count());
    }

    if (m_pendingChannels.isEmpty())
    {
        m_updateTimer.stop();
        updateUsersChannels();
        m_channelListView->setSortingEnabled(true);
        m_channelListView->sortItems(m_oldSortColumn, m_oldSortOrder); //enable sorting
        m_applyBtn->setEnabled(true);
        m_refreshListBtn->setEnabled(true);

        m_bufferLbl->setVisible(false);
        m_buffer->setVisible(false);
    }
}

void ChannelListPanel::applyFilterToItem(QTreeWidgetItem* item)
{
    bool hide=false;

    int minUser = m_minUser->text().toInt();
    int maxUser = m_maxUser->text().toInt();

    if (minUser || maxUser)
    {
        if (item->text(1).toInt() < minUser || (maxUser >= minUser && item->text(1).toInt() > maxUser))
            hide=true;
    }

    if (!m_filterLine->text().isEmpty())
    {
        if (m_channelBox->isChecked())
        {
            if(!item->text(0).contains(QRegExp(m_filterLine->text(), Qt::CaseInsensitive, (m_regexBox->isChecked()) ? QRegExp::FixedString : QRegExp::Wildcard)))
                hide=true;
        }

        if (m_topicBox->isChecked())
        {
            if(!item->text(2).contains(QRegExp(m_filterLine->text(), Qt::CaseInsensitive, (m_regexBox->isChecked()) ? QRegExp::FixedString : QRegExp::Wildcard)))
                hide=true;
        }
    }

    item->setHidden(hide);
    if(!hide)
    {
        m_visibleUsers += item->text(1).toInt();
        ++m_visibleChannels;
    }
}

void ChannelListPanel::applyFilterClicked()
{
    if(!m_numChannels)
    {
        refreshList();
        return;
    }
    else
    {
        QTreeWidgetItemIterator it(m_channelListView);

        m_visibleChannels = 0;
        m_visibleUsers = 0;

        while(*it)
        {
            applyFilterToItem(*it);
            ++it;
        }

        updateUsersChannels();
    }
}

void ChannelListPanel::updateUsersChannels()
{
    m_statsLabel->setText(i18n("Channels: %1 (%2 shown)", m_numChannels, m_visibleChannels) +
                          i18n(" Non-unique users: %1 (%2 shown)", m_numUsers, m_visibleUsers));
}

bool ChannelListPanel::closeYourself()
{
    // make the server delete us so server can reset the pointer to us
    m_server->closeChannelListPanel();
    return true;
}

void ChannelListPanel::contextMenu()
{
    QTreeWidgetItem* item = m_channelListView->currentItem();

    if(!item) return;

    KMenu* menu = new KMenu(this);

    // Join Channel Action
    QAction *joinAction = new QAction(menu);
    joinAction->setText(i18n("Join Channel"));
    #if KDE_IS_VERSION(4,2,85)
    joinAction->setIcon(KIcon("irc-join-channel"));
    #else
    joinAction->setIcon(KIcon("list-add"));
    #endif
    menu->addAction(joinAction);
    connect(joinAction, SIGNAL(triggered()), this, SLOT(joinChannelClicked()));

    // Adds a separator between the Join action and the URL(s) submenu
    menu->addSeparator();

    // open URL submenu
    KMenu *showURLmenu = new KMenu("Open URL", menu);

    QString filteredLine(item->text(2));

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

    int index=0;
    while (static_cast<int>(index) < filteredLine.length())
    {
        if (pattern.indexIn(filteredLine, index) != -1)
        {
            // Remember where we found the url
            index=pattern.pos();

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
            index += url.length();

            // tell the program that we have found a new url
            QAction* action = new QAction(showURLmenu);
            action->setText(href);
            showURLmenu->addAction(action);
            connect(action, SIGNAL(triggered()), this, SLOT(openURL()));
        }
        else
        {
            index++;
        }
    }

    if (showURLmenu->actions().count()==0)
    {
        showURLmenu->setEnabled(false);
    }

    menu->addMenu(showURLmenu);
    menu->exec(QCursor::pos());

    delete menu;
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
    m_filterLine->setText(m_filterLine->text() + text);
}

//Used to disable functions when not connected
void ChannelListPanel::serverOnline(bool online)
{
    m_refreshListBtn->setEnabled(online);
    m_applyBtn->setEnabled(online);
    m_joinChanBtn->setEnabled(online);
}

void ChannelListPanel::emitUpdateInfo()
{
    QString info;
    info = i18n("Channel List for %1", m_server->getDisplayName());
    emit updateInfo(info);
}

void ChannelListPanel::setFilter(const QString& filter)
{
    m_filterLine->setText(filter);
}

#include "channellistpanel.moc"
