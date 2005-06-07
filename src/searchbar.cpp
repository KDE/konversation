/* ============================================================
 * Author: Renchi Raju <renchi@pooh.tam.uiuc.edu>
 * Date  : 2005-06-02
 * Description : 
 * 
 * Copyright 2005 by Renchi Raju

 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * ============================================================ */

#include <qlineedit.h>
#include <qtoolbutton.h>
#include <qtimer.h>
#include <qpalette.h>
#include <qaccel.h>
#include <qlabel.h>
#include <qpixmap.h>

#include <kapplication.h>
#include <kiconloader.h>

#include "searchbar.h"

SearchBar::SearchBar(QWidget* parent)
    : QHBox(parent)
{
    setMargin(4);
    
    m_hideBtn    = new QToolButton(this);
    m_lineEdit   = new QLineEdit(this);
    m_nextBtn    = new QToolButton(this);
    m_fwdBtn     = new QToolButton(this);
    m_caseSenBtn = new QToolButton(this);
    m_statusPixLabel  = new QLabel(this);
    m_statusTextLabel = new QLabel(this);

    setStretchFactor(m_hideBtn,  1);
    setStretchFactor(m_lineEdit, 4);
    setStretchFactor(m_nextBtn, 1);
    setStretchFactor(m_fwdBtn, 1);
    setStretchFactor(m_caseSenBtn, 1);
    setStretchFactor(m_statusPixLabel, 0);
    setStretchFactor(m_statusTextLabel, 3);
    
    KIconLoader* iconLoader = kapp->iconLoader();
    m_hideBtn->setIconSet(iconLoader->loadIconSet("stop", KIcon::Toolbar, 16));
    m_nextBtn->setIconSet(iconLoader->loadIconSet("next", KIcon::Toolbar, 16));

    m_hideBtn->setTextLabel("&Close");
    m_nextBtn->setTextLabel("Find &Next");
    m_fwdBtn->setTextLabel("Search &Forward");
    m_caseSenBtn->setTextLabel("&Match Case");

    m_hideBtn->setUsesTextLabel(true);
    m_nextBtn->setUsesTextLabel(true);
    m_fwdBtn->setUsesTextLabel(true);
    m_caseSenBtn->setUsesTextLabel(true);

    m_hideBtn->setTextPosition(QToolButton::BesideIcon);
    m_nextBtn->setTextPosition(QToolButton::BesideIcon);
    m_fwdBtn->setTextPosition(QToolButton::BesideIcon);
    m_caseSenBtn->setTextPosition(QToolButton::BesideIcon);

    m_hideBtn->setAutoRaise(true);
    m_nextBtn->setAutoRaise(true);
    m_fwdBtn->setAutoRaise(true);
    m_caseSenBtn->setAutoRaise(true);
    
    m_fwdBtn->setToggleButton(true);
    m_caseSenBtn->setToggleButton(true);
    
    m_nextBtn->setEnabled(false);

    m_timer = new QTimer(this);

    QAccel* accel = new QAccel(this);
    accel->connectItem( accel->insertItem(Qt::Key_Escape),
                        this, SLOT(hide()));
    
    connect(m_timer, SIGNAL(timeout()),
            SLOT(slotFind()));
    connect(m_lineEdit, SIGNAL(textChanged(const QString&)),
            SLOT(slotTextChanged()));
    connect(m_lineEdit, SIGNAL(returnPressed()),
            SLOT(slotFindNext()));
    connect(m_nextBtn, SIGNAL(clicked()),
            SLOT(slotFindNext()));
    connect(m_hideBtn, SIGNAL(clicked()),
            SLOT(hide()));
    connect(m_fwdBtn, SIGNAL(clicked()),
            SLOT(slotTextChanged()));
    connect(m_caseSenBtn, SIGNAL(clicked()),
            SLOT(slotTextChanged()));
}

SearchBar::~SearchBar()
{
    
}

void SearchBar::focusInEvent(QFocusEvent* e)
{
    QHBox::focusInEvent(e);
    m_lineEdit->setFocus();
}

void SearchBar::hide()
{
    m_timer->stop();
    QHBox::hide();
}

void SearchBar::slotTextChanged()
{
    m_timer->start(50, true);    
}

void SearchBar::slotFind()
{
    if (m_lineEdit->text().isEmpty())
    {
        unsetPalette();
        m_nextBtn->setEnabled(false);
        setStatus(QPixmap(), "");
        return;
    }
    
    m_nextBtn->setEnabled(true);
    emit signalSearchChanged(m_lineEdit->text());
}

void SearchBar::slotFindNext()
{
    if (m_lineEdit->text().isEmpty())
    {
        unsetPalette();
        m_nextBtn->setEnabled(false);
        return;
    }
    
    m_nextBtn->setEnabled(true);
    emit signalSearchNext();
}

void SearchBar::setHasMatch(bool value)
{
    QPalette pal = palette();
    pal.setColor(QPalette::Active, QColorGroup::Base,
                 value ? Qt::green : Qt::red);
    setPalette(pal);
}

void SearchBar::setStatus(const QPixmap& pix, const QString& text)
{
    m_statusPixLabel->setPixmap(pix);
    m_statusTextLabel->setText(text);    
}

QString SearchBar::pattern() const
{
    return m_lineEdit->text();
}

bool SearchBar::searchForward() const
{
    return m_fwdBtn->isOn();    
}

bool SearchBar::caseSensitive() const
{
    return m_caseSenBtn->isOn();
}

#include "searchbar.moc"
