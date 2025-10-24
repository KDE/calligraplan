/* This file is part of the KDE project
 SPDX-FileCopyrightText: 2001 Thomas zander <zander@kde.org>
 SPDX-FileCopyrightText: 2004-2010, 2012 Dag Andersen <dag.andersen@kdemail.net>
 SPDX-FileCopyrightText: 2007 Florian Piquemal <flotueur@yahoo.fr>
 SPDX-FileCopyrightText: 2007 Alexis Ménard <darktears31@gmail.com>
 SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>

 SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptproject.h"

#include "kptlocale.h"
#include "kptappointment.h"
#include "kpttask.h"
#include "kptdatetime.h"
#include "kpteffortcostmap.h"
#include "kptschedule.h"
#include "kptwbsdefinition.h"
#include "kptxmlloaderobject.h"
#include "XmlSaveContext.h"
#include "kptschedulerplugin.h"
#include "kptdebug.h"

#include <KoXmlReader.h>

#include <krandom.h>
#include <KFormat>
#include <KLocalizedString>

#include <QDateTime>
#include <QLocale>
#include <QElapsedTimer>

namespace KPlato
{

QString generateId()
{
    return QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMddHHmmss")) + KRandom::randomString(10);
}

Project::Project(Node *parent)
        : Node(parent),
        m_accounts(*this),
        m_defaultCalendar(nullptr),
        m_config(&emptyConfig),
        m_schedulerPlugins(),
        m_useSharedResources(false),
        m_sharedResourcesLoaded(false),
        m_useLocalTaskModules(true),
        m_currentScheduleManager(nullptr)
{
    //debugPlan<<"("<<this<<")";
    init();
}

Project::Project(ConfigBase &config, Node *parent)
        : Node(parent),
        m_accounts(*this),
        m_defaultCalendar(nullptr),
        m_config(&config),
        m_schedulerPlugins(),
        m_useSharedResources(false),
        m_sharedResourcesLoaded(false),
        m_useLocalTaskModules(true),
        m_currentScheduleManager(nullptr)
{
    debugPlan<<"("<<this<<")";
    init();
    m_config->setDefaultValues(*this);
}

Project::Project(ConfigBase &config, bool useDefaultValues, Node *parent)
        : Node(parent),
        m_accounts(*this),
        m_defaultCalendar(nullptr),
        m_config(&config),
        m_schedulerPlugins(),
        m_useSharedResources(false),
        m_sharedResourcesLoaded(false),
        m_useLocalTaskModules(true),
        m_currentScheduleManager(nullptr)
{
    debugPlan<<"("<<this<<")";
    init();
    if (useDefaultValues) {
        m_config->setDefaultValues(*this);
    }
}

void Project::init()
{
    m_refCount = 1; // always used by creator

    m_constraint = Node::MustStartOn;
    m_standardWorktime = new StandardWorktime();
    m_timeZone = QTimeZone::systemTimeZone(); // local timezone as default
    //debugPlan<<m_timeZone;
    if (m_parent == nullptr) {
        // set sensible defaults for a project wo parent
        m_constraintStartTime = DateTime(QDate::currentDate());
        m_constraintEndTime = m_constraintStartTime.addYears(2);
    }
    connect(this, &Project::sigCalculationFinished, this, &Project::emitProjectCalculated);
}

void Project::deref()
{
    --m_refCount;
    Q_ASSERT(m_refCount >= 0);
    if (m_refCount <= 0) {
        Q_EMIT aboutToBeDeleted();
        deleteLater();
    }
}

Project::~Project()
{
    disconnect();
    for(Node *n : std::as_const(nodeIdDict)) {
        n->blockChanged();
    }
    for (Resource *r : std::as_const(m_resources)) {
        r->blockChanged();
    }
    for (ResourceGroup *g : std::as_const(resourceGroupIdDict)) {
        g->blockChanged();
    }
    delete m_standardWorktime;
    for (Resource *r : std::as_const(m_resources)) {
        delete r;
    }
    while (!m_resourceGroups.isEmpty())
        delete m_resourceGroups.takeFirst();
    while (!m_calendars.isEmpty())
        delete m_calendars.takeFirst();
    while (!m_managers.isEmpty())
        delete m_managers.takeFirst();

    m_config = nullptr; //not mine, don't delete
}

int Project::type() const { return Node::Type_Project; }

void Project::setTimeZone(const QTimeZone &tz)
{
    m_timeZone = tz;
    if (m_constraintStartTime.isValid()) {
        m_constraintStartTime.setTimeZone(tz);
    }
    if (m_constraintEndTime.isValid()) {
        m_constraintEndTime.setTimeZone(tz);
    }
}

uint Project::status(const ScheduleManager *sm) const
{
    if (m_managerIdMap.isEmpty()) {
        return State_NotScheduled;
    }
    uint st = Node::State_None;
    if (sm) {
        if (sm->isBaselined()) {
            st |= State_Baselined;
        }
        if (!sm->isScheduled()) {
            st |= State_NotScheduled;
        }
        if (sm->scheduling()) {
            st |= State_Scheduling;
        }
    } else {
        if (isBaselined()) {
            st |= State_Baselined;
        }
    }
    const QList<Task*> tasks = allTasks();
    bool started = false;
    bool finished = false;
    bool first = true;
    for (const Task *t : tasks) {
        const Completion completion = t->completion();
        switch (t->type()) {
            case Type_Task:
            case Type_Milestone:
                started |= completion.isStarted();
                if (first) {
                    finished = completion.isFinished();
                    first = false;
                } else {
                    finished &= completion.isFinished();
                }
            default: break;
        }
    }
    if (started) {
        st |= State_Started;
    }
    if (finished) {
        st |= State_Finished;
    }
    return st;
}

void Project::generateUniqueNodeIds()
{
    const auto nodes = nodeIdDict.values();
    for (Node *n : nodes) {
        debugPlan<<n->name()<<"old"<<n->id();
        QString uid = uniqueNodeId();
        nodeIdDict.remove(n->id());
        n->setId(uid);
        nodeIdDict[ uid ] = n;
        debugPlan<<n->name()<<"new"<<n->id();
    }
}

void Project::generateUniqueIds()
{
    generateUniqueNodeIds();

    const auto groups = resourceGroupIdDict.values();
    for (ResourceGroup *g : groups) {
        if (g->isShared()) {
            continue;
        }
        resourceGroupIdDict.remove(g->id());
        g->setId(uniqueResourceGroupId());
        resourceGroupIdDict[ g->id() ] = g;
    }
    const auto resources = resourceIdDict.values();
    for (Resource *r : resources) {
        if (r->isShared()) {
            continue;
        }
        resourceIdDict.remove(r->id());
        r->setId(uniqueResourceId());
        resourceIdDict[ r->id() ] = r;
    }
    const auto calendars = calendarIdDict.values();
    for (Calendar *c : calendars) {
        if (c->isShared()) {
            continue;
        }
        calendarIdDict.remove(c->id());
        c->setId(uniqueCalendarId());
        calendarIdDict[ c->id() ] = c;
    }
}

void Project::calculate(Schedule *schedule, const DateTime &dt)
{
    if (schedule == nullptr) {
        errorPlan << "Schedule == 0, cannot calculate";
        return ;
    }
    m_currentSchedule = schedule;
    calculate(dt);
}

void Project::calculate(const DateTime &dt)
{
    if (m_currentSchedule == nullptr) {
        errorPlan << "No current schedule to calculate";
        return ;
    }
    if (stopcalculation) {
        return;
    }
    QLocale locale;
    DateTime time = dt.isValid() ? dt : DateTime(QDateTime::currentDateTime());
    MainSchedule *cs = static_cast<MainSchedule*>(m_currentSchedule);
    Estimate::Use estType = (Estimate::Use) cs->type();
    if (type() == Type_Project) {
        cs->setPhaseName(0, i18n("Init"));
        cs->logInfo(i18n("Schedule project from: %1", locale.toString(dt, QLocale::ShortFormat)), 0);
        initiateCalculation(*cs);
        initiateCalculationLists(*cs); // must be after initiateCalculation() !!
        propagateEarliestStart(time);
        // Calculate lateFinish from time. If a task has started, remainingEffort is used.
        cs->setPhaseName(1, i18nc("Schedule project forward", "Forward"));
        cs->logInfo(i18n("Calculate finish"), 1);
        cs->lateFinish = calculateForward(estType);
        cs->lateFinish = checkEndConstraints(cs->lateFinish);
        propagateLatestFinish(cs->lateFinish);
        // Calculate earlyFinish. If a task has started, remainingEffort is used.
        cs->setPhaseName(2, i18nc("Schedule project backward","Backward"));
        cs->logInfo(i18n("Calculate start"), 2);
        calculateBackward(estType);
        // Schedule. If a task has started, remainingEffort is used and appointments are copied from parent
        cs->setPhaseName(3, i18n("Schedule"));
        cs->logInfo(i18n("Schedule tasks forward"), 3);
        cs->endTime = scheduleForward(cs->startTime, estType);
        if (stopcalculation) {
            cs->logWarning(i18n("Scheduling canceled"), 3);
        } else {
            cs->logInfo(i18n("Scheduled finish: %1", locale.toString(cs->endTime, QLocale::ShortFormat)), 3);
            if (cs->endTime > m_constraintEndTime) {
                cs->logError(i18n("Could not finish project in time: %1", locale.toString(m_constraintEndTime, QLocale::ShortFormat)), 3);
            } else if (cs->endTime == m_constraintEndTime) {
                cs->logWarning(i18n("Finished project exactly on time: %1", locale.toString(m_constraintEndTime, QLocale::ShortFormat)), 3);
            } else {
                cs->logInfo(i18n("Finished project before time: %1", locale.toString(m_constraintEndTime, QLocale::ShortFormat)), 3);
            }
            calcCriticalPath(false);
            calcResourceOverbooked();
            cs->notScheduled = false;
            calcFreeFloat();
        }
        Q_EMIT scheduleChanged(cs);
        Q_EMIT projectChanged();
    } else if (type() == Type_Subproject) {
        warnPlan << "Subprojects not implemented";
    } else {
        errorPlan << "Illegal project type: " << type();
    }
}

void Project::calculate(ScheduleManager &sm)
{
    Q_EMIT sigCalculationStarted(this, &sm);
    sm.setScheduling(true);
    m_progress = 0;
    int nodes = 0;
    const auto nodesList = nodeIdDict.values();
    for (Node *n : nodesList) {
        if (n->type() == Node::Type_Task || n->type() == Node::Type_Milestone) {
            nodes++;
        }
    }
    int maxprogress = nodes * 3;
    if (sm.recalculate()) {
        Q_EMIT maxProgress(maxprogress);
        sm.setMaxProgress(maxprogress);
        incProgress();
        if (sm.parentManager()) {
            sm.expected()->startTime = sm.parentManager()->expected()->startTime;
            sm.expected()->earlyStart = sm.parentManager()->expected()->earlyStart;
        }
        incProgress();
        calculate(sm.expected(), sm.recalculateFrom());
    } else {
        Q_EMIT maxProgress(maxprogress);
        sm.setMaxProgress(maxprogress);
        calculate(sm.expected());
        Q_EMIT scheduleChanged(sm.expected());
        setCurrentSchedule(sm.expected()->id());
    }
    Q_EMIT sigProgress(maxprogress);
    Q_EMIT sigCalculationFinished(this, &sm);
    Q_EMIT scheduleManagerChanged(&sm);
    Q_EMIT projectChanged();
    sm.setScheduling(false);
}

void Project::calculate(Schedule *schedule)
{
    if (schedule == nullptr) {
        errorPlan << "Schedule == 0, cannot calculate";
        return ;
    }
    m_currentSchedule = schedule;
    calculate();
}

void Project::calculate()
{
    if (m_currentSchedule == nullptr) {
        errorPlan << "No current schedule to calculate";
        return ;
    }
    if (stopcalculation) {
        return;
    }
    MainSchedule *cs = static_cast<MainSchedule*>(m_currentSchedule);
    bool backwards = false;
    if (cs->manager()) {
        backwards = cs->manager()->schedulingDirection();
    }
    QLocale locale;
    Estimate::Use estType = (Estimate::Use) cs->type();
    if (type() == Type_Project) {
        QElapsedTimer timer; timer.start();
        initiateCalculation(*cs);
        initiateCalculationLists(*cs); // must be after initiateCalculation() !!
        if (! backwards) {
            cs->setPhaseName(0, i18n("Init"));
            cs->logInfo(i18n("Schedule project forward from: %1", locale.toString(m_constraintStartTime, QLocale::ShortFormat)), 0);
            cs->startTime = m_constraintStartTime;
            cs->earlyStart = m_constraintStartTime;
            // Calculate from start time
            propagateEarliestStart(cs->earlyStart);
            cs->setPhaseName(1, i18nc("Schedule project forward", "Forward"));
            cs->logInfo(i18n("Calculate late finish"), 1);
            cs->lateFinish = calculateForward(estType);
//            cs->lateFinish = checkEndConstraints(cs->lateFinish);
            cs->logInfo(i18n("Late finish calculated: %1", locale.toString(cs->lateFinish, QLocale::ShortFormat)), 1);
            propagateLatestFinish(cs->lateFinish);
            cs->setPhaseName(2, i18nc("Schedule project backward", "Backward"));
            cs->logInfo(i18n("Calculate early start"), 2);
            calculateBackward(estType);
            if (!stopcalculation) {
                cs->setPhaseName(3, i18n("Schedule"));
                cs->logInfo(i18n("Schedule tasks forward"), 3);
                cs->endTime = scheduleForward(cs->startTime, estType);
                cs->duration = cs->endTime - cs->startTime;
                cs->logInfo(i18n("Scheduled finish: %1", locale.toString(cs->endTime, QLocale::ShortFormat)), 3);
                if (cs->endTime > m_constraintEndTime) {
                    cs->constraintError = true;
                    cs->logError(i18n("Could not finish project in time: %1", locale.toString(m_constraintEndTime, QLocale::ShortFormat)), 3);
                } else if (cs->endTime == m_constraintEndTime) {
                    cs->logWarning(i18n("Finished project exactly on time: %1", locale.toString(m_constraintEndTime, QLocale::ShortFormat)), 3);
                } else {
                    cs->logInfo(i18n("Finished project before time: %1", locale.toString(m_constraintEndTime, QLocale::ShortFormat)), 3);
                }
                calcCriticalPath(false);
            }
        } else {
            cs->setPhaseName(0, i18n("Init"));
            cs->logInfo(i18n("Schedule project backward from: %1", locale.toString(m_constraintEndTime, QLocale::ShortFormat)), 0);
            // Calculate from end time
            propagateLatestFinish(m_constraintEndTime);
            cs->setPhaseName(1, i18nc("Schedule project backward", "Backward"));
            cs->logInfo(i18n("Calculate early start"), 1);
            cs->earlyStart = calculateBackward(estType);
//            cs->earlyStart = checkStartConstraints(cs->earlyStart);
            cs->logInfo(i18n("Early start calculated: %1", locale.toString(cs->earlyStart, QLocale::ShortFormat)), 1);
            propagateEarliestStart(cs->earlyStart);
            cs->setPhaseName(2, i18nc("Schedule project forward", "Forward"));
            cs->logInfo(i18n("Calculate late finish"), 2);
            cs->lateFinish = qMax(m_constraintEndTime, calculateForward(estType));
            cs->logInfo(i18n("Late finish calculated: %1", locale.toString(cs->lateFinish, QLocale::ShortFormat)), 2);
            cs->setPhaseName(3, i18n("Schedule"));
            cs->logInfo(i18n("Schedule tasks backward"), 3);
            cs->startTime = scheduleBackward(cs->lateFinish, estType);
            if (!stopcalculation) {
                cs->endTime = cs->startTime;
                const auto nodes = allNodes();
                for (Node *n : nodes) {
                    if (n->type() == Type_Task || n->type() == Type_Milestone) {
                        DateTime e = n->endTime(cs->id());
                        if (cs->endTime <  e) {
                            cs->endTime = e;
                        }
                    }
                }
                if (cs->endTime > m_constraintEndTime) {
                    cs->constraintError = true;
                    cs->logError(i18n("Failed to finish project within target time"), 3);
                }
                cs->duration = cs->endTime - cs->startTime;
                cs->logInfo(i18n("Scheduled start: %1, target time: %2", locale.toString(cs->startTime, QLocale::ShortFormat), locale.toString(m_constraintStartTime, QLocale::ShortFormat)), 3);
                if (cs->startTime < m_constraintStartTime) {
                    cs->constraintError = true;
                    cs->logError(i18n("Must start project early in order to finish in time: %1", locale.toString(m_constraintStartTime, QLocale::ShortFormat)), 3);
                } else if (cs->startTime == m_constraintStartTime) {
                    cs->logWarning(i18n("Start project exactly on time: %1", locale.toString(m_constraintStartTime, QLocale::ShortFormat)), 3);
                } else {
                    cs->logInfo(i18n("Can start project later than time: %1", locale.toString(m_constraintStartTime, QLocale::ShortFormat)), 3);
                }
                calcCriticalPath(true);
            }
        }
        if (stopcalculation) {
            cs->logWarning(i18n("Scheduling canceled"), 3);
        } else {
            cs->logInfo(i18n("Calculation took: %1", KFormat().formatDuration(timer.elapsed())));
            // TODO: fix this uncertainty, manager should *always* be available
            if (cs->manager()) {
                finishCalculation(*(cs->manager()));
            }
        }
    } else if (type() == Type_Subproject) {
        warnPlan << "Subprojects not implemented";
    } else {
        errorPlan << "Illegal project type: " << type();
    }
}

void Project::finishCalculation(ScheduleManager &sm)
{
    MainSchedule *cs = sm.expected();
    if (nodeIdDict.count() > 1) {
        // calculate project duration
        cs->startTime = m_constraintEndTime;
        cs->endTime = m_constraintStartTime;
        for (const Node *n : std::as_const(nodeIdDict)) {
            cs->startTime = qMin(cs->startTime, n->startTime(cs->id()));
            cs->endTime = qMax(cs->endTime, n->endTime(cs->id()));
        }
        cs->duration = cs->endTime - cs->startTime;
    }

    calcCriticalPath(false);
    calcResourceOverbooked();
    cs->notScheduled = false;
    calcFreeFloat();
    Q_EMIT scheduleChanged(cs);
    Q_EMIT projectChanged();
}

void Project::setProgress(int progress, ScheduleManager *sm)
{
    m_progress = progress;
    if (sm) {
        sm->setProgress(progress);
    }
    Q_EMIT sigProgress(progress);
}

void Project::setMaxProgress(int max, ScheduleManager *sm)
{
    if (sm) {
        sm->setMaxProgress(max);
    }
    emitMaxProgress(max);
}

void Project::incProgress()
{
    m_progress += 1;
    Q_EMIT sigProgress(m_progress);
}

void Project::emitMaxProgress(int value)
{
    Q_EMIT maxProgress(value);
}

bool Project::calcCriticalPath(bool fromEnd)
{
    //debugPlan;
    MainSchedule *cs = static_cast<MainSchedule*>(m_currentSchedule);
    if (cs == nullptr) {
        return false;
    }
    if (fromEnd) {
        QListIterator<Node*> startnodes = cs->startNodes();
        while (startnodes.hasNext()) {
            startnodes.next() ->calcCriticalPath(fromEnd);
        }
    } else {
        QListIterator<Node*> endnodes = cs->endNodes();
        while (endnodes.hasNext()) {
            endnodes.next() ->calcCriticalPath(fromEnd);
        }
    }
    calcCriticalPathList(cs);
    return false;
}

void Project::calcCriticalPathList(MainSchedule *cs)
{
    //debugPlan<<m_name<<", "<<cs->name();
    cs->clearCriticalPathList();
    const auto nodes = allNodes();
    for (Node *n : nodes) {
        if (n->numDependParentNodes() == 0 && n->inCriticalPath(cs->id())) {
            cs->addCriticalPath();
            cs->addCriticalPathNode(n);
            calcCriticalPathList(cs, n);
        }
    }
    cs->criticalPathListCached = true;
    //debugPlan<<*(criticalPathList(cs->id()));
}

void Project::calcCriticalPathList(MainSchedule *cs, Node *node)
{
    //debugPlan<<node->name()<<", "<<cs->id();
    bool newPath = false;
    QList<Node*> lst = *(cs->currentCriticalPath());
    const auto relations = node->dependChildNodes();
    for (Relation *r : relations) {
        if (r->child()->inCriticalPath(cs->id())) {
            if (newPath) {
                cs->addCriticalPath(&lst);
                //debugPlan<<node->name()<<" new path";
            }
            cs->addCriticalPathNode(r->child());
            calcCriticalPathList(cs, r->child());
            newPath = true;
        }
    }
}

const QList< QList<Node*> > *Project::criticalPathList(long id)
{
    Schedule *s = schedule(id);
    if (s == nullptr) {
        //debugPlan<<"No schedule with id="<<id;
        return nullptr;
    }
    MainSchedule *ms = static_cast<MainSchedule*>(s);
    if (! ms->criticalPathListCached) {
        initiateCalculationLists(*ms);
        calcCriticalPathList(ms);
    }
    return ms->criticalPathList();
}

QList<Node*> Project::criticalPath(long id, int index)
{
    Schedule *s = schedule(id);
    if (s == nullptr) {
        //debugPlan<<"No schedule with id="<<id;
        return QList<Node*>();
    }
    MainSchedule *ms = static_cast<MainSchedule*>(s);
    if (! ms->criticalPathListCached) {
        initiateCalculationLists(*ms);
        calcCriticalPathList(ms);
    }
    return ms->criticalPath(index);
}

DateTime Project::startTime(long id) const
{
    Schedule *s = schedule(id);
    return s ? s->startTime : m_constraintStartTime;
}

DateTime Project::endTime(long id) const
{
    Schedule *s = schedule(id);
    return s ? s->endTime : m_constraintEndTime;
}

Duration Project::duration(long id) const
{
    Schedule *s = schedule(id);
    return s ? s->duration : Duration::zeroDuration;
}

Duration *Project::getRandomDuration()
{
    return nullptr;
}

DateTime Project::checkStartConstraints(const DateTime &dt) const
{
    DateTime t = dt;
    const auto nodes = nodeIdDict.values();
    for (Node *n : nodes) {
        if (n->type() == Node::Type_Task || n->type() == Node::Type_Milestone) {
            switch (n->constraint()) {
                case Node::FixedInterval:
                case Node::StartNotEarlier:
                case Node::MustStartOn:
                        t = qMin(t, qMax(n->constraintStartTime(), m_constraintStartTime));
                        break;
                default: break;
            }
        }
    }
    return t;
}

DateTime Project::checkEndConstraints(const DateTime &dt) const
{
    DateTime t = dt;
    const auto nodes = nodeIdDict.values();
    for (Node *n : nodes) {
        if (n->type() == Node::Type_Task || n->type() == Node::Type_Milestone) {
            switch (n->constraint()) {
                case Node::FixedInterval:
                case Node::FinishNotLater:
                case Node::MustFinishOn:
                        t = qMax(t, qMin(n->constraintEndTime(), m_constraintEndTime));
                        break;
                default: break;
            }
        }
    }
    return t;
}

#ifndef PLAN_NLOGDEBUG
bool Project::checkParent(Node *n, const QList<Node*> &list, QList<Relation*> &checked)
{
    if (n->isStartNode()) {
        debugPlan<<n<<"start node"<<list;
        return true;
    }
    debugPlan<<"Check:"<<n<<":"<<checked.count()<<":"<<list;
    if (list.contains(n)) {
        debugPlan<<"Failed:"<<n<<":"<<list;
        return false;
    }
    QList<Node*> lst = list;
    lst << n;
    const auto relations = n->dependParentNodes();
    for (Relation *r : relations) {
        if (checked.contains(r)) {
            debugPlan<<"Depend:"<<n<<":"<<r->parent()<<": checked";
            continue;
        }
        checked << r;
        if (! checkParent(r->parent(), lst, checked)) {
            return false;
        }
    }
    Task *t = static_cast<Task*>(n);
    const auto relations2 = t->parentProxyRelations();
    for (Relation *r : relations2) {
        if (checked.contains(r)) {
            debugPlan<<"Depend:"<<n<<":"<<r->parent()<<": checked";
            continue;
        }
        checked << r;
        debugPlan<<"Proxy:"<<n<<":"<<r->parent()<<":"<<lst;
        if (! checkParent(r->parent(), lst, checked)) {
            return false;
        }
    }
    return true;
}

bool Project::checkChildren(Node *n, const QList<Node*> &list, QList<Relation*> &checked)
{
    if (n->isEndNode()) {
        debugPlan<<n<<"end node"<<list;
        return true;
    }
    debugPlan<<"Check:"<<n<<":"<<checked.count()<<":"<<list;
    if (list.contains(n)) {
        debugPlan<<"Failed:"<<n<<":"<<list;
        return false;
    }
    QList<Node*> lst = list;
    lst << n;
    const auto relations = n->dependChildNodes();
    for (Relation *r : relations) {
        if (checked.contains(r)) {
            debugPlan<<"Depend:"<<n<<":"<<r->parent()<<": checked";
            continue;
        }
        checked << r;
        if (! checkChildren(r->child(), lst, checked)) {
            return false;
        }
    }
    Task *t = static_cast<Task*>(n);
    const auto relations2 = t->childProxyRelations();
    for (Relation *r : relations2) {
        if (checked.contains(r)) {
            debugPlan<<"Depend:"<<n<<":"<<r->parent()<<": checked";
            continue;
        }
        debugPlan<<"Proxy:"<<n<<":"<<r->parent()<<":"<<lst;
        checked << r;
        if (! checkChildren(r->child(), lst, checked)) {
            return false;
        }
    }
    return true;
}
#endif

void Project::tasksForward()
{
    m_hardConstraints.clear();
    m_softConstraints.clear();
    m_terminalNodes.clear();
    m_priorityNodes.clear();
    int prio = 0;
    bool priorityUsed = false;

    // Do these in reverse order to get tasks with same prio in wbs order
    const QList<Task*> tasks = allTasks();
    for (int i = tasks.count() -1; i >= 0; --i) {
        Task *t = tasks.at(i);
        if (i == tasks.count() -1) {
            prio = t->priority();
        } else if (!priorityUsed && t->priority() != prio) {
            priorityUsed = true;
        }
        m_priorityNodes.insert(-t->priority(), t);
        switch (t->constraint()) {
            case Node::MustStartOn:
            case Node::MustFinishOn:
            case Node::FixedInterval:
                m_hardConstraints.prepend(t);
                break;
            case Node::StartNotEarlier:
            case Node::FinishNotLater:
                m_softConstraints.prepend(t);
                break;
            default:
                if (t->isEndNode()) {
                    m_terminalNodes.insert(-t->priority(), t);
                }
                break;
        }
    }
    if (!priorityUsed) {
        m_priorityNodes.clear();
    }
#ifndef PLAN_NLOGDEBUG
    debugPlan<<"End nodes:"<<m_terminalNodes;
    for (Node* n : std::as_const(m_terminalNodes)) {
        QList<Node*> lst;
        QList<Relation*> rel;
        Q_ASSERT(checkParent(n, lst, rel)); Q_UNUSED(n);
    }
#endif
}

void Project::tasksBackward()
{
    m_hardConstraints.clear();
    m_softConstraints.clear();
    m_terminalNodes.clear();
    m_priorityNodes.clear();
    int prio = 0;
    bool priorityUsed = false;

    // Do these in reverse order to get tasks with same prio in wbs order
    const QList<Task*> tasks = allTasks();
    for (int i = tasks.count() -1; i >= 0; --i) {
        Task *t = tasks.at(i);
        if (i == tasks.count() -1) {
            prio = t->priority();
        } else if (!priorityUsed && t->priority() != prio) {
            priorityUsed = true;
        }
        m_priorityNodes.insert(-t->priority(), t);
        switch (t->constraint()) {
            case Node::MustStartOn:
            case Node::MustFinishOn:
            case Node::FixedInterval:
                m_hardConstraints.prepend(t);
                break;
            case Node::StartNotEarlier:
            case Node::FinishNotLater:
                m_softConstraints.prepend(t);
                break;
            default:
                if (t->isStartNode()) {
                    m_terminalNodes.insert(-t->priority(), t);
                }
                break;
        }
    }
    if (!priorityUsed) {
        m_priorityNodes.clear();
    }
#ifndef PLAN_NLOGDEBUG
    debugPlan<<"Start nodes:"<<m_terminalNodes;
    for (Node* n : std::as_const(m_terminalNodes)) {
        QList<Node*> lst;
        QList<Relation*> rel;
        Q_ASSERT(checkChildren(n, lst, rel)); Q_UNUSED(n);
    }
#endif
}

DateTime Project::calculateForward(int use)
{
    //debugPlan<<m_name;
    DateTime finish;
    MainSchedule *cs = static_cast<MainSchedule*>(m_currentSchedule);
    if (cs == nullptr) {
        return finish;
    }
    if (type() == Node::Type_Project) {
        QElapsedTimer timer;
        timer.start();
        cs->logInfo(i18n("Start calculating forward"));
        m_visitedForward = true;
        if (! m_visitedBackward) {
            // setup tasks
            tasksForward();
            if (!m_priorityNodes.isEmpty()) {
                for (Node *n : std::as_const(m_priorityNodes)) {
                    cs->logDebug(QStringLiteral("Calculate task '%1' by priority: %2").arg(n->name()).arg(n->priority()));
                    DateTime time = n->calculateForward(use);
                    if (time > finish) {
                        finish = time;
                        cs->setLatestFinish(time);
                    }
                }
                return finish;
            }
            // Do all hard constrained first
            for (Node *n : std::as_const(m_hardConstraints)) {
                cs->logDebug(QStringLiteral("Calculate task with hard constraint: ") + n->name() + QStringLiteral(" : ") + n->constraintToString());
                DateTime time = n->calculateEarlyFinish(use); // do not do predeccessors
                if (time > finish) {
                    finish = time;
                }
            }
            // do the predeccessors
            for (Node *n : std::as_const(m_hardConstraints)) {
                cs->logDebug(QStringLiteral("Calculate predeccessors to hard constrained task: ") + n->name() + QStringLiteral(" : ") + n->constraintToString());
                DateTime time = n->calculateForward(use);
                if (time > finish) {
                    finish = time;
                }
            }
            // now try to schedule soft constrained *with* predeccessors
            for (Node *n : std::as_const(m_softConstraints)) {
                cs->logDebug(QStringLiteral("Calculate task with soft constraint: ") + n->name() + QStringLiteral(" : ") + n->constraintToString());
                DateTime time = n->calculateForward(use);
                if (time > finish) {
                    finish = time;
                }
            }
            // and then the rest using the end nodes to calculate everything (remaining)
            for (Task *n : std::as_const(m_terminalNodes)) {
                cs->logDebug(QStringLiteral("Calculate using end task: ") + n->name() + QStringLiteral(" : ") + n->constraintToString());
                DateTime time = n->calculateForward(use);
                if (time > finish) {
                    finish = time;
                }
            }
        } else {
            // Tasks have been calculated backwards in this order
            // Do backwords if priority is not used
            if (m_priorityNodes.isEmpty()) {
                const auto nodes =  cs->backwardNodes();
                for (Node *n : nodes) {
                    DateTime time = n->calculateForward(use);
                    if (time > finish) {
                        finish = time;
                    }
                }
            }
        }
        cs->logInfo(i18n("Finished calculating forward: %1 ms", timer.elapsed()));
    } else {
        //TODO: subproject
    }
    return finish;
}

DateTime Project::calculateBackward(int use)
{
    //debugPlan<<m_name;
    DateTime start;
    MainSchedule *cs = static_cast<MainSchedule*>(m_currentSchedule);
    if (cs == nullptr) {
        return start;
    }
    if (type() == Node::Type_Project) {
        QElapsedTimer timer;
        timer.start();
        cs->logInfo(i18n("Start calculating backward"));
        m_visitedBackward = true;
        if (! m_visitedForward) {
            // setup tasks
            tasksBackward();
            if (!m_priorityNodes.isEmpty()) {
                for (Node *n : std::as_const(m_priorityNodes)) {
                    cs->logDebug(QStringLiteral("Calculate task '%1' by priority: %2").arg(n->name()).arg(n->priority()));
                    DateTime time = n->calculateBackward(use);
                    if (! start.isValid() || time < start) {
                        start = time;
                    }
                }
                return start;
            }
            // Do all hard constrained first
            for (Task *n : std::as_const(m_hardConstraints)) {
                cs->logDebug(QStringLiteral("Calculate task with hard constraint: ") + n->name() + QStringLiteral(" : ") + n->constraintToString());
                DateTime time = n->calculateLateStart(use); // do not do predeccessors
                if (! start.isValid() || time < start) {
                    start = time;
                }
            }
            // then do the predeccessors
            for (Task *n : std::as_const(m_hardConstraints)) {
                cs->logDebug(QStringLiteral("Calculate predeccessors to hard constrained task:" ) + n->name() + QStringLiteral(" : ") + n->constraintToString());
                DateTime time = n->calculateBackward(use);
                if (! start.isValid() || time < start) {
                    start = time;
                }
            }
            // now try to schedule soft constrained *with* predeccessors
            for (Task *n : std::as_const(m_softConstraints)) {
                cs->logDebug(QStringLiteral("Calculate task with soft constraint: ") + n->name() + QStringLiteral(" : ") + n->constraintToString());
                DateTime time = n->calculateBackward(use);
                if (! start.isValid() || time < start) {
                    start = time;
                }
            }
            // and then the rest using the start nodes to calculate everything (remaining)
            for (Task *n : std::as_const(m_terminalNodes)) {
                cs->logDebug(QStringLiteral("Calculate using start task: ") + n->name() + QStringLiteral(" : ") + n->constraintToString());
                DateTime time = n->calculateBackward(use);
                if (! start.isValid() || time < start) {
                    start = time;
                }
            }
        } else {
            // tasks have been calculated forwards in this order
            const auto nodes = cs->forwardNodes();
            for (Node *n : nodes) {
                DateTime time = n->calculateBackward(use);
                if (! start.isValid() || time < start) {
                    start = time;
                }
            }
        }
        cs->logInfo(i18n("Finished calculating backward: %1 ms", timer.elapsed()));
    } else {
        //TODO: subproject
    }
    return start;
}

DateTime Project::scheduleForward(const DateTime &earliest, int use)
{
    DateTime end;
    MainSchedule *cs = static_cast<MainSchedule*>(m_currentSchedule);
    if (cs == nullptr || stopcalculation) {
        return DateTime();
    }
    QElapsedTimer timer;
    timer.start();
    cs->logInfo(i18n("Start scheduling forward"));
    resetVisited();

    if (!m_priorityNodes.isEmpty()) {
        for (Node *n : std::as_const(m_priorityNodes)) {
            cs->logDebug(QStringLiteral("Schedule task '%1' by priority: %2").arg(n->name()).arg(n->priority()));
            DateTime time = n->scheduleFromStartTime(use); // do not do predeccessors
            if (time > end) {
                end = time;
                cs->setLatestFinish(time);
            }
        }
        return end;
    }

    // Schedule in the same order as calculated forward
    // Do all hard constrained first
    for (Node *n : std::as_const(m_hardConstraints)) {
        cs->logDebug(QStringLiteral("Schedule task with hard constraint: ") + n->name() + QStringLiteral(" : ") + n->constraintToString());
        DateTime time = n->scheduleFromStartTime(use); // do not do predeccessors
        if (time > end) {
            end = time;
        }
    }
    const auto nodes = cs->forwardNodes();
    for (Node *n : nodes) {
        cs->logDebug(QStringLiteral("Schedule task: ") + n->name() + QStringLiteral(" : ") + n->constraintToString());
        DateTime time = n->scheduleForward(earliest, use);
        if (time > end) {
            end = time;
        }
    }
    // Fix summarytasks
    adjustSummarytask();
    cs->logInfo(i18n("Finished scheduling forward: %1 ms", timer.elapsed()));
    const auto nodes2 = allNodes();
    for (Node *n : nodes2) {
        if (n->type() == Node::Type_Task || n->type() == Node::Type_Milestone) {
            Q_ASSERT(n->isScheduled());
        }
    }

    return end;
}

DateTime Project::scheduleBackward(const DateTime &latest, int use)
{
    DateTime start;
    MainSchedule *cs = static_cast<MainSchedule*>(m_currentSchedule);
    if (cs == nullptr || stopcalculation) {
        return start;
    }
    QElapsedTimer timer;
    timer.start();
    cs->logInfo(i18n("Start scheduling backward"));
    resetVisited();
    // Schedule in the same order as calculated backward
    // Do all hard constrained first
    for (Node *n : std::as_const(m_hardConstraints)) {
        cs->logDebug(QStringLiteral("Schedule task with hard constraint:") + n->name() + QStringLiteral(" : ") + n->constraintToString());
        DateTime time = n->scheduleFromEndTime(use); // do not do predeccessors
        if (! start.isValid() || time < start) {
            start = time;
        }
    }
    const auto nodes = cs->backwardNodes();
    for (Node *n : nodes) {
        cs->logDebug(QStringLiteral("Schedule task:") + n->name() + QStringLiteral(" : ") + n->constraintToString());
        DateTime time = n->scheduleBackward(latest, use);
        if (! start.isValid() || time < start) {
            start = time;
        }
    }
    // Fix summarytasks
    adjustSummarytask();
    cs->logInfo(i18n("Finished scheduling backward: %1 ms", timer.elapsed()));
    const auto nodes2 = allNodes();
    for (Node *n : nodes2) {
        if (n->type() == Node::Type_Task || n->type() == Node::Type_Milestone) {
            Q_ASSERT(n->isScheduled());
        }
    }
    return start;
}

void Project::adjustSummarytask()
{
    MainSchedule *cs = static_cast<MainSchedule*>(m_currentSchedule);
    if (cs == nullptr || stopcalculation) {
        return;
    }
    QListIterator<Node*> it(cs->summaryTasks());
    while (it.hasNext()) {
        it.next() ->adjustSummarytask();
    }
}

void Project::initiateCalculation(MainSchedule &sch)
{
    //debugPlan<<m_name;
    // clear all resource appointments
    m_visitedForward = false;
    m_visitedBackward = false;
    QListIterator<ResourceGroup*> git(m_resourceGroups);
    while (git.hasNext()) {
        git.next() ->initiateCalculation(sch);
    }
    for (Resource *r : std::as_const(m_resources)) {
        r->initiateCalculation(sch);
    }

    Node::initiateCalculation(sch);
}

void Project::initiateCalculationLists(MainSchedule &sch)
{
    //debugPlan<<this;
    sch.clearNodes();
    if (type() == Node::Type_Project) {
        QListIterator<Node*> it = childNodeIterator();
        while (it.hasNext()) {
            it.next() ->initiateCalculationLists(sch);
        }
    } else {
        //TODO: subproject
    }
}

void Project::saveSettings(QDomElement &element, const XmlSaveContext &context) const
{
    Q_UNUSED(context)

    QDomElement settingsElement = element.ownerDocument().createElement(QStringLiteral("project-settings"));
    element.appendChild(settingsElement);

    m_wbsDefinition.saveXML(settingsElement);

    QDomElement loc = settingsElement.ownerDocument().createElement(QStringLiteral("locale"));
    settingsElement.appendChild(loc);
    const Locale *l = locale();
    loc.setAttribute(QStringLiteral("currency-symbol"), l->currencySymbol());
    loc.setAttribute(QStringLiteral("currency-digits"), l->monetaryDecimalPlaces());
    loc.setAttribute(QStringLiteral("language"), l->currencyLanguage());
    loc.setAttribute(QStringLiteral("country"), l->currencyTerritory());

    QDomElement share = settingsElement.ownerDocument().createElement(QStringLiteral("shared-resources"));
    settingsElement.appendChild(share);
    share.setAttribute(QStringLiteral("use"), m_useSharedResources);
    share.setAttribute(QStringLiteral("file"), m_sharedResourcesFile);

    QDomElement wpi = settingsElement.ownerDocument().createElement(QStringLiteral("workpackageinfo"));
    settingsElement.appendChild(wpi);
    wpi.setAttribute(QStringLiteral("check-for-workpackages"), m_workPackageInfo.checkForWorkPackages);
    wpi.setAttribute(QStringLiteral("retrieve-url"), m_workPackageInfo.retrieveUrl.toString(QUrl::None));
    wpi.setAttribute(QStringLiteral("delete-after-retrieval"), m_workPackageInfo.deleteAfterRetrieval);
    wpi.setAttribute(QStringLiteral("archive-after-retrieval"), m_workPackageInfo.archiveAfterRetrieval);
    wpi.setAttribute(QStringLiteral("archive-url"), m_workPackageInfo.archiveUrl.toString(QUrl::None));
    wpi.setAttribute(QStringLiteral("publish-url"), m_workPackageInfo.publishUrl.toString(QUrl::None));

    QDomElement tm = settingsElement.ownerDocument().createElement(QStringLiteral("task-modules"));
    settingsElement.appendChild(tm);
    tm.setAttribute(QStringLiteral("use-local-task-modules"), m_useLocalTaskModules);
    const auto modules = taskModules(false/*no local*/);
    for (const QUrl &url : modules) {
        QDomElement e = tm.ownerDocument().createElement(QStringLiteral("task-module"));
        tm.appendChild(e);
        e.setAttribute(QStringLiteral("url"), url.toString());
    }
    // save standard worktime
    if (m_standardWorktime) {
        m_standardWorktime->save(settingsElement);
    }
    QDomElement freedays = settingsElement.ownerDocument().createElement(QStringLiteral("freedays"));
    settingsElement.appendChild(freedays);
    if (m_freedaysCalendar) {
        freedays.setAttribute(QStringLiteral("calendar-id"), m_freedaysCalendar->id());
    }
}

void Project::save(QDomElement &element, const XmlSaveContext &context) const
{
    debugPlanXml<<context.options;

    QDomElement me = element.ownerDocument().createElement(QStringLiteral("project"));
    element.appendChild(me);

    me.setAttribute(QStringLiteral("name"), m_name);
    me.setAttribute(QStringLiteral("leader"), m_leader);
    me.setAttribute(QStringLiteral("id"), m_id);
    me.setAttribute(QStringLiteral("priority"), QString::number(m_priority));
    me.setAttribute(QStringLiteral("description"), m_description);
    me.setAttribute(QStringLiteral("timezone"), m_timeZone.isValid() ? QString::fromLatin1(m_timeZone.id()) : QString());

    me.setAttribute(QStringLiteral("scheduling"), constraintToString());
    me.setAttribute(QStringLiteral("start-time"), m_constraintStartTime.toString(Qt::ISODate));
    me.setAttribute(QStringLiteral("end-time"), m_constraintEndTime.toString(Qt::ISODate));

    saveSettings(me, context);
    m_documents.save(me); // project documents

    if (context.saveAll(this)) {
        debugPlanXml<<"accounts";
        m_accounts.save(me);

        // save calendars
        debugPlanXml<<"calendars:"<<calendarIdDict.count();
        if (!calendarIdDict.isEmpty()) {
            QDomElement ce = me.ownerDocument().createElement(QStringLiteral("calendars"));
            me.appendChild(ce);
            const auto calendars = calendarIdDict.values();
            for (Calendar *c : calendars) {
                c->save(ce);
            }
        }
        // save project resources
        debugPlanXml<<"resource-groups:"<<m_resourceGroups.count();
        if (!m_resourceGroups.isEmpty()) {
            QDomElement ge = me.ownerDocument().createElement(QStringLiteral("resource-groups"));
            me.appendChild(ge);
            QListIterator<ResourceGroup*> git(m_resourceGroups);
            while (git.hasNext()) {
                git.next()->save(ge);
            }
        }
        debugPlanXml<<"resources:"<<m_resources.count();
        if (!m_resources.isEmpty()) {
            QDomElement re = me.ownerDocument().createElement(QStringLiteral("resources"));
            me.appendChild(re);
            QListIterator<Resource*> rit(m_resources);
            while (rit.hasNext()) {
                rit.next()->save(re);
            }
        }
        debugPlanXml<<"resource-group-relations";
        if (!m_resources.isEmpty() && !m_resourceGroups.isEmpty()) {
            QDomElement e = me.ownerDocument().createElement(QStringLiteral("resource-group-relations"));
            me.appendChild(e);
            for (ResourceGroup *g : std::as_const(m_resourceGroups)) {
                const auto resources = g->resources();
                for (Resource *r : resources) {
                    QDomElement re = e.ownerDocument().createElement(QStringLiteral("resource-group-relation"));
                    e.appendChild(re);
                    re.setAttribute(QStringLiteral("group-id"), g->id());
                    re.setAttribute(QStringLiteral("resource-id"), r->id());
                }
            }
        }
        debugPlanXml<<"required-resources";
        if (m_resources.count() > 1) {
            QList<std::pair<QString, QString> > requiredList;
            for (Resource *resource : std::as_const(m_resources)) {
                const auto requiredIds = resource->requiredIds();
                for (const QString &required : requiredIds) {
                    requiredList << std::pair<QString, QString>(resource->id(), required);
                }
            }
            if (!requiredList.isEmpty()) {
                QDomElement e = me.ownerDocument().createElement(QStringLiteral("required-resources"));
                me.appendChild(e);
                for (const std::pair<QString, QString> &pair : std::as_const(requiredList)) {
                    QDomElement re = e.ownerDocument().createElement(QStringLiteral("required-resource"));
                    e.appendChild(re);
                    re.setAttribute(QStringLiteral("resource-id"), pair.first);
                    re.setAttribute(QStringLiteral("required-id"), pair.second);
                }
            }
        }
        // save resource teams
        debugPlanXml<<"resource-teams";
        QDomElement el = me.ownerDocument().createElement(QStringLiteral("resource-teams"));
        me.appendChild(el);
        for (Resource *r : std::as_const(m_resources)) {
            if (r->type() != Resource::Type_Team) {
                continue;
            }
            const auto ids = r->teamMemberIds();
            for (const QString &id : ids) {
                QDomElement e = el.ownerDocument().createElement(QStringLiteral("team"));
                el.appendChild(e);
                e.setAttribute(QStringLiteral("team-id"), r->id());
                e.setAttribute(QStringLiteral("member-id"), id);
            }
        }
        // save resource usage in other projects
        QList<Resource*> externals;
        for (Resource *resource : m_resources) {
            if (!resource->externalAppointmentList().isEmpty()) {
                externals << resource;
            }
        }
        debugPlanXml<<"external-appointments"<<externals.count();
        if (!externals.isEmpty()) {
            QDomElement e = me.ownerDocument().createElement(QStringLiteral("external-appointments"));
            me.appendChild(e);
            for (Resource *resource : std::as_const(externals)) {
                const QMap<QString, QString> projects = resource->externalProjects();
                QMap<QString, QString>::const_iterator it;
                for (it = projects.constBegin(); it != projects.constEnd(); ++it) {
                    QDomElement re = e.ownerDocument().createElement(QStringLiteral("external-appointment"));
                    e.appendChild(re);
                    re.setAttribute(QStringLiteral("resource-id"), resource->id());
                    re.setAttribute(QStringLiteral("project-id"), it.key());
                    re.setAttribute(QStringLiteral("project-name"), it.value());
                    resource->externalAppointments(it.key()).saveXML(e);
                }
            }
        }
        debugPlanXml<<"tasks:"<<numChildren();
        if (numChildren() > 0) {
            QDomElement e = me.ownerDocument().createElement(QStringLiteral("tasks"));
            me.appendChild(e);
            for (int i = 0; i < numChildren(); i++) {
                childNode(i)->save(e, context);
            }
        }
        // Now we can save relations assuming no tasks have relations outside the project
        QDomElement deps = me.ownerDocument().createElement(QStringLiteral("relations"));
        me.appendChild(deps);
        QListIterator<Node*> nodes(m_nodes);
        while (nodes.hasNext()) {
            const Node *n = nodes.next();
            n->saveRelations(deps, context);
        }
        debugPlanXml<<"task relations:"<<deps.childNodes().count();
        if (!deps.hasChildNodes()) {
            me.removeChild(deps);
        }
        debugPlanXml<<"project-schedules:"<<m_managers.count();
        if (!m_managers.isEmpty()) {
            QDomElement el = me.ownerDocument().createElement(QStringLiteral("project-schedules"));
            me.appendChild(el);
            for (ScheduleManager *sm : std::as_const(m_managers)) {
                sm->saveXML(el);
            }
        }
        // save resource requests
        QMultiHash<Task*, ResourceRequest*> resources;
        const auto tasks = allTasks();
        for (Task *task : tasks) {
            const auto requests = task->requests().resourceRequests(false);
            for (ResourceRequest *rr : requests) {
                resources.insert(task, rr);
            }
        }
        QMultiHash<Task*, std::pair<ResourceRequest*, Resource*> > required; // QHash<Task*, std::pair<ResourceRequest*, Required*>>
        QMultiHash<Task*, std::pair<ResourceRequest*, ResourceRequest*> > alternativeRequests; // QHash<Task*, std::pair<ResourceRequest*, Alternative*>>
        debugPlanXml<<"resource-requests:"<<resources.count();
        if (!resources.isEmpty()) {
            QDomElement el = me.ownerDocument().createElement(QStringLiteral("resource-requests"));
            me.appendChild(el);
            for (auto it = resources.constBegin(); it != resources.constEnd(); ++it) {
                if (!it.value()->resource()) {
                    continue;
                }
                QDomElement re = el.ownerDocument().createElement(QStringLiteral("resource-request"));
                el.appendChild(re);
                re.setAttribute(QStringLiteral("request-id"), it.value()->id());
                re.setAttribute(QStringLiteral("task-id"), it.key()->id());
                re.setAttribute(QStringLiteral("resource-id"), it.value()->resource()->id());
                re.setAttribute(QStringLiteral("units"), QString::number(it.value()->units()));
                // collect required resources and alternative requests
                const auto requiredResources = it.value()->requiredResources();
                for (Resource *r : requiredResources) {
                    required.insert(it.key(), std::pair<ResourceRequest*, Resource*>(it.value(), r));
                }
                const auto altRequests = it.value()->alternativeRequests();
                for (ResourceRequest *r : altRequests) {
                    alternativeRequests.insert(it.key(), std::pair<ResourceRequest*, ResourceRequest*>(it.value(), r));
                }
            }
        }
        debugPlanXml<<"required-resource-requests:"<<required.count();
        if (!required.isEmpty()) {
            QDomElement reqs = me.ownerDocument().createElement(QStringLiteral("required-resource-requests"));
            me.appendChild(reqs);
            for (auto it = required.constBegin(); it != required.constEnd(); ++it) {
                QDomElement req = reqs.ownerDocument().createElement(QStringLiteral("required-resource-request"));
                reqs.appendChild(req);
                req.setAttribute(QStringLiteral("task-id"), it.key()->id());
                req.setAttribute(QStringLiteral("request-id"), it.value().first->id());
                req.setAttribute(QStringLiteral("required-id"), it.value().second->id());
            }
        }
        debugPlanXml<<"alternative-requests:"<<alternativeRequests.count();
        if (!alternativeRequests.isEmpty()) {
            QDomElement reqs = me.ownerDocument().createElement(QStringLiteral("alternative-requests"));
            me.appendChild(reqs);
            for (auto it = alternativeRequests.constBegin(); it != alternativeRequests.constEnd(); ++it) {
                QDomElement req = reqs.ownerDocument().createElement(QStringLiteral("alternative-request"));
                reqs.appendChild(req);
                req.setAttribute(QStringLiteral("task-id"), it.key()->id());
                req.setAttribute(QStringLiteral("request-id"), it.value().first->id());
                req.setAttribute(QStringLiteral("resource-id"), it.value().second->resource()->id());
                req.setAttribute(QStringLiteral("units"), it.value().second->units());
            }
        }
    }
}

void Project::saveWorkPackageXML(QDomElement &element, const Node *node, long id) const
{
    QDomElement me = element.ownerDocument().createElement(QStringLiteral("project"));
    element.appendChild(me);

    me.setAttribute(QStringLiteral("name"), m_name);
    me.setAttribute(QStringLiteral("leader"), m_leader);
    me.setAttribute(QStringLiteral("id"), m_id);
    me.setAttribute(QStringLiteral("description"), m_description);
    me.setAttribute(QStringLiteral("timezone"), m_timeZone.isValid() ? QString::fromLatin1(m_timeZone.id()) : QString());

    me.setAttribute(QStringLiteral("scheduling"), constraintToString());
    me.setAttribute(QStringLiteral("start-time"), m_constraintStartTime.toString(Qt::ISODate));
    me.setAttribute(QStringLiteral("end-time"), m_constraintEndTime.toString(Qt::ISODate));

    if (node == nullptr) {
        return;
    }
    const auto assignedResources = node->assignedResources(id);
    debugPlanWp<<"save resources"<<assignedResources;
    if (!assignedResources.isEmpty()) {
        auto resourcesElemente = me.ownerDocument().createElement(QStringLiteral("resources"));
        me.appendChild(resourcesElemente);
        for (const auto r : std::as_const(m_resources)) {
            if (assignedResources.contains(r)) {
                r->save(resourcesElemente);
            }
        }
    }
    auto tasksElement = element.ownerDocument().createElement(QStringLiteral("tasks"));
    me.appendChild(tasksElement);

    node->saveWorkPackageXML(tasksElement, id);

    const auto managers = m_managerIdMap.values();
    for (ScheduleManager *sm : managers) {
        if (sm->scheduleId() == id) {
            QDomElement el = me.ownerDocument().createElement(QStringLiteral("project-schedules"));
            me.appendChild(el);
            sm->saveWorkPackageXML(el, *node);
            break;
        }
    }
}

void Project::setParentSchedule(Schedule *sch)
{
    QListIterator<Node*> it = m_nodes;
    while (it.hasNext()) {
        it.next() ->setParentSchedule(sch);
    }
}

void Project::addResourceGroup(ResourceGroup *group, ResourceGroup *parent, int index)
{
    if (parent) {
        setResourceGroupId(group);
        parent->addChildGroup(group, index);
    } else {
        Q_ASSERT(!m_resourceGroups.contains(group));
        int i = index == -1 ? m_resourceGroups.count() : index;
        Q_EMIT resourceGroupToBeAdded(this, nullptr, i);
        m_resourceGroups.insert(i, group);
        setResourceGroupId(group);
        group->setProject(this);
        const auto resources = group->resources();
        for (Resource *r : resources) {
            setResourceId(r);
            r->setProject(this);
        }
        Q_EMIT resourceGroupAdded(group);
    }
    Q_EMIT projectChanged();
}

void Project::takeResourceGroup(ResourceGroup *group)
{
    ResourceGroup *parent = group->parentGroup();
    if (parent) {
        removeResourceGroupId(group->id(), group);
        parent->removeChildGroup(group);
        const auto resources = group->resources();
        for (Resource *r : resources) {
            r->removeParentGroup(group);
        }
    } else {
        removeResourceGroupId(group->id());
        int i = m_resourceGroups.indexOf(group);
        if (i == -1) {
            return;
        }
        Q_EMIT resourceGroupToBeRemoved(this, nullptr, i, group);
        ResourceGroup *g = m_resourceGroups.takeAt(i);
        Q_ASSERT(group == g);
        g->setProject(nullptr);
        const auto resources = g->resources();
        for (Resource *r : resources) {
            r->removeParentGroup(g);
        }
        Q_EMIT resourceGroupRemoved();
    }
    Q_EMIT projectChanged();
}

const QList<ResourceGroup*> &Project::resourceGroups() const
{
    return m_resourceGroups;
}

void collectGroups(ResourceGroup *parent, QList<ResourceGroup*> &groups)
{
    groups << parent;
    const auto children = parent->childGroups();
    for (auto child : children) {
        collectGroups(child, groups);
    }
}

QList<ResourceGroup*> Project::allResourceGroups(bool sorted) const
{
    if (sorted) {
        QList<ResourceGroup*> groups;
        for (auto g : m_resourceGroups) {
            collectGroups(g, groups);
        }
        return groups;
    }
    return resourceGroupIdDict.values();
}

void Project::removeResourceGroupId(const QString &id, ResourceGroup *group)
{
    if (resourceGroupIdDict.contains(id)) {
        resourceGroupIdDict.remove(id);
    }
    if (group) {
        const auto  childGroups = group->childGroups();
        for (ResourceGroup *g : childGroups) {
            removeResourceGroupId(g->id(), g);
        }
    }
    return;
}

void Project::insertResourceGroupId(const QString &id, ResourceGroup* group)
{
    resourceGroupIdDict.insert(id, group);
    const auto  childGroups = group->childGroups();
    for (ResourceGroup *g : childGroups) {
        insertResourceGroupId(g->id(), g);
    }
}


void Project::addResource(Resource *resource, int index)
{
    int i = index == -1 ? m_resources.count() : index;
    Q_EMIT resourceToBeAdded(this, i);
    setResourceId(resource);
    m_resources.insert(i, resource);
    resource->setProject(this);
    Q_EMIT resourceAdded(resource);
    Q_EMIT projectChanged();
}

bool Project::takeResource(Resource *resource)
{
    int index = m_resources.indexOf(resource);
    Q_EMIT resourceToBeRemoved(this, index, resource);
    bool result = removeResourceId(resource->id());
    Q_ASSERT(result == true);
    if (!result) {
        warnPlan << "Could not remove resource with id" << resource->id();
    }
    resource->removeRequests(); // not valid anymore
    const auto parentGroups = resource->parentGroups();
    for (ResourceGroup *g : parentGroups) {
        g->takeResource(resource);
    }
    bool rem = m_resources.removeOne(resource);
    Q_ASSERT(!m_resources.contains(resource));
    Q_EMIT resourceRemoved();
    Q_EMIT projectChanged();
    return rem;
}

void Project::moveResource(ResourceGroup *group, Resource *resource)
{
    if (resource->parentGroups().contains(group)) {
        return;
    }
    takeResource(resource);
    addResource(resource);
    group->addResource(resource);
    return;
}

void Project::removeResourceFromProject(Resource *resource)
{
    removeResourceId(resource->id());
    resource->removeRequests();
    m_resources.removeAll(resource);
    resource->setProject(nullptr);
}

QMap< QString, QString > Project::externalProjects() const
{
    QMap< QString, QString > map;
    const auto resources = resourceList();
    for (Resource *r : resources) {
        for(QMapIterator<QString, QString> it(r->externalProjects()); it.hasNext();) {
            it.next();
            if (! map.contains(it.key())) {
                map[ it.key() ] = it.value();
            }
        }
    }
    return map;
}

bool Project::addTask(Node* task, Node* position)
{
    // we want to add a task at the given position. => the new node will
    // become next sibling right after position.
    if (nullptr == position) {
        return addSubTask(task, this);
    }
    //debugPlan<<"Add"<<task->name()<<" after"<<position->name();
    // in case we want to add to the main project, we make it child element
    // of the root element.
    if (Node::Type_Project == position->type()) {
        return addSubTask(task, position);
    }
    // find the position
    // we have to tell the parent that we want to delete one of its children
    Node* parentNode = position->parentNode();
    if (!parentNode) {
        debugPlan <<"parent node not found???";
        return false;
    }
    int index = parentNode->findChildNode(position);
    if (-1 == index) {
        // ok, it does not exist
        debugPlan <<"Task not found???";
        return false;
    }
    return addSubTask(task, index + 1, parentNode);
}

bool Project::addSubTask(Node* task, Node* parent)
{
    // append task to parent
    return addSubTask(task, -1, parent);
}

bool Project::addSubTask(Node* task, int index, Node* parent, bool emitSignal)
{
    // we want to add a subtask to the node "parent" at the given index.
    // If parent is 0, add to this
    Node *p = parent;
    if (nullptr == p) {
        p = this;
    }
    if (task->id().isEmpty()) {
        task->setId(uniqueNodeId());
    }
    if (!registerNodeId(task)) {
        errorPlan << "Failed to register node id, can not add subtask: " << task->name();
        return false;
    }
    int i = index == -1 ? p->numChildren() : index;
    if (emitSignal) Q_EMIT nodeToBeAdded(p, i);
    p->insertChildNode(i, task);
    connect(this, &Project::standardWorktimeChanged, task, &Node::slotStandardWorktimeChanged);
    if (emitSignal) {
        Q_EMIT nodeAdded(task);
        Q_EMIT projectChanged();
        if (p != this && p->numChildren() == 1) {
            Q_EMIT nodeChanged(p, TypeProperty);
        }
    }
    return true;
}

void Project::takeTask(Node *node, bool emitSignal)
{
    //debugPlan<<node->name();
    Node * parent = node->parentNode();
    if (parent == nullptr) {
        debugPlan <<"Node must have a parent!";
        return;
    }
    removeId(node->id());
    if (emitSignal) Q_EMIT nodeToBeRemoved(node);
    disconnect(this, &Project::standardWorktimeChanged, node, &Node::slotStandardWorktimeChanged);
    parent->takeChildNode(node);
    if (emitSignal) {
        Q_EMIT nodeRemoved(node);
        Q_EMIT projectChanged();
        if (parent != this && parent->type() != Node::Type_Summarytask) {
            Q_EMIT nodeChanged(parent, TypeProperty);
        }
    }
}

bool Project::canMoveTask(Node* node, Node *newParent, bool checkBaselined)
{
    //debugPlan<<node->name()<<" to"<<newParent->name();
    if (node == nullptr || newParent == nullptr) {
        return false;
    }
    if (node == this) {
        return false;
    }
    if (checkBaselined) {
        if (node->isBaselined() || (newParent->type() != Node::Type_Summarytask && newParent->isBaselined())) {
            return false;
        }
    }
    Node *p = newParent;
    while (p && p != this) {
        if (! node->canMoveTo(p)) {
            return false;
        }
        p = p->parentNode();
    }
    return true;
}

bool Project::moveTask(Node* node, Node *newParent, int newPos)
{
    //debugPlan<<node->name()<<" to"<<newParent->name()<<","<<newPos;
    if (! canMoveTask(node, newParent)) {
        return false;
    }
    Node *oldParent = node->parentNode();
    int oldPos = oldParent->indexOf(node);
    int i = newPos < 0 ? newParent->numChildren() : newPos;
    if (oldParent == newParent && i == oldPos) {
        // no need to move to where it already is
        return false;
    }
    int newRow = i;
    if (oldParent == newParent && newPos > oldPos) {
        ++newRow; // itemmodels wants new row *before* node is removed from old position
    }
    debugPlan<<node->name()<<"at"<<oldParent->indexOf(node)<<"to"<<newParent->name()<<i<<newRow<<"("<<newPos<<")";
    Q_EMIT nodeToBeMoved(node, oldPos, newParent, newRow);
    takeTask(node, false);
    if (newPos == -1) {
        i = newPos; // if inserted at the end (-1), update i since numChildren() could be modified and > to nb of children
    }
    addSubTask(node, i, newParent, false);
    Q_EMIT nodeMoved(node);
    if (oldParent != this && oldParent->numChildren() == 0) {
        Q_EMIT nodeChanged(oldParent, TypeProperty);
    }
    if (newParent != this && newParent->numChildren() == 1) {
        Q_EMIT nodeChanged(newParent, TypeProperty);
    }
    return true;
}

bool Project::canIndentTask(Node* node)
{
    if (nullptr == node) {
        return false;
    }
    if (node->type() == Node::Type_Project) {
        //debugPlan<<"The root node cannot be indented";
        return false;
    }
    // we have to find the parent of task to manipulate its list of children
    Node* parentNode = node->parentNode();
    if (!parentNode) {
        return false;
    }
    if (parentNode->findChildNode(node) == -1) {
        errorPlan << "Tasknot found???";
        return false;
    }
    Node *sib = node->siblingBefore();
    if (!sib) {
        //debugPlan<<"new parent node not found";
        return false;
    }
    if (node->findParentRelation(sib) || node->findChildRelation(sib)) {
        //debugPlan<<"Cannot have relations to parent";
        return false;
    }
    return true;
}

bool Project::indentTask(Node* node, int index)
{
    if (canIndentTask(node)) {
        Node * newParent = node->siblingBefore();
        int i = index == -1 ? newParent->numChildren() : index;
        moveTask(node, newParent, i);
        //debugPlan;
        return true;
    }
    return false;
}

bool Project::canUnindentTask(Node* node)
{
    if (nullptr == node) {
        return false;
    }
    if (Node::Type_Project == node->type()) {
        //debugPlan<<"The root node cannot be unindented";
        return false;
    }
    // we have to find the parent of task to manipulate its list of children
    // and we need the parent's parent too
    Node* parentNode = node->parentNode();
    if (!parentNode) {
        return false;
    }
    Node* grandParentNode = parentNode->parentNode();
    if (!grandParentNode) {
        //debugPlan<<"This node already is at the top level";
        return false;
    }
    int index = parentNode->findChildNode(node);
    if (-1 == index) {
        errorPlan << "Tasknot found???";
        return false;
    }
    return true;
}

bool Project::unindentTask(Node* node)
{
    if (canUnindentTask(node)) {
        Node * parentNode = node->parentNode();
        Node *grandParentNode = parentNode->parentNode();
        int i = grandParentNode->indexOf(parentNode) + 1;
        if (i == 0)  {
            i = grandParentNode->numChildren();
        }
        moveTask(node, grandParentNode, i);
        //debugPlan;
        return true;
    }
    return false;
}

bool Project::canMoveTaskUp(Node* node)
{
    if (node == nullptr)
        return false; // safety
    // we have to find the parent of task to manipulate its list of children
    Node* parentNode = node->parentNode();
    if (!parentNode) {
        //debugPlan<<"No parent found";
        return false;
    }
    if (parentNode->findChildNode(node) == -1) {
        errorPlan << "Tasknot found???";
        return false;
    }
    if (node->siblingBefore()) {
        return true;
    }
    return false;
}

bool Project::moveTaskUp(Node* node)
{
    if (canMoveTaskUp(node)) {
        moveTask(node, node->parentNode(), node->parentNode()->indexOf(node) - 1);
        return true;
    }
    return false;
}

bool Project::canMoveTaskDown(Node* node)
{
    if (node == nullptr)
        return false; // safety
    // we have to find the parent of task to manipulate its list of children
    Node* parentNode = node->parentNode();
    if (!parentNode) {
        return false;
    }
    if (parentNode->findChildNode(node) == -1) {
        errorPlan << "Tasknot found???";
        return false;
    }
    if (node->siblingAfter()) {
        return true;
    }
    return false;
}

bool Project::moveTaskDown(Node* node)
{
    if (canMoveTaskDown(node)) {
        moveTask(node, node->parentNode(), node->parentNode()->indexOf(node) + 1);
        return true;
    }
    return false;
}

Task *Project::createTask()
{
    Task * node = new Task();
    node->setId(uniqueNodeId());
    reserveId(node->id(), node);
    return node;
}

Task *Project::createTask(const Task &def)
{
    Task * node = new Task(def);
    node->setId(uniqueNodeId());
    reserveId(node->id(), node);
    return node;
}

void Project::allocateDefaultResources(Task *task) const
{
    for (const auto r : std::as_const(m_resources)) {
        if (r->autoAllocate()) {
            task->requests().addResourceRequest(new ResourceRequest(r, 100));
        }
    }
}

Node *Project::findNode(const QString &id) const
{
    if (m_parent == nullptr) {
        if (nodeIdDict.contains(id)) {
            return nodeIdDict[ id ];
        }
        return nullptr;
    }
    return m_parent->findNode(id);
}

bool Project::nodeIdentExists(const QString &id) const
{
    return nodeIdDict.contains(id) || nodeIdReserved.contains(id);
}

QString Project::uniqueNodeId(int seed) const
{
    Q_UNUSED(seed);
    QString ident = generateId();
    while (nodeIdentExists(ident)) {
        ident = generateId();
    }
    return ident;
}

QString Project::uniqueNodeId(const QList<QString> &existingIds, int seed)
{
    QString id = uniqueNodeId(seed);
    while (existingIds.contains(id)) {
        id = uniqueNodeId(seed);
    }
    return id;
}

bool Project::removeId(const QString &id)
{
    //debugPlan <<"id=" << id;
    if (m_parent) {
        return m_parent->removeId(id);
    }
    //debugPlan << "id=" << id<< nodeIdDict.contains(id);
    return nodeIdDict.remove(id);
}

void Project::reserveId(const QString &id, Node *node)
{
    //debugPlan <<"id=" << id << node->name();
    nodeIdReserved.insert(id, node);
}

bool Project::registerNodeId(Node *node)
{
    nodeIdReserved.remove(node->id());
    if (node->id().isEmpty()) {
        warnPlan << "Node id is empty, cannot register it";
        return false;
    }
    Node *rn = findNode(node->id());
    if (rn == nullptr) {
        //debugPlan <<"id=" << node->id() << node->name();
        nodeIdDict.insert(node->id(), node);
        connect(node, &QObject::destroyed, this, &Project::nodeDestroyed, Qt::QueuedConnection);
        return true;
    }
    if (rn != node) {
        errorPlan << node << node->id()<<"id already exists for different task: " << rn;
        return false;
    }
    //debugPlan<<"Already exists" <<"id=" << node->id() << node->name();
    return true;
}

QList<Node*> Project::allNodes(bool ordered, Node* parent) const
{
    QList<Node*> lst;
    if (ordered) {
        const Node *p = parent ? parent : this;
        const auto nodes = p->childNodeIterator();
        for (Node *n : nodes) {
            if (n->type() == Node::Type_Task || n->type() == Type_Milestone || n->type() == Node::Type_Summarytask) {
                lst << static_cast<Task*>(n);
                lst += allNodes(ordered, n);
            }
        }
    } else {
        lst = nodeIdDict.values();
        int me = lst.indexOf(const_cast<Project*>(this));
        if (me != -1) {
            lst.removeAt(me);
        }
    }
    return lst;
}

QList<Task*> Project::allTasks(const Node *parent) const
{
    QList<Task*> lst;
    const Node *p = parent ? parent : this;
    const auto nodes = p->childNodeIterator();
    for (Node *n : nodes) {
        if (n->type() == Node::Type_Task || n->type() == Type_Milestone) {
            lst << static_cast<Task*>(n);
        }
        lst += allTasks(n);
    }
    return lst;
}

QList<Node*> Project::leafNodes() const
{
    QList<Node*> lst;
    for (const auto n : std::as_const(nodeIdDict)) {
        if (n != this && n->numChildren() == 0) {
            lst.append(n);
        }
    }
    return lst;
}

bool Project::isStarted() const
{
    const QList<Task*> tasks = allTasks();
    for (const Task *t : tasks) {
        if (t->isStarted()) {
            return true;
        }
    }
    return false;
}

bool Project::setResourceGroupId(ResourceGroup *group)
{
    if (group == nullptr) {
        return false;
    }
    if (! group->id().isEmpty()) {
        ResourceGroup *g = findResourceGroup(group->id());
        if (group == g) {
            return true;
        } else if (g == nullptr) {
            insertResourceGroupId(group->id(), group);
            return true;
        }
    }
    QString id = uniqueResourceGroupId();
    group->setId(id);
    if (id.isEmpty()) {
        return false;
    }
    insertResourceGroupId(id, group);
    return true;
}

QString Project::uniqueResourceGroupId() const {
    QString id = generateId();
    while (resourceGroupIdDict.contains(id)) {
        id = generateId();
    }
    return id;
}

ResourceGroup *Project::group(const QString& id)
{
    return findResourceGroup(id);
}

ResourceGroup *Project::groupByName(const QString& name) const
{
    const auto groups = resourceGroupIdDict.values();
    for (ResourceGroup *g : groups) {
        if (g->name() == name) {
            return g;
        }
    }
    return nullptr;
}

void Project::removeResourceGroupFromProject(ResourceGroup *group)
{
    removeResourceGroupId(group->id());
    m_resourceGroups.removeAll(group);
    group->setProject(nullptr);
}

QList<Resource*> Project::autoAllocateResources() const
{
    QList<Resource*> lst;
    for (Resource *r : std::as_const(m_resources)) {
        if (r->autoAllocate()) {
            lst << r;
        }
    }
    return lst;
}

void Project::insertResourceId(const QString &id, Resource *resource)
{
    resourceIdDict.insert(id, resource);
}

bool Project::removeResourceId(const QString &id)
{
    return resourceIdDict.remove(id);
}

bool Project::setResourceId(Resource *resource)
{
    if (resource == nullptr) {
        return false;
    }
    if (! resource->id().isEmpty()) {
        Resource *r = findResource(resource->id());
        if (resource == r) {
            return true;
        } else if (r == nullptr) {
            insertResourceId(resource->id(), resource);
            return true;
        }
    }
    QString id = uniqueResourceId();
    resource->setId(id);
    if (id.isEmpty()) {
        return false;
    }
    insertResourceId(id, resource);
    return true;
}

QString Project::uniqueResourceId() const {
    QString id = generateId();
    while (resourceIdDict.contains(id)) {
        id = generateId();
    }
    return id;
}

Resource *Project::resource(const QString& id)
{
    return findResource(id);
}

Resource *Project::resourceByName(const QString& name) const
{
    QHash<QString, Resource*>::const_iterator it;
    for (it = resourceIdDict.constBegin(); it != resourceIdDict.constEnd(); ++it) {
        Resource *r = it.value();
        if (r->name() == name) {
            Q_ASSERT(it.key() == r->id());
            return r;
        }
    }
    return nullptr;
}

QStringList Project::resourceNameList() const
{
    QStringList lst;
    for (Resource *r : m_resources) {
        lst << r->name();
    }
    return lst;
}

int Project::indexOf(Resource *resource) const
{
    return m_resources.indexOf(resource);
}

int Project::resourceCount() const
{
    return m_resources.count();
}

Resource *Project::resourceAt(int pos) const
{
    return m_resources.value(pos);
}

EffortCostMap Project::plannedEffortCostPrDay(QDate  start, QDate end, long id, EffortCostCalculationType typ) const
{
    //debugPlan<<start<<end<<id;
    Schedule *s = schedule(id);
    if (s == nullptr) {
        return EffortCostMap();
    }
    EffortCostMap ec;
    QListIterator<Node*> it(childNodeIterator());
    while (it.hasNext()) {
        ec += it.next() ->plannedEffortCostPrDay(start, end, id, typ);
    }
    return ec;
}

EffortCostMap Project::plannedEffortCostPrDay(const Resource *resource, QDate  start, QDate end, long id, EffortCostCalculationType typ) const
{
    //debugPlan<<start<<end<<id;
    EffortCostMap ec;
    QListIterator<Node*> it(childNodeIterator());
    while (it.hasNext()) {
        ec += it.next() ->plannedEffortCostPrDay(resource, start, end, id, typ);
    }
    return ec;
}

EffortCostMap Project::actualEffortCostPrDay(QDate  start, QDate end, long id, EffortCostCalculationType typ) const
{
    //debugPlan<<start<<end<<id;
    EffortCostMap ec;
    QListIterator<Node*> it(childNodeIterator());
    while (it.hasNext()) {
        ec += it.next() ->actualEffortCostPrDay(start, end, id, typ);
    }
    return ec;
}

EffortCostMap Project::actualEffortCostPrDay(const Resource *resource, QDate  start, QDate end, long id,  EffortCostCalculationType typ) const
{
    //debugPlan<<start<<end<<id;
    EffortCostMap ec;
    QListIterator<Node*> it(childNodeIterator());
    while (it.hasNext()) {
        ec += it.next() ->actualEffortCostPrDay(resource, start, end, id, typ);
    }
    return ec;
}

// Returns the total planned effort for this project (or subproject)
Duration Project::plannedEffort(long id, EffortCostCalculationType typ) const
{
    //debugPlan;
    Duration eff;
    QListIterator<Node*> it(childNodeIterator());
    while (it.hasNext()) {
        eff += it.next() ->plannedEffort(id, typ);
    }
    return eff;
}

// Returns the total planned effort for this project (or subproject) on date
Duration Project::plannedEffort(QDate date, long id, EffortCostCalculationType typ) const
{
    //debugPlan;
    Duration eff;
    QListIterator<Node*> it(childNodeIterator());
    while (it.hasNext()) {
        eff += it.next() ->plannedEffort(date, id, typ);
    }
    return eff;
}

// Returns the total planned effort for this project (or subproject) upto and including date
Duration Project::plannedEffortTo(QDate date, long id, EffortCostCalculationType typ) const
{
    //debugPlan;
    Duration eff;
    QListIterator<Node*> it(childNodeIterator());
    while (it.hasNext()) {
        eff += it.next() ->plannedEffortTo(date, id, typ);
    }
    return eff;
}

// Returns the total actual effort for this project (or subproject) upto and including date
Duration Project::actualEffortTo(QDate date) const
{
    //debugPlan;
    Duration eff;
    QListIterator
    <Node*> it(childNodeIterator());
    while (it.hasNext()) {
        eff += it.next() ->actualEffortTo(date);
    }
    return eff;
}

// Returns the total planned effort for this project (or subproject) upto and including date
double Project::plannedCostTo(QDate date, long id, EffortCostCalculationType typ) const
{
    //debugPlan;
    double c = 0;
    QListIterator
    <Node*> it(childNodeIterator());
    while (it.hasNext()) {
        c += it.next() ->plannedCostTo(date, id, typ);
    }
    return c;
}

// Returns the total actual cost for this project (or subproject) upto and including date
EffortCost Project::actualCostTo(long int id, QDate date) const
{
    //debugPlan;
    EffortCost c;
    QListIterator<Node*> it(childNodeIterator());
    while (it.hasNext()) {
        c += it.next() ->actualCostTo(id, date);
    }
    return c;
}

Duration Project::budgetedWorkPerformed(QDate date, long id) const
{
    //debugPlan;
    Duration e;
    const auto nodes = childNodeIterator();
    for (Node *n : nodes) {
        e += n->budgetedWorkPerformed(date, id);
    }
    return e;
}

double Project::budgetedCostPerformed(QDate date, long id) const
{
    //debugPlan;
    double c = 0.0;
    const auto nodes = childNodeIterator();
    for (Node *n : nodes) {
        c += n->budgetedCostPerformed(date, id);
    }
    return c;
}

double Project::effortPerformanceIndex(QDate date, long id) const
{
    //debugPlan;
    debugPlan<<date<<id;
    Duration b = budgetedWorkPerformed(date, id);
    if (b == Duration::zeroDuration) {
        return 1.0;
    }
    Duration a = actualEffortTo(date);
    if (b == Duration::zeroDuration) {
        return 1.0;
    }
    return b.toDouble() / a.toDouble();
}

double Project::schedulePerformanceIndex(QDate date, long id) const
{
    //debugPlan;
    double r = 1.0;
    double s = bcws(date, id);
    double p = bcwp(date, id);
    if (s > 0.0) {
        r = p / s;
    }
    debugPlan<<s<<p<<r;
    return r;
}

double Project::bcws(QDate date, long id) const
{
    //debugPlan;
    double c = plannedCostTo(date, id, ECCT_EffortWork);
    debugPlan<<c;
    return c;
}

double Project::bcwp(long id) const
{
    QDate date = QDate::currentDate();
    return bcwp(date, id);
}

double Project::bcwp(QDate date, long id) const
{
    debugPlan<<date<<id;
    QDate start = startTime(id).date();
    QDate end = endTime(id).date();
    EffortCostMap plan = plannedEffortCostPrDay(start, end, id, ECCT_EffortWork);
    EffortCostMap actual = actualEffortCostPrDay(start, (end > date ? end : date), id);

    double budgetAtCompletion;
    double plannedCompleted;
    double budgetedCompleted;
    bool useEffort = false; //FIXME
    if (useEffort) {
        budgetAtCompletion = plan.totalEffort().toDouble(Duration::Unit_h);
        plannedCompleted = plan.effortTo(date).toDouble(Duration::Unit_h);
        //actualCompleted = actual.effortTo(date).toDouble(Duration::Unit_h);
        budgetedCompleted = budgetedWorkPerformed(date, id).toDouble(Duration::Unit_h);
    } else {
        budgetAtCompletion = plan.totalCost();
        plannedCompleted = plan.costTo(date);
        budgetedCompleted = budgetedCostPerformed(date, id);
    }
    double c = 0.0;
    if (budgetAtCompletion > 0.0) {
        double percentageCompletion = budgetedCompleted / budgetAtCompletion;
        c = budgetAtCompletion * percentageCompletion; //??
        debugPlan<<percentageCompletion<<budgetAtCompletion<<budgetedCompleted<<plannedCompleted;
    }
    return c;
}

void Project::addCalendar(Calendar *calendar, Calendar *parent, int index)
{
    Q_ASSERT(calendar != nullptr);
    //debugPlan<<calendar->name()<<","<<(parent?parent->name():"No parent");
    int row = parent == nullptr ? m_calendars.count() : parent->calendars().count();
    if (index >= 0 && index < row) {
        row = index;
    }
    Q_EMIT calendarToBeAdded(parent, row);
    calendar->setProject(this);
    if (parent == nullptr) {
        calendar->setParentCal(nullptr); // in case
        m_calendars.insert(row, calendar);
    } else {
        calendar->setParentCal(parent, row);
    }
    if (calendar->isDefault()) {
        setDefaultCalendar(calendar);
    }
    setCalendarId(calendar);
    Q_EMIT calendarAdded(calendar);
    Q_EMIT projectChanged();
}

void Project::takeCalendar(Calendar *calendar)
{
    Q_EMIT calendarToBeRemoved(calendar);
    removeCalendarId(calendar->id());
    if (calendar == m_defaultCalendar) {
        m_defaultCalendar = nullptr;
    }
    if (calendar->parentCal() == nullptr) {
        int i = indexOf(calendar);
        if (i != -1) {
            m_calendars.removeAt(i);
        }
    } else {
        calendar->setParentCal(nullptr);
    }
    Q_EMIT calendarRemoved(calendar);
    calendar->setProject(nullptr);
    Q_EMIT projectChanged();
}

int Project::indexOf(const Calendar *calendar) const
{
    return m_calendars.indexOf(const_cast<Calendar*>(calendar));
}

Calendar *Project::calendar(const QString& id) const
{
    return findCalendar(id);
}

Calendar *Project::calendarByName(const QString& name) const
{
    const auto calendars = calendarIdDict.values();
    for(Calendar *c : calendars) {
        if (c->name() == name) {
            return c;
        }
    }
    return nullptr;
}

const QList<Calendar*> &Project::calendars() const
{
    return m_calendars;
}

QList<Calendar*> Project::allCalendars() const
{
    return calendarIdDict.values();
}

QStringList Project::calendarNames() const
{
    QStringList lst;
    const auto calendars = calendarIdDict.values();
    for (Calendar *c : calendars) {
        lst << c->name();
    }
    return lst;
}

bool Project::setCalendarId(Calendar *calendar)
{
    if (calendar == nullptr) {
        return false;
    }
    if (! calendar->id().isEmpty()) {
        Calendar *c = findCalendar(calendar->id());
        if (calendar == c) {
            return true;
        } else if (c == nullptr) {
            insertCalendarId(calendar->id(), calendar);
            return true;
        }
    }
    QString id = uniqueCalendarId();
    calendar->setId(id);
    if (id.isEmpty()) {
        return false;
    }
    insertCalendarId(id, calendar);
    return true;
}

QString Project::uniqueCalendarId() const {
    QString id = generateId();
    while (calendarIdDict.contains(id)) {
        id = generateId();
    }
    return id;
}

void Project::setDefaultCalendar(Calendar *cal)
{
    if (m_defaultCalendar) {
        m_defaultCalendar->setDefault(false);
    }
    m_defaultCalendar = cal;
    if (cal) {
        cal->setDefault(true);
    }
    Q_EMIT defaultCalendarChanged(cal);
    Q_EMIT projectChanged();
}

void Project::setStandardWorktime(StandardWorktime * worktime)
{
    if (m_standardWorktime != worktime) {
        delete m_standardWorktime;
        m_standardWorktime = worktime;
        m_standardWorktime->setProject(this);
        Q_EMIT standardWorktimeChanged(worktime);
    }
}

void Project::emitDocumentAdded(Node *node , Document *doc , int index)
{
    Q_EMIT documentAdded(node, doc, index);
}

void Project::emitDocumentRemoved(Node *node , Document *doc , int index)
{
    Q_EMIT documentRemoved(node, doc, index);
}

void Project::emitDocumentChanged(Node *node , Document *doc , int index)
{
    Q_EMIT documentChanged(node, doc, index);
}

bool Project::linkExists(const Node *par, const Node *child) const
{
    if (par == nullptr || child == nullptr || par == child || par->isDependChildOf(child)) {
        return false;
    }
    const auto relations = par->dependChildNodes();
    for (Relation *r : relations) {
        if (r->child() == child) {
            return true;
        }
    }
    return false;
}

bool Project::legalToLink(const Node *par, const Node *child) const
{
    //debugPlan<<par.name()<<" ("<<par.numDependParentNodes()<<" parents)"<<child.name()<<" ("<<child.numDependChildNodes()<<" children)";

    if (par == nullptr || child == nullptr || par == child || par->isDependChildOf(child)) {
        return false;
    }
    if (linkExists(par, child)) {
        return false;
    }
    bool legal = true;
    // see if par/child is related
    if (legal && (par->isParentOf(child) || child->isParentOf(par))) {
        legal = false;
    }
    if (legal)
        legal = legalChildren(par, child);
    if (legal)
        legal = legalParents(par, child);

    if (legal) {
        const auto nodes = par->childNodeIterator();
        for (Node *p : nodes) {
            if (! legalToLink(p, child)) {
                return false;
            }
        }
    }
    return legal;
}

bool Project::legalParents(const Node *par, const Node *child) const
{
    bool legal = true;
    //debugPlan<<par->name()<<" ("<<par->numDependParentNodes()<<" parents)"<<child->name()<<" ("<<child->numDependChildNodes()<<" children)";
    for (int i = 0; i < par->numDependParentNodes() && legal; ++i) {
        Node *pNode = par->getDependParentNode(i) ->parent();
        if (child->isParentOf(pNode) || pNode->isParentOf(child)) {
            //debugPlan<<"Found:"<<pNode->name()<<" is related to"<<child->name();
            legal = false;
        } else {
            legal = legalChildren(pNode, child);
        }
        if (legal)
            legal = legalParents(pNode, child);
    }
    return legal;
}

bool Project::legalChildren(const Node *par, const Node *child) const
{
    bool legal = true;
    //debugPlan<<par->name()<<" ("<<par->numDependParentNodes()<<" parents)"<<child->name()<<" ("<<child->numDependChildNodes()<<" children)";
    for (int j = 0; j < child->numDependChildNodes() && legal; ++j) {
        Node *cNode = child->getDependChildNode(j) ->child();
        if (par->isParentOf(cNode) || cNode->isParentOf(par)) {
            //debugPlan<<"Found:"<<par->name()<<" is related to"<<cNode->name();
            legal = false;
        } else {
            legal = legalChildren(par, cNode);
        }
    }
    return legal;
}

WBSDefinition &Project::wbsDefinition()
{
    return m_wbsDefinition;
}

void Project::setWbsDefinition(const WBSDefinition &def)
{
    //debugPlan;
    m_wbsDefinition = def;
    Q_EMIT wbsDefinitionChanged();
    Q_EMIT projectChanged();
}

QString Project::generateWBSCode(QList<int> &indexes, bool sortable) const
{
    QString code = m_wbsDefinition.projectCode();
    if (sortable) {
        int fw = (nodeIdDict.count() / 10) + 1;
        QLatin1Char fc('0');
        for (int index : std::as_const(indexes)) {
            code += QStringLiteral(".%1");
            code = code.arg(QString::number(index), fw, fc);
        }
        debugPlan<<1<<code<<"------------------";
    } else {
        if (! code.isEmpty() && ! indexes.isEmpty()) {
            code += m_wbsDefinition.projectSeparator();
        }
        int level = 1;
        for (int index : std::as_const(indexes)) {
            code += m_wbsDefinition.code(index + 1, level);
            if (level < indexes.count()) {
                // not last level, add separator also
                code += m_wbsDefinition.separator(level);
            }
            ++level;
        }
        debugPlan<<2<<code<<"------------------";
    }
    //debugPlan<<code;
    return code;
}

void Project::setCurrentSchedule(long id)
{
    //debugPlan;
    setCurrentSchedulePtr(findSchedule(id));
    Node::setCurrentSchedule(id);
    for (Resource *r : std::as_const(m_resources)) {
        r->setCurrentSchedule(id);
    }
    Q_EMIT currentScheduleChanged();
    Q_EMIT projectChanged();
}

void Project::setCurrentScheduleManager(ScheduleManager *sm)
{
    m_currentScheduleManager = sm;
}
ScheduleManager *Project::currentScheduleManager() const
{
    return m_currentScheduleManager;
}

ScheduleManager *Project::scheduleManager(long id) const
{
    for (ScheduleManager *sm : std::as_const(m_managers)) {
        if (sm->scheduleId() == id) {
            return sm;
        }
    }
    return nullptr;
}

ScheduleManager *Project::scheduleManager(const QString &id) const
{
    return m_managerIdMap.value(id);
}

ScheduleManager *Project::findScheduleManagerByName(const QString &name) const
{
    //debugPlan;
    ScheduleManager *m = nullptr;
    for (ScheduleManager *sm : std::as_const(m_managers)) {
        m = sm->findManager(name);
        if (m) {
            break;
        }
    }
    return m;
}

QList<ScheduleManager*> Project::allScheduleManagers() const
{
    QList<ScheduleManager*> lst;
    for (ScheduleManager *sm : std::as_const(m_managers)) {
        lst << sm;
        lst << sm->allChildren();
    }
    return lst;
}

QString Project::uniqueScheduleName(const ScheduleManager *parent) const {
    //debugPlan;
    QString n = parent ? parent->name() : i18n("Plan");
    bool unique = findScheduleManagerByName(n) == nullptr;
    if (unique) {
        return n;
    }
    n += parent ? QStringLiteral(".%1") : QStringLiteral(" %1");
    int i = 1;
    for (; true; ++i) {
        if (!findScheduleManagerByName(n.arg(i))) {
            break;
        }
    }
    return n.arg(i);
}

void Project::addScheduleManager(ScheduleManager *sm, ScheduleManager *parent, int index)
{
    int row = parent == nullptr ? m_managers.count() : parent->childCount();
    if (index >= 0 && index < row) {
        row = index;
    }
    if (parent == nullptr) {
        Q_EMIT scheduleManagerToBeAdded(parent, row);
        m_managers.insert(row, sm);
    } else {
        Q_EMIT scheduleManagerToBeAdded(parent, row);
        sm->setParentManager(parent, row);
    }
    if (sm->managerId().isEmpty()) {
        sm->setManagerId(uniqueScheduleManagerId());
    }
    Q_ASSERT(! m_managerIdMap.contains(sm->managerId()));
    m_managerIdMap.insert(sm->managerId(), sm);

    Q_EMIT scheduleManagerAdded(sm);
    Q_EMIT projectChanged();
    //debugPlan<<"Added:"<<sm->name()<<", now"<<m_managers.count();
}

int Project::takeScheduleManager(ScheduleManager *sm)
{
    const auto managers = sm->children();
    for (ScheduleManager *s : managers) {
        takeScheduleManager(s);
    }
    if (sm->scheduling()) {
        sm->stopCalculation();
    }
    int index = -1;
    if (sm->parentManager()) {
        int index = sm->parentManager()->indexOf(sm);
        if (index >= 0) {
            Q_EMIT scheduleManagerToBeRemoved(sm);
            sm->setParentManager(nullptr);
            m_managerIdMap.remove(sm->managerId());
            Q_EMIT scheduleManagerRemoved(sm);
            Q_EMIT projectChanged();
        }
    } else {
        index = indexOf(sm);
        if (index >= 0) {
            Q_EMIT scheduleManagerToBeRemoved(sm);
            m_managers.removeAt(indexOf(sm));
            m_managerIdMap.remove(sm->managerId());
            Q_EMIT scheduleManagerRemoved(sm);
            Q_EMIT projectChanged();
        }
    }
    return index;
}

void Project::swapScheduleManagers(ScheduleManager *from, ScheduleManager *to)
{
    Q_EMIT scheduleManagersSwapped(from, to);
}

void Project::moveScheduleManager(ScheduleManager *sm, ScheduleManager *newparent, int newindex)
{
    //debugPlan<<sm->name()<<newparent<<newindex;
    Q_EMIT scheduleManagerToBeMoved(sm);
    if (! sm->parentManager()) {
        m_managers.removeAt(indexOf(sm));
    }
    sm->setParentManager(newparent, newindex);
    if (! newparent) {
        m_managers.insert(newindex, sm);
    }
    Q_EMIT scheduleManagerMoved(sm, newindex);
}

bool Project::isScheduleManager(void *ptr) const
{
    const ScheduleManager *sm = static_cast<ScheduleManager*>(ptr);
    if (indexOf(sm) >= 0) {
        return true;
    }
    for (ScheduleManager *p : std::as_const(m_managers)) {
        if (p->isParentOf(sm)) {
            return true;
        }
    }
    return false;
}

ScheduleManager *Project::createScheduleManager(const QString &name, const ScheduleManager::Owner &creator)
{
    Q_UNUSED(creator);
    //debugPlan<<name;
    ScheduleManager *sm = new ScheduleManager(*this, name);
    return sm;
}

ScheduleManager *Project::createScheduleManager(const ScheduleManager *parent)
{
    //debugPlan;
    return createScheduleManager(uniqueScheduleName(parent));
}

QString Project::uniqueScheduleManagerId() const
{
    QString ident = KRandom::randomString(10);
    while (m_managerIdMap.contains(ident)) {
        ident = KRandom::randomString(10);
    }
    return ident;
}

bool Project::isBaselined(long id) const
{
    if (id == ANYSCHEDULED) {
        const auto managers = allScheduleManagers();
        for (ScheduleManager *sm : managers) {
            if (sm->isBaselined()) {
                return true;
            }
        }
        return false;
    }
    Schedule *s = schedule(id);
    return s == nullptr ? false : s->isBaselined();
}

MainSchedule *Project::createSchedule(const QString& name, Schedule::Type type, int minId)
{
    //debugPlan<<"No of schedules:"<<m_schedules.count();
    MainSchedule *sch = new MainSchedule();
    sch->setName(name);
    sch->setType(type);
    addMainSchedule(sch, minId);
    return sch;
}

void Project::addMainSchedule(MainSchedule *sch, int minId)
{
    if (sch == nullptr) {
        return;
    }
    //debugPlan<<"No of schedules:"<<m_schedules.count();
    long i = std::max(minId, 1); // keep this positive (negative values are special...)
    while (m_schedules.contains(i)) {
        ++i;
    }
    sch->setId(i);
    sch->setNode(this);
    addSchedule(sch);
}

bool Project::removeCalendarId(const QString &id)
{
    //debugPlan <<"id=" << id;
    return calendarIdDict.remove(id);
}

void Project::insertCalendarId(const QString &id, Calendar *calendar)
{
    //debugPlan <<"id=" << id <<":" << calendar->name();
    calendarIdDict.insert(id, calendar);
}

void Project::changed(Node *node, int property)
{
    if (m_parent == nullptr) {
        Node::changed(node, property); // reset cache
        if (property != Node::TypeProperty) {
            // add/remove node is handled elsewhere
            Q_EMIT nodeChanged(node, property);
            Q_EMIT projectChanged();
        }
        return;
    }
    Node::changed(node, property);
}

void Project::changed(ResourceGroup *group)
{
    Q_UNUSED(group)
    //debugPlan;
    Q_EMIT projectChanged();
}

void Project::changed(ScheduleManager *sm, int property)
{
    Q_EMIT scheduleManagerChanged(sm, property);
    Q_EMIT projectChanged();
}

void Project::changed(MainSchedule *sch)
{
    //debugPlan<<sch->id();
    Q_EMIT scheduleChanged(sch);
    Q_EMIT projectChanged();
}

void Project::sendScheduleToBeAdded(const ScheduleManager *sm, int row)
{
    Q_EMIT scheduleToBeAdded(sm, row);
}

void Project::sendScheduleAdded(const MainSchedule *sch)
{
    //debugPlan<<sch->id();
    Q_EMIT scheduleAdded(sch);
    Q_EMIT projectChanged();
}

void Project::sendScheduleToBeRemoved(const MainSchedule *sch)
{
    //debugPlan<<sch->id();
    Q_EMIT scheduleToBeRemoved(sch);
}

void Project::sendScheduleRemoved(const MainSchedule *sch)
{
    //debugPlan<<sch->id();
    Q_EMIT scheduleRemoved(sch);
    Q_EMIT projectChanged();
}

void Project::changed(Resource *resource)
{
    Q_UNUSED(resource)
    Q_EMIT projectChanged();
}

void Project::changed(Calendar *cal)
{
    Q_EMIT calendarChanged(cal);
    Q_EMIT projectChanged();
}

void Project::changed(StandardWorktime *w)
{
    Q_EMIT standardWorktimeChanged(w);
    Q_EMIT projectChanged();
}

bool Project::addRelation(Relation *rel, bool check)
{
    if (rel->parent() == nullptr || rel->child() == nullptr) {
        return false;
    }
    if (check && !legalToLink(rel->parent(), rel->child())) {
        return false;
    }
    Q_EMIT relationToBeAdded(rel, rel->parent()->numDependChildNodes(), rel->child()->numDependParentNodes());
    rel->parent()->addDependChildNode(rel);
    rel->child()->addDependParentNode(rel);
    Q_EMIT relationAdded(rel);
    Q_EMIT projectChanged();
    return true;
}

void Project::takeRelation(Relation *rel)
{
    Q_EMIT relationToBeRemoved(rel);
    rel->parent() ->takeDependChildNode(rel);
    rel->child() ->takeDependParentNode(rel);
    Q_EMIT relationRemoved(rel);
    Q_EMIT projectChanged();
}

void Project::setRelationType(Relation *rel, Relation::Type type)
{
    Q_EMIT relationToBeModified(rel);
    rel->setType(type);
    Q_EMIT relationModified(rel);
    Q_EMIT projectChanged();
}

void Project::setRelationLag(Relation *rel, const Duration &lag)
{
    Q_EMIT relationToBeModified(rel);
    rel->setLag(lag);
    Q_EMIT relationModified(rel);
    Q_EMIT projectChanged();
}

QList<Node*> Project::flatNodeList(Node *parent)
{
    QList<Node*> lst;
    Node *p = parent == nullptr ? this : parent;
    //debugPlan<<p->name()<<lst.count();
    const auto nodes = p->childNodeIterator();
    for (Node *n : nodes) {
        lst.append(n);
        if (n->numChildren() > 0) {
            lst += flatNodeList(n);
        }
    }
    return lst;
}

void Project::setSchedulerPlugins(const QMap<QString, SchedulerPlugin*> &plugins)
{
    m_schedulerPlugins = plugins;
    debugPlan<<m_schedulerPlugins;
}

void Project::emitLocaleChanged()
{
    Q_EMIT localeChanged();
}

bool Project::useSharedResources() const
{
    return m_useSharedResources;
}

void Project::setUseSharedResources(bool on)
{
    m_useSharedResources = on;
}

bool Project::isSharedResourcesLoaded() const
{
    return m_sharedResourcesLoaded;
}

void Project::setSharedResourcesLoaded(bool on)
{
    m_sharedResourcesLoaded = on;
}

void Project::setSharedResourcesFile(const QString &file)
{
    m_sharedResourcesFile = file;
}

QString Project::sharedResourcesFile() const
{
    return m_sharedResourcesFile;
}

QList<QUrl> Project::taskModules(bool includeLocal) const
{
    if (!includeLocal && m_useLocalTaskModules) {
        QList<QUrl> lst = m_taskModules;
        lst.removeAll(m_localTaskModulesPath);
        return lst;
    }
    return m_taskModules;
}

void Project::setTaskModules(const QList<QUrl> modules, bool useLocalTaskModules)
{
    m_taskModules = modules;
    m_useLocalTaskModules = useLocalTaskModules;
    if (m_useLocalTaskModules && m_localTaskModulesPath.isValid()) {
        m_taskModules.prepend(m_localTaskModulesPath);
    }
    Q_EMIT taskModulesChanged(m_taskModules);
}

bool Project::useLocalTaskModules() const
{
    return m_useLocalTaskModules;
}

void Project::setUseLocalTaskModules(bool value, bool emitChanged)
{
    if (m_useLocalTaskModules) {
        m_taskModules.removeAll(m_localTaskModulesPath);
    }
    m_useLocalTaskModules = value;
    if (m_useLocalTaskModules && m_localTaskModulesPath.isValid()) {
        m_taskModules.prepend(m_localTaskModulesPath);
    }
    if (emitChanged) {
        Q_EMIT taskModulesChanged(m_taskModules);
    }
}

void Project::setLocalTaskModulesPath(const QUrl &url)
{
    m_taskModules.removeAll(m_localTaskModulesPath);
    m_localTaskModulesPath = url;
    if (m_useLocalTaskModules && url.isValid()) {
        m_taskModules.prepend(url);
    }
    Q_EMIT taskModulesChanged(m_taskModules);
}

ulong Project::granularity() const
{
    auto sm = m_currentSchedule ? m_currentSchedule->manager() : nullptr;
    auto g = sm ? sm->granularity() : 0;
    return g;
}

void Project::nodeDestroyed(QObject *obj)
{
    auto node = qobject_cast<Node*>(obj);
    if (node) {
        removeId(node->id());
    }
}

void Project::emitProjectCalculated(KPlato::Project *project, KPlato::ScheduleManager *sm)
{
    Q_UNUSED(project)
    Q_EMIT projectCalculated(sm);
}

void Project::setFreedaysCalendar(Calendar *calendar)
{
    if (m_freedaysCalendar != calendar) {
        m_freedaysCalendar = calendar;
        Q_EMIT freedaysCalendarChanged(calendar);
    }
}

Calendar *Project::freedaysCalendar() const
{
    return m_freedaysCalendar;
}

}  //KPlato namespace
