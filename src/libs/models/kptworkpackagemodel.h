/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2007, 2012 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef WORKPACKAGEMODEL_H
#define WORKPACKAGEMODEL_H

#include "planmodels_export.h"

#include <QSortFilterProxyModel>

class QModelIndex;
class QVariant;
class QMimeData;

/// The main namespace
namespace KPlato
{

class WorkPackage;
class Node;
class Task;
class Project;
class ScheduleManager;
class NodeItemModel;

class PLANMODELS_EXPORT WorkPackageModel : public QObject
{
    Q_OBJECT
public:
    explicit WorkPackageModel(QObject *parent = nullptr)
        : QObject(parent)
     {}
    ~WorkPackageModel() override {}

    QVariant data(const WorkPackage *wp, int column, int role = Qt::DisplayRole) const;

protected:
    QVariant nodeName(const WorkPackage *wp, int role) const;
    QVariant ownerName(const WorkPackage *wp, int role) const;
    QVariant transmitionStatus(const WorkPackage *wp, int role) const;
    QVariant transmitionTime(const WorkPackage *wp, int role) const;

    QVariant completion(const WorkPackage *wp, int role) const;
};

/**
  The WPSortFilterProxyModel only accepts scheduled tasks.
*/
class WPSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit WPSortFilterProxyModel(QObject *parent = nullptr) : QSortFilterProxyModel(parent) {}
protected:
    /// Only accept scheduled tasks
    bool filterAcceptsRow(int source_row, const QModelIndex &sourceParent) const override;
};

/**
  The WorkPackageProxyModel offers a flat list of tasks with workpackage log entries as children.

  The tasks is fetched from the WPSortFilterProxyModel, the work packages is added by this model.

  It uses the NodeItemModel to get the tasks, the FlatProxyModel to convert to a flat list,
  and the WPSortFilterProxyModel to accept only scheduled tasks.
  It depends on the fact that the WPSortFilterProxyModel holds a flat list.
*/
class PLANMODELS_EXPORT WorkPackageProxyModel : public QAbstractProxyModel
{
    Q_OBJECT
public:
    explicit WorkPackageProxyModel(QObject *parent = nullptr);

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    Qt::DropActions supportedDropActions() const override;
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    QStringList mimeTypes () const override;

    void setSourceModel(QAbstractItemModel *sourceModel) override;
    bool hasChildren(const QModelIndex &parent) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;

    NodeItemModel *baseModel() const;

    QModelIndex mapToSource(const QModelIndex &proxyIndex) const override;
    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const override;

    Task *taskFromIndex(const QModelIndex &idx) const;
    QModelIndex indexFromTask(const Node *node) const;

Q_SIGNALS:
    void loadWorkPackage(QList<QString>);

public Q_SLOTS:
    void setProject(KPlato::Project *project);
    void setScheduleManager(KPlato::ScheduleManager *sm);

protected Q_SLOTS:
    void sourceDataChanged(const QModelIndex& start, const QModelIndex& end);
    void sourceModelAboutToBeReset();
    void sourceModelReset();
    void sourceRowsAboutToBeInserted(const QModelIndex&, int, int);
    void sourceRowsInserted(const QModelIndex&, int, int end);
    void sourceRowsAboutToBeRemoved(const QModelIndex& parent, int start, int end);
    void sourceRowsRemoved(const QModelIndex& parent, int start, int);
    void sourceRowsAboutToBeMoved(const QModelIndex&, int sourceStart, int sourceEnd, const QModelIndex&, int destStart);
    void sourceRowsMoved(const QModelIndex&, int , int , const QModelIndex&, int);
    void workPackageToBeAdded(KPlato::Node*, int);
    void workPackageAdded(KPlato::Node*);
    void workPackageToBeRemoved(KPlato::Node*, int);
    void workPackageRemoved(KPlato::Node*);

protected:
    QModelIndex mapFromBaseModel(const QModelIndex &idx) const;
    void detachTasks(Task *task = nullptr);
    void attachTasks(Task *task = nullptr);

    inline bool isTaskIndex(const QModelIndex &idx) const {
        return idx.isValid() && ! idx.internalPointer();
    }
    inline bool isWorkPackageIndex(const QModelIndex &idx) const {
        return idx.isValid() && idx.internalPointer();
    }

private:
    WorkPackageModel m_model;
    NodeItemModel *m_nodemodel;
    QList<QAbstractProxyModel*> m_proxies;
};

} //namespace KPlato

#endif //WORKPACKAGEMODEL_H
