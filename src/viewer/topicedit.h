/*
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License or (at your option) version 3 or any later version
  accepted by the membership of KDE e.V. (or its successor appro-
  ved by the membership of KDE e.V.), which shall act as a proxy
  defined in Section 14 of version 3 of the license.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see http://www.gnu.org/licenses/.
*/

/*
  Copyright (C) 2012 Eike Hein <hein@kde.org>
*/

#ifndef TOPICEDIT_H
#define TOPICEDIT_H

#include <kdeversion.h>
#include <KTextEdit>


#if KDE_IS_VERSION(4, 7, 0)
class KMessageWidget;
#endif

class QEvent;


class TopicEdit : public KTextEdit
{
    Q_OBJECT

    public:
        explicit TopicEdit(QWidget* parent = 0);
        ~TopicEdit();

        int maximumLength() const;
        void setMaximumLength(int length);

#if KDE_IS_VERSION(4, 7, 0)
        QSize minimumSizeHint() const;

        bool eventFilter(QObject* watched, QEvent* event);


    protected:
        void moveEvent(QMoveEvent* event);
#endif


    private slots:
        void contentsChanged(int position, int charsRemoved, int charsAdded);
        void trimExcessText();
        void moveCursorToEnd();

    private:
        void colorizeExcessText();
        void resetTextColorization();

#if KDE_IS_VERSION(4, 7, 0)
        void showWarning();
        void hideWarning();
        void updateWarningGeometry();
#endif

        int m_maximumLength;
        bool m_lastEditPastMaximumLength;


#if KDE_IS_VERSION(4, 7, 0)
        KMessageWidget* m_warning;
        QWidget* m_warningUndercarriage;
#endif
};

#endif
