/* This file is part of the KDE project
* SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
*
* SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <SchedulingLogModel.h>

#include <kptnode.h>
#include <Resource.h>

#include <KLocalizedString>


SchedulingLogFilterModel::SchedulingLogFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    
}

void SchedulingLogFilterModel::setSeveritiesDenied(QList<int> values)
{
    beginResetModel();
    m_severities = values;
    endResetModel();
}

bool SchedulingLogFilterModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if (source_parent.isValid()) {
        return false;
    }
    const auto idx = sourceModel()->index(source_row, SchedulingLogModel::SeverityCol);
    return !m_severities.contains(idx.data(SchedulingLogModel::SeverityRole).toInt());
}

bool SchedulingLogFilterModel::filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const
{
    Q_UNUSED(source_parent);
    return source_column != SchedulingLogModel::PhaseCol;
}

SchedulingLogModel::SchedulingLogModel(QObject *parent)
    : QStandardItemModel(parent)
{
    
}

void SchedulingLogModel::setLog(const QVector<KPlato::Schedule::Log> &log)
{
    // Note: Match enum ColumnNumbers
    static QStringList headers = {
        xi18nc("@title", "Name"),
        xi18nc("@title", "Phase"),
        xi18nc("@title", "Severity"),
        xi18nc("@title", "Message")
    };
    if (columnCount() == 0) {
        setHorizontalHeaderLabels(headers);
    }
    if (rowCount() > 0) {
        removeRows(0, rowCount());
    }
    for (const KPlato::Schedule::Log &l : log) {
        addLogEntry(l);
    }

}

void SchedulingLogModel::addLogEntry(const KPlato::Schedule::Log &log)
{
    QList<QStandardItem*> lst;
    if (log.resource) {
        lst.append(new QStandardItem(log.resource->name()));
    } else if (log.node) {
        lst.append(new QStandardItem(log.node->name()));
    } else {
        lst.append( new QStandardItem(QLatin1String("")));
    }
//     lst.append(new QStandardItem(m_schedule->logPhase(log.phase)));
    lst.append(new QStandardItem(QStringLiteral("??")));
    QStandardItem *item = new QStandardItem(KPlato::MainSchedule::logSeverity(log.severity));
    item->setData(log.severity, SeverityRole);
    item->setEditable(false);
    lst.append(item);
    lst.append(new QStandardItem(log.message));
    for (QStandardItem *itm : std::as_const(lst)) {
            if (log.resource) {
                itm->setData(log.resource->id(), IdentityRole);
            } else if (log.node) {
                itm->setData(log.node->id(), IdentityRole);
            }
            switch (log.severity) {
            case KPlato::Schedule::Log::Type_Debug:
                itm->setData(QColor(Qt::darkYellow), Qt::ForegroundRole);
                break;
            case KPlato::Schedule::Log::Type_Info:
                break;
            case KPlato::Schedule::Log::Type_Warning:
                itm->setData(QColor(Qt::blue), Qt::ForegroundRole);
                break;
            case KPlato::Schedule::Log::Type_Error:
                itm->setData(QColor(Qt::red), Qt::ForegroundRole);
                break;
            default:
                break;
        }
    }
    appendRow(lst);
}
