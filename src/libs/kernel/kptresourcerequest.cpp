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
      m_parent(nullptr),
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
      m_parent(nullptr),
      m_dynamic(r.m_dynamic),
      m_required(r.m_required)
{
}

ResourceRequest::~ResourceRequest() {
    if (m_resource) {
        m_resource->unregisterRequest(this);
    }
    m_resource = 0;
    if (m_parent) {
        m_parent->removeResourceRequest(this);
    }
    if (m_collection) {
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
    if (m_resource == 0) {
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
    return m_parent ? m_parent->task() : 0;
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
    setCurrentSchedulePtr(ns);
    Duration e = m_resource->effort(time, duration, m_units, backward, m_required);
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

/////////
ResourceGroupRequest::ResourceGroupRequest(ResourceGroup *group, int units)
    : m_id(0)
    , m_group(group)
    , m_units(units)
    , m_parent(nullptr)
{
    //debugPlan<<"Request to:"<<(group ? group->name() : QString("None"));
    if (group)
        group->registerRequest(this);
}

ResourceGroupRequest::ResourceGroupRequest(const ResourceGroupRequest &g)
    : m_id(g.m_id)
    , m_group(g.m_group)
    , m_units(g.m_units)
    , m_parent(0)
{
}

ResourceGroupRequest::~ResourceGroupRequest() {
    //debugPlan;
    if (m_group) {
        m_group->unregisterRequest(this);
    }
    for (ResourceRequest *r : m_resourceRequests) {
        r->setParent(nullptr);
    }
    if (m_parent) {
        m_parent->takeRequest(this);
    }
}

int ResourceGroupRequest::id() const
{
    return m_id;
}

void ResourceGroupRequest::setId(int id)
{
    m_id = id;
}

void ResourceGroupRequest::addResourceRequest(ResourceRequest *request) {
    //debugPlan<<"("<<request<<") to Group:"<<(void*)m_group;
    request->setParent(this);
    m_resourceRequests.append(request);
    changed();
}

ResourceRequest *ResourceGroupRequest::takeResourceRequest(ResourceRequest *request) {
    if (request)
        request->unregisterRequest();
    ResourceRequest *r = 0;
    int i = m_resourceRequests.indexOf(request);
    if (i != -1) {
        r = m_resourceRequests.takeAt(i);
    }
    changed();
    return r;
}

ResourceRequest *ResourceGroupRequest::find(const Resource *resource) const {
    foreach (ResourceRequest *gr, m_resourceRequests) {
        if (gr->resource() == resource) {
            return gr;
        }
    }
    return 0;
}

ResourceRequest *ResourceGroupRequest::resourceRequest(const QString &name) {
    foreach (ResourceRequest *r, m_resourceRequests) {
        if (r->resource()->name() == name)
            return r;
    }
    return 0;
}

QStringList ResourceGroupRequest::requestNameList(bool includeGroup) const {
    QStringList lst;
    if (includeGroup && m_units > 0 && m_group) {
        lst << m_group->name();
    }
    foreach (ResourceRequest *r, m_resourceRequests) {
        if (! r->isDynamicallyAllocated()) {
            Q_ASSERT(r->resource());
            lst << r->resource()->name();
        }
    }
    return lst;
}

QList<Resource*> ResourceGroupRequest::requestedResources() const
{
    QList<Resource*> lst;
    foreach (ResourceRequest *r, m_resourceRequests) {
        if (! r->isDynamicallyAllocated()) {
            Q_ASSERT(r->resource());
            lst << r->resource();
        }
    }
    return lst;
}

QList<ResourceRequest*> ResourceGroupRequest::resourceRequests(bool resolveTeam) const
{
    QList<ResourceRequest*> lst;
    foreach (ResourceRequest *rr, m_resourceRequests) {
        if (resolveTeam && rr->resource()->type() == Resource::Type_Team) {
            lst += rr->teamMembers();
        } else {
            lst << rr;
        }
    }
    return lst;
}

bool ResourceGroupRequest::load(KoXmlElement &element, XMLLoaderObject &status) {
    debugPlanXml<<this;
    m_group = status.project().findResourceGroup(element.attribute("group-id"));
    if (m_group == 0) {
        errorPlan<<"The referenced resource group does not exist: group id="<<element.attribute("group-id");
        return false;
    }
    m_group->registerRequest(this);

    KoXmlNode n = element.firstChild();
    for (; ! n.isNull(); n = n.nextSibling()) {
        if (! n.isElement()) {
            continue;
        }
        KoXmlElement e = n.toElement();
        if (e.tagName() == "resource-request") {
            ResourceRequest *r = new ResourceRequest();
            if (r->load(e, status.project()))
                addResourceRequest(r);
            else {
                errorPlan<<"Failed to load resource request";
                delete r;
            }
        }
    }
    // meaning of m_units changed
    // Pre 0.6.6 the number *included* all requests, now it is in *addition* to resource requests
    m_units  = element.attribute("units").toInt();
    if (status.version() < "0.6.6") {
        int x = m_units - m_resourceRequests.count();
        m_units = x > 0 ? x : 0;
    }
    return true;
}

void ResourceGroupRequest::save(QDomElement &element) const {
    QDomElement me = element.ownerDocument().createElement("resourcegroup-request");
    element.appendChild(me);
    me.setAttribute("group-id", m_group->id());
    me.setAttribute("units", QString::number(m_units));
}

int ResourceGroupRequest::units() const {
    return m_units;
}

Duration ResourceGroupRequest::duration(const DateTime &time, const Duration &_effort, Schedule *ns, bool backward) {
    Duration dur;
    if (m_parent) {
        dur = m_parent->duration(m_resourceRequests, time, _effort, ns, backward);
    }
    return dur;
}

DateTime ResourceGroupRequest::workTimeAfter(const DateTime &time, Schedule *ns) {
    DateTime start;
    if (m_resourceRequests.isEmpty()) {
        return start;
    }
    foreach (ResourceRequest *r, m_resourceRequests) {
        DateTime t = r->workTimeAfter(time, ns);
        if (t.isValid() && (!start.isValid() || t < start))
            start = t;
    }
    if (start.isValid() && start < time)
        start = time;
    //debugPlan<<time.toString()<<"="<<start.toString();
    return start;
}

DateTime ResourceGroupRequest::workTimeBefore(const DateTime &time, Schedule *ns) {
    DateTime end;
    if (m_resourceRequests.isEmpty()) {
        return end;
    }
    foreach (ResourceRequest *r, m_resourceRequests) {
        DateTime t = r->workTimeBefore(time, ns);
        if (t.isValid() && (!end.isValid() ||t > end))
            end = t;
    }
    if (!end.isValid() || end > time)
        end = time;
    return end;
}

DateTime ResourceGroupRequest::availableAfter(const DateTime &time, Schedule *ns) {
    DateTime start;
    if (m_resourceRequests.isEmpty()) {
        return start;
    }
    foreach (ResourceRequest *r, m_resourceRequests) {
        DateTime t = r->availableAfter(time, ns);
        if (t.isValid() && (!start.isValid() || t < start))
            start = t;
    }
    if (start.isValid() && start < time)
        start = time;
    //debugPlan<<time.toString()<<"="<<start.toString()<<""<<m_group->name();
    return start;
}

DateTime ResourceGroupRequest::availableBefore(const DateTime &time, Schedule *ns) {
    DateTime end;
    if (m_resourceRequests.isEmpty()) {
        return end;
    }
    foreach (ResourceRequest *r, m_resourceRequests) {
        DateTime t = r->availableBefore(time, ns);
        if (t.isValid() && (!end.isValid() || t > end))
            end = t;
    }
    if (!end.isValid() || end > time)
        end = time;
    //debugPlan<<time.toString()<<"="<<end.toString()<<""<<m_group->name();
    return end;
}

void ResourceGroupRequest::makeAppointments(Schedule *schedule) {
    //debugPlan;
    foreach (ResourceRequest *r, m_resourceRequests) {
        r->makeAppointment(schedule);
    }
}

void ResourceGroupRequest::reserve(const DateTime &start, const Duration &duration) {
    m_start = start;
    m_duration = duration;
}

bool ResourceGroupRequest::isEmpty() const {
    return m_resourceRequests.isEmpty() && m_units == 0;
}

Task *ResourceGroupRequest::task() const {
    return m_parent ? m_parent->task() : 0;
}

void ResourceGroupRequest::changed()
{
     if (m_parent) 
         m_parent->changed();
}

void ResourceGroupRequest::removeResourceRequest(ResourceRequest *request)
{
    m_resourceRequests.removeAll(request);
    changed();
}

void ResourceGroupRequest::deleteResourceRequest(ResourceRequest *request)
{
    int i = m_resourceRequests.indexOf(request);
    if (i != -1) {
        m_resourceRequests.removeAt(i);
    }
    changed();
}

void ResourceGroupRequest::resetDynamicAllocations()
{
    QList<ResourceRequest*> lst;
    foreach (ResourceRequest *r, m_resourceRequests) {
        if (r->isDynamicallyAllocated()) {
            lst << r;
        }
    }
    while (! lst.isEmpty()) {
        deleteResourceRequest(lst.takeFirst());
    }
}

void ResourceGroupRequest::allocateDynamicRequests(const DateTime &time, const Duration &effort, Schedule *ns, bool backward)
{
    int num = m_units;
    if (num <= 0) {
        return;
    }
    if (num == m_group->numResources()) {
        // TODO: allocate all
    }
    Duration e = effort / m_units;
    QMap<long, ResourceRequest*> map;
    foreach (Resource *r, m_group->resources()) {
        if (r->type() == Resource::Type_Team) {
            continue;
        }
        ResourceRequest *rr = find(r);
        if (rr) {
            if (rr->isDynamicallyAllocated()) {
                --num; // already allocated
            }
            continue;
        }
        rr = new ResourceRequest(r, r->units());
        long s = rr->allocationSuitability(time, e, ns, backward);
        if (s == 0) {
            // not suitable at all
            delete rr;
        } else {
            map.insertMulti(s, rr);
        }
    }
    for (--num; num >= 0 && ! map.isEmpty(); --num) {
        long key = map.lastKey();
        ResourceRequest *r = map.take(key);
        r->setAllocatedDynaically(true);
        addResourceRequest(r);
        debugPlan<<key<<r;
    }
    qDeleteAll(map); // delete the unused
}

/////////
ResourceRequestCollection::ResourceRequestCollection(Task *task)
    : m_task(task)
{
    //debugPlan<<this<<(void*)(&task);
}

ResourceRequestCollection::~ResourceRequestCollection()
{
    //debugPlan<<this;
    for (ResourceRequest *r : m_resourceRequests) {
        r->setCollection(nullptr);
    }
    for (ResourceGroupRequest *r : m_groupRequests) {
        r->setParent(nullptr);
    }
    qDeleteAll(m_resourceRequests); // removes themselves from possible group
    qDeleteAll(m_groupRequests);
}

void ResourceRequestCollection::removeRequests()
{
    const QList<ResourceRequest*> resourceRequests = m_resourceRequests.values();
    for (ResourceRequest *r : resourceRequests) {
        removeResourceRequest(r);
    }
    const QList<ResourceGroupRequest*> groupRequests = m_groupRequests.values();
    for (ResourceGroupRequest *r : groupRequests) {
        takeRequest(r);
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

void ResourceRequestCollection::addRequest(ResourceGroupRequest *request)
{
    Q_ASSERT(request->group());
    foreach (ResourceGroupRequest *r, m_groupRequests) {
        if (r->group() == request->group()) {
            errorPlan<<"Request to this group already exists";
            errorPlan<<"Task:"<<m_task->name()<<"Group:"<<request->group()->name();
            Q_ASSERT(false);
        }
    }
    int id = request->id();
    if (id == 0) {
        int i = 1;
        while (m_groupRequests.contains(i)) {
            ++i;
        }
        request->setId(i);
    }
    Q_ASSERT(!m_groupRequests.contains(request->id()));
    m_groupRequests.insert(request->id(), request);
    if (!request->group()->requests().contains(request)) {
        request->group()->registerRequest(request);
    }
    request->setParent(this);
    changed();
}

int ResourceRequestCollection::takeRequest(ResourceGroupRequest *request)
{
    Q_ASSERT(m_groupRequests.contains(request->id()));
    Q_ASSERT(m_groupRequests.value(request->id()) == request);
    int i = m_groupRequests.values().indexOf(request);
    m_groupRequests.remove(request->id());
    changed();
    return i;
}

ResourceGroupRequest *ResourceRequestCollection::groupRequest(int id) const
{
    return m_groupRequests.value(id);
}

ResourceGroupRequest *ResourceRequestCollection::find(const ResourceGroup *group) const {
    foreach (ResourceGroupRequest *r, m_groupRequests) {
        if (r->group() == group)
            return r; // we assume only one request to the same group
    }
    return 0;
}

void ResourceRequestCollection::addResourceRequest(ResourceRequest *request, ResourceGroupRequest *group)
{
    if (group) {
        Q_ASSERT(m_groupRequests.contains(group->id()));
    }
    int id = request->id();
    if (id == 0) {
        int i = 1;
        while (m_resourceRequests.contains(i)) {
            ++i;
        }
        request->setId(i);
    }
    Q_ASSERT(!m_resourceRequests.contains(request->id()));
    request->setCollection(this);
    request->registerRequest();
    m_resourceRequests.insert(request->id(), request);

    if (group && !group->resourceRequests().contains(request)) {
        group->addResourceRequest(request);
    }
}

void ResourceRequestCollection::removeResourceRequest(ResourceRequest *request)
{
    Q_ASSERT(m_resourceRequests.contains(request->id()));
    Q_ASSERT(m_resourceRequests.values().contains(request));
    m_resourceRequests.remove(request->id());
    Q_ASSERT(!m_resourceRequests.values().contains(request));
    if (request->parent()) {
        request->parent()->takeResourceRequest(request);
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

QStringList ResourceRequestCollection::requestNameList(bool includeGroup) const {
    QStringList lst;
    foreach (ResourceGroupRequest *r, m_groupRequests) {
        lst << r->requestNameList(includeGroup);
    }
    return lst;
}

QList<Resource*> ResourceRequestCollection::requestedResources() const {
    QList<Resource*> lst;
    foreach (ResourceGroupRequest *g, m_groupRequests) {
        lst += g->requestedResources();
    }
    return lst;
}

QList<ResourceRequest*> ResourceRequestCollection::resourceRequests(bool resolveTeam) const
{
    // FIXME: skip going through groups, probably separate method for resolveTeam case?
    if (!resolveTeam) {
        return m_resourceRequests.values();
    }
    QList<ResourceRequest*> lst;
    foreach (ResourceGroupRequest *g, m_groupRequests) {
        foreach (ResourceRequest *r, g->resourceRequests(resolveTeam)) {
            lst << r;
        }
    }
    return lst;
}


bool ResourceRequestCollection::contains(const QString &identity) const {
    QStringList lst = requestNameList();
    return lst.indexOf(QRegExp(identity, Qt::CaseSensitive, QRegExp::FixedString)) != -1;
}

ResourceGroupRequest *ResourceRequestCollection::findGroupRequestById(const QString &id) const
{
    foreach (ResourceGroupRequest *r, m_groupRequests) {
        if (r->group()->id() == id) {
            return r;
        }
    }
    return 0;
}

// bool ResourceRequestCollection::load(KoXmlElement &element, Project &project) {
//     //debugPlan;
//     return true;
// }

void ResourceRequestCollection::save(QDomElement &element) const {
    //debugPlan;
    foreach (ResourceGroupRequest *r, m_groupRequests) {
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
    QList<ResourceRequest*> lst;
    foreach (ResourceGroupRequest *r, m_groupRequests) {
        r->allocateDynamicRequests(time, effort, ns, backward);
        if (r->group()->type() == ResourceGroup::Type_Work) {
            lst << r->resourceRequests();
        } else if (r->group()->type() == ResourceGroup::Type_Material) {
            //TODO
        }
    }
    if (! lst.isEmpty()) {
        dur = duration(lst, time, effort, ns, backward);
    }
    return dur;
}

DateTime ResourceRequestCollection::workTimeAfter(const DateTime &time, Schedule *ns) const {
    DateTime start;
    foreach (ResourceGroupRequest *r, m_groupRequests) {
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
    foreach (ResourceGroupRequest *r, m_groupRequests) {
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
    foreach (ResourceGroupRequest *r, m_groupRequests) {
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
    foreach (ResourceGroupRequest *r, m_groupRequests) {
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
    foreach (ResourceGroupRequest *r, m_groupRequests) {
        if (r->group()->type() != ResourceGroup::Type_Work) {
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
    foreach (ResourceGroupRequest *r, m_groupRequests) {
        if (r->group()->type() != ResourceGroup::Type_Work) {
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
    foreach (ResourceGroupRequest *r, m_groupRequests) {
        r->makeAppointments(schedule);
    }
}

void ResourceRequestCollection::reserve(const DateTime &start, const Duration &duration) {
    //debugPlan;
    foreach (ResourceGroupRequest *r, m_groupRequests) {
        r->reserve(start, duration);
    }
}

bool ResourceRequestCollection::isEmpty() const {
    foreach (ResourceGroupRequest *r, m_groupRequests) {
        if (!r->isEmpty())
            return false;
    }
    return true;
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
    foreach (ResourceGroupRequest *g, m_groupRequests) {
        g->resetDynamicAllocations();
    }
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
