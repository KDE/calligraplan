/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2001 Thomas zander <zander@kde.org>
 * SPDX-FileCopyrightText: 2004-2007, 2012 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
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
    : QObject(nullptr), // atm QObject is only for casting
    m_project(nullptr),
    m_autoAllocate(false),
    m_currentSchedule(nullptr),
    m_blockChanged(false),
    m_shared(false)
{
    m_type = Type_Work;
    m_units = 100; // %

//     m_availableFrom = DateTime(QDate::currentDate(), QTime(0, 0, 0));
//     m_availableUntil = m_availableFrom.addYears(2);

    m_cost.normalRate = 100;
    m_cost.overtimeRate = 0;
    m_cost.fixed = 0;
    m_cost.account = nullptr;
    m_calendar = nullptr;
    m_currentSchedule = nullptr;
    //debugPlan<<"("<<this<<")";
    
    // material: by default material is always available
    for (int i = 1; i <= 7; ++i) {
        CalendarDay *wd = m_materialCalendar.weekday(i);
        wd->setState(CalendarDay::Working);
        wd->addInterval(TimeInterval(QTime(0, 0, 0), 24*60*60*1000));
    }
}

Resource::Resource(Resource *resource)
    : QObject(nullptr), // atm QObject is only for casting
    m_project(nullptr),
    m_currentSchedule(nullptr),
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
    for (Schedule *s : std::as_const(m_schedules)) {
        delete s;
    }
    clearExternalAppointments();
    if (m_cost.account) {
        m_cost.account->removeRunning(*this);
    }
    for (ResourceGroup *g : std::as_const(m_parents)) {
        g->takeResource(this);
    }
}

void Resource::removeRequests() {
    for (ResourceRequest *r : std::as_const(m_requests)) {
        r->setResource(nullptr); // avoid the request to mess with my list
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
    m_project = nullptr; // NOTE: Don't copy, will be set when added to a project
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

    m_cost.normalRate = resource->normalRate();
    m_cost.overtimeRate = resource->overtimeRate();
    m_cost.account = resource->account();
    m_calendar = resource->m_calendar;

    m_requiredIds = resource->requiredIds();
    m_teamMemberIds = resource->m_teamMemberIds;

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
    if (!m_blockChanged) {
        Q_EMIT dataChanged(this);
        if (m_project) {
            Q_EMIT m_project->resourceChanged(this);
        }
    }
}

void Resource::setType(Type type)
{
    m_type = type;
    changed();
}

void Resource::setType(const QString &type)
{
    if (type == QStringLiteral("Work"))
        setType(Type_Work);
    else if (type == QStringLiteral("Material"))
        setType(Type_Material);
    else if (type == QStringLiteral("Team"))
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
            << (trans ? xi18nc("@item:inlistbox resource type", "Work") : QStringLiteral("Work"))
            << (trans ? xi18nc("@item:inlistbox resource type", "Material") : QStringLiteral("Material"))
            << (trans ? xi18nc("@item:inlistbox resource type", "Team") : QStringLiteral("Team"));
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
    Calendar *c = nullptr;
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
    if (!m_parents.contains(parent)) {
        Q_EMIT resourceGroupToBeAdded(this, m_parents.count());
        m_parents.append(parent);
        Q_EMIT resourceGroupAdded(parent);
        parent->addResource(this);
    }
}

bool Resource::removeParentGroup(ResourceGroup *parent)
{
    bool result = false;
    if (m_parents.contains(parent)) {
        int row = m_parents.indexOf(parent);
        Q_EMIT resourceGroupToBeRemoved(this, row, parent);
        result = m_parents.removeOne(parent);
        Q_EMIT resourceGroupRemoved();
    }
    parent->takeResource(this);
    return result;
}

void Resource::setParentGroups(const QList<ResourceGroup*> &parents)
{
    while (!m_parents.isEmpty()) {
        removeParentGroup(m_parents.at(0));
    }
    for (ResourceGroup *g : parents) {
        addParentGroup(g);
    }
}

QList<ResourceGroup*> Resource::parentGroups() const
{
    return m_parents;
}

int Resource::groupCount() const
{
    return m_parents.count();
}

int Resource::groupIndexOf(ResourceGroup *group) const
{
    return m_parents.indexOf(group);
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

QList<Resource*> Resource::requiredResources() const
{
    if (m_requiredResources.count() != m_requiredIds.count()) {
        const_cast<Resource*>(this)->m_requiredResources.clear();
        for (const QString &s : std::as_const(m_requiredIds)) {
            Resource *r = findId(s);
            if (r) {
                const_cast<Resource*>(this)->m_requiredResources << r;
            }
        }
    }
    if (m_requiredResources.count() != m_requiredIds.count()) {
        warnPlan<<this<<"Resolving required failed: m_requiredResources != m_requiredIds"<<m_requiredResources.count()<<m_requiredIds.count();
    }
    return m_requiredResources;
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

void Resource::refreshRequiredIds()
{
    m_requiredIds.clear();
    for (const auto r : std::as_const(m_requiredResources)) {
        m_requiredIds << r->id();
    }
}

void Resource::setAccount(Account *account)
{
    if (m_cost.account) {
        m_cost.account->removeRunning(*this);
    }
    m_cost.account = account;
    changed();
}

void Resource::save(QDomElement &element) const {
    //debugPlan;
    QDomElement me = element.ownerDocument().createElement(QStringLiteral("resource"));
    element.appendChild(me);

    if (calendar(true))
        me.setAttribute(QStringLiteral("calendar-id"), m_calendar->id());
    me.setAttribute(QStringLiteral("id"), m_id);
    me.setAttribute(QStringLiteral("name"), m_name);
    me.setAttribute(QStringLiteral("initials"), m_initials);
    me.setAttribute(QStringLiteral("email"), m_email);
    me.setAttribute(QStringLiteral("auto-allocate"), m_autoAllocate);
    me.setAttribute(QStringLiteral("type"), typeToString());
    me.setAttribute(QStringLiteral("origin"), m_shared ? QStringLiteral("shared") : QStringLiteral("local"));
    me.setAttribute(QStringLiteral("units"), QString::number(m_units));
    if (m_availableFrom.isValid()) {
        me.setAttribute(QStringLiteral("available-from"), m_availableFrom.toString(Qt::ISODate));
    }
    if (m_availableUntil.isValid()) {
        me.setAttribute(QStringLiteral("available-until"), m_availableUntil.toString(Qt::ISODate));
    }
    QString money;
    me.setAttribute(QStringLiteral("normal-rate"), money.setNum(m_cost.normalRate));
    me.setAttribute(QStringLiteral("overtime-rate"), money.setNum(m_cost.overtimeRate));
    if (m_cost.account) {
        me.setAttribute(QStringLiteral("account"), m_cost.account->name());
    }
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
    if (s == nullptr) {
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
    if (s == nullptr) {
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
    if (s == nullptr) {
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
        const auto resources = teamMembers();
        for (const Resource *r : resources) {
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
        for (Schedule *s : std::as_const(m_schedules)) {
            if (s->isBaselined()) {
                return s;
            }
        }
    }
    if (id == ANYSCHEDULED) {
        for (Schedule *s : std::as_const(m_schedules)) {
            if (s->isScheduled()) {
                return s;
            }
        }
    }
    return nullptr;
}

bool Resource::isScheduled() const
{
    for (Schedule *s : std::as_const(m_schedules)) {
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
    if (schedule == nullptr)
        return;
    if (m_currentSchedule == schedule)
        m_currentSchedule = nullptr;
    m_schedules.take(schedule->id());
}

void Resource::addSchedule(Schedule *schedule) {
    if (schedule == nullptr)
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
    if (m_currentSchedule) m_currentSchedule->logDebug(QStringLiteral("Required available in interval: %1").arg(interval.toString()));
#endif
    DateTime availableFrom = m_availableFrom.isValid() ? m_availableFrom : (m_project ? m_project->constraintStartTime() : DateTime());
    DateTime availableUntil = m_availableUntil.isValid() ? m_availableUntil : (m_project ? m_project->constraintEndTime() : DateTime());
    DateTimeInterval x = interval.limitedTo(availableFrom, availableUntil);
    if (calendar() == nullptr) {
#ifndef PLAN_NLOGDEBUG
        if (m_currentSchedule) m_currentSchedule->logDebug(QStringLiteral("Required available: no calendar, %1").arg(x.toString()));
#endif
        return x;
    }
    DateTimeInterval i = m_currentSchedule->firstBookedInterval(x, node);
    if (i.isValid()) {
#ifndef PLAN_NLOGDEBUG
        if (m_currentSchedule) m_currentSchedule->logDebug(QStringLiteral("Required available: booked, %1").arg(i.toString()));
#endif
        return i; 
    }
    i = calendar()->firstInterval(x.first, x.second, m_currentSchedule);
#ifndef PLAN_NLOGDEBUG
    if (m_currentSchedule) m_currentSchedule->logDebug(QStringLiteral("Required first available in %1:  %2").arg(x.toString()).arg(i.toString()));
#endif
    return i;
}

void Resource::makeAppointment(Schedule *node, const DateTime &from, const DateTime &end, int load, const QList<Resource*> &required) {
    //debugPlan<<"node id="<<node->id()<<" mode="<<node->calculationMode()<<""<<from<<" -"<<end;
    if (!from.isValid() || !end.isValid()) {
        if (m_currentSchedule) m_currentSchedule->logWarning(i18n("Make appointments: Invalid time"));
        return;
    }
    Calendar *cal = calendar();
    if (cal == nullptr) {
        if (m_currentSchedule) m_currentSchedule->logWarning(i18n("Resource %1 has no calendar defined", m_name));
        return;
    }
#ifndef PLAN_NLOGDEBUG
    if (m_currentSchedule) {
        QStringList lst; for (Resource *r : required) { lst << r->name(); }
        m_currentSchedule->logDebug(QStringLiteral("Make appointments from %1 to %2 load=%4, required: %3").arg(from.toString()).arg(end.toString()).arg(lst.join(QLatin1Char(','))).arg(load));
    }
#endif
    QTimeZone tz = m_project ? m_project->timeZone() : timeZone();
    AppointmentIntervalList lst = workIntervals(from, end, m_currentSchedule).toTimeZone(tz);
    const auto intervals = lst.map().values();
    for (const AppointmentInterval &i : intervals) {
        m_currentSchedule->addAppointment(node, i.startTime(), i.endTime(), load);
        for (Resource *r : required) {
            r->addAppointment(node, i.startTime(), i.endTime(), r->units()); //FIXME: units may not be correct
        }
    }
}

void Resource::makeAppointment(Schedule *node, int load, const QList<Resource*> &required) {
    //debugPlan<<m_name<<": id="<<m_currentSchedule->id()<<" mode="<<m_currentSchedule->calculationMode()<<node->node()->name()<<": id="<<node->id()<<" mode="<<node->calculationMode()<<""<<node->startTime;
    QLocale locale;
    if (!node->startTime.isValid()) {
        if (m_currentSchedule) m_currentSchedule->logWarning(i18n("Make appointments: Node start time is not valid"));
        return;
    }
    if (!node->endTime.isValid()) {
        if (m_currentSchedule) m_currentSchedule->logWarning(i18n("Make appointments: Node end time is not valid"));
        return;
    }
    if (m_type == Type_Team) {
#ifndef PLAN_NLOGDEBUG
        if (m_currentSchedule) m_currentSchedule->logDebug(QStringLiteral("Make appointments to team ") + m_name);
#endif
        Duration e;
        const auto resources = teamMembers();
        for (Resource *r : resources) {
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
        if (cal == nullptr) {
            // Allocate the whole period
            addAppointment(node, from, end, m_units);
            return;
        }
        makeAppointment(node, from, end, load);
        return;
    }
    if (!cal) {
        if (m_currentSchedule) m_currentSchedule->logWarning(i18n("Resource %1 has no calendar defined", m_name));
        return; 
    }
    DateTime time = node->startTime;
    DateTime end = node->endTime;
    if (time == end) {
#ifndef PLAN_NLOGDEBUG
       if (m_currentSchedule)  m_currentSchedule->logDebug(QStringLiteral("Task '%1' start time == end time: %2").arg(node->node()->name(), time.toString(Qt::ISODate)));
#endif
        node->resourceNotAvailable = true;
        return;
    }
    time = availableAfter(time, end);
    if (!time.isValid()) {
        if (m_currentSchedule) m_currentSchedule->logWarning(i18n("Resource %1 not available in interval: %2 to %3", m_name, locale.toString(node->startTime, QLocale::ShortFormat), locale.toString(end, QLocale::ShortFormat)));
        node->resourceNotAvailable = true;
        return;
    }
    end = availableBefore(end, time);
    for (Resource *r : required) {
        time = r->availableAfter(time, end);
        end = r->availableBefore(end, time);
        if (! (time.isValid() && end.isValid())) {
#ifndef PLAN_NLOGDEBUG
            if (m_currentSchedule)
                m_currentSchedule->logDebug(QStringLiteral("The required resource '") + r->name() + QStringLiteral("'is not available in interval:") + node->startTime.toString() + QLatin1Char(',') + node->endTime.toString());
#endif
            break;
        }
    }
    if (!end.isValid()) {
        if (m_currentSchedule) m_currentSchedule->logWarning(i18n("Resource %1 not available in interval: %2 to %3", m_name, locale.toString(time, QLocale::ShortFormat), locale.toString(node->endTime, QLocale::ShortFormat)));
        node->resourceNotAvailable = true;
        return;
    }
    //debugPlan<<time.toString()<<" to"<<end.toString();
    makeAppointment(node, time, end, load, required);
}

AppointmentIntervalList Resource::workIntervals(const DateTime &from, const DateTime &until) const
{
    return workIntervals(from, until, nullptr);
}

AppointmentIntervalList Resource::workIntervals(const DateTime &from, const DateTime &until, Schedule *sch) const
{
    Calendar *cal = calendar();
    if (cal == nullptr) {
        return AppointmentIntervalList();
    }
    // update cache
    calendarIntervals(from, until);
    AppointmentIntervalList work = m_workinfocache.intervals.extractIntervals(from, until);
    if (sch && ! sch->allowOverbooking()) {
        const auto appointments = sch->appointments(sch->calculationMode());
        for (const Appointment *a : appointments) {
            work -= a->intervals();
        }
        for (const Appointment *a : std::as_const(m_externalAppointments)) {
            work -= a->intervals();
        }
    }
    return work;
}

void Resource::calendarIntervals(const DateTime &dtFrom, const DateTime &dtUntil) const
{
    Calendar *cal = calendar();
    if (cal == nullptr) {
        m_workinfocache.clear();
        return;
    }
    if (cal->cacheVersion() != m_workinfocache.version) {
        m_workinfocache.clear();
        m_workinfocache.version = cal->cacheVersion();
    }
    QTimeZone tz = m_project ? m_project->timeZone() : timeZone();
    const DateTime from = dtFrom.toTimeZone(tz);
    const DateTime until = dtUntil.toTimeZone(tz);
    if (! m_workinfocache.isValid()) {
        // First time
//         debugPlan<<"First time:"<<from<<until;
        m_workinfocache.start = from;
        m_workinfocache.end = until;
        m_workinfocache.intervals = cal->workIntervals(from, until, m_units).toTimeZone(tz);
//         debugPlan<<"calendarIntervals (first):"<<m_workinfocache.intervals;
    } else {
        if (from < m_workinfocache.start) {
//             debugPlan<<"Add to start:"<<from<<m_workinfocache.start;
            m_workinfocache.intervals += cal->workIntervals(from, m_workinfocache.start, m_units).toTimeZone(tz);
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
    KoXmlElement e = element.namedItem(QStringLiteral("work-intervals-cache")).toElement();
    if (e.isNull()) {
        errorPlan<<"No 'work-intervals-cache' element";
        return true;
    }
    m_workinfocache.load(e, status);
    debugPlanXml<<this<<m_workinfocache.intervals;
    return true;
}

void Resource::saveCalendarIntervalsCache(QDomElement &element) const
{
    QDomElement me = element.ownerDocument().createElement(QStringLiteral("work-intervals-cache"));
    element.appendChild(me);
    m_workinfocache.save(me);
}

DateTime Resource::WorkInfoCache::firstAvailableAfter(const DateTime &time, const DateTime &limit, Calendar *cal, Schedule *sch) const
{
    const DateTime end = limit.toTimeZone(time.timeZone());
    QMultiMap<QDate, AppointmentInterval>::const_iterator it = intervals.map().constEnd();
    if (start.isValid() && start <= time) {
        // possibly useful cache
        it = intervals.map().lowerBound(time.date());
    }
    if (it == intervals.map().constEnd()) {
        // nothing cached, check the old way
        DateTime t = cal ? cal->firstAvailableAfter(time, end, sch) : DateTime();
        return t;
    }
    AppointmentInterval inp(time, end);
    for (; it != intervals.map().constEnd() && it.key() <= end.date(); ++it) {
        if (! it.value().intersects(inp) && it.value() < inp) {
            continue;
        }
        if (sch) {
            DateTimeInterval ti = sch->available(DateTimeInterval(it.value().startTime(), it.value().endTime()));
            if (ti.isValid() && ti.second > time && ti.first < end) {
                ti.first = qMax(ti.first, time);
                return ti.first;
            }
        } else {
            DateTime t = qMax(it.value().startTime(), time);
            return t;
        }
    }
    if (it == intervals.map().constEnd()) {
        DateTime t = cal ? cal->firstAvailableAfter(time, end, sch) : DateTime();
        return t.toTimeZone(time.timeZone());
    }
    return DateTime();
}

DateTime Resource::WorkInfoCache::firstAvailableBefore(const DateTime &time, const DateTime &limit, Calendar *cal, Schedule *sch) const
{
    const DateTime end = limit.toTimeZone(time.timeZone());
    if (time <= end) {
        return DateTime();
    }
    QMultiMap<QDate, AppointmentInterval>::const_iterator it = intervals.map().constBegin();
    if (time.isValid() && end.isValid() && end.isValid() && end >= time && ! intervals.isEmpty()) {
        // possibly useful cache
        it = intervals.map().upperBound(time.date());
    }
    if (it == intervals.map().constBegin()) {
        // nothing cached, check the old way
        DateTime t = cal ? cal->firstAvailableBefore(time, end, sch) : DateTime();
        return t;
    }
    AppointmentInterval inp(end, time);
    for (--it; it != intervals.map().constBegin() && it.key() >= end.date(); --it) {
        if (! it.value().intersects(inp) && inp < it.value()) {
            continue;
        }
        if (sch) {
            DateTimeInterval ti = sch->available(DateTimeInterval(it.value().startTime(), it.value().endTime()));
            if (ti.isValid() && ti.second > end) {
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
        DateTime t = cal ? cal->firstAvailableBefore(time, end, sch) : DateTime();
        return t.toTimeZone(time.timeZone());
    }
    return DateTime();
}

bool Resource::WorkInfoCache::load(const KoXmlElement &element, XMLLoaderObject &status)
{
    clear();
    version = element.attribute(QStringLiteral("version")).toInt();
    effort = Duration::fromString(element.attribute(QStringLiteral("effort")));
    // DateTime should always be saved in the projects timezone,
    // but due to a bug (fixed) this did not always happen.
    // This code takes care of this situation.
    start = QDateTime::fromString(element.attribute(QStringLiteral("start")), Qt::ISODate).toTimeZone(status.projectTimeZone());
    end = QDateTime::fromString(element.attribute(QStringLiteral("end")), Qt::ISODate).toTimeZone(status.projectTimeZone());
    KoXmlElement e = element.namedItem("intervals").toElement();
    if (! e.isNull()) {
        intervals.loadXML(e, status);
    }
    return true;
}

void Resource::WorkInfoCache::save(QDomElement &element) const
{
    element.setAttribute(QStringLiteral("version"), QString::number(version));
    element.setAttribute(QStringLiteral("effort"), effort.toString());
    element.setAttribute(QStringLiteral("start"), start.toString(Qt::ISODate));
    element.setAttribute(QStringLiteral("end"), end.toString(Qt::ISODate));
    QDomElement me = element.ownerDocument().createElement(QStringLiteral("intervals"));
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
    const DateTime startTime = m_project ? DateTime(start.toTimeZone(m_project->timeZone())) : start;
    //debugPlan<<m_name<<": ("<<(backward?"B)":"F)")<<startTime<<" for duration"<<duration.toString(Duration::Format_Day);
#if 0
    if (sch) sch->logDebug(QStringLiteral("Check effort in interval %1: %2, %3").arg(backward?"backward":"forward").arg(startTime.toString()).arg((backward?startTime-duration:startTime+duration).toString()));
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
    if (cal == nullptr) {
        if (sch) sch->logWarning(i18n("Resource %1 has no calendar defined", m_name));
        return e;
    }
    DateTime from;
    DateTime until;
    if (backward) {
        from = availableAfter(startTime - duration, startTime, sch);
        until = availableBefore(startTime, startTime - duration, sch);
    } else {
        from = availableAfter(startTime, startTime + duration, sch);
        until = availableBefore(startTime + duration, startTime, sch);
    }
    if (! (from.isValid() && until.isValid())) {
#ifndef PLAN_NLOGDEBUG
        if (sch) sch->logDebug(QStringLiteral("Resource not available in interval:") + startTime.toString() + QLatin1Char(',') + (startTime+duration).toString());
#endif
    } else {
        for (Resource *r : required) {
            from = r->availableAfter(from, until);
            until = r->availableBefore(until, from);
            if (! (from.isValid() && until.isValid())) {
#ifndef PLAN_NLOGDEBUG
                if (sch) sch->logDebug(QStringLiteral("The required resource '") + r->name() + QStringLiteral("' is not available in interval: ") + startTime.toString() + QLatin1Char(',') + (startTime+duration).toString());
#endif
                    break;
            }
        }
    }
    if (from.isValid() && until.isValid()) {
#ifndef PLAN_NLOGDEBUG
        if (sch && until < from) sch->logDebug(QStringLiteral(" until < from: until=") + until.toString() + QStringLiteral(" from=") + from.toString());
#endif
        e = workIntervals(from, until).effort(from, until) * units / 100;
        if (sch && (! sch->allowOverbooking() || sch->allowOverbookingState() == Schedule::OBS_Deny)) {
            auto s = m_project ? from.toTimeZone(m_project->timeZone()) : from;
            auto u = m_project ? until.toTimeZone(m_project->timeZone()) : until;
            Duration avail = workIntervals(s, u, sch).effort(s, u);
            if (avail < e) {
                e = avail;
            }
        }
//        e = (cal->effort(from, until, sch)) * m_units / 100;
    }
    //debugPlan<<m_name<<startTime<<" e="<<e.toString(Duration::Format_Day)<<" ("<<m_units<<")";
#ifndef PLAN_NLOGDEBUG
    if (sch) sch->logDebug(QStringLiteral("effort: %1 for %2 effort = %3").arg(startTime.toString()).arg(duration.toString()).arg(e.toString()));
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
    if (cal == nullptr) {
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
    t = m_workinfocache.firstAvailableAfter(t, lmt, cal, sch);
    if (m_project) {
        t = t.toTimeZone(m_project->timeZone());
    }
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
    if (cal == nullptr) {
        return t;
    }
    DateTime availableUntil = m_availableUntil.isValid() ? m_availableUntil : (m_project ? m_project->constraintEndTime() : DateTime());
    if (! availableUntil.isValid()) {
#ifndef PLAN_NLOGDEBUG
        if (sch) sch->logDebug(QStringLiteral("availableUntil is invalid"));
#endif
        t = time;
    } else {
        t = availableUntil < time ? availableUntil : time;
    }
#ifndef PLAN_NLOGDEBUG
    if (sch && t < lmt) sch->logDebug(QStringLiteral("t < lmt: ") + t.toString() + QStringLiteral(" < ") + lmt.toString());
#endif
    t = m_workinfocache.firstAvailableBefore(t, lmt, cal, sch);
    if (m_project) {
        t = t.toTimeZone(m_project->timeZone());
    }
#ifndef PLAN_NLOGDEBUG
    if (sch && t.isValid() && t < lmt) sch->logDebug(QStringLiteral(" t < lmt: t=") + t.toString() + QStringLiteral(" lmt=") + lmt.toString());
#endif
    return t;
}

Resource *Resource::findId(const QString &id) const { 
    return m_project ? m_project->findResource(id) : nullptr; 
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
    return (m_project ? m_project->findCalendar(id) : nullptr); 
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
    if (s == nullptr) {
        return a;
    }
    const auto appointments = static_cast<ResourceSchedule*>(s)->appointments();
    for (const Appointment *app : appointments) {
        a += *app;
    }
    return a;
}

Appointment Resource::appointmentIntervals() const {
    Appointment a;
    if (m_currentSchedule == nullptr)
        return a;
    const auto appointments = m_currentSchedule->appointments();
    for (const Appointment *app : appointments) {
        a += *app;
    }
    return a;
}

EffortCostMap Resource::plannedEffortCostPrDay(const QDate &start, const QDate &end, long id, EffortCostCalculationType typ)
{
    EffortCostMap ec;
    Schedule *s = findSchedule(id);
    if (s == nullptr) {
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
        Q_EMIT externalAppointmentToBeRemoved(this, row);
        delete m_externalAppointments.take(id);
        Q_EMIT externalAppointmentRemoved();
    }
    if (row == -1) {
        m_externalAppointments[ id ] = a;
        row = m_externalAppointments.keys().indexOf(id); // clazy:exclude=container-anti-pattern
        m_externalAppointments.remove(id);
    }
    Q_EMIT externalAppointmentToBeAdded(this, row);
    m_externalAppointments[ id ] = a;
    Q_EMIT externalAppointmentAdded(this, a);
}

void Resource::addExternalAppointment(const QString &id, const QString &name, const DateTime &from, const DateTime &end, double load)
{
    Appointment *a = m_externalAppointments.value(id);
    if (a == nullptr) {
        a = new Appointment();
        a->setAuxcilliaryInfo(name);
        a->addInterval(from, end, load);
        //debugPlan<<m_name<<name<<"new appointment:"<<a<<from<<end<<load;
        m_externalAppointments[ id ] = a;
        int row = m_externalAppointments.keys().indexOf(id); // clazy:exclude=container-anti-pattern
        m_externalAppointments.remove(id);
        Q_EMIT externalAppointmentToBeAdded(this, row);
        m_externalAppointments[ id ] = a;
        Q_EMIT externalAppointmentAdded(this, a);
    } else {
        //debugPlan<<m_name<<name<<"new interval:"<<a<<from<<end<<load;
        a->addInterval(from, end, load);
        Q_EMIT externalAppointmentChanged(this, a);
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
        Q_EMIT externalAppointmentChanged(this, a);
    }
}

void Resource::clearExternalAppointments()
{
    const QStringList keys = m_externalAppointments.keys();
    for (const QString &id : keys) {
        clearExternalAppointments(id);
    }
}

void Resource::clearExternalAppointments(const QString &projectId)
{
    while (m_externalAppointments.contains(projectId)) {
        int row = m_externalAppointments.keys().indexOf(projectId); // clazy:exclude=container-anti-pattern
        Q_EMIT externalAppointmentToBeRemoved(this, row);
        Appointment *a  = m_externalAppointments.take(projectId);
        Q_EMIT externalAppointmentRemoved();
        delete a;
    }
}

Appointment *Resource::takeExternalAppointment(const QString &id)
{
    Appointment *a = nullptr;
    if (m_externalAppointments.contains(id)) {
        int row = m_externalAppointments.keys().indexOf(id); // clazy:exclude=container-anti-pattern
        Q_EMIT externalAppointmentToBeRemoved(this, row);
        a = m_externalAppointments.take(id);
        Q_EMIT externalAppointmentRemoved();
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
    for (Appointment *a : std::as_const(m_externalAppointments)) {
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
        const auto resources = teamMembers();
        for (const Resource *r : resources) {
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
    if (s == nullptr) {
        return dt;
    }
    const auto appointments = s->appointments();
    for (const Appointment *a : appointments) {
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
    if (s == nullptr) {
        return dt;
    }
    const auto appointments = s->appointments();
    for (const Appointment *a : appointments) {
        DateTime t = a->endTime();
        if (! dt.isValid() || t > dt) {
            dt = t;
        }
    }
    return dt;
}

void Resource::updateCache() const
{
    if (m_teamMembers.count() != m_teamMemberIds.count()) {
        const_cast<Resource*>(this)->m_teamMembers.clear();
        for (const QString &s : std::as_const(m_teamMemberIds)) {
            Resource *r = findId(s);
            if (r) {
                const_cast<Resource*>(this)->m_teamMembers << r;
            }
        }
    }
}

int Resource::teamCount() const
{
    updateCache();
    return m_teamMembers.count();
}

QList<Resource*> Resource::teamMembers() const
{
    updateCache();
    return m_teamMembers;
}

QStringList Resource::teamMemberIds() const
{
    return m_teamMemberIds;
}

void Resource::addTeamMemberId(const QString &id)
{
    if (!id.isEmpty() && !m_teamMemberIds.contains(id)) {
        auto resource = findId(id);
        // only emit signals if everything is ok
        if (resource && m_teamMemberIds.count() == m_teamMembers.count()) {
            if (m_project) {
                Q_EMIT teamToBeAdded(this, m_teamMemberIds.count());
            }
            m_teamMemberIds.append(id);
            m_teamMembers << resource;
            if (m_project) {
                Q_EMIT teamAdded(resource);
            }
        } else {
            m_teamMemberIds.append(id);
        }
    }
}

void Resource::removeTeamMemberId(const QString &id)
{
    if (m_teamMemberIds.contains(id)) {
        int row = m_teamMemberIds.indexOf(id);
        Resource *team = m_teamMembers.value(row);
        if (!team) {
            // something wrong, try to clean up
            warnPlan<<"Resource not found:"<<id;
            m_teamMemberIds.removeAt(row);
            m_teamMembers.clear();
            updateCache();
            return;
        }
        Q_EMIT teamToBeRemoved(this, row, team);
        m_teamMemberIds.removeAt(row);
        m_teamMembers.removeAll(team);
#ifndef NDEBUG
        Q_ASSERT(m_teamMemberIds.count() == m_teamMembers.count());
        QStringList ids;
        for (const auto r : std::as_const(m_teamMembers)) {
            ids << r->id();
        }
        Q_ASSERT(ids == m_teamMemberIds);
        for (const auto r : std::as_const(m_teamMembers)) {
            Q_ASSERT(r == findId(r->id()));
        }
#endif
        Q_EMIT teamRemoved();
    } else {
        warnPlan<<"Tried to remove nonexisting id:"<<id;
    }
}

void Resource::setTeamMemberIds(const QStringList &ids)
{
    while (!m_teamMemberIds.isEmpty()) {
        removeTeamMemberId(m_teamMemberIds.last());
    }
    for (const QString &id : ids) {
        addTeamMemberId(id);
    }
}

void Resource::refreshTeamMemberIds()
{
    m_teamMemberIds.clear();
    for (const auto r : std::as_const(m_teamMembers)) {
        m_teamMemberIds << r->id();
    }
}

bool Resource::isShared() const
{
    return m_shared;
}

void Resource::setShared(bool on)
{
    m_shared = on;
    Q_EMIT dataChanged(this);
    if (m_project) {
        Q_EMIT m_project->resourceChanged(this);
    }
}

QDebug operator<<(QDebug dbg, const KPlato::Resource::WorkInfoCache &c)
{
    dbg.nospace()<<"WorkInfoCache: ["<<" version="<<c.version<<" start="<<c.start.toString(Qt::ISODate)<<" end="<<c.end.toString(Qt::ISODate)<<" intervals="<<c.intervals.map().count();
    if (! c.intervals.isEmpty()) {
        const auto intervals = c.intervals.map().values();
        for (const AppointmentInterval &i : intervals) {
        dbg<<'\n'<<"   "<<i;
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

QDebug operator<<(QDebug dbg, const KPlato::Resource &r)
{
    return dbg << &r;
}

QDebug operator<<(QDebug dbg, const KPlato::Resource *r)
{
    if (!r) { return dbg << "Resource[0x0]"; }
    dbg.noquote().nospace() << "Resource[(" << (void*)r << ") ";
    dbg << '"' << (r->name().isEmpty() ? r->id() : r->name()) << '"' << ' ';
    dbg.nospace() << (r->isShared() ? 'S' : 'L');
    switch (r->type()) {
        case Resource::Type_Work: dbg << 'W'; break;
        case Resource::Type_Material: dbg << 'M'; break;
        case Resource::Type_Team: dbg << 'T'; break;
    }
    if (r->type() == Resource::Type_Team) {
        dbg << ':';
        dbg.noquote().nospace() << r->teamMembers();
    }
    dbg.nospace() << ']';
    return dbg.space().quote();
}
