/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2009 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptbuiltinschedulerplugin.h"
#include "kptmaindocument.h"
#include "kptpart.h"

#include "kptproject.h"
#include "kptschedule.h"
#include "kptxmlloaderobject.h"
#include <SchedulingContext.h>
#include <kptcommand.h>

#include <KoDocument.h>
#include <ExtraProperties.h>

#include <KLocalizedString>

namespace KPlato
{

BuiltinSchedulerPlugin::BuiltinSchedulerPlugin(QObject *parent)
    : SchedulerPlugin(parent)
{
    setName(i18nc("Network = task dependency network", "Network Scheduler"));
    setComment(xi18nc("@info:tooltip", "Built-in network (PERT) based scheduler"));

    m_granularityIndex = 1;
    m_granularities << 0
                    << (long unsigned int) 1 * 60 * 1000
                    << (long unsigned int) 15 * 60 * 1000
                    << (long unsigned int) 30 * 60 * 1000
                    << (long unsigned int) 60 * 60 * 1000;
}
 
BuiltinSchedulerPlugin::~BuiltinSchedulerPlugin()
{
}

QString BuiltinSchedulerPlugin::description() const
{
    return xi18nc("@info:whatsthis", "<title>Network (PERT) Scheduler</title>"
                    "<para>The network scheduler generally schedules tasks according to their dependencies."
                    " When a task is scheduled it is scheduled in full, booking the allocated resources if available."
                    " If overbooking is not allowed, subsequent tasks that requests the same resource"
                    " will be scheduled later in time.</para>"
                    "<para>Tasks with time constraints will be scheduled first to minimize the problem"
                    " with resource conflicts</para>"
                    "<para><note>This scheduler does not handle resource conflicts well."
                    "<nl/>You can try a different scheduler if available."
                    " You may also change resource allocations or add dummy dependencies to avoid the conflicts.</note></para>"
                );
}

void BuiltinSchedulerPlugin::calculate(Project &project, ScheduleManager *sm, bool nothread)
{
    KPlatoScheduler *job = new KPlatoScheduler(&project, sm);
    m_jobs << job;
    connect(job, &SchedulerThread::jobStarted, this, &BuiltinSchedulerPlugin::slotStarted);
    connect(job, &SchedulerThread::jobFinished, this, &BuiltinSchedulerPlugin::slotFinished);

    connect(this, &BuiltinSchedulerPlugin::sigCalculationStarted, &project, &Project::sigCalculationStarted);
    connect(this, &BuiltinSchedulerPlugin::sigCalculationFinished, &project, &Project::sigCalculationFinished);

    sm->setScheduling(true);
    if (nothread) {
        connect(job, &SchedulerThread::maxProgressChanged, sm, &ScheduleManager::setMaxProgress);
        connect(job, &SchedulerThread::progressChanged, sm, &ScheduleManager::setProgress);
        job->doRun();
    } else {
        job->start();
    }
    m_synctimer.start();
}

void BuiltinSchedulerPlugin::slotStarted(SchedulerThread *job)
{
    qDebug()<<"BuiltinSchedulerPlugin::slotStarted:"<<job->mainProject()<<job->mainManager();
    
    Q_EMIT sigCalculationStarted(job->mainProject(), job->mainManager());
}

void BuiltinSchedulerPlugin::slotFinished(SchedulerThread *job)
{
    ScheduleManager *sm = job->mainManager();
    Project *mp = job->mainProject();
    qDebug()<<"BuiltinSchedulerPlugin::slotFinished:"<<mp<<sm<<job->isStopped();
    if (job->isStopped()) {
        sm->setCalculationResult(ScheduleManager::CalculationCanceled);
    } else {
        updateLog(job);
        Project *tp = static_cast<KPlatoScheduler*>(job)->project();
        ScheduleManager *tm = static_cast<KPlatoScheduler*>(job)->manager();
        updateProject(tp, tm, mp, sm);
        sm->setCalculationResult(ScheduleManager::CalculationDone);
    }
    sm->setScheduling(false);

    m_jobs.removeAt(m_jobs.indexOf(job));
    if (m_jobs.isEmpty()) {
        m_synctimer.stop();
    }
    Q_EMIT sigCalculationFinished(mp, sm);

    disconnect(this, &BuiltinSchedulerPlugin::sigCalculationStarted, mp, &Project::sigCalculationStarted);
    disconnect(this, &BuiltinSchedulerPlugin::sigCalculationFinished, mp, &Project::sigCalculationFinished);

    job->deleteLater();
    qDebug()<<"BuiltinSchedulerPlugin::slotFinished: <<<";
}

void BuiltinSchedulerPlugin::schedule(SchedulingContext &context)
{
    KPlatoScheduler *job = new KPlatoScheduler();
    m_jobs << job;
    context.scheduleInParallel = scheduleInParallel();
    connect(job, &KPlato::KPlatoScheduler::progressChanged, this, &KPlato::SchedulerPlugin::progressChanged);
    job->schedule(context);
    m_jobs.clear();
    delete job;
}

//--------------------
KPlatoScheduler::KPlatoScheduler(ulong granularityIndex, QObject *parent)
    : SchedulerThread(granularityIndex, parent)
{
    qDebug()<<Q_FUNC_INFO;
}

KPlatoScheduler::KPlatoScheduler(Project *project, ScheduleManager *sm, ulong granularityIndex, QObject *parent)
    : SchedulerThread(project, sm, granularityIndex, parent)
{
    qDebug()<<"KPlatoScheduler::KPlatoScheduler:"<<m_mainmanager<<m_mainmanager->name()<<m_mainmanagerId;
}

KPlatoScheduler::~KPlatoScheduler()
{
    qDebug()<<"KPlatoScheduler::~KPlatoScheduler:"<<QThread::currentThreadId();
}

void KPlatoScheduler::stopScheduling()
{
    m_stopScheduling = true;
    if (m_project) {
        m_project->stopcalculation = true;
    }
}

void KPlatoScheduler::cancelScheduling(SchedulingContext &context)
{
    context.cancelScheduling = true;
    for (const auto doc : std::as_const(context.calculatedDocuments)) {
        doc->project()->stopcalculation = true;
        if (doc->project()->currentScheduleManager()) {
            doc->project()->currentScheduleManager()->setCalculationResult(KPlato::ScheduleManager::CalculationCanceled);
        }
    }
}

void KPlatoScheduler::run()
{
    if (m_haltScheduling) {
        deleteLater();
        return;
    }
    if (m_stopScheduling) {
        return;
    }
    { // mutex -->
        m_projectMutex.lock();
        m_managerMutex.lock();

        m_project = new Project();
        m_project->setSchedulerPlugins(mainProject()->schedulerPlugins());
        loadProject(m_project, m_pdoc);
        m_project->setName(QStringLiteral("Schedule: ") + m_project->name()); //Debug

        m_manager = m_project->scheduleManager(m_mainmanagerId);
        m_manager->setSchedulerPluginId(mainManager()->schedulerPluginId());
        Q_ASSERT(m_manager);
        Q_ASSERT(m_manager->expected());
        Q_ASSERT(m_manager != m_mainmanager);
        Q_ASSERT(m_manager->scheduleId() == m_mainmanager->scheduleId());
        Q_ASSERT(m_manager->expected() != m_mainmanager->expected());
        m_manager->setName(QStringLiteral("Schedule: ") + m_manager->name()); //Debug

        m_managerMutex.unlock();
        m_projectMutex.unlock();
    } // <--- mutex

    connect(m_project, SIGNAL(maxProgress(int)), this, SLOT(setMaxProgress(int)));
    connect(m_project, SIGNAL(sigProgress(int)), this, SLOT(setProgress(int)));

    bool x = connect(m_manager, SIGNAL(sigLogAdded(KPlato::Schedule::Log)), this, SLOT(slotAddLog(KPlato::Schedule::Log)));
    Q_ASSERT(x); Q_UNUSED(x);
    m_project->calculate(*m_manager);
    if (m_haltScheduling) {
        deleteLater();
    }
}

void KPlatoScheduler::slotProgress(int value)
{
    Q_UNUSED(value)
    ++m_progress;
    Q_EMIT progressChanged(m_progress * 100 / m_maxprogress);
}

KoDocument *KPlatoScheduler::copyDocument(KoDocument *doc)
{
    auto part = new Part(nullptr);
    auto copy = new MainDocument(part);
    auto domDoc = doc->saveXML();
    auto xml = KoXmlDocument();
    xml.setContent(domDoc.toString());
    copy->loadXML(xml, nullptr);
    copy->setProperty(SCHEDULEMANAGERNAME, doc->property(SCHEDULEMANAGERNAME));
    copy->project()->setProperty(SCHEDULEMANAGERNAME, doc->project()->property(SCHEDULEMANAGERNAME));
    return copy;
}

void KPlatoScheduler::schedule(SchedulingContext &context)
{
    if (context.projects.isEmpty()) {
        warnPlan<<"WARN:"<<Q_FUNC_INFO<<"No projects";
        logError(context.project, nullptr, QStringLiteral("No projects to schedule"));
        return;
    }
    QHash<KoDocument*, KoDocument*> projectMap;
    QMultiMapIterator<int, KoDocument*> it(context.projects);
    for (it.toBack(); it.hasPrevious();) {
        context.calculatedDocuments << copyDocument(it.previous().value());
        projectMap.insert(context.calculatedDocuments.last(), it.value());
    }

    int taskCount = 0;
    for (const auto doc : std::as_const(context.calculatedDocuments)) {
        auto p = doc->project();
        connect(p, &KPlato::Project::sigProgress, this, &KPlato::KPlatoScheduler::slotProgress);
        taskCount += p->leafNodes().count();
    }
    setMaxProgress(taskCount * 3);

    QElapsedTimer timer;
    timer.start();

    logInfo(context.project, nullptr, i18n("Scheduling started: %1", QDateTime::currentDateTime().toString(Qt::ISODate)));
    if (context.calculateFrom.isValid()) {
        logInfo(context.project, nullptr, i18n("Recalculating from: %1", context.calculateFrom.toString(Qt::ISODate)));
    }

    auto includes = context.resourceBookings;
    for (auto doc : std::as_const(context.calculatedDocuments)) {
        calculateProject(context, doc, includes);
        if (!context.cancelScheduling) {
            includes << doc;
        }
    }
    if (context.cancelScheduling) {
        takeLog();
        logWarning(context.project, nullptr, i18n("Scheduling canceled"));
    } else {
        for (auto doc : std::as_const(context.calculatedDocuments)) {
            mergeProject(doc->project(), projectMap.value(doc)->project());
            projectMap.value(doc)->setProperty(SCHEDULEMANAGERNAME, doc->property(SCHEDULEMANAGERNAME));
        }
        logInfo(context.project, nullptr, i18n("Scheduling finished at %1, elapsed time: %2 seconds", QDateTime::currentDateTime().toString(Qt::ISODate), (double)timer.elapsed()/1000));
    }
    context.log = takeLog();
}

void KPlatoScheduler::calculateProject(SchedulingContext &context, KoDocument *doc, QList<const KoDocument*> includes)
{
    if (context.cancelScheduling) {
        return;
    }
    for (auto d : includes) {
        KPlato::Project *project = d->project();
        project->setProperty(SCHEDULEMANAGERNAME, d->property(SCHEDULEMANAGERNAME));
        QMetaObject::invokeMethod(doc, "insertSharedResourceAssignments", Q_ARG(const KPlato::Project*, project));
        logInfo(doc->project(), nullptr, i18n("Inserting resource bookings from project: %1", project->name()));
    }
    KPlato::Project *project = doc->project();
    KPlato::ScheduleManager *sm = getScheduleManager(project);
    Q_ASSERT(sm);
    if (!sm) {
        return;
    }
    project->setCurrentScheduleManager(sm);
    doc->setProperty(SCHEDULEMANAGERNAME, sm->name());
    connect(sm, &KPlato::ScheduleManager::sigLogAdded, this, &KPlatoScheduler::slotAddLog);
    KPlato::DateTime oldstart = project->constraintStartTime();
    KPlato::DateTime start = context.calculateFrom;
    if (oldstart > start) {
        start = oldstart;
    }
    sm->setRecalculateFrom(start);
    project->calculate(*sm);
    project->setConstraintStartTime(oldstart);
    disconnect(sm, &KPlato::ScheduleManager::sigLogAdded, this, &KPlatoScheduler::slotAddLog);
    project->currentSchedule()->clearLogs();
    doc->setModified(true);
}

/*static*/
void KPlatoScheduler::mergeProject(Project *calculatedProject, Project *originalProject)
{
    Q_ASSERT(originalProject);
    Q_ASSERT(calculatedProject->name() == originalProject->name());

    auto calculatedManager = calculatedProject->currentScheduleManager();
    auto newManager = originalProject->findScheduleManagerByName(calculatedManager->name());
    if (newManager) {
        auto sid = newManager->scheduleId();
        if (sid < 0) {
            // this manager was not scheduled, so need to create the main schedule
            auto sch = new MainSchedule(originalProject, calculatedManager->name(), calculatedManager->expected()->type(), calculatedManager->expected()->id());
            originalProject->addSchedule(sch);
            newManager->setExpected(sch);
        } else {
            // re-calculating existing schedule, need to remove old schedule form tasks and resources first
            const auto tasks = originalProject->allNodes();
            for (auto t : tasks) {
                if (t->type() == KPlato::Node::Type_Project) {
                    continue;
                }
                auto s = t->findSchedule(sid);
                if (s) {
                    t->takeSchedule(s);
                    delete s;
                }
            }
            const auto resources = originalProject->resourceList();
            for (auto r : resources) {
                auto s = r->findSchedule(sid);
                if (s) {
                    r->takeSchedule(s);
                    delete s;
                }
            }
        }
    } else {
        // updateProject() needs manager and main schedule to exist
        newManager = new ScheduleManager(*originalProject);
        newManager->setName(calculatedManager->name());
        auto parentManager = calculatedManager->parentManager();
        if (parentManager) {
            parentManager = originalProject->findScheduleManagerByName(parentManager->name());
        }
        originalProject->addScheduleManager(newManager, parentManager);
        auto sch = new MainSchedule(originalProject, calculatedManager->name(), calculatedManager->expected()->type(), calculatedManager->expected()->id());
        originalProject->addSchedule(sch);
        newManager->setExpected(sch);
    }
    updateProject(calculatedProject, calculatedManager, originalProject, newManager);
    const auto sid = calculatedManager->scheduleId();
    const auto tasks = calculatedProject->allTasks();
    for (auto t : tasks) {
        if (t->state(sid) & Node::State_Error) {
            newManager->setCalculationResult(KPlato::ScheduleManager::CalculationError);
        }
    }
}

} //namespace KPlato

