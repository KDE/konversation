/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
*/

#ifndef IRCINPUT_H
#define IRCINPUT_H

#include <KTextEdit>

class KCompletionBox;

/**
 * The line input widget with chat enhanced functions
 */
class IRCInput : public KTextEdit
{
    Q_OBJECT

    public:
        explicit IRCInput(QWidget* parent);
        ~IRCInput() override;

        void setCompletionMode(char mode);
        char getCompletionMode() const;
        void setOldCursorPosition(int pos);
        int getOldCursorPosition() const;
        QString lastCompletion() const { return m_lastCompletion; }
        void doInlineAutoreplace();

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

        bool event(QEvent* e) override;

        void createHighlighter() override;

    Q_SIGNALS:
        void nickCompletion();
        void endCompletion();                     // tell channel that completion phase is over
        void textPasted(const QString& text);
        void submit();
        void envelopeCommand();

    public Q_SLOTS:
        void paste(bool useSelection);
        void showCompletionList(const QStringList& nicks);
        void setText(const QString& text, bool preserveContents = false);
        void setLastCompletion(const QString& completion);
        virtual void setOverwriteMode(bool) { }
        virtual void updateAppearance();

    protected:
        bool eventFilter(QObject *object,QEvent *event) override;

        void insertFromMimeData(const QMimeData *source) override;
        void keyPressEvent(QKeyEvent* e) override;
        void wheelEvent(QWheelEvent* e) override;
        void showEvent(QShowEvent* e) override;
        void hideEvent(QHideEvent* e) override;
        void resizeEvent(QResizeEvent* e) override;

    private:
        void addHistory(const QString& text);
        bool checkPaste(QString& text);

    private Q_SLOTS:
        void getHistory(bool up);
        void insertCompletion(const QString& nick);
        void disableSpellChecking();
        void setSpellChecking(bool set);

        void maybeResize();

    private:
        QStringList historyList;
        int lineNum;
        int oldPos;
        char completionMode;
        KCompletionBox* completionBox;
        QString m_lastCompletion;
        bool m_multiRow;
        int m_qtBoxPadding; //see comment in constructor

        QTimer* m_disableSpellCheckTimer;

        Q_DISABLE_COPY(IRCInput)
};

#endif
