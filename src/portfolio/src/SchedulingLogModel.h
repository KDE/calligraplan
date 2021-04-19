/* This file is part of the KDE project
* Copyright (C) 2021 Dag Andersen <dag.andersen@kdemail.net>
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Library General Public
* License as published by the Free Software Foundation; either
* version 2 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Library General Public License for more details.
*
* You should have received a copy of the GNU Library General Public License
* along with this library; see the file COPYING.LIB.  If not, write to
* the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
* Boston, MA 02110-1301, USA.
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
