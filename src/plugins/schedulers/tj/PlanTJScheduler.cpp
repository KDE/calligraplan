/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2009, 2010, 2011, 2012 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "PlanTJScheduler.h"

#include "kptproject.h"
#include "kptschedule.h"
#include "kptresource.h"
#include "kpttask.h"
#include "kptrelation.h"
#include "kptduration.h"
#include "kptcalendar.h"
#include "kptdebug.h"

#include <KoDocument.h>
#include <ExtraProperties.h>

#include "taskjuggler/taskjuggler.h"
#include "taskjuggler/Project.h"
#include "taskjuggler/Scenario.h"
#include "taskjuggler/Resource.h"
#include "taskjuggler/Task.h"
#include "taskjuggler/Interval.h"
#include "taskjuggler/Allocation.h"
#include "taskjuggler/Utility.h"
#include "taskjuggler/UsageLimits.h"
#include "taskjuggler/CoreAttributes.h"
#include "taskjuggler/TjMessageHandler.h"
#include "taskjuggler/debug.h"

#include <QString>
#include <QTime>
#include <QMutexLocker>
#include <QMap>
#include <QLocale>

#include <KLocalizedString>
#include <KFormat>

#include <iostream>

#define PROGRESS_MAX_VALUE 100


PlanTJScheduler::PlanTJScheduler(Project *project, ScheduleManager *sm, ulong granularity, QObject *parent)
    : SchedulerThread(project, sm, granularity, parent),
    result(-1),
    m_schedule(nullptr),
    m_recalculate(false),
    m_usePert(false),
    m_backward(false),
    m_tjProject(nullptr)
{
    TJ::TJMH.reset();
    connect(&TJ::TJMH, &TJ::TjMessageHandler::message, this, &PlanTJScheduler::slotMessage);

    connect(this, &PlanTJScheduler::sigCalculationStarted, project, &KPlato::Project::sigCalculationStarted);

    connect(this, &PlanTJScheduler::sigCalculationFinished, project, &KPlato::Project::sigCalculationFinished);
}

PlanTJScheduler::PlanTJScheduler(ulong granularity, QObject *parent)
    : SchedulerThread(granularity, parent)
    , result(-1)
    , m_schedule(nullptr)
    , m_recalculate(false)
    , m_usePert(false)
    , m_backward(false)
    , m_tjProject(nullptr)
{
    TJ::TJMH.reset();
    //connect(&TJ::TJMH, &TJ::TjMessageHandler::message, this, &PlanTJScheduler::slotMessage);

//     connect(this, &PlanTJScheduler::sigCalculationStarted, project, &KPlato::Project::sigCalculationStarted);
//     Q_EMIT sigCalculationStarted(project, sm);
//
//     connect(this, &PlanTJScheduler::sigCalculationFinished, project, &KPlato::Project::sigCalculationFinished);
}

PlanTJScheduler::~PlanTJScheduler()
{
    delete m_tjProject;
}

void PlanTJScheduler::slotMessage(int type, const QString &msg, TJ::CoreAttributes *object)
{
//     debugPlan<<"PlanTJScheduler::slotMessage:"<<msg;
    Schedule::Log log;
    if (object &&  object->getType() == CA_Task && m_taskmap.contains(static_cast<TJ::Task*>(object))) {
        log = Schedule::Log(static_cast<Node*>(m_taskmap[ static_cast<TJ::Task*>(object) ]), type, msg);
    } else if (object && object->getType() == CA_Resource && m_resourcemap.contains(static_cast<TJ::Resource*>(object))) {
        log = Schedule::Log(nullptr, m_resourcemap[ static_cast<TJ::Resource*>(object) ], type, msg);
    } else if (object && ! object->getName().isEmpty()) {
        log = Schedule::Log(static_cast<Node*>(m_project), type, QString("%1: %2").arg(object->getName()).arg(msg));
    } else {
        log = Schedule::Log(static_cast<Node*>(m_project), type, msg);
    }
    slotAddLog(log);
}

void PlanTJScheduler::run()
{
    if (m_haltScheduling) {
        deleteLater();
        return;
    }
    if (m_stopScheduling) {
        return;
    }
    Q_EMIT sigCalculationStarted(m_mainproject, m_mainmanager);
    setMaxProgress(PROGRESS_MAX_VALUE);
    m_tjProject = new TJ::Project();
    { // mutex -->
        m_projectMutex.lock();
        m_managerMutex.lock();

        m_project = new Project();
        loadProject(m_project, m_pdoc);
        m_project->setName("Schedule: " + m_project->name()); //Debug
        m_project->stopcalculation = false;
        m_manager = m_project->scheduleManager(m_mainmanagerId);
        Q_CHECK_PTR(m_manager);
        Q_ASSERT(m_manager->expected());
        Q_ASSERT(m_manager != m_mainmanager);
        Q_ASSERT(m_manager->scheduleId() == m_mainmanager->scheduleId());
        Q_ASSERT(m_manager->expected() != m_mainmanager->expected());
        m_manager->setName("Schedule: " + m_manager->name()); //Debug
        m_schedule = m_manager->expected();

        bool x = connect(m_manager, SIGNAL(sigLogAdded(KPlato::Schedule::Log)), this, SLOT(slotAddLog(KPlato::Schedule::Log)));
        Q_ASSERT(x);
        Q_UNUSED(x);

        m_project->initiateCalculation(*m_schedule);
        m_project->initiateCalculationLists(*m_schedule);

        m_usePert = m_manager->usePert();
        m_recalculate = m_manager->recalculate();
        if (m_recalculate) {
            m_backward = false;
        } else {
            m_backward = m_manager->schedulingDirection();
        }
        m_project->setCurrentSchedule(m_manager->expected()->id());

        m_schedule->setPhaseName(0, xi18nc("@info/plain" , "Init"));
        QLocale locale;
        KFormat format(locale);
        if (! m_backward) {
            logDebug(m_project, nullptr, QString("Schedule project using TJ Scheduler, starting at %1, granularity %2").arg(QDateTime::currentDateTime().toString()).arg(format.formatDuration(m_granularity)), 0);
            if (m_recalculate) {
                m_recalculateFrom = m_manager->recalculateFrom();
                logInfo(m_project, nullptr, xi18nc("@info/plain" , "Re-calculate project from start time: %1", locale.toString(m_recalculateFrom, QLocale::ShortFormat)), 0);
                auto t = new TJ::Task(m_tjProject, "TJ::RECALCULATE_FROM", "TJ::RECALCULATE_FROM", nullptr, QString(), 0);
                t->setMilestone(true);
                t->setSpecifiedStart(0, toTJTime_t(m_recalculateFrom, tjGranularity()));
                logDebug(m_project, nullptr, QString("Re-calculate milestone created at: %1").arg(TJ::formatTime(t->getSpecifiedStart(0)))); //NOLINT(clang-analyzer-cplusplus.NewDeleteLeaks)
            } else {
                logInfo(m_project, nullptr, xi18nc("@info/plain" , "Schedule project from start time: %1", locale.toString(m_project->constraintStartTime(), QLocale::ShortFormat)), 0);
            }
            logInfo(m_project, nullptr, xi18nc("@info/plain" , "Project target finish time: %1", locale.toString(m_project->constraintEndTime(), QLocale::ShortFormat)), 0);
        } else {
            logDebug(m_project, nullptr, QString("Schedule project backward using TJ Scheduler, starting at %1, granularity %2").arg(locale.toString(QDateTime::currentDateTime(), QLocale::ShortFormat)).arg(format.formatDuration(m_granularity)), 0);
            logInfo(m_project, nullptr, xi18nc("@info/plain" , "Schedule project from end time: %1", locale.toString(m_project->constraintEndTime(), QLocale::ShortFormat)), 0);
        }

        m_managerMutex.unlock();
        m_projectMutex.unlock();
    } // <--- mutex
    setProgress(2);
    if (! kplatoToTJ()) {
        result = 1;
        m_manager->setCalculationResult(ScheduleManager::CalculationError);
        setProgress(PROGRESS_MAX_VALUE);
        return;
    }
    setMaxProgress(PROGRESS_MAX_VALUE);
    connect(m_tjProject, SIGNAL(updateProgressBar(int,int)), this, SLOT(setProgress(int)));

    m_schedule->setPhaseName(1, xi18nc("@info/plain" , "Schedule"));
    logInfo(m_project, nullptr, "Start scheduling", 1);
    bool r = solve();
    if (! r) {
        debugPlan<<"Scheduling failed";
        result = 2;
        m_manager->setCalculationResult(ScheduleManager::CalculationError);
        logError(m_project, nullptr, xi18nc("@info/plain" , "Failed to schedule project"));
        setProgress(PROGRESS_MAX_VALUE);
        return;
    }
    if (m_haltScheduling) {
        debugPlan<<"Scheduling halted";
        m_manager->setCalculationResult(ScheduleManager::CalculationStopped);
        logInfo(m_project, nullptr, "Scheduling halted");
        deleteLater();
        return;
    }
    m_schedule->setPhaseName(2, xi18nc("@info/plain" , "Update"));
    logInfo(m_project, nullptr, "Scheduling finished, update project", 2);
    if (! kplatoFromTJ()) {
        m_manager->setCalculationResult(ScheduleManager::CalculationError);
        logError(m_project, nullptr, "Project update failed");
    }
    m_manager->setCalculationResult(ScheduleManager::CalculationDone);
    setProgress(PROGRESS_MAX_VALUE);
    m_schedule->setPhaseName(3, xi18nc("@info/plain" , "Finish"));
}

bool PlanTJScheduler::check()
{
    DebugCtrl.setDebugMode(0);
    DebugCtrl.setDebugLevel(1000);
    return m_tjProject->pass2(true);
}

bool PlanTJScheduler::solve()
{
    debugPlan;
    TJ::Scenario *sc = m_tjProject->getScenario(0);
    if (! sc) {
        logError(m_project, nullptr, xi18nc("@info/plain" , "Failed to find scenario to schedule"));
        return false;
    }
    DebugCtrl.setDebugLevel(0);
    DebugCtrl.setDebugMode(PSDEBUG+TSDEBUG+RSDEBUG+PADEBUG);

    return m_tjProject->scheduleScenario(sc);
}

bool PlanTJScheduler::kplatoToTJ()
{
    m_tjProject->setPriority(m_project->priority());
    m_tjProject->setScheduleGranularity(m_granularity / 1000);
    m_tjProject->getScenario(0)->setMinSlackRate(0.0); // Do not calculate critical path

    m_tjProject->setNow(m_project->constraintStartTime().toSecsSinceEpoch());
    m_tjProject->setStart(m_project->constraintStartTime().toSecsSinceEpoch());
    m_tjProject->setEnd(m_project->constraintEndTime().toSecsSinceEpoch());

    m_tjProject->setDailyWorkingHours(m_project->standardWorktime()->day());

    // Set working days for the project, it is used for tasks with a length specification
    // FIXME: Plan has task specific calendars for this estimate type
    KPlato::Calendar *cal = m_project->defaultCalendar();
    if (! cal) {
        cal = m_project->calendars().value(0);
    }
    if (cal) {
        int days[ 7 ] = { Qt::Sunday, Qt::Monday, Qt::Tuesday, Qt::Wednesday, Qt::Thursday, Qt::Friday, Qt::Saturday };
        for (int i = 0; i < 7; ++i) {
            CalendarDay *d = nullptr;
            for (Calendar *c = cal; c; c = c->parentCal()) {
                QElapsedTimer t; t.start();
                d = c->weekday(days[ i ]);
                Q_ASSERT(d);
                if (d == nullptr || d->state() != CalendarDay::Undefined) {
                    break;
                }
            }
            if (d && d->state() == CalendarDay::Working) {
                QList<TJ::Interval*> lst;
                const auto intervals = d->timeIntervals();
                for (const TimeInterval *ti : intervals) {
                    TJ::Interval *tji = new TJ::Interval(toTJInterval(ti->startTime(), ti->endTime(),tjGranularity()));
                    lst << tji;
                }
                m_tjProject->setWorkingHours(i, lst);
                qDeleteAll(lst);
            }
        }
    }
    addTasks();
    setConstraints();
    addDependencies();
    addRequests();
    addStartEndJob();

    if (result != -1) {
        return false;
    }
    return check();
}

void PlanTJScheduler::addStartEndJob()
{
    const auto startId("TJ::StartJob");
    const auto endId("TJ::EndJob");

    TJ::Task *start = m_tjProject->getTask(startId);
    if (!start) {
        start = new TJ::Task(m_tjProject, startId, startId, nullptr, QString(), 0);
    }
    start->setMilestone(true);
    if (! m_backward) {
        start->setSpecifiedStart(0, m_tjProject->getStart());
        start->setPriority(999);
    } else {
        // backwards: insert a new ms before start and make start an ALAP to push all other jobs ALAP
        TJ::Task *bs = m_tjProject->getTask(startId);
        if (!bs) {
            bs = new TJ::Task(m_tjProject, "TJ::StartJob-B", "TJ::StartJob-B", nullptr, QString(), 0);
            bs->setMilestone(true);
            bs->addPrecedes(start->getId());
            start->addDepends(bs->getId());
            start->setScheduling(TJ::Task::ALAP);
        }
        bs->setSpecifiedStart(0, m_tjProject->getStart());
        bs->setPriority(999);
    }
    TJ::Task *end = m_tjProject->getTask(endId);
    if (!end) {
        end = new TJ::Task(m_tjProject, endId, endId, nullptr, QString(), 0);
    }
    end->setMilestone(true);
    if (m_backward) {
        end->setSpecifiedEnd(0, m_tjProject->getEnd() - 1);
        end->setScheduling(TJ::Task::ALAP);
    }
    for (QMap<TJ::Task*, Node*>::ConstIterator it = m_taskmap.constBegin(); it != m_taskmap.constEnd(); ++it) {
        if (it.value()->isStartNode()) {
            it.key()->addDepends(start->getId());
            //logDebug(m_project, 0, QString("'%1' depends on: '%2'").arg(it.key()->getName()).arg(start->getName()));
            if (start->getScheduling() == TJ::Task::ALAP) {
                start->addPrecedes(it.key()->getId());
                //logDebug(m_project, 0, QString("'%1' precedes: '%2'").arg(start->getName()).arg(it.key()->getName()));
            }
        }
        if (it.value()->isEndNode()) {
            end->addDepends(it.key()->getId());
            if (it.key()->getScheduling() == TJ::Task::ALAP) {
                it.key()->addPrecedes(end->getId());
            }
        }
    }
} //NOLINT(clang-analyzer-cplusplus.NewDeleteLeaks)

// static
int PlanTJScheduler::toTJDayOfWeek(int day)
{
    return day == 7 ? 0 : day;
}

// static
DateTime PlanTJScheduler::fromTime_t(time_t t, const QTimeZone &tz) {
    return DateTime (QDateTime::fromSecsSinceEpoch(t).toTimeZone(tz));
}

time_t PlanTJScheduler::toTJTime_t(const QDateTime &dt, ulong granularity)
{
    int secs = QTime(0, 0, 0).secsTo(dt.time());
    secs -= secs % granularity;
    return QDateTime(dt.date(), QTime(0, 0, 0).addSecs(secs), dt.timeZone()).toSecsSinceEpoch();
}

// static
AppointmentInterval PlanTJScheduler::fromTJInterval(const TJ::Interval &tji, const QTimeZone &tz) {
    AppointmentInterval a(fromTime_t(tji.getStart(), tz), fromTime_t(tji.getEnd(), tz).addSecs(1));
    return a;
}

// static
TJ::Interval PlanTJScheduler::toTJInterval(const QDateTime &start, const QDateTime &end, ulong granularity) {
    int secs = QTime(0, 0, 0).secsTo(start.time());
    secs -= secs % granularity;
    QDateTime s(start.date(), QTime(0, 0, 0).addSecs(secs), start.timeZone());
    secs = QTime(0, 0, 0).secsTo(end.time());
    secs -= secs % granularity;
    QDateTime e(end.date(), QTime(0, 0, 0).addSecs(secs), end.timeZone());
    TJ::Interval ti(s.toSecsSinceEpoch(), e.addSecs(-1).toSecsSinceEpoch());
    return ti;
}

// static
TJ::Interval PlanTJScheduler::toTJInterval(const QTime &start, const QTime &end, ulong granularity) {
    int secs = QTime(0, 0, 0).secsTo(start);
    time_t s =  secs - (secs % granularity);
    secs = (end == QTime(0, 0, 0)) ? 86399 : QTime(0, 0, 0).secsTo(end);
    time_t e = secs - (secs % granularity) - 1;
    TJ::Interval ti(s, e);
    return ti;
}

ulong PlanTJScheduler::tjGranularity() const
{
    return m_tjProject->getScheduleGranularity();
}

bool PlanTJScheduler::kplatoFromTJ()
{
    MainSchedule *cs = static_cast<MainSchedule*>(m_project->currentSchedule());

    QDateTime start;
    QDateTime end;
    for (QMap<TJ::Task*, Node*>::ConstIterator it = m_taskmap.constBegin(); it != m_taskmap.constEnd(); ++it) {
        if (! taskFromTJ(it.key(), it.value())) {
            return false;
        }
        if (! start.isValid() || it.value()->startTime() < start) {
            start = it.value()->startTime();
        }
        if (! end.isValid() || it.value()->endTime() > end) {
            end = it.value()->endTime();
        }
    }
    m_project->setStartTime(start.isValid() ? start : m_project->constraintStartTime());
    m_project->setEndTime(end.isValid() ? end : m_project->constraintEndTime());

    adjustSummaryTasks(m_schedule->summaryTasks());

    for (Node *task : std::as_const(m_taskmap)) {
        calcPertValues(task);
    }

    m_project->calcCriticalPathList(m_schedule);
    // calculate positive float
    for (Node* t : std::as_const(m_taskmap)) {
        if (! t->inCriticalPath() && t->isStartNode()) {
            calcPositiveFloat(t);
        }
    }

    QLocale locale;
    logInfo(m_project, nullptr, xi18nc("@info/plain" , "Project scheduled to start at %1 and finish at %2", locale.toString(m_project->startTime(), QLocale::ShortFormat), locale.toString(m_project->endTime(), QLocale::ShortFormat)));

    if (m_manager) {
        logDebug(m_project, nullptr, QString("Project scheduling finished at %1").arg(locale.toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));
        m_project->finishCalculation(*m_manager);
        m_manager->scheduleChanged(cs);
    }
    return true;
}

bool PlanTJScheduler::taskFromTJ(TJ::Task *job, Node *task)
{
    return taskFromTJ(m_project, job, task);
}

bool PlanTJScheduler::taskFromTJ(Project *project, TJ::Task *job, Node *task)
{
    if (m_haltScheduling) {
        return true;
    }
    if (task->type() == KPlato::Node::Type_Summarytask || task->type() == KPlato::Node::Type_Project) {
        return true;
    }
    Q_ASSERT(project == task->projectNode());
    Schedule *cs = task->currentSchedule();
    Q_ASSERT(cs);
    QTimeZone tz = m_project->timeZone();
    time_t s = job->getStart(0);
    if (s < m_tjProject->getStart() || s > m_tjProject->getEnd()) {
        project->currentSchedule()->setSchedulingError(true);
        cs->setSchedulingError(true);
        s = m_tjProject->getStart();
    }
    time_t e = job->getEnd(0);
    if (job->isMilestone()) {
        Q_ASSERT(s = (e + 1));
        e = s - 1;
    } else if (e <= s || e > m_tjProject->getEnd()) {
        project->currentSchedule()->setSchedulingError(true);
        cs->setSchedulingError(true);
        e = s + (8*60*60);
    }
    task->setStartTime(fromTime_t(s, tz));
    task->setEndTime(fromTime_t(e + 1, tz));
    task->setDuration(task->endTime() - task->startTime());

    //logDebug(task, nullptr, QString("%1, %2 - %3, %4").arg(TJ::time2ISO(s)).arg(task->startTime().toString(Qt::ISODate)).arg(TJ::time2ISO(e+1)).arg(task->endTime().toString(Qt::ISODate)));
    if (! task->startTime().isValid()) {
        logError(task, nullptr, xi18nc("@info/plain", "Invalid start time"));
        return false;
    }
    if (! task->endTime().isValid()) {
        logError(task, nullptr, xi18nc("@info/plain", "Invalid end time"));
        return false;
    }
    if (!project->startTime().isValid() || project->startTime() > task->startTime()) {
        project->setStartTime(task->startTime());
    }
    if (task->endTime() > project->endTime()) {
        project->setEndTime(task->endTime());
    }
    const auto lst = job->getBookedResources(0);
    for (TJ::CoreAttributes *a : lst) {
        TJ::Resource *r = static_cast<TJ::Resource*>(a);
        Resource *res = resource(project, r);
        Q_ASSERT(res);
        const QVector<TJ::Interval> lst = r->getBookedIntervals(0, job);
        for (const TJ::Interval &tji : lst) {
            AppointmentInterval ai = fromTJInterval(tji, tz);
            double load = res->type() == Resource::Type_Material ? res->units() : ai.load() * r->getEfficiency();
            res->addAppointment(cs, ai.startTime(), ai.endTime(), load);
            logDebug(task, nullptr, '\'' + res->name() + "' added appointment: " +  ai.startTime().toString(Qt::ISODate) + " - " + ai.endTime().toString(Qt::ISODate));
        }
    }
    if (m_recalculate && static_cast<Task*>(task)->isStarted() && task->estimate()->type() == Estimate::Type_Effort) {
        addPastAppointments(task);
    }

    cs->setScheduled(true);
    QLocale locale;
    if (task->type() == Node::Type_Milestone) {
        logInfo(task, nullptr, xi18nc("@info/plain" , "Scheduled milestone: %1", locale.toString(task->startTime(), QLocale::ShortFormat)));
    } else {
        logInfo(task, nullptr, xi18nc("@info/plain" , "Scheduled task: %1 - %2", locale.toString(task->startTime(), QLocale::ShortFormat), locale.toString(task->endTime(), QLocale::ShortFormat)));
    }
    return true;
}

void PlanTJScheduler::addPastAppointments(Node *task)
{
    if (!static_cast<Task*>(task)->isStarted()) {
        logDebug(task, nullptr, QStringLiteral("Task is not started, no appointments to copy"));
        return;
    }
    if (static_cast<Task*>(task)->completion().isFinished()) {
        static_cast<Task*>(task)->copySchedule();
    } else {
        static_cast<Task*>(task)->createAndMergeAppointmentsFromCompletion();
    }
    return;
}

Resource *PlanTJScheduler::resource(Project *project, TJ::Resource *tjResource)
{
    return project->resource(tjResource->getId());
}

void PlanTJScheduler::adjustSummaryTasks(const QList<Node*> &nodes)
{
    for (Node *n : nodes) {
        adjustSummaryTasks(n->childNodeIterator());
        if (n->parentNode()->type() == Node::Type_Summarytask) {
            DateTime pt = n->parentNode()->startTime();
            DateTime nt = n->startTime();
            if (! pt.isValid() || pt > nt) {
                n->parentNode()->setStartTime(nt);
            }
            pt = n->parentNode()->endTime();
            nt = n->endTime();
            if (! pt.isValid() || pt < nt) {
                n->parentNode()->setEndTime(nt);
            }
        }
    }
}

Duration PlanTJScheduler::calcPositiveFloat(Node *task)
{
    if (static_cast<Task*>(task)->positiveFloat() != 0) {
        return static_cast<Task*>(task)->positiveFloat();
    }
    Duration x;
    if (task->dependChildNodes().isEmpty() && static_cast<Task*>(task)->childProxyRelations().isEmpty()) {
        x = m_project->endTime() - task->endTime();
    } else {
        const auto lst = task->dependChildNodes() + static_cast<Task*>(task)->childProxyRelations();
        for (const Relation *r : lst) {
            if (! r->child()->inCriticalPath()) {
                Duration f = calcPositiveFloat(static_cast<Task*>(r->child()));
                if (x == 0 || f < x) {
                    x = f;
                }
            }
        }
    }
    Duration totfloat = static_cast<Task*>(task)->freeFloat() + x;
    static_cast<Task*>(task)->setPositiveFloat(totfloat);
    return totfloat;
}

void PlanTJScheduler::calcPertValues(Node *t)
{
    switch (t->constraint()) {
    case Node::MustStartOn:
        if (t->constraintStartTime() != t->startTime()) {
            static_cast<Task*>(t)->setNegativeFloat(t->startTime() - t->constraintStartTime());
        }
        break;
    case Node::StartNotEarlier:
        if (t->startTime() < t->constraintStartTime()) {
            static_cast<Task*>(t)->setNegativeFloat(t->constraintStartTime() - t->startTime());
        }
        break;
    case Node::MustFinishOn:
        if (t->constraintEndTime() != t->endTime()) {
            static_cast<Task*>(t)->setNegativeFloat(t->endTime() - t->constraintEndTime());
        }
        break;
    case Node::FinishNotLater:
        if (t->endTime() > t->constraintEndTime()) {
            static_cast<Task*>(t)->setNegativeFloat(t->endTime() - t->constraintEndTime());
        }
        break;
    case Node::FixedInterval:
        if (t->constraintStartTime() != t->startTime()) {
            static_cast<Task*>(t)->setNegativeFloat(t->startTime() - t->constraintStartTime());
        } else if (t->endTime() != t->constraintEndTime()) {
            static_cast<Task*>(t)->setNegativeFloat(t->endTime() - t->constraintEndTime());
        }
        break;
    default:
        break;
    }
    if (static_cast<Task*>(t)->negativeFloat() != 0) {
        t->currentSchedule()->setConstraintError(true);
        m_project->currentSchedule()->setSchedulingError(true);
        logError(t, nullptr, i18nc("1=type of constraint", "%1: Failed to meet constraint. Negative float=%2", t->constraintToString(true), static_cast<Task*>(t)->negativeFloat().toString(Duration::Format_i18nHour)));
    }
    //debugPlan<<t->name()<<t->startTime()<<t->endTime();
    Duration negativefloat;
    const auto lst = t->dependParentNodes() + static_cast<Task*>(t)->parentProxyRelations();
    for (const Relation *r : lst) {
        if (r->parent()->endTime() + r->lag() > t->startTime()) {
            Duration f = r->parent()->endTime() + r->lag() - t->startTime();
            if (f > negativefloat) {
                negativefloat = f;
            }
        }
    }
    if (negativefloat > 0) {
        t->currentSchedule()->setSchedulingError(true);
        m_project->currentSchedule()->setSchedulingError(true);
        logWarning(t, nullptr, xi18nc("@info/plain", "Failed to meet dependency. Negative float=%1", negativefloat.toString(Duration::Format_i18nHour)));
        if (static_cast<Task*>(t)->negativeFloat() < negativefloat) {
            static_cast<Task*>(t)->setNegativeFloat(negativefloat);
        }
    }
    Duration freefloat;
    const auto relations = t->dependChildNodes() + static_cast<Task*>(t)->childProxyRelations();
    for (const Relation *r : relations) {
        if (t->endTime() + r->lag() <  r->child()->startTime()) {
            Duration f = r->child()->startTime() - r->lag() - t->endTime();
            if (f > 0 && (freefloat == 0 || freefloat > f)) {
                freefloat = f;
            }
        }
    }
    static_cast<Task*>(t)->setFreeFloat(freefloat);
}

bool PlanTJScheduler::exists(QList<CalendarDay*> &lst, CalendarDay *day)
{
    for (CalendarDay *d : std::as_const(lst)) {
        if (d->date() == day->date() && day->state() != CalendarDay::Undefined && d->state() != CalendarDay::Undefined) {
            return true;
        }
    }
    return false;
}

TJ::Resource *PlanTJScheduler::addResource(KPlato::Resource *r)
{
    KPlato::Resource *resource = m_resourceIds.value(r->id());
    if (!resource) {
        m_resourceIds.insert(r->id(), r);
        resource = r;
    }
    TJ::Resource *res = m_resourcemap.key(resource);
    if (res) {
        debugPlan<<r->name()<<"already exist";
        return res;
    }
    res = new TJ::Resource(m_tjProject, resource->id(), resource->name(), nullptr);
    if (resource->type() == Resource::Type_Material) {
        res->setEfficiency(0.0);
    } else {
        res->setEfficiency((double)(resource->units()) / 100.);
    }
    Calendar *cal = resource->calendar();
    Q_ASSERT(cal);
    DateTime start = qMax(resource->availableFrom(), m_project->constraintStartTime());
    DateTime end = m_project->constraintEndTime();
    if (resource->availableUntil().isValid() && end > resource->availableUntil()) {
        end = resource->availableUntil();
    }
    AppointmentIntervalList lst = cal->workIntervals(start, end, 1.0);
//     qDebug()<<r<<lst;
    const QMultiMap<QDate, AppointmentInterval> &map = lst.map();
    QMultiMap<QDate, AppointmentInterval>::const_iterator mapend = map.constEnd();
    QMultiMap<QDate, AppointmentInterval>::const_iterator it = map.constBegin();
    TJ::Shift *shift = new TJ::Shift(m_tjProject, resource->id(), resource->name(), nullptr, QString(), 0);
    for (; it != mapend; ++it) {
        shift->addWorkingInterval(toTJInterval(it.value().startTime(), it.value().endTime(), m_granularity/1000));
    }
    res->addShift(toTJInterval(start, end, m_granularity/1000), shift);
    m_resourcemap[res] = resource;
    logDebug(m_project, nullptr, "Added resource: " + resource->name());
/*    QListIterator<TJ::Interval*> vit = res->getVacationListIterator();
    while (vit.hasNext()) {
        TJ::Interval *i = vit.next();
        logDebug(m_project, 0, "Vacation: " + TJ::time2ISO(i->getStart()) + " - " + TJ::time2ISO(i->getEnd()));
    }*/
    return res;
}

TJ::Task *PlanTJScheduler::addTask(const KPlato::Node *node, TJ::Task *parent)
{
/*    if (m_backward && task->isStartNode()) {
        Relation *r = new Relation(m_backwardTask, task);
        m_project->addRelation(r);
    }*/
    TJ::Task *t = new TJ::Task(m_tjProject, node->id(), node->name(), parent, QString(), 0);
    t->setPriority(node->priority());
    if (node->type() == KPlato::Node::Type_Task) {
        m_taskmap[t] = const_cast<KPlato::Task*>(static_cast<const KPlato::Task*>(node));
        addWorkingTime(static_cast<const KPlato::Task*>(node), t);
    } else if (node->type() == KPlato::Node::Type_Milestone) {
        m_taskmap[t] = const_cast<KPlato::Task*>(static_cast<const KPlato::Task*>(node));
    } else if (node->type() == KPlato::Node::Type_Project) {
        if (node->constraint() == Node::MustStartOn) {
            t->setSpecifiedStart(0, node->constraintStartTime().toSecsSinceEpoch());
        } else {
            t->setSpecifiedEnd(0, node->constraintEndTime().toSecsSinceEpoch());
        }
    }
    return t;
}

void PlanTJScheduler::addWorkingTime(const KPlato::Task *task, TJ::Task *job)
{
    if (task->type() != Node::Type_Task || task->estimate()->type() != Estimate::Type_Duration || ! task->estimate()->calendar()) {
        return;
    }
    int id = 0;
    Calendar *cal = task->estimate()->calendar();
    DateTime start = m_project->constraintStartTime();
    DateTime end = m_project->constraintEndTime();

    AppointmentIntervalList lst = cal->workIntervals(start, end, 1.0);
    const QMultiMap<QDate, AppointmentInterval> &map = lst.map();
    QMultiMap<QDate, AppointmentInterval>::const_iterator mapend = map.constEnd();
    QMultiMap<QDate, AppointmentInterval>::const_iterator it = map.constBegin();
    TJ::Shift *shift = new TJ::Shift(m_tjProject, task->id() + QString("-%1").arg(++id), task->name(), nullptr, QString(), 0);
    for (; it != mapend; ++it) {
        shift->addWorkingInterval(toTJInterval(it.value().startTime(), it.value().endTime(), m_granularity/1000));
    }
    job->addShift(toTJInterval(start, end, m_granularity/1000), shift);
}

void PlanTJScheduler::addTasks()
{
    //debugPlan;
    QList<Node*> list = m_project->allNodes();
    for (int i = 0; i < list.count(); ++i) {
        Node *n = list.at(i);
        TJ::Task *parent = nullptr;
        switch (n->type()) {
            case Node::Type_Summarytask:
                m_schedule->insertSummaryTask(n);
                break;
            case Node::Type_Task:
            case Node::Type_Milestone:
                switch (n->constraint()) {
                    case Node::StartNotEarlier:
                        parent = addStartNotEarlier(n);
                        break;
                case Node::FinishNotLater:
                    parent = addFinishNotLater(n);
                    break;
                }
                addTask(static_cast<Task*>(n), parent);
                break;
            default:
                break;
        }
    }
}

void PlanTJScheduler::addDepends(const Relation *rel)
{
    TJ::Task *child = m_tjProject->getTask(rel->child()->id());
    if (!child) {
        logWarning(rel->parent(), nullptr, xi18nc("@info/plain" , "Failed to add as predecessor to task '%1'", rel->child()->name()));
        return;
    }
    TJ::TaskDependency *d = child->addDepends(rel->parent()->id());
    d->setGapDuration(0, rel->lag().seconds());
}

void PlanTJScheduler::addPrecedes(const Relation *rel)
{
    TJ::Task *parent = m_tjProject->getTask(rel->parent()->id());
    if (!parent) {
        logWarning(rel->child(), nullptr, xi18nc("@info/plain" , "Failed to add as successor to task '%1'", rel->parent()->name()));
        return;
    }
    TJ::TaskDependency *d = parent->addPrecedes(rel->child()->id());
    d->setGapDuration(0, rel->lag().seconds());
}

void PlanTJScheduler::addDependencies(KPlato::Node *task)
{
    const auto lst = task->dependParentNodes() + static_cast<Task*>(task)->parentProxyRelations();
    for (Relation *r : lst) {
        Node *n = r->parent();
        if (n == nullptr || n->type() == Node::Type_Summarytask) {
            continue;
        }
        switch (r->type()) {
            case Relation::FinishStart:
                break;
            case Relation::FinishFinish:
            case Relation::StartStart:
                warnPlan<<"Dependency type not handled. Using FinishStart.";
                logWarning(task, nullptr, xi18nc("@info/plain" , "Dependency type '%1' not handled. Using FinishStart.", r->typeToString(true)));
                break;
        }
        switch (task->constraint()) {
            case Node::ASAP:
            case Node::ALAP:
                addPrecedes(r);
                addDepends(r);
                break;
            case Node::MustStartOn:
            case Node::StartNotEarlier:
                addPrecedes(r);
                if (task->constraintStartTime() < m_project->constraintStartTime()) {
                    addDepends(r);
                }
                break;
            case Node::MustFinishOn:
            case Node::FinishNotLater:
                addDepends(r);
                if (task->constraintEndTime() < m_project->constraintEndTime()) {
                    addPrecedes(r);
                }
                break;
            case Node::FixedInterval:
                break;
        }
    }
}

void PlanTJScheduler::addDependencies()
{
    for (Node *t : std::as_const(m_taskmap)) {
        addDependencies(t);
    }
}

void PlanTJScheduler::setConstraints()
{
    QMap<TJ::Task*, Node*> ::const_iterator it = m_taskmap.constBegin();
    for (; it != m_taskmap.constEnd(); ++it) {
        setConstraint(it.key(), it.value());
    }
}

void PlanTJScheduler::setConstraint(TJ::Task *job, KPlato::Node *task)
{
    switch (task->constraint()) {
        case Node::ASAP:
            if (! job->isMilestone()) {
                job->setScheduling(m_backward ? TJ::Task::ALAP : TJ::Task::ASAP);
            }
            break;
        case Node::ALAP:
            job->setScheduling(TJ::Task::ALAP);
            break;
        case Node::MustStartOn:
            if (task->constraintStartTime() >= m_project->constraintStartTime()) {
                job->setPriority(600);
                job->setSpecifiedStart(0, task->constraintStartTime().toSecsSinceEpoch());
                logDebug(task, nullptr, QString("MSO: set specified start: %1").arg(TJ::time2ISO(task->constraintStartTime().toSecsSinceEpoch())));
            } else {
                logWarning(task, nullptr, xi18nc("@info/plain", "%1: Invalid start constraint", task->constraintToString(true)));
            }
            break;
        case Node::StartNotEarlier: {
            break;
        }
        case Node::MustFinishOn:
            if (task->constraintEndTime() <= m_project->constraintEndTime()) {
                job->setPriority(600);
                job->setScheduling(TJ::Task::ALAP);
                job->setSpecifiedEnd(0, task->constraintEndTime().toSecsSinceEpoch() - 1);
                logDebug(task, nullptr, QString("MFO: set specified end: %1").arg(TJ::time2ISO(task->constraintEndTime().toSecsSinceEpoch())));
            } else {
                logWarning(task, nullptr, xi18nc("@info/plain", "%1: Invalid end constraint", task->constraintToString(true)));
            }
            break;
        case Node::FinishNotLater: {
            break;
        }
        case Node::FixedInterval: {
            job->setPriority(700);
            TJ::Interval i(toTJInterval(task->constraintStartTime(), task->constraintEndTime(), tjGranularity()));
            job->setSpecifiedPeriod(0, i);
            // estimate not allowed
            job->setDuration(0, 0.0);
            job->setLength(0, 0.0);
            job->setEffort(0, 0.0);
            logDebug(task, nullptr, QString("FI: set specified: %1 - %2 -> %3 - %4 (%5)")
                      .arg(TJ::time2ISO(task->constraintStartTime().toSecsSinceEpoch()))
                      .arg(TJ::time2ISO(task->constraintEndTime().toSecsSinceEpoch()))
                      .arg(TJ::time2ISO(i.getStart()))
                      .arg(TJ::time2ISO(i.getEnd()))
                      .arg(tjGranularity()));
            break;
        }
        default:
            logWarning(task, nullptr, xi18nc("@info/plain", "Unhandled time constraint type"));
            break;
    }
}

TJ::Task *PlanTJScheduler::addStartNotEarlier(Node *task)
{
    DateTime time = task->constraintStartTime();
    if (task->estimate()->type() == Estimate::Type_Duration && task->estimate()->calendar() != nullptr) {
        Calendar *cal = task->estimate()->calendar();
        if (cal != m_project->defaultCalendar() && cal != m_project->calendars().value(0)) {
            logWarning(task, nullptr, xi18nc("@info/plain", "Could not use the correct calendar for calculation of task duration"));
        } else {
            time = cal->firstAvailableAfter(time, m_project->constraintEndTime());
        }
    }
    TJ::Task *p = new TJ::Task(m_tjProject, QString("%1-sne").arg(m_tjProject->taskCount() + 1), task->name() + "-sne", nullptr, QString(), 0);
    p->setSpecifiedStart(0, toTJTime_t(time, tjGranularity()));
    p->setSpecifiedEnd(0, m_tjProject->getEnd() - 1);
    //debugPlan<<"PlanTJScheduler::addStartNotEarlier:"<<time<<TJ::time2ISO(toTJTime_t(time, tjGranularity()));
    return p;
}

TJ::Task *PlanTJScheduler::addFinishNotLater(Node *task)
{
    DateTime time = task->constraintEndTime();
    if (task->estimate()->type() == Estimate::Type_Duration && task->estimate()->calendar() != nullptr) {
        Calendar *cal = task->estimate()->calendar();
        if (cal != m_project->defaultCalendar() && cal != m_project->calendars().value(0)) {
            logWarning(task, nullptr, xi18nc("@info/plain", "Could not use the correct calendar for calculation of task duration"));
        } else {
            time = cal->firstAvailableBefore(time, m_project->constraintStartTime());
        }
    }
    TJ::Task *p = new TJ::Task(m_tjProject, QString("%1-fnl").arg(m_tjProject->taskCount() + 1), task->name() + "-fnl", nullptr, QString(), 0);
    p->setSpecifiedEnd(0, toTJTime_t(time, tjGranularity()) - 1);
    p->setSpecifiedStart(0, m_tjProject->getStart());
    return p;
}


void PlanTJScheduler::addRequests()
{
    //debugPlan;
    QMap<TJ::Task*, Node*> ::const_iterator it = m_taskmap.constBegin();
    for (; it != m_taskmap.constEnd(); ++it) {
        addRequest(it.key(), it.value());
    }
}

void PlanTJScheduler::addRequest(TJ::Task *job, Node *task_)
{
    //debugPlan;
    if (task_->type() == Node::Type_Milestone || task_->estimate() == nullptr || (m_recalculate && static_cast<Task*>(task_)->completion().isFinished())) {
        job->setMilestone(true);
        job->setDuration(0, 0.0);
        return;
    }
    auto task = static_cast<Task*>(task_);
    // Note: FI tasks can never have an estimate set (duration, length or effort)
    if (task->constraint() != Node::FixedInterval) {
        if (task->estimate()->type() == Estimate::Type_Duration && task->estimate()->calendar() == nullptr) {
            job->setDuration(0, task->estimate()->value(Estimate::Use_Expected, m_usePert).toDouble(Duration::Unit_d));
            return;
        }
        if (task->estimate()->type() == Estimate::Type_Duration && task->estimate()->calendar() != nullptr) {
            job->setLength(0, task->estimate()->value(Estimate::Use_Expected, m_usePert).toDouble(Duration::Unit_d) * 24.0 / m_tjProject->getDailyWorkingHours());
            if (static_cast<Task*>(task)->isStarted()) {
                job->setSpecifiedStart(0, toTJTime_t(task->completion().startTime(), tjGranularity()));
            }
            return;
        }
        if (m_recalculate) {
            auto tjTask = m_tjProject->getTask("TJ::RECALCULATE_FROM");
            if (tjTask) {
                job->addDepends(tjTask->getId());
                tjTask->addPrecedes(job->getId());
                logInfo(task, nullptr, i18n("Recalculate, earliest start: %1", QLocale().toString(m_recalculateFrom, QLocale::ShortFormat)));
            }
        }
        if (m_recalculate && task->completion().isStarted()) {
            auto id = QString("TJ::%1").arg(task->id());
            auto t = new TJ::Task(m_tjProject, id, id, nullptr, QString(), 0);
            t->setMilestone(true);
            t->setSpecifiedStart(0, toTJTime_t(task->completion().startTime(), tjGranularity()));
            job->addDepends(t->getId());
            t->addPrecedes(job->getId());

            const Estimate *estimate = task->estimate();
            const double e = estimate->scale(task->completion().remainingEffort(), Duration::Unit_d, estimate->scales());
            job->setEffort(0, e);
            logInfo(task, nullptr, i18n("Task has started. Remaining effort: %1d", e));
        } else {
            Estimate *estimate = task->estimate();
            double e = estimate->scale(estimate->value(Estimate::Use_Expected, m_usePert), Duration::Unit_d, estimate->scales());
            job->setEffort(0, e);
        }
    }
    if (task->requests().isEmpty()) {
        if (task->type() == Node::Type_Task) {
            warnPlan<<"Task:"<<task<<"has no allocations";
            logDebug(task, nullptr, "Task has no resource allocations");
        }
        return;
    }
    const auto lst = task->requests().resourceRequests(true /*resolveTeam*/);
    for (ResourceRequest *rr : lst) {
        if (!rr->resource()->calendar()) {
            result = 1; // stops scheduling
            logError(task, nullptr, i18n("No working hours defined for resource: %1", rr->resource()->name()));
            continue; // may happen if no calendar is set, and no default calendar
        }
        TJ::Resource *tjr = addResource(rr->resource());
        TJ::Allocation *a = new TJ::Allocation();
        a->setSelectionMode(TJ::Allocation::order);
        if (rr->units() != 100) {
            TJ::UsageLimits *l = new TJ::UsageLimits();
            l->setDailyUnits(rr->units());
            a->setLimits(l);
        }
        a->addCandidate(tjr);
        const auto alternativeRequests = rr->alternativeRequests();
        for (ResourceRequest *alt : alternativeRequests) {
            TJ::Resource *atjr = addResource(alt->resource());
            if (alt->units() != 100) {
                TJ::UsageLimits *l = new TJ::UsageLimits();
                l->setDailyUnits(alt->units());
                a->setLimits(l);
            }
            a->addCandidate(atjr);
        }
        job->addAllocation(a);
        logDebug(task, nullptr, "Added resource candidate: " + rr->resource()->name());
        const auto lst = rr->requiredResources();
        for (Resource *r : lst) {
            TJ::Resource *tr = addResource(r);
            a->addRequiredResource(tjr, tr);
            logDebug(task, nullptr, "Added required resource: " + r->name());
        }
    }
}

void PlanTJScheduler::schedule(SchedulingContext &context)
{
    if (context.projects.isEmpty()) {
        warnPlan<<"No projects";
        logError(context.project, nullptr, "No projects to schedule");
        return;
    }
    QElapsedTimer timer;
    timer.start();

    m_project = context.project;
    m_tjProject = new TJ::Project();
    connect(m_tjProject, &TJ::Project::updateProgressBar, this, [this](int value, int) {
        Q_EMIT progressChanged(value);
    });

    m_tjProject->setPriority(0);
    m_tjProject->setScheduleGranularity(m_granularity / 1000);
    m_tjProject->getScenario(0)->setMinSlackRate(0.0); // Do not calculate critical path
    if (context.calculateFrom.isValid()) {
        m_recalculate = true;
        m_recalculateFrom = context.calculateFrom;
        auto t = new TJ::Task(m_tjProject, "TJ::RECALCULATE_FROM", "TJ::RECALCULATE_FROM", nullptr, QString(), 0);
        t->setMilestone(true);
        t->setSpecifiedStart(0, toTJTime_t(m_recalculateFrom, tjGranularity()));
    }

    connect(&TJ::TJMH, &TJ::TjMessageHandler::message, this, &PlanTJScheduler::slotMessage);

    logInfo(m_project, nullptr, i18n("Scheduling started: %1", QDateTime::currentDateTime().toString(Qt::ISODate)));
    if (m_recalculate) {
        logInfo(m_project, nullptr, i18n("Recalculating from: %1", context.calculateFrom.toString(Qt::ISODate)));
    }

    if (context.scheduleInParallel) {
        calculateParallel(context);
    } else {
        calculateSequential(context);
    }
    if (context.cancelScheduling) {
        KPlato::Schedule::Log log(m_project, KPlato::Schedule::Log::Type_Warning, i18n("Scheduling canceled"));
        context.log << log;
    } else {
        logInfo(m_project, nullptr, i18n("Scheduling finished at %1, elapsed time: %2 seconds", QDateTime::currentDateTime().toString(Qt::ISODate), (double)timer.elapsed()/1000));
        context.log = takeLog();
        QMultiMap<int, KoDocument*>::const_iterator it = context.projects.constBegin();
        for (; it != context.projects.constEnd(); ++it) {
            it.value()->setModified(true);
        }
    }
    m_project = nullptr; // or else it is deleted
}

void PlanTJScheduler::calculateParallel(KPlato::SchedulingContext &context)
{
    QMultiMap<int, KoDocument*>::const_iterator it = context.projects.constBegin();
    for (; it != context.projects.constEnd(); ++it) {
        logInfo(m_project, nullptr, QString("Inserting project: %1, priority %2").arg(it.value()->projectName()).arg(it.key())); // TODO i18n
        insertProject(it.value(), it.key(), context);
    }
    logInfo(m_project, nullptr, i18n("Scheduling interval: %1 - %2, granularity: %3 minutes",
                                     QDateTime::fromSecsSinceEpoch(m_tjProject->getStart()).toString(Qt::ISODate),
                                     QDateTime::fromSecsSinceEpoch(m_tjProject->getEnd()).toString(Qt::ISODate),
                                     (double)m_tjProject->getScheduleGranularity()/60));
    if (m_tjProject->getStart() > m_tjProject->getEnd()) {
        logError(m_project, nullptr, i18n("Invalid project, start > end"));
        return;
    }
    addRequests();
    insertBookings(context);
    setConstraints();
    addDependencies();
    addStartEndJob();
    if (!check()) {
        logError(m_project, nullptr, i18n("Project check failed"));
    } else {
        if (solve() && !context.cancelScheduling) {
            populateProjects(context);
            const auto &docs = context.projects.values();
            for (auto *doc : docs) {
                auto p = doc->project();
                if (p->currentSchedule()->isScheduled()) {
                    logInfo(p, nullptr, i18n("Scheduled: %1 - %2", p->startTime().toString(Qt::ISODate), p->endTime().toString(Qt::ISODate)));
                } else {
                    logError(p, nullptr, i18n("Scheduling failed"));
                }
            }
        } else if (!context.cancelScheduling) {
            logError(m_project, nullptr, i18n("Project scheduling failed"));
        }
    }
}

void PlanTJScheduler::calculateSequential(KPlato::SchedulingContext &context)
{
    QMultiMapIterator<int, KoDocument*> it(context.projects);
    for (it.toBack(); it.hasPrevious();) {
        if (context.cancelScheduling) {
            break;
        }
        it.previous();
        auto project = it.value()->project();
        logInfo(project, nullptr, QString("Inserting project")); // TODO i18n
        insertProject(it.value(), it.key(), context);
        logInfo(project, nullptr, i18n("Scheduling interval: %1 - %2, granularity: %3 minutes",
                                     QDateTime::fromSecsSinceEpoch(m_tjProject->getStart()).toString(Qt::ISODate),
                                     QDateTime::fromSecsSinceEpoch(m_tjProject->getEnd()).toString(Qt::ISODate),
                                     (double)m_tjProject->getScheduleGranularity()/60.));

        if (m_tjProject->getStart() > m_tjProject->getEnd()) {
            logError(project, nullptr, i18n("Invalid project, start > end"));
            return;
        }
        addRequests();
        insertBookings(context);
        setConstraints();
        addDependencies();
        addStartEndJob();
        if (!check()) {
            logError(project, nullptr, i18n("Project check failed"));
        } else {
            if (solve()) {
                if (context.cancelScheduling) {
                    logWarning(context.project, nullptr, i18n("Scheduling canceled"));
                } else {
                    populateProjects(context);
                    context.resourceBookings << it.value();
                }
            } else {
                logError(project, nullptr, i18n("Project scheduling failed"));
            }
        }
        if (!context.cancelScheduling && it.hasPrevious()) {
            delete m_tjProject;
            m_taskmap.clear();
            m_resourcemap.clear();
            m_resourceIds.clear();
            m_durationTasks.clear();

            //m_granularity = std::max(context.granularity, 5*60*1000 /*5 minutes*/);
            m_tjProject = new TJ::Project();
            connect(m_tjProject, &TJ::Project::updateProgressBar, this, [this](int value, int) {
                Q_EMIT progressChanged(value);
            });
            m_tjProject->setPriority(0);
            m_tjProject->setScheduleGranularity(m_granularity / 1000);
            m_tjProject->getScenario(0)->setMinSlackRate(0.0); // Do not calculate critical path
            if (context.calculateFrom.isValid()) {
                m_recalculate = true;
                m_recalculateFrom = context.calculateFrom;
                auto t = new TJ::Task(m_tjProject, "TJ::RECALCULATE_FROM", "TJ::RECALCULATE_FROM", nullptr, QString(), 0);
                t->setMilestone(true);
                t->setSpecifiedStart(0, toTJTime_t(m_recalculateFrom, tjGranularity()));
            }
        }
    }
}

void PlanTJScheduler::insertBookings(KPlato::SchedulingContext &context)
{
    // Collect appointments from all resource in all projects and
    // map them to tj resources
    QHash<TJ::Resource*, KPlato::Appointment> apps;
    for (const auto doc : std::as_const(context.resourceBookings)) {
        const auto project = doc->project();
        const auto manager = project->findScheduleManagerByName(doc->property(SCHEDULEMANAGERNAME).toString());
        long sid = ANYSCHEDULED;
        if (manager) {
            sid = manager->scheduleId();
        }
        const auto resourceList = project->resourceList();
        for (Resource *r : resourceList) {
            TJ::Resource *tjResource = m_resourcemap.key(m_resourceIds.value(r->id()));
            if (!tjResource) {
                continue;
            }
            apps[tjResource] += r->appointmentIntervals(sid);
        }
    }
    // TODO: Handle load < 100%
    QHash<TJ::Resource*, KPlato::Appointment>::const_iterator ait;
    for (ait = apps.constBegin(); ait != apps.constEnd(); ++ait) {
        KPlato::Resource *r = m_resourcemap.value(ait.key());
        Q_ASSERT(r);
        const QMultiMap<QDate, AppointmentInterval> map = ait.value().intervals().map();
        QMultiMap<QDate, AppointmentInterval>::const_iterator it;
        for (it = map.constBegin(); it != map.constEnd(); ++it) {
            TJ::Interval interval = toTJInterval(it.value().startTime(), it.value().endTime(), m_granularity / 1000);
            ait.key()->bookInterval(0, interval, 3 /*undefined*/);
            if (it.value().load() < r->units()) {
                logWarning(m_project, r, i18n("Appointment with load (%1) less than available resource units (%2) not supported").arg(it.value().load(), r->units()));
            }
        }
    }
}

void PlanTJScheduler::insertProject(KoDocument *doc, int priority, KPlato::SchedulingContext &context)
{
    auto project = doc->project();
    auto sm = getScheduleManager(project);
    Q_ASSERT(sm);
    doc->setProperty(SCHEDULEMANAGERNAME, sm->name());
    if (m_recalculate) {
        sm->setRecalculate(true);
        sm->setRecalculateFrom(context.calculateFrom);
    }
    time_t time = project->constraintStartTime().toSecsSinceEpoch();
    if (m_tjProject->getStart() == 0 || m_tjProject->getStart() > time) {
        m_tjProject->setStart(time);
    }
    time = project->constraintEndTime().toSecsSinceEpoch();
    if (m_tjProject->getEnd() < time) {
        m_tjProject->setEnd(time);
    }
    m_project->setConstraintStartTime(QDateTime::fromSecsSinceEpoch(m_tjProject->getStart()));
    m_project->setConstraintEndTime(QDateTime::fromSecsSinceEpoch(m_tjProject->getEnd()));
    m_tjProject->setDailyWorkingHours(const_cast<KPlato::Project*>(project)->standardWorktime()->day());

    // Set working days for the project, it is used for tasks with a length specification
    // FIXME: Plan has task specific calendars for this estimate type
    KPlato::Calendar *cal = project->defaultCalendar();
    if (! cal) {
        cal = project->calendars().value(0);
    }
    if (cal) {
        // TJ: Sun = 0, Mon = 1 ... Sat = 6, ours: Mon = 1 ... Sun = 7 (as qt)
        int days[ 7 ] = { Qt::Sunday, Qt::Monday, Qt::Tuesday, Qt::Wednesday, Qt::Thursday, Qt::Friday, Qt::Saturday };
        for (int i = 0; i < 7; ++i) {
            CalendarDay *d = nullptr;
            for (Calendar *c = cal; c; c = c->parentCal()) {
                QElapsedTimer t; t.start();
                d = c->weekday(days[ i ]);
                Q_ASSERT(d);
                if (d == nullptr || d->state() != CalendarDay::Undefined) {
                    break;
                }
            }
            if (d && d->state() == CalendarDay::Working) {
                QList<TJ::Interval*> lst;
                const auto intervals = d->timeIntervals();
                for (const TimeInterval *ti : intervals) {
                    TJ::Interval *tji = new TJ::Interval(toTJInterval(ti->startTime(), ti->endTime(),tjGranularity()));
                    lst << tji;
                }
                m_tjProject->setWorkingHours(i, lst);
                qDeleteAll(lst);
            }
        }
    }
    TJ::Task *t = addTask(project, nullptr);
    addTasks(project, t, priority);
}

void PlanTJScheduler::addTasks(const KPlato::Node *parent, TJ::Task *tjParent, int projectPriority)
{
    //debugPlan;
    for (const KPlato::Node *n : parent->childNodeIterator()) {
        switch (n->type()) {
            case Node::Type_Project:
            case Node::Type_Summarytask: {
                TJ::Task *t = addTask(n, tjParent);
                addTasks(n, t);
                break;
            }
            case Node::Type_Task:
            case Node::Type_Milestone: {
                switch (n->constraint()) {
                    case Node::StartNotEarlier:
                        // TODO parent = addStartNotEarlier(n);
                        break;
                    case Node::FinishNotLater:
                        // TODO parent = addFinishNotLater(n);
                        break;
                    default:
                        break;
                }
                TJ::Task *t = addTask(n, tjParent);
                t->setPriority(t->getPriority() + projectPriority);
                break;
            }
            default:
                break;
        }
    }
}

void PlanTJScheduler::cancelScheduling(SchedulingContext &context)
{
    Q_UNUSED(context);
    context.cancelScheduling = true;
    if (m_tjProject) {
        m_tjProject->cancelScheduling();
    }
}

void PlanTJScheduler::populateProjects(KPlato::SchedulingContext &context)
{
    Q_UNUSED(context)

    QMap<TJ::Task*, Node*>::const_iterator it;
    for (it = m_taskmap.constBegin(); it != m_taskmap.constEnd(); ++it) {
        KPlato::Node *node = it.value();
        KPlato::Project *project = qobject_cast<KPlato::Project*>(node->projectNode());
        Q_ASSERT(project);
        if (!project) {
            continue;
        }
        if (!project->currentScheduleManager()->expected()) {
            project->currentScheduleManager()->createSchedules();
        }
        auto mainschedule = project->currentScheduleManager()->expected();
        project->setCurrentSchedule(mainschedule->id());
        switch (node->type()) {
            case KPlato::Node::Type_Task:
            case KPlato::Node::Type_Milestone:
                node->createSchedule(mainschedule);
                node->setCurrentSchedule(mainschedule->id());
                break;
            default:
                break;
        }
        switch (node->type()) {
            case KPlato::Node::Type_Project:
                break;
            case KPlato::Node::Type_Summarytask:
                break;
            case KPlato::Node::Type_Task:
            case KPlato::Node::Type_Milestone:
                taskFromTJ(project, m_tjProject->getTask(node->id()), node);
                break;
            default: break;
        }
        mainschedule->setScheduled(true);
    }
}
