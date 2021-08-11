/* This file is part of the KDE project
* SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
*
* SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PLANPORTFOLIO_SCHEDULINGLOGMODEL_H
#define PLANPORTFOLIO_SCHEDULINGLOGMODEL_H

#include <kptnodeitemmodel.h>

#include <kptschedule.h>

#include <QStandardItemModel>
#include <QSortFilterProxyModel>

class SchedulingLogFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit SchedulingLogFilterModel(QObject *parent = nullptr);

    void setSeveritiesDenied(QList<int> values);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
    bool filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const override;

private:
    QList<int> m_severities;
};

class SchedulingLogModel : public QStandardItemModel
{
    Q_OBJECT
public:
    enum DataRoles { SeverityRole = Qt::UserRole + 1, IdentityRole };
    enum ColumnNumbers { NameCol = 0, PhaseCol, SeverityCol, MessageCol };

    explicit SchedulingLogModel(QObject *parent =  nullptr);

    void setLog(const QVector<KPlato::Schedule::Log> &log);

    void addLogEntry(const KPlato::Schedule::Log &log);

};

#endif
