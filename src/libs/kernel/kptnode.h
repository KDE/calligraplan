/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001 Thomas Zander zander @kde.org
   SPDX-FileCopyrightText: 2004-2011 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTNODE_H
#define KPTNODE_H

#include "plankernel_export.h"

#include "kptglobal.h"
#include "kptrelation.h"
#include "kptduration.h"
#include "kptdatetime.h"
#include "kptschedule.h"
#include "kptdocuments.h"
#include "kptresource.h"

#include <QObject>
#include <QHash>
#include <QString>

#include <KoXmlReaderForward.h>

class QDomElement;


/// The main namespace.
namespace KPlato
{

class Account;
class Project;
class Appointment;
class ResourceGroup;
class Resource;
class ResourceGroupRequest;
class Estimate;
class EffortCostMap;
class EffortCost;
class Calendar;
class KPlatoXmlLoaderBase;
class XmlSaveContext;

/**
 * This class represents any node in the project, a node can be a project or
 * a subproject or any task.
 * This class is basically an abstract interface to make the design more OO.
 */
class PLANKERNEL_EXPORT Node : public QObject
{
    Q_OBJECT
public:
    enum ConstraintType { ASAP, ALAP, MustStartOn, MustFinishOn, StartNotEarlier, FinishNotLater, FixedInterval };

    enum State { 
        State_None = 0,
        State_Started           = 1 << 0,
        State_StartedLate       = 1 << 1,
        State_StartedEarly      = 1 << 2,
        State_Finished          = 1 << 3,
        State_FinishedLate      = 1 << 4,
        State_FinishedEarly     = 1 << 5,
        State_Running           = 1 << 6,
        State_RunningLate       = 1 << 7,
        State_RunningEarly      = 1 << 8,
        State_ReadyToStart      = 1 << 9,  // all preceding tasks finished (if any)
        State_NotReadyToStart   = 1 << 10, // all preceding tasks not finished (must be one or more)
        State_NotScheduled      = 1 << 11,
        State_Late              = 1 << 12,
        State_Baselined         = 1 << 13,
        State_Scheduling        = 1 << 14,
        State_Error             = 1 << 15
    };
    Q_ENUM(State);

    enum Properties {
        NameProperty,
        LeaderProperty,
        DescriptionProperty,
        TypeProperty,
        RunningAccountProperty,
        StartupCostProperty,
        StartupAccountProperty,
        ShutdownCostProperty,
        ShutdownAccountProperty,
        CompletionEntryProperty,
        CompletionStartedProperty,
        CompletionFinishedProperty,
        CompletionStartTimeProperty,
        CompletionFinishTimeProperty,
        CompletionPercentageProperty,
        CompletionRemainingEffortProperty,
        CompletionActualEffortProperty,
        CompletionUsedEffortProperty,
        CompletionNoteProperty,
        ResourceRequestProperty,
        ConstraintTypeProperty,
        StartConstraintProperty,
        EndConstraintProperty,
        PriorityProperty,
        EstimateProperty,
        EstimateRiskProperty,
        EstimateOptimisticProperty,
        EstimatePessimisticProperty
    };
    Q_ENUM(Properties);

    explicit Node(Node *parent = nullptr);
    Node(const Node &node, Node *parent = nullptr);


    // Declare the class abstract
    ~Node() override = 0;

    void setId(const QString& id);
    QString id() const { return m_id; } // unique identity
    
    enum NodeTypes {
	  Type_Node = 0,
	  Type_Project = 1,
	  Type_Subproject = 2,
	  Type_Task = 3,
	  Type_Milestone = 4,
	  Type_Periodic = 5,
      Type_Summarytask = 6
    };

    virtual int type() const = 0;
    
    QString typeToString(bool trans = false) const;
    static QString typeToString(NodeTypes typ, bool trans = false);
    static QStringList typeToStringList(bool trans);
    /**
     * Returns a pointer to the project node (main- or sub-project)
     * Returns 0 if no project exists.
     */
    virtual Node *projectNode();
    /**
     * Returns a pointer to the project node (main- or sub-project)
     * Returns 0 if no project exists.
     */
    virtual const Node *projectNode() const;

    // NOTE: Saving is done here, loading is done using the XmlLoaderObject
    virtual void save(QDomElement &element, const XmlSaveContext &context) const  = 0;
    /// Save me and my childrens relations.
    virtual void saveRelations(QDomElement &element, const XmlSaveContext &context) const;

    /// Save a workpackage document containing @p node with schedule identity @p id
    virtual void saveWorkPackageXML(QDomElement &element, long id) const;

    // simple child node management
    // Child nodes are things like subtasks, basically a task can exists of
    // several sub-tasks. Creating a table has 4 subtasks, 1) measuring
    // 2) cutting 3) building 4) painting.
    Node *parentNode() const { return m_parent; }
    void setParentNode(Node* newParent) { m_parent = newParent;}
    const QList<Node*> &childNodeIterator() const { return m_nodes; }
    int numChildren() const { return m_nodes.count(); }
    virtual void addChildNode(Node *node, Node *after=nullptr);
    virtual void insertChildNode(int index, Node *node);
    void takeChildNode(Node *node);
    void takeChildNode(int number);
    Node* childNode(int number);
    const Node* childNode(int number) const;
    int findChildNode(const Node* node) const;
    bool isChildOf(const Node *node) const;
    int indexOf(const Node *node) const;

    // Time-dependent child-node-management.
    // list all nodes that are dependent upon this one.
    // Building a house requires the table to be finished, therefore the
    // house-building is time dependent on the table-building. So a child
    // of the table-building node is the house-building node.

    int numDependChildNodes() const { return m_dependChildNodes.count(); }
    /// Adds relation of type @p p to both this node and @p node
    virtual void addDependChildNode(Node *node, Relation::Type p=Relation::FinishStart);
    /// Adds relation of type @p p with @p lag to both this node and @p node
    virtual void addDependChildNode(Node *node, Relation::Type p, Duration lag);
    /// Adds @p relation only to this node
    virtual bool addDependChildNode(Relation *relation);
    /// Inserts relation to this node at index @p index and appends relation to @p node
    virtual void insertDependChildNode(unsigned int index, Node *node, Relation::Type p=Relation::FinishStart);
    /**
     * Takes the relation rel from this node only.
     */
    void takeDependChildNode(Relation *rel);
    Relation *getDependChildNode(int number) const {
        return m_dependChildNodes.at(number);
    }
    QList<Relation*> dependChildNodes() const { return m_dependChildNodes; }

    int numDependParentNodes() const { return m_dependParentNodes.count(); }
    /// Adds relation if type @p to both this node and @p node
    virtual void addDependParentNode(Node *node, Relation::Type p=Relation::FinishStart);
    /// Adds relation to both this node and @p node
    virtual void addDependParentNode(Node *node, Relation::Type p, Duration lag);
    /// Adds relation only to this node
    virtual bool addDependParentNode(Relation *relation);
    /// Inserts relation to this node at index and appends relation to @p node
    virtual void insertDependParentNode(unsigned int index, Node *node, Relation::Type p=Relation::FinishStart);
    /**
     * Takes the relation @p rel from this node only.
     */
    void takeDependParentNode(Relation *rel);
    Relation *getDependParentNode(int number) const {
        return m_dependParentNodes.at(number);
    }
    QList<Relation*> dependParentNodes() const { return m_dependParentNodes; }
    QList<Node*> getParentNodes();
    bool isParentOf(const Node *node) const;
    /// Checks if @p node is our predecessor.
    /// Same as isSuccessorof(node, false, true).
    bool isDependChildOf(const Node *node) const;
    /// Checks if @p node is our predecessor.
    /// If @p checkProxis is true, proxy relations are also checked.
    /// If @ checkSummarytasks is true, relations to summarytasks are included.
    bool isSuccessorof(const Node *node, bool checkProxies = true, bool checkSummarytasks = false) const;
    /// If @p checkProxis is true, proxy relations are also checked.
    /// If @ checkSummarytasks is true, relations to summarytasks are included.
    /// Checks if @p node is our successor
    bool isPredecessorof(const Node *node, bool checkProxies = false, bool checkSummarytasks = false) const;
    virtual bool canMoveTo(const Node *newParent) const;
    
    Relation *findParentRelation(const Node *node) const;
    Relation *findChildRelation(const Node *node) const;
    Relation *findRelation(const Node *node) const;

    /// Set the scheduled start time
    void setStartTime(const DateTime &startTime, long id = CURRENTSCHEDULE);
    /// Return the scheduled start time
    virtual DateTime startTime(long id = CURRENTSCHEDULE) const;
    /// Set the scheduled end time
    void setEndTime(const DateTime &endTime, long id = CURRENTSCHEDULE);
    /// Return the scheduled end time
    virtual DateTime endTime(long id = CURRENTSCHEDULE) const;
    /// Set the scheduled duration
    void setDuration(const Duration &duration, long id = CURRENTSCHEDULE);

    DateTime appointmentStartTime(long id = CURRENTSCHEDULE) const;
    DateTime appointmentEndTime(long id = CURRENTSCHEDULE) const;

    /// Return the estimate for this node
    Estimate *estimate() const { return m_estimate; }

    /**
     * Instead of using the expected duration, generate a random value using
     * the Distribution of each Task. This can be used for Monte-Carlo
     * estimation of Project duration.
     */
    virtual Duration *getRandomDuration() = 0;
    /**
     * Calculate the delay of this node. 
     * It is the difference between the actual startTime and scheduled startTime.
     */
    Duration *getDelay(); // TODO


    QString &name() { return m_name; }
    QString &leader() { return m_leader; }
    QString &description() { return m_description; }
    const QString &name() const { return m_name; }
    const QString &leader() const { return m_leader; }
    const QString &description() const { return m_description; }
    void setName(const QString &n);
    void setLeader(const QString &l);
    void setDescription(const QString &d);

    void setConstraint(Node::ConstraintType type);
    void setConstraint(const QString &type);
    int constraint() const { return m_constraint; }
    QString constraintToString(bool trans=false) const;
    static QStringList constraintList(bool trans);
    
    virtual void setConstraintStartTime(const DateTime &time)
        { m_constraintStartTime = time; changed(this, StartConstraintProperty); }
    virtual void setConstraintEndTime(const DateTime &time)
        { m_constraintEndTime = time; changed(this, EndConstraintProperty); }

    virtual DateTime constraintStartTime() const { return m_constraintStartTime; }
    virtual DateTime constraintEndTime() const { return m_constraintEndTime; }
    virtual DateTime startNotEarlier() const { return m_constraintStartTime; }
    virtual DateTime finishNotLater() const { return m_constraintEndTime; }
    virtual DateTime mustStartOn() const { return m_constraintStartTime; }
    virtual DateTime mustFinishOn() const { return m_constraintEndTime; }

    virtual ResourceGroupRequest *resourceGroupRequest(const ResourceGroup * /*group*/) const { return nullptr; }
    virtual QStringList requestNameList() const { return QStringList(); }
    virtual bool containsRequest(const QString &/*identity*/) const { return false; }
    virtual ResourceRequest *resourceRequest(const QString &/*name*/) const { return nullptr; }
    
    /// Return the list of resources assigned to this task
    virtual QStringList assignedNameList(long /*id*/ = CURRENTSCHEDULE) const { return QStringList(); }
    
    virtual void makeAppointments();
    /// Calculates if the assigned resource is overbooked 
    /// within the duration of this node
    virtual void calcResourceOverbooked();

    /// Return the scheduling status of schedule @p id. If @p trans is true, text is translated
    QStringList schedulingStatus(long id, bool trans = false) const;
    /// EstimateType == Estimate, but no resource is requested
    bool resourceError(long id = CURRENTSCHEDULE) const;
    /// The assigned resource is overbooked
    virtual bool resourceOverbooked(long id = CURRENTSCHEDULE) const;
    /// The requested resource is not available
    bool resourceNotAvailable(long id = CURRENTSCHEDULE) const;
    /// The task cannot be scheduled to fulfill all the constraints
    virtual bool constraintError(long id = CURRENTSCHEDULE) const;
    /// The task cannot be scheduled correctly
    virtual bool schedulingError(long id = CURRENTSCHEDULE) const;
    /// The node has not been scheduled
    bool notScheduled(long id = CURRENTSCHEDULE) const;
    /// Return a list of overbooked resources
    virtual QStringList overbookedResources(long id = CURRENTSCHEDULE) const;
    /// The assigned resources can not fulfill the estimated effort.
    virtual bool effortMetError(long /*id*/ = CURRENTSCHEDULE) const { return false; }
    
    virtual EffortCostMap plannedEffortCostPrDay(QDate start, QDate end, long id = CURRENTSCHEDULE, EffortCostCalculationType = ECCT_All) const=0;
    virtual EffortCostMap plannedEffortCostPrDay(const Resource *resource, QDate start, QDate end, long id = CURRENTSCHEDULE, EffortCostCalculationType = ECCT_All) const=0;

    /// Returns the total planned effort for @p resource on this task (or subtasks)
    virtual Duration plannedEffort(const Resource *resource, long id = CURRENTSCHEDULE, EffortCostCalculationType = ECCT_All) const;
    /// Returns the total planned effort for this task (or subtasks) 
    virtual Duration plannedEffort(long id = CURRENTSCHEDULE, EffortCostCalculationType = ECCT_All) const { Q_UNUSED(id); return Duration::zeroDuration; }
    /// Returns the total planned effort for this task (or subtasks) on date
    virtual Duration plannedEffort(QDate , long id = CURRENTSCHEDULE, EffortCostCalculationType = ECCT_All) const { Q_UNUSED(id); return Duration::zeroDuration; }
    /// Returns the total planned effort for @p resource on this task (or subtasks) on date
    virtual Duration plannedEffort(const Resource *resource, QDate date, long id = CURRENTSCHEDULE, EffortCostCalculationType = ECCT_All) const;
    /// Returns the planned effort up to and including date
    virtual Duration plannedEffortTo(QDate , long id = CURRENTSCHEDULE, EffortCostCalculationType = ECCT_All) const { Q_UNUSED(id); return Duration::zeroDuration; }
    /// Returns the planned effort for @p resource up to and including date
    virtual Duration plannedEffortTo(const Resource *resource, QDate date, long id = CURRENTSCHEDULE, EffortCostCalculationType = ECCT_All) const;
    
    /// Returns the total actual effort for this task (or subtasks) 
    virtual Duration actualEffort() const { return Duration::zeroDuration; }
    /// Returns the total actual effort for this task (or subtasks) on date
    virtual Duration actualEffort(QDate /*date*/) const { return Duration::zeroDuration; }
    /// Returns the total actual effort for this task (or subtasks) up to and including date
    virtual Duration actualEffortTo(QDate /*date*/) const { return Duration::zeroDuration; }
    virtual EffortCostMap actualEffortCostPrDay(QDate start, QDate end, long id = CURRENTSCHEDULE, EffortCostCalculationType = ECCT_All) const=0;
    /// Returns the actual effort and cost pr day used by @p resource
    virtual EffortCostMap actualEffortCostPrDay(const Resource *resource, QDate start, QDate end, long id = CURRENTSCHEDULE, EffortCostCalculationType = ECCT_All) const=0;
    
    /**
     * Planned cost is the sum total of all resources and other costs
     * planned for this node.
     */
    virtual EffortCost plannedCost(long id = CURRENTSCHEDULE, EffortCostCalculationType = ECCT_All) const;
    
    /**
     * Planned cost from start of activity up to and including date
     * is the sum of all resource costs and other costs planned for this node.
     */
    virtual double plannedCostTo(QDate /*date*/, long id = CURRENTSCHEDULE, EffortCostCalculationType = ECCT_All) const { Q_UNUSED(id); return 0; }
    /// Actual cost up to and including date
    virtual EffortCost actualCostTo(long int /*id*/, QDate /*date*/) const { return EffortCost(); }
    
    /// Returns the effort planned to be used to reach the actual percent finished
    virtual Duration budgetedWorkPerformed(QDate , long = CURRENTSCHEDULE) const = 0;
    /// Returns the cost planned to be used to reach the actual percent finished
    virtual double budgetedCostPerformed(QDate , long = CURRENTSCHEDULE) const { return 0.0; };

    /// Return map of Budgeted Cost of Work Scheduled pr day
    virtual EffortCostMap bcwsPrDay(long id = CURRENTSCHEDULE, EffortCostCalculationType type = ECCT_All);
    /// Return map of Budgeted Cost of Work Scheduled pr day
    virtual EffortCostMap bcwsPrDay(long id = CURRENTSCHEDULE, EffortCostCalculationType type = ECCT_All) const;
    /// Budgeted Cost of Work Scheduled
    virtual double bcws(QDate /*date*/, long id = CURRENTSCHEDULE) const { Q_UNUSED(id); return 0.0; }

    /// Return map of Budgeted Cost of Work Scheduled pr day (also includes bcws pr day)
    virtual EffortCostMap bcwpPrDay(long id = CURRENTSCHEDULE, EffortCostCalculationType type = ECCT_All);
    /// Return map of Budgeted Cost of Work Scheduled pr day (also includes bcws pr day)
    virtual EffortCostMap bcwpPrDay(long id = CURRENTSCHEDULE, EffortCostCalculationType type = ECCT_All) const;
    /// Budgeted Cost of Work Performed
    virtual double bcwp(long id) const { Q_UNUSED(id); return 0.0; }
    /// Budgeted Cost of Work Performed (up to @p date)
    virtual double bcwp(QDate /*date*/, long id = CURRENTSCHEDULE) const { Q_UNUSED(id); return 0.0; }

    /// Return a map of Actual effort and Cost of Work Performed
    virtual EffortCostMap acwp(long id = CURRENTSCHEDULE, EffortCostCalculationType type = ECCT_All);
    /// Return a map of Actual effort and Cost of Work Performed
    virtual EffortCostMap acwp(long id = CURRENTSCHEDULE, EffortCostCalculationType type = ECCT_All) const;
    /// Return Actual effort and Cost of Work Performed upto @date
    virtual EffortCost acwp(QDate date, long id = CURRENTSCHEDULE) const;

    /// Effort based performance index
    virtual double effortPerformanceIndex(QDate /*date*/, long /*id*/ = CURRENTSCHEDULE) const { return 0.0; }
    /// Schedule performance index
    virtual double schedulePerformanceIndex(QDate /*date*/, long /*id*/ = CURRENTSCHEDULE) const { return 0.0; }
    /// Cost performance index
    virtual double costPerformanceIndex(long int /*id*/, QDate /*date*/, bool * /*error=0*/) const { return 0.0; }
    
    virtual void initiateCalculationLists(MainSchedule &sch) = 0;
    virtual DateTime calculateForward(int /*use*/) = 0;
    virtual DateTime calculateEarlyFinish(int /*use*/) { return DateTime(); }
    virtual DateTime calculateBackward(int /*use*/) = 0;
    virtual DateTime calculateLateStart(int /*use*/) { return DateTime(); }
    virtual DateTime scheduleForward(const DateTime &, int /*use*/) = 0;
    virtual DateTime scheduleFromStartTime(int /*use*/) { return DateTime(); }
    virtual DateTime scheduleBackward(const DateTime &, int /*use*/) = 0;
    virtual DateTime scheduleFromEndTime(int /*use*/) { return DateTime(); }
    virtual void adjustSummarytask() = 0;

    /// Returns the (previously) calculated duration
    Duration duration(long id = CURRENTSCHEDULE) const;
    
    /**
     * The variance is calculated based on
     * the optimistic/pessimistic ratio specified for the estimate.
     */
    double variance(long id = CURRENTSCHEDULE, Duration::Unit unit = Duration::Unit_ms) const;
    /**
     * The standard deviation is calculated based on 
     * the optimistic/pessimistic ratio specified for the estimate.
     */
    double deviation(long id = CURRENTSCHEDULE, Duration::Unit unit = Duration::Unit_ms) const;
    
    Node *siblingBefore();
    Node *childBefore(Node *node);
    Node *siblingAfter();
    Node *childAfter(Node *node);
    bool moveChildUp(Node *node);
    bool moveChildDown(Node *node);
    
    /// Check if this node can be linked to node
    bool legalToLink(const Node *node) const;
    /// Check if node par can be linked to node child. (Reimplement)
    virtual bool legalToLink(const Node *, const Node *) const { return false; }

    /// Save appointments for schedule with id
    virtual void saveAppointments(QDomElement &element, long id) const;
    ///Return the list of appointments for schedule with id.
    QList<Appointment*> appointments(long id = CURRENTSCHEDULE);
    /// Adds appointment to this node only (not to resource)
    virtual bool addAppointment(Appointment *appointment, Schedule &main);
    
    /// Return list of all resources with appointments to this task for schedule with @p id.
    QList<Resource*> assignedResources(long id) const;
    
    /// Find the node with my id
    virtual Node *findNode() const { return findNode(m_id); }
    /// Find the node with identity id
    virtual Node *findNode(const QString &id) const
        { return (m_parent ? m_parent->findNode(id) : nullptr); }
    /// Remove myself from the id register
    virtual bool removeId()  { return removeId(m_id); }
    /// Remove the registered identity @p id
    virtual bool removeId(const QString &id)
        { return (m_parent ? m_parent->removeId(id) : false); }

    /**
     * This is when work can start on this node in accordance with 
     * the calendar of allocated resources. Normally this is the same
     * as @ref startTime(), but may differ if timing constraints are set.
     */
    virtual DateTime workStartTime(long id = CURRENTSCHEDULE) const;
    void setWorkStartTime(const DateTime &dt,  long id = CURRENTSCHEDULE);
    
    /**
     * This is when work can finish on this node in accordance with 
     * the calendar of allocated resources. Normally this is the same
     * as @ref endTime(), but may differ if timing constraints are set.
     */
    virtual DateTime workEndTime(long id = CURRENTSCHEDULE) const;
    void setWorkEndTime(const DateTime &dt,  long id = CURRENTSCHEDULE);
    
    /// Returns true if this node is critical
    virtual bool isCritical(long id = CURRENTSCHEDULE) const { Q_UNUSED(id); return false; }
    /// Returns true if this node is in a critical path
    virtual bool inCriticalPath(long id = CURRENTSCHEDULE) const;
    
    /// Returns the level this node is in the hierarchy. Top node is level 0.
    virtual int level() const;
    /// Generate WBS Code
    virtual QString generateWBSCode(QList<int> &indexes, bool sortable = false) const;
    /// Returns the Work Breakdown Structure Code
    QString wbsCode(bool sortable = false) const;
    
    double startupCost() const { return m_startupCost; }
    void setStartupCost(double cost);
    
    Account *startupAccount() const { return m_startupAccount; }
    void setStartupAccount(Account *acc);
    
    double shutdownCost() const { return m_shutdownCost; }
    void setShutdownCost(double cost);
    
    Account *shutdownAccount() const { return m_shutdownAccount; }
    void setShutdownAccount(Account *acc);
    
    Account *runningAccount() const { return m_runningAccount; }
    void setRunningAccount(Account *acc);

    bool isBaselined(long int id = BASELINESCHEDULE) const;
    /**
     * Return schedule with @p id
     * If @p id == CURRENTSCHEDULE, return m_currentSchedule
     * Return 0 if schedule with @p id doesn't exist.
     */
    Schedule *schedule(long id = CURRENTSCHEDULE) const;
    /// Return current schedule
    Schedule *currentSchedule() const { return m_currentSchedule; }
    /// Set current schedule to schedule with identity @p id, for me and my children
    virtual void setCurrentSchedule(long id);
    /// Return true if this node has a valid schedule with identity == @p id
    bool isScheduled(long id = CURRENTSCHEDULE) const;
    /// Return the list of schedules for this node
    QHash<long, Schedule*> &schedules() { return m_schedules; }
    QHash<long, Schedule*> schedules() const { return m_schedules; }
    /// Find schedule matching name and type. Does not return deleted schedule.
    Schedule *findSchedule(const QString &name, const Schedule::Type type);
    /// Find schedule matching name. Does not return deleted schedule.
    Schedule *findSchedule(const QString &name);
    /// Find schedule matching type.  Does not return deleted schedule.
    Schedule *findSchedule(const Schedule::Type type);
    /// Find schedule matching id.  Also returns deleted schedule.
    Schedule *findSchedule(long id) const;
    
    /// Take, don't delete (as in destruct).
    void takeSchedule(const Schedule *schedule);
    /// Add schedule to list, replace if schedule with same id already exists.
    void addSchedule(Schedule *schedule);
    /// Create a new schedule.
    Schedule *createSchedule(const QString& name, Schedule::Type type, long id);
    /// Create a new schedule.
    Schedule *createSchedule(Schedule *parent);
    
    /// Set deleted = onoff for schedule with id
    void setScheduleDeleted(long id, bool onoff);
    /// Set parent schedule recursively
    virtual void setParentSchedule(Schedule *sch);
    
    const ResourceRequestCollection &requests() const { return m_requests; }
    ResourceRequestCollection &requests() { return m_requests; }
    
    virtual uint state(long id = ANYSCHEDULED) const { Q_UNUSED(id) return State_None; }
    
    const Documents &documents() const { return m_documents; }
    Documents &documents() { return m_documents; }

    virtual void emitDocumentAdded(Node *node, Document *doc, int idx);
    virtual void emitDocumentRemoved(Node *node, Document *doc, int idx);
    virtual void emitDocumentChanged(Node *node, Document *doc, int idx);
    
    void blockChanged(bool on = true);

    /// @return priority
    int priority() const;

    /// @return scheduling granularity
    virtual ulong granularity() const;

public:
    // These shouldn't be available to other than those who inherits
    /// Calculate the critical path
    virtual bool calcCriticalPath(bool fromEnd);
    virtual void calcFreeFloat();
    
    /// Check if this node has any dependent child nodes
    virtual bool isEndNode() const;
    /// Check if this node has any dependent parent nodes
    virtual bool isStartNode() const;
    
    virtual void initiateCalculation(MainSchedule &sch);
    virtual void resetVisited();
    void propagateEarliestStart(DateTime &time);
    void propagateLatestFinish(DateTime &time);
    void moveEarliestStart(DateTime &time);
    void moveLatestFinish(DateTime &time);
    // Reimplement this
    virtual Duration summarytaskDurationForward(const DateTime &/*time*/) 
    { return Duration::zeroDuration; }
    // Reimplement this
    virtual DateTime summarytaskEarliestStart() 
    { return DateTime(); }
    // Reimplement this
    virtual Duration summarytaskDurationBackward(const DateTime &/*time*/) 
    { return Duration::zeroDuration; }
    // Reimplement this
    virtual DateTime summarytaskLatestFinish() 
    { return DateTime(); }
    
    /**
     * earlyStart() returns earliest time this node can start
     * given the constraints of the network.
     */
    DateTime earlyStart(long id = CURRENTSCHEDULE) const;
    /**
     * setEarlyStart() sets earliest time this node can start
     */
    void setEarlyStart(const DateTime &dt, long id = CURRENTSCHEDULE);
    /**
     * lateStart() returns latest time this node can start
     * given the constraints of the network.
     */
    DateTime lateStart(long id = CURRENTSCHEDULE) const;
    /**
     * setLateStart() sets the earliest time this node can start
     */
    void setLateStart(const DateTime &dt, long id = CURRENTSCHEDULE);
    /**
     * earlyFinish() returns earliest time this node can finish
     * given the constraints of the network.
     */
    DateTime earlyFinish(long id = CURRENTSCHEDULE) const;
    /**
     * setEarlyFinish() sets earliest time this node can finish
     */
    void setEarlyFinish(const DateTime &dt, long id = CURRENTSCHEDULE);
    /**
     * lateFinish() returns latest time this node can finish
     * given the constraints of the network.
     */
    DateTime lateFinish(long id = CURRENTSCHEDULE) const;
    /**
     * setLateFinish() sets latest time this node can finish
     */
    void setLateFinish(const DateTime &dt, long id = CURRENTSCHEDULE);
    
    /// Adds appointment to both this node and resource
    virtual void addAppointment(ResourceSchedule *resource, const DateTime &start, const DateTime &end, double load=100);
    
    virtual void clearProxyRelations() {}
    virtual void addParentProxyRelations(const QList<Relation*> &) {}
    virtual void addChildProxyRelations(const QList<Relation*> &) {}
    virtual void addParentProxyRelation(Node *, const Relation *) {}
    virtual void addChildProxyRelation(Node *, const Relation *) {}
    const QList<Relation*> &parentProxyRelations() const { return m_parentProxyRelations; }
    const QList<Relation*> &childProxyRelations() const { return m_childProxyRelations; }

    virtual void changed(int property = -1) { changed(this, property); }
    Duration getmDurationForward(){ return this->m_durationForward;}

public Q_SLOTS:
    void slotStandardWorktimeChanged(KPlato::StandardWorktime*);
    void setPriority(int priority);

protected:
    friend class KPlatoXmlLoaderBase;
    friend class ProjectLoaderBase;
    /**
     * Calculates and returns the duration of the node.
     * Reimplement.
     */
    virtual Duration duration(const DateTime &/*time*/, int /*use*/, bool /*backward*/)
        { return Duration::zeroDuration; }

    // NOTE: Cannot use setCurrentSchedule() due to overload/casting problems
    void setCurrentSchedulePtr(Schedule *schedule) { m_currentSchedule = schedule; }
    virtual void changed(Node *node, int property = -1);
    
    QList<Node*> m_nodes;
    QList<Relation*> m_dependChildNodes;
    QList<Relation*> m_dependParentNodes;
    QList<Node*>m_parentNodes;
    Node *m_parent;
    

    QString m_id; // unique id
    QString m_name;        // Name of this node
    QString m_leader;      // Person or group responsible for this node
    QString m_description; // Description of this node

    Estimate *m_estimate;
    

    ConstraintType m_constraint;

    /**
      * m_constraintTime is used if any of the constraints
      * FixedInterval, StartNotEarlier, MustStartOn or FixedInterval is selected
      */
    DateTime m_constraintStartTime;
    /**
      * m_constraintEndTime is used if any of the constraints
      * FixedInterval, FinishNotLater, MustFinishOn or FixedInterval is selected
      */
    DateTime m_constraintEndTime;

    bool m_visitedForward;
    bool m_visitedBackward;
    Duration m_durationForward;
    Duration m_durationBackward;
    DateTime m_earlyStart;
    DateTime m_earlyFinish;
    DateTime m_lateFinish;
    
    QHash<long, Schedule*> m_schedules;
    Schedule *m_currentSchedule;

    double m_startupCost;
    Account *m_startupAccount;
    double m_shutdownCost;
    Account *m_shutdownAccount;
    Account *m_runningAccount;
    
    Documents m_documents;
    
    ResourceRequestCollection m_requests;

    int m_priority;

    QList<Relation*> m_parentProxyRelations;
    QList<Relation*> m_childProxyRelations;

private:
    void init();
    bool m_blockChanged;
};

////////////////////////////////   Estimate   ////////////////////////////////
/**
  * The Estimate class stores how much time (or effort) it takes to complete a Task.
  * The estimate which is needed to complete the task is not simply a single value but
  * is stored as an optimistic, a pessimistic and an expected value.
  * With statistical calculations using the PERT distribution, one can arrive at a more 
  * realistic estimate than when using the expected value alone.
  */
class PLANKERNEL_EXPORT Estimate {
public:
    /// Constructor
    explicit Estimate(Node *parent = nullptr);
    /// Copy constructor.
    Estimate (const Estimate &estimate, Node *parent = nullptr);
    /// Destructor
    ~Estimate();

    /// Reset estimate
    void clear();
    
    /// Copy values from @p estimate
    Estimate &operator=(const Estimate &estimate);
    
    /// Type defines the types of estimates
    enum Type {
        Type_Effort,        /// Changing amount of resources changes the task duration
        Type_Duration       /// Changing amount of resources will not change the tasks duration
    };
    /// Return the node that owns this Estimate
    Node *parentNode() const { return m_parent; }
    /// Set the node that owns this Estimate
    void setParentNode(Node* parent) { m_parent = parent; }
    
    /// Return estimate Type
    Type type() const { return m_type; }
    /// Set estimate type to @p type
    void setType(Type type);
    /// Set estimate type to type represented by the string @p type
    void setType(const QString& type);
    /// Return estimate type as a string. If @p trans is true, it's translated
    QString typeToString(bool trans=false) const;
    /// Return estimate type @p typ as a string. If @p trans is true, it's translated
    static QString typeToString(Estimate::Type typ, bool trans=false);
    /// Return a stringlist of all estimate types. Translated if @p trans = true.
    static QStringList typeToStringList(bool trans=false);
    
    /// Return the calendar used when Type is Duration
    Calendar *calendar() const { return m_calendar; }
    /// Set the calendar to be used when Type is Duration
    void setCalendar(Calendar *calendar);
    
    enum Risktype { Risk_None, Risk_Low, Risk_High };
    Risktype risktype() const { return m_risktype; }
    void setRisktype(Risktype type);
    void setRisktype(const QString& type);
    QString risktypeToString(bool trans=false) const;
    static QStringList risktypeToStringList(bool trans=false);

    /// Use defines which value to access
    enum Use { Use_Expected=0, Use_Optimistic=1, Use_Pessimistic=2 };
    
    /// Return estimate (scaled) of type @p valueType.
    /// If @p pert is true the pert value is calculated and returned
    Duration value(int valueType, bool pert) const;
    
    /// Return unscaled value
    Duration optimisticValue() const;
    /// Return unscaled value
    Duration pessimisticValue() const;
    /// Return unscaled value
    Duration expectedValue() const;

    /// The unit in which the estimates were entered.
    Duration::Unit unit() const { return m_unit; }
    /// Set display unit.
    void setUnit(Duration::Unit unit);
    
    /// Return the expected estimate (normally entered by user)
    double expectedEstimate() const { return m_expectedEstimate; }
    /// Return the optimistic estimate (normally entered by user)
    double optimisticEstimate() const { return m_optimisticEstimate; }
    /// Return the pessimistic estimate (normally entered by user)
    double pessimisticEstimate() const { return m_pessimisticEstimate; }
    /// Set the expected estimate
    void setExpectedEstimate(double value);
    /// Set the optimistic estimate
    void setOptimisticEstimate(double value);
    /// Set the pessimistic estimate
    void setPessimisticEstimate(double value);

    /**
     * Set the optimistic estimate as a deviation from "expected" in percent
     * @param percent should be a negative value.
     */
    void setOptimisticRatio(int percent);
    /**
     * Return the "optimistic" estimate as deviation from "expected" in percent.
     * This should be a negative value.
     */
    int optimisticRatio() const;
    /**
     * Set the pessimistic estimate as a deviation from "expected" in percent
     * @param percent should be a positive value.
     */
    void setPessimisticRatio(int percent);
    /**
     * Return the "pessimistic" estimate as the deviation from "expected" in percent.
     * This should be a positive value.
     */
    int pessimisticRatio() const;
    
    /**
     * The variance is calculated based on
     * the optimistic/pessimistic estimates, scaled to current unit.
     */
    double variance() const;
    /**
     * The variance is calculated based on
     * the optimistic/pessimistic estimates, scaled to @p unit
     */
    double variance(Duration::Unit unit) const;
    /**
     * The standard deviation is calculated based on 
     * the optimistic/pessimistic estimates, scaled to current unit.
     */
    double deviation() const;
    /**
     * The standard deviation is calculated based on 
     * the optimistic/pessimistic estimates, scaled to @p unit
     */
    double deviation(Duration::Unit unit) const;
    
    /// Returns the expected duration. Calculated based on the estimates expected, optimistic and pessimistic
    Duration pertExpected() const;
    /// Returns the most optimistic duration. Calculated based on the estimates expected, optimistic and pessimistic
    Duration pertOptimistic() const;
    /// Returns the most pessimistic duration. Calculated based on the estimates expected, optimistic and pessimistic
    Duration pertPessimistic() const;
    
    /// Convert the duration @p value (in milliseconds) to a value in @p unit, using the scaling factors in @p scales
    static double scale(const Duration &value, Duration::Unit unit, const QList<qint64> &scales);
    /// Convert the duration @p value (in @p unit) to a value in milliseconds (base unit), using the scaling factors in @p scales
    static Duration scale(double value, Duration::Unit unit, const QList<qint64> &scales);

    /// Return a list of default scales scaling factors
    static QList<qint64> defaultScales();
    /// Return a list of scaling factors fetched from the projects standard worktime
    QList<qint64> scales() const;
    
    /// Load from xml document
    bool load(KoXmlElement &element, XMLLoaderObject &status);
    /// Save to xml document
    void save(QDomElement &element) const;

protected:
    /// Set (calculate) cached value
    void setOptimisticValue();
    /// Set (calculate) cached value
    void setExpectedValue();
    /// Set (calculate) cached value
    void setPessimisticValue();
    /// Notify parent of changes
    void changed(int property) { if (m_parent) m_parent->changed(property); }
    /// Copy @p estimate, parentNode is not copied
    void copy(const Estimate &estimate);
    
private:
    friend class Node;
    Node *m_parent;
    /// Holds the unit entered by user
    Duration::Unit m_unit;
    /// Holds the value entered by user, in unit m_unit
    double m_expectedEstimate;
    /// Holds the value entered by user, in unit m_unit
    double m_optimisticEstimate;
    /// Holds the value entered by user, in unit m_unit
    double m_pessimisticEstimate;

    mutable bool m_expectedCached, m_optimisticCached, m_pessimisticCached, m_pertCached;
    /// Cached value in base unit (milliseconds)
    Duration m_expectedValue;
    /// Cached value in base unit (milliseconds)
    Duration m_optimisticValue;
    /// Cached value in base unit (milliseconds)
    Duration m_pessimisticValue;
    /// Cached pert expected value
    mutable Duration m_pertExpected;

    Type m_type;
    Risktype m_risktype;
    
    /// Calendar may be used when Type is Type_Duration
    Calendar *m_calendar;
};

}  //KPlato namespace

#endif
