/* This file is part of the KDE project
 Copyright (C) 2001 Thomas zander <zander@kde.org>
 Copyright (C) 2004 - 2010, 2012 Dag Andersen <danders@get2net.dk>
 Copyright (C) 2007 Florian Piquemal <flotueur@yahoo.fr>
 Copyright (C) 2007 Alexis Ménard <darktears31@gmail.com>
 Copyright (C) 2019 Dag Andersen <danders@get2net.dk>

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
* Boston, MA 02110-1301, USA.
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

namespace KPlato
{

QString generateId()
{
    return QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMddHHmmss")) + KRandom::randomString(10);
}

Project::Project(Node *parent)
        : Node(parent),
        m_accounts(*this),
        m_defaultCalendar(0),
        m_config(&emptyConfig),
        m_schedulerPlugins(),
        m_useSharedResources(false),
        m_sharedResourcesLoaded(false),
        m_loadProjectsAtStartup(false),
        m_useLocalTaskModules(true)
{
    //debugPlan<<"("<<this<<")";
    init();
}

Project::Project(ConfigBase &config, Node *parent)
        : Node(parent),
        m_accounts(*this),
        m_defaultCalendar(0),
        m_config(&config),
        m_schedulerPlugins(),
        m_useSharedResources(false),
        m_sharedResourcesLoaded(false),
        m_loadProjectsAtStartup(false),
        m_useLocalTaskModules(true)
{
    debugPlan<<"("<<this<<")";
    init();
    m_config->setDefaultValues(*this);
}

Project::Project(ConfigBase &config, bool useDefaultValues, Node *parent)
        : Node(parent),
        m_accounts(*this),
        m_defaultCalendar(0),
        m_config(&config),
        m_schedulerPlugins(),
        m_useSharedResources(false),
        m_sharedResourcesLoaded(false),
        m_loadProjectsAtStartup(false),
        m_useLocalTaskModules(true)
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
    if (m_parent == 0) {
        // set sensible defaults for a project wo parent
        m_constraintStartTime = DateTime(QDate::currentDate());
        m_constraintEndTime = m_constraintStartTime.addYears(2);
    }
}

void Project::deref()
{
    --m_refCount;
    Q_ASSERT(m_refCount >= 0);
    if (m_refCount <= 0) {
        emit aboutToBeDeleted();
        deleteLater();
    }
}

Project::~Project()
{
    debugPlan<<"("<<this<<")";
    disconnect();
    for(Node *n : qAsConst(nodeIdDict)) {
        n->blockChanged();
    }
    for (Resource *r : qAsConst(m_resources)) {
        r->blockChanged();
    }
    for (ResourceGroup *g : qAsConst(resourceGroupIdDict)) {
        g->blockChanged();
    }
    delete m_standardWorktime;
    for (Resource *r : m_resources) {
        delete r;
    }
    while (!m_resourceGroups.isEmpty())
        delete m_resourceGroups.takeFirst();
    while (!m_calendars.isEmpty())
        delete m_calendars.takeFirst();
    while (!m_managers.isEmpty())
        delete m_managers.takeFirst();

    m_config = 0; //not mine, don't delete
}

int Project::type() const { return Node::Type_Project; }

void Project::generateUniqueNodeIds()
{
    foreach (Node *n, nodeIdDict) {
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

    foreach (ResourceGroup *g, resourceGroupIdDict) {
        if (g->isShared()) {
            continue;
        }
        resourceGroupIdDict.remove(g->id());
        g->setId(uniqueResourceGroupId());
        resourceGroupIdDict[ g->id() ] = g;
    }
    foreach (Resource *r, resourceIdDict) {
        if (r->isShared()) {
            continue;
        }
        resourceIdDict.remove(r->id());
        r->setId(uniqueResourceId());
        resourceIdDict[ r->id() ] = r;
    }
    foreach (Calendar *c, calendarIdDict) {
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
    if (schedule == 0) {
        errorPlan << "Schedule == 0, cannot calculate";
        return ;
    }
    m_currentSchedule = schedule;
    calculate(dt);
}

void Project::calculate(const DateTime &dt)
{
    if (m_currentSchedule == 0) {
        errorPlan << "No current schedule to calculate";
        return ;
    }
    stopcalculation = false;
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
        emit scheduleChanged(cs);
        emit projectChanged();
    } else if (type() == Type_Subproject) {
        warnPlan << "Subprojects not implemented";
    } else {
        errorPlan << "Illegal project type: " << type();
    }
}

void Project::calculate(ScheduleManager &sm)
{
    emit sigCalculationStarted(this, &sm);
    sm.setScheduling(true);
    m_progress = 0;
    int nodes = 0;
    foreach (Node *n, nodeIdDict) {
        if (n->type() == Node::Type_Task || n->type() == Node::Type_Milestone) {
            nodes++;
        }
    }
    int maxprogress = nodes * 3;
    if (sm.recalculate()) {
        emit maxProgress(maxprogress);
        sm.setMaxProgress(maxprogress);
        incProgress();
        if (sm.parentManager()) {
            sm.expected()->startTime = sm.parentManager()->expected()->startTime;
            sm.expected()->earlyStart = sm.parentManager()->expected()->earlyStart;
        }
        incProgress();
        calculate(sm.expected(), sm.recalculateFrom());
    } else {
        emit maxProgress(maxprogress);
        sm.setMaxProgress(maxprogress);
        calculate(sm.expected());
        emit scheduleChanged(sm.expected());
        setCurrentSchedule(sm.expected()->id());
    }
    emit sigProgress(maxprogress);
    emit sigCalculationFinished(this, &sm);
    emit scheduleManagerChanged(&sm);
    emit projectCalculated(&sm);
    emit projectChanged();
    sm.setScheduling(false);
}

void Project::calculate(Schedule *schedule)
{
    if (schedule == 0) {
        errorPlan << "Schedule == 0, cannot calculate";
        return ;
    }
    m_currentSchedule = schedule;
    calculate();
}

void Project::calculate()
{
    if (m_currentSchedule == 0) {
        errorPlan << "No current schedule to calculate";
        return ;
    }
    stopcalculation = false;
    MainSchedule *cs = static_cast<MainSchedule*>(m_currentSchedule);
    bool backwards = false;
    if (cs->manager()) {
        backwards = cs->manager()->schedulingDirection();
    }
    QLocale locale;
    Estimate::Use estType = (Estimate::Use) cs->type();
    if (type() == Type_Project) {
        QTime timer; timer.start();
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
            cs->endTime = cs->startTime;
            foreach (Node *n, allNodes()) {
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
        cs->logInfo(i18n("Calculation took: %1", KFormat().formatDuration(timer.elapsed())));
        // TODO: fix this uncertainty, manager should *always* be available
        if (cs->manager()) {
            finishCalculation(*(cs->manager()));
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
        for (const Node *n : qAsConst(nodeIdDict)) {
            cs->startTime = qMin(cs->startTime, n->startTime(cs->id()));
            cs->endTime = qMax(cs->endTime, n->endTime(cs->id()));
        }
        cs->duration = cs->endTime - cs->startTime;
    }

    calcCriticalPath(false);
    calcResourceOverbooked();
    cs->notScheduled = false;
    calcFreeFloat();
    emit scheduleChanged(cs);
    emit projectChanged();
    debugPlan<<cs->startTime<<cs->endTime<<"-------------------------";
}

void Project::setProgress(int progress, ScheduleManager *sm)
{
    m_progress = progress;
    if (sm) {
        sm->setProgress(progress);
    }
    emit sigProgress(progress);
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
    emit sigProgress(m_progress);
}

void Project::emitMaxProgress(int value)
{
    emit maxProgress(value);
}

bool Project::calcCriticalPath(bool fromEnd)
{
    //debugPlan;
    MainSchedule *cs = static_cast<MainSchedule*>(m_currentSchedule);
    if (cs == 0) {
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
    foreach (Node *n, allNodes()) {
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
    foreach (Relation *r, node->dependChildNodes()) {
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
    if (s == 0) {
        //debugPlan<<"No schedule with id="<<id;
        return 0;
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
    if (s == 0) {
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
    return 0L;
}

DateTime Project::checkStartConstraints(const DateTime &dt) const
{
    DateTime t = dt;
    foreach (Node *n, nodeIdDict) {
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
    foreach (Node *n, nodeIdDict) {
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
    foreach (Relation *r, n->dependParentNodes()) {
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
    foreach (Relation *r, t->parentProxyRelations()) {
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
    foreach (Relation *r, n->dependChildNodes()) {
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
    foreach (Relation *r, t->childProxyRelations()) {
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
    // Do these in reverse order to get tasks with same prio in wbs order
    const QList<Task*> tasks = allTasks();
    for (int i = tasks.count() -1; i >= 0; --i) {
        Task *t = tasks.at(i);
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
#ifndef PLAN_NLOGDEBUG
    debugPlan<<"End nodes:"<<m_terminalNodes;
    foreach (Node* n, m_terminalNodes) {
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
    // Do these in reverse order to get tasks with same prio in wbs order
    const QList<Task*> tasks = allTasks();
    for (int i = tasks.count() -1; i >= 0; --i) {
        Task *t = tasks.at(i);
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
#ifndef PLAN_NLOGDEBUG
    debugPlan<<"Start nodes:"<<m_terminalNodes;
    foreach (Node* n, m_terminalNodes) {
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
    if (cs == 0) {
        return finish;
    }
    if (type() == Node::Type_Project) {
        QTime timer;
        timer.start();
        cs->logInfo(i18n("Start calculating forward"));
        m_visitedForward = true;
        if (! m_visitedBackward) {
            // setup tasks
            tasksForward();
            // Do all hard constrained first
            foreach (Node *n, m_hardConstraints) {
                cs->logDebug("Calculate task with hard constraint:" + n->name() + " : " + n->constraintToString());
                DateTime time = n->calculateEarlyFinish(use); // do not do predeccessors
                if (time > finish) {
                    finish = time;
                }
            }
            // do the predeccessors
            foreach (Node *n, m_hardConstraints) {
                cs->logDebug("Calculate predeccessors to hard constrained task:" + n->name() + " : " + n->constraintToString());
                DateTime time = n->calculateForward(use);
                if (time > finish) {
                    finish = time;
                }
            }
            // now try to schedule soft constrained *with* predeccessors
            foreach (Node *n, m_softConstraints) {
                cs->logDebug("Calculate task with soft constraint:" + n->name() + " : " + n->constraintToString());
                DateTime time = n->calculateForward(use);
                if (time > finish) {
                    finish = time;
                }
            }
            // and then the rest using the end nodes to calculate everything (remaining)
            foreach (Task *n, m_terminalNodes) {
                cs->logDebug("Calculate using end task:" + n->name() + " : " + n->constraintToString());
                DateTime time = n->calculateForward(use);
                if (time > finish) {
                    finish = time;
                }
            }
        } else {
            // tasks have been calculated backwards in this order
            foreach (Node *n, cs->backwardNodes()) {
                DateTime time = n->calculateForward(use);
                if (time > finish) {
                    finish = time;
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
    if (cs == 0) {
        return start;
    }
    if (type() == Node::Type_Project) {
        QTime timer;
        timer.start();
        cs->logInfo(i18n("Start calculating backward"));
        m_visitedBackward = true;
        if (! m_visitedForward) {
            // setup tasks
            tasksBackward();
            // Do all hard constrained first
            foreach (Task *n, m_hardConstraints) {
                cs->logDebug("Calculate task with hard constraint:" + n->name() + " : " + n->constraintToString());
                DateTime time = n->calculateLateStart(use); // do not do predeccessors
                if (! start.isValid() || time < start) {
                    start = time;
                }
            }
            // then do the predeccessors
            foreach (Task *n, m_hardConstraints) {
                cs->logDebug("Calculate predeccessors to hard constrained task:" + n->name() + " : " + n->constraintToString());
                DateTime time = n->calculateBackward(use);
                if (! start.isValid() || time < start) {
                    start = time;
                }
            }
            // now try to schedule soft constrained *with* predeccessors
            foreach (Task *n, m_softConstraints) {
                cs->logDebug("Calculate task with soft constraint:" + n->name() + " : " + n->constraintToString());
                DateTime time = n->calculateBackward(use);
                if (! start.isValid() || time < start) {
                    start = time;
                }
            }
            // and then the rest using the start nodes to calculate everything (remaining)
            foreach (Task *n, m_terminalNodes) {
                cs->logDebug("Calculate using start task:" + n->name() + " : " + n->constraintToString());
                DateTime time = n->calculateBackward(use);
                if (! start.isValid() || time < start) {
                    start = time;
                }
            }
        } else {
            // tasks have been calculated forwards in this order
            foreach (Node *n, cs->forwardNodes()) {
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
    if (cs == 0 || stopcalculation) {
        return DateTime();
    }
    QTime timer;
    timer.start();
    cs->logInfo(i18n("Start scheduling forward"));
    resetVisited();
    // Schedule in the same order as calculated forward
    // Do all hard constrained first
    foreach (Node *n, m_hardConstraints) {
        cs->logDebug("Schedule task with hard constraint:" + n->name() + " : " + n->constraintToString());
        DateTime time = n->scheduleFromStartTime(use); // do not do predeccessors
        if (time > end) {
            end = time;
        }
    }
    foreach (Node *n, cs->forwardNodes()) {
        cs->logDebug("Schedule task:" + n->name() + " : " + n->constraintToString());
        DateTime time = n->scheduleForward(earliest, use);
        if (time > end) {
            end = time;
        }
    }
    // Fix summarytasks
    adjustSummarytask();
    cs->logInfo(i18n("Finished scheduling forward: %1 ms", timer.elapsed()));
    foreach (Node *n, allNodes()) {
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
    if (cs == 0 || stopcalculation) {
        return start;
    }
    QTime timer;
    timer.start();
    cs->logInfo(i18n("Start scheduling backward"));
    resetVisited();
    // Schedule in the same order as calculated backward
    // Do all hard constrained first
    foreach (Node *n, m_hardConstraints) {
        cs->logDebug("Schedule task with hard constraint:" + n->name() + " : " + n->constraintToString());
        DateTime time = n->scheduleFromEndTime(use); // do not do predeccessors
        if (! start.isValid() || time < start) {
            start = time;
        }
    }
    foreach (Node *n, cs->backwardNodes()) {
        cs->logDebug("Schedule task:" + n->name() + " : " + n->constraintToString());
        DateTime time = n->scheduleBackward(latest, use);
        if (! start.isValid() || time < start) {
            start = time;
        }
    }
    // Fix summarytasks
    adjustSummarytask();
    cs->logInfo(i18n("Finished scheduling backward: %1 ms", timer.elapsed()));
    foreach (Node *n, allNodes()) {
        if (n->type() == Node::Type_Task || n->type() == Node::Type_Milestone) {
            Q_ASSERT(n->isScheduled());
        }
    }
    return start;
}

void Project::adjustSummarytask()
{
    MainSchedule *cs = static_cast<MainSchedule*>(m_currentSchedule);
    if (cs == 0 || stopcalculation) {
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
    for (Resource *r : m_resources) {
        r->initiateCalculation(sch);
    }

    Node::initiateCalculation(sch);
}

void Project::initiateCalculationLists(MainSchedule &sch)
{
    //debugPlan<<m_name;
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

bool Project::load(KoXmlElement &projectElement, XMLLoaderObject &status)
{
    //debugPlan<<"--->";
    QString s;
    bool ok = false;
    if (projectElement.hasAttribute("name")) {
        setName(projectElement.attribute("name"));
    }
    if (projectElement.hasAttribute("id")) {
        removeId(m_id);
        m_id = projectElement.attribute("id");
        registerNodeId(this);
    }
    if (projectElement.hasAttribute("priority")) {
        m_priority = projectElement.attribute(QStringLiteral("priority"), "0").toInt();
    }
    if (projectElement.hasAttribute("leader")) {
        m_leader = projectElement.attribute("leader");
    }
    if (projectElement.hasAttribute("description")) {
        m_description = projectElement.attribute("description");
    }
    if (projectElement.hasAttribute("timezone")) {
        QTimeZone tz(projectElement.attribute("timezone").toLatin1());
        if (tz.isValid()) {
            m_timeZone = tz;
        } else warnPlan<<"No timezone specified, using default (local)";
        status.setProjectTimeZone(m_timeZone);
    }
    if (projectElement.hasAttribute("scheduling")) {
        // Allow for both numeric and text
        s = projectElement.attribute("scheduling", "0");
        m_constraint = (Node::ConstraintType) s.toInt(&ok);
        if (!ok) {
            setConstraint(s);
        }
        if (m_constraint != Node::MustStartOn && m_constraint != Node::MustFinishOn) {
            errorPlanXml << "Illegal constraint: " << constraintToString();
            setConstraint(Node::MustStartOn);
        }
    }
    if (projectElement.hasAttribute("start-time")) {
        s = projectElement.attribute("start-time");
        if (!s.isEmpty())
            m_constraintStartTime = DateTime::fromString(s, m_timeZone);
    }
    if (projectElement.hasAttribute("end-time")) {
        s = projectElement.attribute("end-time");
        if (!s.isEmpty())
            m_constraintEndTime = DateTime::fromString(s, m_timeZone);
    }
    status.setProgress(10);

    // Load the project children
    KoXmlElement e = projectElement;
    if (status.version() < "0.7.0") {
        e = projectElement;
    } else {
        e = projectElement.namedItem("project-settings").toElement();
    }
    if (!e.isNull()) {
        loadSettings(e, status);
    }
    e = projectElement.namedItem("documents").toElement();
    if (!e.isNull()) {
        m_documents.load(e, status);
    }
    // Do calendars first, they only reference other calendars
    //debugPlan<<"Calendars--->";
    if (status.version() < "0.7.0") {
        e = projectElement;
    } else {
        e = projectElement.namedItem("calendars").toElement();
    }
    if (!e.isNull()) {
        debugPlanXml<<status.version()<<e.tagName();
        QList<Calendar*> cals;
        KoXmlElement ce;
        forEachElement(ce, e) {
            if (ce.tagName() != "calendar") {
                continue;
            }
            // Load the calendar.
            // Referenced by resources
            Calendar *child = new Calendar();
            child->setProject(this);
            if (child->load(ce, status)) {
                cals.append(child); // temporary, reorder later
            } else {
                // TODO: Complain about this
                errorPlan << "Failed to load calendar";
                delete child;
            }
        }
        // calendars references calendars in arbitrary saved order
        bool added = false;
        do {
            added = false;
            QList<Calendar*> lst;
            while (!cals.isEmpty()) {
                Calendar *c = cals.takeFirst();
                c->m_blockversion = true;
                if (c->parentId().isEmpty()) {
                    addCalendar(c, status.baseCalendar()); // handle pre 0.6 version
                    added = true;
                    //debugPlan<<"added to project:"<<c->name();
                } else {
                    Calendar *par = calendar(c->parentId());
                    if (par) {
                        par->m_blockversion = true;
                        addCalendar(c, par);
                        added = true;
                        //debugPlan<<"added:"<<c->name()<<" to parent:"<<par->name();
                        par->m_blockversion = false;
                    } else {
                        lst.append(c); // treat later
                        //debugPlan<<"treat later:"<<c->name();
                    }
                }
                c->m_blockversion = false;
            }
            cals = lst;
        } while (added);
        if (! cals.isEmpty()) {
            errorPlan<<"All calendars not saved!";
        }
        //debugPlan<<"Calendars<---";
    }
    status.setProgress(15);

    KoXmlNode n;
    // Resource groups and resources, can reference calendars
    if (status.version() < "0.7.0") {
        forEachElement(e, projectElement) {
            if (e.tagName() == "resource-group") {
                debugPlanXml<<status.version()<<e.tagName();
                // Load the resources
                // References calendars
                ResourceGroup *child = new ResourceGroup();
                if (child->load(e, status)) {
                    addResourceGroup(child);
                } else {
                    // TODO: Complain about this
                    errorPlan<<Q_FUNC_INFO<<"Failed to load resource group";
                    delete child;
                }
            }
        }
        for (ResourceGroup *g : m_resourceGroups) {
            for (Resource *r : g->resources()) {
                addResource(r);
            }
        }
    } else {
        e = projectElement.namedItem("resource-groups").toElement();
        if (!e.isNull()) {
            debugPlanXml<<status.version()<<e.tagName();
            KoXmlElement ge;
            forEachElement(ge, e) {
                if (ge.nodeName() != "resource-group") {
                    continue;
                }
                ResourceGroup *child = new ResourceGroup();
                if (child->load(ge, status)) {
                    addResourceGroup(child);
                } else {
                    // TODO: Complain about this
                    delete child;
                }
            }
        }
        e = projectElement.namedItem("resources").toElement();
        if (!e.isNull()) {
            debugPlanXml<<status.version()<<e.tagName();
            KoXmlElement re;
            forEachElement(re, e) {
                if (re.nodeName() != "resource") {
                    continue;
                }
                Resource *r = new Resource();
                if (r->load(re, status)) {
                    addResource(r);
                } else {
                    errorPlan<<"Failed to load resource xml";
                    delete r;
                }
            }
        }
        // resource-group relations
        e = projectElement.namedItem("resource-group-relations").toElement();
        if (!e.isNull()) {
            debugPlanXml<<status.version()<<e.tagName();
            KoXmlElement re;
            forEachElement(re, e) {
                if (re.nodeName() != "resource-group-relation") {
                    continue;
                }
                ResourceGroup *g = group(re.attribute("group-id"));
                Resource *r = resource(re.attribute("resource-id"));
                if (r && g) {
                    r->addParentGroup(g);
                } else {
                    errorPlan<<"Failed to load resource-group-relation";
                }
            }
        }
        e = projectElement.namedItem("resource-teams").toElement();
        if (!e.isNull()) {
            debugPlanXml<<status.version()<<e.tagName();
            KoXmlElement el;
            forEachElement(el, e) {
                if (el.tagName() == "team") {
                    Resource *r = findResource(el.attribute("team-id"));
                    Resource *tm = findResource(el.attribute("member-id"));
                    if (r == nullptr || tm == nullptr) {
                        errorPlan<<"resource-teams: cannot find resources";
                        continue;
                    }
                    if (r == tm) {
                        errorPlan<<"resource-teams: a team cannot be a member of itself";
                        continue;
                    }
                    r->addTeamMemberId(tm->id());
                } else {
                    errorPlan<<"resource-teams: unhandled tag"<<el.tagName();
                }
            }
        }
        e = projectElement.namedItem("required-resources").toElement();
        if (!e.isNull()) {
            debugPlanXml<<status.version()<<e.tagName();
            KoXmlElement re;
            forEachElement(re, e) {
                if (re.nodeName() != "required-resource") {
                    continue;
                }
                Resource *required = this->resource(re.attribute("required-id"));
                Resource *resource = this->resource(re.attribute("resource-id"));
                if (required && resource) {
                    resource->addRequiredId(required->id());
                } else {
                    errorPlan<<"Failed to load required-resource";
                }
            }
        }
        e = projectElement.namedItem("external-appointments").toElement();
        if (!e.isNull()) {
            debugPlanXml<<status.version()<<e.tagName();
            KoXmlElement ext;
            forEachElement(ext, e) {
                if (e.nodeName() != "external-appointment") {
                    continue;
                }
                Resource *resource = this->resource(e.attribute("resource-id"));
                if (!resource) {
                    errorPlan<<"Cannot find resource:"<<e.attribute("resource-id");
                    continue;
                }
                QString projectId = e.attribute("project-id");
                if (projectId.isEmpty()) {
                    errorPlan<<"Missing project id";
                    continue;
                }
                resource->clearExternalAppointments(projectId); // in case...
                AppointmentIntervalList lst;
                lst.loadXML(e, status);
                Appointment *a = new Appointment();
                a->setIntervals(lst);
                a->setAuxcilliaryInfo(e.attribute("project-name", "Unknown"));
                resource->addExternalAppointment(projectId, a);
            }
        }
    }

    status.setProgress(20);

    // The main stuff
    if (status.version() < "0.7.0") {
        e = projectElement;
    } else {
        e = projectElement.namedItem("tasks").toElement();
    }
    if (!e.isNull()) {
        debugPlanXml<<status.version()<<"tasks:"<<e.tagName();
        KoXmlElement te;
        forEachElement(te, e) {
            if (te.tagName() != "task") {
                continue;
            }
            //debugPlan<<"Task--->";
            // Depends on resources already loaded
            Task *child = new Task(this);
            if (child->load(te, status)) {
                if (!addTask(child, this)) {
                    delete child; // TODO: Complain about this
                } else {
                    debugPlanXml<<status.version()<<"Added task:"<<child;
                }
            } else {
                // TODO: Complain about this
                delete child;
            }
        }
    }
    if (numChildren() == 0) {
        warnPlanXml<<"No tasks added from"<<e.tagName();
    }

    status.setProgress(70);

    // These go last
    e = projectElement.namedItem("accounts").toElement();
    if (!m_accounts.load(e, *this)) {
        errorPlan << "Failed to load accounts";
    }

    n = projectElement.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        //debugPlan<<n.isElement();
        if (! n.isElement()) {
            continue;
        }
        if (status.version() < "0.7.0" && e.tagName() == "relation") {
            debugPlanXml<<status.version()<<e.tagName();
            // Load the relation
            // References tasks
            Relation *child = new Relation();
            if (!child->load(e, status)) {
                // TODO: Complain about this
                errorPlan << "Failed to load relation";
                delete child;
            }
            //debugPlan<<"Relation<---";
        } else if (status.version() < "0.7.0" && e.tagName() == "resource-requests") {
            // NOTE: Not supported: request to project (or sub-project)
            Q_ASSERT(false);
#if 0
            // Load the resource request
            // Handle multiple requests to same group gracefully (Not really allowed)
            KoXmlElement req;
            forEachElement(req, e) {
                ResourceGroupRequest *r = m_requests.findGroupRequestById(req.attribute(QStringLiteral("group-id")));
                if (r) {
                    warnPlan<<"Multiple requests to same group, loading into existing group";
                    if (! r->load(e, status)) {
                        errorPlan<<"Failed to load resource request";
                    }
                } else {
                    r = new ResourceGroupRequest();
                    if (r->load(e, status)) {
                        addRequest(r);
                    } else {
                        errorPlan<<"Failed to load resource request";
                        delete r;
                    }
                }
            }
#endif
        } else if (status.version() < "0.7.0" && e.tagName() == "wbs-definition") {
            m_wbsDefinition.loadXML(e, status);
        } else if (e.tagName() == "locale") {
            // handled earlier
        } else if (e.tagName() == "resource-group") {
            // handled earlier
        } else if (e.tagName() == "calendar") {
            // handled earlier
        } else if (e.tagName() == "standard-worktime") {
            // handled earlier
        } else if (e.tagName() == "project") {
            // handled earlier
        } else if (e.tagName() == "task") {
            // handled earlier
        } else if (e.tagName() == "shared-resources") {
            // handled earlier
        } else if (e.tagName() == "documents") {
            // handled earlier
        } else if (e.tagName() == "workpackageinfo") {
            // handled earlier
        } else if (e.tagName() == "task-modules") {
            // handled earlier
        } else if (e.tagName() == "accounts") {
            // handled earlier
        } else {
            warnPlan<<"Unhandled tag:"<<e.tagName();
        }
    }
    if (status.version() < "0.7.0") {
        e = projectElement.namedItem("schedules").toElement();
    } else {
        e = projectElement.namedItem("project-schedules").toElement();
    }
    if (!e.isNull()) {
        debugPlanXml<<status.version()<<e.tagName();
        // References tasks and resources
        KoXmlElement sn;
        forEachElement(sn, e) {
            //debugPlan<<sn.tagName()<<" Version="<<status.version();
            ScheduleManager *sm = 0;
            bool add = false;
            if (status.version() <= "0.5") {
                if (sn.tagName() == "schedule") {
                    sm = findScheduleManagerByName(sn.attribute("name"));
                    if (sm == 0) {
                        sm = new ScheduleManager(*this, sn.attribute("name"));
                        add = true;
                    }
                }
            } else if (sn.tagName() == "schedule-management" || (status.version() < "0.7.0" && sn.tagName() == "plan")) {
                sm = new ScheduleManager(*this);
                add = true;
            } else {
                continue;
            }
            if (sm) {
                debugPlan<<"load schedule manager";
                if (sm->loadXML(sn, status)) {
                    if (add)
                        addScheduleManager(sm);
                } else {
                    errorPlan << "Failed to load schedule manager";
                    delete sm;
                }
            } else {
                debugPlan<<"No schedule manager ?!";
            }
        }
        //debugPlan<<"Node schedules<---";
    }
    e = projectElement.namedItem("resource-teams").toElement();
    if (!e.isNull()) {
        debugPlanXml<<status.version()<<e.tagName();
        // References other resources
        KoXmlElement re;
        forEachElement(re, e) {
            if (re.tagName() != "team") {
                continue;
            }
            Resource *r = findResource(re.attribute("team-id"));
            Resource *tm = findResource(re.attribute("member-id"));
            if (r == nullptr || tm == nullptr) {
                errorPlan<<"resource-teams: cannot find resources";
                continue;
            }
            if (r == tm) {
                errorPlan<<"resource-teams: a team cannot be a member of itself";
                continue;
            }
            r->addTeamMemberId(tm->id());
        }
    }
    if (status.version() >= "0.7.0") {
        e = projectElement.namedItem("relations").toElement();
        debugPlanXml<<status.version()<<e.tagName();
        if (!e.isNull()) {
            KoXmlElement de;
            forEachElement(de, e) {
                if (de.tagName() != "relation") {
                    continue;
                }
                Relation *child = new Relation();
                if (!child->load(de, status)) {
                    // TODO: Complain about this
                    errorPlan << "Failed to load relation";
                    delete child;
                }
            }
        }
        e = projectElement.namedItem("resource-requests").toElement();
        if (!e.isNull()) {
            debugPlanXml<<status.version()<<e.tagName();
            KoXmlElement re;
            forEachElement(re, e) {
                if (re.tagName() != "resource-request") {
                    continue;
                }
                Node *task = findNode(re.attribute("task-id"));
                if (!task) {
                    warnPlanXml<<re.tagName()<<"Failed to find task";
                    continue;
                }
                Resource *resource = findResource(re.attribute("resource-id"));
                Q_ASSERT(resource);
                Q_ASSERT(task);
                if (resource && task) {
                    int units = re.attribute("units", "100").toInt();
                    ResourceRequest *request = new ResourceRequest(resource, units);
                    int requestId = re.attribute("request-id").toInt();
                    Q_ASSERT(requestId > 0);
                    request->setId(requestId);
                    task->requests().addResourceRequest(request);
                } else {
                    warnPlanXml<<re.tagName()<<"Failed to find resource";
                }
            }
        }
        e = projectElement.namedItem("required-resource-requests").toElement();
        if (!e.isNull()) {
            debugPlanXml<<status.version()<<e.tagName();
            KoXmlElement re;
            forEachElement(re, e) {
                if (re.tagName() != "required-resource-request") {
                    continue;
                }
                Node *task = findNode(re.attribute("task-id"));
                Q_ASSERT(task);
                if (!task) {
                    continue;
                }
                ResourceRequest *request = task->requests().resourceRequest(re.attribute("request-id").toInt());
                Resource *required = findResource(re.attribute("required-id"));
                QList<Resource*> lst;
                if (required && request->resource() != required) {
                    lst << required;
                }
                request->setRequiredResources(lst);
            }
        }
        e = projectElement.namedItem("alternative-requests").toElement();
        if (!e.isNull()) {
            debugPlanXml<<status.version()<<e.tagName();
            KoXmlElement re;
            forEachElement(re, e) {
                if (re.tagName() != "alternative-request") {
                    continue;
                }
                Node *task = findNode(re.attribute("task-id"));
                if (!task) {
                    warnPlanXml<<re.tagName()<<"Failed to find task";
                    continue;
                }
                //TODO
            }
        }
    }
    //debugPlan<<"<---";

    status.setProgress(90);

    return true;
}

bool Project::loadSettings(KoXmlElement &element, XMLLoaderObject &status)
{
    Q_UNUSED(status)

    m_useSharedResources = false; // default should be off in case old project NOTE: review

    KoXmlElement e;
    forEachElement(e, element) {
        debugPlanXml<<status.version()<<e.tagName();
        if (e.tagName() == "locale") {
            Locale *l = locale();
            l->setCurrencySymbol(e.attribute("currency-symbol", ""));
            if (e.hasAttribute("currency-digits")) {
                l->setMonetaryDecimalPlaces(e.attribute("currency-digits").toInt());
            }
            QLocale::Language language = QLocale::AnyLanguage;
            QLocale::Country country = QLocale::AnyCountry;
            if (e.hasAttribute("language")) {
                language = static_cast<QLocale::Language>(e.attribute("language").toInt());
            }
            if (e.hasAttribute("country")) {
                country = static_cast<QLocale::Country>(e.attribute("country").toInt());
            }
            l->setCurrencyLocale(language, country);
        } else if (e.tagName() == "shared-resources") {
            m_useSharedResources = e.attribute("use", "0").toInt();
            m_sharedResourcesFile = e.attribute("file");
            m_sharedProjectsUrl = QUrl(e.attribute("projects-url"));
            m_loadProjectsAtStartup = (bool)e.attribute("projects-loadatstartup", "0").toInt();
        } else if (e.tagName() == QLatin1String("workpackageinfo")) {
            if (e.hasAttribute("check-for-workpackages")) {
                m_workPackageInfo.checkForWorkPackages = e.attribute("check-for-workpackages").toInt();
            }
            if (e.hasAttribute("retrieve-url")) {
                m_workPackageInfo.retrieveUrl = QUrl(e.attribute("retrieve-url"));
            }
            if (e.hasAttribute("delete-after-retrieval")) {
                m_workPackageInfo.deleteAfterRetrieval = e.attribute("delete-after-retrieval").toInt();
            }
            if (e.hasAttribute("archive-after-retrieval")) {
                m_workPackageInfo.archiveAfterRetrieval = e.attribute("archive-after-retrieval").toInt();
            }
            if (e.hasAttribute("archive-url")) {
                m_workPackageInfo.archiveUrl = QUrl(e.attribute("archive-url"));
            }
            if (e.hasAttribute("publish-url")) {
                m_workPackageInfo.publishUrl = QUrl(e.attribute("publish-url"));
            }
        } else if (e.tagName() == QLatin1String("task-modules")) {
            m_useLocalTaskModules = false;
            QList<QUrl> urls;
            for (KoXmlNode child = e.firstChild(); !child.isNull(); child = child.nextSibling()) {
                KoXmlElement path = child.toElement();
                if (path.isNull()) {
                    continue;
                }
                QString s = path.attribute("url");
                if (!s.isEmpty()) {
                    QUrl url = QUrl::fromUserInput(s);
                    if (!urls.contains(url)) {
                        urls << url;
                    }
                }
            }
            m_taskModules = urls;
            // If set adds local path to taskModules()
            setUseLocalTaskModules((bool)e.attribute("use-local-task-modules").toInt());
        } else if (e.tagName() == "standard-worktime") {
            // Load standard worktime
            StandardWorktime *child = new StandardWorktime();
            if (child->load(e, status)) {
                setStandardWorktime(child);
            } else {
                errorPlanXml << "Failed to load standard worktime";
                delete child;
            }
        }
    }
    return true;
}
void Project::saveSettings(QDomElement &element, const XmlSaveContext &context) const
{
    Q_UNUSED(context)

    QDomElement settingsElement = element.ownerDocument().createElement("project-settings");
    element.appendChild(settingsElement);

    m_wbsDefinition.saveXML(settingsElement);

    QDomElement loc = settingsElement.ownerDocument().createElement("locale");
    settingsElement.appendChild(loc);
    const Locale *l = locale();
    loc.setAttribute("currency-symbol", l->currencySymbol());
    loc.setAttribute("currency-digits", l->monetaryDecimalPlaces());
    loc.setAttribute("language", l->currencyLanguage());
    loc.setAttribute("country", l->currencyCountry());

    QDomElement share = settingsElement.ownerDocument().createElement("shared-resources");
    settingsElement.appendChild(share);
    share.setAttribute("use", m_useSharedResources);
    share.setAttribute("file", m_sharedResourcesFile);
    share.setAttribute("projects-url", QString(m_sharedProjectsUrl.toEncoded()));
    share.setAttribute("projects-loadatstartup", m_loadProjectsAtStartup);

    QDomElement wpi = settingsElement.ownerDocument().createElement("workpackageinfo");
    settingsElement.appendChild(wpi);
    wpi.setAttribute("check-for-workpackages", m_workPackageInfo.checkForWorkPackages);
    wpi.setAttribute("retrieve-url", m_workPackageInfo.retrieveUrl.toString(QUrl::None));
    wpi.setAttribute("delete-after-retrieval", m_workPackageInfo.deleteAfterRetrieval);
    wpi.setAttribute("archive-after-retrieval", m_workPackageInfo.archiveAfterRetrieval);
    wpi.setAttribute("archive-url", m_workPackageInfo.archiveUrl.toString(QUrl::None));
    wpi.setAttribute("publish-url", m_workPackageInfo.publishUrl.toString(QUrl::None));

    QDomElement tm = settingsElement.ownerDocument().createElement("task-modules");
    settingsElement.appendChild(tm);
    tm.setAttribute("use-local-task-modules", m_useLocalTaskModules);
    for (const QUrl &url : taskModules(false/*no local*/)) {
        QDomElement e = tm.ownerDocument().createElement("task-module");
        tm.appendChild(e);
        e.setAttribute("url", url.toString());
    }
    // save standard worktime
    if (m_standardWorktime) {
        m_standardWorktime->save(settingsElement);
    }
}

void Project::save(QDomElement &element, const XmlSaveContext &context) const
{
    debugPlanXml<<context.options;

    QDomElement me = element.ownerDocument().createElement("project");
    element.appendChild(me);

    me.setAttribute("name", m_name);
    me.setAttribute("leader", m_leader);
    me.setAttribute("id", m_id);
    me.setAttribute("priority", QString::number(m_priority));
    me.setAttribute("description", m_description);
    me.setAttribute("timezone", m_timeZone.isValid() ? QString::fromLatin1(m_timeZone.id()) : QString());

    me.setAttribute("scheduling", constraintToString());
    me.setAttribute("start-time", m_constraintStartTime.toString(Qt::ISODate));
    me.setAttribute("end-time", m_constraintEndTime.toString(Qt::ISODate));

    saveSettings(me, context);
    m_documents.save(me); // project documents

    if (context.saveAll(this)) {
        debugPlanXml<<"accounts";
        m_accounts.save(me);

        // save calendars
        debugPlanXml<<"calendars:"<<calendarIdDict.count();
        if (!calendarIdDict.isEmpty()) {
            QDomElement ce = me.ownerDocument().createElement("calendars");
            me.appendChild(ce);
            foreach (Calendar *c, calendarIdDict) {
                c->save(ce);
            }
        }
        // save project resources
        debugPlanXml<<"resource-groups:"<<m_resourceGroups.count();
        if (!m_resourceGroups.isEmpty()) {
            QDomElement ge = me.ownerDocument().createElement("resource-groups");
            me.appendChild(ge);
            QListIterator<ResourceGroup*> git(m_resourceGroups);
            while (git.hasNext()) {
                git.next()->save(ge);
            }
        }
        debugPlanXml<<"resources:"<<m_resources.count();
        if (!m_resources.isEmpty()) {
            QDomElement re = me.ownerDocument().createElement("resources");
            me.appendChild(re);
            QListIterator<Resource*> rit(m_resources);
            while (rit.hasNext()) {
                rit.next()->save(re);
            }
        }
        debugPlanXml<<"resource-group-relations";
        if (!m_resources.isEmpty() && !m_resourceGroups.isEmpty()) {
            QDomElement e = me.ownerDocument().createElement("resource-group-relations");
            me.appendChild(e);
            for (ResourceGroup *g : m_resourceGroups) {
                for (Resource *r : g->resources()) {
                    QDomElement re = e.ownerDocument().createElement("resource-group-relation");
                    e.appendChild(re);
                    re.setAttribute("group-id", g->id());
                    re.setAttribute("resource-id", r->id());
                }
            }
        }
        debugPlanXml<<"required-resources";
        if (m_resources.count() > 1) {
            QList<std::pair<QString, QString> > requiredList;
            for (Resource *resource : m_resources) {
                for (const QString &required : resource->requiredIds()) {
                    requiredList << std::pair<QString, QString>(resource->id(), required);
                }
            }
            if (!requiredList.isEmpty()) {
                QDomElement e = me.ownerDocument().createElement("required-resources");
                me.appendChild(e);
                for (const std::pair<QString, QString> pair : requiredList) {
                    QDomElement re = e.ownerDocument().createElement("required-resource");
                    e.appendChild(re);
                    re.setAttribute("resource-id", pair.first);
                    re.setAttribute("required-id", pair.second);
                }
            }
        }
        // save resource teams
        debugPlanXml<<"resource-teams";
        QDomElement el = me.ownerDocument().createElement("resource-teams");
        me.appendChild(el);
        foreach (Resource *r, m_resources) {
            if (r->type() != Resource::Type_Team) {
                continue;
            }
            foreach (const QString &id, r->teamMemberIds()) {
                QDomElement e = el.ownerDocument().createElement("team");
                el.appendChild(e);
                e.setAttribute("team-id", r->id());
                e.setAttribute("member-id", id);
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
            QDomElement e = me.ownerDocument().createElement("external-appointments");
            me.appendChild(e);
            for (Resource *resource : externals) {
                const QMap<QString, QString> projects = resource->externalProjects();
                QMap<QString, QString>::const_iterator it;
                for (it = projects.constBegin(); it != projects.constEnd(); ++it) {
                    QDomElement re = e.ownerDocument().createElement("external-appointment");
                    e.appendChild(re);
                    re.setAttribute("resource-id", resource->id());
                    re.setAttribute("project-id", it.key());
                    re.setAttribute("project-name", it.value());
                    resource->externalAppointments(it.key()).saveXML(e);
                }
            }
        }
        debugPlanXml<<"tasks:"<<numChildren();
        if (numChildren() > 0) {
            QDomElement e = me.ownerDocument().createElement("tasks");
            me.appendChild(e);
            for (int i = 0; i < numChildren(); i++) {
                childNode(i)->save(e, context);
            }
        }
        // Now we can save relations assuming no tasks have relations outside the project
        QDomElement deps = me.ownerDocument().createElement("relations");
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
            QDomElement el = me.ownerDocument().createElement("project-schedules");
            me.appendChild(el);
            foreach (ScheduleManager *sm, m_managers) {
                sm->saveXML(el);
            }
        }
        // save resource requests
        QHash<Task*, ResourceRequest*> resources;
        for (Task *task : allTasks()) {
            const ResourceRequestCollection &requests = task->requests();
            for (ResourceRequest *rr : requests.resourceRequests(false)) {
                resources.insert(task, rr);
            }
        }
        QHash<Task*, std::pair<ResourceRequest*, Resource*> > required; // QHash<Task*, std::pair<ResourceRequest*, Required*>>
        QHash<Task*, std::pair<ResourceRequest*, ResourceRequest*> > alternativeRequests; // QHash<Task*, std::pair<ResourceRequest*, Alternative*>>
        debugPlanXml<<"resource-requests:"<<resources.count();
        if (!resources.isEmpty()) {
            QDomElement el = me.ownerDocument().createElement("resource-requests");
            me.appendChild(el);
            QHash<Task*, ResourceRequest*>::const_iterator it;
            for (it = resources.constBegin(); it != resources.constEnd(); ++it) {
                if (!it.value()->resource()) {
                    continue;
                }
                QDomElement re = el.ownerDocument().createElement("resource-request");
                el.appendChild(re);
                re.setAttribute("request-id", it.value()->id());
                re.setAttribute("task-id", it.key()->id());
                re.setAttribute("resource-id", it.value()->resource()->id());
                re.setAttribute("units", QString::number(it.value()->units()));
                // collect required resources
                for (Resource *r : it.value()->requiredResources()) {
                    required.insert(it.key(), std::pair<ResourceRequest*, Resource*>(it.value(), r));
                }
                for (ResourceRequest *r : it.value()->alternativeRequests()) {
                    alternativeRequests.insert(it.key(), std::pair<ResourceRequest*, ResourceRequest*>(it.value(), r));
                }
            }
        }
        debugPlanXml<<"required-resource-requests:"<<required.count();
        if (!required.isEmpty()) {
            QDomElement reqs = me.ownerDocument().createElement("required-resource-requests");
            me.appendChild(reqs);
            QHash<Task*, std::pair<ResourceRequest*, Resource*> >::const_iterator it;
            for (it = required.constBegin(); it != required.constEnd(); ++it) {
                QDomElement req = reqs.ownerDocument().createElement("required-resource");
                reqs.appendChild(req);
                req.setAttribute("task-id", it.key()->id());
                req.setAttribute("request-id", it.value().first->id());
                req.setAttribute("required-id", it.value().second->id());
            }
        }
        debugPlanXml<<"alternative-requests:"<<alternativeRequests.count();
        if (!required.isEmpty()) {
            QDomElement reqs = me.ownerDocument().createElement("alternative-requests");
            me.appendChild(reqs);
            QHash<Task*, std::pair<ResourceRequest*, ResourceRequest*> >::const_iterator it;
            for (it = alternativeRequests.constBegin(); it != alternativeRequests.constEnd(); ++it) {
                QDomElement req = reqs.ownerDocument().createElement("alternative-request");
                reqs.appendChild(req);
                req.setAttribute("task-id", it.key()->id());
                req.setAttribute("request-id", it.value().first->id());
                req.setAttribute("alternative-id", it.value().second->id());
            }
        }
    }
}

void Project::saveWorkPackageXML(QDomElement &element, const Node *node, long id) const
{
    QDomElement me = element.ownerDocument().createElement("project");
    element.appendChild(me);

    me.setAttribute("name", m_name);
    me.setAttribute("leader", m_leader);
    me.setAttribute("id", m_id);
    me.setAttribute("description", m_description);
    me.setAttribute("timezone", m_timeZone.isValid() ? QString::fromLatin1(m_timeZone.id()) : QString());

    me.setAttribute("scheduling", constraintToString());
    me.setAttribute("start-time", m_constraintStartTime.toString(Qt::ISODate));
    me.setAttribute("end-time", m_constraintEndTime.toString(Qt::ISODate));

    QListIterator<ResourceGroup*> git(m_resourceGroups);
    while (git.hasNext()) {
        git.next() ->saveWorkPackageXML(me, node->assignedResources(id));
    }

    if (node == 0) {
        return;
    }
    node->saveWorkPackageXML(me, id);

    foreach (ScheduleManager *sm, m_managerIdMap) {
        if (sm->scheduleId() == id) {
            QDomElement el = me.ownerDocument().createElement("schedules");
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

void Project::addResourceGroup(ResourceGroup *group, int index)
{
    int i = index == -1 ? m_resourceGroups.count() : index;
    emit resourceGroupToBeAdded(this, i);
    m_resourceGroups.insert(i, group);
    setResourceGroupId(group);
    group->setProject(this);
    foreach (Resource *r, group->resources()) {
        setResourceId(r);
        r->setProject(this);
    }
    emit resourceGroupAdded(group);
    emit projectChanged();
}

ResourceGroup *Project::takeResourceGroup(ResourceGroup *group)
{
    int i = m_resourceGroups.indexOf(group);
    Q_ASSERT(i != -1);
    if (i == -1) {
        return 0;
    }
    emit resourceGroupToBeRemoved(this, i, group);
    ResourceGroup *g = m_resourceGroups.takeAt(i);
    Q_ASSERT(group == g);
    g->setProject(0);
    removeResourceGroupId(g->id());
    foreach (Resource *r, g->resources()) {
        r->removeParentGroup(g);
    }
    emit resourceGroupRemoved();
    emit projectChanged();
    return g;
}

QList<ResourceGroup*> &Project::resourceGroups()
{
    return m_resourceGroups;
}

void Project::addResource(Resource *resource, int index)
{
    int i = index == -1 ? m_resources.count() : index;
    emit resourceToBeAdded(this, i);
    setResourceId(resource);
    m_resources.insert(i, resource);
    resource->setProject(this);
    emit resourceAdded(resource);
    emit projectChanged();
}

bool Project::takeResource(Resource *resource)
{
    int index = m_resources.indexOf(resource);
    emit resourceToBeRemoved(this, index, resource);
    bool result = removeResourceId(resource->id());
    Q_ASSERT(result == true);
    if (!result) {
        warnPlan << "Could not remove resource with id" << resource->id();
    }
    resource->removeRequests(); // not valid anymore
    for (ResourceGroup *g : resource->parentGroups()) {
        g->takeResource(resource);
    }
    bool rem = m_resources.removeOne(resource);
    Q_ASSERT(!m_resources.contains(resource));
    emit resourceRemoved();
    emit projectChanged();
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

QMap< QString, QString > Project::externalProjects() const
{
    QMap< QString, QString > map;
    foreach (Resource *r, resourceList()) {
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
    if (0 == position) {
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
    if (0 == p) {
        p = this;
    }
    if (!registerNodeId(task)) {
        errorPlan << "Failed to register node id, can not add subtask: " << task->name();
        return false;
    }
    int i = index == -1 ? p->numChildren() : index;
    if (emitSignal) emit nodeToBeAdded(p, i);
    p->insertChildNode(i, task);
    connect(this, &Project::standardWorktimeChanged, task, &Node::slotStandardWorktimeChanged);
    if (emitSignal) {
        emit nodeAdded(task);
        emit projectChanged();
        if (p != this && p->numChildren() == 1) {
            emit nodeChanged(p, TypeProperty);
        }
    }
    return true;
}

void Project::takeTask(Node *node, bool emitSignal)
{
    //debugPlan<<node->name();
    Node * parent = node->parentNode();
    if (parent == 0) {
        debugPlan <<"Node must have a parent!";
        return;
    }
    removeId(node->id());
    if (emitSignal) emit nodeToBeRemoved(node);
    disconnect(this, &Project::standardWorktimeChanged, node, &Node::slotStandardWorktimeChanged);
    parent->takeChildNode(node);
    if (emitSignal) {
        emit nodeRemoved(node);
        emit projectChanged();
        if (parent != this && parent->type() != Node::Type_Summarytask) {
            emit nodeChanged(parent, TypeProperty);
        }
    }
}

bool Project::canMoveTask(Node* node, Node *newParent, bool checkBaselined)
{
    //debugPlan<<node->name()<<" to"<<newParent->name();
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
    emit nodeToBeMoved(node, oldPos, newParent, newRow);
    takeTask(node, false);
    addSubTask(node, i, newParent, false);
    emit nodeMoved(node);
    if (oldParent != this && oldParent->numChildren() == 0) {
        emit nodeChanged(oldParent, TypeProperty);
    }
    if (newParent != this && newParent->numChildren() == 1) {
        emit nodeChanged(newParent, TypeProperty);
    }
    return true;
}

bool Project::canIndentTask(Node* node)
{
    if (0 == node) {
        // should always be != 0. At least we would get the Project,
        // but you never know who might change that, so better be careful
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
    if (0 == node) {
        // is always != 0. At least we would get the Project, but you
        // never know who might change that, so better be careful
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
    if (node == 0)
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
    if (node == 0)
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

Node *Project::findNode(const QString &id) const
{
    if (m_parent == 0) {
        if (nodeIdDict.contains(id)) {
            return nodeIdDict[ id ];
        }
        return 0;
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
    if (rn == 0) {
        //debugPlan <<"id=" << node->id() << node->name();
        nodeIdDict.insert(node->id(), node);
        return true;
    }
    if (rn != node) {
        errorPlan << "Id already exists for different task: " << node->id();
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
        foreach (Node *n, p->childNodeIterator()) {
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
    foreach (Node *n, p->childNodeIterator()) {
        if (n->type() == Node::Type_Task || n->type() == Type_Milestone) {
            lst << static_cast<Task*>(n);
        }
        lst += allTasks(n);
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
    if (group == 0) {
        return false;
    }
    if (! group->id().isEmpty()) {
        ResourceGroup *g = findResourceGroup(group->id());
        if (group == g) {
            return true;
        } else if (g == 0) {
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
    foreach (ResourceGroup *g, resourceGroupIdDict) {
        if (g->name() == name) {
            return g;
        }
    }
    return 0;
}

QList<Resource*> Project::autoAllocateResources() const
{
    QList<Resource*> lst;
    foreach (Resource *r, m_resources) {
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
    if (resource == 0) {
        return false;
    }
    if (! resource->id().isEmpty()) {
        Resource *r = findResource(resource->id());
        if (resource == r) {
            return true;
        } else if (r == 0) {
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
    return 0;
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
    if (s == 0) {
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
    foreach (Node *n, childNodeIterator()) {
        e += n->budgetedWorkPerformed(date, id);
    }
    return e;
}

double Project::budgetedCostPerformed(QDate date, long id) const
{
    //debugPlan;
    double c = 0.0;
    foreach (Node *n, childNodeIterator()) {
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
    Q_ASSERT(calendar != 0);
    //debugPlan<<calendar->name()<<","<<(parent?parent->name():"No parent");
    int row = parent == 0 ? m_calendars.count() : parent->calendars().count();
    if (index >= 0 && index < row) {
        row = index;
    }
    emit calendarToBeAdded(parent, row);
    calendar->setProject(this);
    if (parent == 0) {
        calendar->setParentCal(0); // in case
        m_calendars.insert(row, calendar);
    } else {
        calendar->setParentCal(parent, row);
    }
    if (calendar->isDefault()) {
        setDefaultCalendar(calendar);
    }
    setCalendarId(calendar);
    emit calendarAdded(calendar);
    emit projectChanged();
}

void Project::takeCalendar(Calendar *calendar)
{
    emit calendarToBeRemoved(calendar);
    removeCalendarId(calendar->id());
    if (calendar == m_defaultCalendar) {
        m_defaultCalendar = 0;
    }
    if (calendar->parentCal() == 0) {
        int i = indexOf(calendar);
        if (i != -1) {
            m_calendars.removeAt(i);
        }
    } else {
        calendar->setParentCal(0);
    }
    emit calendarRemoved(calendar);
    calendar->setProject(0);
    emit projectChanged();
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
    foreach(Calendar *c, calendarIdDict) {
        if (c->name() == name) {
            return c;
        }
    }
    return 0;
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
    foreach(Calendar *c, calendarIdDict) {
        lst << c->name();
    }
    return lst;
}

bool Project::setCalendarId(Calendar *calendar)
{
    if (calendar == 0) {
        return false;
    }
    if (! calendar->id().isEmpty()) {
        Calendar *c = findCalendar(calendar->id());
        if (calendar == c) {
            return true;
        } else if (c == 0) {
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
    emit defaultCalendarChanged(cal);
    emit projectChanged();
}

void Project::setStandardWorktime(StandardWorktime * worktime)
{
    if (m_standardWorktime != worktime) {
        delete m_standardWorktime;
        m_standardWorktime = worktime;
        m_standardWorktime->setProject(this);
        emit standardWorktimeChanged(worktime);
    }
}

void Project::emitDocumentAdded(Node *node , Document *doc , int index)
{
    emit documentAdded(node, doc, index);
}

void Project::emitDocumentRemoved(Node *node , Document *doc , int index)
{
    emit documentRemoved(node, doc, index);
}

void Project::emitDocumentChanged(Node *node , Document *doc , int index)
{
    emit documentChanged(node, doc, index);
}

bool Project::linkExists(const Node *par, const Node *child) const
{
    if (par == 0 || child == 0 || par == child || par->isDependChildOf(child)) {
        return false;
    }
    foreach (Relation *r, par->dependChildNodes()) {
        if (r->child() == child) {
            return true;
        }
    }
    return false;
}

bool Project::legalToLink(const Node *par, const Node *child) const
{
    //debugPlan<<par.name()<<" ("<<par.numDependParentNodes()<<" parents)"<<child.name()<<" ("<<child.numDependChildNodes()<<" children)";

    if (par == 0 || child == 0 || par == child || par->isDependChildOf(child)) {
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
        foreach (Node *p, par->childNodeIterator()) {
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
    emit wbsDefinitionChanged();
    emit projectChanged();
}

QString Project::generateWBSCode(QList<int> &indexes, bool sortable) const
{
    QString code = m_wbsDefinition.projectCode();
    if (sortable) {
        int fw = (nodeIdDict.count() / 10) + 1;
        QLatin1Char fc('0');
        foreach (int index, indexes) {
            code += ".%1";
            code = code.arg(QString::number(index), fw, fc);
        }
        //debugPlan<<code<<"------------------";
    } else {
        if (! code.isEmpty() && ! indexes.isEmpty()) {
            code += m_wbsDefinition.projectSeparator();
        }
        int level = 1;
        foreach (int index, indexes) {
            code += m_wbsDefinition.code(index + 1, level);
            if (level < indexes.count()) {
                // not last level, add separator also
                code += m_wbsDefinition.separator(level);
            }
            ++level;
        }
    }
    //debugPlan<<code;
    return code;
}

void Project::setCurrentSchedule(long id)
{
    //debugPlan;
    setCurrentSchedulePtr(findSchedule(id));
    Node::setCurrentSchedule(id);
    for (Resource *r : m_resources) {
        r->setCurrentSchedule(id);
    }
    emit currentScheduleChanged();
    emit projectChanged();
}

ScheduleManager *Project::scheduleManager(long id) const
{
    foreach (ScheduleManager *sm, m_managers) {
        if (sm->scheduleId() == id) {
            return sm;
        }
    }
    return 0;
}

ScheduleManager *Project::scheduleManager(const QString &id) const
{
    return m_managerIdMap.value(id);
}

ScheduleManager *Project::findScheduleManagerByName(const QString &name) const
{
    //debugPlan;
    ScheduleManager *m = 0;
    foreach(ScheduleManager *sm, m_managers) {
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
    foreach (ScheduleManager *sm, m_managers) {
        lst << sm;
        lst << sm->allChildren();
    }
    return lst;
}

QString Project::uniqueScheduleName() const {
    //debugPlan;
    QString n = i18n("Plan");
    bool unique = findScheduleManagerByName(n) == 0;
    if (unique) {
        return n;
    }
    n += " %1";
    int i = 1;
    for (; true; ++i) {
        unique = findScheduleManagerByName(n.arg(i)) == 0;
        if (unique) {
            break;
        }
    }
    return n.arg(i);
}

void Project::addScheduleManager(ScheduleManager *sm, ScheduleManager *parent, int index)
{
    int row = parent == 0 ? m_managers.count() : parent->childCount();
    if (index >= 0 && index < row) {
        row = index;
    }
    if (parent == 0) {
        emit scheduleManagerToBeAdded(parent, row);
        m_managers.insert(row, sm);
    } else {
        emit scheduleManagerToBeAdded(parent, row);
        sm->setParentManager(parent, row);
    }
    if (sm->managerId().isEmpty()) {
        sm->setManagerId(uniqueScheduleManagerId());
    }
    Q_ASSERT(! m_managerIdMap.contains(sm->managerId()));
    m_managerIdMap.insert(sm->managerId(), sm);

    emit scheduleManagerAdded(sm);
    emit projectChanged();
    //debugPlan<<"Added:"<<sm->name()<<", now"<<m_managers.count();
}

int Project::takeScheduleManager(ScheduleManager *sm)
{
    foreach (ScheduleManager *s, sm->children()) {
        takeScheduleManager(s);
    }
    if (sm->scheduling()) {
        sm->stopCalculation();
    }
    int index = -1;
    if (sm->parentManager()) {
        int index = sm->parentManager()->indexOf(sm);
        if (index >= 0) {
            emit scheduleManagerToBeRemoved(sm);
            sm->setParentManager(0);
            m_managerIdMap.remove(sm->managerId());
            emit scheduleManagerRemoved(sm);
            emit projectChanged();
        }
    } else {
        index = indexOf(sm);
        if (index >= 0) {
            emit scheduleManagerToBeRemoved(sm);
            m_managers.removeAt(indexOf(sm));
            m_managerIdMap.remove(sm->managerId());
            emit scheduleManagerRemoved(sm);
            emit projectChanged();
        }
    }
    return index;
}

void Project::swapScheduleManagers(ScheduleManager *from, ScheduleManager *to)
{
    emit scheduleManagersSwapped(from, to);
}

void Project::moveScheduleManager(ScheduleManager *sm, ScheduleManager *newparent, int newindex)
{
    //debugPlan<<sm->name()<<newparent<<newindex;
    emit scheduleManagerToBeMoved(sm);
    if (! sm->parentManager()) {
        m_managers.removeAt(indexOf(sm));
    }
    sm->setParentManager(newparent, newindex);
    if (! newparent) {
        m_managers.insert(newindex, sm);
    }
    emit scheduleManagerMoved(sm, newindex);
}

bool Project::isScheduleManager(void *ptr) const
{
    const ScheduleManager *sm = static_cast<ScheduleManager*>(ptr);
    if (indexOf(sm) >= 0) {
        return true;
    }
    foreach (ScheduleManager *p, m_managers) {
        if (p->isParentOf(sm)) {
            return true;
        }
    }
    return false;
}

ScheduleManager *Project::createScheduleManager(const QString &name)
{
    //debugPlan<<name;
    ScheduleManager *sm = new ScheduleManager(*this, name);
    return sm;
}

ScheduleManager *Project::createScheduleManager()
{
    //debugPlan;
    return createScheduleManager(uniqueScheduleName());
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
        foreach (ScheduleManager *sm, allScheduleManagers()) {
            if (sm->isBaselined()) {
                return true;
            }
        }
        return false;
    }
    Schedule *s = schedule(id);
    return s == 0 ? false : s->isBaselined();
}

MainSchedule *Project::createSchedule(const QString& name, Schedule::Type type)
{
    //debugPlan<<"No of schedules:"<<m_schedules.count();
    MainSchedule *sch = new MainSchedule();
    sch->setName(name);
    sch->setType(type);
    addMainSchedule(sch);
    return sch;
}

void Project::addMainSchedule(MainSchedule *sch)
{
    if (sch == 0) {
        return;
    }
    //debugPlan<<"No of schedules:"<<m_schedules.count();
    long i = 1; // keep this positive (negative values are special...)
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
    if (m_parent == 0) {
        Node::changed(node, property); // reset cache
        if (property != Node::TypeProperty) {
            // add/remove node is handled elsewhere
            emit nodeChanged(node, property);
            emit projectChanged();
        }
        return;
    }
    Node::changed(node, property);
}

void Project::changed(ResourceGroup *group)
{
    //debugPlan;
//     emit resourceGroupChanged(group);
    emit projectChanged();
}

void Project::changed(ScheduleManager *sm, int property)
{
    emit scheduleManagerChanged(sm, property);
    emit projectChanged();
}

void Project::changed(MainSchedule *sch)
{
    //debugPlan<<sch->id();
    emit scheduleChanged(sch);
    emit projectChanged();
}

void Project::sendScheduleToBeAdded(const ScheduleManager *sm, int row)
{
    emit scheduleToBeAdded(sm, row);
}

void Project::sendScheduleAdded(const MainSchedule *sch)
{
    //debugPlan<<sch->id();
    emit scheduleAdded(sch);
    emit projectChanged();
}

void Project::sendScheduleToBeRemoved(const MainSchedule *sch)
{
    //debugPlan<<sch->id();
    emit scheduleToBeRemoved(sch);
}

void Project::sendScheduleRemoved(const MainSchedule *sch)
{
    //debugPlan<<sch->id();
    emit scheduleRemoved(sch);
    emit projectChanged();
}

void Project::changed(Resource *resource)
{
//     emit resourceChanged(resource);
    emit projectChanged();
}

void Project::changed(Calendar *cal)
{
    emit calendarChanged(cal);
    emit projectChanged();
}

void Project::changed(StandardWorktime *w)
{
    emit standardWorktimeChanged(w);
    emit projectChanged();
}

bool Project::addRelation(Relation *rel, bool check)
{
    if (rel->parent() == 0 || rel->child() == 0) {
        return false;
    }
    if (check && !legalToLink(rel->parent(), rel->child())) {
        return false;
    }
    emit relationToBeAdded(rel, rel->parent()->numDependChildNodes(), rel->child()->numDependParentNodes());
    rel->parent()->addDependChildNode(rel);
    rel->child()->addDependParentNode(rel);
    emit relationAdded(rel);
    emit projectChanged();
    return true;
}

void Project::takeRelation(Relation *rel)
{
    emit relationToBeRemoved(rel);
    rel->parent() ->takeDependChildNode(rel);
    rel->child() ->takeDependParentNode(rel);
    emit relationRemoved(rel);
    emit projectChanged();
}

void Project::setRelationType(Relation *rel, Relation::Type type)
{
    emit relationToBeModified(rel);
    rel->setType(type);
    emit relationModified(rel);
    emit projectChanged();
}

void Project::setRelationLag(Relation *rel, const Duration &lag)
{
    emit relationToBeModified(rel);
    rel->setLag(lag);
    emit relationModified(rel);
    emit projectChanged();
}

QList<Node*> Project::flatNodeList(Node *parent)
{
    QList<Node*> lst;
    Node *p = parent == 0 ? this : parent;
    //debugPlan<<p->name()<<lst.count();
    foreach (Node *n, p->childNodeIterator()) {
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
    emit localeChanged();
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

void Project::setSharedProjectsUrl(const QUrl &url)
{
    m_sharedProjectsUrl = url;
}

QUrl Project::sharedProjectsUrl() const
{
    return m_sharedProjectsUrl;
}

void Project::setLoadProjectsAtStartup(bool value)
{
    m_loadProjectsAtStartup = value;
}

bool Project::loadProjectsAtStartup() const
{
    return m_loadProjectsAtStartup;
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
    emit taskModulesChanged(m_taskModules);
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
        emit taskModulesChanged(m_taskModules);
    }
}

void Project::setLocalTaskModulesPath(const QUrl &url)
{
    m_taskModules.removeAll(m_localTaskModulesPath);
    m_localTaskModulesPath = url;
    if (m_useLocalTaskModules && url.isValid()) {
        m_taskModules.prepend(url);
    }
    emit taskModulesChanged(m_taskModules);
}

}  //KPlato namespace
