/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2005-2011, 2012 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "kptschedule.h"

#include "kptappointment.h"
#include "kptdatetime.h"
#include "kptduration.h"
#include "kptnode.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptxmlloaderobject.h"
#include "kptschedulerplugin.h"
#include "ProjectLoaderBase.h"
#include "kptdebug.h"

#include <KoXmlReader.h>

#include <KLocalizedString>

#include <QStringList>


namespace KPlato
{

class ScheduleManager;

Schedule::Log::Log(const Node *n, int sev, const QString &msg, int ph)
    : node(n), resource(nullptr), message(msg), severity(sev), phase(ph)
{
    Q_ASSERT(n);
//     debugPlan<<*this<<nodeId;
}

Schedule::Log::Log(const Node *n, const Resource *r, int sev, const QString &msg, int ph)
    : node(n), resource(r), message(msg), severity(sev), phase(ph)
{
    Q_ASSERT(r);
//     debugPlan<<*this<<resourceId;
}

Schedule::Log::Log(const Log &other)
{
    node = other.node;
    resource = other.resource;
    message = other.message;
    severity = other.severity;
    phase = other.phase;
}

Schedule::Log &Schedule::Log::operator=(const Schedule::Log &other)
{
    node = other.node;
    resource = other.resource;
    message = other.message;
    severity = other.severity;
    phase = other.phase;
    return *this;
}

Schedule::Schedule()
        : m_type(Expected),
        m_id(0),
        m_deleted(false),
        m_parent(nullptr),
        m_obstate(OBS_Parent),
        m_calculationMode(Schedule::Scheduling),
        resourceError(false),
        resourceOverbooked(false),
        resourceNotAvailable(false),
        constraintError(false),
        notScheduled(true),
        effortNotMet(false),
        schedulingError(false),
        schedulingCanceled(false)
{
    initiateCalculation();
}

Schedule::Schedule(Schedule *parent)
        : m_type(Expected),
        m_id(0),
        m_deleted(false),
        m_parent(parent),
        m_obstate(OBS_Parent),
        m_calculationMode(Schedule::Scheduling),
        resourceError(false),
        resourceOverbooked(false),
        resourceNotAvailable(false),
        constraintError(false),
        notScheduled(true),
        effortNotMet(false),
        schedulingError(false),
        schedulingCanceled(false)
{

    if (parent) {
        m_name = parent->name();
        m_type = parent->type();
        m_id = parent->id();
    }
    initiateCalculation();
    //debugPlan<<"("<<this<<") Name: '"<<name<<"' Type="<<type<<" id="<<id;
}

Schedule::Schedule(const QString& name, Type type, long id)
        : m_name(name),
        m_type(type),
        m_id(id),
        m_deleted(false),
        m_parent(nullptr),
        m_obstate(OBS_Parent),
        m_calculationMode(Schedule::Scheduling),
        resourceError(false),
        resourceOverbooked(false),
        resourceNotAvailable(false),
        constraintError(false),
        notScheduled(true),
        effortNotMet(false),
        schedulingError(false),
        schedulingCanceled(false)
{
    //debugPlan<<"("<<this<<") Name: '"<<name<<"' Type="<<type<<" id="<<id;
    initiateCalculation();
}

Schedule::~Schedule()
{
}

void Schedule::setParent(Schedule *parent)
{
    m_parent = parent;
}

void Schedule::setDeleted(bool on)
{
    //debugPlan<<"deleted="<<on;
    m_deleted = on;
    //changed(this); don't do this!
}

bool Schedule::isDeleted() const
{
    return m_parent == nullptr ? m_deleted : m_parent->isDeleted();
}

void Schedule::setType(const QString& type)
{
    m_type = Expected;
    if (type == QStringLiteral("Expected"))
        m_type = Expected;
}

QString Schedule::typeToString(bool translate) const
{
    if (translate) {
        return i18n("Expected");
    } else {
        return QStringLiteral("Expected");
    }
}

QStringList Schedule::state() const
{
    QStringList lst;
    if (m_deleted)
        lst << SchedulingState::deleted();
    if (notScheduled)
        lst << SchedulingState::notScheduled();
    if (constraintError)
        lst << SchedulingState::constraintsNotMet();
    if (resourceError)
        lst << SchedulingState::resourceNotAllocated();
    if (resourceNotAvailable)
        lst << SchedulingState::resourceNotAvailable();
    if (resourceOverbooked)
        lst << SchedulingState::resourceOverbooked();
    if (effortNotMet)
        lst << SchedulingState::effortNotMet();
    if (schedulingError)
        lst << SchedulingState::schedulingError();
    if (schedulingCanceled)
        lst << SchedulingState::schedulingCanceled();
    if (lst.isEmpty())
        lst << SchedulingState::scheduled();
    return lst;
}

bool Schedule::isBaselined() const
{
    if (m_parent) {
        return m_parent->isBaselined();
    }
    return false;
}

bool Schedule::usePert() const
{
    if (m_parent) {
        return m_parent->usePert();
    }
    return false;
}

void Schedule::setAllowOverbookingState(Schedule::OBState state)
{
    m_obstate = state;
}

Schedule::OBState Schedule::allowOverbookingState() const
{
    return m_obstate;
}

bool Schedule::allowOverbooking() const
{
    if (m_obstate == OBS_Parent && m_parent) {
        return m_parent->allowOverbooking();
    }
    return m_obstate == OBS_Allow;
}

bool Schedule::checkExternalAppointments() const
{
    if (m_parent) {
        return m_parent->checkExternalAppointments();
    }
    return false;
}

void Schedule::setScheduled(bool on)
{
    notScheduled = !on;
    changed(this);
}

Duration Schedule::effort(const DateTimeInterval &interval) const
{
    return interval.second - interval.first;
}

DateTimeInterval Schedule::available(const DateTimeInterval &interval) const
{
    return DateTimeInterval(interval.first, interval.second);
}

void Schedule::initiateCalculation()
{
    resourceError = false;
    resourceOverbooked = false;
    resourceNotAvailable = false;
    constraintError = false;
    schedulingError = false;
    inCriticalPath = false;
    effortNotMet = false;
    workStartTime = DateTime();
    workEndTime = DateTime();
}

void Schedule::calcResourceOverbooked()
{
    resourceOverbooked = false;
    for (Appointment *a : std::as_const(m_appointments)) {
        if (a->resource() ->isOverbooked(a->startTime(), a->endTime())) {
            resourceOverbooked = true;
            break;
        }
    }
}

DateTimeInterval Schedule::firstBookedInterval(const DateTimeInterval &interval, const Schedule *node) const
{
    QList<Appointment*> lst = m_appointments;
    switch (m_calculationMode) {
        case CalculateForward: lst = m_forward; break;
        case CalculateBackward: lst = m_backward; break;
        default: break;
    }
    for (Appointment *a : std::as_const(lst)) {
        if (a->node() == node) {
            AppointmentIntervalList i = a->intervals(interval.first, interval.second);
            if (i.isEmpty()) {
                break;
            }
            return DateTimeInterval(i.map().values().first().startTime(), i.map().values().first().endTime());
        }
    }
    return DateTimeInterval();
}

QStringList Schedule::overbookedResources() const
{
    QStringList rl;
    for (Appointment *a : std::as_const(m_appointments)) {
        if (a->resource() ->isOverbooked(a->startTime(), a->endTime())) {
            rl += a->resource() ->resource() ->name();
        }
    }
    return rl;
}

QStringList Schedule::resourceNameList() const
{
    return QStringList();
}

QList<Resource*> Schedule::resources() const
{
    return QList<Resource*>();
}

void Schedule::saveXML(QDomElement &element) const
{
    QDomElement sch = element.ownerDocument().createElement(QStringLiteral("task-schedule"));
    element.appendChild(sch);
    saveCommonXML(sch);
}

void Schedule::saveCommonXML(QDomElement &element) const
{
    //debugPlan<<m_name<<" save schedule";
    element.setAttribute(QStringLiteral("name"), m_name);
    element.setAttribute(QStringLiteral("type"), typeToString());
    element.setAttribute(QStringLiteral("id"), QString::number(qlonglong(m_id)));
}

void Schedule::saveAppointments(QDomElement &element) const
{
    //debugPlan;
    QListIterator<Appointment*> it = m_appointments;
    while (it.hasNext()) {
        it.next() ->saveXML(element);
    }
}

void Schedule::insertForwardNode(Node *node)
{
    if (m_parent) {
        m_parent->insertForwardNode(node);
    }
}

void Schedule::insertBackwardNode(Node *node)
{
    if (m_parent) {
        m_parent->insertBackwardNode(node);
    }
}

// used (directly) when appointment wants to attach itself again
bool Schedule::attach(Appointment *appointment)
{
    int mode = appointment->calculationMode();
    //debugPlan<<appointment<<mode;
    if (mode == Scheduling) {
        if (m_appointments.indexOf(appointment) != -1) {
            errorPlan << "Appointment already exists" << '\n';
            return false;
        }
        m_appointments.append(appointment);
        //if (resource()) debugPlan<<appointment<<" For resource '"<<resource()->name()<<"'"<<" count="<<m_appointments.count();
        //if (node()) debugPlan<<"("<<this<<")"<<appointment<<" For node '"<<node()->name()<<"'"<<" count="<<m_appointments.count();
        return true;
    }
    if (mode == CalculateForward) {
        if (m_forward.indexOf(appointment) != -1) {
            errorPlan << "Appointment already exists" << '\n';
            return false;
        }
        m_forward.append(appointment);
        //if (resource()) debugPlan<<"For resource '"<<resource()->name()<<"'";
        //if (node()) debugPlan<<"For node '"<<node()->name()<<"'";
        return true;
    }
    if (mode == CalculateBackward) {
        if (m_backward.indexOf(appointment) != -1) {
            errorPlan << "Appointment already exists" << '\n';
            return false;
        }
        m_backward.append(appointment);
        //if (resource()) debugPlan<<"For resource '"<<resource()->name()<<"'";
        //if (node()) debugPlan<<"For node '"<<node()->name()<<"'";
        return true;
    }
    errorPlan<<"Unknown mode: "<<m_calculationMode<<'\n';
    return false;
}

// used to add new schedules
bool Schedule::add(Appointment *appointment)
{
    //debugPlan<<this;
    appointment->setCalculationMode(m_calculationMode);
    return attach(appointment);
}

void Schedule::takeAppointment(Appointment *appointment, int mode)
{
    Q_UNUSED(mode);
    //debugPlan<<"("<<this<<")"<<mode<<":"<<appointment<<","<<appointment->calculationMode();
    int i = m_forward.indexOf(appointment);
    if (i != -1) {
        m_forward.removeAt(i);
        Q_ASSERT(mode == CalculateForward);
    }
    i = m_backward.indexOf(appointment);
    if (i != -1) {
        m_backward.removeAt(i);
        Q_ASSERT(mode == CalculateBackward);
    }
    i = m_appointments.indexOf(appointment);
    if (i != -1) {
        m_appointments.removeAt(i);
        Q_ASSERT(mode == Scheduling);
    }
}

Appointment *Schedule::findAppointment(Schedule *resource, Schedule *node, int mode)
{
    //debugPlan<<this<<" ("<<resourceError<<","<<node<<")"<<mode;
    if (mode == Scheduling) {
        for (Appointment *a : std::as_const(m_appointments)) {
            if (a->node() == node && a->resource() == resource) {
                return a;
            }
        }
        return nullptr;
    } else if (mode == CalculateForward) {
        for (Appointment *a : std::as_const(m_forward)) {
            if (a->node() == node && a->resource() == resource) {
                return a;
            }
        }
    } else if (mode == CalculateBackward) {
        for (Appointment *a : std::as_const(m_backward)) {
            if (a->node() == node && a->resource() == resource) {
                Q_ASSERT(mode == CalculateBackward);
                return a;
            }
        }
    } else {
        Q_ASSERT(false); // unknown mode
    }
    return nullptr;
}

DateTime Schedule::appointmentStartTime() const
{
    DateTime dt;
    for (const Appointment *a : std::as_const(m_appointments)) {
        if (! dt.isValid() || dt > a->startTime()) {
            dt = a->startTime();
        }
    }
    return dt;
}
DateTime Schedule::appointmentEndTime() const
{
    DateTime dt;
    for (const Appointment *a : std::as_const(m_appointments)) {
        if (! dt.isValid() || dt > a->endTime()) {
            dt = a->endTime();
        }
    }
    return dt;
}

bool Schedule::hasAppointments(int which) const
{
    if (which == CalculateForward) {
        return m_forward.isEmpty();
    } else if (which == CalculateBackward) {
        return m_backward.isEmpty();
    }
    return m_appointments.isEmpty();
}

QList<Appointment*> Schedule::appointments(int which) const
{
    if (which == CalculateForward) {
        return m_forward;
    } else if (which == CalculateBackward) {
        return m_backward;
    }
    return m_appointments;
}

Appointment Schedule::appointmentIntervals(int which, const DateTimeInterval &interval) const
{
    Appointment app;
    if (which == Schedule::CalculateForward) {
        //debugPlan<<"list == CalculateForward";
        for (const Appointment *a : std::as_const(m_forward)) {
            app += interval.isValid() ? a->extractIntervals(interval) : *a;
        }
        return app;
    } else if (which == Schedule::CalculateBackward) {
        //debugPlan<<"list == CalculateBackward";
        for (const Appointment *a : std::as_const(m_backward)) {
            app += interval.isValid() ? a->extractIntervals(interval) : *a;
        }
        //debugPlan<<"list == CalculateBackward:"<<m_backward.count();
        return app;
    }
    for (const Appointment *a : std::as_const(m_appointments)) {
        app += interval.isValid() ? a->extractIntervals(interval) : *a;
    }
    return app;
}

void Schedule::copyAppointments(Schedule::CalculationMode from, Schedule::CalculationMode to)
{
    switch (to) {
        case Scheduling:
            m_appointments.clear();
            switch(from) {
                case CalculateForward:
                    m_appointments = m_forward;
                    break;
                case CalculateBackward:
                    m_appointments = m_backward;
                    break;
                default:
                    break;
            }
            break;
        case CalculateForward: break;
        case CalculateBackward: break;
    }
}

EffortCostMap Schedule::bcwsPrDay(EffortCostCalculationType type) const
{
    return const_cast<Schedule*>(this)->bcwsPrDay(type);
}

EffortCostMap Schedule::bcwsPrDay(EffortCostCalculationType type)
{
    //debugPlan<<m_name<<m_appointments;
    EffortCostCache &ec = m_bcwsPrDay[ (int)type ];
    if (! ec.cached) {
        for (const Appointment *a : std::as_const(m_appointments)) {
            ec.effortcostmap += a->plannedPrDay(a->startTime().date(), a->endTime().date(), type);
        }
    }
    return ec.effortcostmap;
}

EffortCostMap Schedule::plannedEffortCostPrDay(const QDate &start, const QDate &end, EffortCostCalculationType type) const
{
    //debugPlan<<m_name<<m_appointments;
    EffortCostMap ec;
    QListIterator<Appointment*> it(m_appointments);
    while (it.hasNext()) {
        //debugPlan<<m_name;
        ec += it.next() ->plannedPrDay(start, end, type);
    }
    return ec;
}

EffortCostMap Schedule::plannedEffortCostPrDay(const Resource *resource, const QDate &start, const QDate &end, EffortCostCalculationType type) const
{
    //debugPlan<<m_name<<m_appointments;
    EffortCostMap ec;
    for (const Appointment *a : std::as_const(m_appointments)) {
        if (a->resource() && a->resource()->resource() == resource) {
            ec += a->plannedPrDay(start, end, type);
            break;
        }
    }
    return ec;
}

Duration Schedule::plannedEffort(const Resource *resource, EffortCostCalculationType type) const
{
    //debugPlan;
    Duration eff;
    QListIterator<Appointment*> it(m_appointments);
    while (it.hasNext()) {
        eff += it.next() ->plannedEffort(resource, type);
    }
    return eff;
}

Duration Schedule::plannedEffort(EffortCostCalculationType type) const
{
    //debugPlan;
    Duration eff;
    QListIterator<Appointment*> it(m_appointments);
    while (it.hasNext()) {
        eff += it.next() ->plannedEffort(type);
    }
    return eff;
}

Duration Schedule::plannedEffort(const QDate &date, EffortCostCalculationType type) const
{
    //debugPlan;
    Duration eff;
    QListIterator<Appointment*> it(m_appointments);
    while (it.hasNext()) {
        eff += it.next() ->plannedEffort(date, type);
    }
    return eff;
}

Duration Schedule::plannedEffort(const Resource *resource, const QDate &date, EffortCostCalculationType type) const
{
    //debugPlan;
    Duration eff;
    QListIterator<Appointment*> it(m_appointments);
    while (it.hasNext()) {
        eff += it.next() ->plannedEffort(resource, date, type);
    }
    return eff;
}

Duration Schedule::plannedEffortTo(const QDate &date, EffortCostCalculationType type) const
{
    //debugPlan;
    Duration eff;
    QListIterator<Appointment*> it(m_appointments);
    while (it.hasNext()) {
        eff += it.next() ->plannedEffortTo(date, type);
    }
    return eff;
}

Duration Schedule::plannedEffortTo(const Resource *resource, const QDate &date, EffortCostCalculationType type) const
{
    //debugPlan;
    Duration eff;
    QListIterator<Appointment*> it(m_appointments);
    while (it.hasNext()) {
        eff += it.next() ->plannedEffortTo(resource, date, type);
    }
    return eff;
}

Duration Schedule::plannedEffortTo(const QDateTime &time, EffortCostCalculationType type) const
{
    //debugPlan;
    Duration eff;
    QListIterator<Appointment*> it(m_appointments);
    while (it.hasNext()) {
        eff += it.next() ->plannedEffortTo(time, type);
    }
    return eff;
}

EffortCost Schedule::plannedCost(EffortCostCalculationType type) const
{
    //debugPlan;
    EffortCost c;
    QListIterator<Appointment*> it(m_appointments);
    while (it.hasNext()) {
        c += it.next() ->plannedCost(type);
    }
    return c;
}

double Schedule::plannedCost(const QDate &date, EffortCostCalculationType type) const
{
    //debugPlan;
    double c = 0;
    QListIterator<Appointment*> it(m_appointments);
    while (it.hasNext()) {
        c += it.next() ->plannedCost(date, type);
    }
    return c;
}

double Schedule::plannedCostTo(const QDate &date, EffortCostCalculationType type) const
{
    //debugPlan;
    double c = 0;
    QListIterator<Appointment*> it(m_appointments);
    while (it.hasNext()) {
        c += it.next() ->plannedCostTo(date, type);
    }
    return c;
}

void Schedule::addLog(const Schedule::Log &log)
{
    if (m_parent) {
        m_parent->addLog(log);
    }
}

QString Schedule::Log::formatMsg() const
 {
    QString s;
    s += node ? QStringLiteral("%1 ").arg(node->name(), -8) : QString();
    s += resource ? QStringLiteral("%1 ").arg(resource->name(), -8) : QString();
    s += message;
    return s;
}

void Schedule::clearPerformanceCache()
{
    m_bcwsPrDay.clear();
    m_bcwpPrDay.clear();
    m_acwp.clear();
}

//-------------------------------------------------
NodeSchedule::NodeSchedule()
        : Schedule(),
        m_node(nullptr)
{
    //debugPlan<<"("<<this<<")";
    init();
}

NodeSchedule::NodeSchedule(Node *node, const QString& name, Schedule::Type type, long id)
        : Schedule(name, type, id),
        m_node(node)
{
    //debugPlan<<"node name:"<<node->name();
    init();
}

NodeSchedule::NodeSchedule(Schedule *parent, Node *node)
        : Schedule(parent),
        m_node(node)
{

    //debugPlan<<"node name:"<<node->name();
    init();
}

NodeSchedule::~NodeSchedule()
{
    //debugPlan<<this<<""<<m_appointments.count();
    while (!m_appointments.isEmpty()) {
        Appointment *a = m_appointments.takeFirst();
        a->setNode(nullptr);
        delete a;
    }
    //debugPlan<<"forw"<<m_forward.count();
    while (!m_forward.isEmpty()) {
        Appointment *a = m_forward.takeFirst();
        a->setNode(nullptr);
        delete a;
    }
    //debugPlan<<"backw"<<m_backward.count();
    while (!m_backward.isEmpty()) {
        Appointment *a = m_backward.takeFirst();
        a->setNode(nullptr);
        delete a;
    }
}

void NodeSchedule::init()
{
    resourceError = false;
    resourceOverbooked = false;
    resourceNotAvailable = false;
    constraintError = false;
    schedulingError = false;
    notScheduled = true;
    inCriticalPath = false;
    m_calculationMode = Schedule::Scheduling;
    positiveFloat = Duration::zeroDuration;
    negativeFloat = Duration::zeroDuration;
    freeFloat = Duration::zeroDuration;
}

void NodeSchedule::setDeleted(bool on)
{
    //debugPlan<<"deleted="<<on;
    m_deleted = on;
    // set deleted also for possible resource schedules
    QListIterator<Appointment*> it = m_appointments;
    while (it.hasNext()) {
        Appointment * a = it.next();
        if (a->resource()) {
            a->resource() ->setDeleted(on);
        }
    }
}

void NodeSchedule::saveXML(QDomElement &element) const
{
    //debugPlan;
    QDomElement sch = element.ownerDocument().createElement(QStringLiteral("task-schedule"));
    element.appendChild(sch);
    saveCommonXML(sch);

    if (earlyStart.isValid()) {
        sch.setAttribute(QStringLiteral("earlystart"), earlyStart.toString(Qt::ISODate));
    }
    if (lateStart.isValid()) {
        sch.setAttribute(QStringLiteral("latestart"), lateStart.toString(Qt::ISODate));
    }
    if (earlyFinish.isValid()) {
        sch.setAttribute(QStringLiteral("earlyfinish"), earlyFinish.toString(Qt::ISODate));
    }
    if (lateFinish.isValid()) {
        sch.setAttribute(QStringLiteral("latefinish"), lateFinish.toString(Qt::ISODate));
    }
    if (startTime.isValid())
        sch.setAttribute(QStringLiteral("start"), startTime.toString(Qt::ISODate));
    if (endTime.isValid())
        sch.setAttribute(QStringLiteral("end"), endTime.toString(Qt::ISODate));
    if (workStartTime.isValid())
        sch.setAttribute(QStringLiteral("start-work"), workStartTime.toString(Qt::ISODate));
    if (workEndTime.isValid())
        sch.setAttribute(QStringLiteral("end-work"), workEndTime.toString(Qt::ISODate));

    sch.setAttribute(QStringLiteral("duration"), duration.toString());

    sch.setAttribute(QStringLiteral("in-critical-path"), QString::number(inCriticalPath));
    sch.setAttribute(QStringLiteral("resource-error"), QString::number(resourceError));
    sch.setAttribute(QStringLiteral("resource-overbooked"), QString::number(resourceOverbooked));
    sch.setAttribute(QStringLiteral("resource-not-available"), QString::number(resourceNotAvailable));
    sch.setAttribute(QStringLiteral("scheduling-conflict"), QString::number(constraintError));
    sch.setAttribute(QStringLiteral("scheduling-error"), QString::number(schedulingError));
    sch.setAttribute(QStringLiteral("not-scheduled"), QString::number(notScheduled));

    sch.setAttribute(QStringLiteral("positive-float"), positiveFloat.toString());
    sch.setAttribute(QStringLiteral("negative-float"), negativeFloat.toString());
    sch.setAttribute(QStringLiteral("free-float"), freeFloat.toString());
}

void NodeSchedule::addAppointment(Schedule *resource, const DateTime &start, const DateTime &end, double load)
{
    //debugPlan;
    Appointment * a = findAppointment(resource, this, m_calculationMode);
    if (a != nullptr) {
        //debugPlan<<"Add interval to existing"<<a;
        a->addInterval(start, end, load);
        return ;
    }
    a = new Appointment(resource, this, start, end, load);
    bool result = add(a);
    Q_ASSERT (result);
    result = resource->add(a);
    Q_ASSERT (result);
    Q_UNUSED (result); // cheating the compiler in release mode to not warn about unused-but-set-variable
    //debugPlan<<"Added interval to new"<<a;
}

void NodeSchedule::takeAppointment(Appointment *appointment, int mode)
{
    Schedule::takeAppointment(appointment, mode);
    appointment->setNode(nullptr); // not my appointment anymore
    //debugPlan<<"Taken:"<<appointment;
    if (appointment->resource())
        appointment->resource() ->takeAppointment(appointment);
}

QList<Resource*> NodeSchedule::resources() const
{
    QList<Resource*> rl;
    for (const Appointment *a : std::as_const(m_appointments)) {
        rl += a->resource() ->resource();
    }
    return rl;
}

QStringList NodeSchedule::resourceNameList() const
{
    QStringList rl;
    for (const Appointment *a : std::as_const(m_appointments)) {
        rl += a->resource() ->resource() ->name();
    }
    return rl;
}

void NodeSchedule::logError(const QString &msg, int phase)
{
    Schedule::Log log(m_node, Log::Type_Error, msg, phase);
    if (m_parent) {
        m_parent->addLog(log);
    } else {
        addLog(log);
    }
}

void NodeSchedule::logWarning(const QString &msg, int phase)
{
    Schedule::Log log(m_node, Log::Type_Warning, msg, phase);
    if (m_parent) {
        m_parent->addLog(log);
    } else {
        addLog(log);
    }
}

void NodeSchedule::logInfo(const QString &msg, int phase)
{
    Schedule::Log log(m_node, Log::Type_Info, msg, phase);
    if (m_parent) {
        m_parent->addLog(log);
    } else {
        addLog(log);
    }
}

void NodeSchedule::logDebug(const QString &msg, int phase)
{
    Schedule::Log log(m_node, Log::Type_Debug, msg, phase);
    if (m_parent) {
        m_parent->addLog(log);
    } else {
        addLog(log);
    }
}

//-----------------------------------------------
ResourceSchedule::ResourceSchedule()
        : Schedule(),
        m_resource(nullptr)
{
    //debugPlan<<"("<<this<<")";
}

ResourceSchedule::ResourceSchedule(Resource *resource, const QString& name, Schedule::Type type, long id)
        : Schedule(name, type, id),
        m_resource(resource),
        m_parent(nullptr),
        m_nodeSchedule(nullptr)
{
    //debugPlan<<"resource:"<<resource->name();
}

ResourceSchedule::ResourceSchedule(Schedule *parent, Resource *resource)
        : Schedule(parent),
        m_resource(resource),
        m_parent(parent),
        m_nodeSchedule(nullptr)
{
    //debugPlan<<"resource:"<<resource->name();
}

ResourceSchedule::~ResourceSchedule()
{
    //debugPlan<<this<<""<<m_appointments.count();
    while (!m_appointments.isEmpty()) {
        Appointment *a = m_appointments.takeFirst();
        a->setResource(nullptr);
        delete a;
    }
    //debugPlan<<"forw"<<m_forward.count();
    while (!m_forward.isEmpty()) {
        Appointment *a = m_forward.takeFirst();
        a->setResource(nullptr);
        delete a;
    }
    //debugPlan<<"backw"<<m_backward.count();
    while (!m_backward.isEmpty()) {
        Appointment *a = m_backward.takeFirst();
        a->setResource(nullptr);
        delete a;
    }
}

// called from the resource
void ResourceSchedule::addAppointment(Schedule *node, const DateTime &start, const DateTime &end, double load)
{
    Q_ASSERT(start < end);
    //debugPlan<<"("<<this<<")"<<node<<","<<m_calculationMode;
    Appointment * a = findAppointment(this, node, m_calculationMode);
    if (a != nullptr) {
        //debugPlan<<"Add interval to existing"<<a;
        a->addInterval(start, end, load);
        return ;
    }
    a = new Appointment(this, node, start, end, load);
    bool result = add(a);
    Q_ASSERT (result == true);
    result = node->add(a);
    Q_ASSERT (result == true);
    Q_UNUSED (result);  //don't warn about unused-but-set-variable in release mode
    //debugPlan<<"Added interval to new"<<a;
}

void ResourceSchedule::takeAppointment(Appointment *appointment, int mode)
{
    Schedule::takeAppointment(appointment, mode);
    appointment->setResource(nullptr);
    //debugPlan<<"Taken:"<<appointment;
    if (appointment->node())
        appointment->node() ->takeAppointment(appointment);
}

bool ResourceSchedule::isOverbooked() const
{
    return false;
}

bool ResourceSchedule::isOverbooked(const DateTime &start, const DateTime &end) const
{
    if (m_resource == nullptr)
        return false;
    //debugPlan<<start.toString()<<" -"<<end.toString();
    Appointment a = appointmentIntervals();
    const auto intervals = a.intervals().map().values();
    for (const AppointmentInterval &i : intervals) {
        if ((!end.isValid() || i.startTime() < end) &&
                (!start.isValid() || i.endTime() > start)) {
            if (i.load() > m_resource->units()) {
                //debugPlan<<m_name<<" overbooked";
                return true;
            }
        }
        if (i.startTime() >= end)
            break;
    }
    //debugPlan<<m_name<<" not overbooked";
    return false;
}

double ResourceSchedule::normalRatePrHour() const
{
    return m_resource ? m_resource->normalRate() : 0.0;
}

//TODO change to return the booked effort
Duration ResourceSchedule::effort(const DateTimeInterval &interval) const
{
    Duration eff = interval.second - interval.first;
    if (allowOverbooking()) {
        return eff;
    }
    Appointment a;
    if (checkExternalAppointments()) {
        a.setIntervals(m_resource->externalAppointments());
    }
    a.merge(appointmentIntervals(m_calculationMode));
    if (a.isEmpty() || a.startTime() >= interval.second || a.endTime() <= interval.first) {
        return eff;
    }
    const auto intervals = a.intervals().map().values();
    for (const AppointmentInterval &i : intervals) {
        if (interval.second <= i.startTime()) {
            break;
        }
        if (interval.first >= i.startTime()) {
            DateTime et = i.endTime() < interval.second ? i.endTime() : interval.second;
            eff -= (et - interval.first) * ((double)i.load()/100.0);
        } else {
            DateTime et = i.endTime() < interval.second ? i.endTime() : interval.second;
            eff -= (et - i.startTime()) * ((double)i.load()/100.0);
        }
    }
    return eff;
}

DateTimeInterval ResourceSchedule::available(const DateTimeInterval &interval) const
{
    //const_cast<ResourceSchedule*>(this)->logDebug(QString("Schedule available id=%1, Mode=%2: interval=%3 - %4").arg(m_id).arg(m_calculationMode).arg(interval.first.toString()).arg(interval.second.toString()));
    if (allowOverbooking()) {
        return DateTimeInterval(interval.first, interval.second);
    }
    QTimeZone projectTimeZone = QTimeZone::systemTimeZone();
    if (m_resource) {
        projectTimeZone = m_resource->project()->timeZone();
    }
    DateTimeInterval ci(interval.first.toTimeZone(projectTimeZone), interval.second.toTimeZone(projectTimeZone));
    Appointment a;
    if (checkExternalAppointments() && m_resource) {
        a.setIntervals(m_resource->externalAppointments(ci));
    }
    a.merge(appointmentIntervals(m_calculationMode, ci));
    if (a.isEmpty() || a.startTime() >= ci.second || a.endTime() <= ci.first) {
        //debugPlan<<this<<"id="<<m_id<<"Mode="<<m_calculationMode<<""<<interval.first<<","<<interval.second<<" FREE";
        return DateTimeInterval(interval.first, interval.second); // just return the interval
    }
    //debugPlan<<"available:"<<interval<<'\n'<<a.intervals();
    DateTimeInterval res;
    int units = m_resource ? m_resource->units() : 100;
    const auto intervals = a.intervals().map().values();
    for (const AppointmentInterval &i : intervals) {
        //const_cast<ResourceSchedule*>(this)->logDebug(QString("Schedule available check interval=%1 - %2").arg(i.startTime().toString()).arg(i.endTime().toString()));
        if (i.startTime() < ci.second && i.endTime() > ci.first) {
            // interval intersects appointment
            if (ci.first >= i.startTime() && ci.second <= i.endTime()) {
                // interval within appointment
                if (i.load() < units) {
                    if (! res.first.isValid()) {
                        res.first = qMax(ci.first, i.startTime());
                    }
                    res.second = qMin(ci.second, i.endTime());
                } else {
                    // fully booked
                    if (res.first.isValid()) {
                        res.second = i.startTime();
                        if (res.first >= res.second) {
                            res = DateTimeInterval();
                        }
                    }
                }
                //debugPlan<<"available within:"<<interval<<i<<":"<<ci<<res;
                break;
            }
            DateTime t = i.startTime();
            if (ci.first < t) {
                // Interval starts before appointment, so free from interval start
                //debugPlan<<"available before:"<<interval<<i<<":"<<ci<<res;
                //const_cast<ResourceSchedule*>(this)->logDebug(QString("Schedule available t>first: returns interval=%1 - %2").arg(ci.first.toString()).arg(t.toString()));
                if (! res.first.isValid()) {
                    res.first = ci.first;
                }
                res.second = t;
                if (i.load() < units) {
                    res.second = qMin(ci.second, i.endTime());
                    if (ci.second > i.endTime()) {
                        ci.first = i.endTime();
                        //debugPlan<<"available next 1:"<<interval<<i<<":"<<ci<<res;
                        continue; // check next appointment
                    }
                }
                //debugPlan<<"available:"<<interval<<i<<":"<<ci<<res;
                break;
            }
            // interval start >= appointment start
            t = i.endTime();
            if (t < ci.second) {
                // check if rest of appointment is free
                if (units <= i.load()) {
                    ci.first = t; // fully booked, so move forward to appointment end
                }
                res = ci;
                //debugPlan<<"available next 2:"<<interval<<i<<":"<<ci<<res;
                continue;
            }
            //debugPlan<<"available:"<<interval<<i<<":"<<ci<<res;
            Q_ASSERT(false);
        } else if (i.startTime() >= interval.second) {
            // no more overlaps
            break;
        }
    }
    //debugPlan<<"available: result="<<interval<<":"<<res;
    return DateTimeInterval(res.first.toTimeZone(interval.first.timeZone()), res.second.toTimeZone(interval.second.timeZone()));
}

void ResourceSchedule::logError(const QString &msg, int phase)
{
    if (m_parent) {
        Schedule::Log log(m_nodeSchedule ? m_nodeSchedule->node() : nullptr, m_resource, Log::Type_Error, msg, phase);
        m_parent->addLog(log);
    }
}

void ResourceSchedule::logWarning(const QString &msg, int phase)
{
    if (m_parent) {
        Schedule::Log log(m_nodeSchedule ? m_nodeSchedule->node() : nullptr, m_resource, Log::Type_Warning, msg, phase);
        m_parent->addLog(log);
    }
}

void ResourceSchedule::logInfo(const QString &msg, int phase)
{
    if (m_parent) {
        Schedule::Log log(m_nodeSchedule ? m_nodeSchedule->node() : nullptr, m_resource, Log::Type_Info, msg, phase);
        m_parent->addLog(log);
    }
}

void ResourceSchedule::logDebug(const QString &msg, int phase)
{
    if (m_parent) {
        Schedule::Log log(m_nodeSchedule ? m_nodeSchedule->node() : nullptr, m_resource, Log::Type_Debug, msg, phase);
        m_parent->addLog(log);
    }
}

//--------------------------------------
MainSchedule::MainSchedule()
    : NodeSchedule(),

    m_manager(nullptr)
{
    //debugPlan<<"("<<this<<")";
    init();
}

MainSchedule::MainSchedule(Node *node, const QString& name, Schedule::Type type, long id)
    : NodeSchedule(node, name, type, id),
      criticalPathListCached(false),
      m_manager(nullptr),
      m_currentCriticalPath(nullptr)
{
    //debugPlan<<"node name:"<<node->name();
    init();
}

MainSchedule::~MainSchedule()
{
    //debugPlan<<"("<<this<<")";
}

void MainSchedule::incProgress()
{
    if (m_manager) m_manager->incProgress();
}

bool MainSchedule::isBaselined() const
{
    return m_manager == nullptr ? false : m_manager->isBaselined();
}

bool MainSchedule::usePert() const
{
    return m_manager == nullptr ? false : m_manager->usePert();
}

bool MainSchedule::allowOverbooking() const
{
    return m_manager == nullptr ? false : m_manager->allowOverbooking();
}

bool MainSchedule::checkExternalAppointments() const
{
    return m_manager == nullptr ? false : m_manager->checkExternalAppointments();
}

void MainSchedule::changed(Schedule *sch)
{
    if (m_manager) {
        m_manager->scheduleChanged(static_cast<MainSchedule*>(sch));
    }
}

void MainSchedule::insertHardConstraint(Node *node)
{
    m_hardconstraints.append(node);
}

QList<Node*> MainSchedule::hardConstraints() const
{
    return m_hardconstraints;
}

void MainSchedule::insertSoftConstraint(Node *node)
{
    m_softconstraints.insert(-node->priority(), node);
}

QList<Node*> MainSchedule::softConstraints() const
{
    return m_softconstraints.values();
}

QList<Node*> MainSchedule::forwardNodes() const
{
    return m_forwardnodes;
}

void MainSchedule::insertForwardNode(Node *node)
{
    m_forwardnodes.append(node);
}

QList<Node*> MainSchedule::backwardNodes() const
{
    return m_backwardnodes;
}

void MainSchedule::insertBackwardNode(Node *node)
{
    m_backwardnodes.append(node);
}

void MainSchedule::insertStartNode(Node *node)
{
    m_startNodes.insert(-node->priority(), node);
}

QList<Node*> MainSchedule::startNodes() const
{
    return m_startNodes.values();
}

void MainSchedule::insertEndNode(Node *node)
{
    m_endNodes.insert(-node->priority(), node);
}

QList<Node*> MainSchedule::endNodes() const
{
    return m_endNodes.values();
}

void MainSchedule::insertSummaryTask(Node *node)
{
    m_summarytasks.append(node);
}

QList<Node*> MainSchedule::summaryTasks() const
{
    return m_summarytasks;
}

void MainSchedule::saveXML(QDomElement &element) const
{
    saveCommonXML(element);

    element.setAttribute(QStringLiteral("start"), startTime.toString(Qt::ISODate));
    element.setAttribute(QStringLiteral("end"), endTime.toString(Qt::ISODate));
    element.setAttribute(QStringLiteral("duration"), duration.toString());
    element.setAttribute(QStringLiteral("scheduling-conflict"), QString::number(constraintError));
    element.setAttribute(QStringLiteral("scheduling-error"), QString::number(schedulingError));
    element.setAttribute(QStringLiteral("not-scheduled"), QString::number(notScheduled));

    if (! m_pathlists.isEmpty()) {
        QDomElement lists = element.ownerDocument().createElement(QStringLiteral("criticalpath-list"));
        element.appendChild(lists);
        for (const QList<Node*> &l : std::as_const(m_pathlists)) {
            if (l.isEmpty()) {
                continue;
            }
            QDomElement list = lists.ownerDocument().createElement(QStringLiteral("criticalpath"));
            lists.appendChild(list);
            for (Node *n : std::as_const(l)) {
                QDomElement el = list.ownerDocument().createElement(QStringLiteral("node"));
                list.appendChild(el);
                el.setAttribute(QStringLiteral("id"), n->id());
            }
        }
    }
}

DateTime MainSchedule::calculateForward(int use)
{
    DateTime late;
    for (Node *n : std::as_const(m_backwardnodes)) {
        DateTime t = n->calculateForward(use);
        if (!late.isValid() || late < t) {
            late = t;
        }
    }
    return late;
}

DateTime MainSchedule::calculateBackward(int use)
{
    DateTime early;
    for (Node *n : std::as_const(m_forwardnodes)) {
        DateTime t = n->calculateBackward(use);
        if (!early.isValid() || early > t) {
            early = t;
        }
    }
    return early;
}

DateTime MainSchedule::scheduleForward(const DateTime &earliest, int use)
{
    DateTime end;
    for (Node *n : std::as_const(m_forwardnodes)) {
        DateTime t = n->scheduleForward(earliest, use);
        if (!end.isValid() || end < t) {
            end = t;
        }
    }
    return end;
}

DateTime MainSchedule::scheduleBackward(const DateTime &latest, int use)
{
    DateTime start;
    for (Node *n : std::as_const(m_backwardnodes)) {
        DateTime t = n->scheduleBackward(latest, use);
        if (!start.isValid() || start > t) {
            start = t;
        }
    }
    return start;
}

bool MainSchedule::recalculate() const
{
    return m_manager == nullptr ? false : m_manager->recalculate();
}

DateTime MainSchedule::recalculateFrom() const
{
    return m_manager == nullptr ? DateTime() : m_manager->recalculateFrom();
}

long MainSchedule::parentScheduleId() const
{
    return m_manager == nullptr ? -2 : m_manager->parentScheduleId();
}

void MainSchedule::clearCriticalPathList()
{
    m_pathlists.clear();
    m_currentCriticalPath = nullptr;
    criticalPathListCached = false;
}

QList<Node*> *MainSchedule::currentCriticalPath() const
{
    return m_currentCriticalPath;
}

void MainSchedule::addCriticalPath(QList<Node*> *lst)
{
    QList<Node*> l;
    if (lst) {
        l = *lst;
    }
    m_pathlists.append(l);
    m_currentCriticalPath = &(m_pathlists.last());
}

void MainSchedule::addCriticalPathNode(Node *node)
{
    if (m_currentCriticalPath == nullptr) {
        errorPlan<<"No currentCriticalPath"<<'\n';
        return;
    }
    m_currentCriticalPath->append(node);
}

QVector<Schedule::Log> MainSchedule::logs() const
{
    return m_log;
}

void MainSchedule::addLog(const KPlato::Schedule::Log &log)
{
    Q_ASSERT(log.resource || log.node);
#ifndef NDEBUG
    if (log.resource) {
        Q_ASSERT(manager()->project().findResource(log.resource->id()) == log.resource);
    } else if (log.node) {
        Q_ASSERT(manager()->project().findNode(log.node->id()) == log.node);
    }
#endif
    const int phaseToSet = (log.phase == -1 && ! m_log.isEmpty()) ? m_log.last().phase : -1;
    m_log.append(log);
    if (phaseToSet != -1) {
        m_log.last().phase = phaseToSet;
    }
    if (m_manager) {
        m_manager->logAdded(m_log.last());
    }
}

//static
QString MainSchedule::logSeverity(int severity)
{
    switch (severity) {
        case Log::Type_Debug: return i18n("Debug");
        case Log::Type_Info: return i18n("Info");
        case Log::Type_Warning: return i18n("Warning");
        case Log::Type_Error: return i18n("Error");
        default: break;
    }
    return QStringLiteral("Severity %1").arg(severity);
}

QStringList MainSchedule::logMessages() const
{
    QStringList lst;
    for (const Schedule::Log &l : std::as_const(m_log)) {
        lst << l.formatMsg();
    }
    return lst;
}

//-----------------------------------------
ScheduleManager::ScheduleManager(Project &project, const QString name, const Owner &creator)
    : m_project(project),
    m_parent(nullptr),
    m_name(name),
    m_baselined(false),
    m_allowOverbooking(false),
    m_checkExternalAppointments(true),
    m_usePert(false),
    m_recalculate(false),
    m_schedulingDirection(false),
    m_scheduling(false),
    m_progress(0),
    m_maxprogress(0),
    m_expected(nullptr),
    m_calculationresult(0),
    m_schedulingMode(false),
    m_owner(creator)
{
    //debugPlan<<name;
}

ScheduleManager::~ScheduleManager()
{
    qDeleteAll(m_children);
    setParentManager(nullptr);
}

void ScheduleManager::setParentManager(ScheduleManager *sm, int index)
{
    if (m_parent) {
        m_parent->removeChild(this);
    }
    m_parent = sm;
    if (sm) {
        sm->insertChild(this, index);
    }
}

int ScheduleManager::removeChild(const ScheduleManager *sm)
{
    int i = m_children.indexOf(const_cast<ScheduleManager*>(sm));
    if (i != -1) {
        m_children.removeAt(i);
    }
    return i;
}

void ScheduleManager::insertChild(ScheduleManager *sm, int index)
{
    //debugPlan<<m_name<<", insert"<<sm->name()<<","<<index;
    if (index == -1) {
        m_children.append(sm);
    } else {
        m_children.insert(index, sm);
    }
}

void ScheduleManager::createSchedules()
{
    setExpected(m_project.createSchedule(m_name, Schedule::Expected, m_owner == OwnerPlan ? 1 : 100));
}

int ScheduleManager::indexOf(const ScheduleManager *child) const
{
    //debugPlan<<this<<","<<child;
    return m_children.indexOf(const_cast<ScheduleManager*>(child));
}

ScheduleManager *ScheduleManager::findManager(const QString& name) const
{
    if (m_name == name) {
        return const_cast<ScheduleManager*>(this);
    }
    for (ScheduleManager *sm : std::as_const(m_children)) {
        ScheduleManager *m = sm->findManager(name);
        if (m) {
            return m;
        }
    }
    return nullptr;
}

QList<ScheduleManager*> ScheduleManager::allChildren() const
{
    QList<ScheduleManager*> lst;
    for (ScheduleManager *sm : std::as_const(m_children)) {
        lst << sm;
        lst << sm->allChildren();
    }
    return lst;
}

bool ScheduleManager::isParentOf(const ScheduleManager *sm) const
{
    if (indexOf(sm) >= 0) {
        return true;
    }
    for (ScheduleManager *p : std::as_const(m_children)) {
        if (p->isParentOf(sm)) {
            return true;
        }
    }
    return false;
}

void ScheduleManager::setName(const QString& name)
{
    m_name = name;
#ifndef NDEBUG
    setObjectName(name);
#endif
    if (m_expected) {
        m_expected->setName(name);
        m_project.changed(m_expected);
    }
    m_project.changed(this);
}

bool ScheduleManager::isChildBaselined() const
{
    //debugPlan<<on;
    for (ScheduleManager *sm : std::as_const(m_children)) {
        if (sm->isBaselined() || sm->isChildBaselined()) {
            return true;
        }
    }
    return false;
}

void ScheduleManager::setBaselined(bool on)
{
    //debugPlan<<on;
    m_baselined = on;
    m_project.changed(this);
}

void ScheduleManager::setAllowOverbooking(bool on)
{
    //debugPlan<<on;
    m_allowOverbooking = on;
    m_project.changed(this, OverbookProperty);
}

bool ScheduleManager::allowOverbooking() const
{
    //debugPlan<<m_name<<"="<<m_allowOverbooking;
    return m_allowOverbooking;
}

bool ScheduleManager::checkExternalAppointments() const
{
    //debugPlan<<m_name<<"="<<m_allowOverbooking;
    return m_checkExternalAppointments;
}

void ScheduleManager::setCheckExternalAppointments(bool on)
{
    //debugPlan<<m_name<<"="<<m_checkExternalAppointments;
    m_checkExternalAppointments = on;
}

void ScheduleManager::scheduleChanged(MainSchedule *sch)
{
    m_project.changed(sch);
    m_project.changed(this); //hmmm, due to aggregated info
}

void ScheduleManager::setUsePert(bool on)
{
    m_usePert = on;
    m_project.changed(this, DistributionProperty);
}

void ScheduleManager::setSchedulingDirection(bool on)
{
    //debugPlan<<on;
    m_schedulingDirection = on;
    m_project.changed(this, DirectionProperty);
}

void ScheduleManager::setScheduling(bool on)
{
    m_scheduling = on;
    if (! on) {
        m_project.setProgress(0, this);
    }
    m_project.changed(this);
}

const QList<SchedulerPlugin*> ScheduleManager::schedulerPlugins() const
{
    return m_project.schedulerPlugins().values();
}

QString ScheduleManager::schedulerPluginId() const
{
    return m_schedulerPluginId;
}

void ScheduleManager::setSchedulerPluginId(const QString &id)
{
    m_schedulerPluginId = id;
    m_project.changed(this);
}

SchedulerPlugin *ScheduleManager::schedulerPlugin() const
{
    if (m_schedulerPluginId.isEmpty() || !m_project.schedulerPlugins().contains(m_schedulerPluginId)) {
        // try to avoid crash
        return m_project.schedulerPlugins().value(m_project.schedulerPlugins().keys().value(0));
    }
    return m_project.schedulerPlugins().value(m_schedulerPluginId);
}

QStringList ScheduleManager::schedulerPluginNames() const
{
    QStringList lst;
    QMap<QString, SchedulerPlugin*>::const_iterator it = m_project.schedulerPlugins().constBegin();
    QMap<QString, SchedulerPlugin*>::const_iterator end = m_project.schedulerPlugins().constEnd();
    for (; it != end; ++it) {
        lst << it.value()->name();
    }
    return lst;
}

int ScheduleManager::schedulerPluginIndex() const
{
    if (m_schedulerPluginId.isEmpty()) {
        return 0;
    }
    return m_project.schedulerPlugins().keys().indexOf(m_schedulerPluginId);
}

void ScheduleManager::setSchedulerPlugin(int index)
{
    if (schedulerPlugin()) {
        schedulerPlugin()->stopCalculation(this); // in case...
    }

    m_schedulerPluginId = m_project.schedulerPlugins().keys().value(index);
    debugPlan<<index<<m_schedulerPluginId;
    m_project.changed(this);
}

void ScheduleManager::calculateSchedule()
{
    m_calculationresult = CalculationRunning;
    if (schedulerPlugin()) {
        schedulerPlugin()->calculate(m_project, this);
    }
}

void ScheduleManager::stopCalculation()
{
    if (schedulerPlugin()) {
        schedulerPlugin()->stopCalculation(this);
    }
}

void ScheduleManager::haltCalculation()
{
    if (schedulerPlugin()) {
        schedulerPlugin()->haltCalculation(this);
    }
}

void ScheduleManager::setMaxProgress(int value)
{
    m_maxprogress = value;
    Q_EMIT maxProgressChanged(value);
    m_project.changed(this);
}

void ScheduleManager::setProgress(int value)
{
    m_progress = value;
    Q_EMIT progressChanged(value);
    m_project.changed(this);
}

void ScheduleManager::setDeleted(bool on)
{
    if (m_expected) {
        m_expected->setDeleted(on);
    }
    m_project.changed(this);
}

void ScheduleManager::setExpected(MainSchedule *sch)
{
    //debugPlan<<m_expected<<","<<sch;
    if (m_expected) {
        m_project.sendScheduleToBeRemoved(m_expected);
        m_expected->setDeleted(true);
        m_project.sendScheduleRemoved(m_expected);
    }
    m_expected = sch;
    if (sch) {
        m_project.sendScheduleToBeAdded(this, 0);
        sch->setManager(this);
        m_expected->setDeleted(false);
        m_project.sendScheduleAdded(sch);
    }
    m_project.changed(this);
}

QStringList ScheduleManager::state() const
{
    QStringList lst;
    if (m_scheduling) {
        return lst << i18n("Scheduling");
    }
    if (m_owner == OwnerPortfolio) {
        lst << i18n("Portfolio");
    }
    if (m_expected == nullptr) {
        return lst << i18n("Not scheduled");
    }
    if (isBaselined()) {
        lst << i18n("Baselined");
    }
    if (Schedule *s = m_expected) {
        if (s->resourceError || s->resourceOverbooked || s->resourceNotAvailable || s->constraintError || s->schedulingError) {
            return lst << i18n("Error");
        }
        if (!isBaselined()) {
            lst << s->state();
        }
    }
    return lst;
}

QList<long unsigned int> ScheduleManager::supportedGranularities() const
{
    QList<long unsigned int> lst;
    if (schedulerPlugin()) {
        lst = schedulerPlugin()->granularities();
    }
    return lst;
}

int ScheduleManager::granularityIndex() const
{
    if (schedulerPlugin()) {
        return schedulerPlugin()->granularityIndex();
    }
    return 0;
}

void ScheduleManager::setGranularityIndex(int duration)
{
    if (schedulerPlugin()) {
        schedulerPlugin()->setGranularityIndex(duration);
    }
    m_project.changed(this, GranularityIndexProperty);
}

ulong ScheduleManager::granularity() const
{
    return schedulerPlugin() ? schedulerPlugin()->granularity() : 0;
}

bool ScheduleManager::schedulingMode() const
{
    return m_schedulingMode;
}

void ScheduleManager::setSchedulingMode(int mode)
{
    m_schedulingMode = mode;
    m_project.changed(this, SchedulingModeProperty);
}

DateTime ScheduleManager::scheduledStartTime() const
{
    return m_expected ? m_expected->startTime : DateTime();
}

DateTime ScheduleManager::scheduledEndTime() const
{
    return m_expected ? m_expected->endTime : DateTime();
}

void ScheduleManager::incProgress()
{
    m_project.incProgress();
}

void ScheduleManager::logAdded(const Schedule::Log &log)
{
    Q_EMIT sigLogAdded(log);
    int row = expected()->logs().count() - 1;
    Q_EMIT logInserted(expected(), row, row);
}

void ScheduleManager::slotAddLog(const QVector<KPlato::Schedule::Log> &log)
{
    if (expected() && ! log.isEmpty()) {
        int first = expected()->logs().count();
        int last = first + log.count() - 1;
        Q_UNUSED(last);
        for (const KPlato::Schedule::Log &l : log) {
            expected()->addLog(l);
        }
    }
}

QMap< int, QString > ScheduleManager::phaseNames() const
{
    if (expected()) {
        return expected()->phaseNames();
    }
    return QMap<int, QString>();
}

void ScheduleManager::setPhaseNames(const QMap<int, QString> &phasenames)
{
    if (expected()) {
        expected()->setPhaseNames(phasenames);
    }
}

void ScheduleManager::saveXML(QDomElement &element) const
{
    QDomElement el = element.ownerDocument().createElement(QStringLiteral("schedule-management"));
    element.appendChild(el);
    el.setAttribute(QStringLiteral("name"), m_name);
    el.setAttribute(QStringLiteral("id"), m_id);
    el.setAttribute(QStringLiteral("distribution"), QString::number(m_usePert ? 1 : 0));
    el.setAttribute(QStringLiteral("overbooking"), QString::number(m_allowOverbooking));
    el.setAttribute(QStringLiteral("check-external-appointments"), QString::number(m_checkExternalAppointments));
    el.setAttribute(QStringLiteral("scheduling-direction"), QString::number(m_schedulingDirection));
    el.setAttribute(QStringLiteral("baselined"), QString::number(m_baselined));
    el.setAttribute(QStringLiteral("scheduler-plugin-id"), m_schedulerPluginId);
    el.setAttribute(QStringLiteral("granularity"), QString::number(granularityIndex()));
    el.setAttribute(QStringLiteral("recalculate"), QString::number(m_recalculate));
    el.setAttribute(QStringLiteral("recalculate-from"), m_recalculateFrom.toString(Qt::ISODate));
    el.setAttribute(QStringLiteral("scheduling-mode"), m_schedulingMode);
    el.setAttribute(QStringLiteral("origin"), m_owner);
    if (m_expected && ! m_expected->isDeleted()) {
        QDomElement schs = el.ownerDocument().createElement(QStringLiteral("project-schedule"));
        el.appendChild(schs);
        m_expected->saveXML(schs);
        m_project.saveAppointments(schs, m_expected->id());
    }
    for (ScheduleManager *sm : std::as_const(m_children)) {
        sm->saveXML(el);
    }

}

void ScheduleManager::saveWorkPackageXML(QDomElement &element, const Node &node) const
{
    QDomElement el = element.ownerDocument().createElement(QStringLiteral("schedule-management"));
    element.appendChild(el);
    el.setAttribute(QStringLiteral("name"), m_name);
    el.setAttribute(QStringLiteral("id"), m_id);
    el.setAttribute(QStringLiteral("distribution"), QString::number(m_usePert ? 1 : 0));
    el.setAttribute(QStringLiteral("overbooking"), QString::number(m_allowOverbooking));
    el.setAttribute(QStringLiteral("check-external-appointments"), QString::number(m_checkExternalAppointments));
    el.setAttribute(QStringLiteral("scheduling-direction"), QString::number(m_schedulingDirection));
    el.setAttribute(QStringLiteral("baselined"), QString::number(m_baselined));
    el.setAttribute(QStringLiteral("origin"), m_owner);
    if (m_expected && ! m_expected->isDeleted()) { // TODO: should we check isScheduled() ?
        QDomElement schs = el.ownerDocument().createElement(QStringLiteral("schedule"));
        el.appendChild(schs);
        m_expected->saveXML(schs);
        Schedule *s = node.findSchedule(m_expected->id());
        if (s && ! s->isDeleted()) {
            s->saveAppointments(schs);
        }
    }
}

ScheduleManager::Owner ScheduleManager::owner() const
{
    return m_owner;
}

void ScheduleManager::setOwner(const ScheduleManager::Owner origin)
{
    m_owner = origin;
    m_project.changed(this);
}

} //namespace KPlato

QDebug operator<<(QDebug dbg, const KPlato::Schedule *s)
{
    if (s) {
        return dbg<<(*s);
    }
    return dbg<<"Schedule(0x0)";
}
QDebug operator<<(QDebug dbg, const KPlato::Schedule &s)
{
    dbg.nospace()<<"Schedule["<<(void*)&s<<' '<<s.id();
    if (s.isDeleted()) {
        dbg.nospace()<<": Deleted";
    } else {
        dbg.nospace()<<": "<<s.name();
    }
    dbg.nospace()<<"]";
    return dbg.space();
}

QDebug operator<<(QDebug dbg, const KPlato::Schedule::Log &log)
{
    dbg.nospace()<<"Schedule::Log: "<<log.formatMsg();
    return dbg.space();
}

QDebug operator<<(QDebug dbg, const KPlato::ScheduleManager *s)
{
    if (s) {
        return dbg<<(*s);
    }
    return dbg<<"ScheduleManager(0x0)";
}
QDebug operator<<(QDebug dbg, const KPlato::ScheduleManager &s)
{
    dbg.nospace()<<"ScheduleManager["<<(void*)&s<<' '<<s.name();
    if (s.isBaselined()) {
        dbg.nospace()<<" B";
    } else if (s.isScheduled()) {
        dbg.nospace()<<" S";
    }
    if (s.recalculate()) {
        dbg.nospace()<<" R: "<<s.recalculateFrom().toString(Qt::ISODate);
    }
    dbg.nospace()<<"]";
    return dbg.space();
}
