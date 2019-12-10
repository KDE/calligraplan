/* This file is part of the KDE project
 * Copyright (C) 2001 Thomas zander <zander@kde.org>
 * Copyright (C) 2004-2007, 2012 Dag Andersen <danders@get2net.dk>
 * Copyright (C) 2016 Dag Andersen <danders@get2net.dk>
 * Copyright (C) 2019 Dag Andersen <danders@get2net.dk>
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

// clazy:excludeall=qstring-arg
#include "Resource.h"
#include "ResourceGroup.h"
#include "kptresourcerequest.h"

#include "kptlocale.h"
#include "kptaccount.h"
#include "kptappointment.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptdatetime.h"
#include "kptcalendar.h"
#include "kpteffortcostmap.h"
#include "kptschedule.h"
#include "kptxmlloaderobject.h"
#include "kptdebug.h"

#include <KoXmlReader.h>

#include <KLocalizedString>

#include <QLocale>


using namespace KPlato;

Resource::Resource()
    : QObject(0), // atm QObject is only for casting
    m_project(0),
    m_autoAllocate(false),
    m_currentSchedule(0),
    m_blockChanged(false),
    m_shared(false)
{
    m_type = Type_Work;
    m_units = 100; // %

//     m_availableFrom = DateTime(QDate::currentDate(), QTime(0, 0, 0));
//     m_availableUntil = m_availableFrom.addYears(2);

    cost.normalRate = 100;
    cost.overtimeRate = 0;
    cost.fixed = 0;
    cost.account = 0;
    m_calendar = 0;
    m_currentSchedule = 0;
    //debugPlan<<"("<<this<<")";
    
    // material: by default material is always available
    for (int i = 1; i <= 7; ++i) {
        CalendarDay *wd = m_materialCalendar.weekday(i);
        wd->setState(CalendarDay::Working);
        wd->addInterval(TimeInterval(QTime(0, 0, 0), 24*60*60*1000));
    }
}

Resource::Resource(Resource *resource)
    : QObject(0), // atm QObject is only for casting
    m_project(0),
    m_currentSchedule(0),
    m_shared(false)
{
    //debugPlan<<"("<<this<<") from ("<<resource<<")";
    copy(resource); 
}

Resource::~Resource() {
    //debugPlan<<"("<<this<<")";
    if (findId() == this) {
        removeId(); // only remove myself (I may be just a working copy)
    }
    removeRequests();
    foreach (Schedule *s, m_schedules) {
        delete s;
    }
    clearExternalAppointments();
    if (cost.account) {
        cost.account->removeRunning(*this);
    }
    for (ResourceGroup *g : m_parents) {
        g->takeResource(this);
    }
}

void Resource::removeRequests() {
    foreach (ResourceRequest *r, m_requests) {
        r->setResource(0); // avoid the request to mess with my list
        delete r;
    }
    m_requests.clear();
}

void Resource::registerRequest(ResourceRequest *request)
{
    Q_ASSERT(!m_requests.contains(request));
    m_requests.append(request);
}

void Resource::unregisterRequest(ResourceRequest *request)
{
    m_requests.removeOne(request);
    Q_ASSERT(!m_requests.contains(request));
}

const QList<ResourceRequest*> &Resource::requests() const
{
    return m_requests;
}

void Resource::setId(const QString& id) {
    //debugPlan<<id;
    m_id = id;
}

void Resource::copy(Resource *resource) {
    m_project = 0; // NOTE: Don't copy, will be set when added to a project
    //m_appointments = resource->appointments(); // Note
    m_id = resource->id();
    m_name = resource->name();
    m_initials = resource->initials();
    m_email = resource->email();
    m_autoAllocate = resource->m_autoAllocate;
    m_availableFrom = resource->availableFrom();
    m_availableUntil = resource->availableUntil();
    
    m_units = resource->units(); // available units in percent

    m_type = resource->type();

    cost.normalRate = resource->normalRate();
    cost.overtimeRate = resource->overtimeRate();
    cost.account = resource->account();
    m_calendar = resource->m_calendar;

    m_requiredIds = resource->requiredIds();
    m_teamMembers = resource->m_teamMembers;

    // No: m_parents = resource->m_parents;

    // hmmmm
    //m_externalAppointments = resource->m_externalAppointments;
    //m_externalNames = resource->m_externalNames;
}

void Resource::blockChanged(bool on)
{
    m_blockChanged = on;
}

void Resource::changed()
{
    if (m_project && !m_blockChanged) {
        m_project->changed(this);
    }
}

void Resource::setType(Type type)
{
    m_type = type;
    changed();
}

void Resource::setType(const QString &type)
{
    if (type == "Work")
        setType(Type_Work);
    else if (type == "Material")
        setType(Type_Material);
    else if (type == "Team")
        setType(Type_Team);
    else
        setType(Type_Work);
}

QString Resource::typeToString(bool trans) const {
    return typeToStringList(trans).at(m_type);
}

QStringList Resource::typeToStringList(bool trans) {
    // keep these in the same order as the enum!
    return QStringList() 
            << (trans ? xi18nc("@item:inlistbox resource type", "Work") : QString("Work"))
            << (trans ? xi18nc("@item:inlistbox resource type", "Material") : QString("Material"))
            << (trans ? xi18nc("@item:inlistbox resource type", "Team") : QString("Team"));
}

void Resource::setName(const QString &n)
{
    m_name = n.trimmed();
    changed();
}

void Resource::setInitials(const QString &initials)
{
    m_initials = initials.trimmed();
    changed();
}

void Resource::setEmail(const QString &email)
{
    m_email = email;
    changed();
}

bool Resource::autoAllocate() const
{
    return m_autoAllocate;
}

void Resource::setAutoAllocate(bool on)
{
    if (m_autoAllocate != on) {
        m_autoAllocate = on;
        changed();
    }
}

void Resource::setUnits(int units)
{
    m_units = units;
    m_workinfocache.clear();
    changed();
}

Calendar *Resource::calendar(bool local) const {
    if (local || m_calendar) {
        return m_calendar;
    }
    // No calendar is set, try default calendar
    Calendar *c = 0;
    if (m_type == Type_Work && project()) {
        c =  project()->defaultCalendar();
    } else if (m_type == Type_Material) {
        c = const_cast<Calendar*>(&m_materialCalendar);
    }
    return c;
}

void Resource::setCalendar(Calendar *calendar)
{
    m_calendar = calendar;
    m_workinfocache.clear();
    changed();
}

void Resource::addParentGroup(ResourceGroup *parent)
{
    if (!parent) {
        return;
    }
    if (!m_parents.contains(parent)) {
        m_parents.append(parent);
        parent->addResource(this);
    }
}

bool Resource::removeParentGroup(ResourceGroup *parent)
{
    if (parent) {
        parent->takeResource(this);
    }
    return m_parents.removeOne(parent);
}

void Resource::setParentGroups(QList<ResourceGroup*> &parents)
{
    for (ResourceGroup *g : m_parents) {
        removeParentGroup(g);
    }
    m_parents = parents;
}

QList<ResourceGroup*> Resource::parentGroups() const
{
    return m_parents;
}

DateTime Resource::firstAvailableAfter(const DateTime &, const DateTime &) const {
    return DateTime();
}

DateTime Resource::getBestAvailableTime(const Duration &/*duration*/) {
    return DateTime();
}

DateTime Resource::getBestAvailableTime(const DateTime &/*after*/, const Duration &/*duration*/) {
    return DateTime();
}

bool Resource::load(KoXmlElement &element, XMLLoaderObject &status) {
    //debugPlan;
    const Locale *locale = status.project().locale();
    QString s;
    setId(element.attribute("id"));
    m_name = element.attribute("name");
    m_initials = element.attribute("initials");
    m_email = element.attribute("email");
    m_autoAllocate = (bool)(element.attribute("auto-allocate", "0").toInt());
    setType(element.attribute("type"));
    m_shared = element.attribute("shared", "0").toInt();
    m_calendar = status.project().findCalendar(element.attribute("calendar-id"));
    m_units = element.attribute("units", "100").toInt();
    s = element.attribute("available-from");
    if (!s.isEmpty())
        m_availableFrom = DateTime::fromString(s, status.projectTimeZone());
    s = element.attribute("available-until");
    if (!s.isEmpty())
        m_availableUntil = DateTime::fromString(s, status.projectTimeZone());

    // NOTE: money was earlier (2.x) saved with symbol so we need to handle that
    QString money = element.attribute("normal-rate");
    bool ok = false;
    cost.normalRate = money.toDouble(&ok);
    if (!ok) {
        cost.normalRate = locale->readMoney(money);
        debugPlan<<"normal-rate failed, tried readMoney()"<<money<<"->"<<cost.normalRate;;
    }
    money = element.attribute("overtime-rate");
    cost.overtimeRate = money.toDouble(&ok);
    if (!ok) {
        cost.overtimeRate = locale->readMoney(money);
        debugPlan<<"overtime-rate failed, tried readMoney()"<<money<<"->"<<cost.overtimeRate;;
    }
    cost.account = status.project().accounts().findAccount(element.attribute("account"));

    if (status.version() < "0.7.0") {
        KoXmlElement e;
        KoXmlElement parent = element.namedItem("required-resources").toElement();
        forEachElement(e, parent) {
            if (e.nodeName() == "resource") {
                QString id = e.attribute("id");
                if (id.isEmpty()) {
                    warnPlan<<"Missing resource id";
                    continue;
                }
                addRequiredId(id);
            }
        }
        parent = element.namedItem("external-appointments").toElement();
        forEachElement(e, parent) {
            if (e.nodeName() == "project") {
                QString id = e.attribute("id");
                if (id.isEmpty()) {
                    errorPlan<<"Missing project id";
                    continue;
                }
                clearExternalAppointments(id); // in case...
                AppointmentIntervalList lst;
                lst.loadXML(e, status);
                Appointment *a = new Appointment();
                a->setIntervals(lst);
                a->setAuxcilliaryInfo(e.attribute("name", "Unknown"));
                m_externalAppointments[ id ] = a;
            }
        }
    }
    loadCalendarIntervalsCache(element, status);
    return true;
}

QList<Resource*> Resource::requiredResources() const
{
    QList<Resource*> lst;
    foreach (const QString &s, m_requiredIds) {
        Resource *r = findId(s);
        if (r) {
            lst << r;
        }
    }
    return lst;
}

void Resource::setRequiredIds(const QStringList &ids)
{
    debugPlan<<ids;
    m_requiredIds = ids;
}

void Resource::addRequiredId(const QString &id)
{
    if (! id.isEmpty() && ! m_requiredIds.contains(id)) {
        m_requiredIds << id;
    }
}


void Resource::setAccount(Account *account)
{
    if (cost.account) {
        cost.account->removeRunning(*this);
    }
    cost.account = account;
    changed();
}

void Resource::save(QDomElement &element) const {
    //debugPlan;
    QDomElement me = element.ownerDocument().createElement("resource");
    element.appendChild(me);

    if (calendar(true))
        me.setAttribute("calendar-id", m_calendar->id());
    me.setAttribute("id", m_id);
    me.setAttribute("name", m_name);
    me.setAttribute("initials", m_initials);
    me.setAttribute("email", m_email);
    me.setAttribute("auto-allocate", m_autoAllocate);
    me.setAttribute("type", typeToString());
    me.setAttribute("shared", m_shared);
    me.setAttribute("units", QString::number(m_units));
    if (m_availableFrom.isValid()) {
        me.setAttribute("available-from", m_availableFrom.toString(Qt::ISODate));
    }
    if (m_availableUntil.isValid()) {
        me.setAttribute("available-until", m_availableUntil.toString(Qt::ISODate));
    }
    QString money;
    me.setAttribute("normal-rate", money.setNum(cost.normalRate));
    me.setAttribute("overtime-rate", money.setNum(cost.overtimeRate));
    if (cost.account) {
        me.setAttribute("account", cost.account->name());
    }
    saveCalendarIntervalsCache(me);
}

bool Resource::isAvailable(Task * /*task*/) {
    bool busy = false;
/*
    foreach (Appointment *a, m_appointments) {
        if (a->isBusy(task->startTime(), task->endTime())) {
            busy = true;
            break;
        }
    }*/
    return !busy;
}

QList<Appointment*> Resource::appointments(long id) const {
    Schedule *s = schedule(id);
    if (s == 0) {
        return QList<Appointment*>();
    }
    return s->appointments();
}

bool Resource::addAppointment(Appointment *appointment) {
    if (m_currentSchedule)
        return m_currentSchedule->add(appointment);
    return false;
}

bool Resource::addAppointment(Appointment *appointment, Schedule &main) {
    Schedule *s = findSchedule(main.id());
    if (s == 0) {
        s = createSchedule(&main);
    }
    appointment->setResource(s);
    return s->add(appointment);
}

// called from makeAppointment
void Resource::addAppointment(Schedule *node, const DateTime &start, const DateTime &end, double load)
{
    Q_ASSERT(start < end);
    Schedule *s = findSchedule(node->id());
    if (s == 0) {
        s = createSchedule(node->parent());
    }
    s->setCalculationMode(node->calculationMode());
    //debugPlan<<"id="<<node->id()<<" Mode="<<node->calculationMode()<<""<<start<<end;
    s->addAppointment(node, start, end, load);
}

void Resource::initiateCalculation(Schedule &sch) {
    m_currentSchedule = createSchedule(&sch);
}

Schedule *Resource::schedule(long id) const
{
    return id == -1 ? m_currentSchedule : findSchedule(id);
}

bool Resource::isBaselined(long id) const
{
    if (m_type == Resource::Type_Team) {
        foreach (const Resource *r, teamMembers()) {
            if (r->isBaselined(id)) {
                return true;
            }
        }
        return false;
    }
    Schedule *s = schedule(id);
    return s ? s->isBaselined() : false;
}

Schedule *Resource::findSchedule(long id) const
{
    if (m_schedules.contains(id)) {
        return m_schedules[ id ];
    }
    if (id == CURRENTSCHEDULE) {
        return m_currentSchedule;
    }
    if (id == BASELINESCHEDULE || id == ANYSCHEDULED) {
        foreach (Schedule *s, m_schedules) {
            if (s->isBaselined()) {
                return s;
            }
        }
    }
    if (id == ANYSCHEDULED) {
        foreach (Schedule *s, m_schedules) {
            if (s->isScheduled()) {
                return s;
            }
        }
    }
    return 0;
}

bool Resource::isScheduled() const
{
    foreach (Schedule *s, m_schedules) {
        if (s->isScheduled()) {
            return true;
        }
    }
    return false;
}

void Resource::deleteSchedule(Schedule *schedule) {
    takeSchedule(schedule);
    delete schedule;
}

void Resource::takeSchedule(const Schedule *schedule) {
    if (schedule == 0)
        return;
    if (m_currentSchedule == schedule)
        m_currentSchedule = 0;
    m_schedules.take(schedule->id());
}

void Resource::addSchedule(Schedule *schedule) {
    if (schedule == 0)
        return;
    m_schedules.remove(schedule->id());
    m_schedules.insert(schedule->id(), schedule);
}

ResourceSchedule *Resource::createSchedule(const QString& name, int type, long id) {
    ResourceSchedule *sch = new ResourceSchedule(this, name, (Schedule::Type)type, id);
    addSchedule(sch);
    return sch;
}

ResourceSchedule *Resource::createSchedule(Schedule *parent) {
    ResourceSchedule *sch = new ResourceSchedule(parent, this);
    //debugPlan<<"id="<<sch->id();
    addSchedule(sch);
    return sch;
}

QTimeZone Resource::timeZone() const
{
    Calendar *cal = calendar();

    return
        cal ?       cal->timeZone() :
        m_project ? m_project->timeZone() :
        /* else */  QTimeZone();
}

DateTimeInterval Resource::requiredAvailable(Schedule *node, const DateTime &start, const DateTime &end) const
{
    Q_ASSERT(m_currentSchedule);
    DateTimeInterval interval(start, end);
#ifndef PLAN_NLOGDEBUG
    if (m_currentSchedule) m_currentSchedule->logDebug(QString("Required available in interval: %1").arg(interval.toString()));
#endif
    DateTime availableFrom = m_availableFrom.isValid() ? m_availableFrom : (m_project ? m_project->constraintStartTime() : DateTime());
    DateTime availableUntil = m_availableUntil.isValid() ? m_availableUntil : (m_project ? m_project->constraintEndTime() : DateTime());
    DateTimeInterval x = interval.limitedTo(availableFrom, availableUntil);
    if (calendar() == 0) {
#ifndef PLAN_NLOGDEBUG
        if (m_currentSchedule) m_currentSchedule->logDebug(QString("Required available: no calendar, %1").arg(x.toString()));
#endif
        return x;
    }
    DateTimeInterval i = m_currentSchedule->firstBookedInterval(x, node);
    if (i.isValid()) {
#ifndef PLAN_NLOGDEBUG
        if (m_currentSchedule) m_currentSchedule->logDebug(QString("Required available: booked, %1").arg(i.toString()));
#endif
        return i; 
    }
    i = calendar()->firstInterval(x.first, x.second, m_currentSchedule);
#ifndef PLAN_NLOGDEBUG
    if (m_currentSchedule) m_currentSchedule->logDebug(QString("Required first available in %1:  %2").arg(x.toString()).arg(i.toString()));
#endif
    return i;
}

void Resource::makeAppointment(Schedule *node, const DateTime &from, const DateTime &end, int load, const QList<Resource*> &required) {
    //debugPlan<<"node id="<<node->id()<<" mode="<<node->calculationMode()<<""<<from<<" -"<<end;
    if (!from.isValid() || !end.isValid()) {
        m_currentSchedule->logWarning(i18n("Make appointments: Invalid time"));
        return;
    }
    Calendar *cal = calendar();
    if (cal == 0) {
        m_currentSchedule->logWarning(i18n("Resource %1 has no calendar defined", m_name));
        return;
    }
#ifndef PLAN_NLOGDEBUG
    if (m_currentSchedule) {
        QStringList lst; foreach (Resource *r, required) { lst << r->name(); }
        m_currentSchedule->logDebug(QString("Make appointments from %1 to %2 load=%4, required: %3").arg(from.toString()).arg(end.toString()).arg(lst.join(",")).arg(load));
    }
#endif
    AppointmentIntervalList lst = workIntervals(from, end, m_currentSchedule);
    foreach (const AppointmentInterval &i, lst.map()) {
        m_currentSchedule->addAppointment(node, i.startTime(), i.endTime(), load);
        foreach (Resource *r, required) {
            r->addAppointment(node, i.startTime(), i.endTime(), r->units()); //FIXME: units may not be correct
        }
    }
}

void Resource::makeAppointment(Schedule *node, int load, const QList<Resource*> &required) {
    //debugPlan<<m_name<<": id="<<m_currentSchedule->id()<<" mode="<<m_currentSchedule->calculationMode()<<node->node()->name()<<": id="<<node->id()<<" mode="<<node->calculationMode()<<""<<node->startTime;
    QLocale locale;
    if (!node->startTime.isValid()) {
        m_currentSchedule->logWarning(i18n("Make appointments: Node start time is not valid"));
        return;
    }
    if (!node->endTime.isValid()) {
        m_currentSchedule->logWarning(i18n("Make appointments: Node end time is not valid"));
        return;
    }
    if (m_type == Type_Team) {
#ifndef PLAN_NLOGDEBUG
        m_currentSchedule->logDebug("Make appointments to team " + m_name);
#endif
        Duration e;
        foreach (Resource *r, teamMembers()) {
            r->makeAppointment(node, load, required);
        }
        return;
    }
    node->resourceNotAvailable = false;
    node->workStartTime = DateTime();
    node->workEndTime = DateTime();
    Calendar *cal = calendar();
    if (m_type == Type_Material) {
        DateTime from = availableAfter(node->startTime, node->endTime);
        DateTime end = availableBefore(node->endTime, node->startTime);
        if (!from.isValid() || !end.isValid()) {
            return;
        }
        if (cal == 0) {
            // Allocate the whole period
            addAppointment(node, from, end, m_units);
            return;
        }
        makeAppointment(node, from, end, load);
        return;
    }
    if (!cal) {
        m_currentSchedule->logWarning(i18n("Resource %1 has no calendar defined", m_name));
        return; 
    }
    DateTime time = node->startTime;
    DateTime end = node->endTime;
    if (time == end) {
#ifndef PLAN_NLOGDEBUG
        m_currentSchedule->logDebug(QString("Task '%1' start time == end time: %2").arg(node->node()->name(), time.toString(Qt::ISODate)));
#endif
        node->resourceNotAvailable = true;
        return;
    }
    time = availableAfter(time, end);
    if (!time.isValid()) {
        m_currentSchedule->logWarning(i18n("Resource %1 not available in interval: %2 to %3", m_name, locale.toString(node->startTime, QLocale::ShortFormat), locale.toString(end, QLocale::ShortFormat)));
        node->resourceNotAvailable = true;
        return;
    }
    end = availableBefore(end, time);
    foreach (Resource *r, required) {
        time = r->availableAfter(time, end);
        end = r->availableBefore(end, time);
        if (! (time.isValid() && end.isValid())) {
#ifndef PLAN_NLOGDEBUG
            if (m_currentSchedule) m_currentSchedule->logDebug("The required resource '" + r->name() + "'is not available in interval:" + node->startTime.toString() + ',' + node->endTime.toString());
#endif
            break;
        }
    }
    if (!end.isValid()) {
        m_currentSchedule->logWarning(i18n("Resource %1 not available in interval: %2 to %3", m_name, locale.toString(time, QLocale::ShortFormat), locale.toString(node->endTime, QLocale::ShortFormat)));
        node->resourceNotAvailable = true;
        return;
    }
    //debugPlan<<time.toString()<<" to"<<end.toString();
    makeAppointment(node, time, end, load, required);
}

AppointmentIntervalList Resource::workIntervals(const DateTime &from, const DateTime &until) const
{
    return workIntervals(from, until, 0);
}

AppointmentIntervalList Resource::workIntervals(const DateTime &from, const DateTime &until, Schedule *sch) const
{
    Calendar *cal = calendar();
    if (cal == 0) {
        return AppointmentIntervalList();
    }
    // update cache
    calendarIntervals(from, until);
    AppointmentIntervalList work = m_workinfocache.intervals.extractIntervals(from, until);
    if (sch && ! sch->allowOverbooking()) {
        foreach (const Appointment *a, sch->appointments(sch->calculationMode())) {
            work -= a->intervals();
        }
        foreach (const Appointment *a, m_externalAppointments) {
            work -= a->intervals();
        }
    }
    return work;
}

void Resource::calendarIntervals(const DateTime &from, const DateTime &until) const
{
    Calendar *cal = calendar();
    if (cal == 0) {
        m_workinfocache.clear();
        return;
    }
    if (cal->cacheVersion() != m_workinfocache.version) {
        m_workinfocache.clear();
        m_workinfocache.version = cal->cacheVersion();
    }
    if (! m_workinfocache.isValid()) {
        // First time
//         debugPlan<<"First time:"<<from<<until;
        m_workinfocache.start = from;
        m_workinfocache.end = until;
        m_workinfocache.intervals = cal->workIntervals(from, until, m_units);
//         debugPlan<<"calendarIntervals (first):"<<m_workinfocache.intervals;
    } else {
        if (from < m_workinfocache.start) {
//             debugPlan<<"Add to start:"<<from<<m_workinfocache.start;
            m_workinfocache.intervals += cal->workIntervals(from, m_workinfocache.start, m_units);
            m_workinfocache.start = from;
//             debugPlan<<"calendarIntervals (start):"<<m_workinfocache.intervals;
        }
        if (until > m_workinfocache.end) {
//             debugPlan<<"Add to end:"<<m_workinfocache.end<<until;
            m_workinfocache.intervals += cal->workIntervals(m_workinfocache.end, until, m_units);
            m_workinfocache.end = until;
//             debugPlan<<"calendarIntervals: (end)"<<m_workinfocache.intervals;
        }
    }
}

bool Resource::loadCalendarIntervalsCache(const KoXmlElement &element, XMLLoaderObject &status)
{
    KoXmlElement e = element.namedItem("work-intervals-cache").toElement();
    if (e.isNull()) {
        errorPlan<<"No 'work-intervals-cache' element";
        return true;
    }
    m_workinfocache.load(e, status);
    return true;
}

void Resource::saveCalendarIntervalsCache(QDomElement &element) const
{
    QDomElement me = element.ownerDocument().createElement("work-intervals-cache");
    element.appendChild(me);
    m_workinfocache.save(me);
}

DateTime Resource::WorkInfoCache::firstAvailableAfter(const DateTime &time, const DateTime &limit, Calendar *cal, Schedule *sch) const
{
    QMultiMap<QDate, AppointmentInterval>::const_iterator it = intervals.map().constEnd();
    if (start.isValid() && start <= time) {
        // possibly useful cache
        it = intervals.map().lowerBound(time.date());
    }
    if (it == intervals.map().constEnd()) {
        // nothing cached, check the old way
        DateTime t = cal ? cal->firstAvailableAfter(time, limit, sch) : DateTime();
        return t;
    }
    AppointmentInterval inp(time, limit);
    for (; it != intervals.map().constEnd() && it.key() <= limit.date(); ++it) {
        if (! it.value().intersects(inp) && it.value() < inp) {
            continue;
        }
        if (sch) {
            DateTimeInterval ti = sch->available(DateTimeInterval(it.value().startTime(), it.value().endTime()));
            if (ti.isValid() && ti.second > time && ti.first < limit) {
                ti.first = qMax(ti.first, time);
                return ti.first;
            }
        } else {
            DateTime t = qMax(it.value().startTime(), time);
            return t;
        }
    }
    if (it == intervals.map().constEnd()) {
        // ran out of cache, check the old way
        DateTime t = cal ? cal->firstAvailableAfter(time, limit, sch) : DateTime();
        return t;
    }
    return DateTime();
}

DateTime Resource::WorkInfoCache::firstAvailableBefore(const DateTime &time, const DateTime &limit, Calendar *cal, Schedule *sch) const
{
    if (time <= limit) {
        return DateTime();
    }
    QMultiMap<QDate, AppointmentInterval>::const_iterator it = intervals.map().constBegin();
    if (time.isValid() && limit.isValid() && end.isValid() && end >= time && ! intervals.isEmpty()) {
        // possibly useful cache
        it = intervals.map().upperBound(time.date());
    }
    if (it == intervals.map().constBegin()) {
        // nothing cached, check the old way
        DateTime t = cal ? cal->firstAvailableBefore(time, limit, sch) : DateTime();
        return t;
    }
    AppointmentInterval inp(limit, time);
    for (--it; it != intervals.map().constBegin() && it.key() >= limit.date(); --it) {
        if (! it.value().intersects(inp) && inp < it.value()) {
            continue;
        }
        if (sch) {
            DateTimeInterval ti = sch->available(DateTimeInterval(it.value().startTime(), it.value().endTime()));
            if (ti.isValid() && ti.second > limit) {
                ti.second = qMin(ti.second, time);
                return ti.second;
            }
        } else {
            DateTime t = qMin(it.value().endTime(), time);
            return t;
        }
    }
    if (it == intervals.map().constBegin()) {
        // ran out of cache, check the old way
        DateTime t = cal ? cal->firstAvailableBefore(time, limit, sch) : DateTime();
        return t;
    }
    return DateTime();
}

bool Resource::WorkInfoCache::load(const KoXmlElement &element, XMLLoaderObject &status)
{
    clear();
    version = element.attribute("version").toInt();
    effort = Duration::fromString(element.attribute("effort"));
    start = DateTime::fromString(element.attribute("start"));
    end = DateTime::fromString(element.attribute("end"));
    KoXmlElement e = element.namedItem("intervals").toElement();
    if (! e.isNull()) {
        intervals.loadXML(e, status);
    }
    //debugPlan<<*this;
    return true;
}

void Resource::WorkInfoCache::save(QDomElement &element) const
{
    element.setAttribute("version", QString::number(version));
    element.setAttribute("effort", effort.toString());
    element.setAttribute("start", start.toString(Qt::ISODate));
    element.setAttribute("end", end.toString(Qt::ISODate));
    QDomElement me = element.ownerDocument().createElement("intervals");
    element.appendChild(me);

    intervals.saveXML(me);
}

Duration Resource::effort(const DateTime& start, const Duration& duration, int units, bool backward, const QList< Resource* >& required) const
{
    return effort(m_currentSchedule, start, duration, units, backward, required);
}

// the amount of effort we can do within the duration
Duration Resource::effort(Schedule *sch, const DateTime &start, const Duration &duration, int units, bool backward, const QList<Resource*> &required) const
{
    //debugPlan<<m_name<<": ("<<(backward?"B)":"F)")<<start<<" for duration"<<duration.toString(Duration::Format_Day);
#if 0
    if (sch) sch->logDebug(QString("Check effort in interval %1: %2, %3").arg(backward?"backward":"forward").arg(start.toString()).arg((backward?start-duration:start+duration).toString()));
#endif
    Duration e;
    if (duration == 0 || m_units == 0 || units == 0) {
        warnPlan<<"zero duration or zero units";
        return e;
    }
    if (m_type == Type_Team) {
        errorPlan<<"A team resource cannot deliver any effort";
        return e;
    }
    Calendar *cal = calendar();
    if (cal == 0) {
        if (sch) sch->logWarning(i18n("Resource %1 has no calendar defined", m_name));
        return e;
    }
    DateTime from;
    DateTime until;
    if (backward) {
        from = availableAfter(start - duration, start, sch);
        until = availableBefore(start, start - duration, sch);
    } else {
        from = availableAfter(start, start + duration, sch);
        until = availableBefore(start + duration, start, sch);
    }
    if (! (from.isValid() && until.isValid())) {
#ifndef PLAN_NLOGDEBUG
        if (sch) sch->logDebug("Resource not available in interval:" + start.toString() + ',' + (start+duration).toString());
#endif
    } else {
        foreach (Resource *r, required) {
            from = r->availableAfter(from, until);
            until = r->availableBefore(until, from);
            if (! (from.isValid() && until.isValid())) {
#ifndef PLAN_NLOGDEBUG
                if (sch) sch->logDebug("The required resource '" + r->name() + "'is not available in interval:" + start.toString() + ',' + (start+duration).toString());
#endif
                    break;
            }
        }
    }
    if (from.isValid() && until.isValid()) {
#ifndef PLAN_NLOGDEBUG
        if (sch && until < from) sch->logDebug(" until < from: until=" + until.toString() + " from=" + from.toString());
#endif
        e = workIntervals(from, until).effort(from, until) * units / 100;
        if (sch && (! sch->allowOverbooking() || sch->allowOverbookingState() == Schedule::OBS_Deny)) {
            Duration avail = workIntervals(from, until, sch).effort(from, until);
            if (avail < e) {
                e = avail;
            }
        }
//        e = (cal->effort(from, until, sch)) * m_units / 100;
    }
    //debugPlan<<m_name<<start<<" e="<<e.toString(Duration::Format_Day)<<" ("<<m_units<<")";
#ifndef PLAN_NLOGDEBUG
    if (sch) sch->logDebug(QString("effort: %1 for %2 hours = %3").arg(start.toString()).arg(duration.toString(Duration::Format_HourFraction)).arg(e.toString(Duration::Format_HourFraction)));
#endif
    return e;
}

DateTime Resource::availableAfter(const DateTime &time, const DateTime &limit) const {
    return availableAfter(time, limit, m_currentSchedule);
}

DateTime Resource::availableBefore(const DateTime &time, const DateTime &limit) const {
    return availableBefore(time, limit, m_currentSchedule);
}

DateTime Resource::availableAfter(const DateTime &time, const DateTime &limit, Schedule *sch) const {
//     debugPlan<<time<<limit;
    DateTime t;
    if (m_units == 0) {
        debugPlan<<this<<"zero units";
        return t;
    }
    DateTime lmt = m_availableUntil.isValid() ? m_availableUntil : (m_project ? m_project->constraintEndTime() : DateTime());
    if (limit.isValid() && limit < lmt) {
        lmt = limit;
    }
    if (time >= lmt) {
        debugPlan<<this<<"time >= limit"<<time<<lmt<<m_project;
        return t;
    }
    Calendar *cal = calendar();
    if (cal == 0) {
        if (sch) sch->logWarning(i18n("Resource %1 has no calendar defined", m_name));
        debugPlan<<this<<"No calendar";
        return t;
    }
    DateTime availableFrom = m_availableFrom.isValid() ? m_availableFrom : (m_project ? m_project->constraintStartTime() : DateTime());
    t = availableFrom > time ? availableFrom : time;
    if (t >= lmt) {
        debugPlan<<this<<t<<lmt;
        return DateTime();
    }
    QTimeZone tz = cal->timeZone();
    t = t.toTimeZone(tz);
    lmt = lmt.toTimeZone(tz);
    t = m_workinfocache.firstAvailableAfter(t, lmt, cal, sch);
//    t = cal->firstAvailableAfter(t, lmt, sch);
    //if (sch) debugPlan<<sch<<""<<m_name<<" id="<<sch->id()<<" mode="<<sch->calculationMode()<<" returns:"<<time.toString()<<"="<<t.toString()<<""<<lmt.toString();
    return t;
}

DateTime Resource::availableBefore(const DateTime &time, const DateTime &limit, Schedule *sch) const {
    DateTime t;
    if (m_units == 0) {
        return t;
    }
    DateTime lmt = m_availableFrom.isValid() ? m_availableFrom : (m_project ? m_project->constraintStartTime() : DateTime());
    if (limit.isValid() && limit > lmt) {
        lmt = limit;
    }
    if (time <= lmt) {
        return t;
    }
    Calendar *cal = calendar();
    if (cal == 0) {
        return t;
    }
    DateTime availableUntil = m_availableUntil.isValid() ? m_availableUntil : (m_project ? m_project->constraintEndTime() : DateTime());
    if (! availableUntil.isValid()) {
#ifndef PLAN_NLOGDEBUG
        if (sch) sch->logDebug("availableUntil is invalid");
#endif
        t = time;
    } else {
        t = availableUntil < time ? availableUntil : time;
    }
#ifndef PLAN_NLOGDEBUG
    if (sch && t < lmt) sch->logDebug("t < lmt: " + t.toString() + " < " + lmt.toString());
#endif
    QTimeZone tz = cal->timeZone();
    t = t.toTimeZone(tz);
    lmt = lmt.toTimeZone(tz);
    t = m_workinfocache.firstAvailableBefore(t, lmt, cal, sch);
//    t = cal->firstAvailableBefore(t, lmt, sch);
#ifndef PLAN_NLOGDEBUG
    if (sch && t.isValid() && t < lmt) sch->logDebug(" t < lmt: t=" + t.toString() + " lmt=" + lmt.toString());
#endif
    return t;
}

Resource *Resource::findId(const QString &id) const { 
    return m_project ? m_project->findResource(id) : 0; 
}

bool Resource::removeId(const QString &id) { 
    return m_project ? m_project->removeResourceId(id) : false; 
}

void Resource::insertId(const QString &id) { 
    //debugPlan;
    if (m_project)
        m_project->insertResourceId(id, this); 
}

Calendar *Resource::findCalendar(const QString &id) const { 
    return (m_project ? m_project->findCalendar(id) : 0); 
}

bool Resource::isOverbooked() const {
    return isOverbooked(DateTime(), DateTime());
}

bool Resource::isOverbooked(const QDate &date) const {
    return isOverbooked(DateTime(date), DateTime(date.addDays(1)));
}

bool Resource::isOverbooked(const DateTime &start, const DateTime &end) const {
    //debugPlan<<m_name<<":"<<start.toString()<<" -"<<end.toString()<<" cs=("<<m_currentSchedule<<")";
    return m_currentSchedule ? m_currentSchedule->isOverbooked(start, end) : false;
}

Appointment Resource::appointmentIntervals(long id) const {
    Appointment a;
    Schedule *s = findSchedule(id);
    if (s == 0) {
        return a;
    }
    foreach (Appointment *app, static_cast<ResourceSchedule*>(s)->appointments()) {
        a += *app;
    }
    return a;
}

Appointment Resource::appointmentIntervals() const {
    Appointment a;
    if (m_currentSchedule == 0)
        return a;
    foreach (Appointment *app, m_currentSchedule->appointments()) {
        a += *app;
    }
    return a;
}

EffortCostMap Resource::plannedEffortCostPrDay(const QDate &start, const QDate &end, long id, EffortCostCalculationType typ)
{
    EffortCostMap ec;
    Schedule *s = findSchedule(id);
    if (s == 0) {
        return ec;
    }
    ec = s->plannedEffortCostPrDay(start, end, typ);
    return ec;
}

Duration Resource::plannedEffort(const QDate &date, EffortCostCalculationType typ) const
{
    return m_currentSchedule ? m_currentSchedule->plannedEffort(date, typ) : Duration::zeroDuration;
}

void Resource::setProject(Project *project)
{
    if (project != m_project) {
        if (m_project) {
            removeId();
        }
    }
    m_project = project;
}

void Resource::addExternalAppointment(const QString& id, Appointment* a)
{
    int row = -1;
    if (m_externalAppointments.contains(id)) {
        int row = m_externalAppointments.keys().indexOf(id); // clazy:exclude=container-anti-pattern
        emit externalAppointmentToBeRemoved(this, row);
        delete m_externalAppointments.take(id);
        emit externalAppointmentRemoved();
    }
    if (row == -1) {
        m_externalAppointments[ id ] = a;
        row = m_externalAppointments.keys().indexOf(id); // clazy:exclude=container-anti-pattern
        m_externalAppointments.remove(id);
    }
    emit externalAppointmentToBeAdded(this, row);
    m_externalAppointments[ id ] = a;
    emit externalAppointmentAdded(this, a);
}

void Resource::addExternalAppointment(const QString &id, const QString &name, const DateTime &from, const DateTime &end, double load)
{
    Appointment *a = m_externalAppointments.value(id);
    if (a == 0) {
        a = new Appointment();
        a->setAuxcilliaryInfo(name);
        a->addInterval(from, end, load);
        //debugPlan<<m_name<<name<<"new appointment:"<<a<<from<<end<<load;
        m_externalAppointments[ id ] = a;
        int row = m_externalAppointments.keys().indexOf(id); // clazy:exclude=container-anti-pattern
        m_externalAppointments.remove(id);
        emit externalAppointmentToBeAdded(this, row);
        m_externalAppointments[ id ] = a;
        emit externalAppointmentAdded(this, a);
    } else {
        //debugPlan<<m_name<<name<<"new interval:"<<a<<from<<end<<load;
        a->addInterval(from, end, load);
        emit externalAppointmentChanged(this, a);
    }
}

void Resource::subtractExternalAppointment(const QString &id, const DateTime &start, const DateTime &end, double load)
{
    Appointment *a = m_externalAppointments.value(id);
    if (a) {
        //debugPlan<<m_name<<name<<"new interval:"<<a<<from<<end<<load;
        Appointment app;
        app.addInterval(start, end, load);
        *a -= app;
        emit externalAppointmentChanged(this, a);
    }
}

void Resource::clearExternalAppointments()
{
    const QStringList keys = m_externalAppointments.keys();
    foreach (const QString &id, keys) {
        clearExternalAppointments(id);
    }
}

void Resource::clearExternalAppointments(const QString &projectId)
{
    while (m_externalAppointments.contains(projectId)) {
        int row = m_externalAppointments.keys().indexOf(projectId); // clazy:exclude=container-anti-pattern
        emit externalAppointmentToBeRemoved(this, row);
        Appointment *a  = m_externalAppointments.take(projectId);
        emit externalAppointmentRemoved();
        delete a;
    }
}

Appointment *Resource::takeExternalAppointment(const QString &id)
{
    Appointment *a = 0;
    if (m_externalAppointments.contains(id)) {
        int row = m_externalAppointments.keys().indexOf(id); // clazy:exclude=container-anti-pattern
        emit externalAppointmentToBeRemoved(this, row);
        a = m_externalAppointments.take(id);
        emit externalAppointmentRemoved();
    }
    return a;
}

AppointmentIntervalList Resource::externalAppointments(const QString &id)
{
    if (! m_externalAppointments.contains(id)) {
        return AppointmentIntervalList();
    }
    return m_externalAppointments[ id ]->intervals();
}

AppointmentIntervalList Resource::externalAppointments(const DateTimeInterval &interval) const
{
    //debugPlan<<m_externalAppointments;
    Appointment app;
    foreach (Appointment *a, m_externalAppointments) {
        app += interval.isValid() ? a->extractIntervals(interval) : *a;
    }
    return app.intervals();
}

QMap<QString, QString> Resource::externalProjects() const
{
    QMap<QString, QString> map;
    for (QMapIterator<QString, Appointment*> it(m_externalAppointments); it.hasNext();) {
        it.next();
        if (! map.contains(it.key())) {
            map[ it.key() ] = it.value()->auxcilliaryInfo();
        }
    }
//     debugPlan<<map;
    return map;
}

long Resource::allocationSuitability(const DateTime &time, const Duration &duration, bool backward)
{
    // FIXME: This is not *very* intelligent...
    Duration e;
    if (m_type == Type_Team) {
        foreach (Resource *r, teamMembers()) {
            e += r->effort(time, duration, 100, backward);
        }
    } else {
        e = effort(time, duration, 100, backward);
    }
    return e.minutes();
}

DateTime Resource::startTime(long id) const
{
    DateTime dt;
    Schedule *s = schedule(id);
    if (s == 0) {
        return dt;
    }
    foreach (Appointment *a, s->appointments()) {
        DateTime t = a->startTime();
        if (! dt.isValid() || t < dt) {
            dt = t;
        }
    }
    return dt;
}

DateTime Resource::endTime(long id) const
{
    DateTime dt;
    Schedule *s = schedule(id);
    if (s == 0) {
        return dt;
    }
    foreach (Appointment *a, s->appointments()) {
        DateTime t = a->endTime();
        if (! dt.isValid() || t > dt) {
            dt = t;
        }
    }
    return dt;
}

QList<Resource*> Resource::teamMembers() const
{
    QList<Resource*> lst;
    foreach (const QString &s, m_teamMembers) {
        Resource *r = findId(s);
        if (r) {
            lst << r;
        }
    }
    return lst;

}

QStringList Resource::teamMemberIds() const
{
    return m_teamMembers;
}

void Resource::addTeamMemberId(const QString &id)
{
    if (! id.isEmpty() && ! m_teamMembers.contains(id)) {
        m_teamMembers.append(id);
    }
}

void Resource::removeTeamMemberId(const QString &id)
{
    if (m_teamMembers.contains(id)) {
        m_teamMembers.removeAt(m_teamMembers.indexOf(id));
    }
}

void Resource::setTeamMemberIds(const QStringList &ids)
{
    m_teamMembers = ids;
}

bool Resource::isShared() const
{
    return m_shared;
}

void Resource::setShared(bool on)
{
    m_shared = on;
}

QDebug operator<<(QDebug dbg, const KPlato::Resource::WorkInfoCache &c)
{
    dbg.nospace()<<"WorkInfoCache: ["<<" version="<<c.version<<" start="<<c.start.toString(Qt::ISODate)<<" end="<<c.end.toString(Qt::ISODate)<<" intervals="<<c.intervals.map().count();
    if (! c.intervals.isEmpty()) {
        foreach (const AppointmentInterval &i, c.intervals.map()) {
        dbg<<endl<<"   "<<i;
        }
    }
    dbg<<"]";
    return dbg;
}

/////////   Risk   /////////
Risk::Risk(Node *n, Resource *r, RiskType rt) {
    m_node=n;
    m_resource=r;
    m_riskType=rt;
}

Risk::~Risk() {
}

QDebug operator<<(QDebug dbg, KPlato::Resource *r)
{
    if (!r) { return dbg << "Resource[0x0]"; }
    dbg << "Resource[" << r->type();
    dbg << (r->name().isEmpty() ? r->id() : r->name());
    dbg << ']';
    return dbg;
}
