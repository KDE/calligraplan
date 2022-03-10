/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001 Thomas Zander zander @kde.org
   SPDX-FileCopyrightText: 2004-2007 Dag Andersen <dag.andersen@kdemail.net>
   SPDX-FileCopyrightText: 2007 Florian Piquemal <flotueur@yahoo.fr>
   SPDX-FileCopyrightText: 2007 Alexis Ménard <darktears31@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTTASK_H
#define KPTTASK_H

#include "plankernel_export.h"

#include "kptnode.h"
#include "kptglobal.h"
#include "kptdatetime.h"
#include "kptduration.h"
#include "kptresource.h"
#include "WorkPackage.h"

#include <QList>
#include <QMap>
#include <utility>

/// The main namespace.
namespace KPlato
{

class Completion;
class XmlSaveContext;
class Appointment;

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

    /// Return task type. Can be Type_Task, Type_Summarytask, or Type_Milestone.
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

    Completion &completion();
    const Completion &completion() const;
    
    WorkPackage &workPackage();
    const WorkPackage &workPackage() const;

    int workPackageLogCount() const;
    QList<WorkPackage*> workPackageLog() const;
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

    QList<Relation*> parentProxyRelations() const;
    QList<Relation*> childProxyRelations() const;

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
    void copyAppointmentsFromParentSchedule(const DateTime &start, const DateTime &end = DateTime());
    /// Creates appointments based on completion data and merges them to current schedule
    void createAndMergeAppointmentsFromCompletion();

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

#endif
