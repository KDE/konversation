//
// C++ Interface: dccresumedialog
//
// Description: 
//
//
// Authors: Dario Abatianni <eisfuchs@tigress.com>, (C) 2004
//          Shintaro Matsuoka <shin@shoegazed.org>, (C) 2004
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef DCCRESUMEDIALOG_H
#define DCCRESUMEDIALOG_H

#include <kdialogbase.h>

/*
  @author Dario Abatianni
*/

class KURLRequester;
class DccTransferRecv;

class DccResumeDialog : public KDialogBase
{
  Q_OBJECT

  public:
    enum ReceiveAction
    {
      Overwrite,
      Rename,
      Resume,
      Cancel
    };
  
    ~DccResumeDialog();

    static ReceiveAction ask(DccTransferRecv* parentItem);

  protected slots:
    void slotOkClicked();
    void slotUser1Clicked();
    void suggestNewName();
    void setDefaultName();
    void updateDialogButtons();
        
  protected:
    DccResumeDialog(DccTransferRecv* item);
    
    // UI
    KURLRequester* urlreqFilePath;
    
    // data
    DccTransferRecv* item;
    ReceiveAction action;
  
};

#endif
