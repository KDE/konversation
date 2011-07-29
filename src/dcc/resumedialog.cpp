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

#include <KFileDialog>
#include <KLocale>
#include <KUrl>
#include <KUrlRequester>

namespace Konversation
{
    namespace DCC
    {
        ResumeDialog::ReceiveAction ResumeDialog::ask(TransferRecv* item, const QString& message, int enabledActions, ReceiveAction defaultAction)
        {
            QFlags<KDialog::ButtonCode> enabledButtonCodes = 0;
            KDialog::ButtonCode defaultButtonCode = KDialog::Ok;

            if(enabledActions & RA_Rename || enabledActions & RA_Overwrite)
                enabledButtonCodes |= KDialog::Ok;
            if(enabledActions & RA_Resume)
                enabledButtonCodes |= KDialog::User1;
            if(enabledActions & RA_Cancel)
                enabledButtonCodes |= KDialog::Cancel;

            if(defaultAction == RA_Rename || defaultAction == RA_Overwrite)
                defaultButtonCode = KDialog::Ok;
            else if(defaultAction == RA_Resume)
                defaultButtonCode = KDialog::User1;
            else if(defaultAction == RA_Cancel)
                defaultButtonCode = KDialog::Cancel;

            QPointer<ResumeDialog> dlg = new ResumeDialog(item, i18n("DCC Receive Question"), message, enabledActions, enabledButtonCodes, defaultButtonCode);
            dlg->exec();

            ReceiveAction ra = dlg->m_selectedAction;

            if (ra == RA_Rename)
            {
                item->setFileURL( dlg->m_urlreqFileURL->url() );
                if ((enabledActions & RA_OverwriteDefaultPath) && dlg->m_overwriteDefaultPathCheckBox->isChecked())
                {
                    Preferences::self()->setDccPath(dlg->m_urlreqFileURL->url().upUrl());
                }
            }

            delete dlg;

            return ra;
        }

        ResumeDialog::ResumeDialog(TransferRecv* item, const QString& caption, const QString& message, int enabledActions, QFlags<KDialog::ButtonCode> enabledButtonCodes,
                         KDialog::ButtonCode defaultButtonCode)
        : KDialog(0)
        , m_overwriteDefaultPathCheckBox(0)
        , m_item(item)
        , m_enabledActions(enabledActions)
        , m_selectedAction(RA_Cancel)
        {
            setCaption(caption);
            setModal(true);
            setButtons(enabledButtonCodes);
            setDefaultButton(defaultButtonCode);
            if(enabledButtonCodes & KDialog::User1)
                setButtonText(KDialog::User1, i18n("&Resume"));

            QWidget* page = mainWidget();
            QVBoxLayout* pageLayout = new QVBoxLayout(page);

            QLabel* labelMessage = new QLabel(page);
            labelMessage->setText(message);

            m_urlreqFileURL = new KUrlRequester(m_item->getFileURL().prettyUrl(), page);
            m_urlreqFileURL->setMode(KFile::File | KFile::LocalOnly);
            m_urlreqFileURL->fileDialog()->setKeepLocation(true);
            connect(m_urlreqFileURL, SIGNAL(textChanged(QString)), this, SLOT(updateDialogButtons()));

            pageLayout->addWidget(labelMessage);
            pageLayout->addWidget(m_urlreqFileURL);

            if (m_enabledActions & RA_Rename)
            {
                QFrame* filePathToolsFrame = new QFrame(page);
                QHBoxLayout* filePathToolsLayout = new QHBoxLayout(filePathToolsFrame);
                filePathToolsLayout->setSpacing(spacingHint());

                QPushButton* btnDefaultName = new QPushButton(i18n("O&riginal Filename"),filePathToolsFrame);
                QPushButton* btnSuggestNewName = new QPushButton(i18n("Suggest &New Filename"),filePathToolsFrame);
                filePathToolsLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
                filePathToolsLayout->addWidget(btnDefaultName);
                filePathToolsLayout->addWidget(btnSuggestNewName);
                connect(btnSuggestNewName, SIGNAL(clicked()), this, SLOT(suggestNewName()));
                connect(btnDefaultName, SIGNAL(clicked()), this, SLOT(setDefaultName()));

                pageLayout->addWidget(filePathToolsFrame);
            }
            if (m_enabledActions & RA_OverwriteDefaultPath)
            {
                QFrame* settingsFrame = new QFrame(page);
                QVBoxLayout* settingsLayout = new QVBoxLayout(settingsFrame);

                m_overwriteDefaultPathCheckBox = new QCheckBox(i18n("Use as new default download folder"), settingsFrame);
                settingsLayout->addWidget(m_overwriteDefaultPathCheckBox);

                pageLayout->addWidget(settingsFrame);
            }

            updateDialogButtons();
            setInitialSize(QSize(500, sizeHint().height()));
        }

        ResumeDialog::~ResumeDialog()
        {
        }

        void ResumeDialog::slotButtonClicked(int button)
        {
            switch (button)
            {
                case KDialog::Ok:
                    if (m_item->getFileURL() == m_urlreqFileURL->url())
                        m_selectedAction = RA_Overwrite;
                    else
                        m_selectedAction = RA_Rename;
                    break;
                case KDialog::User1:
                    m_selectedAction = RA_Resume;
                    accept();
                    break;
                case KDialog::Cancel:
                    m_selectedAction = RA_Cancel;
                    break;
            }
            KDialog::slotButtonClicked(button);
        }

        void ResumeDialog::updateDialogButtons() // slot
        {
            if(m_item->getFileURL() == m_urlreqFileURL->url())
            {
                setButtonText(KDialog::Ok, i18n("&Overwrite"));
                enableButton(KDialog::Ok, m_enabledActions & RA_Overwrite);
                enableButton(KDialog::User1, true);
                if (m_enabledActions & RA_OverwriteDefaultPath)
                    m_overwriteDefaultPathCheckBox->setEnabled(false);
            }
            else
            {
                setButtonText(KDialog::Ok, i18n("R&ename"));
                enableButton(KDialog::Ok, m_enabledActions & RA_Rename);
                enableButton(KDialog::User1, false);
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
            KUrl baseURL(m_urlreqFileURL->url().url().section('/', 0, -2));

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
                exists = QFileInfo( baseURL.path(KUrl::AddTrailingSlash) + suggestedName ).exists();

            m_urlreqFileURL->setUrl( QString(baseURL.path(KUrl::AddTrailingSlash) + suggestedName ));

            if ( exists ) // already exists -> recurse
                suggestNewName();
        }

        void ResumeDialog::setDefaultName() // slot
        {
            m_urlreqFileURL->setUrl(m_item->getFileURL().prettyUrl());
        }
    }
}

#include "resumedialog.moc"
