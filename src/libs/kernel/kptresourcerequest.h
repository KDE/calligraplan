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

#ifndef KPTRESOURCEREQUEST_H
#define KPTRESOURCEREQUEST_H

#include "plankernel_export.h"

#include "kptglobal.h"
#include "kptduration.h"
#include "kptdatetime.h"

#include <KoXmlReaderForward.h>

#include <QHash>
#include <QString>
#include <QList>

class QDomElement;

/// The main namespace.
namespace KPlato
{

class Account;
class Risk;
class Effort;
class Appointment;
class Task;
class Node;
class Project;
class Resource;
class ResourceGroup;
class ResourceRequest;
class ResourceGroupRequest;
class ResourceRequestCollection;
class Schedule;
class ResourceSchedule;
class Schedule;
class XMLLoaderObject;
class DateTimeInterval;


class PLANKERNEL_EXPORT ResourceRequest
{
public:
    explicit ResourceRequest(Resource *resource = 0, int units = 1);
    explicit ResourceRequest(const ResourceRequest &r);

    ~ResourceRequest();

    ResourceGroupRequest *parent() const { return m_parent; }
    void setParent(ResourceGroupRequest *parent) { m_parent = parent; }

    Resource *resource() const { return m_resource; }
    void setResource(Resource* resource) { m_resource = resource; }

    bool load(KoXmlElement &element, Project &project);
    void save(QDomElement &element) const;

    /**
    * Get amount of requested resource units in percent
    */
    int units() const;
    void setUnits(int value);

    void registerRequest();
    void unregisterRequest();

    void makeAppointment(Schedule *schedule, int amount);
    void makeAppointment(Schedule *schedule);
    Task *task() const;

    /// Return the datetime from when the resource is available.
    /// If it is not valid, the project constraint start time is used.
    /// For teams the earliest time for any team member is used.
    DateTime availableFrom();
    /// Return the datetime until when the resource is available.
    /// If it is not valid, the project constraint end time is used.
    /// For teams the latest time for any team member is used.
    DateTime availableUntil();

    Schedule *resourceSchedule(Schedule *ns, Resource *resource = 0);
    DateTime availableAfter(const DateTime &time, Schedule *ns);
    DateTime availableBefore(const DateTime &time, Schedule *ns);
    Duration effort(const DateTime &time, const Duration &duration, Schedule *ns, bool backward);
    DateTime workTimeAfter(const DateTime &dt, Schedule *ns = 0);
    DateTime workTimeBefore(const DateTime &dt, Schedule *ns = 0);

    /// Resource is allocated dynamically by the group request
    bool isDynamicallyAllocated() const { return m_dynamic; }
    /// Set resource is allocated dynamically
    void setAllocatedDynaically(bool dyn) { m_dynamic = dyn; }

    /// Return a measure of how suitable the resource is for allocation
    long allocationSuitability(const DateTime &time, const Duration &duration, Schedule *ns, bool backward);

    /// Returns a list of all the required resources that will be used in scheduling.
    /// Note: This list overrides the resources own list which is just used as default for allocation dialog.
    QList<Resource*> requiredResources() const { return m_required; }
    /// Set the list of required resources that will be used in scheduling.
    void setRequiredResources(const QList<Resource*> &lst) { m_required = lst; }

private:
    friend class ResourceGroupRequest;
    QList<ResourceRequest*> teamMembers() const;

protected:
    void changed();

    void setCurrentSchedulePtr(Schedule *ns);
    void setCurrentSchedulePtr(Resource *resource, Schedule *ns);

private:
    Resource *m_resource;
    int m_units;
    ResourceGroupRequest *m_parent;
    bool m_dynamic;
    QList<Resource*> m_required;
    mutable QList<ResourceRequest*> m_teamMembers;

#ifndef NDEBUG
public:
    void printDebug(const QString& ident);
#endif
};

class PLANKERNEL_EXPORT ResourceGroupRequest
{
public:
    explicit ResourceGroupRequest(ResourceGroup *group = 0, int units = 0);
    explicit ResourceGroupRequest(const ResourceGroupRequest &group);
    ~ResourceGroupRequest();

    void setParent(ResourceRequestCollection *parent) { m_parent = parent;}
    ResourceRequestCollection *parent() const { return m_parent; }

    ResourceGroup *group() const { return m_group; }
    void setGroup(ResourceGroup *group) { m_group = group; }
    void unregister(const ResourceGroup *group) { if (group == m_group) m_group = 0; }
    /// Return a list of resource requests.
    /// If @p resolveTeam is true, include the team members,
    /// if @p resolveTeam is false, include the team resource itself.
    QList<ResourceRequest*> resourceRequests(bool resolveTeam=true) const;
    void addResourceRequest(ResourceRequest *request);
    void deleteResourceRequest(ResourceRequest *request);
    int count() const { return m_resourceRequests.count(); }
    ResourceRequest *requestAt(int idx) const { return m_resourceRequests.value(idx); }

    ResourceRequest *takeResourceRequest(ResourceRequest *request);
    ResourceRequest *find(const Resource *resource) const;
    ResourceRequest *resourceRequest(const QString &name);
    /// Return a list of allocated resources, allocation to group is not included by default.
    QStringList requestNameList(bool includeGroup = false) const;
    /// Return a list of allocated resources.
    /// Allocations to groups are not included.
    /// Team resources are included but *not* the team members.
    /// Any dynamically allocated resource is not included.
    QList<Resource*> requestedResources() const;
    bool load(KoXmlElement &element, XMLLoaderObject &status);
    void save(QDomElement &element) const;

    /// The number of requested resources
    int units() const;
    void setUnits(int value) { m_units = value; changed(); }

    /**
     * Returns the duration needed to do the @p effort starting at @p start.
     */
    Duration duration(const DateTime &start, const Duration &effort, Schedule *ns, bool backward = false);

    DateTime availableAfter(const DateTime &time, Schedule *ns);
    DateTime availableBefore(const DateTime &time, Schedule *ns);
    DateTime workTimeAfter(const DateTime &dt, Schedule *ns = 0);
    DateTime workTimeBefore(const DateTime &dt, Schedule *ns = 0);

    /**
     * Makes appointments for schedule @p schedule to the
     * requested resources for the duration found in @ref duration().
     * @param schedule the schedule
     */
    void makeAppointments(Schedule *schedule);

    /**
     * Reserves the requested resources for the specified interval
     */
    void reserve(const DateTime &start, const Duration &duration);

    bool isEmpty() const;

    Task *task() const;

    void changed();

    /// Reset dynamic resource allocations
    void resetDynamicAllocations();
    /// Allocate dynamic requests. Do nothing if already allocated.
    void allocateDynamicRequests(const DateTime &time, const Duration &effort, Schedule *ns, bool backward);

private:
    ResourceGroup *m_group;
    int m_units;
    ResourceRequestCollection *m_parent;

    QList<ResourceRequest*> m_resourceRequests;
    DateTime m_start;
    Duration m_duration;

#ifndef NDEBUG
public:
    void printDebug(const QString& ident);
#endif
};

class PLANKERNEL_EXPORT ResourceRequestCollection
{
public:
    explicit ResourceRequestCollection(Task *task = 0);
    ~ResourceRequestCollection();

    QList<ResourceGroupRequest*> requests() const { return m_requests; }
    void addRequest(ResourceGroupRequest *request);
    void deleteRequest(ResourceGroupRequest *request)
    {
        int i = m_requests.indexOf(request);
        if (i != -1)
            m_requests.removeAt(i);
        delete request;
        changed();
    }

    int takeRequest(ResourceGroupRequest *request)
    {
        int i = m_requests.indexOf(request);
        if (i != -1) {
            m_requests.removeAt(i);
            changed();
        }
        return i;
    }

    ResourceGroupRequest *find(const ResourceGroup *resource) const;
    ResourceRequest *find(const Resource *resource) const;
    ResourceRequest *resourceRequest(const QString &name) const;
    /// The ResourceRequestCollection has no requests
    bool isEmpty() const;
    /// Empty the ResourceRequestCollection of all requets
    void clear() { m_requests.clear(); }
    /// Reset dynamic resource allocations
    void resetDynamicAllocations();

    bool contains(const QString &identity) const;
    ResourceGroupRequest *findGroupRequestById(const QString &id) const;
    /// Return a list of names of allocated resources.
    /// Allocations to groups are not included by default.
    /// Team resources are included but *not* the team members.
    /// Any dynamically allocated resource is not included.
    QStringList requestNameList(bool includeGroup = false) const;
    /// Return a list of allocated resources.
    /// Allocations to groups are not included.
    /// Team resources are included but *not* the team members.
    /// Any dynamically allocated resource is not included.
    QList<Resource*> requestedResources() const;

    /// Return a list of all resource requests.
    /// If @p resolveTeam is true, include the team members,
    /// if @p resolveTeam is false, include the team resource itself.
    QList<ResourceRequest*> resourceRequests(bool resolveTeam=true) const;

    //bool load(KoXmlElement &element, Project &project);
    void save(QDomElement &element) const;

    /**
    * Returns the duration needed to do the @p effort starting at @p time.
    */
    Duration duration(const DateTime &time, const Duration &effort, Schedule *sch, bool backward = false);

    DateTime availableAfter(const DateTime &time, Schedule *ns);
    DateTime availableBefore(const DateTime &time, Schedule *ns);
    DateTime workTimeAfter(const DateTime &dt, Schedule *ns = 0) const;
    DateTime workTimeBefore(const DateTime &dt, Schedule *ns = 0) const;
    DateTime workStartAfter(const DateTime &time, Schedule *ns);
    DateTime workFinishBefore(const DateTime &time, Schedule *ns);

    /**
    * Makes appointments for the schedule @p schedule to the requested resources.
    * Assumes that @ref duration() has been run.
    * @param schedule the schedule
    */
    void makeAppointments(Schedule *schedule);
    /**
     * Reserves the requested resources for the specified interval
     */
    void reserve(const DateTime &start, const Duration &duration);

    Task *task() const { return m_task; }
    void setTask(Task *t) { m_task = t; }

    void changed();

    Duration effort(const QList<ResourceRequest*> &lst, const DateTime &time, const Duration &duration, Schedule *ns, bool backward) const;
    int numDays(const QList<ResourceRequest*> &lst, const DateTime &time, bool backward) const;
    Duration duration(const QList<ResourceRequest*> &lst, const DateTime &time, const Duration &_effort, Schedule *ns, bool backward);

private:
    Task *m_task;
    QList<ResourceGroupRequest*> m_requests;
};

}  //KPlato namespace

PLANKERNEL_EXPORT QDebug operator<<(QDebug dbg, const KPlato::ResourceRequest *r);
PLANKERNEL_EXPORT QDebug operator<<(QDebug dbg, const KPlato::ResourceRequest &r);

#endif
