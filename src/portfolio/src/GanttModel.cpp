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

#include "GanttModel.h"
#include <MainDocument.h>

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

QVariant GanttModel::data(const QModelIndex &idx, int role) const
{
    int rl = role;
    switch (idx.column()) {
        case 1: { // Type
            switch (rl) {
                case Qt::DisplayRole:
                    return KGantt::TypeTask;
                default:
                    return QVariant();
            }
            break;
        }
        case 2: { // Start
            switch (rl) {
                case Qt::EditRole: {
                    KoDocument *doc = ProjectsFilterModel::data(idx, DOCUMENT_ROLE).value<KoDocument*>();
                    if (!doc) {
                        qInfo()<<Q_FUNC_INFO<<idx<<"No document"<<portfolio()->documents();
                        return QVariant();
                    }
                    KPlato::ScheduleManager *sm = portfolio()->scheduleManager(doc);
                    if (!sm) {
                        return QVariant();
                    }
                    QDateTime start = ProjectsFilterModel::data(idx, rl).toDateTime();
                    if (sm->recalculate()) {
                        if (start.isValid() && start < sm->recalculateFrom()) {
                            start = projectRestartTime(doc->project(), sm);
                        }
                    }
                    qInfo()<<Q_FUNC_INFO<<sm->recalculateFrom()<<start;
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
    QVariant v = ProjectsFilterModel::data(idx, rl);
    return v;
}

QDateTime GanttModel::projectRestartTime(const KPlato::Project *project, KPlato::ScheduleManager *sm) const
{
    Q_ASSERT(project);
    QDateTime restart;
    auto id = sm->scheduleId();
    if (id == NOTSCHEDULED) {
        return restart;
    }
    const auto tasks = project->allTasks();
    for (const KPlato::Task *t : tasks) {
        if (t->type() != KPlato::Node::Type_Task) {
            continue;
        }
        KPlato::Schedule *s = t->schedule(id);
        if (!s) {
            continue;
        }
        const auto apps = s->appointments();
        for (const auto *a : apps) {
            const auto intervals = a->intervals(sm->recalculateFrom(), project->endTime(id)).map();
            if (intervals.isEmpty()) {
                continue;
            }
            auto time = intervals.values(intervals.keys().constFirst()).constLast().startTime();
            qInfo()<<Q_FUNC_INFO<<time<<intervals.values(intervals.keys().constFirst());
            if (!restart.isValid() || restart < time) {
                restart = time;
            }
        }
    }
    return restart;
}
