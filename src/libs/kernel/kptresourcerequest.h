/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2001 Thomas Zander zander @kde.org
 * SPDX-FileCopyrightText: 2004-2007 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2011 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
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
class ResourceRequest;
class ResourceRequestCollection;
class Schedule;
class ResourceSchedule;
class Schedule;
class XMLLoaderObject;
class DateTimeInterval;
class ResourceRequestCollection;

class PLANKERNEL_EXPORT ResourceRequest
{
public:
    explicit ResourceRequest(Resource *resource = nullptr, int units = 1);
    explicit ResourceRequest(const ResourceRequest &r);

    ~ResourceRequest();

    int id() const;
    void setId(int id);

    ResourceRequestCollection *collection() const;
    void setCollection(ResourceRequestCollection *collection);

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

    Schedule *resourceSchedule(Schedule *ns, Resource *resource = nullptr);
    DateTime availableAfter(const DateTime &time, Schedule *ns);
    DateTime availableBefore(const DateTime &time, Schedule *ns);
    Duration effort(const DateTime &time, const Duration &duration, Schedule *ns, bool backward);
    DateTime workTimeAfter(const DateTime &dt, Schedule *ns = nullptr);
    DateTime workTimeBefore(const DateTime &dt, Schedule *ns = nullptr);

    /// Resource is allocated dynamically by the group request
    bool isDynamicallyAllocated() const { return m_dynamic; }
    /// Set resource is allocated dynamically
    void setAllocatedDynaically(bool dyn) { m_dynamic = dyn; }

    /// Return a measure of how suitable the resource is for allocation
    long allocationSuitability(const DateTime &time, const Duration &duration, Schedule *ns, bool backward);

    /// Returns a list of all the required resources that will be used in scheduling.
    /// Note: This list overrides the resources own list which is just used as default for allocation dialog.
    QList<Resource*> requiredResources() const;
    /// Set the list of required resources that will be used in scheduling.
    void setRequiredResources(const QList<Resource*> &lst);
    /// Add @p resource to list of required resources
    void addRequiredResource(Resource *resource);
    /// Remove @p resource from list of required resources
    void removeRequiredResource(Resource *resource);

    QList<ResourceRequest*> alternativeRequests() const;
    void setAlternativeRequests(const QList<ResourceRequest*> requests);
    bool addAlternativeRequest(ResourceRequest *request);
    bool removeAlternativeRequest(ResourceRequest *request);

    QList<ResourceRequest*> teamMembers() const;

private:
    void emitAlternativeRequestToBeAdded(ResourceRequest *request, int row);
    void emitAlternativeRequestAdded(ResourceRequest *alternative);
    void emitAlternativeRequestToBeRemoved(ResourceRequest *request, int row, ResourceRequest *alternative);
    void emitAlternativeRequestRemoved();

protected:
    void changed();

    void setCurrentSchedulePtr(Schedule *ns);
    void setCurrentSchedulePtr(Resource *resource, Schedule *ns);

private:
    int m_id;
    Resource *m_resource;
    int m_units;
    ResourceRequestCollection *m_collection;
    bool m_dynamic;
    QList<Resource*> m_required;
    mutable QList<ResourceRequest*> m_teamMembers;

    QList<ResourceRequest*> m_alternativeRequests;

#ifndef NDEBUG
public:
    void printDebug(const QString& ident);
#endif
};

class PLANKERNEL_EXPORT ResourceRequestCollection : public QObject
{
    Q_OBJECT
public:
    explicit ResourceRequestCollection(Task *task = nullptr);
    ~ResourceRequestCollection();

    bool contains(ResourceRequest *request) const;

    /// Remove all group- and resource requests
    /// Note: Does not delete
    void removeRequests();

    ResourceRequest *resourceRequest(int id) const;
    ResourceRequest *find(const Resource *resource) const;
    ResourceRequest *resourceRequest(const QString &name) const;
    /// The ResourceRequestCollection has no requests
    bool isEmpty() const;
    /// Reset used resource requests
    void reset();

    bool contains(const QString &identity) const;
    /// Return a list of names of allocated resources.
    /// Team resources are included but *not* the team members.
    QStringList requestNameList() const;
    /// Return a list of allocated resources.
    /// Team resources are included but *not* the team members.
    QList<Resource*> requestedResources() const;

    /// Return a list of all resource requests.
    /// If @p resolveTeam is true, include the team members,
    /// if @p resolveTeam is false, include the team resource itself.
    QList<ResourceRequest*> resourceRequests(bool resolveTeam=true) const;

    /// Add the resource request @p request
    void addResourceRequest(ResourceRequest *request);
    /// Remove resource request @p request
    void removeResourceRequest(ResourceRequest *request);
    /// Delete resource request @p request
    void deleteResourceRequest(ResourceRequest *request);

    //bool load(KoXmlElement &element, Project &project);
    void save(QDomElement &element) const;

    /**
    * Returns the duration needed to do the @p effort starting at @p time.
    */
    Duration duration(const DateTime &time, const Duration &effort, Schedule *sch, bool backward = false);

    DateTime availableAfter(const DateTime &time, Schedule *ns);
    DateTime availableBefore(const DateTime &time, Schedule *ns);
    DateTime workTimeAfter(const DateTime &dt, Schedule *ns = nullptr) const;
    DateTime workTimeBefore(const DateTime &dt, Schedule *ns = nullptr) const;
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

    Task *task() const;
    void setTask(Task *t);

    void changed();

    Duration effort(const QList<ResourceRequest*> &lst, const DateTime &time, const Duration &duration, Schedule *ns, bool backward) const;
    int numDays(const QList<ResourceRequest*> &lst, const DateTime &time, bool backward) const;
    Duration duration(const QList<ResourceRequest*> &lst, const DateTime &time, const Duration &_effort, Schedule *ns, bool backward);

    ulong granularity() const;
    bool accepted(const Duration &estimate, const Duration &result, Schedule *ns = nullptr) const;

Q_SIGNALS:
    void alternativeRequestToBeAdded(KPlato::ResourceRequest *request, int row);
    void alternativeRequestAdded(KPlato::ResourceRequest *alternative);
    void alternativeRequestToBeRemoved(KPlato::ResourceRequest *request, int row, KPlato::ResourceRequest *alternative);
    void alternativeRequestRemoved();

private:
    /// Return a list of best requests.
    /// If ns is not nullptr, bookings are checked and the list is stored for later use.
    QList<ResourceRequest*> initUsedResourceRequests(const DateTime &time, Schedule *ns, bool backward) const;

private:
    Task *m_task;
    int m_lastResourceId;
    QMap<int, ResourceRequest*> m_resourceRequests;
    QList<ResourceRequest*> m_usedResourceRequests;
};

}  //KPlato namespace

PLANKERNEL_EXPORT QDebug operator<<(QDebug dbg, const KPlato::ResourceRequest *r);
PLANKERNEL_EXPORT QDebug operator<<(QDebug dbg, const KPlato::ResourceRequest &r);

#endif
