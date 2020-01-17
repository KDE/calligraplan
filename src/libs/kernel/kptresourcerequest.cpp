/* This file is part of the KDE project
 * Copyright (C) 2001 Thomas Zander zander@kde.org
 * Copyright (C) 2004-2007 Dag Andersen <danders@get2net.dk>
 * Copyright (C) 2011 Dag Andersen <danders@get2net.dk>
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

#include <QLocale>
#include <QDomElement>

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
    m_resource = 0;
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

bool ResourceRequest::load(KoXmlElement &element, Project &project) {
    debugPlanXml<<this;
    m_resource = project.resource(element.attribute("resource-id"));
    if (m_resource == nullptr) {
        warnPlan<<"The referenced resource does not exist: resource id="<<element.attribute("resource-id");
        return false;
    }
    m_units  = element.attribute("units").toInt();

    KoXmlElement parent = element.namedItem("required-resources").toElement();
    KoXmlElement e;
    forEachElement(e, parent) {
        if (e.nodeName() == "resource") {
            QString id = e.attribute("id");
            if (id.isEmpty()) {
                errorPlan<<"Missing project id";
                continue;
            }
            Resource *r = project.resource(id);
            if (r == 0) {
                warnPlan<<"The referenced resource does not exist: resource id="<<element.attribute("resource-id");
            } else {
                if (r != m_resource) {
                    m_required << r;
                    debugPlanXml<<"added reqired"<<r;
                }
            }
        }
    }
    return true;
}

void ResourceRequest::save(QDomElement &element) const {
    QDomElement me = element.ownerDocument().createElement("resource-request");
    element.appendChild(me);
    me.setAttribute("resource-id", m_resource->id());
    me.setAttribute("units", QString::number(m_units));
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
    return m_collection ? m_collection->task() : 0;
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
        foreach (Resource *member, resource->teamMembers()) {
            member->setCurrentSchedulePtr(resourceSchedule(ns, member));
        }
    }
    foreach (Resource *r, m_required) {
        r->setCurrentSchedulePtr(resourceSchedule(ns, r));
    }
}

Schedule *ResourceRequest::resourceSchedule(Schedule *ns, Resource *res)
{
    if (ns == 0) {
        return 0;
    }
    Resource *r = res == 0 ? resource() : res;
    Schedule *s = r->findSchedule(ns->id());
    if (s == 0) {
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
        foreach (Resource *r, m_required) {
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
        foreach (Resource *r, m_required) {
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
        foreach (Resource *r, m_resource->teamMembers()) {
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
        foreach (Resource *r, m_resource->teamMembers()) {
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
        for (ResourceRequest *rr : teamMembers()) {
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
        foreach (Resource *r, m_resource->teamMembers()) {
            m_teamMembers << new ResourceRequest(r, m_units);
        }
    }
    return m_teamMembers;
}

QList<ResourceRequest*> ResourceRequest::alternativeRequests() const
{
    return m_alternativeRequests;
}

void ResourceRequest::setAlternativeRequests(const QList<ResourceRequest*> requests)
{
    for (ResourceRequest *r : m_alternativeRequests) {
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
        emit m_collection->alternativeRequestToBeAdded(request, row);
    }
}

void ResourceRequest::emitAlternativeRequestAdded(ResourceRequest *alternative)
{
    if (m_collection) {
        emit m_collection->alternativeRequestAdded(alternative);
    }
}

void ResourceRequest::emitAlternativeRequestToBeRemoved(ResourceRequest *request, int row, ResourceRequest *alternative)
{
    if (m_collection) {
        emit m_collection->alternativeRequestToBeRemoved(request, row, alternative);
    }
}

void ResourceRequest::emitAlternativeRequestRemoved()
{
    if (m_collection) {
        emit m_collection->alternativeRequestRemoved();
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
    for (ResourceRequest *r : m_resourceRequests) {
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
    Q_ASSERT(m_resourceRequests.values().contains(request));
    m_resourceRequests.remove(request->id());
    request->unregisterRequest();
    Q_ASSERT(!m_resourceRequests.values().contains(request));
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
    return lst.indexOf(QRegExp(identity, Qt::CaseSensitive, QRegExp::FixedString)) != -1;
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
    Duration dur = effort;
    // TODO: alternatives
    QList<ResourceRequest*> lst;
    QMap<int, ResourceRequest*>::const_iterator it; 
    for (it = m_resourceRequests.constBegin(); it != m_resourceRequests.constEnd(); ++it) {
        if (it.value()->resource()->type() != Resource::Type_Material) {
            lst << it.value();
        }
    }
    if (!lst.isEmpty()) {
        dur = duration(lst, time, effort, ns, backward);
    }
    return dur;
}

DateTime ResourceRequestCollection::workTimeAfter(const DateTime &time, Schedule *ns) const {
    DateTime start;
    // TODO: Alternatives
    for (ResourceRequest *r : m_resourceRequests) {
        DateTime t = r->workTimeAfter(time, ns);
        if (t.isValid() && (!start.isValid() || t < start))
            start = t;
    }
    if (start.isValid() && start < time)
        start = time;
    //debugPlan<<time.toString()<<"="<<start.toString();
    return start;
}

DateTime ResourceRequestCollection::workTimeBefore(const DateTime &time, Schedule *ns) const {
    DateTime end;
    // TODO: Alternatives
    for (ResourceRequest *r : m_resourceRequests) {
        DateTime t = r->workTimeBefore(time, ns);
        if (t.isValid() && (!end.isValid() ||t > end))
            end = t;
    }
    if (!end.isValid() || end > time)
        end = time;
    return end;
}

DateTime ResourceRequestCollection::availableAfter(const DateTime &time, Schedule *ns) {
    DateTime start;
    // TODO: Alternatives
    for (ResourceRequest *r : m_resourceRequests) {
        DateTime t = r->availableAfter(time, ns);
        if (t.isValid() && (!start.isValid() || t < start))
            start = t;
    }
    if (start.isValid() && start < time)
        start = time;
    //debugPlan<<time.toString()<<"="<<start.toString();
    return start;
}

DateTime ResourceRequestCollection::availableBefore(const DateTime &time, Schedule *ns) {
    DateTime end;
    // TODO: Alternatives
    for (ResourceRequest *r : m_resourceRequests) {
        DateTime t = r->availableBefore(time, ns);
        if (t.isValid() && (!end.isValid() ||t > end))
            end = t;
    }
    if (!end.isValid() || end > time)
        end = time;
    return end;
}

DateTime ResourceRequestCollection::workStartAfter(const DateTime &time, Schedule *ns) {
    DateTime start;
    // TODO: Alternatives
    for (ResourceRequest *r : m_resourceRequests) {
        if (r->resource()->type() != Resource::Type_Work) {
            continue;
        }
        DateTime t = r->availableAfter(time, ns);
        if (t.isValid() && (!start.isValid() || t < start))
            start = t;
    }
    if (start.isValid() && start < time)
        start = time;
    //debugPlan<<time.toString()<<"="<<start.toString();
    return start;
}

DateTime ResourceRequestCollection::workFinishBefore(const DateTime &time, Schedule *ns) {
    DateTime end;
    // TODO: Alternatives
    for (ResourceRequest *r : m_resourceRequests) {
        if (r->resource()->type() != Resource::Type_Work) {
            continue;
        }
        DateTime t = r->availableBefore(time, ns);
        if (t.isValid() && (!end.isValid() ||t > end))
            end = t;
    }
    if (!end.isValid() || end > time)
        end = time;
    return end;
}


void ResourceRequestCollection::makeAppointments(Schedule *schedule) {
    //debugPlan;
    // TODO: Alternatives
    for (ResourceRequest *r : m_resourceRequests) {
        r->makeAppointment(schedule);
    }
}

void ResourceRequestCollection::reserve(const DateTime &start, const Duration &duration) {
    //debugPlan;
    // TODO: Alternatives
    for (ResourceRequest *r : m_resourceRequests) {
//         r->reserve(start, duration); //FIXME
    }
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

void ResourceRequestCollection::resetDynamicAllocations()
{
    //TODO
}

Duration ResourceRequestCollection::effort(const QList<ResourceRequest*> &lst, const DateTime &time, const Duration &duration, Schedule *ns, bool backward) const {
    Duration e;
    foreach (ResourceRequest *r, lst) {
        e += r->effort(time, duration, ns, backward);
        //debugPlan<<(backward?"(B)":"(F)")<<r<<": time="<<time<<" dur="<<duration.toString()<<"gave e="<<e.toString();
    }
    //debugPlan<<time.toString()<<"d="<<duration.toString()<<": e="<<e.toString();
    return e;
}

int ResourceRequestCollection::numDays(const QList<ResourceRequest*> &lst, const DateTime &time, bool backward) const {
    DateTime t1, t2 = time;
    if (backward) {
        foreach (ResourceRequest *r, lst) {
            t1 = r->availableFrom();
            if (!t2.isValid() || t2 > t1)
                t2 = t1;
        }
        //debugPlan<<"bw"<<time.toString()<<":"<<t2.daysTo(time);
        return t2.daysTo(time);
    }
    foreach (ResourceRequest *r, lst) {
        t1 = r->availableUntil();
        if (!t2.isValid() || t2 < t1)
            t2 = t1;
    }
    //debugPlan<<"fw"<<time.toString()<<":"<<time.daysTo(t2);
    return time.daysTo(t2);
}

Duration ResourceRequestCollection::duration(const QList<ResourceRequest*> &lst, const DateTime &time, const Duration &_effort, Schedule *ns, bool backward) {
    //debugPlan<<"--->"<<(backward?"(B)":"(F)")<<time.toString()<<": effort:"<<_effort.toString(Duration::Format_Day)<<" ("<<_effort.milliseconds()<<")";
#if 0
    if (ns) {
        QStringList nl;
        foreach (ResourceRequest *r, lst) { nl << r->resource()->name(); }
        ns->logDebug("Match effort:" + time.toString() + "," + _effort.toString());
        ns->logDebug("Resources: " + (nl.isEmpty() ? QString("None") : nl.join(", ")));
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
    if (! match && day <= nDays) {
#ifndef PLAN_NLOGDEBUG
        if (ns) ns->logDebug("Days: duration " + logtime.toString() + " - " + end.toString() + " e=" + e.toString() + " (" + (_effort - e).toString() + ')');
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
    }
    if (! match && day <= nDays) {
#ifndef PLAN_NLOGDEBUG
        if (ns) ns->logDebug("Hours: duration " + logtime.toString() + " - " + end.toString() + " e=" + e.toString() + " (" + (_effort - e).toString() + ')');
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
    }
    // FIXME: better solution
    // If effort to match is reasonably large, accept a match if deviation <= 1 min
    if (! match && _effort > 5 * 60000) {
        if ((_effort - e) <= 60000){
            match = true;
#ifndef PLAN_NLOGDEBUG
            if (ns) ns->logDebug("Deviation match:" + logtime.toString() + " - " + end.toString() + " e=" + e.toString() + " (" + (_effort - e).toString() + ')');
#endif
        }
    }
    if (! match && day <= nDays) {
#ifndef PLAN_NLOGDEBUG
        if (ns) ns->logDebug("Minutes: duration " + logtime.toString() + " - " + end.toString() + " e=" + e.toString() + " (" + (_effort - e).toString() + ')');
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
    }
    if (! match && day <= nDays) {
#ifndef PLAN_NLOGDEBUG
        if (ns) ns->logDebug("Seconds: duration " + logtime.toString() + " - " + end.toString() + " e=" + e.toString() + " (" + (_effort - e).toString() + ')');
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
    }
    if (!match && ns) {
        ns->logError(i18n("Could not match effort. Want: %1 got: %2", _effort.toString(Duration::Format_Hour), e.toString(Duration::Format_Hour)));
        foreach (ResourceRequest *r, lst) {
            Resource *res = r->resource();
            ns->logInfo(i18n("Resource %1 available from %2 to %3", res->name(), locale.toString(r->availableFrom(), QLocale::ShortFormat), locale.toString(r->availableUntil(), QLocale::ShortFormat)));
        }

    }
    DateTime t;
    if (e != Duration::zeroDuration) {
        foreach (ResourceRequest *r, lst) {
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
        dbg<<"ResourceRequest["<<rr.id()<<':'<<rr.resource()->name()<<']';
    } else {
        dbg<<"ResourceRequest["<<rr.id()<<':'<<"No resource]";
    }
    return dbg;
}
