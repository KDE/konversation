/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

    SPDX-FileCopyrightText: 2012 Eike Hein <hein@kde.org>
*/

#ifndef TOPICEDIT_H
#define TOPICEDIT_H


#include <KTextEdit>

class Channel;

class QEvent;

class KMessageWidget;


class TopicEdit : public KTextEdit
{
    Q_OBJECT

    public:
        explicit TopicEdit(QWidget* parent = nullptr);
        ~TopicEdit() override;

        Channel* channel() const;
        void setChannel(Channel* channel);

        int maximumLength() const;
        void setMaximumLength(int length);

        QSize minimumSizeHint() const override;

        bool eventFilter(QObject* watched, QEvent* event) override;


    protected:
        void moveEvent(QMoveEvent* event) override;


    private Q_SLOTS:
        void contentsChanged(int position, int charsRemoved, int charsAdded);
        void trimExcessText();
        void moveCursorToEnd();

    private:
        bool colorizeExcessText();
        void resetTextColorization();

        void showWarning();
        void hideWarning();
        void updateWarningGeometry();

    private:
        int m_maximumLength;
        int m_maxCursorPos;

        KMessageWidget* m_warning;
        QWidget* m_warningUndercarriage;

        Channel* m_channel;

        Q_DISABLE_COPY(TopicEdit)
};

#endif
