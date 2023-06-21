/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2009, 2010 Bernd Buschinski <b.buschinski@web.de>
*/

#include "transferlistmodel.h"

#include "konversation_log.h"

#include <KCategorizedSortFilterProxyModel>
#include <KLocalizedString>
#include <KCategoryDrawer>

#include <QApplication>

namespace Konversation
{
    namespace DCC
    {
        QString TransferHeaderData::typeToName(int headertype)
        {
            switch (headertype)
            {
                case FileName:
                    return i18n("File");
                case PartnerNick:
                    return i18n("Partner");
                case OfferDate:
                    return i18n("Started At");
                case Progress:
                    return i18n("Progress");
                case Position:
                    return i18n("Position");
                case CurrentSpeed:
                    return i18n("Speed");
                case SenderAdress:
                    return i18n("Sender Address");
                case Status:
                    return i18n("Status");
                case TimeLeft:
                    return i18n("Remaining");
                case TypeIcon:
                    return i18n("Type");
                default:
                    return QString();
            };
        }

        TransferSizeDelegate::TransferSizeDelegate(KCategoryDrawer* categoryDrawer, QObject* parent)
            : QStyledItemDelegate(parent)
        {
            m_categoryDrawer = categoryDrawer;
        }

        QSize TransferSizeDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
        {
            const int itemType = index.data(TransferListModel::TransferDisplayType).toInt();
            int height;
            QFontMetrics metrics(option.font);

            int width = metrics.horizontalAdvance(index.data(Qt::DisplayRole).toString());

            if (itemType == TransferItemData::SendCategory || itemType == TransferItemData::ReceiveCategory)
            {
                height = m_categoryDrawer->categoryHeight(index, QStyleOption());
            }
            else
            {
                height = metrics.height();
                QVariant pixmapVariant = index.data(Qt::DecorationRole);
                if (!pixmapVariant.isNull())
                {
                    QPixmap tPix = pixmapVariant.value<QPixmap>();
                    height = qMax(height, tPix.height());
                    width = qMax(width, tPix.width());
                }
            }

            return QSize(width, height);
        }


        void TransferSizeDelegate::paint(QPainter *painter,
                                                const QStyleOptionViewItem &option,
                                                const QModelIndex &index) const
        {
            if (index.isValid())
            {
                int type = index.data(TransferListModel::TransferDisplayType).toInt();

                if (type == TransferItemData::SpaceRow)
                    return;

                QStyledItemDelegate::paint(painter, option, index);
            }
        }

        TransferProgressBarDelegate::TransferProgressBarDelegate(QObject *parent)
            : QStyledItemDelegate(parent)
        {
        }

        void TransferProgressBarDelegate::paint(QPainter *painter,
                                                const QStyleOptionViewItem &option,
                                                const QModelIndex &index) const
        {
            if (index.isValid())
            {
                int type = index.data(TransferListModel::TransferDisplayType).toInt();
                if (type == TransferItemData::SendCategory ||
                    type == TransferItemData::ReceiveCategory ||
                    type == TransferItemData::SpaceRow)
                {
                    return;
                }
            }
            QStyle* style = option.widget ? option.widget->style() : QApplication::style();
            style->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, option.widget);

            QStyleOptionProgressBar progressBarOption;
            progressBarOption.rect = option.rect;
            progressBarOption.minimum = 0;
            progressBarOption.maximum = 100;
            progressBarOption.progress = index.data().toInt();
            progressBarOption.text = QString::number(progressBarOption.progress) + QLatin1Char('%');
            progressBarOption.textVisible = true;
            progressBarOption.state = option.state;

            style->drawControl(QStyle::CE_ProgressBar, &progressBarOption, painter);
        }


        TransferListProxyModel::TransferListProxyModel(QObject *parent)
            : QSortFilterProxyModel(parent)
        {
        }

        bool TransferListProxyModel::lessThan(const QModelIndex &left,
                                              const QModelIndex &right) const
        {
            int leftType = left.data(TransferListModel::TransferDisplayType).toInt();
            int rightType = right.data(TransferListModel::TransferDisplayType).toInt();

            if (leftType == rightType)
            {
                const int headerType = left.data(TransferListModel::HeaderType).toInt();
                if (headerType == TransferHeaderData::Position)
                {
                    return (left.data(TransferListModel::TransferProgress).toInt() <
                            right.data(TransferListModel::TransferProgress).toInt());
                }
                else if (headerType == TransferHeaderData::OfferDate)
                {
                    return (left.data(TransferListModel::TransferOfferDate).toDateTime() <
                            right.data(TransferListModel::TransferOfferDate).toDateTime());
                }
                return QSortFilterProxyModel::lessThan(left, right);
            }

            //visible order should always be
            //DCCReceiveCategory > DCCReceiveItem > SpacerRow > DCCSendCategory > DCCSendItem
            if (sortOrder() == Qt::AscendingOrder)
            {
                return !(leftType < rightType);
            }
            return (leftType < rightType);
        }

        TransferListModel::TransferListModel(QObject *parent)
            : QAbstractListModel(parent)
        {
        }

        void TransferListModel::append(const TransferItemData &item)
        {
            beginInsertRows(QModelIndex(), rowCount(), rowCount());
            m_transferList.append(item);
            endInsertRows();

            if (item.transfer)
            {
                connect (item.transfer, &Transfer::statusChanged,
                         this, &TransferListModel::transferStatusChanged);
            }
        }

        Qt::ItemFlags TransferListModel::flags (const QModelIndex &index) const
        {
            if (index.isValid())
            {
                int type = index.data(TransferDisplayType).toInt();
                if (type == TransferItemData::ReceiveCategory ||
                    type == TransferItemData::SendCategory ||
                    type == TransferItemData::SpaceRow)
                {
                    return Qt::NoItemFlags;
                }
            }
            return QAbstractItemModel::flags(index);
        }

        void TransferListModel::transferStatusChanged (Transfer *transfer, int/* oldstatus*/,
                                                       int/* newstatus*/)
        {
            for (int i = 0; i < m_transferList.count(); ++i)
            {
                const TransferItemData &item = m_transferList.at(i);
                if (transfer == item.transfer)
                {
                    Q_EMIT dataChanged(createIndex(i, 0), createIndex(i, columnCount() - 1));
                    return;
                }
            }
        }

        void TransferListModel::appendHeader(TransferHeaderData data)
        {
            data.name = TransferHeaderData::typeToName(data.type);
            m_headerList.append(data);
        }

        QVariant TransferListModel::data (const QModelIndex &index, int role) const
        {
            if(!index.isValid() || index.row() >= m_transferList.count())
            {
                return QVariant();
            }

            if (role == TransferDisplayType)
            {
                return m_transferList[index.row()].displayType;
            }
            else if (role == KCategorizedSortFilterProxyModel::CategoryDisplayRole)
            {
                return displayTypeToString(m_transferList[index.row()].displayType);
            }
            else if (role == HeaderType)
            {
                return headerData(index.column(), Qt::Horizontal, role);
            }

            Transfer *transfer = nullptr;
            if (m_transferList[index.row()].displayType == TransferItemData::ReceiveItem ||
                m_transferList[index.row()].displayType == TransferItemData::SendItem)
            {
                transfer = m_transferList[index.row()].transfer;
            }
            else
            {
                //category item
                return QVariant();
            }

            switch (role)
            {
                case Qt::DisplayRole:
                {
                    int type = columnToHeaderType(index.column());
                    switch (type)
                    {
                        case TransferHeaderData::FileName:
                            return transfer->getFileName();
                        case TransferHeaderData::PartnerNick:
                            return transfer->getPartnerNick();
                        case TransferHeaderData::Progress:
                            return transfer->getProgress();
                        case TransferHeaderData::OfferDate:
                            return transfer->getTimeOffer().toString(QStringLiteral("hh:mm:ss"));
                        case TransferHeaderData::Position:
                            return getPositionPrettyText(transfer->getTransferringPosition(),
                                                         transfer->getFileSize());
                        case TransferHeaderData::CurrentSpeed:
                            return getSpeedPrettyText(transfer->getCurrentSpeed());
                        case TransferHeaderData::SenderAdress:
                            return getSenderAddressPrettyText(transfer);
                        case TransferHeaderData::Status:
                            return getStatusText(transfer->getStatus(), transfer->getType());
                        case TransferHeaderData::TimeLeft:
                            return getTimeLeftPrettyText(transfer->getTimeLeft());

                        default:
                            qCDebug(KONVERSATION_LOG) << "unknown columntype: " << type;
                            Q_FALLTHROUGH();
                        case TransferHeaderData::TypeIcon:
                            return QVariant();
                    };
                    break;
                }

                case TransferType:
                    return transfer->getType();
                case TransferStatus:
                    return transfer->getStatus();
                case TransferPointer:
                    return QVariant::fromValue<QObject*>(transfer);
                case TransferProgress:
                    return transfer->getProgress();
                case TransferOfferDate:
                    return transfer->getTimeOffer();
                case Qt::DecorationRole:
                {
                    int type = columnToHeaderType(index.column());
                    switch (type)
                    {
                        case TransferHeaderData::Status:
                        {
                            return getStatusIcon(transfer->getStatus());
                        }
                        case TransferHeaderData::TypeIcon:
                        {
                            return QIcon::fromTheme((transfer->getType() == Transfer::Send) ? QStringLiteral("arrow-up") : QStringLiteral("arrow-down"));
                        }
                        default:
                            return QVariant();
                    }
                    break;
                }
                case Qt::ToolTipRole:
                {
                    int type = columnToHeaderType(index.column());
                    QString tooltip;
                    switch (type)
                    {
                        case TransferHeaderData::FileName:
                            tooltip = transfer->getFileName();
                            break;
                        case TransferHeaderData::PartnerNick:
                            tooltip = transfer->getPartnerNick();
                            break;
                        case TransferHeaderData::Progress:
                            tooltip = QString::number(transfer->getProgress()) + QLatin1Char('%');
                            break;
                        case TransferHeaderData::OfferDate:
                            tooltip = transfer->getTimeOffer().toString(QStringLiteral("hh:mm:ss"));
                            break;
                        case TransferHeaderData::Position:
                            tooltip = getPositionPrettyText(transfer->getTransferringPosition(),
                                                            transfer->getFileSize());
                            break;
                        case TransferHeaderData::CurrentSpeed:
                            tooltip = getSpeedPrettyText(transfer->getCurrentSpeed());
                            break;
                        case TransferHeaderData::SenderAdress:
                            tooltip = getSenderAddressPrettyText(transfer);
                            break;
                        case TransferHeaderData::Status:
                            tooltip = getStatusDescription(transfer->getStatus(), transfer->getType(), transfer->getStatusDetail());
                            break;
                        case TransferHeaderData::TimeLeft:
                            tooltip = getTimeLeftPrettyText(transfer->getTimeLeft());
                            break;
                        case TransferHeaderData::TypeIcon:
                            tooltip = displayTypeToString(m_transferList[index.row()].displayType);
                            break;
                        default:
                            qCDebug(KONVERSATION_LOG) << "unknown columntype: " << type;
                            break;
                    };

                    if (tooltip.isEmpty())
                    {
                        return QVariant();
                    }
                    else
                    {
                        return QString(QLatin1String("<qt>") + tooltip + QLatin1String("</qt>"));
                    }
                }
                default:
                    return QVariant();
            }
        }

        QIcon TransferListModel::getStatusIcon(Transfer::Status status) const
        {
            QString icon;
            switch (status)
            {
                case Transfer::Queued:
                    icon = QStringLiteral("media-playback-stop");
                    break;
                case Transfer::Preparing:
                case Transfer::WaitingRemote:
                case Transfer::Connecting:
                    icon = QStringLiteral("network-disconnect");
                    break;
                case Transfer::Transferring:
                    icon = QStringLiteral("media-playback-start");
                    break;
                case Transfer::Done:
                    icon = QStringLiteral("dialog-ok");
                    break;
                case Transfer::Aborted:
                case Transfer::Failed:
                    icon = QStringLiteral("process-stop");
                    break;
                default:
                break;
            }
            return QIcon::fromTheme(icon);
        }

        QString TransferListModel::getSpeedPrettyText (transferspeed_t speed)
        {
            if (speed == Transfer::Calculating || speed == Transfer::InfiniteValue)
            {
                return QStringLiteral("?");
            }
            else if (speed == Transfer::NotInTransfer)
            {
                return QString();
            }
            else
            {
                return i18n("%1/sec", KIO::convertSize((KIO::fileoffset_t)speed));
            }
        }

        QString TransferListModel::getPositionPrettyText(KIO::fileoffset_t position,
                                                         KIO::filesize_t filesize) const
        {
            return KIO::convertSize(position) + QLatin1String(" / ") + KIO::convertSize(filesize);
        }

        QString TransferListModel::getSenderAddressPrettyText(Transfer *transfer) const
        {
            if (transfer->getType() == Transfer::Send)
            {
                return QStringLiteral("%1:%2").arg(transfer->getOwnIp()).arg(transfer->getOwnPort());
            }
            else
            {
                return QStringLiteral("%1:%2").arg(transfer->getPartnerIp()).arg(transfer->getPartnerPort());
            }
        }

        QString TransferListModel::getTimeLeftPrettyText(int timeleft)
        {
            switch (timeleft)
            {
                case Transfer::InfiniteValue:
                    return QStringLiteral("?");
                default:
                    return secToHMS(timeleft);
                case Transfer::NotInTransfer:
                    return QString();
            }
        }

        QString TransferListModel::secToHMS(long sec)
        {
            int remSec = sec;
            int remHour = remSec / 3600;
            remSec -= remHour * 3600;
            int remMin = remSec / 60;
            remSec -= remMin * 60;

            // remHour can be more than 25, so we can't use QTime here.
            return QStringLiteral("%1:%2:%3")
                .arg(QString::number(remHour).rightJustified(2, QLatin1Char('0'), false),
                     QString::number(remMin).rightJustified(2, QLatin1Char('0')),
                     QString::number(remSec).rightJustified(2, QLatin1Char('0')));
        }

        QString TransferListModel::getStatusText(Transfer::Status status, Transfer::Type type)
        {
            switch (status)
            {
                case Transfer::Queued:
                    return i18n("Queued");
                case Transfer::Preparing:
                    return i18n("Preparing");
                case Transfer::WaitingRemote:
                    return i18nc("Transfer is waiting for the partner to accept or reject it", "Pending");
                case Transfer::Connecting:
                    return i18nc("Transfer is connecting to the partner", "Connecting");
                case Transfer::Transferring:
                    switch (type)
                    {
                        case Transfer::Receive:
                            return i18n("Receiving");
                        case Transfer::Send:
                            return i18n("Sending");
                        default:
                            qCDebug(KONVERSATION_LOG) << "unknown type: " << type;
                            return QString();
                    }
                case Transfer::Done:
                    return i18nc("Transfer has completed successfully", "Done");
                case Transfer::Failed:
                    return i18nc("Transfer failed", "Failed");
                case Transfer::Aborted:
                    return i18n("Aborted");
                default:
                    qCDebug(KONVERSATION_LOG) << "unknown status: " << status;
                    return QString();
            }
        }

        QString TransferListModel::getStatusDescription(Transfer::Status status, Transfer::Type type, const QString& errorMessage) const
        {
            switch (status)
            {
                case Transfer::Queued:
                    return i18n("Queued - Transfer is waiting for you to accept or reject it");
                case Transfer::Preparing:
                    switch (type)
                    {
                        case Transfer::Receive:
                            return i18n("Preparing - Transfer is checking for resumable files");
                        case Transfer::Send:
                            return i18n("Preparing - Transfer is acquiring the data to send");
                        default:
                            qCDebug(KONVERSATION_LOG) << "unknown type: " << type;
                            return QString();
                    }
                case Transfer::WaitingRemote:
                    return i18n("Pending - Transfer is waiting for the partner to accept or reject it");
                case Transfer::Connecting:
                    return i18n("Connecting - Transfer is connecting to the partner");
                case Transfer::Transferring:
                    switch (type)
                    {
                        case Transfer::Receive:
                            return i18n("Receiving - Transfer is receiving data from the partner");
                        case Transfer::Send:
                            return i18n("Sending - Transfer is sending data to partner");
                        default:
                            qCDebug(KONVERSATION_LOG) << "unknown type: " << type;
                            return QString();
                    }
                case Transfer::Done:
                    return i18n("Done - Transfer has completed successfully");
                case Transfer::Failed:
                    if (errorMessage.isEmpty())
                    {
                        return i18n("Failed - Transfer failed with 'unknown reason'");
                    }
                    else
                    {
                        return i18n("Failed - Transfer failed with reason '%1'", errorMessage);
                    }
                case Transfer::Aborted:
                    return i18n("Aborted - Transfer was aborted by the User");
                default:
                    qCDebug(KONVERSATION_LOG) << "unknown status: " << status;
                    return QString();
            }
        }

        int TransferListModel::columnToHeaderType (int column) const
        {
            return m_headerList.at(column).type;
        }

        int TransferListModel::columnCount (const QModelIndex &/*parent*/) const
        {
            return m_headerList.count();
        }

        QVariant TransferListModel::headerData (int section, Qt::Orientation orientation,
                                                int role) const
        {
            if(orientation == Qt::Vertical || section >= columnCount() || section < 0)
            {
                return QVariant();
            }

            switch (role)
            {
                case Qt::DisplayRole:
                    return m_headerList.at(section).name;
                case HeaderType:
                    return m_headerList.at(section).type;
                default:
                    return QVariant();
            }
        }

        int TransferListModel::rowCount (const QModelIndex &/*parent*/) const
        {
            return m_transferList.count();
        }

        bool TransferListModel::removeRow (int row, const QModelIndex &parent)
        {
            return removeRows(row, 1, parent);
        }

        bool TransferListModel::removeRows (int row, int count, const QModelIndex &parent)
        {
            if (count < 1 || row < 0 || (row + count) > rowCount(parent))
            {
                return false;
            }

            beginRemoveRows(parent, row, row + count - 1);

            bool blockSignal = signalsBlocked();
            blockSignals(true);

            for (int i = row + count - 1; i >= row; --i)
            {
                TransferItemData item = m_transferList.takeAt(i);
                //Category items don't have a transfer
                if (item.transfer)
                {
                    item.transfer->removedFromView();
                }
            }

            blockSignals(blockSignal);
            endRemoveRows();

            Q_EMIT rowsPermanentlyRemoved (row, row + count - 1);

            return true;
        }

        int TransferListModel::itemCount (TransferItemData::ItemDisplayType displaytype) const
        {
            int count = 0;
            for (const TransferItemData &item : m_transferList) {
                if (item.displayType == displaytype)
                {
                    ++count;
                }
            }
            return count;
        }

        QString TransferListModel::displayTypeToString (int type) const
        {
            if (type == TransferItemData::ReceiveCategory)
            {
                return i18n("Incoming Transfers");
            }
            else
            {
                return i18n("Outgoing Transfers");
            }
        }

    }
}

#include "moc_transferlistmodel.cpp"
