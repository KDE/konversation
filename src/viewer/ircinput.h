/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  The line input widget with chat enhanced functions
  begin:     Tue Mar 5 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef IRCINPUT_H
#define IRCINPUT_H

#include <KTextEdit>

class KCompletionBox;

class IRCInput : public KTextEdit
{
    Q_OBJECT

    public:
        explicit IRCInput(QWidget* parent);
        ~IRCInput();

        void setCompletionMode(char mode);
        char getCompletionMode();
        void setOldCursorPosition(int pos);
        int getOldCursorPosition();
        QString lastCompletion() const { return m_lastCompletion; }
        void doInlineAutoreplace();

        QSize sizeHint() const Q_DECL_OVERRIDE;
        QSize minimumSizeHint() const Q_DECL_OVERRIDE;

        bool event(QEvent* e) Q_DECL_OVERRIDE;

        void createHighlighter() Q_DECL_OVERRIDE;

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

    protected Q_SLOTS:
        void getHistory(bool up);
        void insertCompletion(const QString& nick);
        void disableSpellChecking();
        void setSpellChecking(bool set);

        void maybeResize();

    protected:
        bool eventFilter(QObject *object,QEvent *event) Q_DECL_OVERRIDE;
        void addHistory(const QString& text);
        bool checkPaste(QString& text);

        void insertFromMimeData(const QMimeData *source) Q_DECL_OVERRIDE;
        void keyPressEvent(QKeyEvent* e) Q_DECL_OVERRIDE;
        void wheelEvent(QWheelEvent* e) Q_DECL_OVERRIDE;
        void showEvent(QShowEvent* e) Q_DECL_OVERRIDE;
        void hideEvent(QHideEvent* e) Q_DECL_OVERRIDE;
        void resizeEvent(QResizeEvent* e) Q_DECL_OVERRIDE;

        QStringList historyList;
        int lineNum;
        int oldPos;
        char completionMode;
        KCompletionBox* completionBox;
        QString m_lastCompletion;
        bool m_multiRow;
        int m_qtBoxPadding; //see comment in constructor

        QTimer* m_disableSpellCheckTimer;

};
#endif
