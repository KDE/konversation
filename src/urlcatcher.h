/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  shows all URLs found by the client
  begin:     Die Mai 27 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef URLCATCHER_H
#define URLCATCHER_H

#include "chatwindow.h"


class QTreeWidget;
class QTreeWidgetItem;
class QPushButton;
class KTreeWidgetSearchLineWidget;

class UrlCatcher : public ChatWindow
{
    Q_OBJECT

        public:
        explicit UrlCatcher(QWidget* parent);
        ~UrlCatcher();

        virtual bool canBeFrontView()   { return true; }

        signals:
        void deleteUrl(const QString& who,const QString& url);
        void clearUrlList();

    public slots:
        void addUrl(const QString& who,const QString& url);

    protected slots:
        void urlSelected();
        void openUrl(QTreeWidgetItem* item);

        void openUrlClicked();
        void copyUrlClicked();
        void deleteUrlClicked();
        void saveListClicked();
        void clearListClicked();

    protected:
        QTreeWidget* urlListView;
        KTreeWidgetSearchLineWidget* searchWidget;

        /** Called from ChatWindow adjustFocus */
        virtual void childAdjustFocus();
        QPushButton* openUrlButton;
        QPushButton* copyUrlButton;
        QPushButton* deleteUrlButton;
        QPushButton* saveListButton;
        QPushButton* clearListButton;
};
#endif
