/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  editserverdialog.h  -  description
  begin:     Tue Feb 12 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef EDITSERVERDIALOG_H
#define EDITSERVERDIALOG_H

#include <kdialogbase.h>

/*
  @author Dario Abatianni
*/

class KLineEdit;
class KComboBox;

class EditServerDialog : public KDialogBase
{
  Q_OBJECT

  public:
    EditServerDialog(QWidget* parent=0,QString group=QString::null,
                                       QString name=QString::null,
                                       QString port="6667",
                                       QString serverKey=QString::null,
                                       QString channel=QString::null,
                                       QString channelKey=QString::null,
                                       QString currentIdentity=QString::null,
                                       QString connectCommands=QString::null);
    ~EditServerDialog();

  signals:
    void serverChanged(const QString& group,
                       const QString& serverName,
                       const QString& port,
                       const QString& serverKey,
                       const QString& channelName,
                       const QString& channelKey,
                       const QString& identity,
                       const QString& connectCommands);
  protected slots:
    void slotOk();

  protected:
    KLineEdit* groupNameInput;
    KLineEdit* serverNameInput;
    KLineEdit* serverPortInput;
    KLineEdit* serverKeyInput;
    KLineEdit* channelNameInput;
    KLineEdit* connectCommandsInput;
    KLineEdit* channelKeyInput;
    KComboBox* identityCombo;
};

#endif
