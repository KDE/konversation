/***************************************************************************
    colorconfiguration.h  -  Color configuration dialog
    -------------------
    begin                : Sat Jul 20 2002
    copyright            : (C) 2002 by Matthias Gierlings
    email                : gismore@users.sourceforge.net
    redesign             : Wed Feb 19 2003
    by                   : eisfuchs@tigress.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef COLORCONFIGURATION_H
#define COLORCONFIGURATION_H

#include <kdialogbase.h>

#include <qlabel.h>
#include <qwidget.h>
#include <qcolor.h>
#include <qsize.h>
#include <qstring.h>

/**
  *@author Matthias Gierlings
  */

class ColorConfiguration : public KDialogBase
{
  Q_OBJECT

  public:
    ColorConfiguration(QString passed_actionTextColor, QString passed_backlogTextColor,
                       QString passed_channelTextColor, QString passed_commandTextColor,
                       QString passed_linkTextColor, QString passed_queryTextColor,
                       QString passed_serverTextColor, QString passed_timeColor,
                       QString passed_background,
                       QSize passed_windowSize);

    ~ColorConfiguration();

  protected:
    QColor        channelTextColor, queryTextColor, serverTextColor, actionTextColor, backlogTextColor,
                  commandTextColor, linkTextColor, timeColor, backgroundColor;

  signals:
    void closeFontColorConfiguration(QSize windowSize);
    void saveFontColorSettings(QString channelTextColor, QString queryTextColor, QString serverTextColor,
                               QString actionTextColor, QString backlogTextColor, QString commandTextColor,
                               QString linkTextColor, QString timeColor, QString backgroundColor);

  protected slots:
    void closeEvent(QCloseEvent *ev);

    void setChannelTextColor(const QColor& passed_color) {channelTextColor = passed_color;}
    void setQueryTextColor(const QColor& passed_color) {queryTextColor = passed_color;}
    void setServerTextColor(const QColor& passed_color) {serverTextColor = passed_color;}
    void setActionTextColor(const QColor& passed_color) {actionTextColor = passed_color;}
    void setBacklogTextColor(const QColor& passed_color) {backlogTextColor = passed_color;}
    void setCommandTextColor(const QColor& passed_color) {commandTextColor = passed_color;}
    void setLinkTextColor(const QColor& passed_color) {linkTextColor = passed_color;}
    void setTimeColor(const QColor& passed_color) { timeColor = passed_color;}
    void setBackgroundColor(const QColor& passed_color) { backgroundColor = passed_color;}

    void slotOk();
    void slotApply();
    void slotCancel();
};

#endif
