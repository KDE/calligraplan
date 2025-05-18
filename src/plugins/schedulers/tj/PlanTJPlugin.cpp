/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2009, 2011 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "PlanTJPlugin.h"

#include "PlanTJScheduler.h"

#include "kptproject.h"
#include "kptschedule.h"

#include "kptdebug.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <QApplication>

#ifndef PLAN_NOPLUGIN
K_PLUGIN_CLASS_WITH_JSON(PlanTJPlugin, "plantjscheduler.json")
#endif

using namespace KPlato;

PlanTJPlugin::PlanTJPlugin(QObject * parent, const QVariantList &)
    : KPlato::SchedulerPlugin(parent)
{
    m_granularities << (long unsigned int) 5 * 60 * 1000
                    << (long unsigned int) 15 * 60 * 1000
                    << (long unsigned int) 30 * 60 * 1000
                    << (long unsigned int) 60 * 60 * 1000;
}

PlanTJPlugin::~PlanTJPlugin()
{
}

QString PlanTJPlugin::description() const
{
    return xi18nc("@info:whatsthis", "<title>TaskJuggler Scheduler</title>"
                    "<para>This is a slightly modified version of the scheduler used in TaskJuggler."
                    " It has been enhanced to handle resource units.</para>"
                    "<para>Scheduling backwards is simulated by scheduling all tasks as late as possible.</para>"
                    "<para><note>Plan does not utilize all of its functionality.</note></para>"
                );
}

int PlanTJPlugin::capabilities() const
{
    return SchedulerPlugin::AvoidOverbooking | SchedulerPlugin::ScheduleForward | SchedulerPlugin::ScheduleBackward | SchedulerPlugin::ScheduleInParallel;
}

ulong PlanTJPlugin::currentGranularity() const
{
    ulong v = m_granularities.value(m_granularityIndex);
    return qMax(v, (ulong)300000); // minimum 5 min
}

void PlanTJPlugin::calculate(KPlato::Project &project, KPlato::ScheduleManager *sm, bool nothread)
{
    for (SchedulerThread *j : std::as_const(m_jobs)) {
        if (j->manager() == sm) {
            return;
        }
    }
    sm->setScheduling(true);

    PlanTJScheduler *job = new PlanTJScheduler(&project, sm, currentGranularity());
    m_jobs << job;
    connect(job, &KPlato::SchedulerThread::jobFinished, this, &PlanTJPlugin::slotFinished);

    project.changed(sm);

    connect(this, SIGNAL(sigCalculationStarted(KPlato::Project*,KPlato::ScheduleManager*)), &project, SIGNAL(sigCalculationStarted(KPlato::Project*,KPlato::ScheduleManager*)));
    connect(this, SIGNAL(sigCalculationFinished(KPlato::Project*,KPlato::ScheduleManager*)), &project, SIGNAL(sigCalculationFinished(KPlato::Project*,KPlato::ScheduleManager*)));

    connect(job, &KPlato::SchedulerThread::maxProgressChanged, sm, &KPlato::ScheduleManager::setMaxProgress);
    connect(job, &KPlato::SchedulerThread::progressChanged, sm, &KPlato::ScheduleManager::setProgress);

    if (nothread) {
        job->doRun();
    } else {
        job->start();
    }
}

void PlanTJPlugin::stopAllCalculations()
{
    for (SchedulerThread *s : std::as_const(m_jobs)) {
        stopCalculation(s);
    }
}

void PlanTJPlugin::stopCalculation(SchedulerThread *sch)
{
    if (sch) {
         //FIXME: this should just call stopScheduling() and let the job finish "normally"
        disconnect(sch, &KPlato::SchedulerThread::jobFinished, this, &PlanTJPlugin::slotFinished);
        sch->stopScheduling();
        // wait max 20 seconds.
        sch->mainManager()->setCalculationResult(ScheduleManager::CalculationStopped);
        if (! sch->wait(20000)) {
            sch->deleteLater();
            m_jobs.removeAt(m_jobs.indexOf(sch));
        } else {
            slotFinished(sch);
        }
    }
}

void PlanTJPlugin::slotStarted(SchedulerThread *job)
{
//    debugPlan<<"PlanTJPlugin::slotStarted:";
    Q_EMIT sigCalculationStarted(job->mainProject(), job->mainManager());
}

void PlanTJPlugin::slotFinished(SchedulerThread *j)
{
    PlanTJScheduler *job = static_cast<PlanTJScheduler*>(j);
    Project *mp = job->mainProject();
    ScheduleManager *sm = job->mainManager();
    //debugPlan<<"PlanTJPlugin::slotFinished:"<<mp<<sm<<job->isStopped();
    if (job->isStopped()) {
        sm->setCalculationResult(ScheduleManager::CalculationCanceled);
    } else {
        updateLog(job);
        if (job->result > 0) {
            sm->setCalculationResult(ScheduleManager::CalculationError);
        } else {
            Project *tp = job->project();
            ScheduleManager *tm = job->manager();
            updateProject(tp, tm, mp, sm);
            sm->setCalculationResult(ScheduleManager::CalculationDone);
        }
    }
    sm->setScheduling(false);

    m_jobs.removeAt(m_jobs.indexOf(job));
    if (m_jobs.isEmpty()) {
        m_synctimer.stop();
    }
    Q_EMIT sigCalculationFinished(mp, sm);

    disconnect(this, &PlanTJPlugin::sigCalculationStarted, mp, &KPlato::Project::sigCalculationStarted);
    disconnect(this, &PlanTJPlugin::sigCalculationFinished, mp, &KPlato::Project::sigCalculationFinished);

    job->deleteLater();
}

void PlanTJPlugin::schedule(SchedulingContext &context)
{
    PlanTJScheduler *job = new PlanTJScheduler(currentGranularity());
    m_jobs << job;
    connect(job, &PlanTJScheduler::progressChanged, this, &PlanTJPlugin::progressChanged);
    context.scheduleInParallel = scheduleInParallel();
    job->schedule(context);
    m_jobs.clear();
    delete job;
}

#include "PlanTJPlugin.moc"
