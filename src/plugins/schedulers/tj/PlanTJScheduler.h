/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2009 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2011 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PLANTJSCHEDULER_H
#define PLANTJSCHEDULER_H

#include "plantj_export.h"

#include "kptschedulerplugin.h"

#include "kptdatetime.h"
#include "kptappointment.h"

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

class PLANTJ_EXPORT PlanTJScheduler : public KPlato::SchedulerThread
{
    Q_OBJECT

private:

public:
    PlanTJScheduler(Project *project, ScheduleManager *sm, ulong granularity, QObject *parent = nullptr);
    explicit PlanTJScheduler(ulong granularity = 0, QObject *parent = nullptr);
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
    void insertProject(KoDocument *doc, int priority, KPlato::SchedulingContext &context);
    void insertBookings(KPlato::SchedulingContext &context);
    void addTasks(const KPlato::Node *parent, TJ::Task *tjParent = nullptr, int projectPriority = 0);
    void addPastAppointments(Node *task);

    void calculateParallel(KPlato::SchedulingContext &context);
    void calculateSequential(KPlato::SchedulingContext &context);

private:
    MainSchedule *m_schedule;
    bool m_recalculate;
    DateTime m_recalculateFrom;
    bool m_usePert;
    bool m_backward;
    TJ::Project *m_tjProject;
//     Task *m_backwardTask;

    QMap<TJ::Task*, Node*> m_taskmap;
    QMap<TJ::Resource*, Resource*> m_resourcemap;
    QMap<QString, Resource*> m_resourceIds;
    QList<Task*> m_durationTasks;
};

#endif // PLANTJSCHEDULER_H
