/* This file is part of the KDE project
   Copyright (C) 2001 Thomas Zander zander@kde.org
   Copyright (C) 2004 - 2007 Dag Andersen <dag.andersen@kdemail.net>
   Copyright (C) 2007 Florian Piquemal <flotueur@yahoo.fr>
   Copyright (C) 2007 Alexis Ménard <darktears31@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#ifndef KPTTASK_H
#define KPTTASK_H

#include "plankernel_export.h"

#include "kptnode.h"
#include "kptglobal.h"
#include "kptdatetime.h"
#include "kptduration.h"
#include "kptresource.h"

#include <QList>
#include <QMap>
#include <utility>

/// The main namespace.
namespace KPlato
{

class Completion;
class XmlSaveContext;

/**
 * The Completion class holds information about the tasks progress.
 */
class PLANKERNEL_EXPORT Completion
{
    
public:
    class PLANKERNEL_EXPORT UsedEffort
    {
        public:
            class PLANKERNEL_EXPORT ActualEffort : public std::pair<Duration, Duration>
            {
                public:
                    explicit ActualEffort(KPlato::Duration ne = Duration::zeroDuration, KPlato::Duration oe = Duration::zeroDuration)
                        : std::pair<Duration, Duration>(ne, oe)
                    {}
                    ActualEffort(const ActualEffort &e)
                        : std::pair<Duration, Duration>(e.first, e.second)
                    {}
                    ~ActualEffort() {}
                    bool isNull() const { return (first + second) == Duration::zeroDuration; }
                    Duration normalEffort() const { return first; }
                    void setNormalEffort(KPlato::Duration e) { first = e; }
                    Duration overtimeEffort() const { return second; }
                    void setOvertimeEffort(KPlato::Duration e) { second = e; }
                    /// Returns the sum of normalEffort + overtimeEffort
                    Duration effort() const { return first + second; }
                    void setEffort(KPlato::Duration ne, KPlato::Duration oe = Duration::zeroDuration) { first = ne; second = oe; }
                    ActualEffort &operator=(const ActualEffort &other) {
                        first = other.first;
                        second = other.second;
                        return *this;
                    }
            };
            UsedEffort();
            UsedEffort(const UsedEffort &e);
            ~UsedEffort();
            bool operator==(const UsedEffort &e) const;
            bool operator!=(const UsedEffort &e) const { return !operator==(e); }
            void mergeEffort(const UsedEffort &value);
            void setEffort(QDate date, const ActualEffort &value);
            /// Returns the total effort up to @p date
            Duration effortTo(QDate date) const;
            /// Returns the total effort on @p date
            ActualEffort effort(QDate date) const { return m_actual.value(date); }
            ActualEffort takeEffort(QDate date) { return m_actual.take(date); }
            /// Returns the total effort for all registered dates
            Duration effort() const;
            QDate firstDate() const { return m_actual.firstKey(); }
            QDate lastDate() const { return m_actual.lastKey(); }
            QMap<QDate, ActualEffort> actualEffortMap() const { return m_actual; }
            
            /// Load from document
            bool loadXML(KoXmlElement &element, XMLLoaderObject &status);
            /// Save to document
            void saveXML(QDomElement &element) const;
            bool contains(QDate date) const { return m_actual.contains(date); }

        private:
            QMap<QDate, ActualEffort> m_actual;
    };
    typedef QMap<QDate, UsedEffort::ActualEffort> DateUsedEffortMap;
    
    class PLANKERNEL_EXPORT Entry
    {
        public:
            Entry()
            : percentFinished(0),
              remainingEffort(Duration::zeroDuration),
              totalPerformed(Duration::zeroDuration)
            {}
            Entry(int percent,  Duration remaining, Duration performed)
            : percentFinished(percent),
              remainingEffort(remaining),
              totalPerformed(performed)
            {}
            Entry(const Entry &e) { copy(e); }
            bool operator==(const Entry &e) const {
                return percentFinished == e.percentFinished
                    && remainingEffort == e.remainingEffort
                    && totalPerformed == e.totalPerformed
                    && note == e.note;
            }
            bool operator!=(const Entry &e) const { return ! operator==(e); }
            Entry &operator=(const Entry &e) { copy(e); return *this; }
            
            int percentFinished;
            Duration remainingEffort;
            Duration totalPerformed;
            QString note;
        protected:
            void copy(const Entry &e) {
                percentFinished = e.percentFinished;
                remainingEffort = e.remainingEffort;
                totalPerformed = e.totalPerformed;
                note = e.note;
            }
    };
    typedef QMap<QDate, Entry*> EntryList;

    typedef QHash<const Resource*, UsedEffort*> ResourceUsedEffortMap;
    
    explicit Completion(Node *node = nullptr);  // review * or &, or at all?
    Completion(const Completion &copy);
    virtual ~Completion();
    
    bool operator==(const Completion &p);
    bool operator!=(Completion &p) { return !(*this == p); }
    Completion &operator=(const Completion &p);
    
    /// Load from document
    bool loadXML(KoXmlElement &element, XMLLoaderObject &status);
    /// Save to document
    void saveXML(QDomElement &element) const;
    
    bool startIsValid() const { return m_started && m_startTime.isValid(); }
    bool isStarted() const { return m_started; }
    void setStarted(bool on);
    bool finishIsValid() const { return m_finished && m_finishTime.isValid(); }
    bool isFinished() const { return m_finished; }
    void setFinished(bool on);
    DateTime startTime() const { return m_startTime; }
    void setStartTime(const DateTime &dt);
    DateTime finishTime() const { return m_finishTime; }
    void setFinishTime(const DateTime &dt);
    void setPercentFinished(QDate date, int value);
    void setRemainingEffort(QDate date, Duration value);
    void setActualEffort(QDate date, Duration value);
    
    /// Return a list of the resource that has done any work on this task
    QList<const Resource*> resources() const { return m_usedEffort.keys(); }
    
    const EntryList &entries() const { return m_entries; }
    void addEntry(QDate date, Entry *entry);
    Entry *takeEntry(QDate date) { return m_entries.take(date); changed(); }
    Entry *entry(QDate date) const { return m_entries[ date ]; }
    
    /// Returns the date of the latest entry
    QDate entryDate() const;
    /// Returns the percentFinished of the latest entry
    int percentFinished() const;
    /// Returns the percentFinished on @p date
    int percentFinished(QDate date) const;
    /// Returns the estimated remaining effort
    Duration remainingEffort() const;
    /// Returns the estimated remaining effort on @p date
    Duration remainingEffort(QDate date) const;
    /// Returns the total actual effort
    Duration actualEffort() const;
    /// Returns the total actual effort on @p date
    Duration actualEffort(QDate date) const;
    /// Returns the total actual effort upto and including @p date
    Duration actualEffortTo(QDate date) const;
    /// Returns the actual effort for @p resource on @p date
    Duration actualEffort(const Resource *resource, QDate date) const;
    /// TODO
    QString note() const;
    /// TODO
    void setNote(const QString &str);
    
    /// Returns the total actual cost
    double actualCost() const;
    /// Returns the actual cost for @p resource
    double actualCost(const Resource *resource) const;
    /// Returns the actual cost on @p date
    double actualCost(QDate date) const;
    /// Returns the total actual cost for @p resource on @p date
    double actualCost(const Resource *resource, QDate date) const;
    /// Returns the total actual effort and cost upto and including @p date
    EffortCost actualCostTo(long int id, QDate date) const;
    
    /**
     * Returns a map of all actual effort and cost entered
     */
    virtual EffortCostMap actualEffortCost(long id, EffortCostCalculationType type = ECCT_All) const;

    void addUsedEffort(const Resource *resource, UsedEffort *value = nullptr);
    UsedEffort *takeUsedEffort(const Resource *r) { return m_usedEffort.take(const_cast<Resource*>(r) ); changed(); }
    UsedEffort *usedEffort(const Resource *r) const { return m_usedEffort.value(const_cast<Resource*>(r) ); }
    const ResourceUsedEffortMap &usedEffortMap() const { return m_usedEffort; }

    void setActualEffort(Resource *resource, const QDate &date, const UsedEffort::ActualEffort &value);
    // FIXME name clash
    UsedEffort::ActualEffort getActualEffort(Resource *resource, const QDate &date) const;

    void changed(int property = -1);
    Node *node() const { return m_node; }
    void setNode(Node *node) { m_node = node; }
    
    enum Entrymode { FollowPlan, EnterCompleted, EnterEffortPerTask, EnterEffortPerResource };
    void setEntrymode(Entrymode mode) { m_entrymode = mode; }
    Entrymode entrymode() const { return m_entrymode; }
    void setEntrymode(const QString &mode);
    QString entryModeToString() const;
    QStringList entrymodeList() const;
    
    EffortCostMap effortCostPrDay(QDate start, QDate end, long id = -1) const;
    /// Returns the actual effort and cost pr day used by @p resource
    EffortCostMap effortCostPrDay(const Resource *resource, QDate start, QDate end, long id = CURRENTSCHEDULE) const;

protected:
    void copy(const Completion &copy);
    double averageCostPrHour(QDate date, long id) const;
    std::pair<QDate, QDate> actualStartEndDates() const;

private:
    Node *m_node;
    bool m_started, m_finished;
    DateTime m_startTime, m_finishTime;
    EntryList m_entries;
    ResourceUsedEffortMap m_usedEffort;
    Entrymode m_entrymode;
    
#ifndef NDEBUG
public:
    void printDebug(const QByteArray &ident) const;
#endif
};

/**
 * The WorkPackage class controls work flow for a task
 */
class PLANKERNEL_EXPORT WorkPackage
{
public:

    /// @enum WPTransmitionStatus describes if this package was sent or received
    enum WPTransmitionStatus {
        TS_None,        /// Not sent nor received
        TS_Send,        /// Package was sent to resource
        TS_Receive,     /// Package was received from resource
        TS_Rejected     /// Received package was rejected by project manager
    };

    explicit WorkPackage(Task *task = nullptr);
    explicit WorkPackage(const WorkPackage &wp);
    virtual ~WorkPackage();

    Task *parentTask() const { return m_task; }
    void setParentTask(Task *task) { m_task = task; }

    /// Returns the transmission status of this package
    WPTransmitionStatus transmitionStatus() const { return m_transmitionStatus; }
    void setTransmitionStatus(WPTransmitionStatus sts) { m_transmitionStatus = sts; }
    static QString transmitionStatusToString(WPTransmitionStatus sts, bool trans = false);
    static WPTransmitionStatus transmitionStatusFromString(const QString &sts);
    
    /// Load from document
    virtual bool loadXML(KoXmlElement &element, XMLLoaderObject &status);
    /// Save the full workpackage
    virtual void saveXML(QDomElement &element) const;

    /// Load from document
    virtual bool loadLoggedXML(KoXmlElement &element, XMLLoaderObject &status);
    /// Save the full workpackage
    virtual void saveLoggedXML(QDomElement &element) const;

    /// Set schedule manager
    void setScheduleManager(ScheduleManager *sm);
    /// Return schedule manager
    ScheduleManager *scheduleManager() const { return m_manager; }
    /// Return the schedule id, or NOTSCHEDULED if no schedule manager is set
    long id() const { return m_manager ? m_manager->scheduleId() : NOTSCHEDULED; }

    Completion &completion();
    const Completion &completion() const;

    void addLogEntry(DateTime &dt, const QString &str);
    QMap<DateTime, QString> log() const;
    QStringList log();

    /// Return a list of resources fetched from the appointments or requests
    /// merged with resources added to completion
    QList<Resource*> fetchResources();

    /// Return a list of resources fetched from the appointments or requests
    /// merged with resources added to completion
    QList<Resource*> fetchResources(long id);

    /// Returns id of the resource that owns this package. If empty, task leader owns it.
    QString ownerId() const { return m_ownerId; }
    /// Set the resource that owns this package to @p owner. If empty, task leader owns it.
    void setOwnerId(const QString &id) { m_ownerId = id; }

    /// Returns the name of the resource that owns this package.
    QString ownerName() const { return m_ownerName; }
    /// Set the name of the resource that owns this package.
    void setOwnerName(const QString &name) { m_ownerName = name; }

    DateTime transmitionTime() const { return m_transmitionTime; }
    void setTransmitionTime(const DateTime &dt) { m_transmitionTime = dt; }

    /// Clear workpackage data
    void clear();

private:
    Task *m_task;
    ScheduleManager *m_manager;
    Completion m_completion;
    QString m_ownerName;
    QString m_ownerId;
    WPTransmitionStatus m_transmitionStatus;
    DateTime m_transmitionTime;

    QMap<DateTime, QString> m_log;
};

class PLANKERNEL_EXPORT WorkPackageSettings
{
public:
    WorkPackageSettings();
    bool loadXML(const KoXmlElement &element);
    void saveXML(QDomElement &element) const;
    bool operator==(WorkPackageSettings settings) const;
    bool operator!=(WorkPackageSettings settings) const;
    bool usedEffort;
    bool progress;
    bool documents;
    bool remainingEffort;
};

/**
  * A task in the scheduling software is represented by this class. A task
  * can be anything from 'build house' to 'drill hole' It will always mean
  * an activity.
  */
class PLANKERNEL_EXPORT Task : public Node {
    Q_OBJECT
public:
    explicit Task(Node *parent = nullptr);
    explicit Task(const Task &task, Node *parent = nullptr);
    ~Task() override;

    /// Return task type. Can be Type_Task, Type_Summarytask ot Type_Milestone.
    int type() const override;

    /**
     * Instead of using the expected duration, generate a random value using
     * the Distribution of each Task. This can be used for Monte-Carlo
     * estimation of Project duration.
     */
    Duration *getRandomDuration() override;

//     void clearResourceRequests();
    void makeAppointments() override;
    QStringList requestNameList() const override;
    virtual QList<Resource*> requestedResources() const;
    bool containsRequest(const QString &/*identity*/) const override;
    ResourceRequest *resourceRequest(const QString &/*name*/) const override;
    
    /// Return the list of resources assigned to this task
    QStringList assignedNameList(long id = CURRENTSCHEDULE) const override;

    /**
     * Calculates if the assigned resource is overbooked 
     * within the duration of this task
     */
    void calcResourceOverbooked() override;
    
    /// Load from document
    bool load(KoXmlElement &element, XMLLoaderObject &status) override;
    /// Save to document
    void save(QDomElement &element, const XmlSaveContext &context) const override;
    /// Save appointments for schedule with id
    void saveAppointments(QDomElement &element, long id) const override;
    
    /// Save a workpackage document with schedule identity @p id
    void saveWorkPackageXML(QDomElement &element, long id) const override;

    /**
     * Returns a list of planned effort and cost for this task
     * for the interval start, end inclusive
     */
    EffortCostMap plannedEffortCostPrDay(QDate start, QDate end,  long id = CURRENTSCHEDULE, EffortCostCalculationType = ECCT_All) const override;
    /**
     * Returns a list of planned effort and cost for the @p resource
     * for the interval @p start, @p end inclusive, useng schedule with identity @p id
     */
    EffortCostMap plannedEffortCostPrDay(const Resource *resource, QDate start, QDate end,  long id = CURRENTSCHEDULE, EffortCostCalculationType = ECCT_All) const override;
    
    /// Returns the total planned effort for @p resource on this task (or subtasks)
    Duration plannedEffort(const Resource *resource, long id = CURRENTSCHEDULE, EffortCostCalculationType = ECCT_All) const override;
    /// Returns the total planned effort for this task (or subtasks) 
    Duration plannedEffort(long id = CURRENTSCHEDULE, EffortCostCalculationType = ECCT_All) const override;
    /// Returns the total planned effort for this task (or subtasks) on date
    Duration plannedEffort(QDate date, long id = CURRENTSCHEDULE, EffortCostCalculationType = ECCT_All) const override;
    /// Returns the total planned effort for @p resource on this task (or subtasks) on date
    Duration plannedEffort(const Resource *resource, QDate date, long id = CURRENTSCHEDULE, EffortCostCalculationType = ECCT_All) const override;
    /// Returns the planned effort up to and including date
    Duration plannedEffortTo(QDate date, long id = CURRENTSCHEDULE, EffortCostCalculationType = ECCT_All) const override;
    /// Returns the planned effort for @p resource up to and including date
    Duration plannedEffortTo(const Resource *resource, QDate date, long id = CURRENTSCHEDULE, EffortCostCalculationType = ECCT_All) const override;
    
    /// Returns the total actual effort for this task (or subtasks) 
    Duration actualEffort() const override;
    /// Returns the total actual effort for this task (or subtasks) on date
    Duration actualEffort(QDate date) const override;
    /// Returns the actual effort up to and including date
    Duration actualEffortTo(QDate date) const override;
    
    /**
     * Returns the total planned cost for this task (or subtasks)
     */
    EffortCost plannedCost(long id = CURRENTSCHEDULE, EffortCostCalculationType = ECCT_All) const override;
    /// Planned cost up to and including date
    double plannedCostTo(QDate /*date*/, long id = CURRENTSCHEDULE, EffortCostCalculationType = ECCT_All) const override;
    
    /// Returns actual effort and cost up to and including @p date
    EffortCost actualCostTo(long int id, QDate date) const override;

    /**
     * Returns a list of actual effort and cost for this task
     * for the interval start, end inclusive
     */
    EffortCostMap actualEffortCostPrDay(QDate start, QDate end,  long id = CURRENTSCHEDULE, EffortCostCalculationType = ECCT_All) const override;
    /// Returns the actual effort and cost pr day used by @p resource
    EffortCostMap actualEffortCostPrDay(const Resource *resource, QDate start, QDate end, long id = CURRENTSCHEDULE, EffortCostCalculationType = ECCT_All) const override;

    /// Returns the effort planned to be used to reach the actual percent finished
    Duration budgetedWorkPerformed(QDate date, long id = CURRENTSCHEDULE) const override;

    /// Returns the cost planned to be used to reach the actual percent finished
    double budgetedCostPerformed(QDate date, long id = CURRENTSCHEDULE) const override;

    using Node::bcwsPrDay;
    /// Return map of Budgeted Cost of Work Scheduled pr day
    EffortCostMap bcwsPrDay(long id = CURRENTSCHEDULE, EffortCostCalculationType type = ECCT_All) override;
    
    /// Budgeted Cost of Work Scheduled
    double bcws(QDate date, long id = CURRENTSCHEDULE) const override;

    using Node::bcwpPrDay;
    /// Return map of Budgeted Cost of Work Performed pr day (also includes bcwsPrDay)
    EffortCostMap bcwpPrDay(long id = CURRENTSCHEDULE, EffortCostCalculationType type = ECCT_All) override;
    /// Budgeted Cost of Work Performed
    double bcwp(long id = CURRENTSCHEDULE) const override;
    /// Budgeted Cost of Work Performed (up to @p date)
    double bcwp(QDate date, long id = CURRENTSCHEDULE) const override;

    using Node::acwp;
    /// Map of Actual Cost of Work Performed
    EffortCostMap acwp(long id = CURRENTSCHEDULE, EffortCostCalculationType type = ECCT_All) override;
    /// Actual Cost of Work Performed up to dat
    EffortCost acwp(QDate date, long id = CURRENTSCHEDULE) const override;

    /// Effort based performance index
    double effortPerformanceIndex(QDate date, long id = CURRENTSCHEDULE) const override;

    /// Schedule performance index
    double schedulePerformanceIndex(QDate date, long id = CURRENTSCHEDULE) const override;
    /// Cost performance index
    double costPerformanceIndex(long int id, QDate date, bool *error=nullptr) const override;
    
    /**
     * Return the duration that an activity's start can be delayed 
     * without affecting the project completion date. 
     * An activity with positive float is not on the critical path.
     * @param id Schedule identity. If id is CURRENTSCHEDULE, use current schedule.
     */
    Duration positiveFloat(long id = CURRENTSCHEDULE) const;
    void setPositiveFloat(Duration fl, long id = CURRENTSCHEDULE) const;
    /**
     * Return the duration by which the duration of an activity or path 
     * has to be reduced in order to fulfill a timing- or dependency constraint.
     * @param id Schedule identity. If id is CURRENTSCHEDULE, use current schedule.
     */
    Duration negativeFloat(long id = CURRENTSCHEDULE) const;
    void setNegativeFloat(Duration fl, long id = CURRENTSCHEDULE) const;
    /**
     * Return the duration by which an activity can be delayed or extended 
     * without affecting the start of any succeeding activity.
     * @param id Schedule identity. If id is CURRENTSCHEDULE, use current schedule.
     */
    Duration freeFloat(long id = CURRENTSCHEDULE) const;
    void setFreeFloat(Duration fl, long id = CURRENTSCHEDULE) const;
    /**
     * Return the duration from Early Start to Late Start.
     * @param id Schedule identity. If id is CURRENTSCHEDULE, use current schedule.
     */
    Duration startFloat(long id = CURRENTSCHEDULE) const;
    /**
     * Return the duration from Early Finish to Late Finish.
     * @param id Schedule identity. If id is CURRENTSCHEDULE, use current schedule.
     */
    Duration finishFloat(long id = CURRENTSCHEDULE) const;
    
    /**
     * A task is critical if positive float equals 0
     * @param id Schedule identity. If id is CURRENTSCHEDULE, use current schedule.
     */
    bool isCritical(long id = CURRENTSCHEDULE) const override;
    
    /**
     * Set current schedule to schedule with identity id, for me and my children.
     * @param id Schedule identity
     */
    void setCurrentSchedule(long id) override;
    
    /**
     * The assigned resources can not fulfill the estimated effort.
     * @param id Schedule identity. If id is CURRENTSCHEDULE, use current schedule.
     */
    bool effortMetError(long id = CURRENTSCHEDULE) const override;

    /// @return true if this task has been started
    bool isStarted() const;

    Completion &completion() { return m_workPackage.completion(); }
    const Completion &completion() const { return m_workPackage.completion(); }
    
    WorkPackage &workPackage() { return m_workPackage; }
    const WorkPackage &workPackage() const { return m_workPackage; }

    int workPackageLogCount() const { return m_packageLog.count(); }
    QList<WorkPackage*> workPackageLog() const { return m_packageLog; }
    void addWorkPackage(WorkPackage *wp);
    void removeWorkPackage(WorkPackage *wp);
    WorkPackage *workPackageAt(int index) const;

    QString wpOwnerName() const;
    WorkPackage::WPTransmitionStatus wpTransmitionStatus() const;
    DateTime wpTransmitionTime() const;

    /**
     * Returns the state of the task
     * @param id The identity of the schedule used when calculating the state
     */
    uint state(long id = CURRENTSCHEDULE) const override;

    /// Check if this node has any dependent child nodes
    bool isEndNode() const override;
    /// Check if this node has any dependent parent nodes
    bool isStartNode() const override;
    
    QList<Relation*> parentProxyRelations() const { return  m_parentProxyRelations; }
    QList<Relation*> childProxyRelations() const { return  m_childProxyRelations; }

    /**
     * Calculates and returns the duration of the node.
     * Uses the correct expected-, optimistic- or pessimistic effort
     * dependent on @p use.
     * @param time Where to start calculation.
     * @param use Calculate using expected-, optimistic- or pessimistic estimate.
     * @param backward If true, time specifies when the task should end.
     */
    Duration duration(const DateTime &time, int use, bool backward) override;

    /**
     * Return the duration calculated on bases of the estimates calendar
     */
    Duration length(const DateTime &time, Duration duration, bool backward);
    Duration length(const DateTime &time, Duration uration, Schedule *sch, bool backward);

    /// Copy info from parent schedule
    void copySchedule();
    /// Copy intervals from parent schedule
    void copyAppointments();
    /// Copy intervals from parent schedule in the range @p start, @p end
    void copyAppointments(const DateTime &start, const DateTime &end = DateTime());

Q_SIGNALS:
    void workPackageToBeAdded(KPlato::Node *node, int row);
    void workPackageAdded(KPlato::Node *node);
    void workPackageToBeRemoved(KPlato::Node *node, int row);
    void workPackageRemoved(KPlato::Node *node);

public:
    void initiateCalculation(MainSchedule &sch) override;
    /**
     * Sets up the lists used for calculation.
     * This includes adding summarytasks relations to subtasks
     * and lists for start- and endnodes.
     */
    void initiateCalculationLists(MainSchedule &sch) override;
    /**
     * Calculates early start and early finish, first for all predeccessors,
     * then for this task.
     * @param use Calculate using expected-, optimistic- or pessimistic estimate.
     */
    DateTime calculateForward(int use) override;
    /**
     * Calculates ref m_durationForward from ref earliestStart and
     * returns the resulting end time (early finish),
     * which will be used as the successors ref earliestStart.
     *
     * @param use Calculate using expected-, optimistic- or pessimistic estimate.
     */
    DateTime calculateEarlyFinish(int use) override;
    /**
     * Calculates late start and late finish, first for all successors,
     * then for this task.
     * @param use Calculate using expected-, optimistic- or pessimistic estimate.
     */
    DateTime calculateBackward(int use) override;
    /**
     * Calculates ref m_durationBackward from ref latestFinish and
     * returns the resulting start time (late start),
     * which will be used as the predecessors ref latestFinish.
     *
     * @param use Calculate using expected-, optimistic- or pessimistic estimate.
     */
    DateTime calculateLateStart(int use) override;
    /**
     * Schedules the task within the limits of earliestStart and latestFinish.
     * Calculates ref m_startTime, ref m_endTime and ref m_duration,
     * Assumes ref calculateForward() and ref calculateBackward() has been run.
     *
     * @param earliest The task is not scheduled to start earlier than this
     * @param use Calculate using expected-, optimistic- or pessimistic estimate.
     * @return The tasks endtime which can be used for scheduling the successor.
     */
    DateTime scheduleForward(const DateTime &earliest, int use) override;
    /**
     * Schedules the task within the limits of start time and latestFinish,
     * Calculates end time and duration.
     * Assumes ref calculateForward() and ref calculateBackward() has been run.
     *
     * @param use Calculate using expected-, optimistic- or pessimistic estimate.
     * @return The tasks endtime which can be used for scheduling the successor.
     */
    DateTime scheduleFromStartTime(int use) override;
    /**
     * Schedules the task within the limits of earliestStart and latestFinish.
     * Calculates ref m_startTime, ref m_endTime and ref m_duration,
     * Assumes ref calculateForward() and ref calculateBackward() has been run.
     *
     * @param latest The task is not scheduled to end later than this
     * @param use Calculate using expected-, optimistic- or pessimistic estimate.
     * @return The tasks starttime which can be used for scheduling the predeccessor.
     */
    DateTime scheduleBackward(const DateTime &latest, int use) override;
    /**
     * Schedules the task within the limits of end time and latestFinish.
     * Calculates endTime and duration.
     * Assumes ref calculateForward() and ref calculateBackward() has been run.
     *
     * @param use Calculate using expected-, optimistic- or pessimistic estimate.
     * @return The tasks starttime which can be used for scheduling the predeccessor.
     */
    DateTime scheduleFromEndTime(int use) override;
    
    /**
     * Summarytasks (with milestones) need special treatment because 
     * milestones are always 'glued' to their predecessors.
     */
    void adjustSummarytask() override;
    
    /// Calculate the critical path
    bool calcCriticalPath(bool fromEnd) override;
    void calcFreeFloat() override;
    
    // Proxy relations are relations to/from summarytasks. 
    // These relations are distributed to the child tasks before calculation.
    void clearProxyRelations() override;
    void addParentProxyRelations(const QList<Relation*> &) override;
    void addChildProxyRelations(const QList<Relation*> &) override;
    void addParentProxyRelation(Node *, const Relation *) override;
    void addChildProxyRelation(Node *, const Relation *) override;

public:
    DateTime earlyStartDate();
    void setEarlyStartDate(DateTime value);

    DateTime earlyFinishDate();
    void setEarlyFinishDate(DateTime value);

    DateTime lateStartDate();
    void setLateStartDate(DateTime value);

    DateTime lateFinishDate();
    void setLateFinishDate(DateTime value);

    int activitySlack();
    void setActivitySlack(int value);

    int activityFreeMargin();
    void setActivityFreeMargin(int value);

protected:
    /**
     * Return the duration calculated on bases of the requested resources
     */
    Duration calcDuration(const DateTime &time, Duration effort, bool backward);

private:
    DateTime calculateSuccessors(const QList<Relation*> &list, int use);
    DateTime calculatePredeccessors(const QList<Relation*> &list, int use);
    DateTime scheduleSuccessors(const QList<Relation*> &list, int use);
    DateTime schedulePredeccessors(const QList<Relation*> &list, int use);
    
    /// Fixed duration: Returns @p dt
    /// Duration with calendar: Returns first available after @p dt
    /// Has working resource(s) allocated: Returns the earliest time a resource can start work after @p dt, and checks appointments if @p sch is not null.
    DateTime workTimeAfter(const DateTime &dt, Schedule *sch = nullptr) const;
    /// Fixed duration: Returns @p dt
    /// Duration with calendar: Returns first available before @p dt
    /// Has working resource(s) allocated: Returns the latest time a resource can finish work, and checks appointments if @p sch is not null.
    DateTime workTimeBefore(const DateTime &dt, Schedule *sch = nullptr) const;
    
private:
    QList<ResourceGroup*> m_resource;

    QList<Relation*> m_parentProxyRelations;
    QList<Relation*> m_childProxyRelations;
    
    // This list store pointers to linked task
    QList<Node*> m_requiredTasks;

    WorkPackage m_workPackage;
    QList<WorkPackage*> m_packageLog;

    bool m_calculateForwardRun;
    bool m_calculateBackwardRun;
    bool m_scheduleForwardRun;
    bool m_scheduleBackwardRun;
};

}  //KPlato namespace

Q_DECLARE_METATYPE(KPlato::Completion::UsedEffort::ActualEffort)

#ifndef QT_NO_DEBUG_STREAM
PLANKERNEL_EXPORT QDebug operator<<(QDebug dbg, const KPlato::Completion::UsedEffort::ActualEffort &ae);
#endif

#endif
