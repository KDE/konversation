// dccdetaildialog.h
// Copyright (C) 2004 Shintaro Matsuoka <shin@shoegazed.org>
// See COPYING file for licensing information

#ifndef DCCDETAILDIALOG_H
#define DCCDETAILDIALOG_H

#include <kdialog.h>

class KLineEdit;
class KProgress;
class KPushButton;
class KURLRequester;
class DccTransfer;

class DccDetailDialog : public KDialog
{
  Q_OBJECT
  
  public:
    DccDetailDialog( DccTransfer* item );
    virtual ~DccDetailDialog();
    
    void updateView();
  
  protected slots:
    void slotLocalPathChanged( const QString& newFilePath );
    void slotOpenFile();
    void slotAccept();
    void slotAbort();
    void slotClose();
    
  protected:
    DccTransfer* m_item;
    
    // UI
    KURLRequester* m_localPath;
    KPushButton* m_localPathOpen;
    KLineEdit* m_partner;
    KLineEdit* m_self;
    KLineEdit* m_status;
    KProgress* m_progress;
    KLineEdit* m_position;
    
    KPushButton* m_buttonAccept;
    KPushButton* m_buttonAbort;
};

#endif // DCCDETAILDIALOG_H
