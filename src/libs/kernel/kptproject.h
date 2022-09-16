/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2001 Thomas Zander zander @kde.org
  SPDX-FileCopyrightText: 2004-2010 Dag Andersen <dag.andersen@kdemail.net>
  SPDX-FileCopyrightText: 2007 Florian Piquemal <flotueur@yahoo.fr>
  SPDX-FileCopyrightText: 2007 Alexis Ménard <darktears31@gmail.com>
  SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
  
  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTPROJECT_H
#define KPTPROJECT_H

#include "plankernel_export.h"

#include "kptnode.h"

#include "kptglobal.h"
#include "kptaccount.h"
#include "kptcalendar.h"
#include "kptdatetime.h"
#include "kptduration.h"
#include "kptresource.h"
#include "kptwbsdefinition.h"
#include "kptconfigbase.h"

#include <QMap>
#include <QList>
#include <QHash>
#include <QPointer>
#include <QTimeZone>
#include <QPointer>

/// The main namespace.
namespace KPlato
{

class Locale;
class Schedule;
class StandardWorktime;
class ScheduleManager;
class XMLLoaderObject;
class Task;
class SchedulerPlugin;
class KPlatoXmlLoaderBase;
class XmlSaveContext;

/**
 * Project is the main node in a project, it contains child nodes and
 * possibly sub-projects. A sub-project is just another instantiation of this
 * node however.
 *
 * A note on timezones:
 * To be able to handle resources working in different timezones and
 * to facilitate data exchange with other applications like PIMs or
 * and groupware servers, the project has a timezone that is used for
 * all datetimes in nodes and schedules.
 * By default the local timezone is used.
 *
 * A resources timezone is defined by the associated calendar.
 *
 * Note that a projects datetimes are always displayed/modified in the timezone
 * it was originally created, not necessarily in your current local timezone.
 */
class PLANKERNEL_EXPORT Project : public Node
{
    Q_OBJECT
public:
    explicit Project(Node *parent = nullptr);
    explicit Project(ConfigBase &config, Node *parent = nullptr);
    explicit Project(ConfigBase &config, bool useDefaultValues, Node *parent = nullptr);

    ~Project() override;

    /// Reference this project.
    void ref() { ++m_refCount; }
    /// De-reference this project. Deletes project if ref count <= 0
    void deref();

    /// Returns the node type. Can be Type_Project or Type_Subproject.
    int type() const override;

    uint status(const ScheduleManager *sm = nullptr) const;
    /**
     * Calculate the schedules managed by the schedule manager
     *
     * @param sm Schedule manager
     */
    void calculate(ScheduleManager &sm);

    /**
     * Re-calculate the schedules managed by the schedule manager
     *
     * @param sm Schedule manager
     * @param dt The datetime from when the schedule shall be re-calculated
     */
    void calculate(ScheduleManager &sm, const DateTime &dt);

    DateTime startTime(long id = -1) const override;
    DateTime endTime(long id = -1) const override;

    /// Returns the calculated duration for schedule @p id
    Duration duration(long id = -1) const;
    using Node::duration;
    /**
     * Instead of using the expected duration, generate a random value using
     * the Distribution of each Task. This can be used for Monte-Carlo
     * estimation of Project duration.
     */
    Duration *getRandomDuration() override;

    bool load(KoXmlElement &element, XMLLoaderObject &status) override;
    void save(QDomElement &element, const XmlSaveContext &context) const override;

    using Node::saveWorkPackageXML;
    /// Save a workpackage document containing @p node with schedule identity @p id
    void saveWorkPackageXML(QDomElement &element, const Node *node, long id) const;

    /**
     * Add the node @p task to the project, after node @p position
     * If @p position is zero or the project node, it will be added to this project.
     */
    bool addTask(Node* task, Node* position);
    /**
     * Add the node @p task to the @p parent
     */
    bool addSubTask(Node* task, Node* parent);
    /**
     * Add the node @p task to @p parent, in position @p index
     * If @p parent is zero, it will be added to this project.
     */
    bool addSubTask(Node* task, int index, Node* parent, bool emitSignal = true);
    /**
     * Remove the @p node.
     * The node is not deleted.
     */
    void takeTask(Node *node, bool emitSignal = true);
    bool canMoveTask(Node* node, Node *newParent, bool checkBaselined = false);
    bool moveTask(Node* node, Node *newParent, int newPos);
    bool canIndentTask(Node* node);
    bool indentTask(Node* node, int index = -1);
    bool canUnindentTask(Node* node);
    bool unindentTask(Node* node);
    bool canMoveTaskUp(Node* node);
    bool moveTaskUp(Node* node);
    bool canMoveTaskDown(Node* node);
    bool moveTaskDown(Node* node);
    /**
     * Create a task with a unique id.
     * The task is not added to the project. Do this with addSubTask().
     */
    Task *createTask();
    /**
     * Create a copy of @p def with a unique id.
     * The task is not added to the project. Do this with addSubTask().
     */
    Task *createTask(const Task &def);

    /**
     * Allocate resources marked 'default' (if any)
     * @param task
     */
    void allocateDefaultResources(Task *task) const;

    /// @return true if any of the tasks has been started
    bool isStarted() const;

    int resourceGroupCount() const { return m_resourceGroups.count(); }
    const QList<ResourceGroup *> &resourceGroups() const;
    QList<ResourceGroup*> allResourceGroups() const;
    /// Adds the resource group to the project.
    virtual void addResourceGroup(ResourceGroup *resource, ResourceGroup *parent = nullptr,  int index = -1);
    /**
     * Removes the resource group @p resource from the project.
     * The resource group is not deleted.
     */
    void takeResourceGroup(ResourceGroup *resource);
    int indexOf(ResourceGroup *resource) const { return m_resourceGroups.indexOf(resource); }
    ResourceGroup *resourceGroupAt(int pos) const { return m_resourceGroups.value(pos); }
    int numResourceGroups() const { return m_resourceGroups.count(); }

    /// Returns the resourcegroup with identity id.
    ResourceGroup *group(const QString& id);
    /// Returns the resource group with the matching name, 0 if no match is found.
    ResourceGroup *groupByName(const QString& name) const;

    /**
     * Adds the resource to the project
     * Always use this to add resources.
     */
    void addResource(Resource *resource, int index = -1);
    /**
     * Removes the resource from the project and all resource groups.
     * The resource is not deleted.
     * Always use this to remove resources.
     */
    bool takeResource(Resource *resource);
    /// Move @p resource to the new @p group. Requests are removed.
    void moveResource(ResourceGroup *group, Resource *resource);
    /// Returns the resource with identity id.
    Resource *resource(const QString& id);
    /// Returns the resource with matching name, 0 if no match is found.
    Resource *resourceByName(const QString& name) const;
    QStringList resourceNameList() const;
    /// Returns a list of all resources
    QList<Resource*> resourceList() const { return m_resources; }
    int indexOf(Resource *resource) const;
    int resourceCount() const;
    Resource *resourceAt(int pos) const;

    EffortCostMap plannedEffortCostPrDay(QDate start, QDate end, long id = CURRENTSCHEDULE, EffortCostCalculationType = ECCT_All) const override;
    EffortCostMap plannedEffortCostPrDay(const Resource *resource, QDate start, QDate end, long id = CURRENTSCHEDULE, EffortCostCalculationType = ECCT_All) const override;

    using Node::plannedEffort;
    /// Returns the total planned effort for this project (or subproject)
    Duration plannedEffort(long id = CURRENTSCHEDULE, EffortCostCalculationType = ECCT_All) const override;
    /// Returns the total planned effort for this project (or subproject) on date
    Duration plannedEffort(QDate date, long id = CURRENTSCHEDULE, EffortCostCalculationType = ECCT_All  ) const override;
    using Node::plannedEffortTo;
    /// Returns the planned effort up to and including date
    Duration plannedEffortTo(QDate date, long id = CURRENTSCHEDULE, EffortCostCalculationType = ECCT_All  ) const override;

    /// Returns the actual effort up to and including @p date
    Duration actualEffortTo(QDate date) const override;
    /**
     * Planned cost up to and including date
     * @param date The cost is calculated from the start of the project upto including date.
     * @param id Identity of the schedule to be used.
     * @param typ the type of calculation.
     * @sa EffortCostCalculationType
     */
    double plannedCostTo(QDate date, long id = CURRENTSCHEDULE, EffortCostCalculationType = ECCT_All  ) const override;

    /**
     * Actual cost up to and including @p date
     * @param id Identity of the schedule to be used.
     * @param date The cost is calculated from the start of the project upto including date.
     */
    EffortCost actualCostTo(long int id, QDate date) const override;

    EffortCostMap actualEffortCostPrDay(QDate start, QDate end, long id = CURRENTSCHEDULE, EffortCostCalculationType = ECCT_All) const override;

    EffortCostMap actualEffortCostPrDay(const Resource *resource, QDate start, QDate end, long id = CURRENTSCHEDULE, EffortCostCalculationType = ECCT_All) const override;

    double effortPerformanceIndex(QDate date, long id) const override;

    double schedulePerformanceIndex(QDate date, long id) const override;

    /// Returns the effort planned to be used to reach the actual percent finished
    Duration budgetedWorkPerformed(QDate date, long id = CURRENTSCHEDULE) const override;
    /// Returns the cost planned to be used to reach the actual percent finished
    double budgetedCostPerformed(QDate date, long id = CURRENTSCHEDULE) const override;

    /// Budgeted Cost of Work Scheduled (up to @p date)
    double bcws(QDate date, long id = BASELINESCHEDULE) const override;
    /// Budgeted Cost of Work Performed
    double bcwp(long id = BASELINESCHEDULE) const override;
    /// Budgeted Cost of Work Performed (up to @p date)
    double bcwp(QDate date, long id = BASELINESCHEDULE) const override;

    Calendar *defaultCalendar() const { return m_defaultCalendar; }
    void setDefaultCalendar(Calendar *cal);
    const QList<Calendar*> &calendars() const;
    void addCalendar(Calendar *calendar, Calendar *parent = nullptr, int index = -1);
    void takeCalendar(Calendar *calendar);
    int indexOf(const Calendar *calendar) const;
    /// Returns the calendar with identity id.
    Calendar *calendar(const QString& id) const;
    /// Returns a list of all calendars
    QStringList calendarNames() const;
    /// Find calendar by name
    Calendar *calendarByName(const QString &name) const;
    void changed(Calendar *cal);
    QList<Calendar*> allCalendars() const;
    /// Return number of calendars
    int calendarCount() const { return m_calendars.count(); }
    /// Return the calendar at @p index, 0 if index out of bounds
    Calendar *calendarAt(int index) const { return m_calendars.value(index); }
    /**
     * Defines the length of days, weeks, months and years
     * and the standard working week.
     * Used for estimation and calculation of effort,
     * and presentation in gantt chart.
     */
    StandardWorktime *standardWorktime() { return m_standardWorktime; }
    void setStandardWorktime(StandardWorktime * worktime);
    void changed(StandardWorktime*);

    /// Check if a link exists between node @p par and @p child.
    bool linkExists(const Node *par, const Node *child) const;
    /// Check if node @p par can be linked to node @p child.
    bool legalToLink(const Node *par, const Node *child) const override;
    using Node::legalToLink;

    virtual const QHash<QString, Node*> &nodeDict() { return nodeIdDict; }
    /// Return a list of all nodes in the project (excluding myself)
    QList<Node*> allNodes(bool ordered = false, Node *parent = nullptr) const;
    /// Return the number of all nodes in the project (excluding myself)
    int nodeCount() const { return nodeIdDict.count() - 1; }

    /// Return a list of all tasks and milestones int the wbs order
    QList<Task*> allTasks(const Node *parent = nullptr) const;
    /// Return a list of all leaf nodes
    QList<Node*> leafNodes() const;

    using Node::findNode;
    /// Find the node with identity id
    Node *findNode(const QString &id) const override;

    using Node::removeId;
    /// Remove the node with identity id from the registers
    bool removeId(const QString &id) override;

    /// Reserve @p id for the @p node
    virtual void reserveId(const QString &id, Node *node);
    /// Register @p node. The nodes id must be unique and non-empty.
    bool registerNodeId(Node *node);
    /// Create a unique id.
    QString uniqueNodeId(int seed = 1) const;
    /// Check if node @p id is used
    bool nodeIdentExists(const QString &id) const;

    /// Create a unique id.
    QString uniqueNodeId(const QList<QString> &existingIds, int seed = 1);

    ResourceGroup *findResourceGroup(const QString &id) const
    {
        if (resourceGroupIdDict.contains(id) )
            return resourceGroupIdDict[ id ];
        return nullptr;
    }
    /// Remove the resourcegroup with identity id from the register
    /// If group is not nullptr, remove recursively
    void removeResourceGroupId(const QString &id, ResourceGroup *group = nullptr);
    /// Insert the resourcegroup with identity id
    /// Always insert recursively
    void insertResourceGroupId(const QString &id, ResourceGroup* group);
    /// Generate, set and insert unique id
    bool setResourceGroupId(ResourceGroup *group);
    /// returns a unique resourcegroup id
    QString uniqueResourceGroupId() const;

    /// Return a list of resources that will be allocated to new tasks
    QList<Resource*> autoAllocateResources() const;

    Resource *findResource(const QString &id) const
    {
        if (resourceIdDict.contains(id) )
            return resourceIdDict[ id ];
        return nullptr;
    }
    /// Remove the resource with identity id from the register
    bool removeResourceId(const QString &id);
    /// Insert the resource with identity id
    void insertResourceId(const QString &id, Resource *resource);
    /// Generate, set and insert unique id
    bool setResourceId(Resource *resource);
    /// returns a unique resource id
    QString uniqueResourceId() const;

    /// Find the calendar with identity id
    virtual Calendar *findCalendar(const QString &id) const
    {
        if (id.isEmpty() || !calendarIdDict.contains(id) )
            return nullptr;
        return calendarIdDict[ id ];
    }
    /// Remove the calendar with identity id from the register
    virtual bool removeCalendarId(const QString &id);
    /// Insert the calendar with identity id
    virtual void insertCalendarId(const QString &id, Calendar *calendar);
    /// Set and insert a unique id for calendar
    bool setCalendarId(Calendar *calendar);
    /// returns a unique calendar id
    QString uniqueCalendarId() const;
    /// Return reference to WBS Definition
    WBSDefinition &wbsDefinition();
    /// Set WBS Definition to @p def
    void setWbsDefinition(const WBSDefinition &def);
    /// Generate WBS Code
    QString generateWBSCode(QList<int> &indexes, bool sortable = false) const override;

    Accounts &accounts() { return m_accounts; }
    const Accounts &accounts() const { return m_accounts; }

    /**
     * Set current schedule to the schedule with identity @p id, for me and my children
     * Note that this is used (and may be changed) when calculating schedules
     */
    void setCurrentSchedule(long id) override;
    /// Create new schedule with unique name and id of type Expected.
    MainSchedule *createSchedule();
    /// Create new schedule with unique id.
    MainSchedule *createSchedule(const QString& name, Schedule::Type type);
    /// Add the schedule to the project. A fresh id will be generated for the schedule.
    void addMainSchedule(MainSchedule *schedule);
    /// Set parent schedule for my children
    void setParentSchedule(Schedule *sch) override;

    /// Set current schedule manager
    void setCurrentScheduleManager(ScheduleManager *sm);
    /// Get the schedule manager
    ScheduleManager *currentScheduleManager() const;
    /// Find the schedule manager that manages the Schedule with @p id
    ScheduleManager *scheduleManager(long id) const;
    /// Find the schedule manager with @p id
    ScheduleManager *scheduleManager(const QString &id) const;
    /// Create a unique schedule name (This may later be changed by the user)
    QString uniqueScheduleName(const ScheduleManager *parent = nullptr) const;
    /// Create a unique schedule manager identity
    QString uniqueScheduleManagerId() const;
    ScheduleManager *createScheduleManager(const ScheduleManager *parent = nullptr);
    ScheduleManager *createScheduleManager(const QString &name);
    /// Returns a list of all top level schedule managers
    QList<ScheduleManager*> scheduleManagers() const { return m_managers; }
    int numScheduleManagers() const { return m_managers.count(); }
    int indexOf(const ScheduleManager *sm) const { return m_managers.indexOf(const_cast<ScheduleManager*>(sm)); }
    bool isScheduleManager(void* ptr) const;
    void addScheduleManager(ScheduleManager *sm, ScheduleManager *parent = nullptr, int index = -1);
    int takeScheduleManager(ScheduleManager *sm);
    void moveScheduleManager(ScheduleManager *sm, ScheduleManager *newparent = nullptr, int newindex = -1);
    ScheduleManager *findScheduleManagerByName(const QString &name) const;
    /// Returns a list of all schedule managers
    QList<ScheduleManager*> allScheduleManagers() const;
    /// Return true if schedule with identity @p id is baselined
    bool isBaselined(long id = ANYSCHEDULED) const;

    void changed(ResourceGroup *group);
    void changed(Resource *resource);

    void changed(ScheduleManager *sm, int property = -1);
    void changed(MainSchedule *sch);
    void sendScheduleAdded(const MainSchedule *sch);
    void sendScheduleToBeAdded(const ScheduleManager *manager, int row);
    void sendScheduleRemoved(const MainSchedule *sch);
    void sendScheduleToBeRemoved(const MainSchedule *sch);

    /// Return the time zone used in this project
    QTimeZone timeZone() const { return m_timeZone; }
    /// Set the time zone to be used in this project
    void setTimeZone(const QTimeZone &tz);

    /**
     * Add a relation between the nodes specified in the relation rel.
     * Emits signals relationToBeAdded() before the relation is added,
     * and relationAdded() after it has been added.
     * @param rel The relation to be added.
     * @param check If true, the relation is checked for validity
     * @return true if successful.
     */
    bool addRelation(Relation *rel, bool check=true);
    /**
     * Removes the relation @p rel without deleting it.
     * Emits signals relationToBeRemoved() before the relation is removed,
     * and relationRemoved() after it has been removed.
     */
    void takeRelation(Relation *rel);

    /**
     * Modify the @p type of the @p relation.
     */
    void setRelationType(Relation *relation, Relation::Type type);
    /**
     * Modify the @p lag of the @p relation.
     */
    void setRelationLag(Relation *relation, const Duration &lag);

    void calcCriticalPathList(MainSchedule *cs);
    void calcCriticalPathList(MainSchedule *cs, Node *node);
    /**
     * Returns the list of critical paths for schedule @p id
     */
    const QList< QList<Node*> > *criticalPathList(long id = CURRENTSCHEDULE);
    QList<Node*> criticalPath(long id = CURRENTSCHEDULE, int index = 0);

    /// Returns a flat list af all nodes
    QList<Node*> flatNodeList(Node *parent = nullptr);

    void generateUniqueNodeIds();
    void generateUniqueIds();

    const ConfigBase &config() const { return m_config ? *m_config : emptyConfig; }
    /// Set configuration data
    void setConfig(ConfigBase *config) { m_config = config; }

    const Task &taskDefaults() const { return config().taskDefaults(); }

    /// Return locale. (Used for currency, everything else is from KGlobal::locale)
    Locale *locale() { return const_cast<ConfigBase&>(config()).locale(); }
    /// Return locale. (Used for currency, everything else is from KGlobal::locale)
    const Locale *locale() const { return config().locale(); }
    /// Signal that locale data has changed
    void emitLocaleChanged();

    void setSchedulerPlugins(const QMap<QString, SchedulerPlugin*> &plugins);
    const QMap<QString, SchedulerPlugin*> &schedulerPlugins() const { return m_schedulerPlugins; }

    void initiateCalculation(MainSchedule &sch) override;
    void initiateCalculationLists(MainSchedule &sch) override;

    void finishCalculation(ScheduleManager &sm);
    void adjustSummarytask() override;

    /// Increments progress and emits signal sigProgress()
    void incProgress();
    /// Emits signal maxProgress()
    void emitMaxProgress(int value);

    bool stopcalculation = false;

    /// return a <id, name> map of all external projects
    QMap<QString, QString> externalProjects() const;

    void emitDocumentAdded(Node*, Document*, int index) override;
    void emitDocumentRemoved(Node*, Document*, int index) override;
    void emitDocumentChanged(Node*, Document*, int index) override;

    bool useSharedResources() const;
    void setUseSharedResources(bool on);
    bool isSharedResourcesLoaded() const;
    void setSharedResourcesLoaded(bool on);
    void setSharedResourcesFile(const QString &file);
    QString sharedResourcesFile() const;

    QList<QUrl> taskModules(bool includeLocal = true) const;
    void setTaskModules(const QList<QUrl> modules, bool useLocalTaskModules);
    bool useLocalTaskModules() const;
    void setUseLocalTaskModules(bool value, bool emitChanged = true);
    void setLocalTaskModulesPath(const QUrl &url);

    ulong granularity() const override;

public Q_SLOTS:
    /// Sets m_progress to @p progress and emits signal sigProgress()
    /// If @p sm is not 0, progress is also set for the schedule manager
    void setProgress(int progress, KPlato::ScheduleManager *sm = nullptr);
    /// Sets m_maxprogress to @p max and emits signal maxProgress()
    /// If @p sm is not 0, max progress is also set for the schedule manager
    void setMaxProgress(int max, KPlato::ScheduleManager *sm = nullptr);

    void swapScheduleManagers(KPlato::ScheduleManager *from, KPlato::ScheduleManager *to);

Q_SIGNALS:
    void scheduleManagersSwapped(KPlato::ScheduleManager *from, KPlato::ScheduleManager *to);
    /// Emitted when the project is about to be deleted (The destroyed signal is disabled)
    void aboutToBeDeleted();
    /// Emitted when anything in the project is changed (use with care)
    void projectChanged();
    /// Emitted when the WBS code definition has changed. This may change all nodes.
    void wbsDefinitionChanged();
    /// Emitted when a schedule has been calculated
    void projectCalculated(KPlato::ScheduleManager *sm);
    /// Emitted when the pointer to the current schedule has been changed
    void currentScheduleChanged();
    /// Use to show progress during calculation
    void sigProgress(int);
    /// Use to set the maximum progress (minimum always 0)
    void maxProgress(int);
    /// Emitted when calculation starts
    void sigCalculationStarted(KPlato::Project *project, KPlato::ScheduleManager *sm);
    /// Emitted when calculation is finished
    void sigCalculationFinished(KPlato::Project *project, KPlato::ScheduleManager *sm);
    /// This signal is emitted when one of the nodes members is changed.
    void nodeChanged(KPlato::Node*, int);
    /// This signal is emitted when the node is to be added to the project.
    void nodeToBeAdded(KPlato::Node*, int);
    /// This signal is emitted when the node has been added to the project.
    void nodeAdded(KPlato::Node*);
    /// This signal is emitted when the node is to be removed from the project.
    void nodeToBeRemoved(KPlato::Node*);
    /// This signal is emitted when the node has been removed from the project.
    void nodeRemoved(KPlato::Node*);
    /// This signal is emitted when the node is to be moved up, moved down, indented or unindented.
    void nodeToBeMoved(KPlato::Node* node, int pos, KPlato::Node* newParent, int newPos);
    /// This signal is emitted when the node has been moved up, moved down, indented or unindented.
    void nodeMoved(KPlato::Node*);

    /// This signal is emitted when a document is added
    void documentAdded(KPlato::Node*, KPlato::Document*, int index);
    /// This signal is emitted when a document is removed
    void documentRemoved(KPlato::Node*, KPlato::Document*, int index);
    /// This signal is emitted when a document is changed
    void documentChanged(KPlato::Node*, KPlato::Document*, int index);

    void resourceGroupChanged(KPlato::ResourceGroup *group);
    void resourceGroupToBeAdded(KPlato::Project *project, KPlato::ResourceGroup *parent, int row);
    void resourceGroupAdded(KPlato::ResourceGroup *group);
    void resourceGroupToBeRemoved(KPlato::Project *project, KPlato::ResourceGroup *parent, int row, KPlato::ResourceGroup *group);
    void resourceGroupRemoved();

    void resourceChanged(KPlato::Resource *resource);
    void resourceToBeAdded(KPlato::Project *project, int row);
    void resourceAdded(KPlato::Resource *resource);
    void resourceToBeRemoved(KPlato::Project *project, int row, KPlato::Resource *resource);
    void resourceRemoved();

    void scheduleManagerChanged(KPlato::ScheduleManager *sch, int property = -1);
    void scheduleManagerAdded(const KPlato::ScheduleManager *sch);
    void scheduleManagerToBeAdded(const KPlato::ScheduleManager *sch, int row);
    void scheduleManagerRemoved(const KPlato::ScheduleManager *sch);
    void scheduleManagerToBeRemoved(const KPlato::ScheduleManager *sch);
    void scheduleManagerMoved(const KPlato::ScheduleManager *sch, int row);
    void scheduleManagerToBeMoved(const KPlato::ScheduleManager *sch);

    void scheduleChanged(KPlato::MainSchedule *sch);
    void scheduleToBeAdded(const KPlato::ScheduleManager *manager, int row);
    void scheduleAdded(const KPlato::MainSchedule *sch);
    void scheduleToBeRemoved(const KPlato::MainSchedule *sch);
    void scheduleRemoved(const KPlato::MainSchedule *sch);

//    void currentViewScheduleIdChanged(long id);

    void calendarChanged(KPlato::Calendar *cal);
    void calendarToBeAdded(const KPlato::Calendar *cal, int row);
    void calendarAdded(const KPlato::Calendar *cal);
    void calendarToBeRemoved(const KPlato::Calendar *cal);
    void calendarRemoved(const KPlato::Calendar *cal);

    /**
     * Emitted when the default calendar pointer has changed
     * @param cal The new default calendar. May be 0.
     */
    void defaultCalendarChanged(KPlato::Calendar *cal);
    /**
     * Emitted when the standard worktime has been changed.
     */
    void standardWorktimeChanged(KPlato::StandardWorktime*);

    /// Emitted when the relation @p rel is about to be added.
    void relationToBeAdded(KPlato::Relation *rel, int parentIndex, int childIndex);
    /// Emitted when the relation @p rel has been added.
    void relationAdded(KPlato::Relation *rel);
    /// Emitted when the relation @p rel is about to be removed.
    void relationToBeRemoved(KPlato::Relation *rel);
    /// Emitted when the relation @p rel has been removed.
    void relationRemoved(KPlato::Relation *rel);
    /// Emitted when the relation @p rel shall be modified.
    void relationToBeModified(KPlato::Relation *rel);
    /// Emitted when the relation @p rel has been modified.
    void relationModified(KPlato::Relation *rel);

    /// Emitted when locale data has changed
    void localeChanged();

    void taskModulesChanged(const QList<QUrl> &modules);

protected:
    /// Calculate the schedule.
    void calculate(Schedule *scedule);
    /// Calculate current schedule
    void calculate();

    /// Re-calculate the schedule from @p dt
    void calculate(Schedule *scedule, const DateTime &dt);
    /// Calculate current schedule from @p dt (Always calculates forward)
    void calculate(const DateTime &dt);

    /// Calculate critical path
    bool calcCriticalPath(bool fromEnd) override;

    /// Prepare task lists for scheduling
    void tasksForward();
    /// Prepare task lists for scheduling
    void tasksBackward();

    void saveSettings(QDomElement &element, const XmlSaveContext &context) const;
    bool loadSettings(KoXmlElement &element, XMLLoaderObject &status);

protected:
    friend class KPlatoXmlLoaderBase;
    using Node::changed;
    void changed(Node *node, int property = -1) override;

    Accounts m_accounts;
    QList<ResourceGroup*> m_resourceGroups;
    QList<Resource*> m_resources;

    QList<Calendar*> m_calendars;
    Calendar * m_defaultCalendar;

    StandardWorktime *m_standardWorktime;

    DateTime calculateForward(int use) override;
    DateTime calculateBackward(int use) override;
    DateTime scheduleForward(const DateTime &earliest, int use) override;
    DateTime scheduleBackward(const DateTime &latest, int use) override;
    DateTime checkStartConstraints(const DateTime &dt) const;
    DateTime checkEndConstraints(const DateTime &dt) const;

    bool legalParents(const Node *par, const Node *child) const;
    bool legalChildren(const Node *par, const Node *child) const;

    bool priorityUsed() const;

#ifndef PLAN_NLOGDEBUG
private:
    static bool checkParent(Node *n, const QList<Node*> &list, QList<Relation*> &checked);
    static bool checkChildren(Node *n, const QList<Node*> &list, QList<Relation*> &checked);
#endif

private Q_SLOTS:
   void nodeDestroyed(QObject *obj);
   // TODO: cleanup signals projectCalculated/sigCalculationFinished.
   //       Problem is projectCalculated is not emitted when calculation is threaded.
   void emitProjectCalculated(KPlato::Project *project, KPlato::ScheduleManager *sm);

private:
    void init();

    QHash<QString, ResourceGroup*> resourceGroupIdDict;
    QHash<QString, Resource*> resourceIdDict;
    QHash<QString, Node*> nodeIdDict;
    QMap<QString, Node*> nodeIdReserved;
    QMap<QString, Calendar*> calendarIdDict;
    QMap<QString, ScheduleManager*> m_managerIdMap;

    QList<ScheduleManager*> m_managers;
    QTimeZone m_timeZone;

    WBSDefinition m_wbsDefinition;

    ConfigBase emptyConfig;
    QPointer<ConfigBase> m_config; // this one is not owned by me, don't delete

    int m_progress;

    QMap<QString, SchedulerPlugin*> m_schedulerPlugins;

    int m_refCount; // make it possible to use the project by different threads

    QList<Task*> m_hardConstraints;
    QList<Task*> m_softConstraints;
    QMultiMap<int, Task*> m_terminalNodes;
    QMultiMap<int, Task*> m_priorityNodes;

    bool m_useSharedResources;
    bool m_sharedResourcesLoaded;
    QString m_sharedResourcesFile;

public:
    class WorkPackageInfo {
    public:
        WorkPackageInfo() : checkForWorkPackages(false), deleteAfterRetrieval(true), archiveAfterRetrieval(false) {}
        bool checkForWorkPackages;
        QUrl retrieveUrl;
        bool deleteAfterRetrieval;
        bool archiveAfterRetrieval;
        QUrl archiveUrl;
        QUrl publishUrl;

        bool operator!=(const WorkPackageInfo &o) { return !operator==(o); }
        bool operator==(const WorkPackageInfo &o) {
            return checkForWorkPackages == o.checkForWorkPackages
                && deleteAfterRetrieval == o.deleteAfterRetrieval
                && archiveAfterRetrieval == o.archiveAfterRetrieval
                && retrieveUrl == o.retrieveUrl
                && archiveUrl == o.archiveUrl
                && publishUrl == o.publishUrl;
        }
    };
    void setWorkPackageInfo(const WorkPackageInfo &wpInfo) { m_workPackageInfo = wpInfo; }
    WorkPackageInfo workPackageInfo() const { return m_workPackageInfo; }
private:
    WorkPackageInfo m_workPackageInfo;

    QList<QUrl> m_taskModules;
    bool m_useLocalTaskModules;
    QUrl m_localTaskModulesPath;
    QPointer<ScheduleManager> m_currentScheduleManager;
};

}  //KPlato namespace

#endif
