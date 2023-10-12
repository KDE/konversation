/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2005 Ismail Donmez <ismail@kde.org>
    SPDX-FileCopyrightText: 2006 John Tapsell <johnflux@gmail.com>
    SPDX-FileCopyrightText: 2009 Michael Kreitzer <mrgrim@gr1m.org>
*/

#include "dcc_config.h"
#include "application.h"
#include "transfermanager.h"

#include <QNetworkInterface>

using namespace Konversation;

DCC_Config::DCC_Config(QWidget *parent, const char* name) :
  QWidget(parent)
{
    setObjectName(QString::fromLatin1(name));
    setupUi(this);

    kcfg_DccPath->setMode(KFile::Directory | KFile::ExistingOnly | KFile::LocalOnly);

    languageChange();
    connect(kcfg_DccMethodToGetOwnIp, QOverload<int>::of(&QComboBox::activated), this, &DCC_Config::dccMethodChanged);
    connect(kcfg_DccUPnP, &QCheckBox::stateChanged, this, &DCC_Config::dccUPnPChanged);
    dccMethodChanged(kcfg_DccMethodToGetOwnIp->currentIndex());
    kcfg_DccBufferSize->setSuffix(ki18np(" byte", " bytes"));
    kcfg_DccSendTimeout->setSuffix(ki18np(" second", " seconds"));

    const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface& interface : interfaces) {
        kcfg_DccIPv4FallbackIface->addItem(interface.humanReadableName());
    }

#ifdef Q_OS_WIN
    //This option does nothing under windows, it just confuses the user
    //reenable if ipv6FallbackAddress is implemented for windows in dcc/dcccommon.cpp
    kcfg_DccIPv4Fallback->setEnabled(false);
    kcfg_DccIPv4FallbackIface->setEnabled(false);
#endif
}

void DCC_Config::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    kcfg_DccSpecificOwnIp->setEnabled(kcfg_DccMethodToGetOwnIp->currentIndex() == 2);
}

void DCC_Config::dccMethodChanged(int index)
{
    kcfg_DccSpecificOwnIp->setEnabled( index == 2 );
}

void DCC_Config::dccUPnPChanged(int state)
{
    DCC::TransferManager *transferManager = Application::instance()->getDccTransferManager();

    if (state == Qt::Checked && transferManager->getUPnPRouter() == nullptr)
    {
        transferManager->startupUPnP();
    }
    else if (state == Qt::Unchecked && transferManager->getUPnPRouter() != nullptr)
    {
        transferManager->shutdownUPnP();
    }
}

void DCC_Config::languageChange()
{
    kcfg_DccMethodToGetOwnIp->clear();
    kcfg_DccMethodToGetOwnIp->addItem(i18n("Network Interface"));
    kcfg_DccMethodToGetOwnIp->addItem(i18n("Reply From IRC Server"));
    kcfg_DccMethodToGetOwnIp->addItem(i18n("Specify Manually"));
}

DCC_Config::~DCC_Config()
{
}

#include "moc_dcc_config.cpp"
