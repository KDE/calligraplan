/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2001 Thomas Zander zander @kde.org
 * SPDX-FileCopyrightText: 2004-2007 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2011 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KPTRESOURCE_H
#define KPTRESOURCE_H

#include "plankernel_export.h"

#include "kptglobal.h"
#include "kptduration.h"
#include "kptdatetime.h"
#include "kptappointment.h"
#include "kptcalendar.h"

#include <KoXmlReaderForward.h>

#include <QHash>
#include <QString>
#include <QList>


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
class ResourceGroup;
class ResourceRequest;
class ResourceGroupRequest;
class ResourceRequestCollection;
class Schedule;
class ResourceSchedule;
class Schedule;
class XMLLoaderObject;

/**
  * Any resource that is used by a task. A resource can be a worker, or maybe wood.
  * If the resources is a worker or a piece of equipment which can be reused but
  * can only be used by one node in time, then we can use the scheduling methods of the
  * resource to schedule the resource available time for the project.
  * The Idea is that all nodes which need this resource point to it and the scheduling
  * code (partly implemented here) schedules the actual usage.
  * See also @ref ResourceGroup
  */

class PLANKERNEL_EXPORT Resource : public QObject
{
    Q_OBJECT
public:

    Resource();
    explicit Resource(Resource *resource);
    ~Resource() override;

    QString id() const { return m_id; }
    void setId(const QString& id);

    enum Type { Type_Work, Type_Material, Type_Team };
    void setType(Type type);
    void setType(const QString &type);
    Type type() const { return m_type; }
    QString typeToString(bool trans = false) const;
    static QStringList typeToStringList(bool trans = false);

    void setName(const QString &n);
    const QString &name() const { return m_name;}

    void setInitials(const QString &initials);
    const QString &initials() const { return m_initials;}

    void setEmail(const QString &email);
    const QString &email() const { return m_email;}

    /// Returns true if this resource will be allocated by default to new tasks
    bool autoAllocate() const;
    /// Set if this resource will be allocated by default to new tasks
    void setAutoAllocate(bool on);

    void copy(Resource *resource);

    int groupCount() const;
    int groupIndexOf(ResourceGroup *group) const;
    void addParentGroup(ResourceGroup *parent);
    bool removeParentGroup(ResourceGroup *parent);
    void setParentGroups(QList<ResourceGroup*> &parents);
    QList<ResourceGroup*> parentGroups() const;

    /// Set the time from when the resource is available to this project
    void setAvailableFrom(const DateTime &af) { m_availableFrom = af; changed(); }
    /// Return the time when the resource is available to this project
    const DateTime &availableFrom() const { return m_availableFrom;}
    /// Set the time when the resource is no longer available to this project
    void setAvailableUntil(const DateTime &au) { m_availableUntil = au; changed(); }
    /// Return the time when the resource is no longer available to this project.
    const DateTime &availableUntil() const { return m_availableUntil;}

    DateTime firstAvailableAfter(const DateTime &time, const DateTime &limit) const;

    DateTime getBestAvailableTime(const Duration &duration);
    DateTime getBestAvailableTime(const DateTime &after, const Duration &duration);

    // NOTE: Saving is done here, loading is done using the XmlLoaderObject
    void save(QDomElement &element) const;

    /// Return the list of appointments for schedule @p id.
    QList<Appointment*> appointments(long id = -1) const;
    /// Return the number of appointments (nodes)
    int numAppointments(long id = -1) const { return appointments(id).count(); }
    /// Return the appointment at @p index for schedule @p id
    Appointment *appointmentAt(int index, long id = -1) const { return appointments(id).value(index); }
    int indexOf(Appointment *a, long id = -1) const { return appointments(id).indexOf(a); }

    /// Adds appointment to current schedule
    virtual bool addAppointment(Appointment *appointment);
    /// Adds appointment to schedule sch
    virtual bool addAppointment(Appointment *appointment, Schedule &main);
    /// Adds appointment to both this resource and node
    virtual void addAppointment(Schedule *node, const DateTime &start, const DateTime &end, double load = 100);

    void initiateCalculation(Schedule &sch);
    bool isAvailable(Task *task);
    void makeAppointment(Schedule *schedule, int load, const QList<Resource*> &required = QList<Resource*>());

    bool isOverbooked() const;
    /// check if overbooked on date.
    bool isOverbooked(const QDate &date) const;
    /// check if overbooked within the interval start, end.
    bool isOverbooked(const DateTime &start, const DateTime &end) const;

    double normalRate() const { return m_cost.normalRate; }
    void setNormalRate(double rate) { m_cost.normalRate = rate; changed(); }
    double overtimeRate() const { return m_cost.overtimeRate; }
    void setOvertimeRate(double rate) { m_cost.overtimeRate = rate; changed(); }

    /**
     * Return available units in percent
     */
    int units() const { return m_units; }
    /**
     * Set available units in percent
     */
    void setUnits(int units);

    Project *project() const { return m_project; }
    /// Return the resources timespec. Defaults to local.
    QTimeZone timeZone() const;

    /**
     * Get the calendar for this resource.
     * Working resources may have a default calendar if the a calendar is marked as default,
     * this is checked if local=false.
     * If no calendar can be found for a working resource, the resource is not available.
     *
     * Material resources must have calendar explicitly set.
     * If there is no calendar set for a material resource, the resource is always available.
     */
    Calendar *calendar(bool local = false) const;
    //Calendar *calendar(const QString& id) const;
    void setCalendar(Calendar *calendar);

    /// Delete all requests for me
    void removeRequests();
    /**
     * Used to clean up requests when the resource is deleted.
     */
    void registerRequest(ResourceRequest *request);
    void unregisterRequest(ResourceRequest *request);
    const QList<ResourceRequest*> &requests() const;

    /// Returns a list of work intervals in the interval @p from, @p until.
    /// Appointments are subtracted if @p schedule is not 0 and overbooking is not allowed.
    AppointmentIntervalList workIntervals(const DateTime &from, const DateTime &until, Schedule *schedule) const;

    /// Returns a list of work intervals in the interval @p from, @p until.
    AppointmentIntervalList workIntervals(const DateTime &from, const DateTime &until) const;

    /// Updates work interval cache a list of work intervals extracted from the resource calendar
    /// with @p load in the interval @p from, @p until.
    /// The load of the intervals is set to m_units
    /// Note: The list may contain intervals outside @p from, @p until
    void calendarIntervals(const DateTime &from, const DateTime &until) const;
    /// Load cache from @p element
    bool loadCalendarIntervalsCache(const KoXmlElement& element, KPlato::XMLLoaderObject& status);
    /// Save cache to @p element
    void saveCalendarIntervalsCache(QDomElement &element) const;

    /// Returns the effort that can be done starting at @p start within @p duration.
    /// The current schedule is used to check for appointments.
    /// If @p  backward is true, checks backward in time.
    Duration effort(const DateTime &start, const Duration &duration, int units = 100, bool backward = false, const QList<Resource*> &required = QList<Resource*>()) const;

    /// Returns the effort that can be done starting at @p start within @p duration.
    /// The schedule @p sch is used to check for appointments.
    /// If @p  backward is true, checks backward in time.
    /// Status is returned in @p ok
    Duration effort(KPlato::Schedule* sch, const DateTime &start, const Duration& duration, int units = 100, bool backward = false, const QList< Resource* >& required = QList<Resource*>()) const;


    /**
     * Find the first available time after @p time, within @p limit.
     * Returns invalid DateTime if not available.
     * Uses the current schedule to check for appointments.
     */
    DateTime availableAfter(const DateTime &time, const DateTime &limit = DateTime()) const;
    /**
     * Find the first available time before @p time, within @p limit.
     * Returns invalid DateTime if not available.
     * Uses the current schedule to check for appointments.
     */
    DateTime availableBefore(const DateTime &time, const DateTime &limit = DateTime()) const;

    /**
     * Find the first available time after @p time, within @p limit.
     * Returns invalid DateTime if not available.
     * If @p sch == 0, Appointments are not checked.
     */
    DateTime availableAfter(const DateTime &time, const DateTime &limit, Schedule *sch) const;
    /**
     * Find the first available time before @p time, within @p limit.
     * Returns invalid DateTime if not available.
     * If @p sch == 0, Appointments are not checked.
     */
    DateTime availableBefore(const DateTime &time, const DateTime &limit, Schedule *sch) const;

    Resource *findId() const { return findId(m_id); }
    Resource *findId(const QString &id) const;
    bool removeId() { return removeId(m_id); }
    bool removeId(const QString &id);
    void insertId(const QString &id);

    Calendar *findCalendar(const QString &id) const;

    Appointment appointmentIntervals(long id) const;
    Appointment appointmentIntervals() const;

    EffortCostMap plannedEffortCostPrDay(const QDate &start, const QDate &end, long id, EffortCostCalculationType = ECCT_All);
    Duration plannedEffort(const QDate &date, EffortCostCalculationType = ECCT_All) const;

    void setCurrentSchedulePtr(Schedule *schedule) { m_currentSchedule = schedule; }
    void setCurrentSchedule(long id) { m_currentSchedule = findSchedule(id); }
    Schedule *currentSchedule() const { return m_currentSchedule; }

    bool isScheduled() const;
    QHash<long, Schedule*> schedules() const { return m_schedules; }
    /**
     * Return schedule with @p id
     * If @p id == CURRENTSCHEDULE, return m_currentSchedule
     * Return 0 if schedule with @p id doesn't exist.
     */
    Schedule *schedule(long id = CURRENTSCHEDULE) const;
    /// Returns true if schedule with @p id is baselined.
    /// if Team resource, if any of the team members is baselined
    /// By default returns true if any schedule is baselined
    bool isBaselined(long id = BASELINESCHEDULE) const;
    /**
     * Return schedule with @p id
     * Return 0 if schedule with @p id doesn't exist.
     */
    Schedule *findSchedule(long id) const;
    /// Take, and delete.
    void deleteSchedule(Schedule *schedule);
    /// Take, don't delete.
    void takeSchedule(const Schedule *schedule);
    void addSchedule(Schedule *schedule);
    ResourceSchedule *createSchedule(const QString& name, int type, long id);
    ResourceSchedule *createSchedule(Schedule *parent);

    // m_project is set when the resource (or the parent) is added to the project,
    // and reset when the resource is removed from the project
    void setProject(Project *project);

    void addExternalAppointment(const QString &id, Appointment *a);

    void addExternalAppointment(const QString &id, const QString &name, const DateTime &from, const DateTime &end, double load = 100);
    void subtractExternalAppointment(const QString &id, const DateTime &from, const DateTime &end, double load);

    void clearExternalAppointments();
    void clearExternalAppointments(const QString &id);
    /// Take the external appointments with identity @p id from the list of external appointments
    Appointment *takeExternalAppointment(const QString &id);
    /// Return external appointments with identity @p id
    AppointmentIntervalList externalAppointments(const QString &id);
    AppointmentIntervalList externalAppointments(const DateTimeInterval &interval = DateTimeInterval()) const;

    int numExternalAppointments() const { return m_externalAppointments.count(); }
    QList<Appointment*> externalAppointmentList() const { return m_externalAppointments.values(); }
    /// return a map of project id, project name
    QMap<QString, QString> externalProjects() const;

    /// Return a measure of how suitable the resource is for allocation
    long allocationSuitability(const DateTime &time, const Duration &duration, bool backward);

    DateTime startTime(long id) const;
    DateTime endTime(long id) const;

    /// Returns the list of required resources.
    /// Note: This list is used as default for allocation dialog, not for scheduling.
    QList<Resource*> requiredResources() const;
    /// Set the list of the required resources's ids so they can be resolved when used
    /// A required resource may not exist in the project yet
    void setRequiredIds(const QStringList &lst);
    /// Add a resource id to the required ids list
    void addRequiredId(const QString &id);
    /// Returns the list of required resource ids.
    QStringList requiredIds() const { return m_requiredIds; }
    /// Update the ids from the resources
    void refreshRequiredIds();

    /// Number of team members
    int teamCount() const;
    /// Return the list of team members.
    QList<Resource*> teamMembers() const;
    /// Return the list of team members.
    QStringList teamMemberIds() const;
    /// Clear the list of team members.
    void clearTeamMembers() { m_teamMemberIds.clear(); }
    /// Add resource @p id to the list of team members.
    void addTeamMemberId(const QString &id);
    /// Remove resource @p id from the list of team members.
    void removeTeamMemberId(const QString &id);
    /// Set the list of team members to @p ids
    void setTeamMemberIds(const QStringList &ids);
    /// Update the ids from the resources
    void refreshTeamMemberIds();

    /// Return the account
    Account *account() const { return m_cost.account; }
    /// Set the @p account
    void setAccount(Account *account);

    void blockChanged(bool on = true);

    /// A resource group can be local to this project, or
    /// defined externally and shared with other projects
    bool isShared() const;
    /// Set resource group to be shared if on = true, or local if on = false
    void setShared(bool on);

    // for xml loading code

    class WorkInfoCache
    {
    public:
        WorkInfoCache() { clear(); }
        void clear() { start = end = DateTime(); effort = Duration::zeroDuration; intervals.clear(); version = -1; }
        bool isValid() const { return start.isValid() && end.isValid(); }
        DateTime firstAvailableAfter(const DateTime &time, const DateTime &limit, Calendar *cal, Schedule *sch) const;
        DateTime firstAvailableBefore(const DateTime &time, const DateTime &limit, Calendar *cal, Schedule *sch) const;

        DateTime start;
        DateTime end;
        Duration effort;
        AppointmentIntervalList intervals;
        int version;

        bool load(const KoXmlElement& element, KPlato::XMLLoaderObject& status);
        void save(QDomElement &element) const;
    };
    const WorkInfoCache &workInfoCache() const { return m_workinfocache; }

Q_SIGNALS:
    void dataChanged(KPlato::Resource *resource);

    void resourceGroupToBeAdded(KPlato::Resource *resource, int row);
    void resourceGroupAdded(KPlato::ResourceGroup *group);
    void resourceGroupToBeRemoved(KPlato::Resource *resource, int row, KPlato::ResourceGroup *group);
    void resourceGroupRemoved();

    void externalAppointmentToBeAdded(KPlato::Resource *r, int row);
    void externalAppointmentAdded(KPlato::Resource*, KPlato::Appointment*);
    void externalAppointmentToBeRemoved(KPlato::Resource *r, int row);
    void externalAppointmentRemoved();
    void externalAppointmentChanged(KPlato::Resource *r, KPlato::Appointment *a);

    void teamToBeAdded(KPlato::Resource *r, int row);
    void teamAdded(KPlato::Resource*);
    void teamToBeRemoved(KPlato::Resource *r, int row, KPlato::Resource *team);
    void teamRemoved();

protected:
    DateTimeInterval requiredAvailable(Schedule *node, const DateTime &start, const DateTime &end) const;
    void makeAppointment(Schedule *node, const DateTime &from, const DateTime &end, int load, const QList<Resource*> &required = QList<Resource*>());
    virtual void changed();

private:
    inline void updateCache() const;

private:
    Project *m_project;
    QList<ResourceGroup*> m_parents;
    QHash<long, Schedule*> m_schedules;
    QString m_id; // unique id
    QString m_name;
    QString m_initials;
    QString m_email;
    bool m_autoAllocate;
    DateTime m_availableFrom;
    DateTime m_availableUntil;
    QMap<QString, Appointment*> m_externalAppointments;

    int m_units; // available units in percent

    Type m_type;

    struct Cost
    {
        double normalRate;
        double overtimeRate;
        double fixed ;
        Account *account;
    }
    m_cost;
public:
    Cost &cost() { return m_cost; }

private:
    Calendar *m_calendar;
    QList<ResourceRequest*> m_requests;
    QStringList m_requiredIds;
    QList<Resource*> m_requiredResources; // cache

    QStringList m_teamMemberIds;
    QList<Resource*> m_teamMembers; // cache

    Schedule *m_currentSchedule;

    mutable WorkInfoCache m_workinfocache;

    // return this if resource has no calendar and is a material resource
    Calendar m_materialCalendar;
    bool m_blockChanged;
    bool m_shared;

#ifndef NDEBUG
public:
    void printDebug(const QString& ident);
#endif
};

PLANKERNEL_EXPORT QDebug operator<<(QDebug dbg, const KPlato::Resource::WorkInfoCache &c);

/**
 * Risk is associated with a resource/task pairing to indicate the planner's confidence in the
 * estimated effort. Risk can be one of none, low, or high. Some factors that may be taken into
 * account for risk are the experience of the person and the reliability of equipment.
 */
class Risk
{
public:

    enum RiskType {
        NONE = 0,
        LOW = 1,
        HIGH = 2
    };

    Risk(Node *n, Resource *r, RiskType rt = NONE);
    ~Risk();

    RiskType riskType() { return m_riskType; }

    Node *node() { return m_node; }
    Resource *resource() { return m_resource; }

private:
    Node *m_node;
    Resource *m_resource;
    RiskType m_riskType;
};

} // namespace KPlato

PLANKERNEL_EXPORT QDebug operator<<(QDebug dbg, const KPlato::Resource &r);
PLANKERNEL_EXPORT QDebug operator<<(QDebug dbg, const KPlato::Resource *r);

#endif
