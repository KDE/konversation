/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2004 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2004 Shintaro Matsuoka <shin@shoegazed.org>
*/

#ifndef DCCRESUMEDIALOG_H
#define DCCRESUMEDIALOG_H

#include <kdialog.h>


class KUrlRequester;
class DccTransferRecv;

class DccResumeDialog : public KDialog
{
    Q_OBJECT

        public:
        enum ReceiveAction
        {
            RA_Rename    = 0x01,
            RA_Overwrite = 0x02,
            RA_Resume    = 0x04,
            RA_Cancel    = 0x08
        };

        virtual ~DccResumeDialog();

        static ReceiveAction ask(DccTransferRecv* item, const QString& message, int enabledActions, ReceiveAction defaultAction);

    protected slots:
        void slotButtonClicked(int button);
        void suggestNewName();
        void setDefaultName();
        void updateDialogButtons();

    protected:
        DccResumeDialog(DccTransferRecv* item, const QString& caption, const QString& message, int enabledActions, QFlags<KDialog::ButtonCode> enabledButtonCodes, KDialog::ButtonCode defaultButtonCode);

        // UI
        KUrlRequester* m_urlreqFileURL;

        // data
        DccTransferRecv* m_item;
        int m_enabledActions;
        ReceiveAction m_selectedAction;

};
#endif
