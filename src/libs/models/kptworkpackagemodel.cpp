/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2007, 2012 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptworkpackagemodel.h"

#include "kptglobal.h"
#include "kptcommonstrings.h"
#include "kptflatproxymodel.h"
#include "kptnodeitemmodel.h"
#include "kptnode.h"
#include "kpttask.h"
#include "kptproject.h"
#include "kptschedule.h"
#include "kptdebug.h"

#include <KLocalizedString>

#include <QModelIndex>
#include <QVariant>
#include <QMimeData>

namespace KPlato
{

QVariant WorkPackageModel::nodeName(const WorkPackage *wp, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            return wp->parentTask() ? wp->parentTask()->name() : QLatin1String("");
        case Qt::EditRole:
            return wp->parentTask() ? wp->parentTask()->name() : QLatin1String("");
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}


QVariant WorkPackageModel::ownerName(const WorkPackage *wp, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            return wp->ownerName();
        case Qt::EditRole:
            return wp->ownerName();
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant WorkPackageModel::transmitionStatus(const WorkPackage *wp, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            return wp->transmitionStatusToString(wp->transmitionStatus(), true);
        case Qt::EditRole:
            return wp->transmitionStatus();
        case Qt::ToolTipRole: {
            int sts = wp->transmitionStatus();
            if (sts == WorkPackage::TS_Send) {
                return i18n("Sent to %1 at %2", wp->ownerName(), transmitionTime(wp, Qt::DisplayRole).toString());
            }
            if (sts == WorkPackage::TS_Receive) {
                return i18n("Received from %1 at %2", wp->ownerName(), transmitionTime(wp, Qt::DisplayRole).toString());
            }
            return i18n("Not available");
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant WorkPackageModel::transmitionTime(const WorkPackage *wp, int role) const
{
    if (! wp) {
        return QVariant();
    }
    switch (role) {
        case Qt::DisplayRole:
            return QLocale().toString(wp->transmitionTime(), QLocale::ShortFormat);
        case Qt::EditRole:
            return wp->transmitionTime();
        case Qt::ToolTipRole: {
            int sts = wp->transmitionStatus();
            QString t = QLocale().toString(wp->transmitionTime(), QLocale::LongFormat);
            if (sts == WorkPackage::TS_Send) {
                return i18n("Work package sent at: %1", t);
            }
            if (sts == WorkPackage::TS_Receive) {
                return i18n("Work package transmission received at: %1", t);
            }
            return i18n("Not available");
        }

        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant WorkPackageModel::completion(const WorkPackage *wp, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            if (wp->transmitionStatus() == WorkPackage::TS_Receive) {
                return wp->completion().percentFinished();
            }
            break;
        case Qt::EditRole:
            if (wp->transmitionStatus() == WorkPackage::TS_Receive) {
                return wp->completion().percentFinished();
            }
            break;
        case Qt::ToolTipRole:
            if (wp->transmitionStatus() == WorkPackage::TS_Receive) {
                return i18n("Task reported %1% completed", wp->completion().percentFinished());
            }
            break;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant WorkPackageModel::data(const WorkPackage *wp, int column, int role) const
{
    switch (column) {
        case NodeModel::WPOwnerName:
        case NodeModel::NodeName: return ownerName(wp, role);
        case NodeModel::WPTransmitionStatus:
        case NodeModel::NodeStatus: return transmitionStatus(wp, role);
        case NodeModel::NodeCompleted: return completion(wp, role);
        case NodeModel::WPTransmitionTime:
        case NodeModel::NodeActualStart: return transmitionTime(wp, role);

        default: break;
    }
    return QVariant();
}

//-----------------------------
bool WPSortFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if (sourceModel()->index(source_row, NodeModel::NodeType, source_parent).data(Qt::EditRole).toInt() != Node::Type_Task) {
        return false;
    }
    if (sourceModel()->index(source_row, NodeModel::NodeStatus, source_parent).data(Qt::EditRole).toInt() & Node::State_NotScheduled) {
        return false;
    }
    return true;
}

WorkPackageProxyModel::WorkPackageProxyModel(QObject *parent)
    : QAbstractProxyModel(parent)
{
    m_proxies << new WPSortFilterProxyModel(this);
    m_proxies << new FlatProxyModel(this);
    m_nodemodel = new NodeItemModel(this);
    QAbstractProxyModel *p = this;
    for (QAbstractProxyModel *m : std::as_const(m_proxies)) {
        p->setSourceModel(m);
        p = m;
    }
    p->setSourceModel(m_nodemodel);

}

Qt::ItemFlags WorkPackageProxyModel::flags(const QModelIndex &index) const
{
    if (isWorkPackageIndex(index)) {
        return Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;
    }
    return QAbstractProxyModel::flags(index) | Qt::ItemIsDropEnabled;
}

Qt::DropActions WorkPackageProxyModel::supportedDropActions() const
{
    return Qt::CopyAction;
}

bool WorkPackageProxyModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(action);
    Q_UNUSED(row);
    Q_UNUSED(column)
    Q_UNUSED(parent);

    if (data->hasFormat(QStringLiteral("text/uri-list"))) {
        const auto files = QString::fromUtf8(data->data(QStringLiteral("text/uri-list"))).split(QStringLiteral("\r\n"), Qt::SkipEmptyParts);
        for (const QString &f : files) {
            if (f.endsWith(QStringLiteral(".planwork"))) {
                return true;
            }
        }
    }
    return false;
}

bool WorkPackageProxyModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (!canDropMimeData(data, action, row, column, parent)) {
        return false;
    }
    if (action == Qt::IgnoreAction) {
        return true;
    }
    if (data->hasFormat(QStringLiteral("text/uri-list"))) {
        QStringList files;
        const auto uris = QString::fromUtf8(data->data(QStringLiteral("text/uri-list"))).split(QStringLiteral("\r\n"), Qt::SkipEmptyParts);
        for (const QString &f : uris) {
            if (f.endsWith(QStringLiteral(".planwork"))) {
                files << f;
            }
            Q_EMIT loadWorkPackage(files);
        }
        return true;
    }
    return false;
}

QStringList WorkPackageProxyModel::mimeTypes () const
{
    return QStringList() << QStringLiteral("application/x-vnd.kde.plan.work");
}

void WorkPackageProxyModel::setSourceModel(QAbstractItemModel *model)
{
    if (sourceModel()) {
        disconnect(sourceModel(), &QAbstractItemModel::dataChanged,
                this, &WorkPackageProxyModel::sourceDataChanged);
/*        disconnect(sourceModel(), SIGNAL(headerDataChanged(Qt::Orientation,int,int)),
                this, SLOT(sourceHeaderDataChanged(Qt::Orientation,int,int)));*/
        disconnect(sourceModel(), &QAbstractItemModel::layoutChanged,
                this, &QAbstractItemModel::layoutChanged);
        disconnect(sourceModel(), &QAbstractItemModel::layoutAboutToBeChanged,
                this, &QAbstractItemModel::layoutAboutToBeChanged);
        disconnect(sourceModel(), &QAbstractItemModel::rowsAboutToBeInserted,
                this, &WorkPackageProxyModel::sourceRowsAboutToBeInserted);
        disconnect(sourceModel(), &QAbstractItemModel::rowsInserted,
                this, &WorkPackageProxyModel::sourceRowsInserted);
        disconnect(sourceModel(), &QAbstractItemModel::rowsAboutToBeRemoved,
                this, &WorkPackageProxyModel::sourceRowsAboutToBeRemoved);
        disconnect(sourceModel(), &QAbstractItemModel::rowsRemoved,
                this, &WorkPackageProxyModel::sourceRowsAboutToBeRemoved);
/*
        disconnect(sourceModel(), SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)),
                this, SLOT(sourceColumnsAboutToBeInserted(QModelIndex,int,int)));
        disconnect(sourceModel(), SIGNAL(columnsInserted(QModelIndex,int,int)),
                this, SLOT(sourceColumnsInserted(QModelIndex,int,int)));

        disconnect(sourceModel(), SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
                this, SLOT(sourceColumnsAboutToBeRemoved(QModelIndex,int,int)));
        disconnect(sourceModel(), SIGNAL(columnsRemoved(QModelIndex,int,int)),
                this, SLOT(sourceColumnsRemoved(QModelIndex,int,int)));
        */
        disconnect(sourceModel(), &QAbstractItemModel::modelAboutToBeReset, this, &WorkPackageProxyModel::sourceModelAboutToBeReset);
        disconnect(sourceModel(), &QAbstractItemModel::modelReset, this, &WorkPackageProxyModel::sourceModelReset);

        disconnect(sourceModel(), &QAbstractItemModel::rowsAboutToBeMoved,
                this, &WorkPackageProxyModel::sourceRowsAboutToBeMoved);
        disconnect(sourceModel(), &QAbstractItemModel::rowsMoved,
                this, &WorkPackageProxyModel::sourceRowsMoved);
/*
        disconnect(sourceModel(), SIGNAL(columnsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)),
                this, SLOT(sourceColumnsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)));
        disconnect(sourceModel(), SIGNAL(columnsMoved(QModelIndex&parent,int,int,QModelIndex,int)),
                this, SLOT(sourceColumnsMoved(QModelIndex&parent,int,int,QModelIndex,int)));*/
    }
    QAbstractProxyModel::setSourceModel(model);
    if (model) {
        connect(model, &QAbstractItemModel::dataChanged,
                this, &WorkPackageProxyModel::sourceDataChanged);
/*        connect(model, SIGNAL(headerDataChanged(Qt::Orientation,int,int)),
                this, SLOT(sourceHeaderDataChanged(Qt::Orientation,int,int)));*/
        connect(model, &QAbstractItemModel::layoutChanged,
                this, &QAbstractItemModel::layoutChanged);
        connect(model, &QAbstractItemModel::layoutAboutToBeChanged,
                this, &QAbstractItemModel::layoutAboutToBeChanged);
        connect(model, &QAbstractItemModel::rowsAboutToBeInserted,
                this, &WorkPackageProxyModel::sourceRowsAboutToBeInserted);
        connect(model, &QAbstractItemModel::rowsInserted,
                this, &WorkPackageProxyModel::sourceRowsInserted);
        connect(model, &QAbstractItemModel::rowsAboutToBeRemoved,
                this, &WorkPackageProxyModel::sourceRowsAboutToBeRemoved);
        connect(model, &QAbstractItemModel::rowsRemoved,
                this, &WorkPackageProxyModel::sourceRowsRemoved);
/*
        connect(model, SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)),
                this, SLOT(sourceColumnsAboutToBeInserted(QModelIndex,int,int)));
        connect(model, SIGNAL(columnsInserted(QModelIndex,int,int)),
                this, SLOT(sourceColumnsInserted(QModelIndex,int,int)));

        connect(model, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
                this, SLOT(sourceColumnsAboutToBeRemoved(QModelIndex,int,int)));
        connect(model, SIGNAL(columnsRemoved(QModelIndex,int,int)),
                this, SLOT(sourceColumnsRemoved(QModelIndex,int,int)));
        */
        connect(model, &QAbstractItemModel::modelAboutToBeReset, this, &WorkPackageProxyModel::sourceModelAboutToBeReset);
        connect(model, &QAbstractItemModel::modelReset, this, &WorkPackageProxyModel::sourceModelReset);

        connect(model, &QAbstractItemModel::rowsAboutToBeMoved,
                this, &WorkPackageProxyModel::sourceRowsAboutToBeMoved);
        connect(model, &QAbstractItemModel::rowsMoved,
                this, &WorkPackageProxyModel::sourceRowsMoved);
/*
        connect(model, SIGNAL(columnsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)),
                this, SLOT(sourceColumnsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)));
        connect(model, SIGNAL(columnsMoved(QModelIndex&parent,int,int,QModelIndex,int)),
                this, SLOT(sourceColumnsMoved(QModelIndex&parent,int,int,QModelIndex,int)));*/
    }
}

void WorkPackageProxyModel::sourceDataChanged(const QModelIndex &start, const QModelIndex &end)
{
    Q_EMIT dataChanged(mapFromSource(start), mapFromSource(end));
}

void WorkPackageProxyModel::sourceModelAboutToBeReset()
{
//    debugPlan;
    beginResetModel();
    detachTasks();
}

void WorkPackageProxyModel::sourceModelReset()
{
//    debugPlan;
    attachTasks();
#if 0
    for (int r = 0; r < rowCount(); ++r) {
        debugPlan<<index(r, 0).data();
    }
#endif
    endResetModel();
}

void WorkPackageProxyModel::sourceRowsAboutToBeInserted(const QModelIndex &parent, int start, int end)
{
    debugPlan<<parent<<start<<end;
    Q_ASSERT(! parent.isValid());
    beginInsertRows(QModelIndex(), start, end);
}

void WorkPackageProxyModel::sourceRowsInserted(const QModelIndex &parent, int start, int end)
{
    debugPlan<<parent<<start<<end<<":"<<rowCount();
    Q_ASSERT(! parent.isValid());
    for (int r = start; r <= end; ++r) {
        QModelIndex i = index(r, 0);
        Task *task = taskFromIndex(i);
        if (task) {
            attachTasks(task);
        }
    }
    endInsertRows();
}

void WorkPackageProxyModel::sourceRowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    debugPlan<<parent<<start<<end;
    Q_ASSERT(! parent.isValid());
    beginInsertRows(QModelIndex(), start, end);
}

void WorkPackageProxyModel::sourceRowsRemoved(const QModelIndex &parent, int start, int end)
{
    debugPlan<<parent<<start<<end;
    Q_ASSERT(! parent.isValid());
    for (int r = start; r <= end; ++r) {
        QModelIndex i = index(r, 0);
        Task *task = taskFromIndex(i);
        if (task) {
            detachTasks(task);
        }
    }
    endInsertRows();
}

void WorkPackageProxyModel::sourceRowsAboutToBeMoved(const QModelIndex&, int sourceStart, int sourceEnd, const QModelIndex&, int destStart)
{
    beginMoveRows(QModelIndex(), sourceStart, sourceEnd, QModelIndex(), destStart);
}

void WorkPackageProxyModel::sourceRowsMoved(const QModelIndex &, int , int , const QModelIndex &, int)
{
    endMoveRows();
}

bool WorkPackageProxyModel::hasChildren(const QModelIndex &parent) const
{
    return rowCount(parent) > 0;
}

int WorkPackageProxyModel::rowCount(const QModelIndex &parent) const
{
    int rows = 0;
    if (! parent.isValid()) {
        rows = sourceModel()->rowCount();
    } else if (isTaskIndex(parent)) {
        Task *task = taskFromIndex(parent);
        rows = task ? task->workPackageLogCount() : 0;
    }
//    debugPlan<<rows;
    for (int r = 0; r < rows; ++r) {
//        debugPlan<<r<<index(r, 0).data();
    }
    return rows;
}

int WorkPackageProxyModel::columnCount(const QModelIndex &/*parent*/) const
{
    return sourceModel()->columnCount();
}

QModelIndex WorkPackageProxyModel::mapToSource(const QModelIndex &proxyIndex) const
{
    if (! proxyIndex.isValid()) {
        return QModelIndex();
    }
    if (isWorkPackageIndex(proxyIndex)) {
        // workpackage, not mapped to source model
        return QModelIndex();
    }
    return sourceModel()->index(proxyIndex.row(), proxyIndex.column());
}

QModelIndex WorkPackageProxyModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    // index from source model is always a node
    return createIndex(sourceIndex.row(), sourceIndex.column());
}

QModelIndex WorkPackageProxyModel::parent(const QModelIndex &child) const
{
    QModelIndex idx;
    if (isWorkPackageIndex(child)) {
        // only work packages have parent
        idx = m_nodemodel->index(static_cast<Node*>(child.internalPointer()));
        idx = mapFromBaseModel(idx);
    }
//    debugPlan<<child<<idx;
    return idx;
}

QModelIndex WorkPackageProxyModel::index(int row, int column, const QModelIndex &parent) const
{
    QModelIndex idx;
    if (! parent.isValid()) {
        // this should be a node
        idx = createIndex(row, column);
    } else if (isTaskIndex(parent)) {
        // Should be a work package, parent should be a task
        Task *task = taskFromIndex(parent);
        if (task) {
            idx = createIndex(row, column, task);
        }
    }
/*    if (! idx.isValid()) {
        debugPlan<<"not valid:"<<parent<<row<<column<<idx;
    } else {
        debugPlan<<parent<<row<<column<<idx;
    }*/
    return idx;
}

QVariant WorkPackageProxyModel::data(const QModelIndex &idx, int role) const
{
    QVariant value;
    if (isTaskIndex(idx)) {
        value = mapToSource(idx).data(role);
    } else if (isWorkPackageIndex(idx)) {
        Task *task = taskFromIndex(idx);
        if (task) {
            value = m_model.data(task->workPackageAt(idx.row()), idx.column(), role);
        }
    }
//    debugPlan<<idx<<value;
    return value;
}

Task *WorkPackageProxyModel::taskFromIndex(const QModelIndex &idx) const
{
    Task *task = nullptr;
    if (idx.internalPointer()) {
        task = static_cast<Task*>(idx.internalPointer());
    } else if (idx.isValid()) {
        QVariant obj = data(idx, Role::Object);
        task = qobject_cast<Task*>(obj.value<QObject*>());
    }
//    debugPlan<<idx<<task;
    return task;
}

QModelIndex WorkPackageProxyModel::indexFromTask(const Node *node) const
{
    return mapFromBaseModel(m_nodemodel->index(node));
}

QModelIndex WorkPackageProxyModel::mapFromBaseModel(const QModelIndex &idx) const
{
    if (! idx.isValid()) {
        return QModelIndex();
    }
    QModelIndex in = idx;
    for (int i = m_proxies.count() -1; i >= 0; --i) {
        in = m_proxies.at(i)->mapFromSource(in);
    }
    return mapFromSource(in);
}
void WorkPackageProxyModel::setProject(Project *project)
{
    debugPlan<<project;
    m_nodemodel->setProject(project);
}

void WorkPackageProxyModel::setScheduleManager(ScheduleManager *sm)
{
    debugPlan<<sm;
    m_nodemodel->setScheduleManager(sm);
}

NodeItemModel *WorkPackageProxyModel::baseModel() const
{
    return m_nodemodel;
}

void WorkPackageProxyModel::detachTasks(Task *task)
{
    if (task) {
        disconnect(task, &Task::workPackageToBeAdded, this, &WorkPackageProxyModel::workPackageToBeAdded);
        disconnect(task, &Task::workPackageAdded, this, &WorkPackageProxyModel::workPackageAdded);
        disconnect(task, &Task::workPackageToBeRemoved, this, &WorkPackageProxyModel::workPackageToBeRemoved);
        disconnect(task, &Task::workPackageRemoved, this, &WorkPackageProxyModel::workPackageRemoved);
//        debugPlan<<task;
    } else {
        for (int r = 0; r < rowCount(); ++r) {
            Task *t = taskFromIndex(index(r, 0));
            if (t) {
                detachTasks(t);
            }
        }
    }
}

void WorkPackageProxyModel::attachTasks(Task *task)
{
    if (task) {
        connect(task, &Task::workPackageToBeAdded, this, &WorkPackageProxyModel::workPackageToBeAdded);
        connect(task, &Task::workPackageAdded, this, &WorkPackageProxyModel::workPackageAdded);
        connect(task, &Task::workPackageToBeRemoved, this, &WorkPackageProxyModel::workPackageToBeRemoved);
        connect(task, &Task::workPackageRemoved, this, &WorkPackageProxyModel::workPackageRemoved);
//        debugPlan<<task;
    } else {
        for (int r = 0; r < rowCount(); ++r) {
            Task *t = taskFromIndex(index(r, 0));
            if (t) {
                attachTasks(t);
            }
        }
    }
}

void WorkPackageProxyModel::workPackageToBeAdded(Node *node, int row)
{
    QModelIndex idx = indexFromTask(node);
    debugPlan<<node<<row<<idx;
    beginInsertRows(idx, row, row);
}

void WorkPackageProxyModel::workPackageAdded(Node *)
{
    endInsertRows();
}

void WorkPackageProxyModel::workPackageToBeRemoved(Node *node, int row)
{
    QModelIndex idx = indexFromTask(node);
    beginRemoveRows(idx, row, row);
}

void WorkPackageProxyModel::workPackageRemoved(Node *)
{
    endRemoveRows();
}

} //namespace KPlato
