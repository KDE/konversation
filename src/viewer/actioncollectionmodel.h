
/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2018 Eike Hein <hein@kde.org>
*/

#ifndef ACTIONCOLLECTIONMODEL_H
#define ACTIONCOLLECTIONMODEL_H

#include <QAbstractListModel>

#include <KActionCollection>

class ActionCollectionModel : public QAbstractListModel
{
    Q_OBJECT

    public:
        enum AdditionalRoles {
            KeySequence = Qt::UserRole + 1,
        };
        Q_ENUM(AdditionalRoles)

        explicit ActionCollectionModel(QObject *parent = nullptr);
        ~ActionCollectionModel() override;

        QHash<int, QByteArray> roleNames() const override;

        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        int rowCount(const QModelIndex &parent = QModelIndex()) const override;

        KActionCollection *actionCollection() const;
        void setActionCollection(KActionCollection *collection);

        Q_INVOKABLE void trigger(int row);

    private Q_SLOTS:
        void reset();

    private:
        KActionCollection *m_actionCollection;
};

#endif
