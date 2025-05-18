/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2001 Thomas zander <zander@kde.org>
 * SPDX-FileCopyrightText: 2004-2007 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2007 Florian Piquemal <flotueur@yahoo.fr>
 * SPDX-FileCopyrightText: 2007 Alexis Ménard <darktears31@gmail.com>
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "kpttask.h"
#include "kptappointment.h"
#include "kptproject.h"
#include "kptduration.h"
#include "kptrelation.h"
#include "kptdatetime.h"
#include "kptcalendar.h"
#include "kpteffortcostmap.h"
#include "kptschedule.h"
#include "kptxmlloaderobject.h"
#include "XmlSaveContext.h"
#include <kptdebug.h>

#include <KoXmlReader.h>

#include <KLocalizedString>
#include <KFormat>

#include <QElapsedTimer>

namespace KPlato
{

Task::Task(Node *parent)
    : Node(parent),
      m_resource(),
      m_workPackage(this)
{
    //debugPlan<<"("<<this<<')';
    m_requests.setTask(this);
    Duration d(1, 0, 0);
    m_estimate = new Estimate();
    m_estimate->setOptimisticRatio(-10);
    m_estimate->setPessimisticRatio(20);
    m_estimate->setParentNode(this);

    if (m_parent)
        m_leader = m_parent->leader();
}

Task::Task(const Task &task, Node *parent)
    : Node(task, parent),
      m_resource(),
      m_workPackage(this)
{
    //debugPlan<<"("<<this<<')';
    m_requests.setTask(this);
    delete m_estimate;
    if (task.estimate()) {
        m_estimate = new Estimate(*(task.estimate()));
    } else {
        m_estimate = new Estimate();
    }
    m_estimate->setParentNode(this);
}


Task::~Task() {
    while (!m_resource.isEmpty()) {
        delete m_resource.takeFirst();
    }
    while (!m_parentProxyRelations.isEmpty()) {
        delete m_parentProxyRelations.takeFirst();
    }
    while (!m_childProxyRelations.isEmpty()) {
        delete m_childProxyRelations.takeFirst();
    }
}

int Task::type() const {
    if (numChildren() > 0) {
        return Node::Type_Summarytask;
    } else if (m_constraint == Node::FixedInterval) {
        if (m_constraintEndTime == m_constraintStartTime) {
            return Node::Type_Milestone;
        }
    } else if (m_estimate->expectedEstimate() == 0.0) {
        return Node::Type_Milestone;
    }
    return Node::Type_Task;
}

Duration *Task::getRandomDuration() {
    return nullptr;
}

// void Task::clearResourceRequests() {
//     m_requests.clear();
//     changed(this, ResourceRequestProperty);
// }

QStringList Task::requestNameList() const {
    return m_requests.requestNameList();
}

QList<Resource*> Task::requestedResources() const {
    return m_requests.requestedResources();
}

bool Task::containsRequest(const QString &identity) const {
    return m_requests.contains(identity);
}

ResourceRequest *Task::resourceRequest(const QString &name) const {
    return m_requests.resourceRequest(name);
}

QStringList Task::assignedNameList(long id) const {
    Schedule *s = schedule(id);
    if (s == nullptr) {
        return QStringList();
    }
    return s->resourceNameList();
}

void Task::makeAppointments() {
    if (m_currentSchedule == nullptr)
        return;
    if (type() == Node::Type_Task) {
        //debugPlan<<m_name<<":"<<m_currentSchedule->startTime<<","<<m_currentSchedule->endTime<<";"<<m_currentSchedule->duration.toString();
        m_requests.makeAppointments(m_currentSchedule);
        //debugPlan<<m_name<<":"<<m_currentSchedule->startTime<<","<<m_currentSchedule->endTime<<";"<<m_currentSchedule->duration.toString();
    } else if (type() == Node::Type_Summarytask) {
        for (Node *n : std::as_const(m_nodes)) {
            n->makeAppointments();
        }
    } else if (type() == Node::Type_Milestone) {
        //debugPlan<<"Milestone not implemented";
        // Well, shouldn't have resources anyway...
    }
}

void Task::copySchedule()
{
    if (m_currentSchedule == nullptr || type() != Node::Type_Task) {
        return;
    }
    int parentId = m_currentSchedule->parentScheduleId();
    NodeSchedule *parentSchedule = static_cast<NodeSchedule*>(findSchedule(parentId));
    if (parentSchedule == nullptr) {
        warnPlan<<Q_FUNC_INFO<<this<<"No parent schedule to copy from";
        return;
    }
    if (type() == Node::Type_Task) {
        copyAppointmentsFromParentSchedule(parentSchedule->startTime, parentSchedule->endTime);
    }
    m_currentSchedule->startTime = parentSchedule->startTime;
    m_currentSchedule->earlyStart = parentSchedule->earlyStart;
    m_currentSchedule->endTime = parentSchedule->endTime;
    m_currentSchedule->lateFinish = parentSchedule->lateFinish;
    m_currentSchedule->duration = parentSchedule->duration;
    // TODO: status flags, etc
    //debugPlan;
}

void Task::copyAppointments()
{
    if (!isStarted() || completion().isFinished()) {
        warnPlan<<Q_FUNC_INFO<<this<<"Not started or already finished";
        return;
    }
    int id = m_currentSchedule->parentScheduleId();
    NodeSchedule *parentSchedule = static_cast<NodeSchedule*>(findSchedule(id));
    if (parentSchedule == nullptr) {
        warnPlan<<Q_FUNC_INFO<<"no parent schedule to copy from";
        m_currentSchedule->logDebug(QStringLiteral("No parent schedule found"));
        return;
    }
    DateTime time = m_currentSchedule->recalculateFrom();
    qreal plannedEffort = parentSchedule->plannedEffortTo(time).toDouble();
    if (plannedEffort == 0.0) {
        // This *could* happen if work has been done on a task that is planned
        // to start later than the recalculation date.
        // This is probably not a situation that will happen in real life, so we do nothing for now.
        // Possibly one could use the tasks recalculated start time as limit, but let's see...
        warnPlan<<Q_FUNC_INFO<<this<<"No planned effort up to recalculation date:"<<time;
        m_currentSchedule->logDebug(QStringLiteral("No planned effort at this time: %1").arg(time.toString()));
        return;
    }
    createAndMergeAppointmentsFromCompletion();
}

void Task::createAndMergeAppointmentsFromCompletion()
{
    if (m_currentSchedule == nullptr || type() != Node::Type_Task) {
        return;
    }
    int id = m_currentSchedule->parentScheduleId();
    NodeSchedule *parentSchedule = static_cast<NodeSchedule*>(findSchedule(id));
    if (parentSchedule == nullptr) {
        m_currentSchedule->logDebug(QStringLiteral("AppointmentsFromCompletion: No parent schedule found"));
        return;
    }
    const auto appointments = completion().createAppointments();
    QHash<Resource*, Appointment>::const_iterator it;
    for (it = appointments.constBegin(); it != appointments.constEnd(); ++it) {
        auto resource = it.key();
        auto appointment = it.value();
        auto resourceSchedule = resource->findSchedule(currentSchedule()->id());
        if (!resourceSchedule) {
            warnPlan<<"No resourceSchedule, creating";
            resourceSchedule = resource->createSchedule(currentSchedule()->name(), currentSchedule()->type(), currentSchedule()->id());
            resource->addSchedule(resourceSchedule);
        }
        appointment.setNode(currentSchedule());
        appointment.setResource(resourceSchedule);
        // find appointment to merge with
        Appointment *curr = nullptr;
        const auto currAppointments = m_currentSchedule->appointments();
        for (Appointment *c : currAppointments ) {
            if (c->resource() == appointment.resource()) {
                //debugPlan<<"Found current appointment to"<<a->resource()->resource()<<c;
                curr = c;
                break;
            }
        }
        if (curr == nullptr) {
            // A resource that is not planned to work, has done work on this task
            m_currentSchedule->logDebug(QStringLiteral("%1: Worked on task, but not planned to, so create appointment"));
            curr = new Appointment();
            m_currentSchedule->add(curr);
            curr->setNode(m_currentSchedule);
            auto resource = appointment.resource()->resource();
            ResourceSchedule *rs = static_cast<ResourceSchedule*>(resource->findSchedule(m_currentSchedule->id()));
            if (rs == nullptr) {
                rs = resource->createSchedule(m_currentSchedule->parent());
                rs->setId(m_currentSchedule->id());
                rs->setName(m_currentSchedule->name());
                rs->setType(m_currentSchedule->type());
                //debugPlan<<"Resource schedule not found, id="<<m_currentSchedule->id();
            }
            rs->setCalculationMode(m_currentSchedule->calculationMode());
            if (!rs->appointments().contains(curr)) {
                //debugPlan<<"add to resource"<<rs<<curr;
                rs->add(curr);
                curr->setResource(rs);
            }
            m_currentSchedule->logDebug(QStringLiteral("%1: Created appointment").arg(curr->resource()->name()));
            //debugPlan<<"Created new appointment"<<curr;
        }
        m_currentSchedule->logDebug(QStringLiteral("%1: Merging appointment: %2 - %3").arg(resource->name()).arg(appointment.startTime().toString(Qt::ISODate)).arg(appointment.endTime().toString(Qt::ISODate)));
        curr->merge(appointment);
        //debugPlan<<"Appointments added:"<<curr;
    }
    m_currentSchedule->startTime = DateTime();
    const auto apps = m_currentSchedule->appointments();
    for (const auto appointment : apps) {
        if (!m_currentSchedule->startTime.isValid() || m_currentSchedule->startTime > appointment->startTime()) {
            m_currentSchedule->startTime = appointment->startTime();
        }
    }
    m_currentSchedule->earlyStart = m_currentSchedule->startTime;
}

void Task::copyAppointmentsFromParentSchedule(const DateTime &start, const DateTime &end)
{
    if (m_currentSchedule == nullptr || type() != Node::Type_Task) {
        return;
    }
    auto parentId = m_currentSchedule->parentScheduleId();
    NodeSchedule *parentSchedule = static_cast<NodeSchedule*>(findSchedule(parentId));
    if (parentSchedule == nullptr) {
        m_currentSchedule->logDebug(QStringLiteral("Cannot copy appointments, no parent schedule found"));
        return;
    }
    DateTime st = start.isValid() ? start : parentSchedule->startTime;
    DateTime et = end.isValid() ? end : parentSchedule->endTime;
    //debugPlan<<m_name<<st.toString()<<et.toString()<<m_currentSchedule->calculationMode();
    const auto appointments = parentSchedule->appointments();
    for (const Appointment *a : appointments) {
        Resource *r = a->resource() == nullptr ? nullptr : a->resource()->resource();
        if (r == nullptr) {
            errorPlan<<"No resource";
            continue;
        }
        AppointmentIntervalList lst;
        const auto intervals = a->intervals(st, et).map();
        for (AppointmentInterval i : intervals) {
            i.setLoad(i.load());
            lst.add(i);
        }
        if (lst.isEmpty()) {
            //debugPlan<<"No intervals to copy from"<<a;
            m_currentSchedule->logDebug(QStringLiteral("%1: No appointment intervals to copy").arg(r->name()));
            continue;
        }
        Appointment *curr = nullptr;
        const auto appointments2 = m_currentSchedule->appointments();
        for (Appointment *c : appointments2 ) {
            if (c->resource()->resource() == r) {
                //debugPlan<<"Found current appointment to"<<a->resource()->resource()->name()<<c;
                curr = c;
                break;
            }
        }
        if (curr == nullptr) {
            curr = new Appointment();
            m_currentSchedule->add(curr);
            curr->setNode(m_currentSchedule);
            //debugPlan<<"Created new appointment"<<curr;
            m_currentSchedule->logDebug(QStringLiteral("%1: Created new appointment").arg(r->name()));
        }
        ResourceSchedule *rs = static_cast<ResourceSchedule*>(r->findSchedule(m_currentSchedule->id()));
        if (rs == nullptr) {
            rs = r->createSchedule(m_currentSchedule->parent());
            rs->setId(m_currentSchedule->id());
            rs->setName(m_currentSchedule->name());
            rs->setType(m_currentSchedule->type());
            //debugPlan<<"Resource schedule not found, id="<<m_currentSchedule->id();
            m_currentSchedule->logDebug(QStringLiteral("%1: Created new resource schedule").arg(r->name()));
        }
        rs->setCalculationMode(m_currentSchedule->calculationMode());
        if (!rs->appointments().contains(curr)) {
            //debugPlan<<"add to resource"<<rs<<curr;
            rs->add(curr);
            curr->setResource(rs);
        }
        Appointment app;
        app.setIntervals(lst);
        m_currentSchedule->logDebug(QStringLiteral("%1: Appointments to be added: %2 - %3").arg(r->name()).arg(app.startTime().toString()).arg(app.endTime().toString()));
        //debugPlan<<"Add appointments:"<<app;
        curr->merge(app);
        //debugPlan<<"Appointments added";
    }
    m_currentSchedule->startTime = parentSchedule->startTime;
    m_currentSchedule->earlyStart = parentSchedule->earlyStart;
}

void Task::calcResourceOverbooked() {
    if (m_currentSchedule)
        m_currentSchedule->calcResourceOverbooked();
}

void Task::save(QDomElement &element, const XmlSaveContext &context)  const
{
    if (!context.saveNode(this)) {
        return;
    }
    QDomElement me = element.ownerDocument().createElement(QStringLiteral("task"));
    element.appendChild(me);

    me.setAttribute(QStringLiteral("id"), m_id);
    me.setAttribute(QStringLiteral("priority"), QString::number(m_priority));
    me.setAttribute(QStringLiteral("name"), m_name);
    me.setAttribute(QStringLiteral("leader"), m_leader);
    me.setAttribute(QStringLiteral("description"), m_description);

    me.setAttribute(QStringLiteral("scheduling"),constraintToString());
    me.setAttribute(QStringLiteral("constraint-starttime"),m_constraintStartTime.toString(Qt::ISODate));
    me.setAttribute(QStringLiteral("constraint-endtime"),m_constraintEndTime.toString(Qt::ISODate));

    me.setAttribute(QStringLiteral("startup-cost"), QString::number(m_startupCost));
    me.setAttribute(QStringLiteral("shutdown-cost"), QString::number(m_shutdownCost));

    me.setAttribute(QStringLiteral("wbs"), wbsCode()); //NOTE: included for information

    m_estimate->save(me);

    m_documents.save(me);

    if (context.saveAll(this)) {
        if (!m_schedules.isEmpty()) {
            QDomElement schs = me.ownerDocument().createElement(QStringLiteral("task-schedules"));
            me.appendChild(schs);
            for (const Schedule *s : std::as_const(m_schedules)) {
                if (!s->isDeleted()) {
                    s->saveXML(schs);
                }
            }
        }
        completion().saveXML(me);

        m_workPackage.saveXML(me);
        // The workpackage log
        if (!m_packageLog.isEmpty()) {
            QDomElement log = me.ownerDocument().createElement(QStringLiteral("workpackage-log"));
            me.appendChild(log);
            for (const WorkPackage *wp : std::as_const(m_packageLog)) {
                wp->saveLoggedXML(log);
            }
        }
    }
    if (context.saveChildren(this)) {
        for (int i=0; i<numChildren(); i++) {
            childNode(i)->save(me, context);
        }
    }
}

void Task::saveAppointments(QDomElement &element, long id) const {
    //debugPlan<<m_name<<" id="<<id;
    Schedule *sch = findSchedule(id);
    if (sch) {
        sch->saveAppointments(element);
    }
    for (const Node *n : std::as_const(m_nodes)) {
        n->saveAppointments(element, id);
    }
}

void Task::saveWorkPackageXML(QDomElement &element, long id)  const
{
    QDomElement me = element.ownerDocument().createElement(QStringLiteral("task"));
    element.appendChild(me);

    me.setAttribute(QStringLiteral("id"), m_id);
    me.setAttribute(QStringLiteral("name"), m_name);
    me.setAttribute(QStringLiteral("leader"), m_leader);
    me.setAttribute(QStringLiteral("description"), m_description);

    me.setAttribute(QStringLiteral("scheduling"),constraintToString());
    me.setAttribute(QStringLiteral("constraint-starttime"),m_constraintStartTime.toString(Qt::ISODate));
    me.setAttribute(QStringLiteral("constraint-endtime"),m_constraintEndTime.toString(Qt::ISODate));

    me.setAttribute(QStringLiteral("startup-cost"), QString::number(m_startupCost));
    me.setAttribute(QStringLiteral("shutdown-cost"), QString::number(m_shutdownCost));

    me.setAttribute(QStringLiteral("wbs"), wbsCode()); // NOTE: included for information

    m_estimate->save(me);

    completion().saveXML(me);
    if (id == ANYSCHEDULED) {
        for (const auto s : m_schedules) {
            if (!s->isDeleted()) {
                id = s->id();
            }
        }
    }
    if (m_schedules.contains(id) && ! m_schedules[ id ]->isDeleted()) {
        QDomElement schs = me.ownerDocument().createElement(QStringLiteral("task-schedules"));
        me.appendChild(schs);
        m_schedules[ id ]->saveXML(schs);
    }
    m_documents.save(me); // TODO: copying documents
}

bool Task::isStarted() const
{
    return completion().isStarted();
}

EffortCostMap Task::plannedEffortCostPrDay(QDate start, QDate end, long id, EffortCostCalculationType typ) const {
    //debugPlan<<m_name;
    if (type() == Node::Type_Summarytask) {
        EffortCostMap ec;
        QListIterator<Node*> it(childNodeIterator());
        while (it.hasNext()) {
            ec += it.next() ->plannedEffortCostPrDay(start, end, id, typ);
        }
        return ec;
    }
    Schedule *s = schedule(id);
    if (s) {
        return s->plannedEffortCostPrDay(start, end, typ);
    }
    return EffortCostMap();
}

EffortCostMap Task::plannedEffortCostPrDay(const Resource *resource, QDate start, QDate end, long id, EffortCostCalculationType typ) const {
    //debugPlan<<m_name;
    if (type() == Node::Type_Summarytask) {
        EffortCostMap ec;
        QListIterator<Node*> it(childNodeIterator());
        while (it.hasNext()) {
            ec += it.next() ->plannedEffortCostPrDay(resource, start, end, id, typ);
        }
        return ec;
    }
    Schedule *s = schedule(id);
    if (s) {
        return s->plannedEffortCostPrDay(resource, start, end, typ);
    }
    return EffortCostMap();
}

EffortCostMap Task::actualEffortCostPrDay(QDate start, QDate end, long id, EffortCostCalculationType typ) const {
    //debugPlan<<m_name;
    if (type() == Node::Type_Summarytask) {
        EffortCostMap ec;
        QListIterator<Node*> it(childNodeIterator());
        while (it.hasNext()) {
            ec += it.next() ->actualEffortCostPrDay(start, end, id, typ);
        }
        return ec;
    }
    switch (completion().entrymode()) {
        case Completion::FollowPlan:
            return plannedEffortCostPrDay(start, end, id, typ);
        default:
            return completion().effortCostPrDay(start, end, id);
    }
    return EffortCostMap();
}

EffortCostMap Task::actualEffortCostPrDay(const Resource *resource, QDate start, QDate end, long id, EffortCostCalculationType typ) const {
    //debugPlan<<m_name;
    if (type() == Node::Type_Summarytask) {
        EffortCostMap ec;
        QListIterator<Node*> it(childNodeIterator());
        while (it.hasNext()) {
            ec += it.next() ->actualEffortCostPrDay(resource, start, end, id, typ);
        }
        return ec;
    }
    switch (completion().entrymode()) {
        case Completion::FollowPlan:
            return plannedEffortCostPrDay(resource, start, end, id, typ);
        default:
            return completion().effortCostPrDay(resource, start, end);
    }
    return EffortCostMap();
}

// Returns the total planned effort for this task (or subtasks)
Duration Task::plannedEffort(const Resource *resource, long id, EffortCostCalculationType typ) const {
   //debugPlan;
    Duration eff;
    if (type() == Node::Type_Summarytask) {
        for (const Node *n : std::as_const(m_nodes)) {
            eff += n->plannedEffort(resource, id, typ);
        }
        return eff;
    }
    Schedule *s = schedule(id);
    if (s) {
        eff = s->plannedEffort(resource, typ);
    }
    return eff;
}

// Returns the total planned effort for this task (or subtasks)
Duration Task::plannedEffort(long id, EffortCostCalculationType typ) const {
   //debugPlan;
    Duration eff;
    if (type() == Node::Type_Summarytask) {
        for (const Node *n : std::as_const(m_nodes)) {
            eff += n->plannedEffort(id, typ);
        }
        return eff;
    }
    Schedule *s = schedule(id);
    if (s) {
        eff = s->plannedEffort(typ);
    }
    return eff;
}

// Returns the total planned effort for this task (or subtasks) on date
Duration Task::plannedEffort(const Resource *resource, QDate date, long id, EffortCostCalculationType typ) const {
   //debugPlan;
    Duration eff;
    if (type() == Node::Type_Summarytask) {
        for (const Node *n : std::as_const(m_nodes)) {
            eff += n->plannedEffort(resource, date, id, typ);
        }
        return eff;
    }
    Schedule *s = schedule(id);
    if (s) {
        eff = s->plannedEffort(resource, date, typ);
    }
    return eff;
}

// Returns the total planned effort for this task (or subtasks) on date
Duration Task::plannedEffort(QDate date, long id, EffortCostCalculationType typ) const {
   //debugPlan;
    Duration eff;
    if (type() == Node::Type_Summarytask) {
        for (const Node *n : std::as_const(m_nodes)) {
            eff += n->plannedEffort(date, id, typ);
        }
        return eff;
    }
    Schedule *s = schedule(id);
    if (s) {
        eff = s->plannedEffort(date, typ);
    }
    return eff;
}

// Returns the total planned effort for this task (or subtasks) upto and including date
Duration Task::plannedEffortTo(QDate date, long id, EffortCostCalculationType typ) const {
    //debugPlan;
    Duration eff;
    if (type() == Node::Type_Summarytask) {
        for (const Node *n : std::as_const(m_nodes)) {
            eff += n->plannedEffortTo(date, id, typ);
        }
        return eff;
    }
    Schedule *s = schedule(id);
    if (s) {
        eff = s->plannedEffortTo(date, typ);
    }
    return eff;
}

// Returns the total planned effort for this task (or subtasks) upto and including date
Duration Task::plannedEffortTo(const Resource *resource, QDate date, long id, EffortCostCalculationType typ) const {
    //debugPlan;
    Duration eff;
    if (type() == Node::Type_Summarytask) {
        for (const Node *n : std::as_const(m_nodes)) {
            eff += n->plannedEffortTo(resource, date, id, typ);
        }
        return eff;
    }
    Schedule *s = schedule(id);
    if (s) {
        eff = s->plannedEffortTo(resource, date, typ);
    }
    return eff;
}

// Returns the total actual effort for this task (or subtasks)
Duration Task::actualEffort() const {
   //debugPlan;
    Duration eff;
    if (type() == Node::Type_Summarytask) {
        for (const Node *n : std::as_const(m_nodes)) {
            eff += n->actualEffort();
        }
    }
    return completion().actualEffort();
}

// Returns the total actual effort for this task (or subtasks) on date
Duration Task::actualEffort(QDate date) const {
   //debugPlan;
    Duration eff;
    if (type() == Node::Type_Summarytask) {
        for (const Node *n : std::as_const(m_nodes)) {
            eff += n->actualEffort(date);
        }
        return eff;
    }
    return completion().actualEffort(date);
}

// Returns the total actual effort for this task (or subtasks) to date
Duration Task::actualEffortTo(QDate date) const {
   //debugPlan;
    Duration eff;
    if (type() == Node::Type_Summarytask) {
        for (const Node *n : std::as_const(m_nodes)) {
            eff += n->actualEffortTo(date);
        }
        return eff;
    }
    return completion().actualEffortTo(date);
}

EffortCost Task::plannedCost(long id, EffortCostCalculationType typ) const {
    //debugPlan;
    if (type() == Node::Type_Summarytask) {
        return Node::plannedCost(id, typ);
    }
    EffortCost c;
    Schedule *s = schedule(id);
    if (s) {
        c = s->plannedCost(typ);
    }
    return c;
}

double Task::plannedCostTo(QDate date, long id, EffortCostCalculationType typ) const {
    //debugPlan;
    double c = 0;
    if (type() == Node::Type_Summarytask) {
        for (const Node *n : std::as_const(m_nodes)) {
            c += n->plannedCostTo(date, id, typ);
        }
        return c;
    }
    Schedule *s = schedule(id);
    if (s == nullptr) {
        return c;
    }
    c = s->plannedCostTo(date, typ);
    if (date >= s->startTime.date()) {
        c += m_startupCost;
    }
    if (date >= s->endTime.date()) {
        c += m_shutdownCost;
    }
    return c;
}

EffortCost Task::actualCostTo(long int id, QDate date) const {
    //debugPlan;
    EffortCostMap ecm = acwp(id);
    return EffortCost(ecm.effortTo(date), ecm.costTo(date));
}

double Task::bcws(QDate date, long id) const
{
    //debugPlan;
    double c = plannedCostTo(date, id);
    //debugPlan<<c;
    return c;
}

EffortCostMap Task::bcwsPrDay(long int id, EffortCostCalculationType typ)
{
    //debugPlan;
    if (type() == Node::Type_Summarytask) {
        return Node::bcwsPrDay(id);
    }
    Schedule *s = schedule(id);
    if (s == nullptr) {
        return EffortCostMap();
    }
    EffortCostCache &cache = s->bcwsPrDayCache(typ);
    if (! cache.cached) {
        EffortCostMap ec = s->bcwsPrDay(typ);
        if (typ != ECCT_Work) {
            if (m_startupCost > 0.0) {
                ec.add(s->startTime.date(), Duration::zeroDuration, m_startupCost);
            }
            if (m_shutdownCost > 0.0) {
                ec.add(s->endTime.date(), Duration::zeroDuration, m_shutdownCost);
            }
            cache.effortcostmap = ec;
            cache.cached = true;
        }
    }
    return cache.effortcostmap;
}

EffortCostMap Task::bcwpPrDay(long int id, EffortCostCalculationType typ)
{
    //debugPlan;
    if (type() == Node::Type_Summarytask) {
        return Node::bcwpPrDay(id, typ);
    }
    Schedule *s = schedule(id);
    if (s == nullptr) {
        return EffortCostMap();
    }
    EffortCostCache cache = s->bcwpPrDayCache(typ);
    if (! cache.cached) {
        // do not use bcws cache, it includes startup/shutdown cost
        EffortCostMap e = s->plannedEffortCostPrDay(s->appointmentStartTime().date(), s->appointmentEndTime().date(), typ);
        if (completion().isStarted() && ! e.isEmpty()) {
            // calculate bcwp on bases of bcws *without* startup/shutdown cost
            double totEff = e.totalEffort().toDouble(Duration::Unit_h);
            double totCost = e.totalCost();
            QDate sd = completion().entries().keys().value(0);
            if (! sd.isValid() || e.startDate() < sd) {
                sd = e.startDate();
            }
            QDate ed = qMax(e.endDate(), completion().entryDate());
            for (QDate d = sd; d <= ed; d = d.addDays(1)) {
                double p = (double)(completion().percentFinished(d)) / 100.0;
                EffortCost ec = e.days()[ d ];
                ec.setBcwpEffort(totEff  * p);
                ec.setBcwpCost(totCost  * p);
                e.insert(d, ec);
            }
        }
        if (typ != ECCT_Work) {
            // add bcws startup/shutdown cost
            if (m_startupCost > 0.0) {
                e.add(s->startTime.date(), Duration::zeroDuration, m_startupCost);
            }
            if (m_shutdownCost > 0.0) {
                e.add(s->endTime.date(), Duration::zeroDuration, m_shutdownCost);
            }
            // add bcwp startup/shutdown cost
            if (m_shutdownCost > 0.0 && completion().finishIsValid()) {
                QDate finish = completion().finishTime().date();
                e.addBcwpCost(finish, m_shutdownCost);
                debugPlan<<"addBcwpCost:"<<finish<<m_shutdownCost;
                // bcwp is cumulative so add to all entries after finish (in case task finished early)
                for (EffortCostDayMap::const_iterator it = e.days().constBegin(); it != e.days().constEnd(); ++it) {
                    const QDate date = it.key();
                    if (date > finish) {
                        e.addBcwpCost(date, m_shutdownCost);
                        debugPlan<<"addBcwpCost:"<<date<<m_shutdownCost;
                    }
                }
            }
            if (m_startupCost > 0.0 && completion().startIsValid()) {
                QDate start = completion().startTime().date();
                e.addBcwpCost(start, m_startupCost);
                // bcwp is cumulative so add to all entries after start
                for (EffortCostDayMap::const_iterator it = e.days().constBegin(); it != e.days().constEnd(); ++it) {
                    const QDate date = it.key();
                    if (date > start) {
                        e.addBcwpCost(date, m_startupCost);
                    }
                }
            }
        }
        cache.effortcostmap = e;
        cache.cached = true;
    }
    return cache.effortcostmap;
}

Duration Task::budgetedWorkPerformed(QDate date, long id) const
{
    //debugPlan;
    Duration e;
    if (type() == Node::Type_Summarytask) {
        for (const Node *n : std::as_const(m_nodes)) {
            e += n->budgetedWorkPerformed(date, id);
        }
        return e;
    }

    e = plannedEffort(id) * (double)completion().percentFinished(date) / 100.0;
    //debugPlan<<m_name<<"("<<id<<")"<<date<<"="<<e.toString();
    return e;
}

double Task::budgetedCostPerformed(QDate date, long id) const
{
    //debugPlan;
    double c = 0.0;
    if (type() == Node::Type_Summarytask) {
        for (const Node *n : std::as_const(m_nodes)) {
            c += n->budgetedCostPerformed(date, id);
        }
        return c;
    }

    c = plannedCost(id).cost() * (double)completion().percentFinished(date) / 100.0;
    if (completion().isStarted() && date >= completion().startTime().date()) {
        c += m_startupCost;
    }
    if (completion().isFinished() && date >= completion().finishTime().date()) {
        c += m_shutdownCost;
    }
    //debugPlan<<m_name<<"("<<id<<")"<<date<<"="<<e.toString();
    return c;
}

double Task::bcwp(long id) const
{
    return bcwp(QDate::currentDate(), id);
}

double Task::bcwp(QDate date, long id) const
{
    return budgetedCostPerformed(date, id);
}

EffortCostMap Task::acwp(long int id, KPlato::EffortCostCalculationType typ)
{
    if (type() == Node::Type_Summarytask) {
        return Node::acwp(id, typ);
    }
    Schedule *s = schedule(id);
    if (s == nullptr) {
        return EffortCostMap();
    }
    EffortCostCache ec = s->acwpCache(typ);
    if (! ec.cached) {
        //debugPlan<<m_name<<completion().entrymode();
        EffortCostMap m;
        switch (completion().entrymode()) {
            case Completion::FollowPlan:
                //TODO
                break;
            case Completion::EnterCompleted:
                //hmmm
            default: {
                m = completion().actualEffortCost(id);
                if (completion().isStarted()) {
                    EffortCost e;
                    e.setCost(m_startupCost);
                    m.add(completion().startTime().date(), e);
                }
                if (completion().isFinished()) {
                    EffortCost e;
                    e.setCost(m_shutdownCost);
                    m.add(completion().finishTime().date(), e);
                }
            }
        }
        ec.effortcostmap = m;
        ec.cached = true;
    }
    return ec.effortcostmap;
}

EffortCost Task::acwp(QDate date, long id) const
{
    //debugPlan;
    if (type() == Node::Type_Summarytask) {
        return Node::acwp(date, id);
    }
    EffortCost c;
    c = completion().actualCostTo(id, date);
    if (completion().isStarted() && date >= completion().startTime().date()) {
        c.add(Duration::zeroDuration, m_startupCost);
    }
    if (completion().isFinished() && date >= completion().finishTime().date()) {
        c.add(Duration::zeroDuration, m_shutdownCost);
    }
    return c;
}

double Task::schedulePerformanceIndex(QDate date, long id) const {
    //debugPlan;
    double r = 1.0;
    double s = bcws(date, id);
    double p = bcwp(date, id);
    if (s > 0.0) {
        r = p / s;
    }
    return r;
}

double Task::effortPerformanceIndex(QDate date, long id) const {
    //debugPlan;
    double r = 1.0;
    Duration a, b;
    if (m_estimate->type() == Estimate::Type_Effort) {
        Duration b = budgetedWorkPerformed(date, id);
        if (b == Duration::zeroDuration) {
            return r;
        }
        Duration a = actualEffortTo(date);
        if (b == Duration::zeroDuration) {
            return 1.0;
        }
        r = b.toDouble() / a.toDouble();
    } else if (m_estimate->type() == Estimate::Type_Duration) {
        //TODO
    }
    return r;
}


//FIXME Handle summarytasks
double Task::costPerformanceIndex(long int id, QDate date, bool *error) const
{
    double res = 0.0;
    double ac = actualCostTo(id, date).cost();

    bool e = (ac == 0.0 || completion().percentFinished() == 0);
    if (error) {
        *error = e;
    }
    if (!e) {
        res = (plannedCostTo(date, id) * completion().percentFinished()) / (100 * ac);
    }
    return res;
}

void Task::initiateCalculation(MainSchedule &sch) {
    debugPlan<<this<<" schedule:"<<sch.name()<<" id="<<sch.id();
    m_currentSchedule = createSchedule(&sch);
    m_currentSchedule->initiateCalculation();
    clearProxyRelations();
    Node::initiateCalculation(sch);
    m_calculateForwardRun = false;
    m_calculateBackwardRun = false;
    m_scheduleForwardRun = false;
    m_scheduleBackwardRun = false;
    m_requests.reset();
}


void Task::initiateCalculationLists(MainSchedule &sch) {
    //debugPlan<<this<<type();
    if (type() == Node::Type_Summarytask) {
        sch.insertSummaryTask(this);
        // propagate my relations to my children and dependent nodes
        for (Node *n : std::as_const(m_nodes)) {
            if (!dependParentNodes().isEmpty()) {
                n->addParentProxyRelations(dependParentNodes());
            }
            if (!dependChildNodes().isEmpty()) {
                n->addChildProxyRelations(dependChildNodes());
            }
            n->initiateCalculationLists(sch);
        }
    } else {
        if (isEndNode()) {
            sch.insertEndNode(this);
            //debugPlan<<"endnodes append:"<<m_name;
        }
        if (isStartNode()) {
            sch.insertStartNode(this);
            //debugPlan<<"startnodes append:"<<m_name;
        }
        if ((m_constraint == Node::MustStartOn) ||
            (m_constraint == Node::MustFinishOn) ||
            (m_constraint == Node::FixedInterval))
        {
            sch.insertHardConstraint(this);
        }
        else if ((m_constraint == Node::StartNotEarlier) ||
                  (m_constraint == Node::FinishNotLater))
        {
            sch.insertSoftConstraint(this);
        }
    }
}

DateTime Task::calculatePredeccessors(const QList<Relation*> &list_, int use) {
    DateTime time;
    // do them forward
    QMultiMap<int, Relation*> lst;
    for (Relation* r : list_) {
        lst.insert(-r->parent()->priority(), r);
    }
    const QList<Relation*> list = lst.values();
    for (Relation *r : list) {
        if (r->parent()->type() == Type_Summarytask) {
            //debugPlan<<"Skip summarytask:"<<it.current()->parent()->name();
            continue; // skip summarytasks
        }
        DateTime t = r->parent()->calculateForward(use); // early finish
        switch (r->type()) {
            case Relation::StartStart:
                // I can't start earlier than my predesseccor
                t = r->parent()->earlyStart() + r->lag();
                break;
            case Relation::FinishFinish: {
                // I can't finish earlier than my predeccessor, so
                // I can't start earlier than it's (earlyfinish+lag)- my duration
                t += r->lag();
                Schedule::OBState obs = m_currentSchedule->allowOverbookingState();
                m_currentSchedule->setAllowOverbookingState(Schedule::OBS_Allow);
#ifndef PLAN_NLOGDEBUG
                m_currentSchedule->logDebug(QStringLiteral("FinishFinish: get duration to calculate early finish"));
#endif
                t -= duration(t, use, true);
                m_currentSchedule->setAllowOverbookingState(obs);
                break;
            }
            default:
                t += r->lag();
                break;
        }
        if (!time.isValid() || t > time)
            time = t;
    }
    //debugPlan<<time.toString()<<""<<m_name<<" calculatePredeccessors() ("<<list.count()<<")";
    return time;
}

DateTime Task::calculateForward(int use)
{
    if (m_calculateForwardRun) {
        return m_currentSchedule->earlyFinish;
    }
    if (m_currentSchedule == nullptr) {
        return DateTime();
    }
    Schedule *cs = m_currentSchedule;
    cs->setCalculationMode(Schedule::CalculateForward);
    //cs->logDebug(QStringLiteral("calculateForward: earlyStart=") + cs->earlyStart.toString());
    // calculate all predecessors
    if (!dependParentNodes().isEmpty()) {
        DateTime time = calculatePredeccessors(dependParentNodes(), use);
        if (time.isValid() && time > cs->earlyStart) {
            cs->earlyStart = time;
            //cs->logDebug(QString("calculate forward: early start moved to: %1").arg(cs->earlyStart.toString()));
        }
    }
    if (!m_parentProxyRelations.isEmpty()) {
        DateTime time = calculatePredeccessors(m_parentProxyRelations, use);
        if (time.isValid() && time > cs->earlyStart) {
            cs->earlyStart = time;
            //cs->logDebug(QString("calculate forward: early start moved to: %1").arg(cs->earlyStart.toString()));
        }
    }
    m_calculateForwardRun = true;
    //cs->logDebug(QStringLiteral("calculateForward: earlyStart=") + cs->earlyStart.toString());
    return calculateEarlyFinish(use);
}

DateTime Task::calculateEarlyFinish(int use) {
    //debugPlan<<m_name;
    if (m_currentSchedule == nullptr || static_cast<Project*>(projectNode())->stopcalculation) {
        return DateTime();
    }
    Schedule *cs = m_currentSchedule;
    if (m_visitedForward) {
        //debugPlan<<earliestStart.toString()<<" +"<<m_durationBackward.toString()<<""<<m_name<<" calculateForward() (visited)";
        return m_earlyFinish;
    }
    bool pert = cs->usePert();
    cs->setCalculationMode(Schedule::CalculateForward);
#ifndef PLAN_NLOGDEBUG
    QElapsedTimer timer;
    timer.start();
    cs->logDebug(QStringLiteral("Start calculate forward: %1 ").arg(constraintToString(true)));
#endif
    QLocale locale;
    cs->logInfo(i18n("Calculate early finish "));
    //debugPlan<<"------>"<<m_name<<""<<cs->earlyStart;
    if (type() == Node::Type_Task) {
        m_durationForward = m_estimate->value(use, pert);
        switch (constraint()) {
            case Node::ASAP:
            case Node::ALAP:
            {
                //debugPlan<<m_name<<" ASAP/ALAP:"<<cs->earlyStart;
                cs->earlyStart = workTimeAfter(cs->earlyStart);
                m_durationForward = duration(cs->earlyStart, use, false);
                m_earlyFinish = cs->earlyStart + m_durationForward;
#ifndef PLAN_NLOGDEBUG
                cs->logDebug(QStringLiteral("ASAP/ALAP: ") + cs->earlyStart.toString() + QLatin1Char('+') + m_durationForward.toString() + QLatin1Char('=') + m_earlyFinish.toString());
#endif
                if (!cs->allowOverbooking()) {
                    cs->startTime = cs->earlyStart;
                    cs->endTime = m_earlyFinish;
                    makeAppointments();

                    // calculate duration wo checking booking = the earliest finish possible
                    Schedule::OBState obs = cs->allowOverbookingState();
                    cs->setAllowOverbookingState(Schedule::OBS_Allow);
                    m_durationForward = duration(cs->earlyStart, use, false);
                    cs->setAllowOverbookingState(obs);
#ifndef PLAN_NLOGDEBUG
                    cs->logDebug(QStringLiteral("ASAP/ALAP earliest possible: ") + cs->earlyStart.toString() + QLatin1Char('+') + m_durationForward.toString() + QLatin1Char('=') + (cs->earlyStart+m_durationForward).toString());
#endif
                }
                break;
            }
            case Node::MustFinishOn:
            {
                cs->earlyStart = workTimeAfter(cs->earlyStart);
                m_durationForward = duration(cs->earlyStart, use, false);
                cs->earlyFinish = cs->earlyStart + m_durationForward;
                //debugPlan<<"MustFinishOn:"<<m_constraintEndTime<<cs->earlyStart<<cs->earlyFinish;
                if (cs->earlyFinish > m_constraintEndTime) {
                    cs->logWarning(i18nc("1=type of constraint", "%1: Failed to meet constraint", constraintToString(true)));
                }
                cs->earlyFinish = qMax(cs->earlyFinish, m_constraintEndTime);
                if (!cs->allowOverbooking()) {
                    cs->endTime = cs->earlyFinish;
                    cs->startTime = cs->earlyFinish - duration(cs->earlyFinish, use, true);
                    makeAppointments();
                }
                m_earlyFinish = cs->earlyFinish;
                m_durationForward = m_earlyFinish - cs->earlyStart;
                break;
            }
            case Node::FinishNotLater:
            {
                m_durationForward = duration(cs->earlyStart, use, false);
                cs->earlyFinish = cs->earlyStart + m_durationForward;
                //debugPlan<<"FinishNotLater:"<<m_constraintEndTime<<cs->earlyStart<<cs->earlyFinish;
                if (cs->earlyFinish > m_constraintEndTime) {
                    cs->logWarning(i18nc("1=type of constraint", "%1: Failed to meet constraint", constraintToString(true)));
                }
                if (!cs->allowOverbooking()) {
                    cs->startTime = cs->earlyStart;
                    cs->endTime = cs->earlyFinish;
                    makeAppointments();
                }
                m_earlyFinish = cs->earlyStart + m_durationForward;
                break;
            }
            case Node::MustStartOn:
            case Node::StartNotEarlier:
            {
                //debugPlan<<"MSO/SNE:"<<m_constraintStartTime<<cs->earlyStart;
                cs->logDebug(constraintToString() + QStringLiteral(": ") + m_constraintStartTime.toString() + QLatin1Char(' ') + cs->earlyStart.toString());
                cs->earlyStart = workTimeAfter(qMax(cs->earlyStart, m_constraintStartTime));
                if (cs->earlyStart < m_constraintStartTime) {
                    cs->logWarning(i18nc("1=type of constraint", "%1: Failed to meet constraint", constraintToString(true)));
                }
                m_durationForward = duration(cs->earlyStart, use, false);
                m_earlyFinish = cs->earlyStart + m_durationForward;
                if (!cs->allowOverbooking()) {
                    cs->startTime = cs->earlyStart;
                    cs->endTime = m_earlyFinish;
                    makeAppointments();

                    // calculate duration wo checking booking = the earliest finish possible
                    Schedule::OBState obs = cs->allowOverbookingState();
                    cs->setAllowOverbookingState(Schedule::OBS_Allow);
                    m_durationForward = duration(cs->startTime, use, false);
                    cs->setAllowOverbookingState(obs);
                    m_earlyFinish = cs->earlyStart + m_durationForward;
#ifndef PLAN_NLOGDEBUG
                    cs->logDebug(QStringLiteral("MSO/SNE earliest possible: ") + cs->earlyStart.toString() + QLatin1Char('+') + m_durationForward.toString() + QLatin1Char('=') + (cs->earlyStart+m_durationForward).toString());
#endif
                }
                break;
            }
            case Node::FixedInterval: {
                if (cs->earlyStart > m_constraintStartTime) {
                    cs->logWarning(i18nc("1=type of constraint", "%1: Failed to meet constraint", constraintToString(true)));
                }
                //cs->earlyStart = m_constraintStartTime;
                m_durationForward = m_constraintEndTime - m_constraintStartTime;
                if (cs->earlyStart < m_constraintStartTime) {
                    m_durationForward = m_constraintEndTime - cs->earlyStart;
                }
                if (!cs->allowOverbooking()) {
                    cs->startTime = m_constraintStartTime;
                    cs->endTime = m_constraintEndTime;
                    makeAppointments();
                }
                m_earlyFinish = cs->earlyStart + m_durationForward;
                break;
            }
        }
    } else if (type() == Node::Type_Milestone) {
        m_durationForward = Duration::zeroDuration;
        switch (constraint()) {
            case Node::MustFinishOn:
                //debugPlan<<"MustFinishOn:"<<m_constraintEndTime<<cs->earlyStart;
                //cs->logDebug(QString("%1: %2, early start: %3").arg(constraintToString()).arg(m_constraintEndTime.toString()).arg(cs->earlyStart.toString()));
                if (cs->earlyStart < m_constraintEndTime) {
                    m_durationForward = m_constraintEndTime - cs->earlyStart;
                }
                if (cs->earlyStart > m_constraintEndTime) {
                    cs->logWarning(i18nc("1=type of constraint", "%1: Failed to meet constraint", constraintToString(true)));
                }
                m_earlyFinish = cs->earlyStart + m_durationForward;
                break;
            case Node::FinishNotLater:
                //debugPlan<<"FinishNotLater:"<<m_constraintEndTime<<cs->earlyStart;
                if (cs->earlyStart > m_constraintEndTime) {
                    cs->logWarning(i18nc("1=type of constraint", "%1: Failed to meet constraint", constraintToString(true)));
                }
                m_earlyFinish = cs->earlyStart;
                break;
            case Node::MustStartOn:
                //debugPlan<<"MustStartOn:"<<m_constraintStartTime<<cs->earlyStart;
                if (cs->earlyStart < m_constraintStartTime) {
                    m_durationForward = m_constraintStartTime - cs->earlyStart;
                }
                if (cs->earlyStart > m_constraintStartTime) {
                    cs->logWarning(i18nc("1=type of constraint", "%1: Failed to meet constraint", constraintToString(true)));
                }
                m_earlyFinish = cs->earlyStart + m_durationForward;
                break;
            case Node::StartNotEarlier:
                //debugPlan<<"StartNotEarlier:"<<m_constraintStartTime<<cs->earlyStart;
                if (cs->earlyStart < m_constraintStartTime) {
                    m_durationForward = m_constraintStartTime - cs->earlyStart;
                }
                m_earlyFinish = cs->earlyStart + m_durationForward;
                break;
            case Node::FixedInterval:
                m_earlyFinish = cs->earlyStart + m_durationForward;
                break;
            default:
                m_earlyFinish = cs->earlyStart + m_durationForward;
                break;
        }
        //debugPlan<<m_name<<""<<earliestStart.toString();
    } else if (type() == Node::Type_Summarytask) {
        warnPlan<<"Summarytasks should not be calculated here: "<<m_name;
    } else { // ???
        m_durationForward = Duration::zeroDuration;
    }
    m_visitedForward = true;
    cs->insertForwardNode(this);
    cs->earlyFinish = cs->earlyStart + m_durationForward;
    const auto appointments = cs->appointments(Schedule::CalculateForward);
    for (const Appointment *a : appointments) {
        cs->logInfo(i18n("Resource %1 booked from %2 to %3", a->resource()->resource()->name(), locale.toString(a->startTime(), QLocale::ShortFormat), locale.toString(a->endTime(), QLocale::ShortFormat)));
    }
    // clean up temporary usage
    cs->startTime = DateTime();
    cs->endTime = DateTime();
    cs->duration = Duration::zeroDuration;
    cs->logInfo(i18n("Early finish calculated: %1", locale.toString(cs->earlyFinish, QLocale::ShortFormat)));
    cs->incProgress();
#ifndef PLAN_NLOGDEBUG
    cs->logDebug(QStringLiteral("Finished calculate forward: %1 ms").arg(timer.elapsed()));
#endif
    return m_earlyFinish;
}

DateTime Task::calculateSuccessors(const QList<Relation*> &list_, int use) {
    DateTime time;
    QMultiMap<int, Relation*> lst;
    for (Relation* r : list_) {
        lst.insert(-r->child()->priority(), r);
    }
    const QList<Relation*> list = lst.values();
    for (Relation *r : list) {
        if (r->child()->type() == Type_Summarytask) {
            //debugPlan<<"Skip summarytask:"<<r->parent()->name();
            continue; // skip summarytasks
        }
        DateTime t = r->child()->calculateBackward(use);
        switch (r->type()) {
            case Relation::StartStart: {
                // I must start before my successor, so
                // I can't finish later than it's (starttime-lag) + my duration
                t -= r->lag();
                Schedule::OBState obs = m_currentSchedule->allowOverbookingState();
                m_currentSchedule->setAllowOverbookingState(Schedule::OBS_Allow);
#ifndef PLAN_NLOGDEBUG
                m_currentSchedule->logDebug(QStringLiteral("StartStart: get duration to calculate late start"));
#endif
                t += duration(t, use, false);
                m_currentSchedule->setAllowOverbookingState(obs);
                break;
            }
            case Relation::FinishFinish:
                // My successor cannot finish before me, so
                // I can't finish later than it's latest finish - lag
                t = r->child()->lateFinish() -  r->lag();
                break;
            default:
                t -= r->lag();
                break;
        }
        if (!time.isValid() || t < time)
            time = t;
    }
    //debugPlan<<time.toString()<<""<<m_name<<" calculateSuccessors() ("<<list.count()<<")";
    return time;
}

DateTime Task::calculateBackward(int use) {
    //debugPlan<<m_name;
    if (static_cast<Project*>(projectNode())->stopcalculation) {
        return DateTime();
    }
    if (m_calculateBackwardRun) {
        return m_currentSchedule->lateStart;
    }
    if (m_currentSchedule == nullptr) {
        return DateTime();
    }
    Schedule *cs = m_currentSchedule;
    cs->setCalculationMode(Schedule::CalculateBackward);
    //cs->lateFinish = projectNode()->constraintEndTime();
    // calculate all successors
    if (!dependChildNodes().isEmpty()) {
        DateTime time = calculateSuccessors(dependChildNodes(), use);
        if (time.isValid() && time < cs->lateFinish) {
            cs->lateFinish = time;
        }
    }
    if (!m_childProxyRelations.isEmpty()) {
        DateTime time = calculateSuccessors(m_childProxyRelations, use);
        if (time.isValid() && time < cs->lateFinish) {
            cs->lateFinish = time;
        }
    }
    m_calculateBackwardRun = true;
    return calculateLateStart(use);
}

DateTime Task::calculateLateStart(int use) {
    //debugPlan<<m_name;
    if (m_currentSchedule == nullptr) {
        return DateTime();
    }
    Schedule *cs = m_currentSchedule;
    if (m_visitedBackward) {
        //debugPlan<<latestFinish.toString()<<" -"<<m_durationBackward.toString()<<""<<m_name<<" calculateBackward() (visited)";
        return cs->lateStart;
    }
    bool pert = cs->usePert();
    cs->setCalculationMode(Schedule::CalculateBackward);
#ifndef PLAN_NLOGDEBUG
    QElapsedTimer timer;
    timer.start();
    cs->logDebug(QStringLiteral("Start calculate backward: %1 ").arg(constraintToString(true)));
#endif
    QLocale locale;
    cs->logInfo(i18n("Calculate late start"));
    cs->logDebug(QStringLiteral("%1: late finish= %2").arg(constraintToString()).arg(cs->lateFinish.toString()));
    //debugPlan<<m_name<<" id="<<cs->id()<<" mode="<<cs->calculationMode()<<": latestFinish="<<cs->lateFinish;
    if (type() == Node::Type_Task) {
        m_durationBackward = m_estimate->value(use, pert);
        switch (constraint()) {
            case Node::ASAP:
            case Node::ALAP:
                //debugPlan<<m_name<<" ASAP/ALAP:"<<cs->lateFinish;
                cs->lateFinish = workTimeBefore(cs->lateFinish);
                m_durationBackward = duration(cs->lateFinish, use, true);
                cs->lateStart = cs->lateFinish - m_durationBackward;
#ifndef PLAN_NLOGDEBUG
                cs->logDebug(QStringLiteral("ASAP/ALAP: ") + cs->lateFinish.toString() + QLatin1Char('-') + m_durationBackward.toString() + QLatin1Char('=') + cs->lateStart.toString());
#endif
                if (!cs->allowOverbooking()) {
                    cs->startTime = cs->lateStart;
                    cs->endTime = cs->lateFinish;
                    makeAppointments();

                    // calculate wo checking bookings = latest start possible
                    Schedule::OBState obs = cs->allowOverbookingState();
                    cs->setAllowOverbookingState(Schedule::OBS_Allow);
                    m_durationBackward = duration(cs->lateFinish, use, true);
                    cs->setAllowOverbookingState(obs);
#ifndef PLAN_NLOGDEBUG
                    cs->logDebug(QStringLiteral("ASAP/ALAP latest start possible: ") + cs->lateFinish.toString() + QLatin1Char('-') + m_durationBackward.toString() + QLatin1Char('=') + (cs->lateFinish-m_durationBackward).toString());
#endif
                }
                break;
            case Node::MustStartOn:
            case Node::StartNotEarlier:
            {
                //debugPlan<<"MustStartOn:"<<m_constraintStartTime<<cs->lateFinish;
                cs->lateFinish = workTimeBefore(cs->lateFinish);
                m_durationBackward = duration(cs->lateFinish, use, true);
                cs->lateStart = cs->lateFinish - m_durationBackward;
                if (cs->lateStart < m_constraintStartTime) {
                    cs->logWarning(i18nc("1=type of constraint", "%1: Failed to meet constraint", constraintToString(true)));
                } else {
                    cs->lateStart = qMax(cs->earlyStart, m_constraintStartTime);
                }
                if (!cs->allowOverbooking()) {
                    if (constraint() == MustStartOn) {
                        cs->startTime = m_constraintStartTime;
                        cs->endTime = m_constraintStartTime + duration(m_constraintStartTime, use, false);
                    } else {
                        cs->startTime = qMax(cs->lateStart, m_constraintStartTime);
                        cs->endTime = qMax(cs->lateFinish, cs->startTime); // safety
                    }
                    makeAppointments();
                }
                cs->lateStart = cs->lateFinish - m_durationBackward;
                break;
            }
            case Node::MustFinishOn:
            case Node::FinishNotLater:
                //debugPlan<<"MustFinishOn:"<<m_constraintEndTime<<cs->lateFinish;
                cs->lateFinish = workTimeBefore(cs->lateFinish);
                cs->endTime = cs->lateFinish;
                if (cs->lateFinish < m_constraintEndTime) {
                    cs->logWarning(i18nc("1=type of constraint", "%1: Failed to meet constraint", constraintToString(true)));
                } else {
                    cs->endTime = qMax(cs->earlyFinish, m_constraintEndTime);
                }
                m_durationBackward = duration(cs->endTime, use, true);
                cs->startTime = cs->endTime - m_durationBackward;
                if (!cs->allowOverbooking()) {
                    makeAppointments();
                }
                m_durationBackward = cs->lateFinish - cs->startTime;
                cs->lateStart = cs->lateFinish - m_durationBackward;
                break;
            case Node::FixedInterval: {
                //cs->lateFinish = m_constraintEndTime;
                if (cs->lateFinish < m_constraintEndTime) {
                    cs->logWarning(i18nc("1=type of constraint", "%1: Failed to meet constraint", constraintToString(true)));
                }
                m_durationBackward = m_constraintEndTime - m_constraintStartTime;
                if (cs->lateFinish > m_constraintEndTime) {
                    m_durationBackward = cs->lateFinish - m_constraintStartTime;
                }
                if (!cs->allowOverbooking()) {
                    cs->startTime = m_constraintStartTime;
                    cs->endTime = m_constraintEndTime;
                    makeAppointments();
                }
                cs->lateStart = cs->lateFinish - m_durationBackward;
                break;
            }
        }
    } else if (type() == Node::Type_Milestone) {
        m_durationBackward = Duration::zeroDuration;
        switch (constraint()) {
            case Node::MustFinishOn:
                //debugPlan<<"MustFinishOn:"<<m_constraintEndTime<<cs->lateFinish;
                if (m_constraintEndTime < cs->lateFinish) {
                    m_durationBackward = cs->lateFinish - m_constraintEndTime;
                } else if (m_constraintEndTime > cs->lateFinish) {
                    cs->logWarning(i18nc("1=type of constraint", "%1: Failed to meet constraint", constraintToString(true)));
                }
                cs->lateStart = cs->lateFinish - m_durationBackward;
                break;
            case Node::FinishNotLater:
                //debugPlan<<"FinishNotLater:"<<m_constraintEndTime<<cs->lateFinish;
                if (m_constraintEndTime < cs->lateFinish) {
                    m_durationBackward = cs->lateFinish - m_constraintEndTime;
                } else if (m_constraintEndTime > cs->lateFinish) {
                    cs->logWarning(i18nc("1=type of constraint", "%1: Failed to meet constraint", constraintToString(true)));
                }
                cs->lateStart = cs->lateFinish - m_durationBackward;
                break;
            case Node::MustStartOn:
                //debugPlan<<"MustStartOn:"<<m_constraintStartTime<<cs->lateFinish;
                if (m_constraintStartTime < cs->lateFinish) {
                    m_durationBackward = cs->lateFinish - m_constraintStartTime;
                } else if (m_constraintStartTime > cs->lateFinish) {
                    cs->logWarning(i18nc("1=type of constraint", "%1: Failed to meet constraint", constraintToString(true)));
                }
                cs->lateStart = cs->lateFinish - m_durationBackward;
                //cs->logDebug(QString("%1: constraint:%2, start=%3, finish=%4").arg(constraintToString()).arg(m_constraintStartTime.toString()).arg(cs->lateStart.toString()).arg(cs->lateFinish.toString()));
                break;
            case Node::StartNotEarlier:
                //debugPlan<<"MustStartOn:"<<m_constraintStartTime<<cs->lateFinish;
                if (m_constraintStartTime > cs->lateFinish) {
                    cs->logWarning(i18nc("1=type of constraint", "%1: Failed to meet constraint", constraintToString(true)));
                }
                cs->lateStart = cs->lateFinish;
                break;
            case Node::FixedInterval:
                cs->lateStart = cs->lateFinish - m_durationBackward;
                break;
            default:
                cs->lateStart = cs->lateFinish - m_durationBackward;
                break;
        }
        //debugPlan<<m_name<<""<<cs->lateFinish;
    } else if (type() == Node::Type_Summarytask) {
        warnPlan<<"Summarytasks should not be calculated here: "<<m_name;
    } else { // ???
        m_durationBackward = Duration::zeroDuration;
    }
    m_visitedBackward = true;
    cs->insertBackwardNode(this);
    cs->lateStart = cs->lateFinish - m_durationBackward;
    const auto appointments = cs->appointments(Schedule::CalculateBackward);
    for (const Appointment *a : appointments) {
        cs->logInfo(i18n("Resource %1 booked from %2 to %3", a->resource()->resource()->name(), locale.toString(a->startTime(), QLocale::ShortFormat), locale.toString(a->endTime(), QLocale::ShortFormat)));
    }
    // clean up temporary usage
    cs->startTime = DateTime();
    cs->endTime = DateTime();
    cs->duration = Duration::zeroDuration;
    cs->logInfo(i18n("Late start calculated: %1", locale.toString(cs->lateStart, QLocale::ShortFormat)));
    cs->incProgress();
#ifndef PLAN_NLOGDEBUG
    cs->logDebug(QStringLiteral("Finished calculate backward: %1 ms").arg(timer.elapsed()));
#endif
    return cs->lateStart;
}

DateTime Task::schedulePredeccessors(const QList<Relation*> &list_, int use) {
    DateTime time;
    QMultiMap<int, Relation*> lst;
    for (Relation* r : list_) {
        lst.insert(-r->parent()->priority(), r);
    }
    const QList<Relation*> list = lst.values();
    for (Relation *r : list) {
        if (r->parent()->type() == Type_Summarytask) {
            //debugPlan<<"Skip summarytask:"<<r->parent()->name();
            continue; // skip summarytasks
        }
        // schedule the predecessors
        DateTime earliest = r->parent()->earlyStart();
        DateTime t = r->parent()->scheduleForward(earliest, use);
        switch (r->type()) {
            case Relation::StartStart:
                // I can't start before my predesseccor
                t = r->parent()->startTime() + r->lag();
                break;
            case Relation::FinishFinish:
                // I can't end before my predecessor, so
                // I can't start before it's endtime - my duration
#ifndef PLAN_NLOGDEBUG
                m_currentSchedule->logDebug(QStringLiteral("FinishFinish: get duration to calculate earliest start"));
#endif
                t -= duration(t + r->lag(), use, true);
                break;
            default:
                t += r->lag();
                break;
        }
        if (!time.isValid() || t > time)
            time = t;
    }
    //debugPlan<<time.toString()<<""<<m_name<<" schedulePredeccessors()";
    return time;
}

DateTime Task::scheduleForward(const DateTime &earliest, int use) {
    if (static_cast<Project*>(projectNode())->stopcalculation) {
        return DateTime();
    }
    if (m_scheduleForwardRun) {
        return m_currentSchedule->endTime;
    }
    if (m_currentSchedule == nullptr) {
        return DateTime();
    }
    Schedule *cs = m_currentSchedule;
    //cs->logDebug(QString("Schedule forward (early start: %1)").arg(cs->earlyStart.toString()));
    cs->setCalculationMode(Schedule::Scheduling);
    DateTime startTime = earliest > cs->earlyStart ? earliest : cs->earlyStart;
    // First, calculate all my own predecessors
    DateTime time = schedulePredeccessors(dependParentNodes(), use);
    if (time > startTime) {
        startTime = time;
        //debugPlan<<m_name<<" new startime="<<cs->startTime;
    }
    // Then my parents
    time = schedulePredeccessors(m_parentProxyRelations, use);
    if (time > startTime) {
        startTime = time;
    }
    if (! m_visitedForward) {
        cs->startTime = startTime;
    }
    m_scheduleForwardRun = true;
    return scheduleFromStartTime(use);
}

DateTime Task::scheduleFromStartTime(int use) {
    //debugPlan<<m_name;
    if (m_currentSchedule == nullptr) {
        return DateTime();
    }
    Schedule *cs = m_currentSchedule;
    cs->setCalculationMode(Schedule::Scheduling);
    bool pert = cs->usePert();
    if (m_visitedForward) {
        return cs->endTime;
    }
    cs->notScheduled = false;
    if (!cs->startTime.isValid()) {
        //cs->logDebug(QString("Schedule from start time: no start time use early start: %1").arg(cs->earlyStart.toString()));
        cs->startTime = cs->earlyStart;
    }
    QElapsedTimer timer;
    timer.start();
    cs->logInfo(i18n("Start schedule forward: %1 ", constraintToString(true)));
    QLocale locale;
    cs->logInfo(i18n("Schedule from start %1", locale.toString(cs->startTime, QLocale::ShortFormat)));
    //debugPlan<<m_name<<" startTime="<<cs->startTime;
    if(type() == Node::Type_Task) {
        if (cs->recalculate() && completion().isFinished()) {
            copySchedule();
            m_visitedForward = true;
            return cs->endTime;
        }
        cs->duration = m_estimate->value(use, pert);
        switch (m_constraint) {
        case Node::ASAP:
            // cs->startTime calculated above
            //debugPlan<<m_name<<"ASAP:"<<cs->startTime<<"earliest:"<<cs->earlyStart;
            if (estimate()->type() == Estimate::Type_Duration && cs->recalculate() && completion().isStarted()) {
                cs->startTime = completion().startTime();
            } else {
                cs->startTime = workTimeAfter(cs->startTime, cs);
            }
#ifndef PLAN_NLOGDEBUG
            cs->logDebug(QStringLiteral("ASAP: ") + cs->startTime.toString() + QStringLiteral(" earliest: ") + cs->earlyStart.toString());
#endif
            cs->duration = duration(cs->startTime, use, false);
            cs->endTime = cs->startTime + cs->duration;
            makeAppointments();
            if (cs->recalculate() && completion().isStarted()) {
                cs->logDebug(QStringLiteral("Create appointments from completion"));
                copyAppointments();
                cs->duration = cs->endTime - cs->startTime;
            }

            if (cs->lateFinish > cs->endTime) {
                cs->positiveFloat = workTimeBefore(cs->lateFinish) - cs->endTime;
            } else {
                cs->positiveFloat = Duration::zeroDuration;
            }
            break;
        case Node::ALAP:
            // cs->startTime calculated above
            //debugPlan<<m_name<<"ALAP:"<<cs->startTime<<cs->endTime<<" latest="<<cs->lateFinish;
            cs->endTime = workTimeBefore(cs->lateFinish, cs);
            cs->duration = duration(cs->endTime, use, true);
            cs->startTime = cs->endTime - cs->duration;
            //debugPlan<<m_name<<" endTime="<<cs->endTime<<" latest="<<cs->lateFinish;
            makeAppointments();
            if (cs->plannedEffort() == 0 && cs->lateFinish < cs->earlyFinish) {
                // the backward pass failed to calculate sane values, try to handle it
                //TODO add an error indication
                cs->logWarning(i18n("%1: Scheduling failed using late finish, trying early finish instead.", constraintToString()));
                cs->endTime = workTimeBefore(cs->earlyFinish, cs);
                cs->duration = duration(cs->endTime, use, true);
                cs->startTime = cs->endTime - cs->duration;
                makeAppointments();
            }
            if (cs->lateFinish > cs->endTime) {
                cs->positiveFloat = workTimeBefore(cs->lateFinish) - cs->endTime;
            } else {
                cs->positiveFloat = Duration::zeroDuration;
            }
            if (cs->recalculate() && completion().isStarted()) {
                cs->earlyStart = cs->startTime = completion().startTime();
            }
            break;
        case Node::StartNotEarlier:
            // cs->startTime calculated above
            //debugPlan<<"StartNotEarlier:"<<m_constraintStartTime<<cs->startTime<<cs->lateStart;
            cs->startTime = workTimeAfter(qMax(cs->startTime, m_constraintStartTime), cs);
            cs->duration = duration(cs->startTime, use, false);
            cs->endTime = cs->startTime + cs->duration;
            makeAppointments();
            if (cs->recalculate() && completion().isStarted()) {
                // copy start times + appointments from parent schedule
                copyAppointments();
                cs->duration = cs->endTime - cs->startTime;
            }
            if (cs->lateFinish > cs->endTime) {
                cs->positiveFloat = workTimeBefore(cs->lateFinish) - cs->endTime;
            } else {
                cs->positiveFloat = Duration::zeroDuration;
            }
            if (cs->startTime < m_constraintStartTime) {
                cs->constraintError = true;
                cs->negativeFloat = cs->startTime - m_constraintStartTime;
                cs->logError(i18nc("1=type of constraint", "%1: Failed to meet constraint. Negative float=%2", constraintToString(true), cs->negativeFloat.toString(Duration::Format_i18nHour)));
            }
            break;
        case Node::FinishNotLater:
            // cs->startTime calculated above
            //debugPlan<<"FinishNotLater:"<<m_constraintEndTime<<cs->startTime;
            cs->startTime = workTimeAfter(cs->startTime, cs);
            cs->duration = duration(cs->startTime, use, false);
            cs->endTime = cs->startTime + cs->duration;
            makeAppointments();
            if (cs->recalculate() && completion().isStarted()) {
                // copy start times + appointments from parent schedule
                copyAppointments();
                cs->duration = cs->endTime - cs->startTime;
            }
            if (cs->lateFinish > cs->endTime) {
                cs->positiveFloat = workTimeBefore(cs->lateFinish) - cs->endTime;
            } else {
                cs->positiveFloat = Duration::zeroDuration;
            }
            if (cs->endTime > m_constraintEndTime) {
                //warnPlan<<"cs->endTime > m_constraintEndTime";
                cs->constraintError = true;
                cs->negativeFloat = cs->endTime - m_constraintEndTime;
                cs->logError(i18nc("1=type of constraint", "%1: Failed to meet constraint. Negative float=%2", constraintToString(true), cs->negativeFloat.toString(Duration::Format_i18nHour)));
            }
            break;
        case Node::MustStartOn:
            // Always try to put it on time
            cs->startTime = workTimeAfter(m_constraintStartTime, cs);
            //debugPlan<<"MustStartOn="<<m_constraintStartTime<<"<"<<cs->startTime;
            cs->duration = duration(cs->startTime, use, false);
            cs->endTime = cs->startTime + cs->duration;
#ifndef PLAN_NLOGDEBUG
            cs->logDebug(QStringLiteral("%1: Schedule from %2 to %3").arg(constraintToString()).arg(cs->startTime.toString()).arg(cs->endTime.toString()));
#endif
            makeAppointments();
            if (cs->recalculate() && completion().isStarted()) {
                // copy start times + appointments from parent schedule
                copyAppointments();
                cs->duration = cs->endTime - cs->startTime;
            }
            if (cs->lateFinish > cs->endTime) {
                cs->positiveFloat = workTimeBefore(cs->lateFinish) - cs->endTime;
            } else {
                cs->positiveFloat = Duration::zeroDuration;
            }
            if (m_constraintStartTime < cs->startTime) {
                cs->constraintError = true;
                cs->negativeFloat = cs->startTime - m_constraintStartTime;
                cs->logError(i18nc("1=type of constraint", "%1: Failed to meet constraint. Negative float=%2", constraintToString(true), cs->negativeFloat.toString(Duration::Format_i18nHour)));
            }
            break;
        case Node::MustFinishOn:
            // Just try to schedule on time
            cs->endTime = workTimeBefore(m_constraintEndTime, cs);
            cs->duration = duration(cs->endTime, use, true);
            cs->startTime = cs->endTime - cs->duration;

            //debugPlan<<"MustFinishOn:"<<m_constraintEndTime<<","<<cs->lateFinish<<":"<<cs->startTime<<cs->endTime;
            makeAppointments();
            if (cs->recalculate() && completion().isStarted()) {
                // copy start times + appointments from parent schedule
                copyAppointments();
                cs->duration = cs->endTime - cs->startTime;
            }
            if (cs->lateFinish > cs->endTime) {
                cs->positiveFloat = workTimeBefore(cs->lateFinish) - cs->endTime;
            } else {
                cs->positiveFloat = Duration::zeroDuration;
            }
            if (cs->endTime != m_constraintEndTime) {
                cs->constraintError = true;
                cs->negativeFloat = cs->endTime - m_constraintEndTime;
                cs->logError(i18nc("1=type of constraint", "%1: Failed to meet constraint. Negative float=%2", constraintToString(true), cs->negativeFloat.toString(Duration::Format_i18nHour)));
            }
            break;
        case Node::FixedInterval: {
            // cs->startTime calculated above
            //debugPlan<<"FixedInterval="<<m_constraintStartTime<<""<<cs->startTime;
            cs->duration = m_constraintEndTime - m_constraintStartTime;
            if (m_constraintStartTime >= cs->earlyStart) {
                cs->startTime = m_constraintStartTime;
                cs->endTime = m_constraintEndTime;
            } else {
                cs->startTime = cs->earlyStart;
                cs->endTime = cs->startTime + cs->duration;
                cs->constraintError = true;
            }
            if (m_constraintStartTime < cs->startTime) {
                cs->constraintError = true;
                cs->negativeFloat = cs->startTime - m_constraintStartTime;
                cs->logError(i18nc("1=type of constraint", "%1: Failed to meet constraint. Negative float=%2", constraintToString(true), cs->negativeFloat.toString(Duration::Format_i18nHour)));
            }
            if (cs->lateFinish > cs->endTime) {
                cs->positiveFloat = workTimeBefore(cs->lateFinish) - cs->endTime;
            } else {
                cs->positiveFloat = Duration::zeroDuration;
            }
            cs->workStartTime = m_constraintStartTime;
            cs->workEndTime = m_constraintEndTime;
            //debugPlan<<"FixedInterval="<<cs->startTime<<","<<cs->endTime;
            makeAppointments();
            break;
        }
        default:
            break;
        }
        if (m_estimate->type() == Estimate::Type_Effort) {
            const qint64 granularity = this->granularity();
            const auto estimate = m_estimate->value(use, cs->usePert());
            const auto planned = cs->plannedEffort();
            const auto deviation = estimate - planned;
            if (deviation != 0) {
                warnPlan<<"Estimated effort not met:"<<estimate.toString()<<planned.toString();
                if (deviation <= granularity) {
                    cs->logInfo(i18n("Effort deviation. Estimate: %1, scheduled: %2, granularity: %3",
                                     estimate.toString(Duration::Format_i18nDayTime),
                                     planned.toString(Duration::Format_i18nDayTime),
                                     Duration(granularity).toString(Duration::Format_i18nDay)));
                }
            }
            cs->effortNotMet = deviation > granularity;
            if (cs->effortNotMet) {
                cs->logError(i18n("Effort not met. Estimate: %1, scheduled: %2", estimate.toHours(), planned.toHours()));
            }
        }
    } else if (type() == Node::Type_Milestone) {
        if (cs->recalculate() && completion().isFinished()) {
            cs->startTime = completion().startTime();
            cs->endTime = completion().finishTime();
            m_visitedForward = true;
            return cs->endTime;
        }
        switch (m_constraint) {
        case Node::ASAP: {
            cs->endTime = cs->startTime;
            // TODO check, do we need to check successors earliestStart?
            cs->positiveFloat = cs->lateFinish - cs->endTime;
            break;
        }
        case Node::ALAP: {
            cs->startTime = qMax(cs->lateFinish, cs->earlyFinish);
            cs->endTime = cs->startTime;
            cs->positiveFloat = Duration::zeroDuration;
            break;
        }
        case Node::MustStartOn:
        case Node::MustFinishOn:
        case Node::FixedInterval: {
            //debugPlan<<"MustStartOn:"<<m_constraintStartTime<<cs->startTime;
            DateTime contime = m_constraint == Node::MustFinishOn ? m_constraintEndTime : m_constraintStartTime;
#ifndef PLAN_NLOGDEBUG
            cs->logDebug(QStringLiteral("%1: constraint time=%2, start time=%3").arg(constraintToString()).arg(contime.toString()).arg(cs->startTime.toString()));
#endif
            if (cs->startTime < contime) {
                if (contime <= cs->lateFinish || contime <= cs->earlyFinish) {
                    cs->startTime = contime;
                }
            }
            cs->negativeFloat = cs->startTime > contime ? cs->startTime - contime :  contime - cs->startTime;
            if (cs->negativeFloat != 0) {
                cs->constraintError = true;
                cs->logError(i18nc("1=type of constraint", "%1: Failed to meet constraint. Negative float=%2", constraintToString(true), cs->negativeFloat.toString(Duration::Format_i18nHour)));
            }
            cs->endTime = cs->startTime;
            if (cs->negativeFloat == Duration::zeroDuration) {
                cs->positiveFloat = cs->lateFinish - cs->endTime;
            }
            break;
        }
        case Node::StartNotEarlier:
            if (cs->startTime < m_constraintStartTime) {
                if (m_constraintStartTime <= cs->lateFinish || m_constraintStartTime <= cs->earlyFinish) {
                    cs->startTime = m_constraintStartTime;
                }
            }
            if (cs->startTime < m_constraintStartTime) {
                cs->constraintError = true;
                cs->negativeFloat = m_constraintStartTime - cs->startTime;
                cs->logError(i18nc("1=type of constraint", "%1: Failed to meet constraint. Negative float=%2", constraintToString(true), cs->negativeFloat.toString(Duration::Format_i18nHour)));
            }
            cs->endTime = cs->startTime;
            if (cs->negativeFloat == Duration::zeroDuration) {
                cs->positiveFloat = cs->lateFinish - cs->endTime;
            }
            break;
        case Node::FinishNotLater:
            //debugPlan<<m_constraintEndTime<<cs->startTime;
            if (cs->startTime > m_constraintEndTime) {
                cs->constraintError = true;
                cs->negativeFloat = cs->startTime - m_constraintEndTime;
                cs->logError(i18nc("1=type of constraint", "%1: Failed to meet constraint. Negative float=%2", constraintToString(true), cs->negativeFloat.toString(Duration::Format_i18nHour)));
            }
            cs->endTime = cs->startTime;
            if (cs->negativeFloat == Duration::zeroDuration) {
                cs->positiveFloat = cs->lateFinish - cs->endTime;
            }
            break;
        default:
            break;
        }
        cs->duration = Duration::zeroDuration;
        //debugPlan<<m_name<<":"<<cs->startTime<<","<<cs->endTime;
    } else if (type() == Node::Type_Summarytask) {
        //shouldn't come here
        cs->endTime = cs->startTime;
        cs->duration = cs->endTime - cs->startTime;
        warnPlan<<"Summarytasks should not be calculated here: "<<m_name;
    }
    //debugPlan<<cs->startTime<<" :"<<cs->endTime<<""<<m_name<<" scheduleForward()";
    if (cs->startTime < projectNode()->constraintStartTime() || cs->endTime > projectNode()->constraintEndTime()) {
        cs->logError(i18n("Failed to schedule within project target time"));
    }
    const auto appointments = cs->appointments();
    for (const Appointment *a : appointments) {
        cs->logInfo(i18n("Resource %1 booked from %2 to %3", a->resource()->resource()->name(), locale.toString(a->startTime(), QLocale::ShortFormat), locale.toString(a->endTime(), QLocale::ShortFormat)));
    }
    if (cs->startTime < cs->earlyStart) {
        cs->logWarning(i18n("Starting earlier than early start"));
    }
    if (cs->endTime > cs->lateFinish) {
        cs->logWarning(i18n("Finishing later than late finish"));
    }
    cs->logInfo(i18n("Scheduled: %1 to %2", locale.toString(cs->startTime, QLocale::ShortFormat), locale.toString(cs->endTime, QLocale::ShortFormat)));
    m_visitedForward = true;
    cs->incProgress();
    cs->logInfo(i18n("Finished schedule forward: %1 ms", timer.elapsed()));
    return cs->endTime;
}

DateTime Task::scheduleSuccessors(const QList<Relation*> &list_, int use) {
    DateTime time;
    QMultiMap<int, Relation*> lst;
    for (Relation* r : list_) {
        lst.insert(-r->child()->priority(), r);
    }
    const QList<Relation*> relations = lst.values();
    for (Relation *r : relations) {
        if (r->child()->type() == Type_Summarytask) {
            //debugPlan<<"Skip summarytask:"<<r->child()->name();
            continue;
        }
        // get the successors starttime
        DateTime latest = r->child()->lateFinish();
        DateTime t = r->child()->scheduleBackward(latest, use);
        switch (r->type()) {
            case Relation::StartStart:
                // I can't start before my successor, so
                // I can't finish later than it's starttime + my duration
#ifndef PLAN_NLOGDEBUG
                m_currentSchedule->logDebug(QStringLiteral("StartStart: get duration to calculate late finish"));
#endif
                t += duration(t - r->lag(), use, false);
                break;
            case Relation::FinishFinish:
                t = r->child()->endTime() - r->lag();
                break;
            default:
                t -= r->lag();
                break;
        }
        if (!time.isValid() || t < time)
            time = t;
   }
   return time;
}

DateTime Task::scheduleBackward(const DateTime &latest, int use)
{
    if (static_cast<Project*>(projectNode())->stopcalculation) {
        return DateTime();
    }
    if (m_scheduleBackwardRun) {
        return m_currentSchedule->startTime;
    }
    if (m_currentSchedule == nullptr) {
        return DateTime();
    }
    Schedule *cs = m_currentSchedule;
    cs->setCalculationMode(Schedule::Scheduling);

    DateTime endTime = latest < cs->lateFinish ? latest : cs->lateFinish;
    // First, calculate all my own successors
    DateTime time = scheduleSuccessors(dependChildNodes(), use);
    if (time.isValid() && time < endTime) {
        endTime = time;
    }
    // Then my parents
    time = scheduleSuccessors(m_childProxyRelations, use);
    if (time.isValid() && time < endTime) {
        endTime = time;
    }
    if (! m_visitedBackward) {
        cs->endTime = endTime;
    }
    m_scheduleBackwardRun = true;
    return scheduleFromEndTime(use);
}

DateTime Task::scheduleFromEndTime(int use)
{
    //debugPlan<<m_name;
    if (static_cast<Project*>(projectNode())->stopcalculation) {
        return DateTime();
    }
    if (m_currentSchedule == nullptr) {
        return DateTime();
    }
    Schedule *cs = m_currentSchedule;
    cs->setCalculationMode(Schedule::Scheduling);
    bool pert = cs->usePert();
    if (m_visitedBackward) {
        return cs->startTime;
    }
    cs->notScheduled = false;
    if (!cs->endTime.isValid()) {
        cs->endTime = cs->lateFinish;
    }
#ifndef PLAN_NLOGDEBUG
    QElapsedTimer timer;
    timer.start();
    cs->logDebug(QStringLiteral("Start schedule backward: %1 ").arg(constraintToString(true)));
#endif
    QLocale locale;
    cs->logInfo(i18n("Schedule from end time: %1", cs->endTime.toString()));
    if (type() == Node::Type_Task) {
        cs->duration = m_estimate->value(use, pert);
        switch (m_constraint) {
        case Node::ASAP: {
            // cs->endTime calculated above
            //debugPlan<<m_name<<": end="<<cs->endTime<<"  early="<<cs->earlyStart;
            //TODO: try to keep within projects constraint times
            cs->endTime = workTimeBefore(cs->endTime, cs);
            cs->startTime = workTimeAfter(cs->earlyStart, cs);
            DateTime e;
            if (cs->startTime < cs->endTime) {
                cs->duration = duration(cs->startTime, use, false);
                e = cs->startTime + cs->duration;
            } else {
#ifndef PLAN_NLOGDEBUG
                cs->logDebug(QStringLiteral("%1: Latest allowed end time earlier than early start").arg(constraintToString()));
#endif
                cs->duration = duration(cs->endTime, use, true);
                e = cs->endTime;
                cs->startTime = e - cs->duration;
            }
            if (e > cs->lateFinish) {
                cs->schedulingError = true;
                cs->logError(i18nc("1=type of constraint", "%1: Failed to schedule within late finish.", constraintToString()));
#ifndef PLAN_NLOGDEBUG
                cs->logDebug(QStringLiteral("ASAP: late finish=") + cs->lateFinish.toString() + QStringLiteral(" end time=") + e.toString());
#endif
            } else if (e > cs->endTime) {
                cs->schedulingError = true;
                cs->logWarning(i18nc("1=type of constraint", "%1: Failed to schedule within successors start time",  constraintToString()));
#ifndef PLAN_NLOGDEBUG
                cs->logDebug(QStringLiteral("ASAP: succ. start=") + cs->endTime.toString() + QStringLiteral(" end time=") + e.toString());
#endif
            }
            if (cs->lateFinish > e) {
                DateTime w = workTimeBefore(cs->lateFinish);
                if (w > e) {
                    cs->positiveFloat = w - e;
                }
#ifndef PLAN_NLOGDEBUG
                cs->logDebug(QStringLiteral("ASAP: positiveFloat=") + cs->positiveFloat.toString());
#endif
            }
            cs->endTime = e;
            makeAppointments();
            break;
        }
        case Node::ALAP:
        {
            // cs->endTime calculated above
            //debugPlan<<m_name<<": end="<<cs->endTime<<"  late="<<cs->lateFinish;
            cs->endTime = workTimeBefore(cs->endTime, cs);
            cs->duration = duration(cs->endTime, use, true);
            cs->startTime = cs->endTime - cs->duration;
            if (cs->startTime < cs->earlyStart) {
                cs->schedulingError = true;
                cs->logError(i18nc("1=type of constraint", "%1: Failed to schedule after early start.", constraintToString()));
#ifndef PLAN_NLOGDEBUG
                cs->logDebug(QStringLiteral("ALAP: earlyStart=") + cs->earlyStart.toString() + QStringLiteral(" cs->startTime=") + cs->startTime.toString());
#endif
            } else if (cs->lateFinish > cs->endTime) {
                cs->positiveFloat = workTimeBefore(cs->lateFinish) - cs->endTime;
#ifndef PLAN_NLOGDEBUG
                cs->logDebug(QStringLiteral("ALAP: positiveFloat=") + cs->positiveFloat.toString());
#endif
            }
            //debugPlan<<m_name<<": lateStart="<<cs->startTime;
            makeAppointments();
            break;
        }
        case Node::StartNotEarlier:
            // cs->endTime calculated above
            //debugPlan<<"StartNotEarlier:"<<m_constraintStartTime<<cs->endTime;
            cs->endTime = workTimeBefore(cs->endTime, cs);
            cs->duration = duration(cs->endTime, use, true);
            cs->startTime = cs->endTime - cs->duration;
            if (cs->startTime < m_constraintStartTime) {
                //warnPlan<<"m_constraintStartTime > cs->lateStart";
                cs->constraintError = true;
                cs->negativeFloat = m_constraintStartTime - cs->startTime;
                cs->logError(i18nc("1=type of constraint", "%1: Failed to meet constraint. Negative float=%2", constraintToString(true), cs->negativeFloat.toString(Duration::Format_i18nHour)));
            }
            makeAppointments();
            if (cs->lateFinish > cs->endTime) {
                cs->positiveFloat = workTimeBefore(cs->lateFinish) - cs->endTime;
            }
            break;
        case Node::FinishNotLater:
            // cs->endTime calculated above
            //debugPlan<<"FinishNotLater:"<<m_constraintEndTime<<cs->endTime;
            if (cs->endTime > m_constraintEndTime) {
                cs->endTime = qMax(qMin(m_constraintEndTime, cs->lateFinish), cs->earlyFinish);
            }
            cs->endTime = workTimeBefore(cs->endTime, cs);
            cs->duration = duration(cs->endTime, use, true);
            cs->startTime = cs->endTime - cs->duration;
            if (cs->endTime > m_constraintEndTime) {
                cs->negativeFloat = cs->endTime - m_constraintEndTime;
                cs->constraintError = true;
                cs->logError(i18nc("1=type of constraint", "%1: Failed to meet constraint. Negative float=%2", constraintToString(true), cs->negativeFloat.toString(Duration::Format_i18nHour)));
            }
            makeAppointments();
            if (cs->lateFinish > cs->endTime) {
                cs->positiveFloat = workTimeBefore(cs->lateFinish) - cs->endTime;
            }
            break;
        case Node::MustStartOn:
            // Just try to schedule on time
            //debugPlan<<"MustStartOn="<<m_constraintStartTime.toString()<<""<<cs->startTime.toString();
            cs->startTime = workTimeAfter(m_constraintStartTime, cs);
            cs->duration = duration(cs->startTime, use, false);
            if (cs->endTime >= cs->startTime + cs->duration) {
                cs->endTime = cs->startTime + cs->duration;
            } else {
                cs->endTime = workTimeBefore(cs->endTime);
                cs->duration = duration(cs->endTime, use, true);
                cs->startTime = cs->endTime - cs->duration;
            }
            if (m_constraintStartTime != cs->startTime) {
                cs->constraintError = true;
                cs->negativeFloat = m_constraintStartTime - cs->startTime;
                cs->logError(i18nc("1=type of constraint", "%1: Failed to meet constraint. Negative float=%2", constraintToString(true), cs->negativeFloat.toString(Duration::Format_i18nHour)));
            }
            makeAppointments();
            if (cs->lateFinish > cs->endTime) {
                cs->positiveFloat = workTimeBefore(cs->lateFinish) - cs->endTime;
            }
            break;
        case Node::MustFinishOn:
            // Just try to schedule on time
            //debugPlan<<m_name<<"MustFinishOn:"<<m_constraintEndTime<<cs->endTime<<cs->earlyFinish;
            cs->endTime = workTimeBefore(m_constraintEndTime, cs);
            cs->duration = duration(cs->endTime, use, true);
            cs->startTime = cs->endTime - cs->duration;
            if (m_constraintEndTime != cs->endTime) {
                cs->negativeFloat = m_constraintEndTime - cs->endTime;
                cs->constraintError = true;
                cs->logError(i18nc("1=type of constraint", "%1: Failed to meet constraint. Negative float=%2", constraintToString(true), cs->negativeFloat.toString(Duration::Format_i18nHour)));
                //warnPlan<<"m_constraintEndTime > cs->endTime";
            }
            makeAppointments();
            if (cs->lateFinish > cs->endTime) {
                cs->positiveFloat = workTimeBefore(cs->lateFinish) - cs->endTime;
            }
            break;
        case Node::FixedInterval: {
            // cs->endTime calculated above
            //debugPlan<<m_name<<"FixedInterval="<<m_constraintEndTime<<""<<cs->endTime;
            cs->duration = m_constraintEndTime - m_constraintStartTime;
            if (cs->endTime > m_constraintEndTime) {
                cs->endTime = qMax(m_constraintEndTime, cs->earlyFinish);
            }
            cs->startTime = cs->endTime - cs->duration;
            if (m_constraintEndTime != cs->endTime) {
                cs->negativeFloat = m_constraintEndTime - cs->endTime;
                cs->constraintError = true;
                cs->logError(i18nc("1=type of constraint", "%1: Failed to meet constraint. Negative float=%2", constraintToString(true), cs->negativeFloat.toString(Duration::Format_i18nHour)));
            }
            cs->workStartTime = workTimeAfter(cs->startTime);
            cs->workEndTime = workTimeBefore(cs->endTime);
            makeAppointments();
            if (cs->negativeFloat == Duration::zeroDuration) {
                cs->positiveFloat = workTimeBefore(cs->lateFinish) - cs->endTime;
            }
            break;
        }
        default:
            break;
        }
        m_requests.reserve(cs->startTime, cs->duration);
        if (m_estimate->type() == Estimate::Type_Effort) {
            const qint64 granularity = this->granularity();
            const auto estimate = m_estimate->value(use, cs->usePert());
            const auto planned = cs->plannedEffort();
            const auto deviation = estimate - planned;
            if (deviation != 0) {
                warnPlan<<"Estimated effort not met:"<<estimate.toString()<<planned.toString();
                if (deviation <= granularity) {
                    cs->logInfo(i18n("Effort deviation. Estimate: %1, scheduled: %2, granularity: %3",
                                     estimate.toString(Duration::Format_i18nDayTime),
                                     planned.toString(Duration::Format_i18nDayTime),
                                     Duration(granularity).toString(Duration::Format_i18nDay)));
                }
            }
            cs->effortNotMet = deviation > granularity;
            if (cs->effortNotMet) {
                cs->logError(i18n("Effort not met. Estimate: %1, scheduled: %2", estimate.toHours(), planned.toHours()));
            }
        }
    } else if (type() == Node::Type_Milestone) {
        switch (m_constraint) {
        case Node::ASAP:
            if (cs->endTime < cs->earlyStart) {
                cs->schedulingError = true;
                cs->logError(i18nc("1=type of constraint", "%1: Failed to schedule after early start.", constraintToString()));
                cs->endTime = cs->earlyStart;
            } else {
                cs->positiveFloat = cs->lateFinish - cs->endTime;
            }
            //cs->endTime = cs->earlyStart; FIXME need to follow predeccessors. Defer scheduling?
            cs->startTime = cs->endTime;
            break;
        case Node::ALAP:
            cs->startTime = cs->endTime;
            cs->positiveFloat = cs->lateFinish - cs->endTime;
            break;
        case Node::MustStartOn:
        case Node::MustFinishOn:
        case Node::FixedInterval: {
            DateTime contime = m_constraint == Node::MustFinishOn ? m_constraintEndTime : m_constraintStartTime;
            if (contime < cs->earlyStart) {
                if (cs->earlyStart < cs->endTime) {
                    cs->endTime = cs->earlyStart;
                }
            } else if (contime < cs->endTime) {
                cs->endTime = contime;
            }
            cs->negativeFloat = cs->endTime > contime ? cs->endTime - contime : contime - cs->endTime;
            if (cs->negativeFloat != 0) {
                cs->constraintError = true;
                cs->logError(i18nc("1=type of constraint", "%1: Failed to meet constraint. Negative float=%2", constraintToString(true), cs->negativeFloat.toString(Duration::Format_i18nHour)));
            }
            cs->startTime = cs->endTime;
            if (cs->negativeFloat == Duration::zeroDuration) {
                cs->positiveFloat = cs->lateFinish - cs->endTime;
            }
            break;
        }
        case Node::StartNotEarlier:
            cs->startTime = cs->endTime;
            if (m_constraintStartTime > cs->startTime) {
                cs->constraintError = true;
                cs->negativeFloat = m_constraintStartTime - cs->startTime;
                cs->logError(i18nc("1=type of constraint", "%1: Failed to meet constraint. Negative float=%2", constraintToString(true), cs->negativeFloat.toString(Duration::Format_i18nHour)));
            }
            if (cs->negativeFloat == Duration::zeroDuration) {
                cs->positiveFloat = cs->lateFinish - cs->endTime;
            }
            break;
        case Node::FinishNotLater:
            if (m_constraintEndTime < cs->earlyStart) {
                if (cs->earlyStart < cs->endTime) {
                    cs->endTime = cs->earlyStart;
                }
            } else if (m_constraintEndTime < cs->endTime) {
                cs->endTime = m_constraintEndTime;
            }
            if (m_constraintEndTime > cs->endTime) {
                cs->constraintError = true;
                cs->negativeFloat = cs->endTime - m_constraintEndTime;
                cs->logError(i18nc("1=type of constraint", "%1: Failed to meet constraint. Negative float=%2", constraintToString(true), cs->negativeFloat.toString(Duration::Format_i18nHour)));
            }
            cs->startTime = cs->endTime;
            if (cs->negativeFloat == Duration::zeroDuration) {
                cs->positiveFloat = cs->lateFinish - cs->endTime;
            }
            break;
        default:
            break;
        }
        cs->duration = Duration::zeroDuration;
    } else if (type() == Node::Type_Summarytask) {
        //shouldn't come here
        cs->startTime = cs->endTime;
        cs->duration = cs->endTime - cs->startTime;
        warnPlan<<"Summarytasks should not be calculated here: "<<m_name;
    }
    if (cs->startTime < projectNode()->constraintStartTime() || cs->endTime > projectNode()->constraintEndTime()) {
        cs->logError(i18n("Failed to schedule within project target time"));
    }
    const auto appointments = cs->appointments();
    for (const Appointment *a : appointments) {
        cs->logInfo(i18n("Resource %1 booked from %2 to %3", a->resource()->resource()->name(), locale.toString(a->startTime(), QLocale::ShortFormat), locale.toString(a->endTime(), QLocale::ShortFormat)));
    }
    if (cs->startTime < cs->earlyStart) {
        cs->logWarning(i18n("Starting earlier than early start"));
    }
    if (cs->endTime > cs->lateFinish) {
        cs->logWarning(i18n("Finishing later than late finish"));
    }
    cs->logInfo(i18n("Scheduled: %1 to %2", locale.toString(cs->startTime, QLocale::ShortFormat), locale.toString(cs->endTime, QLocale::ShortFormat)));
    m_visitedBackward = true;
    cs->incProgress();
#ifndef PLAN_NLOGDEBUG
    cs->logDebug(QStringLiteral("Finished schedule backward: %1 ms").arg(timer.elapsed()));
#endif
    return cs->startTime;
}

void Task::adjustSummarytask()
{
    if (static_cast<Project*>(projectNode())->stopcalculation) {
        return;
    }
    if (m_currentSchedule == nullptr) {
        return;
    }
    if (type() == Type_Summarytask) {
        DateTime start = m_currentSchedule->lateFinish;
        DateTime end = m_currentSchedule->earlyStart;
        for (Node *n : std::as_const(m_nodes)) {
            n->adjustSummarytask();
            if (n->startTime() < start)
                start = n->startTime();
            if (n->endTime() > end)
                end = n->endTime();
        }
        m_currentSchedule->startTime = start;
        m_currentSchedule->endTime = end;
        m_currentSchedule->duration = end - start;
        m_currentSchedule->notScheduled = false;
        //debugPlan<<cs->name<<":"<<m_currentSchedule->startTime.toString()<<" :"<<m_currentSchedule->endTime.toString();
    }
}

Duration Task::duration(const DateTime &time, int use, bool backward) {
    //debugPlan;
    // TODO: handle risc
    if (m_currentSchedule == nullptr) {
        errorPlan<<"No current schedule";
        return Duration::zeroDuration;
    }
    if (!time.isValid()) {
#ifndef PLAN_NLOGDEBUG
        m_currentSchedule->logDebug(QStringLiteral("Calculate duration: Start time is not valid"));
#endif
        return Duration::zeroDuration;
    }
    //debugPlan<<m_name<<": Use="<<use;
    Duration eff;
    if (m_currentSchedule->recalculate() && completion().isStarted() && estimate()->type() == Estimate::Type_Effort) {
        eff = completion().remainingEffort();
        //debugPlan<<m_name<<": recalculate, effort="<<eff.toDouble(Duration::Unit_h);
        if (eff == 0 || completion().isFinished()) {
            return eff;
        }
    } else {
        eff = m_estimate->value(use, m_currentSchedule->usePert());
    }
    return calcDuration(time, eff, backward);
}


Duration Task::calcDuration(const DateTime &time, KPlato::Duration effort, bool backward) {
    //debugPlan<<"--------> calcDuration"<<(backward?"(B)":"(F)")<<m_name<<" time="<<time<<" effort="<<effort.toString(Duration::Format_Day);

    // Already checked: m_currentSchedule and time.
    Duration dur = effort; // use effort as default duration
    if (m_estimate->type() == Estimate::Type_Effort) {
        if (m_requests.isEmpty()) {
            m_currentSchedule->resourceError = true;
            m_currentSchedule->logError(i18n("No resource has been allocated"));
            return effort;
        }
        dur = m_requests.duration(time, effort, m_currentSchedule, backward);
        if (dur == Duration::zeroDuration) {
            warnPlan<<"zero duration: Resource not available";
            m_currentSchedule->resourceNotAvailable = true;
            dur = effort; //???
        }
        return dur;
    }
    if (m_estimate->type() == Estimate::Type_Duration) {
        return length(time, dur, backward);
    }
    errorPlan<<"Unsupported estimate type: "<<m_estimate->type();
    return dur;
}

Duration Task::length(const DateTime &time, KPlato::Duration duration, bool backward)
{
    return length(time, duration, m_currentSchedule, backward);
}

Duration Task::length(const DateTime &time, KPlato::Duration duration, Schedule *sch, bool backward)
{
    if (static_cast<Project*>(projectNode())->stopcalculation) {
        return Duration::zeroDuration;
    }
    //debugPlan<<"--->"<<(backward?"(B)":"(F)")<<m_name<<""<<time.toString()<<": duration:"<<duration.toString(Duration::Format_Day)<<" ("<<duration.milliseconds()<<")";

    Duration l;
    if (duration == Duration::zeroDuration) {
#ifndef PLAN_NLOGDEBUG
        if (sch) sch->logDebug(QStringLiteral("Calculate length: estimate == 0"));
#else
        Q_UNUSED(sch)
#endif
        return l;
    }
    Calendar *cal = m_estimate->calendar();
    if (cal == nullptr) {
#ifndef PLAN_NLOGDEBUG
        if (sch) sch->logDebug(QStringLiteral("Calculate length: No calendar, return estimate ") + duration.toString());
#endif
        return duration;
    }
#ifndef PLAN_NLOGDEBUG
    if (sch) sch->logDebug(QStringLiteral("Calculate length from: ") + time.toString());
#endif
    DateTime logtime = time;
    bool sts=true;
    bool match = false;
    DateTime start = time;
    int inc = backward ? -1 : 1;
    DateTime end = start;
    Duration l1;
    int nDays = backward ? projectNode()->constraintStartTime().daysTo(time) : time.daysTo(projectNode()->constraintEndTime());
    for (int i=0; !match && i <= nDays; ++i) {
        // days
        end = end.addDays(inc);
        l1 = backward ? cal->effort(end, start) : cal->effort(start, end);
        //debugPlan<<"["<<i<<"of"<<nDays<<"]"<<(backward?"(B)":"(F):")<<"  start="<<start<<" l+l1="<<(l+l1).toString()<<" match"<<duration.toString();
        if (l + l1 < duration) {
            l += l1;
            start = end;
        } else if (l + l1 == duration) {
            l += l1;
            match = true;
        } else {
            end = start;
            break;
        }
    }
    if (! match) {
#ifndef PLAN_NLOGDEBUG
        if (sch) sch->logDebug(QStringLiteral("Days: duration ") + logtime.toString() + QStringLiteral(" - ") + end.toString() + QStringLiteral(" = ") + l.toString() + QStringLiteral(" (") + (duration - l).toString() + QLatin1Char(')'));
#endif
        logtime = start;
        for (int i=0; !match && i < 24; ++i) {
            // hours
            end = end.addSecs(inc*60*60);
            l1 = backward ? cal->effort(end, start) : cal->effort(start, end);
            if (l + l1 < duration) {
                l += l1;
                start = end;
            } else if (l + l1 == duration) {
                l += l1;
                match = true;
            } else {
                end = start;
                break;
            }
            //debugPlan<<"duration(h)["<<i<<"]"<<(backward?"backward":"forward:")<<" time="<<start.time()<<" l="<<l.toString()<<" ("<<l.milliseconds()<<')';
        }
        //debugPlan<<"duration"<<(backward?"backward":"forward:")<<start.toString()<<" l="<<l.toString()<<" ("<<l.milliseconds()<<")  match="<<match<<" sts="<<sts;
    }
    if (! match) {
#ifndef PLAN_NLOGDEBUG
        if (sch) sch->logDebug(QStringLiteral("Hours: duration ") + logtime.toString() + QStringLiteral(" - ") + end.toString() + QStringLiteral(" = ") + l.toString() + QStringLiteral(" (") + (duration - l).toString() + QLatin1Char(')'));
#endif
        logtime = start;
        for (int i=0; !match && i < 60; ++i) {
            //minutes
            end = end.addSecs(inc*60);
            l1 = backward ? cal->effort(end, start) : cal->effort(start, end);
            if (l + l1 < duration) {
                l += l1;
                start = end;
            } else if (l + l1 == duration) {
                l += l1;
                match = true;
            } else if (l + l1 > duration) {
                end = start;
                break;
            }
            //debugPlan<<"duration(m)"<<(backward?"backward":"forward:")<<"  time="<<start.time().toString()<<" l="<<l.toString()<<" ("<<l.milliseconds()<<QLatin1Char(')');
        }
        //debugPlan<<"duration"<<(backward?"backward":"forward:")<<"  start="<<start.toString()<<" l="<<l.toString()<<" match="<<match<<" sts="<<sts;
    }
    if (! match) {
#ifndef PLAN_NLOGDEBUG
        if (sch) sch->logDebug(QStringLiteral("Minutes: duration ") + logtime.toString() + QStringLiteral(" - ") + end.toString() + QStringLiteral(" = ") + l.toString() + QStringLiteral(" (") + (duration - l).toString() + QLatin1Char(')'));
#endif
        logtime = start;
        for (int i=0; !match && i < 60 && sts; ++i) {
            //seconds
            end = end.addSecs(inc);
            l1 = backward ? cal->effort(end, start) : cal->effort(start, end);
            if (l + l1 < duration) {
                l += l1;
                start = end;
            } else if (l + l1 == duration) {
                l += l1;
                match = true;
            } else if (l + l1 > duration) {
                end = start;
                break;
            }
            //debugPlan<<"duration(s)["<<i<<"]"<<(backward?"backward":"forward:")<<" time="<<start.time().toString()<<" l="<<l.toString()<<" ("<<l.milliseconds()<<QLatin1Char(')');
        }
    }
    if (! match) {
#ifndef PLAN_NLOGDEBUG
        if (sch) sch->logDebug(QStringLiteral("Seconds: duration ") + logtime.toString() + QStringLiteral(" - ") + end.toString() + QStringLiteral(" l ") + l.toString() + QStringLiteral(" (") + (duration - l).toString() + QLatin1Char(')'));
#endif
        for (int i=0; !match && i < 1000; ++i) {
            //milliseconds
            end.setTime(end.time().addMSecs(inc));
            l1 = backward ? cal->effort(end, start) : cal->effort(start, end);
            if (l + l1 < duration) {
                l += l1;
                start = end;
            } else if (l + l1 == duration) {
                l += l1;
                match = true;
            } else {
#ifndef PLAN_NLOGDEBUG
                if (sch) sch->logDebug(QStringLiteral("Got more than asked for, should not happen! Want: ") + duration.toString(Duration::Format_Hour) + QStringLiteral(" got: ") + l.toString(Duration::Format_Hour));
#endif
                break;
            }
            //debugPlan<<"duration(ms)["<<i<<"]"<<(backward?"backward":"forward:")<<" time="<<start.time().toString()<<" l="<<l.toString()<<" ("<<l.milliseconds()<<QLatin1Char(')');
        }
    }
    if (!match) {
        m_currentSchedule->logError(i18n("Could not match work duration. Want: %1 got: %2",  l.toString(Duration::Format_i18nHour), duration.toString(Duration::Format_i18nHour)));
    }
    DateTime t = end;
    if (l != Duration::zeroDuration) {
        if (backward) {
            if (end < projectNode()->constraintEndTime()) {
                t = cal->firstAvailableAfter(end, projectNode()->constraintEndTime());
            }
        } else {
            if (end > projectNode()->constraintStartTime()) {
                t = cal->firstAvailableBefore(end, projectNode()->constraintStartTime());
            }
        }
#ifndef PLAN_NLOGDEBUG
        if (sch) sch->logDebug(QStringLiteral("Moved end to work: ") + end.toString() + QStringLiteral(" -> ") + t.toString());
#endif
    }
    end = t.isValid() ? t : time;
    //debugPlan<<"<---"<<(backward?"(B)":"(F)")<<m_name<<":"<<end.toString()<<"-"<<time.toString()<<"="<<(end - time).toString()<<" duration:"<<duration.toString(Duration::Format_Day);
    l = end>time ? end-time : time-end;
    if (match) {
#ifndef PLAN_NLOGDEBUG
        if (sch) sch->logDebug(QStringLiteral("Calculated length: ") + time.toString() + QStringLiteral(" - ") + end.toString() + QStringLiteral(" = ") + l.toString());
#endif
    }
    return l;
}

void Task::clearProxyRelations()
{
    m_parentProxyRelations.clear();
    m_childProxyRelations.clear();
}

void Task::addParentProxyRelations(const QList<Relation*> &list)
{
    //debugPlan<<this<<list;
    if (type() == Type_Summarytask) {
        // propagate to my children
        //debugPlan<<m_name<<" is summary task";
        for (Node *n : std::as_const(m_nodes)) {
            n->addParentProxyRelations(list);
            n->addParentProxyRelations(dependParentNodes());
        }
    } else {
        // add 'this' as child relation to the relations parent
        //debugPlan<<m_name<<" is not summary task";
        for (Relation *r : list) {
            r->parent()->addChildProxyRelation(this, r);
            // add a parent relation to myself
            addParentProxyRelation(r->parent(), r);
        }
    }
}

void Task::addChildProxyRelations(const QList<Relation*> &list) {
    //debugPlan<<this<<list;
    if (type() == Type_Summarytask) {
        // propagate to my children
        //debugPlan<<m_name<<" is summary task";
        for (Node *n : std::as_const(m_nodes)) {
            n->addChildProxyRelations(list);
            n->addChildProxyRelations(dependChildNodes());
        }
    } else {
        // add 'this' as parent relation to the relations child
        //debugPlan<<m_name<<" is not summary task";
        for (Relation *r : list) {
            r->child()->addParentProxyRelation(this, r);
            // add a child relation to myself
            addChildProxyRelation(r->child(), r);
        }
    }
}

void Task::addParentProxyRelation(Node *node, const Relation *rel) {
    if (node->type() != Type_Summarytask) {
        if (type() == Type_Summarytask) {
            //debugPlan<<"Add parent proxy from my children"<<m_name<<" to"<<node->name();
            for (Node *n : std::as_const(m_nodes)) {
                n->addParentProxyRelation(node, rel);
            }
        } else {
            //debugPlan<<"Add parent proxy from"<<node<<" to (me)"<<this;
            m_parentProxyRelations.append(new ProxyRelation(node, this, rel->type(), rel->lag()));
        }
        //debugPlan<<this<<rel<<parentProxyRelations();
    }
}

void Task::addChildProxyRelation(Node *node, const Relation *rel) {
    if (node->type() != Type_Summarytask) {
        if (type() == Type_Summarytask) {
            //debugPlan<<"Add child proxy from my children"<<m_name<<" to"<<node->name();
            for (Node *n : std::as_const(m_nodes)) {
                n->addChildProxyRelation(node, rel);
            }
        } else {
            //debugPlan<<"Add child proxy from (me)"<<this<<" to"<<node;
            m_childProxyRelations.append(new ProxyRelation(this, node, rel->type(), rel->lag()));
        }
        //debugPlan<<this<<rel<<childProxyRelations();
    }
}

bool Task::isEndNode() const
{
    return m_dependChildNodes.isEmpty() && m_childProxyRelations.isEmpty();
}
bool Task::isStartNode() const
{
    return m_dependParentNodes.isEmpty() && m_parentProxyRelations.isEmpty();
}

QList<Relation*> Task::parentProxyRelations() const
{
    return  m_parentProxyRelations;
}

QList<Relation*> Task::childProxyRelations() const
{
    return  m_childProxyRelations;
}

DateTime Task::workTimeAfter(const DateTime &dt, Schedule *sch) const {
    DateTime t;
    if (m_estimate->type() == Estimate::Type_Duration) {
        if (m_estimate->calendar()) {
            t = m_estimate->calendar()->firstAvailableAfter(dt, projectNode()->constraintEndTime());
        }
    } else {
        t = m_requests.workTimeAfter(dt, sch);
#ifndef PLAN_NLOGDEBUG
        if (sch) sch->logDebug(QStringLiteral("workTimeAfter: %1 = %2").arg(dt.toString()).arg(t.toString()));
#endif
    }
    return t.isValid() ? t : dt;
}

DateTime Task::workTimeBefore(const DateTime &dt, Schedule *sch) const {
    DateTime t;
    if (m_estimate->type() == Estimate::Type_Duration) {
        if (m_estimate->calendar()) {
            t = m_estimate->calendar()->firstAvailableBefore(dt, projectNode()->constraintStartTime());
        }
    } else {
        t = m_requests.workTimeBefore(dt, sch);
    }
    return t.isValid() ? t : dt;
}

Duration Task::positiveFloat(long id) const
{
    Schedule *s = schedule(id);
    return s == nullptr ? Duration::zeroDuration : s->positiveFloat;
}

void Task::setPositiveFloat(KPlato::Duration fl, long id) const
{
    Schedule *s = schedule(id);
    if (s)
        s->positiveFloat = fl;
}

Duration Task::negativeFloat(long id) const
{
    Schedule *s = schedule(id);
    return s == nullptr ? Duration::zeroDuration : s->negativeFloat;
}

void Task::setNegativeFloat(KPlato::Duration fl, long id) const
{
    Schedule *s = schedule(id);
    if (s)
        s->negativeFloat = fl;
}

Duration Task::freeFloat(long id) const
{
    Schedule *s = schedule(id);
    return s == nullptr ? Duration::zeroDuration : s->freeFloat;
}

void Task::setFreeFloat(KPlato::Duration fl, long id) const
{
    Schedule *s = schedule(id);
    if (s)
        s->freeFloat = fl;
}

Duration Task::startFloat(long id) const
{
    Schedule *s = schedule(id);
    return s == nullptr || s->earlyStart > s->lateStart ? Duration::zeroDuration : (s->earlyStart - s->lateStart);
}

Duration Task::finishFloat(long id) const
{
    Schedule *s = schedule(id);
    return s == nullptr || s->lateFinish < s->earlyFinish ? Duration::zeroDuration : (s->lateFinish - s->earlyFinish);
}

bool Task::isCritical(long id) const
{
    Schedule *s = schedule(id);
    return s == nullptr ? false : s->isCritical();
}

bool Task::calcCriticalPath(bool fromEnd)
{
    if (m_currentSchedule == nullptr)
        return false;
    //debugPlan<<m_name<<" fromEnd="<<fromEnd<<" cp="<<m_currentSchedule->inCriticalPath;
    if (m_currentSchedule->inCriticalPath) {
        return true; // path already calculated
    }
    if (!isCritical()) {
        return false;
    }
    if (fromEnd) {
        if (isEndNode() && startFloat() == 0 && finishFloat() == 0) {
            m_currentSchedule->inCriticalPath = true;
            //debugPlan<<m_name<<" end node";
            return true;
        }
        for (Relation *r : std::as_const(m_childProxyRelations)) {
            if (r->child()->calcCriticalPath(fromEnd)) {
                m_currentSchedule->inCriticalPath = true;
            }
        }
        for (Relation *r : std::as_const(m_dependChildNodes)) {
            if (r->child()->calcCriticalPath(fromEnd)) {
                m_currentSchedule->inCriticalPath = true;
            }
        }
    } else {
        if (isStartNode() && startFloat() == 0 && finishFloat() == 0) {
            m_currentSchedule->inCriticalPath = true;
            //debugPlan<<m_name<<" start node";
            return true;
        }
        for (Relation *r : std::as_const(m_parentProxyRelations)) {
            if (r->parent()->calcCriticalPath(fromEnd)) {
                m_currentSchedule->inCriticalPath = true;
            }
        }
        for (Relation *r : std::as_const(m_dependParentNodes)) {
            if (r->parent()->calcCriticalPath(fromEnd)) {
                m_currentSchedule->inCriticalPath = true;
            }
        }
    }
    //debugPlan<<m_name<<" return cp="<<m_currentSchedule->inCriticalPath;
    return m_currentSchedule->inCriticalPath;
}

void Task::calcFreeFloat()
{
    //debugPlan<<m_name;
    if (type() == Node::Type_Summarytask) {
        Node::calcFreeFloat();
        return;
    }
    Schedule *cs = m_currentSchedule;
    if (cs == nullptr) {
        return;
    }
    DateTime t;
    for (Relation *r : std::as_const(m_dependChildNodes)) {
        DateTime c = r->child()->startTime();
        if (!t.isValid() || c < t) {
            t = c;
        }
    }
    for (Relation *r : std::as_const(m_childProxyRelations)) {
        DateTime c = r->child()->startTime();
        if (!t.isValid() || c < t) {
            t = c;
        }
    }
    if (t.isValid() && t > cs->endTime) {
        cs->freeFloat = t - cs->endTime;
        //debugPlan<<m_name<<": "<<cs->freeFloat.toString();
    }
}

void Task::setCurrentSchedule(long id)
{
    setCurrentSchedulePtr(findSchedule(id));
    Node::setCurrentSchedule(id);
}

bool Task::effortMetError(long id) const
{
    Schedule *s = schedule(id);
    if (s == nullptr || s->notScheduled || m_estimate->type() != Estimate::Type_Effort) {
        return false;
    }
    return s->effortNotMet;
}

uint Task::state(long id) const
{
    int st = Node::State_None;
    if (! isScheduled(id)) {
        st |= State_NotScheduled;
    }
    const auto sch = findSchedule(id);
    if (sch) {
        if (sch->resourceError) {
            st |= State_Error;
        }
    }
    if (completion().isFinished()) {
        st |= Node::State_Finished;
        if (completion().finishTime() > endTime(id)) {
            st |= State_FinishedLate;
        }
        if (completion().finishTime() < endTime(id)) {
            st |= State_FinishedEarly;
        }
    } else if (completion().isStarted()) {
        st |= Node::State_Started;
        if (completion().startTime() > startTime(id)) {
            st |= State_StartedLate;
        }
        if (completion().startTime() < startTime(id)) {
            st |= State_StartedEarly;
        }
        if (completion().percentFinished() > 0) {
            st |= State_Running;
        }
        if (endTime(id) < QDateTime::currentDateTime()) {
            st |= State_Late;
        }
    } else if (isScheduled(id)) {
        if (startTime(id) < QDateTime::currentDateTime()) {
            st |= State_Late;
        }
    }
    st |= State_ReadyToStart;
    //TODO: check proxy relations
    for (const Relation *r : std::as_const(m_dependParentNodes)) {
        if (! static_cast<Task*>(r->parent())->completion().isFinished()) {
            st &= ~Node::State_ReadyToStart;
            st |= Node::State_NotReadyToStart;
            break;
        }
    }
    return st;
}

Completion &Task::completion()
{
    return m_workPackage.completion();
}

const Completion &Task::completion() const
{
    return m_workPackage.completion();
}

WorkPackage &Task::workPackage()
{ return m_workPackage;
}

const WorkPackage &Task::workPackage() const
{
    return m_workPackage;
}

int Task::workPackageLogCount() const
{
    return m_packageLog.count();
}

QList<WorkPackage*> Task::workPackageLog() const
{
    return m_packageLog;
}

void Task::addWorkPackage(WorkPackage *wp)
{
    Q_EMIT workPackageToBeAdded(this, m_packageLog.count());
    m_packageLog.append(wp);
    Q_EMIT workPackageAdded(this);
}

void Task::removeWorkPackage(WorkPackage *wp)
{
    int index = m_packageLog.indexOf(wp);
    if (index < 0) {
        return;
    }
    Q_EMIT workPackageToBeRemoved(this, index);
    m_packageLog.removeAt(index);
    Q_EMIT workPackageRemoved(this);
}

WorkPackage *Task::workPackageAt(int index) const
{
    Q_ASSERT (index >= 0 && index < m_packageLog.count());
    return m_packageLog.at(index);
}

QString Task::wpOwnerName() const
{
    if (m_packageLog.isEmpty()) {
        return m_workPackage.ownerName();
    }
    return m_packageLog.last()->ownerName();
}

WorkPackage::WPTransmitionStatus Task::wpTransmitionStatus() const
{
    if (m_packageLog.isEmpty()) {
        return m_workPackage.transmitionStatus();
    }
    return m_packageLog.last()->transmitionStatus();
}

DateTime Task::wpTransmitionTime() const
{
    if (m_packageLog.isEmpty()) {
        return m_workPackage.transmitionTime();
    }
    return m_packageLog.last()->transmitionTime();
}

QList<Resource*> Task::usedResources(Schedule *ns) const
{
    QList<Resource*> resources;
    if (ns && ns->parent() && ns->parent()->manager() && ns->parent()->manager()->parentManager()  && isStarted()) {
        const auto parentManager = ns->parent()->manager()->parentManager();
        const auto id = parentManager->expected()->id();
        const auto schedule = m_schedules.value(id);
        Q_ASSERT(schedule);
        ns->logDebug(QStringLiteral("Search for resource in manager: %1").arg(parentManager->name()));
        const auto apps = schedule->appointments();
        for (const auto a : apps) {
            ns->logDebug(QStringLiteral("Resource '%1': Appointment empty: %2").arg(a->resource()->resource()->name()).arg(a->isEmpty()));
            resources << a->resource()->resource();
        }
    }
    return resources;
}

}  //KPlato namespace
