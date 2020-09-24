/* This file is part of the KDE project
 * Copyright (C) 2007 - 2009, 2012 Dag Andersen <dag.andersen@kdemail.net>
 * Copyright (C) 2016 Dag Andersen <dag.andersen@kdemail.net>
 * Copyright (C) 2019 Dag Andersen <dag.andersen@kdemail.net>
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

#ifndef NODEITEMMODEL_H
#define NODEITEMMODEL_H

#include "kptitemmodelbase.h"
#include "kptschedule.h"
#include "kptworkpackagemodel.h"

#include <QDate>
#include <QMetaEnum>
#include <QSortFilterProxyModel>
#include <QUrl>

class QMimeData;
class KUndo2Command;
class KoXmlWriter;
class KoStore;
class KoOdfWriteStore;

namespace KPlato
{

class Project;
class Node;
class Estimate;

class PLANMODELS_EXPORT NodeModel : public QObject
{
    Q_OBJECT
public:
    NodeModel();
    ~NodeModel() override {}
    
    enum SpecialRoles {
        SortableRole = Qt::UserRole + 5024 // unlikely high number
    };

    enum Properties {
        NodeName = 0,
        NodeType,
        NodePriority,
        NodeResponsible,
        NodeAllocation,
        NodeEstimateType,
        NodeEstimateCalendar,
        NodeEstimate,
        NodeOptimisticRatio,
        NodePessimisticRatio,
        NodeRisk,
        NodeConstraint,
        NodeConstraintStart,
        NodeConstraintEnd,
        NodeRunningAccount,
        NodeStartupAccount,
        NodeStartupCost,
        NodeShutdownAccount,
        NodeShutdownCost,
        NodeDescription,

        // Based on edited values
        NodeExpected,
        NodeVarianceEstimate,
        NodeOptimistic,
        NodePessimistic,

        // After scheduling
        NodeStartTime,
        NodeEndTime,
        NodeEarlyStart,
        NodeEarlyFinish,
        NodeLateStart,
        NodeLateFinish,
        NodePositiveFloat,
        NodeFreeFloat,
        NodeNegativeFloat,
        NodeStartFloat,
        NodeFinishFloat,
        NodeAssignments,

        // Based on scheduled values
        NodeDuration,
        NodeVarianceDuration,
        NodeOptimisticDuration,
        NodePessimisticDuration,

        // Completion
        NodeStatus,
        NodeCompleted,
        NodePlannedEffort,
        NodeActualEffort,
        NodeRemainingEffort,
        NodePlannedCost,
        NodeActualCost,
        NodeActualStart,
        NodeStarted,
        NodeActualFinish,
        NodeFinished,
        NodeStatusNote,
            
        // Scheduling errors
        NodeSchedulingStatus,
        NodeNotScheduled,
        NodeAssignmentMissing,
        NodeResourceOverbooked,
        NodeResourceUnavailable,
        NodeConstraintsError,
        NodeEffortNotMet,
        NodeSchedulingError,

        NodeWBSCode,
        NodeLevel,
        
        // Performance
        NodeBCWS,
        NodeBCWP,
        NodeACWP,
        NodePerformanceIndex,
        //
        NodeCritical,
        NodeCriticalPath,

        // Info from latest work package transmission
        WPOwnerName,
        WPTransmitionStatus,
        WPTransmitionTime
    };
    Q_ENUM(Properties)
    const QMetaEnum columnMap() const;
    
    void setProject(Project *project);
    void setManager(ScheduleManager *sm);
    Project *project() const { return m_project; }
    ScheduleManager *manager() const { return m_manager; }
    long id() const { return m_manager == nullptr ? -1 : m_manager->scheduleId(); }
    
    QVariant data(const Node *node, int property, int role = Qt::DisplayRole) const; 
    KUndo2Command *setData(Node *node, int property, const QVariant & value, int role = Qt::EditRole);
    
    static QVariant headerData(int section, int role = Qt::DisplayRole);

    int propertyCount() const;
    
    void setNow(const QDate &now) { m_now = now; }
    QDate now() const { return m_now; }
    
    QVariant name(const Node *node, int role) const;
    QVariant leader(const Node *node, int role) const;
    QVariant allocation(const Node *node, int role) const;
    QVariant description(const Node *node, int role) const;
    QVariant type(const Node *node, int role) const;
    QVariant constraint(const Node *node, int role) const;
    QVariant constraintStartTime(const Node *node, int role) const;
    QVariant constraintEndTime(const Node *node, int role) const;
    QVariant estimateType(const Node *node, int role) const;
    QVariant estimateCalendar(const Node *node, int role) const;
    QVariant estimate(const Node *node, int role) const;
    QVariant optimisticRatio(const Node *node, int role) const;
    QVariant pessimisticRatio(const Node *node, int role) const;
    QVariant riskType(const Node *node, int role) const;
    QVariant priority(const Node *node, int role) const;
    QVariant runningAccount(const Node *node, int role) const;
    QVariant startupAccount(const Node *node, int role) const;
    QVariant startupCost(const Node *node, int role) const;
    QVariant shutdownAccount(const Node *node, int role) const;
    QVariant shutdownCost(const Node *node, int role) const;
    
    QVariant startTime(const Node *node, int role) const;
    QVariant endTime(const Node *node, int role) const;

    QVariant duration(const Node *node, int role) const;
    QVariant varianceDuration(const Node *node, int role) const;
    QVariant varianceEstimate(const Estimate *est, int role) const;
    QVariant optimisticDuration(const Node *node, int role) const;
    QVariant optimisticEstimate(const Estimate *est, int role) const;
    QVariant pertExpected(const Estimate *est, int role) const;
    QVariant pessimisticDuration(const Node *node, int role) const;
    QVariant pessimisticEstimate(const Estimate *est, int role) const;

    QVariant earlyStart(const Node *node, int role) const;
    QVariant earlyFinish(const Node *node, int role) const;
    QVariant lateStart(const Node *node, int role) const;
    QVariant lateFinish(const Node *node, int role) const;
    QVariant positiveFloat(const Node *node, int role) const;
    QVariant freeFloat(const Node *node, int role) const;
    QVariant negativeFloat(const Node *node, int role) const;
    QVariant startFloat(const Node *node, int role) const;
    QVariant finishFloat(const Node *node, int role) const;
    QVariant assignedResources(const Node *node, int role) const;
    
    QVariant status(const Node *node, int role) const;
    QVariant completed(const Node *node, int role) const;
    QVariant startedTime(const Node *node, int role) const;
    QVariant isStarted(const Node *node, int role) const;
    QVariant finishedTime(const Node *node, int role) const;
    QVariant isFinished(const Node *node, int role) const;
    QVariant plannedEffortTo(const Node *node, int role) const;
    QVariant actualEffortTo(const Node *node, int role) const;
    QVariant remainingEffort(const Node *node, int role) const;
    QVariant plannedCostTo(const Node *node, int role) const;
    QVariant actualCostTo(const Node *node, int role) const;
    QVariant note(const Node *node, int role) const;

    /// The nodes scheduling status
    QVariant nodeSchedulingStatus(const Node *node, int role) const;
    /// Set if the node has not been scheduled
    QVariant nodeIsNotScheduled(const Node *node, int role) const;
    /// Set if EffortType == Effort, but no resource is requested
    QVariant resourceIsMissing(const Node *node, int role) const;
    /// Set if the assigned resource is overbooked
    QVariant resourceIsOverbooked(const Node *node, int role) const;
    /// Set if the requested resource is not available
    QVariant resourceIsNotAvailable(const Node *node, int role) const;
    /// Set if the task cannot be scheduled to fulfill all the constraints
    QVariant schedulingConstraintsError(const Node *node, int role) const;
    /// Resources could not fulfill estimate
    QVariant effortNotMet(const Node *node, int role) const;
    /// Other scheduling error occurred
    QVariant schedulingError(const Node *node, int role) const;

    QVariant wbsCode(const Node *node, int role) const;
    QVariant nodeLevel(const Node *node, int role) const;
    
    QVariant nodeBCWS(const Node *node, int role) const;
    QVariant nodeBCWP(const Node *node, int role) const;
    QVariant nodeACWP(const Node *node, int role) const;
    QVariant nodePerformanceIndex(const Node *node, int role) const;

    QVariant nodeIsCritical(const Node *node, int role) const;
    QVariant nodeInCriticalPath(const Node *node, int role) const;

    QVariant wpOwnerName(const Node *node, int role) const;
    QVariant wpTransmitionStatus(const Node *node, int role) const;
    QVariant wpTransmitionTime(const Node *node, int role) const;

    KUndo2Command *setName(Node *node, const QVariant &value, int role);
    KUndo2Command *setLeader(Node *node, const QVariant &value, int role);
    KUndo2Command *setAllocation(Node *node, const QVariant &value, int role);
    KUndo2Command *setDescription(Node *node, const QVariant &value, int role);
    KUndo2Command *setType(Node *node, const QVariant &value, int role);
    KUndo2Command *setConstraint(Node *node, const QVariant &value, int role);
    KUndo2Command *setConstraintStartTime(Node *node, const QVariant &value, int role);
    KUndo2Command *setConstraintEndTime(Node *node, const QVariant &value, int role);
    KUndo2Command *setEstimateType(Node *node, const QVariant &value, int role);
    KUndo2Command *setEstimateCalendar(Node *node, const QVariant &value, int role);
    KUndo2Command *setEstimate(Node *node, const QVariant &value, int role);
    KUndo2Command *setOptimisticRatio(Node *node, const QVariant &value, int role);
    KUndo2Command *setPessimisticRatio(Node *node, const QVariant &value, int role);
    KUndo2Command *setRiskType(Node *node, const QVariant &value, int role);
    KUndo2Command *setPriority(Node *node, const QVariant &value, int role);
    KUndo2Command *setRunningAccount(Node *node, const QVariant &value, int role);
    KUndo2Command *setStartupAccount(Node *node, const QVariant &value, int role);
    KUndo2Command *setStartupCost(Node *node, const QVariant &value, int role);
    KUndo2Command *setShutdownAccount(Node *node, const QVariant &value, int role);
    KUndo2Command *setShutdownCost(Node *node, const QVariant &value, int role);
    KUndo2Command *setCompletion(Node *node, const QVariant &value, int role);
    KUndo2Command *setActualEffort(Node *node, const QVariant &value, int role);
    KUndo2Command *setRemainingEffort(Node *node, const QVariant &value, int role);
    KUndo2Command *setStartedTime(Node *node, const QVariant &value, int role);
    KUndo2Command *setFinishedTime(Node *node, const QVariant &value, int role);

private:
    Project *m_project;
    ScheduleManager *m_manager;
    QDate m_now;
    int m_prec;
};

class PLANMODELS_EXPORT NodeItemModel : public ItemModelBase
{
    Q_OBJECT
public:
    explicit NodeItemModel(QObject *parent = nullptr);
    ~NodeItemModel() override;
    
    /// Returns a column number/- name map for this model
    const QMetaEnum columnMap() const override { return m_nodemodel.columnMap(); }
    
    ScheduleManager *manager() const { return m_nodemodel.manager(); }
    long id() const { return m_nodemodel.id(); }

    Qt::ItemFlags flags(const QModelIndex & index) const override;
    
    QModelIndex parent(const QModelIndex & index) const override;
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const override;
    virtual QModelIndex index(const Node *node, int column = 0) const;
    
    int columnCount(const QModelIndex & parent = QModelIndex()) const override; 
    int rowCount(const QModelIndex & parent = QModelIndex()) const override; 
    
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override; 
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;

    
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    
    QMimeData * mimeData(const QModelIndexList & indexes) const override;
    QStringList mimeTypes () const override;
    Qt::DropActions supportedDropActions() const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

    Node *node(const QModelIndex &index) const;
    QAbstractItemDelegate *createDelegate(int column, QWidget *parent) const override;

    QModelIndex insertTask(Node *node, Node *after);
    QModelIndex insertSubtask(Node *node, Node *parent);
    
    QList<Node*> nodeList(QDataStream &stream);
    QList<Resource*> resourceList(QDataStream &stream);
    static QList<Node*> removeChildNodes(const QList<Node*> &nodes);
    bool dropAllowed(Node *on, const QMimeData *data);
    
    bool dropAllowed(const QModelIndex &index, int dropIndicatorPosition, const QMimeData *data) override;
    
    bool projectShown() const { return m_projectshown; }

    /// Return the sortorder to be used for @p column
    int sortRole(int column) const override;

Q_SIGNALS:
    void nodeInserted(KPlato::Node *node);
    void projectShownChanged(bool);

public Q_SLOTS:
    void setProject(KPlato::Project *project) override;
    void setScheduleManager(KPlato::ScheduleManager *sm) override;
    void setShowProject(bool on);

protected Q_SLOTS:
    virtual void slotWbsDefinitionChanged();
    virtual void slotNodeChanged(KPlato::Node*, int);
    virtual void slotNodeToBeInserted(KPlato::Node *node, int row);
    virtual void slotNodeInserted(KPlato::Node *node);
    virtual void slotNodeToBeRemoved(KPlato::Node *node);
    virtual void slotNodeRemoved(KPlato::Node *node);

    virtual void slotNodeToBeMoved(KPlato::Node *node, int pos, KPlato::Node *newParent, int newPos);
    virtual void slotNodeMoved(KPlato::Node *node);

    void slotLayoutChanged() override;
    virtual void slotProjectCalculated(KPlato::ScheduleManager *sm);

protected:
    virtual bool setType(Node *node, const QVariant &value, int role);
    bool setCompletion(Node *node, const QVariant &value, int role);
    bool setAllocation(Node *node, const QVariant &value, int role);

    bool dropResourceMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
    bool dropProjectMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
    bool dropTaskModuleMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
    KUndo2Command *createAllocationCommand(Task &task, const QList<Resource*> &lst);
    bool dropUrlMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
    bool importProjectFile(const QUrl &url, Qt::DropAction action, int row, int column, const QModelIndex &parent);

protected:
    Node *m_node; // for sanity check
    NodeModel m_nodemodel;
    bool m_projectshown;
};

//--------------------------------------
class PLANMODELS_EXPORT GanttItemModel : public NodeItemModel
{
    Q_OBJECT
public:
    enum GanttModelRoles { SpecialItemTypeRole = Qt::UserRole + 123 }; //FIXME

    explicit GanttItemModel(QObject *parent = nullptr);
    ~GanttItemModel() override;

    int rowCount(const QModelIndex &parent) const override;
    using NodeItemModel::index;
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &idx) const override;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;

    void setShowSpecial(bool on) { m_showSpecial = on; }
    bool showSpecial() const { return m_showSpecial; }

private:
    bool m_showSpecial;
    QMultiMap<Node*, void*> parentmap;
};

// TODO: Rename, this is now a flat node item model
class PLANMODELS_EXPORT MilestoneItemModel : public ItemModelBase
{
    Q_OBJECT
public:
    explicit MilestoneItemModel(QObject *parent = nullptr);
    ~MilestoneItemModel() override;

    /// Returns a column number/- name map for this model
    const QMetaEnum columnMap() const override { return m_nodemodel.columnMap(); }

    ScheduleManager *manager() const { return m_nodemodel.manager(); }
    long id() const { return m_nodemodel.id(); }

    Qt::ItemFlags flags(const QModelIndex & index) const override;

    QModelIndex parent(const QModelIndex & index) const override;
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const override;
    virtual QModelIndex index(const Node *node) const;

    int columnCount(const QModelIndex & parent = QModelIndex()) const override; 
    int rowCount(const QModelIndex & parent = QModelIndex()) const override; 

    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override; 
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;


    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QMimeData * mimeData(const QModelIndexList & indexes) const override;
    QStringList mimeTypes () const override;
    Qt::DropActions supportedDropActions() const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

    Node *node(const QModelIndex &index) const;
    QAbstractItemDelegate *createDelegate(int column, QWidget *parent) const override;

    QModelIndex insertTask(Node *node, Node *after);
    QModelIndex insertSubtask(Node *node, Node *parent);

    QList<Node*> nodeList(QDataStream &stream);
    static QList<Node*> removeChildNodes(const QList<Node*> &nodes);
    bool dropAllowed(Node *on, const QMimeData *data);

    bool dropAllowed(const QModelIndex &index, int dropIndicatorPosition, const QMimeData *data) override;

    QList<Node*> mileStones() const;

    int sortRole(int column) const override;

public Q_SLOTS:
    void setProject(KPlato::Project *project) override;
    void setScheduleManager(KPlato::ScheduleManager *sm) override;

protected Q_SLOTS:
    void slotNodeChanged(KPlato::Node*);
    void slotNodeToBeInserted(KPlato::Node *node, int row);
    void slotNodeInserted(KPlato::Node *node);
    void slotNodeToBeRemoved(KPlato::Node *node);
    void slotNodeRemoved(KPlato::Node *node);
    void slotNodeToBeMoved(KPlato::Node *node, int pos,KPlato::Node *newParent, int newPos);
    void slotNodeMoved(KPlato::Node *node);

    void slotLayoutChanged() override;
    void slotWbsDefinitionChanged();

protected:
    bool resetData();
    void resetModel();
    
private:
    NodeModel m_nodemodel;
    QMap<QString, Node*> m_nodemap;
};

class PLANMODELS_EXPORT NodeSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    NodeSortFilterProxyModel(ItemModelBase* model, QObject *parent, bool filterUnscheduled = true);

    ItemModelBase *itemModel() const;
    void setFilterUnscheduled(bool on);
    bool filterUnscheduled() const { return m_filterUnscheduled; }

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

protected:
    bool filterAcceptsRow (int source_row, const QModelIndex & source_parent) const override;

private:
    NodeItemModel *m_model;
    bool m_filterUnscheduled;
};

class PLANMODELS_EXPORT TaskModuleModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit TaskModuleModel(QObject *parent = nullptr);

    void addTaskModule(Project *project , const QUrl &url);

    Qt::ItemFlags flags(const QModelIndex &idx) const override;
    int columnCount(const QModelIndex &idx = QModelIndex()) const override;
    int rowCount(const QModelIndex &idx = QModelIndex()) const override;
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QModelIndex parent(const QModelIndex &idx) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QStringList mimeTypes() const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    QMimeData *mimeData(const QModelIndexList &idx) const override;

    bool importProject(const QUrl &url, bool emitsignal = true);
    Project *loadProjectFromUrl(const QUrl &url) const;

public Q_SLOTS:
    void setProject(Project *project);
    void loadTaskModules(const QStringList &files);
    void slotTaskModulesChanged(const QList<QUrl> &modules);

    void slotReset();

Q_SIGNALS:
    void executeCommand(KUndo2Command *cmd);
    void saveTaskModule(const QUrl &url, KPlato::Project *project);
    void removeTaskModule(const QUrl &url);

protected:
    void stripProject(Project *project) const;

private:
    Project *m_project;
    QList<Project*> m_modules;
    QList<QUrl> m_urls;
};

} //namespace KPlato

#endif //NODEITEMMODEL_H
