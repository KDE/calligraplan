/* This file is part of the KDE project
* SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
*
* SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "GanttModel.h"
#include <MainDocument.h>
#include <PlanGroupDebug.h>

#include <kptproject.h>
#include <kpttask.h>
#include <kptschedule.h>
#include <kptappointment.h>

#include <KGanttGlobal>

GanttModel::GanttModel(QObject *parent)
    : ProjectsFilterModel(parent)
{
    QList<int> columns = QList<int>()
    << KPlato::NodeModel::NodeName
    << KPlato::NodeModel::NodeType
    << KPlato::NodeModel::NodeStartTime
    << KPlato::NodeModel::NodeEndTime;
    setAcceptedColumns(columns);
}

GanttModel::~GanttModel()
{
}

KPlato::ScheduleManager *GanttModel::scheduleManager(const QModelIndex &idx) const
{
    KoDocument *doc = ProjectsFilterModel::data(idx, DOCUMENT_ROLE).value<KoDocument*>();
    if (!doc) {
        debugPortfolio<<idx<<"No document"<<portfolio()->documents();
        return nullptr;
    }
    return portfolio()->scheduleManager(doc);
}

QVariant GanttModel::data(const QModelIndex &idx, int role) const
{
    switch (idx.column()) {
        case 1: { // Type
            switch (role) {
                case Qt::DisplayRole: {
                    auto sm = scheduleManager(idx);
                    if (!sm || !sm->isScheduled()) {
                        return QVariant();
                    }
                    return KGantt::TypeTask;
                }
                default:
                    return QVariant();
            }
            break;
        }
        case 2: { // Start
            switch (role) {
                case Qt::DisplayRole:
                case Qt::EditRole: {
                    auto sm = scheduleManager(idx);
                    if (!sm || !sm->isScheduled()) {
                        return QVariant();
                    }
                    QDateTime start = ProjectsFilterModel::data(idx, Qt::EditRole).toDateTime();
                    if (sm->recalculate()) {
                        if (start.isValid() && start < sm->recalculateFrom()) {
                            start = projectRestartTime(sm);
                        }
                    }
                    return start;
                }
                default:
                    break;
            }
            break;
        }
        default: {
            break;
        }
    }
    QVariant v = ProjectsFilterModel::data(idx, role);
    return v;
}

QDateTime GanttModel::projectRestartTime(KPlato::ScheduleManager *sm) const
{
    QDateTime restart;
    auto id = sm->scheduleId();
    if (id == NOTSCHEDULED) {
        return restart;
    }
    // Get the projects actual re-start time.
    // A re-calculated project may re-start earlier than re-calculation time
    // because started tasks will be scheduled from their actual start time.
    // Finished tasks are disregarded.
    const auto tasks = sm->project().allTasks();
    for (const KPlato::Task *t : tasks) {
        if (t->type() != KPlato::Node::Type_Task) {
            continue;
        }
        if (t->completion().isFinished()) {
            continue;
        }
        KPlato::Schedule *s = t->schedule(id);
        if (!s) {
            continue;
        }
        const auto dt = t->startTime(id);
        if (!restart.isValid() || (dt.isValid() &&  dt < restart)) {
            restart = dt;
        }
    }
    return restart;
}
