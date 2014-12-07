/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2004 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2004 Shintaro Matsuoka <shin@shoegazed.org>
  Copyright (C) 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#include "resumedialog.h"
#include "transferrecv.h"

#include <preferences.h>

#include <QLabel>
#include <QPointer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QUrl>

#include <KLocalizedString>
#include <KUrlRequester>

namespace Konversation
{
    namespace DCC
    {
        ResumeDialog::ReceiveAction ResumeDialog::ask(TransferRecv* item, const QString& message, int enabledActions, ReceiveAction defaultAction)
        {
            QFlags<QDialogButtonBox::StandardButton> enabledButtonCodes = QDialogButtonBox::NoButton;
            QDialogButtonBox::StandardButton defaultButtonCode = QDialogButtonBox::Ok;

            if(enabledActions & RA_Rename || enabledActions & RA_Overwrite)
                enabledButtonCodes |= QDialogButtonBox::Ok;
            if(enabledActions & RA_Resume)
                enabledButtonCodes |= QDialogButtonBox::Retry;
            if(enabledActions & RA_Cancel)
                enabledButtonCodes |= QDialogButtonBox::Cancel;

            if(defaultAction == RA_Rename || defaultAction == RA_Overwrite)
                defaultButtonCode = QDialogButtonBox::Ok;
            else if(defaultAction == RA_Resume)
                defaultButtonCode = QDialogButtonBox::Retry;
            else if(defaultAction == RA_Cancel)
                defaultButtonCode = QDialogButtonBox::Cancel;

            QPointer<ResumeDialog> dlg = new ResumeDialog(item, i18n("DCC Receive Question"), message, enabledActions, enabledButtonCodes, defaultButtonCode);
            dlg->exec();

            ReceiveAction ra = dlg->m_selectedAction;

            if (ra == RA_Rename)
            {
                item->setFileURL( dlg->m_urlreqFileURL->url() );
                if ((enabledActions & RA_OverwriteDefaultPath) && dlg->m_overwriteDefaultPathCheckBox->isChecked())
                {
                    Preferences::self()->setDccPath(KIO::upUrl(dlg->m_urlreqFileURL->url()));
                }
            }

            delete dlg;

            return ra;
        }

        ResumeDialog::ResumeDialog(TransferRecv* item, const QString& caption, const QString& message, int enabledActions, QFlags<QDialogButtonBox::StandardButton> enabledButtonCodes,
                         QDialogButtonBox::StandardButton defaultButtonCode)
        : QDialog(0)
        , m_overwriteDefaultPathCheckBox(0)
        , m_item(item)
        , m_enabledActions(enabledActions)
        , m_selectedAction(RA_Cancel)
        {
            setWindowTitle(caption);
            setModal(true);

            QVBoxLayout *mainLayout = new QVBoxLayout;
            setLayout(mainLayout);

            QLabel* labelMessage = new QLabel(this);
            labelMessage->setText(message);

            m_urlreqFileURL = new KUrlRequester(m_item->getFileURL(), this);
            m_urlreqFileURL->setMode(KFile::File | KFile::LocalOnly);
            connect(m_urlreqFileURL, &KUrlRequester::textChanged, this, &ResumeDialog::updateDialogButtons);

            mainLayout->addWidget(labelMessage);
            mainLayout->addWidget(m_urlreqFileURL);

            if (m_enabledActions & RA_Rename)
            {
                QFrame* filePathToolsFrame = new QFrame(this);
                QHBoxLayout* filePathToolsLayout = new QHBoxLayout(filePathToolsFrame);

                QPushButton* btnDefaultName = new QPushButton(i18n("O&riginal Filename"),filePathToolsFrame);
                QPushButton* btnSuggestNewName = new QPushButton(i18n("Suggest &New Filename"),filePathToolsFrame);
                filePathToolsLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
                filePathToolsLayout->addWidget(btnDefaultName);
                filePathToolsLayout->addWidget(btnSuggestNewName);
                connect(btnSuggestNewName, &QPushButton::clicked, this, &ResumeDialog::suggestNewName);
                connect(btnDefaultName, &QPushButton::clicked, this, &ResumeDialog::setDefaultName);

                mainLayout->addWidget(filePathToolsFrame);
            }
            if (m_enabledActions & RA_OverwriteDefaultPath)
            {
                QFrame* settingsFrame = new QFrame(this);
                QVBoxLayout* settingsLayout = new QVBoxLayout(settingsFrame);

                m_overwriteDefaultPathCheckBox = new QCheckBox(i18n("Use as new default download folder"), settingsFrame);
                settingsLayout->addWidget(m_overwriteDefaultPathCheckBox);

                mainLayout->addWidget(settingsFrame);
            }

            m_buttonBox = new QDialogButtonBox(enabledButtonCodes);
            QPushButton *defaultButton = m_buttonBox->button(defaultButtonCode);
            defaultButton->setDefault(true);

            if (enabledButtonCodes & QDialogButtonBox::Retry)
                m_buttonBox->button(QDialogButtonBox::Retry)->setText(i18n("&Resume"));
            mainLayout->addWidget(m_buttonBox);

            connect(m_buttonBox, &QDialogButtonBox::clicked, this, &ResumeDialog::buttonClicked);

            updateDialogButtons();
        }

        ResumeDialog::~ResumeDialog()
        {
        }

        void ResumeDialog::buttonClicked(QAbstractButton* button)
        {
            if (m_buttonBox->button(QDialogButtonBox::Ok) == button)
            {
                if (m_item->getFileURL() == m_urlreqFileURL->url())
                    m_selectedAction = RA_Overwrite;
                else
                    m_selectedAction = RA_Rename;
                accept();
            }
            else if (m_buttonBox->button(QDialogButtonBox::Cancel) == button)
            {
                m_selectedAction = RA_Cancel;
                reject();
            }
            else if (m_buttonBox->button(QDialogButtonBox::Retry) == button)
            {
                m_selectedAction = RA_Resume;
                accept();
            }
        }

        void ResumeDialog::updateDialogButtons() // slot
        {
            if(m_item->getFileURL() == m_urlreqFileURL->url())
            {
                m_buttonBox->button(QDialogButtonBox::Ok)->setText(i18n("&Overwrite"));
                m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(m_enabledActions & RA_Overwrite);

                m_buttonBox->button(QDialogButtonBox::Retry)->setEnabled(true);
                if (m_enabledActions & RA_OverwriteDefaultPath)
                    m_overwriteDefaultPathCheckBox->setEnabled(false);
            }
            else
            {
                m_buttonBox->button(QDialogButtonBox::Ok)->setText(i18n("R&ename"));
                m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(m_enabledActions & RA_Rename);

                m_buttonBox->button(QDialogButtonBox::Retry)->setEnabled(false);
                if (m_enabledActions & RA_OverwriteDefaultPath)
                    m_overwriteDefaultPathCheckBox->setEnabled(true);
            }
        }

        // FIXME: kio-fy me!
        // taken and adapted from kio::renamedlg.cpp
        void ResumeDialog::suggestNewName() // slot
        {
            QString dotSuffix, suggestedName;
            QString basename = m_urlreqFileURL->url().url().section('/', -1);
            QUrl baseURL(m_urlreqFileURL->url().url().section('/', 0, -2));

            int index = basename.indexOf( '.' );
            if ( index != -1 )
            {
                dotSuffix = basename.mid( index );
                basename.truncate( index );
            }

            int pos = basename.lastIndexOf( '_' );
            if(pos != -1 )
            {
                QString tmp = basename.mid( pos+1 );
                bool ok;
                int number = tmp.toInt( &ok );
                if ( !ok ) // ok there is no number
                {
                    suggestedName = basename + '1' + dotSuffix;
                }
                else
                {
                    // yes there's already a number behind the _ so increment it by one
                    basename.replace( pos+1, tmp.length(), QString::number(number+1) );
                    suggestedName = basename + dotSuffix;
                }
            }
            else // no underscore yet
                suggestedName = basename + "_1" + dotSuffix;

            // Check if suggested name already exists
            bool exists = false;
            // TODO: network transparency. However, using NetAccess from a modal dialog
            // could be a problem, no? (given that it uses a modal widget itself....)
            if ( baseURL.isLocalFile() )
                exists = QFileInfo(baseURL.adjusted(QUrl::StripTrailingSlash).toLocalFile() + QDir::separator() + suggestedName).exists();

            m_urlreqFileURL->setUrl(QUrl::fromLocalFile(baseURL.adjusted(QUrl::StripTrailingSlash).toLocalFile() + QDir::separator() + suggestedName));

            if ( exists ) // already exists -> recurse
                suggestNewName();
        }

        void ResumeDialog::setDefaultName() // slot
        {
            m_urlreqFileURL->setUrl(m_item->getFileURL());
        }
    }
}


