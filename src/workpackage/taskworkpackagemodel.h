/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2009, 2011 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef TASKWORKPACKAGEMODEL_H
#define TASKWORKPACKAGEMODEL_H

#include "planwork_export.h"

#include "kptitemmodelbase.h"
#include "kptnodeitemmodel.h"
#include "kptschedule.h"

#include <QMetaEnum>


class QModelIndex;
class QAbstractItemDelegate;

namespace KPlato
{

class Project;
class Node;
class Resource;
class Document;

}

/// The main namespace
namespace KPlatoWork
{

class Part;
class WorkPackage;

/**
 * The TaskWorkPackageModel class gives access to workpackage status
 * for the resources assigned to the task in this package.
 *
 * The model stores a nodes parentNode() in the index's internalPointer().
 */
class PLANWORK_EXPORT TaskWorkPackageModel : public KPlato::ItemModelBase
{
    Q_OBJECT
public:
    explicit TaskWorkPackageModel(Part *part, QObject *parent = nullptr);
    ~TaskWorkPackageModel() override {}

    enum Properties {
        NodeName = 0,
        NodeType,
        NodeResponsible,
        NodeDescription,

        // After scheduling
        NodeStartTime,
        NodeEndTime,
        NodeAssignments,

        // Completion
        NodeCompleted,
        NodeActualEffort,
        NodeRemainingEffort,
        NodePlannedEffort,
        NodeActualStart,
        NodeStarted,
        NodeActualFinish,
        NodeFinished,
        NodeStatus,
        NodeStatusNote,

        ProjectName,
        ProjectManager
    };
    Q_ENUM(Properties)
    const QMetaEnum columnMap() const override
    {
        return metaObject()->enumerator(metaObject()->indexOfEnumerator("Properties"));
    }

    WorkPackage *workPackage(int index) const;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override; 
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;
    
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &index = QModelIndex()) const override;
    
    KPlato::Node *nodeForIndex(const QModelIndex &index) const;
    QModelIndex indexForNode(KPlato::Node *node) const;

    QAbstractItemDelegate *createDelegate(int column, QWidget *parent) const override;

    KPlato::Document *documentForIndex(const QModelIndex &idx) const;

    WorkPackage *ptrToWorkPackage(const QModelIndex &idx) const;
    KPlato::Node *ptrToNode(const QModelIndex &idx) const;

    bool isNode(const QModelIndex &idx) const;
    bool isDocument(const QModelIndex &idx) const;

public Q_SLOTS:
    void addWorkPackage(KPlatoWork::WorkPackage *package, int row);
    void removeWorkPackage(KPlatoWork::WorkPackage *package, int row);

protected Q_SLOTS:
    void slotNodeChanged(KPlato::Node*);
    void slotNodeToBeInserted(KPlato::Node *node, int row);
    void slotNodeInserted(KPlato::Node *node);
    void slotNodeToBeRemoved(KPlato::Node *node);
    void slotNodeRemoved(KPlato::Node *node);

    void slotDocumentAdded(KPlato::Node *node, KPlato::Document *doc, int index);
    void slotDocumentRemoved(KPlato::Node *node, KPlato::Document *doc, int index);
    void slotDocumentChanged(KPlato::Node *node, KPlato::Document *doc, int index);

protected:
    QVariant nodeData(KPlato::Node *node, int column, int role) const; 
    QVariant documentData(KPlato::Document *doc, int column, int role) const; 

    QVariant name(const KPlato::Resource *r, int role) const;
    QVariant email(const KPlato::Resource *r, int role) const;
    QVariant sendStatus(const KPlato::Resource *r, int role) const;
    QVariant sendTime(const KPlato::Resource *r, int role) const;
    QVariant responseType(const KPlato::Resource *r, int role) const;
    QVariant requiredTime(const KPlato::Resource *r, int role) const;
    QVariant responseStatus(const KPlato::Resource *r, int role) const;
    QVariant responseTime(const KPlato::Resource *r, int role) const;
    QVariant lastAction(const KPlato::Resource *r, int role) const;
    QVariant projectName(const KPlato::Node *n, int role) const;
    QVariant projectManager(const KPlato::Node *n, int role) const;
    
    bool setCompletion(KPlato::Node *node, const QVariant &value, int role);
    bool setRemainingEffort(KPlato::Node *node, const QVariant &value, int role);
    bool setActualEffort(KPlato::Node *node, const QVariant &value, int role);
    bool setStartedTime(KPlato::Node *node, const QVariant &value, int role);
    bool setFinishedTime(KPlato::Node *node, const QVariant &value, int role);

    QVariant actualStart(KPlato::Node *n, int role) const;
    QVariant actualFinish(KPlato::Node *n, int role) const;
    QVariant plannedEffort(KPlato::Node *n, int role) const;

    QVariant status(KPlato::Node *n, int role) const;

private:
    KPlato::NodeModel m_nodemodel;
    Part *m_part;
    QList<WorkPackage*> m_packages;
};


} //namespace KPlato

#endif //WORKPACKAGEMODEL_H
