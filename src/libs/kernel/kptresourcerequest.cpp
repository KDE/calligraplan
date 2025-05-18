/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2001 Thomas Zander zander @kde.org
 * SPDX-FileCopyrightText: 2004-2007 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2011 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "kptresourcerequest.h"

#include "ResourceGroup.h"
#include "Resource.h"
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
#include <KFormat>

#include <QLocale>
#include <QDomElement>
#include <QRegularExpression>

using namespace KPlato;

ResourceRequest::ResourceRequest(Resource *resource, int units)
    : m_id(0),
      m_resource(resource),
      m_units(units),
      m_collection(nullptr),
      m_dynamic(false)
{
    if (resource) {
        m_required = resource->requiredResources();
    }
    //debugPlan<<"("<<this<<") Request to:"<<(resource ? resource->name() : QString("None"));
}

ResourceRequest::ResourceRequest(const ResourceRequest &r)
    : m_id(r.m_id),
      m_resource(r.m_resource),
      m_units(r.m_units),
      m_collection(nullptr),
      m_dynamic(r.m_dynamic),
      m_required(r.m_required),
      m_alternativeRequests(r.m_alternativeRequests)
{
}

ResourceRequest::~ResourceRequest() {
    if (m_resource) {
        m_resource->unregisterRequest(this);
    }
    m_resource = nullptr;
    if (m_collection && m_collection->contains(this)) {
        m_collection->removeResourceRequest(this);
    }
}

ResourceRequestCollection *ResourceRequest::collection() const
{
    return m_collection;
}

void ResourceRequest::setCollection(ResourceRequestCollection *collection)
{
    m_collection = collection;
}

int ResourceRequest::id() const
{
    return m_id;
}

void ResourceRequest::setId(int id)
{
    m_id = id;
}

void ResourceRequest::registerRequest()
{
    if (m_resource)
        m_resource->registerRequest(this);
}

void ResourceRequest::unregisterRequest()
{
    if (m_resource)
        m_resource->unregisterRequest(this);
}

void ResourceRequest::save(QDomElement &element) const {
    QDomElement me = element.ownerDocument().createElement(QStringLiteral("resource-request"));
    element.appendChild(me);
    me.setAttribute(QStringLiteral("resource-id"), m_resource->id());
    me.setAttribute(QStringLiteral("units"), QString::number(m_units));
}

int ResourceRequest::units() const {
    //debugPlan<<m_resource->name()<<": units="<<m_units;
    return m_units;
}

void ResourceRequest::setUnits(int value)
{
    m_units = value; changed();
}

Task *ResourceRequest::task() const {
    return m_collection ? m_collection->task() : nullptr;
}

void ResourceRequest::changed()
{
    if (task()) {
        task()->changed(Node::ResourceRequestProperty);
    }
}

void ResourceRequest::setCurrentSchedulePtr(Schedule *ns)
{
    setCurrentSchedulePtr(m_resource, ns);
}

void ResourceRequest::setCurrentSchedulePtr(Resource *resource, Schedule *ns)
{
    resource->setCurrentSchedulePtr(resourceSchedule(ns, resource));
    if(resource->type() == Resource::Type_Team) {
        const auto resources = resource->teamMembers();
        for (Resource *member : resources) {
            member->setCurrentSchedulePtr(resourceSchedule(ns, member));
        }
    }
    for (Resource *r : std::as_const(m_required)) {
        r->setCurrentSchedulePtr(resourceSchedule(ns, r));
    }
}

Schedule *ResourceRequest::resourceSchedule(Schedule *ns, Resource *res)
{
    if (ns == nullptr) {
        return nullptr;
    }
    Resource *r = res == nullptr ? resource() : res;
    Schedule *s = r->findSchedule(ns->id());
    if (s == nullptr) {
        s = r->createSchedule(ns->parent());
    }
    s->setCalculationMode(ns->calculationMode());
    s->setAllowOverbookingState(ns->allowOverbookingState());
    static_cast<ResourceSchedule*>(s)->setNodeSchedule(ns);
    //debugPlan<<s->name()<<": id="<<s->id()<<" mode="<<s->calculationMode();
    return s;
}

DateTime ResourceRequest::workTimeAfter(const DateTime &dt, Schedule *ns) {
    if (m_resource->type() == Resource::Type_Work) {
        DateTime t = availableAfter(dt, ns);
        for (Resource *r : std::as_const(m_required)) {
            if (! t.isValid()) {
                break;
            }
            t = r->availableAfter(t, DateTime(), resourceSchedule(ns, r));
        }
        return t;
    } else if (m_resource->type() == Resource::Type_Team) {
        return availableAfter(dt, ns);
    }
    return DateTime();
}

DateTime ResourceRequest::workTimeBefore(const DateTime &dt, Schedule *ns) {
    if (m_resource->type() == Resource::Type_Work) {
        DateTime t = availableBefore(dt, ns);
        for (Resource *r : std::as_const(m_required)) {
            if (! t.isValid()) {
                break;
            }
            t = r->availableBefore(t, DateTime(), resourceSchedule(ns, r));
        }
        return t;
    } else if (m_resource->type() == Resource::Type_Team) {
        return availableBefore(dt, ns);
    }
    return DateTime();
}

DateTime ResourceRequest::availableFrom()
{
    DateTime dt = m_resource->availableFrom();
    if (! dt.isValid()) {
        dt = m_resource->project()->constraintStartTime();
    }
    return dt;
}

DateTime ResourceRequest::availableUntil()
{
    DateTime dt = m_resource->availableUntil();
    if (! dt.isValid()) {
        dt = m_resource->project()->constraintEndTime();
    }
    return dt;
}

DateTime ResourceRequest::availableAfter(const DateTime &time, Schedule *ns) {
    if (m_resource->type() == Resource::Type_Team) {
        DateTime t;// = m_resource->availableFrom();
        const auto resources = m_resource->teamMembers();
        for (Resource *r : resources) {
            setCurrentSchedulePtr(r, ns);
            DateTime x = r->availableAfter(time);
            if (x.isValid()) {
                t = t.isValid() ? qMin(t, x) : x;
            }
        }
        return t;
    }
    setCurrentSchedulePtr(ns);
    return m_resource->availableAfter(time);
}

DateTime ResourceRequest::availableBefore(const DateTime &time, Schedule *ns) {
    if (m_resource->type() == Resource::Type_Team) {
        DateTime t;
        const auto resources = m_resource->teamMembers();
        for (Resource *r : resources) {
            setCurrentSchedulePtr(r, ns);
            DateTime x = r->availableBefore(time);
            if (x.isValid()) {
                t = t.isValid() ? qMax(t, x) : x;
            }
        }
        return t;
    }
    setCurrentSchedulePtr(ns);
    return resource()->availableBefore(time);
}

Duration ResourceRequest::effort(const DateTime &time, const Duration &duration, Schedule *ns, bool backward)
{
    Duration e;
    if (m_resource->type() == Resource::Type_Team) {
        const auto members = teamMembers();
        for (ResourceRequest *rr : members) {
            e += rr->effort(time, duration, ns, backward);
        }
    } else {
        setCurrentSchedulePtr(ns);
        e = m_resource->effort(time, duration, m_units, backward, m_required);
    }
    //debugPlan<<m_resource->name()<<time<<duration.toString()<<"delivers:"<<e.toString()<<"request:"<<(m_units/100)<<"parts";
    return e;
}

void ResourceRequest::makeAppointment(Schedule *ns)
{
    if (m_resource) {
        setCurrentSchedulePtr(ns);
        m_resource->makeAppointment(ns, (m_resource->units() * m_units / 100), m_required);
    }
}

void ResourceRequest::makeAppointment(Schedule *ns, int amount)
{
    if (m_resource) {
        setCurrentSchedulePtr(ns);
        m_resource->makeAppointment(ns, amount, m_required);
    }
}

long ResourceRequest::allocationSuitability(const DateTime &time, const Duration &duration, Schedule *ns, bool backward)
{
    setCurrentSchedulePtr(ns);
    return resource()->allocationSuitability(time, duration, backward);
}

QList<ResourceRequest*> ResourceRequest::teamMembers() const
{
    qDeleteAll(m_teamMembers);
    m_teamMembers.clear();
    if (m_resource->type() == Resource::Type_Team) {
        const auto resources = m_resource->teamMembers();
        for (Resource *r : resources) {
            m_teamMembers << new ResourceRequest(r, m_units);
        }
    }
    return m_teamMembers;
}

QList<Resource*> ResourceRequest::requiredResources() const
{
    return m_required;
}

void ResourceRequest::setRequiredResources(const QList<Resource*> &lst)
{
    m_required = lst;
    changed();
}

void ResourceRequest::addRequiredResource(Resource *resource)
{
    Q_ASSERT(!m_required.contains(resource));
    if (!m_required.contains(resource)) {
        m_required << resource;
        changed();
    }
}

void ResourceRequest::removeRequiredResource(Resource *resource)
{
    Q_ASSERT(m_required.contains(resource));
    if (m_required.contains(resource)) {
        m_required.removeAll(resource);
        changed();
    }
}

QList<ResourceRequest*> ResourceRequest::alternativeRequests() const
{
    return m_alternativeRequests;
}

void ResourceRequest::setAlternativeRequests(const QList<ResourceRequest*> requests)
{
    for (ResourceRequest *r : std::as_const(m_alternativeRequests)) {
        removeAlternativeRequest(r);
    }
    for (ResourceRequest *r : requests) {
        addAlternativeRequest(r);
    }
}

bool ResourceRequest::addAlternativeRequest(ResourceRequest *request)
{
    emitAlternativeRequestToBeAdded(this, m_alternativeRequests.count());
    m_alternativeRequests.append(request);
    emitAlternativeRequestAdded(request);
    return true;
}

bool ResourceRequest::removeAlternativeRequest(ResourceRequest *request)
{
    emitAlternativeRequestToBeRemoved(this, m_alternativeRequests.indexOf(request), request);
    m_alternativeRequests.removeAll(request);
    emitAlternativeRequestRemoved();
    return true;
}

void ResourceRequest::emitAlternativeRequestToBeAdded(ResourceRequest *request, int row)
{
    if (m_collection) {
        Q_EMIT m_collection->alternativeRequestToBeAdded(request, row);
    }
}

void ResourceRequest::emitAlternativeRequestAdded(ResourceRequest *alternative)
{
    if (m_collection) {
        Q_EMIT m_collection->alternativeRequestAdded(alternative);
    }
}

void ResourceRequest::emitAlternativeRequestToBeRemoved(ResourceRequest *request, int row, ResourceRequest *alternative)
{
    if (m_collection) {
        Q_EMIT m_collection->alternativeRequestToBeRemoved(request, row, alternative);
    }
}

void ResourceRequest::emitAlternativeRequestRemoved()
{
    if (m_collection) {
        Q_EMIT m_collection->alternativeRequestRemoved();
    }
}

/////////
ResourceRequestCollection::ResourceRequestCollection(Task *task)
    : QObject()
    , m_task(task)
    , m_lastResourceId(0)
{
    //debugPlan<<this<<(void*)(&task);
}

ResourceRequestCollection::~ResourceRequestCollection()
{
    //debugPlan<<this;
    for (ResourceRequest *r : std::as_const(m_resourceRequests)) {
        r->setCollection(nullptr);
    }
    qDeleteAll(m_resourceRequests); // removes themselves from possible group
}

bool ResourceRequestCollection::contains(ResourceRequest *request) const
{
    return m_resourceRequests.contains(request->id()) && m_resourceRequests.value(request->id()) == request;
}

void ResourceRequestCollection::removeRequests()
{
    const QList<ResourceRequest*> resourceRequests = m_resourceRequests.values();
    for (ResourceRequest *r : resourceRequests) {
        removeResourceRequest(r);
    }
}

Task *ResourceRequestCollection::task() const
{
    return m_task;
}

void ResourceRequestCollection::setTask(Task *t)
{
    m_task = t;
}

void ResourceRequestCollection::addResourceRequest(ResourceRequest *request)
{
    int id = request->id();
    m_lastResourceId = std::max(m_lastResourceId, id);
    Q_ASSERT(!m_resourceRequests.contains(id));
    if (id == 0) {
        while (m_resourceRequests.contains(++m_lastResourceId)) {
        }
        request->setId(m_lastResourceId);
    }
    Q_ASSERT(!m_resourceRequests.contains(request->id()));
    request->setCollection(this);
    request->registerRequest();
    m_resourceRequests.insert(request->id(), request);
    if (m_task) {
        m_task->changed(Node::ResourceRequestProperty);
    }
}

void ResourceRequestCollection::removeResourceRequest(ResourceRequest *request)
{
    Q_ASSERT(m_resourceRequests.contains(request->id()));
    Q_ASSERT(m_resourceRequests.values().contains(request)); // clazy:exclude=container-anti-pattern
    m_resourceRequests.remove(request->id());
    request->unregisterRequest();
    Q_ASSERT(!m_resourceRequests.values().contains(request)); // clazy:exclude=container-anti-pattern
    if (m_task) {
        m_task->changed(Node::ResourceRequestProperty);
    }
}

void ResourceRequestCollection::deleteResourceRequest(ResourceRequest *request)
{
    delete request;
}

ResourceRequest *ResourceRequestCollection::find(const Resource *resource) const
{
    for (ResourceRequest *r : m_resourceRequests) {
        if (r->resource() == resource) {
            return r;
        }
        const auto alts = r->alternativeRequests();
        for (const auto rr : alts) {
            if (rr->resource() == resource) {
                return rr;
            }
        }
    }
    return nullptr;
}

ResourceRequest *ResourceRequestCollection::resourceRequest(int id) const
{
    return m_resourceRequests.value(id);
}

ResourceRequest *ResourceRequestCollection::resourceRequest(const QString &name) const
{
    for (ResourceRequest *r : m_resourceRequests) {
        if (r->resource()->name() == name) {
            return r;
        }
    }
    return nullptr;
}

QStringList ResourceRequestCollection::requestNameList() const
{
    QStringList lst;
    for (ResourceRequest *r : m_resourceRequests) {
        lst << r->resource()->name();
    }
    return lst;
}

QList<Resource*> ResourceRequestCollection::requestedResources() const {
    QList<Resource*> lst;
    for (ResourceRequest *r : m_resourceRequests) {
        lst += r->resource();
    }
    return lst;
}

QList<ResourceRequest*> ResourceRequestCollection::resourceRequests(bool resolveTeam) const
{
    QList<ResourceRequest*> lst = m_resourceRequests.values();
    if (resolveTeam) {
        for (ResourceRequest *r : m_resourceRequests) {
            lst += r->teamMembers();
        }
    }
    return lst;
}


bool ResourceRequestCollection::contains(const QString &identity) const
{
    QStringList lst = requestNameList();
    QRegularExpression pattern(QRegularExpression::escape(identity));
    return lst.indexOf(QRegularExpression(pattern)) != -1;
}

// bool ResourceRequestCollection::load(KoXmlElement &element, Project &project) {
//     //debugPlan;
//     return true;
// }

void ResourceRequestCollection::save(QDomElement &element) const {
    //debugPlan;
    for (ResourceRequest *r : m_resourceRequests) {
        r->save(element);
    }
}

// Returns the duration needed by the working resources
// "Material type" of resourcegroups does not (atm) affect the duration.
Duration ResourceRequestCollection::duration(const DateTime &time, const Duration &effort, Schedule *ns, bool backward) {
    //debugPlan<<"time="<<time.toString()<<" effort="<<effort.toString(Duration::Format_Day)<<" backward="<<backward;
    if (isEmpty()) {
        return effort;
    }
    initUsedResourceRequests(time, ns, backward);
    Duration dur = effort;
    QList<ResourceRequest*> lst;
    for (const auto rr : std::as_const(m_usedResourceRequests)) {
        if (rr->resource()->type() != Resource::Type_Material) {
            lst << rr;
        }
    }
    if (!lst.isEmpty()) {
        dur = duration(lst, time, effort, ns, backward);
    }
    return dur;
}

DateTime ResourceRequestCollection::workTimeAfter(const DateTime &time, Schedule *ns) const
{
    DateTime start;
    const auto requests = initUsedResourceRequests(time, ns, false);
    for (ResourceRequest *r : requests) {
        DateTime t = r->workTimeAfter(time, ns);
        if (t.isValid() && (!start.isValid() || t < start))
            start = t;
    }
    if (start.isValid() && start < time) {
        start = time;
    }
    //debugPlan<<time.toString()<<"="<<start.toString();
    const_cast<ResourceRequestCollection*>(this)->reset();
    return start;
}

DateTime ResourceRequestCollection::workTimeBefore(const DateTime &time, Schedule *ns) const
{
    DateTime end;
    const auto requests = initUsedResourceRequests(time, ns, true);
    for (ResourceRequest *r : requests) {
        DateTime t = r->workTimeBefore(time, ns);
        if (t.isValid() && (!end.isValid() ||t > end))
            end = t;
    }
    if (!end.isValid() || end > time) {
        end = time;
    }
    const_cast<ResourceRequestCollection*>(this)->reset();
    return end;
}

DateTime ResourceRequestCollection::availableAfter(const DateTime &time, Schedule *ns)
{
    DateTime start;
    const auto requests = initUsedResourceRequests(time, ns, false);
    for (ResourceRequest *r : requests) {
        DateTime t = r->availableAfter(time, ns);
        if (t.isValid() && (!start.isValid() || t < start))
            start = t;
    }
    if (start.isValid() && start < time) {
        start = time;
    }
    //debugPlan<<time.toString()<<"="<<start.toString();
    const_cast<ResourceRequestCollection*>(this)->reset();
    return start;
}

DateTime ResourceRequestCollection::availableBefore(const DateTime &time, Schedule *ns)
{
    DateTime end;
    const auto requests = initUsedResourceRequests(time, ns, true);
    for (ResourceRequest *r : requests) {
        DateTime t = r->availableBefore(time, ns);
        if (t.isValid() && (!end.isValid() ||t > end))
            end = t;
    }
    if (!end.isValid() || end > time) {
        end = time;
    }
    const_cast<ResourceRequestCollection*>(this)->reset();
    return end;
}

DateTime ResourceRequestCollection::workStartAfter(const DateTime &time, Schedule *ns)
{
    DateTime start;
    const auto requests = initUsedResourceRequests(time, ns, false);
    for (ResourceRequest *r : requests) {
        if (r->resource()->type() != Resource::Type_Work) {
            continue;
        }
        DateTime t = r->availableAfter(time, ns);
        if (t.isValid() && (!start.isValid() || t < start))
            start = t;
    }
    if (start.isValid() && start < time) {
        start = time;
    }
    //debugPlan<<time.toString()<<"="<<start.toString();
    const_cast<ResourceRequestCollection*>(this)->reset();
    return start;
}

DateTime ResourceRequestCollection::workFinishBefore(const DateTime &time, Schedule *ns)
{
    DateTime end;
    const auto requests = initUsedResourceRequests(time, ns, true);
    for (ResourceRequest *r : requests) {
        if (r->resource()->type() != Resource::Type_Work) {
            continue;
        }
        DateTime t = r->availableBefore(time, ns);
        if (t.isValid() && (!end.isValid() ||t > end))
            end = t;
    }
    if (!end.isValid() || end > time) {
        end = time;
    }
    const_cast<ResourceRequestCollection*>(this)->reset();
    return end;
}


void ResourceRequestCollection::makeAppointments(Schedule *schedule)
{
    //debugPlan;
    // TODO: ALAP ?
    const auto requests = initUsedResourceRequests(schedule->startTime, schedule, false);
    for (ResourceRequest *r : std::as_const(requests)) {
        r->makeAppointment(schedule);
    }
}

void ResourceRequestCollection::reserve(const DateTime &start, const Duration &duration)
{
    Q_UNUSED(start)
    Q_UNUSED(duration)
    //debugPlan;
    // TODO: Alternatives
//     for (ResourceRequest *r : std::as_const(m_resourceRequests)) {
//         r->reserve(start, duration); //FIXME
//     }
}

bool ResourceRequestCollection::isEmpty() const {
    return m_resourceRequests.isEmpty();
}

void ResourceRequestCollection::changed()
{
    //debugPlan<<m_task;
    if (m_task) {
        m_task->changed(Node::ResourceRequestProperty);
    }
}

QList<ResourceRequest*> ResourceRequestCollection::initUsedResourceRequests(const DateTime &time, Schedule *ns, bool backward) const
{
    if (ns && !m_usedResourceRequests.isEmpty()) {
        return m_usedResourceRequests;
    }
    QList<ResourceRequest*> requests;
    if (ns && ns->parent() && ns->parent()->manager() && m_task->isStarted()) {
        // Try to reuse the same resources to avoid switching resources in the middle of a task
        // TODO: Make it configurable?
        const auto resources = m_task->usedResources(ns);
        for (const auto r : resources) {
            auto request = find(r);
            if (request) {
                requests << request;
            }
        }
        if (requests.isEmpty()) {
            ns->logWarning(i18n("Re-scheduling but no used resources was found"));
        }
    }
    if (requests.isEmpty()) {
        for (auto rr : std::as_const(m_resourceRequests)) {
            const auto dt = backward ? rr->availableBefore(time, ns) : rr->availableAfter(time, ns);
            if (!dt.isValid()) {
                continue;
            }
            auto request = rr;
            qint64 best = std::abs(dt.toMSecsSinceEpoch() - time.toMSecsSinceEpoch());
            const auto alts = rr->alternativeRequests();
            for (auto ar : alts) {
                if (requests.contains(ar)) {
                    continue;
                }
                const auto dt = backward ? ar->availableBefore(time, ns) : ar->availableAfter(time, ns);
                if (!dt.isValid()) {
                    continue;
                }
                auto best2 = std::abs(dt.toMSecsSinceEpoch() - time.toMSecsSinceEpoch());
                if (best > best2) {
                    best = best2;
                    request = ar;
                }
            }
            requests.append(request);
        }
    }
    if (ns) {
        const_cast<ResourceRequestCollection*>(this)->m_usedResourceRequests = requests;

        QStringList resources;
        for (const auto rr : m_usedResourceRequests) {
            resources << rr->resource()->name();
        }
        if (resources.isEmpty()) {
            resources << QLatin1String("None");
        }
        ns->logDebug(QStringLiteral("Schedule with resources: %1").arg(resources.join(QLatin1String(", "))));
    }
    return requests;
}

void ResourceRequestCollection::reset()
{
    m_usedResourceRequests.clear();
}

Duration ResourceRequestCollection::effort(const QList<ResourceRequest*> &lst, const DateTime &time, const Duration &duration, Schedule *ns, bool backward) const {
    Duration e;
    for (ResourceRequest *r : lst) {
        e += r->effort(time, duration, ns, backward);
        //debugPlan<<(backward?"(B)":"(F)")<<r<<": time="<<time<<" dur="<<duration.toString()<<"gave e="<<e.toString();
    }
    //debugPlan<<time.toString()<<"d="<<duration.toString()<<": e="<<e.toString();
    return e;
}

int ResourceRequestCollection::numDays(const QList<ResourceRequest*> &lst, const DateTime &time, bool backward) const {
    DateTime t1, t2 = time;
    if (backward) {
        for (ResourceRequest *r : lst) {
            t1 = r->availableFrom();
            if (!t2.isValid() || t2 > t1)
                t2 = t1;
        }
        //debugPlan<<"bw"<<time.toString()<<":"<<t2.daysTo(time);
        return t2.daysTo(time);
    }
    for (ResourceRequest *r : lst) {
        t1 = r->availableUntil();
        if (!t2.isValid() || t2 < t1)
            t2 = t1;
    }
    //debugPlan<<"fw"<<time.toString()<<":"<<time.daysTo(t2);
    return time.daysTo(t2);
}

ulong ResourceRequestCollection::granularity() const
{
    return m_task ? m_task->granularity() : 60000;
}

bool ResourceRequestCollection::accepted(const Duration &estimate, const Duration &result, Schedule *ns) const
{
    const auto deviation = estimate.milliseconds() - result.milliseconds();
    if (ns) {
        ns->logDebug(QStringLiteral("Deviation: %1").arg(KFormat().formatDuration(deviation, KFormat::FoldHours)));
    }
    if (deviation == 0) {
        return true;
    }
    const qlonglong limit = granularity();
    if (std::abs(deviation) <= limit) {
#if 0
        if (ns) {
            ns->logWarning(i18n("Deviation match: granularity: %1, deviation: %2",
                                Duration(limit).toString(Duration::Format_i18nDay),
                                Duration(deviation).toString(Duration::Format_i18nDay)));
        }
#endif
        return true;
    }
    return false;
}

Duration ResourceRequestCollection::duration(const QList<ResourceRequest*> &lst, const DateTime &time, const Duration &_effort, Schedule *ns, bool backward) {
    //debugPlan<<"--->"<<(backward?"(B)":"(F)")<<time.toString()<<": effort:"<<_effort.toString(Duration::Format_Day)<<" ("<<_effort.milliseconds()<<")";
#if 0
    if (ns) {
        QStringList nl;
        foreach (ResourceRequest *r, lst) { nl << r->resource()->name(); }
        ns->logDebug(QStringLiteral("Match effort:" + time.toString() + "," + _effort.toString());
        ns->logDebug(QStringLiteral("Resources: " + (nl.isEmpty() ? QString("None") : nl.join(", ")));
    }
#endif
    QLocale locale;
    Duration e;
    if (_effort == Duration::zeroDuration) {
        return e;
    }
    DateTime logtime = time;
    bool match = false;
    DateTime start = time;
    int inc = backward ? -1 : 1;
    DateTime end = start;
    Duration e1;
    int nDays = numDays(lst, time, backward) + 1;
    int day = 0;
    for (day=0; !match && day <= nDays; ++day) {
        // days
        end = end.addDays(inc);
        e1 = effort(lst, start, backward ? start - end : end - start, ns, backward);
        //debugPlan<<"["<<i<<"of"<<nDays<<"]"<<(backward?"(B)":"(F):")<<"  start="<<start<<" e+e1="<<(e+e1).toString()<<" match"<<_effort.toString();
        if (e + e1 < _effort) {
            e += e1;
            start = end;
        } else if (e + e1 == _effort) {
            e += e1;
            match = true;
        } else {
            end = start;
            break;
        }
    }
    match = accepted(_effort, e, ns);
    if (ns) ns->logDebug(QStringLiteral("days done: %1").arg(match?QStringLiteral("match"):QStringLiteral("nomatch")));
    if (! match && day <= nDays) {
#ifndef PLAN_NLOGDEBUG
        if (ns) ns->logDebug(QStringLiteral("Days: duration ") + logtime.toString() + QStringLiteral(" - ") + end.toString() + QStringLiteral(" e=") + e.toString() + QStringLiteral(" (") + (_effort - e).toString() + QLatin1Char(')'));
#endif
        logtime = start;
        for (int i=0; !match && i < 24; ++i) {
            // hours
            end = end.addSecs(inc*60*60);
            e1 = effort(lst, start, backward ? start - end : end - start, ns, backward);
            if (e + e1 < _effort) {
                e += e1;
                start = end;
            } else if (e + e1 == _effort) {
                e += e1;
                match = true;
            } else {
                if (false/*roundToHour*/ && (_effort - e) <  (e + e1 - _effort)) {
                    end = start;
                    match = true;
                } else {
                    end = start;
                }
                break;
            }
            //debugPlan<<"duration(h)["<<i<<"]"<<(backward?"backward":"forward:")<<" time="<<start.time()<<" e="<<e.toString()<<" ("<<e.milliseconds()<<")";
        }
        //debugPlan<<"duration"<<(backward?"backward":"forward:")<<start.toString()<<" e="<<e.toString()<<" ("<<e.milliseconds()<<")  match="<<match<<" sts="<<sts;
        match = accepted(_effort, e, ns);
        if (ns) ns->logDebug(QStringLiteral("hours done: %1").arg(match?QStringLiteral("match"):QStringLiteral("nomatch")));
    }
    if (! match && day <= nDays) {
#ifndef PLAN_NLOGDEBUG
        if (ns) ns->logDebug(QStringLiteral("Hours: duration ") + logtime.toString() + QStringLiteral(" - ") + end.toString() + QStringLiteral(" e=") + e.toString() + QStringLiteral(" (") + (_effort - e).toString() + QLatin1Char(')'));
#endif
        logtime = start;
        for (int i=0; !match && i < 60; ++i) {
            //minutes
            end = end.addSecs(inc*60);
            e1 = effort(lst, start, backward ? start - end : end - start, ns, backward);
            if (e + e1 < _effort) {
                e += e1;
                start = end;
            } else if (e + e1 == _effort) {
                e += e1;
                match = true;
            } else if (e + e1 > _effort) {
                end = start;
                break;
            }
            //debugPlan<<"duration(m)"<<(backward?"backward":"forward:")<<"  time="<<start.time().toString()<<" e="<<e.toString()<<" ("<<e.milliseconds()<<")";
        }
        //debugPlan<<"duration"<<(backward?"backward":"forward:")<<"  start="<<start.toString()<<" e="<<e.toString()<<" match="<<match<<" sts="<<sts;
        match = accepted(_effort, e, ns);
        if (ns) ns->logDebug(QStringLiteral("minutes done: %1").arg(match?QStringLiteral("match"):QStringLiteral("nomatch")));
    }
    if (! match && day <= nDays) {
#ifndef PLAN_NLOGDEBUG
        if (ns) ns->logDebug(QStringLiteral("Minutes: duration ") + logtime.toString() + QStringLiteral(" - ") + end.toString() + QStringLiteral(" e=") + e.toString() + QStringLiteral(" (") + (_effort - e).toString() + QLatin1Char(')'));
#endif
        logtime = start;
        for (int i=0; !match && i < 60; ++i) {
            //seconds
            end = end.addSecs(inc);
            e1 = effort(lst, start, backward ? start - end : end - start, ns, backward);
            if (e + e1 < _effort) {
                e += e1;
                start = end;
            } else if (e + e1 == _effort) {
                e += e1;
                match = true;
            } else if (e + e1 > _effort) {
                end = start;
                break;
            }
            //debugPlan<<"duration(s)["<<i<<"]"<<(backward?"backward":"forward:")<<" time="<<start.time().toString()<<" e="<<e.toString()<<" ("<<e.milliseconds()<<")";
        }
        match = accepted(_effort, e, ns);
        if (ns) ns->logDebug(QStringLiteral("seconds done: %1").arg(match?QStringLiteral("match"):QStringLiteral("nomatch")));
    }
    if (! match && day <= nDays) {
#ifndef PLAN_NLOGDEBUG
        if (ns) ns->logDebug(QStringLiteral("Seconds: duration ") + logtime.toString() + QStringLiteral(" - ") + end.toString() + QStringLiteral(" e=") + e.toString() + QStringLiteral(" (") + (_effort - e).toString() +QLatin1Char(')'));
#endif
        for (int i=0; !match && i < 1000; ++i) {
            //milliseconds
            end.setTime(end.time().addMSecs(inc));
            e1 = effort(lst, start, backward ? start - end : end - start, ns, backward);
            if (e + e1 < _effort) {
                e += e1;
                start = end;
            } else if (e + e1 == _effort) {
                e += e1;
                match = true;
            } else if (e + e1 > _effort) {
                break;
            }
            //debugPlan<<"duration(ms)["<<i<<"]"<<(backward?"backward":"forward:")<<" time="<<start.time().toString()<<" e="<<e.toString()<<" ("<<e.milliseconds()<<")";
        }
        if (ns) ns->logDebug(QStringLiteral("milliseconds done: %1").arg(match?QStringLiteral("match"):QStringLiteral("nomatch")));
        match = accepted(_effort, e, ns);
    }
    if (!match && ns) {
        ns->setResourceError(true);
        ns->logError(i18n("Could not match effort. Want: %1 got: %2 (%3)", _effort.toString(Duration::Format_Hour), e.toString(Duration::Format_Hour), e.milliseconds()));
        for (ResourceRequest *r : lst) {
            Resource *res = r->resource();
            ns->logInfo(i18n("Resource %1 available from %2 to %3", res->name(), locale.toString(r->availableFrom(), QLocale::ShortFormat), locale.toString(r->availableUntil(), QLocale::ShortFormat)));
        }

    }
    DateTime t;
    if (e != Duration::zeroDuration) {
        for (ResourceRequest *r : lst) {
            DateTime tt;
            if (backward) {
                tt = r->availableAfter(end, ns);
                if (tt.isValid() && (! t.isValid() || tt < t)) {
                    t = tt;
                }
            } else {
                tt = r->availableBefore(end, ns);
                if (tt.isValid() && (! t.isValid() || tt > t)) {
                    t = tt;
                }
            }
        }
    }
    end = t.isValid() ? t : time;
    //debugPlan<<"<---"<<(backward?"(B)":"(F)")<<":"<<end.toString()<<"-"<<time.toString()<<"="<<(end - time).toString()<<" effort:"<<_effort.toString(Duration::Format_Day);
    return (end>time?end-time:time-end);
}

QDebug operator<<(QDebug dbg, const KPlato::ResourceRequest *rr)
{
    if (rr) {
        dbg<<*rr;
    } else {
        dbg<<(void*)rr;
    }
    return dbg;
}
QDebug operator<<(QDebug dbg, const KPlato::ResourceRequest &rr)
{
    if (rr.resource()) {
        dbg<<"ResourceRequest["<<rr.id()<<':'<<rr.resource()->name()<<QStringLiteral("%1%").arg(rr.units())<<']';
    } else {
        dbg<<"ResourceRequest["<<rr.id()<<':'<<"No resource]";
    }
    return dbg;
}
