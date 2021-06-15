/* This file is part of the KDE project
  Copyright (C) 2009, 2010, 2012 Dag Andersen <dag.andersen@kdemail.net>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

// clazy:excludeall=qstring-arg
#include "kptschedulerplugin.h"

#include "kptproject.h"
#include "kptschedule.h"
#include "kptxmlloaderobject.h"
#include "XmlSaveContext.h"
#include "kptdebug.h"

#include "KoXmlReader.h"
#include <KoDocument.h>

#include <KLocalizedString>

namespace KPlato
{

class Q_DECL_HIDDEN SchedulerPlugin::Private
{
public:
    Private() : scheduleInParallel(false) {}

    QString name;
    QString comment;
    bool scheduleInParallel;
};

SchedulerPlugin::SchedulerPlugin(QObject *parent)
    : QObject(parent),
      d(new SchedulerPlugin::Private()),
      m_granularity(0)
{
    // register Schedule::Log so it can be used in queued connections
    qRegisterMetaType<Schedule::Log>("Schedule::Log");

    m_synctimer.setInterval(500);
    connect(&m_synctimer, &QTimer::timeout, this, &SchedulerPlugin::slotSyncData);
}

SchedulerPlugin::~SchedulerPlugin()
{
    for (SchedulerThread *s : qAsConst(m_jobs)) {
        s->mainManager()->setScheduling(false);
        s->haltScheduling();
        s->wait();
    }
    delete d;
}

void SchedulerPlugin::setName(const QString &name)
{
    d->name = name;
}

QString SchedulerPlugin::name() const
{
    return d->name;
}

void SchedulerPlugin::setComment(const QString &comment)
{
    d->comment = comment;
}

QString SchedulerPlugin::comment() const
{
    return d->comment;
}

int SchedulerPlugin::capabilities() const
{
    return AvoidOverbooking | AllowOverbooking | ScheduleForward | ScheduleBackward;
}

void SchedulerPlugin::stopCalculation(ScheduleManager *sm)
{
    for (SchedulerThread *j : qAsConst(m_jobs)) {
        if (sm == j->mainManager()) {
            j->stopScheduling();
        }
    }
}

void SchedulerPlugin::haltCalculation(ScheduleManager *sm)
{
    debugPlan<<sm;
    for (SchedulerThread *j : qAsConst(m_jobs)) {
        if (sm == j->mainManager()) {
            haltCalculation(j);
            break;
        }
    }
    sm->setCalculationResult(ScheduleManager::CalculationCanceled);
    sm->setScheduling(false);
}

void SchedulerPlugin::stopCalculation(SchedulerThread *job)
{
    job->stopScheduling();
}

void SchedulerPlugin::haltCalculation(SchedulerThread *job)
{
    debugPlan<<job<<m_jobs.contains(job);
    disconnect(this, nullptr, job, nullptr);
    job->haltScheduling();
    if (m_jobs.contains(job)) {
        debugPlan<<"SchedulerPlugin::haltCalculation: remove"<<job;
        m_jobs.removeAt(m_jobs.indexOf(job));
    }
}

QList<long unsigned int> SchedulerPlugin::granularities() const
{
    return m_granularities;
}

int SchedulerPlugin::granularity() const
{
    return m_granularity;
}

void SchedulerPlugin::setGranularity(int index)
{
    m_granularity = qMin(index, m_granularities.count() - 1);
    qInfo()<<Q_FUNC_INFO<<m_granularities.count()<<':'<<index<<':'<<m_granularity;
}

void SchedulerPlugin::slotSyncData()
{
    updateProgress();
    updateLog();
}

void SchedulerPlugin::setScheduleInParallel(bool value)
{
    d->scheduleInParallel = value;
}

bool SchedulerPlugin::scheduleInParallel() const
{
    return (capabilities() & ScheduleInParallel) && d->scheduleInParallel;
}

void SchedulerPlugin::updateProgress()
{
    for (SchedulerThread *j : qAsConst(m_jobs)) {
        ScheduleManager *sm = j->mainManager();
        if (sm->maxProgress() != j->maxProgress()) {
            sm->setMaxProgress(j->maxProgress());
        }
        sm->setProgress(j->progress());
    }
}

void SchedulerPlugin::updateLog()
{
    for (SchedulerThread *j : qAsConst(m_jobs)) {
        updateLog(j);
    }
}

void SchedulerPlugin::updateLog(SchedulerThread *j)
{
    ScheduleManager *sm = j->mainManager();
    Project *p = j->mainProject();
    Q_ASSERT(p == &(sm->project()));
#ifdef NDEBUG
    Q_UNUSED(p)
#endif

    if (j->manager()) {
        sm->setPhaseNames(j->phaseNames());
    }

    QVector<Schedule::Log> logs = j->takeLog();
    QMutableVectorIterator<Schedule::Log> i(logs);
    while (i.hasNext()) {
        Schedule::Log &l = i.next();
        // map log from temporary project to real project
        if (l.resource) {
            l.resource = sm->project().findResource(l.resource->id());
            // resource may have been deleted while scheduling is running
            if (!l.resource) {
                i.remove();
                continue;
            }
            Q_ASSERT(l.resource->project() == p);
        }
        if (l.node) {
            const Node *n = l.node;
            if (l.node->type() == Node::Type_Project) {
                l.node = &(sm->project());
            } else {
                l.node = sm->project().findNode(l.node->id());
                // task may have been deleted while scheduling is running
                if (!l.node) {
                    i.remove();
                    continue;
                }
            }
            Q_ASSERT(n != l.node);
#ifdef NDEBUG
            Q_UNUSED(n)
#endif
            Q_ASSERT(l.node->projectNode() == p);
        }
    }
    if (! logs.isEmpty()) {
        sm->slotAddLog(logs);
    }
}

void SchedulerPlugin::updateProject(const Project *tp, const ScheduleManager *tm, Project *mp, ScheduleManager *sm) const
{
    SchedulerThread::updateProject(tp, tm, mp, sm);
}

void SchedulerPlugin::schedule(SchedulingContext &context)
{
    Q_UNUSED(context)
}

//----------------------
SchedulerThread::SchedulerThread(Project *project, ScheduleManager *manager, QObject *parent)
    : QThread(parent),
    m_mainproject(project),
    m_mainmanager(manager),
    m_mainmanagerId(manager->managerId()),
    m_project(nullptr),
    m_manager(nullptr),
    m_stopScheduling(false),
    m_haltScheduling(false),
    m_progress(0)
{
    manager->createSchedules(); // creates expected() to get log messages during calculation

    QDomDocument document("kplato");
    saveProject(project, document);

    m_pdoc.setContent(document.toString());


    connect(this, &QThread::started, this, &SchedulerThread::slotStarted);
    connect(this, &QThread::finished, this, &SchedulerThread::slotFinished);
}

SchedulerThread::SchedulerThread(QObject *parent)
    : QThread(parent)
    , m_mainproject(nullptr)
    , m_mainmanager(nullptr)
    , m_project(nullptr)
    , m_manager(nullptr)
    , m_stopScheduling(false)
    , m_haltScheduling(false)
    , m_progress(0)
{
}

SchedulerThread::~SchedulerThread()
{
    //debugPlan<<QThread::currentThreadId();
    delete m_project;
    m_project = nullptr;
    wait();
}

void SchedulerThread::setMaxProgress(int value)
{
    m_maxprogressMutex.lock();
    m_maxprogress = value;
    m_maxprogressMutex.unlock();
    Q_EMIT maxProgressChanged(value, m_mainmanager);
}

int SchedulerThread::maxProgress() const
{
    QMutexLocker m(&m_maxprogressMutex);
    return m_maxprogress;
}

void SchedulerThread::setProgress(int value)
{
    m_progressMutex.lock();
    m_progress = value;
    m_progressMutex.unlock();
    Q_EMIT progressChanged(value, m_mainmanager);
}

int SchedulerThread::progress() const
{
    QMutexLocker m(&m_progressMutex);
    return m_progress;
}

void SchedulerThread::slotAddLog(const KPlato::Schedule::Log &log)
{
//     debugPlan<<log;
    QMutexLocker m(&m_logMutex);
    m_logs << log;
}

QVector<Schedule::Log> SchedulerThread::takeLog()
{
    QMutexLocker m(&m_logMutex);
    const QVector<KPlato::Schedule::Log> l = m_logs;
    m_logs.clear();
    return l;
}

QMap<int, QString> SchedulerThread::phaseNames() const
{
    QMutexLocker m(&m_managerMutex);
    return m_manager->phaseNames();
}


void SchedulerThread::slotStarted()
{
    Q_EMIT jobStarted(this);
}

void SchedulerThread::slotFinished()
{
    if (m_haltScheduling) {
        deleteLater();
    } else {
        Q_EMIT jobFinished(this);
    }
}

void SchedulerThread::doRun()
{
    slotStarted();
    run();
    slotFinished();
}

ScheduleManager *SchedulerThread::manager() const
{
    QMutexLocker m(&m_managerMutex);
    return m_manager;
}

Project *SchedulerThread::project() const
{
    QMutexLocker m(&m_projectMutex);
    return m_project;
}

void SchedulerThread::stopScheduling()
{
    debugPlan;
    m_stopScheduling = true;
}

void SchedulerThread::haltScheduling()
{
    debugPlan;
    m_haltScheduling = true;
}

void SchedulerThread::logError(Node *n, Resource *r, const QString &msg, int phase)
{
    Schedule::Log log;
    if (r == nullptr) {
        log = Schedule::Log(n, Schedule::Log::Type_Error, msg, phase);
    } else {
        log = Schedule::Log(n, r, Schedule::Log::Type_Error, msg, phase);
    }
    slotAddLog(log);
}

void SchedulerThread::logWarning(Node *n, Resource *r, const QString &msg, int phase)
{
    Schedule::Log log;
    if (r == nullptr) {
        log = Schedule::Log(n, Schedule::Log::Type_Warning, msg, phase);
    } else {
        log = Schedule::Log(n, r, Schedule::Log::Type_Warning, msg, phase);
    }
    slotAddLog(log);
}

void SchedulerThread::logInfo(Node *n, Resource *r, const QString &msg, int phase)
{
    Schedule::Log log;
    if (r == nullptr) {
        log = Schedule::Log(n, Schedule::Log::Type_Info, msg, phase);
    } else {
        log = Schedule::Log(n, r, Schedule::Log::Type_Info, msg, phase);
    }
    slotAddLog(log);
}

void SchedulerThread::logDebug(Node *n, Resource *r, const QString &msg, int phase)
{
    Schedule::Log log;
    if (r == nullptr) {
        log = Schedule::Log(n, Schedule::Log::Type_Debug, msg, phase);
    } else {
        log = Schedule::Log(n, r, Schedule::Log::Type_Debug, msg, phase);
    }
    slotAddLog(log);
}

//static
void SchedulerThread::saveProject(Project *project, QDomDocument &document)
{
    document.appendChild(document.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\""));

    QDomElement doc = document.createElement("kplato");
    doc.setAttribute("editor", "Plan");
    doc.setAttribute("mime", "application/x-vnd.kde.plan");
    doc.setAttribute("version", PLAN_FILE_SYNTAX_VERSION);
    document.appendChild(doc);
    project->save(doc, XmlSaveContext());
}

//static
bool SchedulerThread::loadProject(Project *project, const KoXmlDocument &doc)
{
    KoXmlElement pel = doc.documentElement().namedItem("project").toElement();
    if (pel.isNull()) {
        return false;
    }
    XMLLoaderObject status;
    status.setVersion(PLAN_FILE_SYNTAX_VERSION);
    status.setProject(project);
    return project->load(pel, status);
}

// static
void SchedulerThread::updateProject(const Project *tp, const ScheduleManager *tm, Project *mp, ScheduleManager *sm)
{
    Q_CHECK_PTR(tp);
    Q_CHECK_PTR(tm);
    Q_CHECK_PTR(mp);
    Q_CHECK_PTR(sm);
    //debugPlan<<Q_FUNC_INFO<<tp<<tm<<tm->calculationResult()<<"->"<<mp<<sm;
    Q_ASSERT(tp != mp && tm != sm);
    long sid = tm->scheduleId();
    Q_ASSERT(sid == sm->scheduleId());

    sm->setCalculationResult(tm->calculationResult());

    XMLLoaderObject status;
    status.setVersion(PLAN_FILE_SYNTAX_VERSION);
    status.setProject(mp);
    status.setProjectTimeZone(mp->timeZone());

    const auto nodes = tp->allNodes();
    for (const Node *tn : nodes) {
        Node *mn = mp->findNode(tn->id());
        if (mn) {
            updateNode(tn, mn, sid, status);
        }
    }
    const auto resources = tp->resourceList();
    for (const Resource *tr : resources) {
        Resource *r = mp->findResource(tr->id());
        if (r) {
            updateResource(tr, r, status);
        }
    }
    // update main schedule and appointments
    updateAppointments(tp, tm, mp, sm, status);
    sm->scheduleChanged(sm->expected());
}

// static
void SchedulerThread::updateNode(const Node *tn, Node *mn, long sid, XMLLoaderObject &status)
{
    //debugPlan<<Q_FUNC_INFO<<tn<<"->"<<mn;
    NodeSchedule *s = static_cast<NodeSchedule*>(tn->schedule(sid));
    if (s == nullptr) {
        warnPlan<<Q_FUNC_INFO<<"Task:"<<tn->name()<<"could not find schedule with id:"<<sid;
        return;
    }
    QDomDocument doc("tmp");
    QDomElement e = doc.createElement("schedules");
    doc.appendChild(e);
    s->saveXML(e);

    Q_ASSERT(! mn->findSchedule(sid));
    s = static_cast<NodeSchedule*>(mn->schedule(sid));
    Q_ASSERT(s == nullptr);
    s = new NodeSchedule();

    KoXmlDocument xd;
    xd.setContent(doc.toString());
    KoXmlElement se = xd.documentElement().namedItem("schedule").toElement();
    Q_ASSERT(! se.isNull());

    s->loadXML(se, status);
    s->setDeleted(false);
    s->setNode(mn);
    mn->addSchedule(s);
}

// static
void SchedulerThread::updateResource(const Resource *tr, Resource *r, XMLLoaderObject &status)
{
    QDomDocument doc("tmp");
    QDomElement e = doc.createElement("cache");
    doc.appendChild(e);
    tr->saveCalendarIntervalsCache(e);

    KoXmlDocument xd;
    QString err;
    xd.setContent(doc.toString(), &err);
    KoXmlElement se = xd.documentElement();
    Q_ASSERT(! se.isNull());
    r->loadCalendarIntervalsCache(se, status);

    Calendar *cr = tr->calendar();
    Calendar *c = r->calendar();
    if (cr == nullptr || c == nullptr) {
        return;
    }
    //debugPlan<<"cr:"<<cr->cacheVersion()<<"c"<<c->cacheVersion();
    c->setCacheVersion(cr->cacheVersion());
}

// static
void SchedulerThread::updateAppointments(const Project *tp, const ScheduleManager *tm, Project *mp, ScheduleManager *sm, XMLLoaderObject &status)
{
    MainSchedule *sch = tm->expected();
    Q_ASSERT(sch);
    Q_ASSERT(sch != sm->expected());
    Q_ASSERT(sch->id() == sm->expected()->id());

    QDomDocument doc("tmp");
    QDomElement e = doc.createElement("schedule");
    doc.appendChild(e);
    sch->saveXML(e);
    tp->saveAppointments(e, sch->id());

    KoXmlDocument xd;
    xd.setContent(doc.toString());
    KoXmlElement se = xd.namedItem("schedule").toElement();
    Q_ASSERT(! se.isNull());

    bool ret = sm->loadMainSchedule(sm->expected(), se, status); // also loads appointments
    Q_ASSERT(ret);
#ifdef NDEBUG
    Q_UNUSED(ret)
#endif
    mp->setCurrentSchedule(sch->id());
    sm->expected()->setPhaseNames(sch->phaseNames());
    mp->changed(sm);
}

void SchedulerThread::schedule(SchedulingContext &context)
{
    Q_UNUSED(context)
}

ScheduleManager *SchedulerThread::getScheduleManager(Project *project)
{
    auto sm = project->findScheduleManagerByName(project->property(SCHEDULEMANAGERNAME).toString());
    if (!sm) {
        sm = project->currentScheduleManager();
    }
    if (!sm) {
        // create a new schedule top level schedule
        sm = project->createScheduleManager(project->uniqueScheduleName());
        project->addScheduleManager(sm);
        project->setProperty(SCHEDULEMANAGERNAME, sm->name());
        logDebug(project, nullptr, QString("Could not find suitable schedule, creating a new top level schedule: %1").arg(sm->name()));
    } else if (sm->isBaselined() || (sm->isScheduled() && project->isStarted() && !sm->parentManager())) {
        // create a subschedule
        KPlato::ScheduleManager *parent = sm;
        sm = project->createScheduleManager(parent);
        project->addScheduleManager(sm, parent);
        project->setProperty(SCHEDULEMANAGERNAME, sm->name());
        sm->setRecalculate(true);
        sm->setRecalculateFrom(QDateTime::currentDateTime());
        if (parent->isBaselined()) {
            logDebug(project, nullptr, QString("Selected schedule is baselined, a new sub-schedule is created: %1").arg(sm->name()));
        } else {
            logDebug(project, nullptr, QString("Project is started, a new sub-schedule is created: %1").arg(sm->name()));
        }
    } else if (sm->parentManager()) {
        sm->setRecalculate(true);
        sm->setRecalculateFrom(QDateTime::currentDateTime());
        logDebug(project, nullptr, QString("Re-schedule selected schedule: %1").arg(sm->name()));
    }
    Q_ASSERT(sm);
    if (!sm->expected()) {
        sm->createSchedules();
    }
    Q_ASSERT(sm->expected());
    Q_ASSERT(!project->schedules().isEmpty());
    project->setCurrentScheduleManager(sm);
    project->setCurrentSchedule(sm->scheduleId());
    return sm;
}

} //namespace KPlato

