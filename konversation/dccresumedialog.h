//
// C++ Interface: dccresumedialog
//
// Description: 
//
//
// Author: Dario Abatianni <eisfuchs@tigress.com>, (C) 2004
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef DCCRESUMEDIALOG_H
#define DCCRESUMEDIALOG_H

#include <kdialogbase.h>
#include <kurl.h>

/*
  @author Dario Abatianni
*/

class DccResumeDialog : public KDialogBase
{
  Q_OBJECT

  public:
    DccResumeDialog(QWidget *parent,QString filename,const KURL& baseURL);
    ~DccResumeDialog();

    static int ask(QWidget* parent,QString& filename,const KURL& url);

  protected slots:
    void suggestNewName();
    void filenameChanged(const QString& newName);
    void renameClicked();
    void overwriteClicked();

  protected:
    static QString newFilename;
    KURL baseURL;
    KLineEdit* nameInput;
};

#endif
