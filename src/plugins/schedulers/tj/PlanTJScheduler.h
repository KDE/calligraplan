/* This file is part of the KDE project
 * Copyright (C) 2009 Dag Andersen <dag.andersen@kdemail.net>
 * Copyright (C) 2011 Dag Andersen <dag.andersen@kdemail.net>
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

#ifndef PLANTJSCHEDULER_H
#define PLANTJSCHEDULER_H

#include "plantj_export.h"

#include "kptschedulerplugin.h"

#include "kptdatetime.h"

#include <QThread>
#include <QObject>
#include <QMap>
#include <QList>

class QDateTime;

namespace TJ
{
    class Project;
    class Task;
    class Resource;
    class Interval;
    class CoreAttributes;

}

namespace KPlato
{
    class Project;
    class ScheduleManager;
    class Schedule;
    class MainSchedule;
    class Resource;
    class Task;
    class Node;
    class Relation;
}
using namespace KPlato;

class PlanTJScheduler : public KPlato::SchedulerThread
{
    Q_OBJECT

private:

public:
    PlanTJScheduler(Project *project, ScheduleManager *sm, ulong granularity, QObject *parent = nullptr);
    explicit PlanTJScheduler(QObject *parent = nullptr);
    ~PlanTJScheduler() override;

    bool check();
    bool solve();
    int result;

    /// Fill project data into TJ structure
    bool kplatoToTJ();
    /// Fetch project data from TJ structure
    bool kplatoFromTJ();

    void schedule(SchedulingContext &context) override;

Q_SIGNALS:
    void sigCalculationStarted(KPlato::Project*, KPlato::ScheduleManager*);
    void sigCalculationFinished(KPlato::Project*, KPlato::ScheduleManager*);
    const char* taskname();

public Q_SLOTS:
    void slotMessage(int type, const QString &msg, TJ::CoreAttributes *object);

protected:
    void run() override;

    void adjustSummaryTasks(const QList<Node*> &nodes);

    TJ::Resource *addResource(KPlato::Resource *resource);
    void addTasks();
    void addWorkingTime(const KPlato::Task *task, TJ::Task *job);
    TJ::Task *addTask(const KPlato::Node *node , TJ::Task *parent = nullptr);
    void addDependencies();
    void addPrecedes(const Relation *rel);
    void addDepends(const Relation *rel);
    void addDependencies(Node *task);
    void setConstraints();
    void setConstraint(TJ::Task *job, KPlato::Node *task);
    TJ::Task *addStartNotEarlier(Node *task);
    TJ::Task *addFinishNotLater(Node *task);
    void addRequests();
    void addRequest(TJ::Task *job, Node *task);
    void addStartEndJob();
    bool taskFromTJ(TJ::Task *job, Node *task);
    bool taskFromTJ(Project *project, TJ::Task *job, Node *task);
    void calcPertValues(Node *task);
    Duration calcPositiveFloat(Node *task);
    Resource *resource(Project *project, TJ::Resource *tjResource);

    void populateProjects(KPlato::SchedulingContext &context);

    static bool exists(QList<CalendarDay*> &lst, CalendarDay *day);
    static int toTJDayOfWeek(int day);
    static DateTime fromTime_t(time_t, const QTimeZone &tz);
    static time_t toTJTime_t(const QDateTime &dt, ulong granularity);
    AppointmentInterval fromTJInterval(const TJ::Interval &tji, const QTimeZone &tz);
    static TJ::Interval toTJInterval(const QDateTime &start, const QDateTime &end, ulong tjGranularity);
    static TJ::Interval toTJInterval(const QTime &start, const QTime &end, ulong tjGranularity);
    
private:
    ulong tjGranularity() const;
    void insertProject(const KPlato::Project *project, int priority, KPlato::SchedulingContext &context);
    void insertBookings(KPlato::SchedulingContext &context);
    void addTasks(const KPlato::Node *parent, TJ::Task *tjParent = nullptr, int projectPriority = 0);

private:
    MainSchedule *m_schedule;
    bool m_recalculate;
    bool m_usePert;
    bool m_backward;
    TJ::Project *m_tjProject;
//     Task *m_backwardTask;

    QMap<TJ::Task*, Node*> m_taskmap;
    QMap<TJ::Resource*, Resource*> m_resourcemap;
    QMap<QString, Resource*> m_resourceIds;

    ulong m_granularity;
};

#endif // PLANTJSCHEDULER_H
