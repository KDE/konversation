//
// C++ Implementation: dccresumedialog
//
// Description: Lets the user decide if they want to overwrite, rename or cancel the resume
//
//
// Authors: Dario Abatianni <eisfuchs@tigress.com>, (C) 2004
//          Shintaro Matsuoka <shin@shoegazed.org>, (C) 2004
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <qlabel.h>
#include <qlayout.h>
#include <qvbox.h>

#include <kdebug.h>
#include <klocale.h>
#include <kurl.h>
#include <kurlrequester.h>

#include "dcctransferrecv.h"

#include "dccresumedialog.h"

                                                  // public static
DccResumeDialog::ReceiveAction DccResumeDialog::ask(DccTransferRecv* item, const QString& message, int enabledActions, ReceiveAction defaultAction)
{
    int enabledButtonCodes = 0;
    KDialogBase::ButtonCode defaultButtonCode = KDialogBase::Ok;

    if(enabledActions & RA_Rename || enabledActions & RA_Overwrite)
        enabledButtonCodes |= KDialogBase::Ok;
    if(enabledActions & RA_Resume)
        enabledButtonCodes |= KDialogBase::User1;
    if(enabledActions & RA_Cancel)
        enabledButtonCodes |= KDialogBase::Cancel;

    if(defaultAction == RA_Rename || defaultAction == RA_Overwrite)
        defaultButtonCode = KDialogBase::Ok;
    else if(defaultAction == RA_Resume)
        defaultButtonCode = KDialogBase::User1;
    else if(defaultAction == RA_Cancel)
        defaultButtonCode = KDialogBase::Cancel;

    DccResumeDialog dlg(item, i18n("DCC Receive Question"), message, enabledActions, enabledButtonCodes, defaultButtonCode);
    dlg.exec();

    ReceiveAction ra = dlg.m_selectedAction;

    if(ra == RA_Rename)
        item->m_fileURL = dlg.m_urlreqFileURL->url();

    return ra;
}

DccResumeDialog::DccResumeDialog(DccTransferRecv* item, const QString& caption, const QString& message, int enabledActions, int enabledButtonCodes, KDialogBase::ButtonCode defaultButtonCode)
: KDialogBase(0, "dcc_resume_dialog", true, caption, enabledButtonCodes, defaultButtonCode, true)
, m_item(item)
, m_enabledActions(enabledActions)
, m_selectedAction(RA_Cancel)
{
    if(enabledButtonCodes & KDialogBase::User1)
        setButtonText(KDialogBase::User1, i18n("&Resume"));

    QVBox* page = new QVBox(this);
    setMainWidget(page);

    QLabel* labelMessage = new QLabel(page);
    labelMessage->setText(message);

    m_urlreqFileURL = new KURLRequester(m_item->getFileURL().prettyURL(), page);
    connect(m_urlreqFileURL, SIGNAL(textChanged(const QString&)), this, SLOT(updateDialogButtons()));

    if(m_enabledActions & RA_Rename)
    {
        QFrame* filePathToolsFrame = new QFrame(page);
        QHBoxLayout* filePathToolsLayout = new QHBoxLayout(filePathToolsFrame);
        filePathToolsLayout->setSpacing(spacingHint());
        QPushButton* btnDefaultName = new QPushButton(i18n("&Default"), filePathToolsFrame);
        QPushButton* btnSuggestNewName = new QPushButton(i18n("Suggest &New Name"), filePathToolsFrame);
        filePathToolsLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
        filePathToolsLayout->addWidget(btnDefaultName);
        filePathToolsLayout->addWidget(btnSuggestNewName);
        connect(btnSuggestNewName, SIGNAL(clicked()), this, SLOT(suggestNewName()));
        connect(btnDefaultName, SIGNAL(clicked()), this, SLOT(setDefaultName()));
    }

    updateDialogButtons();
    setInitialSize(QSize(500, sizeHint().height()));

}

DccResumeDialog::~DccResumeDialog()
{
}

void DccResumeDialog::slotOk()                    // slot
{
    if(m_item->getFileURL() == m_urlreqFileURL->url())
        m_selectedAction = RA_Overwrite;
    else
        m_selectedAction = RA_Rename;
    KDialogBase::slotOk();
}

void DccResumeDialog::slotUser1()                 // slot
{
    m_selectedAction = RA_Resume;
    done(KDialogBase::User1);
}

void DccResumeDialog::slotCancel()                // slot
{
    m_selectedAction = RA_Cancel;
    KDialogBase::slotCancel();
}

void DccResumeDialog::updateDialogButtons()       // slot
{
    if(m_item->getFileURL() == m_urlreqFileURL->url())
    {
        setButtonText(KDialogBase::Ok, i18n("&Overwrite"));
        enableButton(KDialogBase::Ok, m_enabledActions & RA_Overwrite);
        enableButton(KDialogBase::User1, true);
    }
    else
    {
        setButtonText(KDialogBase::Ok, i18n("R&ename"));
        enableButton(KDialogBase::Ok, m_enabledActions & RA_Rename);
        enableButton(KDialogBase::User1, false);
    }
}

// FIXME: kio-fy me!
// taken and adapted from kio::renamedlg.cpp
void DccResumeDialog::suggestNewName()            // slot
{
    QString dotSuffix, suggestedName;
    QString basename = m_urlreqFileURL->url().section("/", -1);
    KURL baseURL(m_urlreqFileURL->url().section("/", 0, -2));

    int index = basename.find( '.' );
    if ( index != -1 )
    {
        dotSuffix = basename.mid( index );
        basename.truncate( index );
    }

    int pos = basename.findRev( '_' );
    if(pos != -1 )
    {
        QString tmp = basename.mid( pos+1 );
        bool ok;
        int number = tmp.toInt( &ok );
        if ( !ok )                                // ok there is no number
        {
            suggestedName = basename + "1" + dotSuffix;
        }
        else
        {
            // yes there's already a number behind the _ so increment it by one
            basename.replace( pos+1, tmp.length(), QString::number(number+1) );
            suggestedName = basename + dotSuffix;
        }
    }
    else                                          // no underscore yet
        suggestedName = basename + "_1" + dotSuffix ;

    // Check if suggested name already exists
    bool exists = false;
    // TODO: network transparency. However, using NetAccess from a modal dialog
    // could be a problem, no? (given that it uses a modal widget itself....)
    if ( baseURL.isLocalFile() )
        exists = QFileInfo( baseURL.path(+1) + suggestedName ).exists();

    m_urlreqFileURL->setURL( baseURL.path(+1) + suggestedName );

    if ( exists )                                 // already exists -> recurse
        suggestNewName();
}

void DccResumeDialog::setDefaultName()            // slot
{
    m_urlreqFileURL->setURL(m_item->getFileURL().prettyURL());
}

#include "dccresumedialog.moc"
