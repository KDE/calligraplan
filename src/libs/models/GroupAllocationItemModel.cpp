/* This file is part of the KDE project
 * Copyright (C) 2020 Dag Andersen <danders@get2net>
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

// clazy:excludeall=qstring-arg
#include "GroupAllocationItemModel.h"

#include "RequieredResourceDelegate.h"
#include "kptcommonstrings.h"
#include "kptcommand.h"
#include "kptitemmodelbase.h"
#include "kptcalendar.h"
#include "kptduration.h"
#include "kptnode.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptresource.h"
#include "kptdatetime.h"
#include "kptdebug.h"

#include <KLocalizedString>

#include <QStringList>


using namespace KPlato;

//--------------------------------------

GroupAllocationModel::GroupAllocationModel(QObject *parent)
    : QObject(parent),
    m_project(0),
    m_task(0)
{
}

GroupAllocationModel::~GroupAllocationModel()
{
}

const QMetaEnum GroupAllocationModel::columnMap() const
{
    return metaObject()->enumerator(metaObject()->indexOfEnumerator("Properties"));
}

void GroupAllocationModel::setProject(Project *project)
{
    m_project = project;
}

void GroupAllocationModel::setTask(Task *task)
{
    m_task = task;
}

int GroupAllocationModel::propertyCount() const
{
    return columnMap().keyCount();
}

QVariant GroupAllocationModel::name(const  ResourceGroup *res, int role) const
{
    //debugPlan<<res->name()<<","<<role;
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case Qt::ToolTipRole:
            return res->name();
            break;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant GroupAllocationModel::type(const ResourceGroup *res, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case Qt::ToolTipRole:
            return res->typeToString(true);
        case Role::EnumList:
            return res->typeToStringList(true);
        case Role::EnumListValue:
            return (int)res->type();
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant GroupAllocationModel::allocation(const ResourceGroup *group, int role) const
{
    if (m_project == 0 || m_task == 0) {
        return QVariant();
    }
    const ResourceGroupRequest *req = m_task->requests().find(group);
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return req ? req->units() : 0;
        case Qt::ToolTipRole:
            return QVariant();
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Role::Minimum:
            return 0;
        case Role::Maximum:
            return group->numResources();
    }
    return QVariant();
}

QVariant GroupAllocationModel::maximum(const ResourceGroup *group, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return group->numResources();
        case Qt::ToolTipRole:
            return i18np("There is %1 resource available in this group", "There are %1 resources available in this group", group->numResources());
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant GroupAllocationModel::data(const ResourceGroup *group, int property, int role) const
{
    QVariant result;
    if (group == nullptr) {
        return result;
    }
    switch (property) {
        case RequestName: result = name(group, role); break;
        case RequestType: result = type(group, role); break;
        case RequestAllocation: result = allocation(group, role); break;
        case RequestMaximum: result = maximum(group, role); break;
        default:
            if (role == Qt::DisplayRole) {
                if (property < propertyCount()) {
                    result = QString();
                } else {
                    debugPlan<<"data: invalid display value column"<<property;
                    return QVariant();
                }
            }
            break;
    }
    return result;
}


QVariant GroupAllocationModel::headerData(int section, int role)
{
    if (role == Qt::DisplayRole) {
        switch (section) {
            case RequestName: return i18n("Name");
            case RequestType: return i18n("Type");
            case RequestAllocation: return i18n("Allocation");
            case RequestMaximum: return xi18nc("@title:column", "Available");
            default: return QVariant();
        }
    } else if (role == Qt::TextAlignmentRole) {
        switch (section) {
            case 0: return QVariant();
            default: return Qt::AlignCenter;
        }
    } else if (role == Qt::ToolTipRole) {
        switch (section) {
            case RequestName: return ToolTip::resourceName();
            case RequestType: return ToolTip::resourceType();
            case RequestAllocation: return i18n("Resource allocation");
            case RequestMaximum: return xi18nc("@info:tootip", "Available resources or resource units");
            default: return QVariant();
        }
    }
    return QVariant();
}
//--------------------------------------

GroupAllocationItemModel::GroupAllocationItemModel(QObject *parent)
    : ItemModelBase(parent)
{
}

GroupAllocationItemModel::~GroupAllocationItemModel()
{
}

void GroupAllocationItemModel::slotResourceToBeAdded(ResourceGroup *group, int row)
{
    //debugPlan<<group->name()<<","<<row;
//     beginInsertRows(index(group), row, row);
}

void GroupAllocationItemModel::slotResourceAdded(KPlato::Resource *resource)
{
//     connectSignals(resource, true);
//     endInsertRows();
}

void GroupAllocationItemModel::slotResourceToBeRemoved(KPlato::ResourceGroup *group, int row, KPlato::Resource *resource)
{
//     beginRemoveRows(index(group), row, row);
//     if (resource->groupCount() == 1 && resource->parentGroups().at(0) == group) {
//         connectSignals(resource, false);
//     }
}

void GroupAllocationItemModel::slotResourceRemoved()
{
//     endRemoveRows();
}

void GroupAllocationItemModel::slotResourceGroupToBeInserted(Project *project, int row)
{
    Q_UNUSED(project)
    beginInsertRows(QModelIndex(), row, row);
}

void GroupAllocationItemModel::slotResourceGroupInserted(ResourceGroup *group)
{
    //debugPlan<<group->name();
    connectSignals(group, true);
    endInsertRows();
}

void GroupAllocationItemModel::slotResourceGroupToBeRemoved(Project *project, int row, ResourceGroup *group)
{
    Q_UNUSED(project)
    beginRemoveRows(QModelIndex(), row, row);
    connectSignals(group, false);
}

void GroupAllocationItemModel::slotResourceGroupRemoved()
{
    endRemoveRows();
}

void GroupAllocationItemModel::setProject(Project *project)
{
    beginResetModel();
    if (m_project) {
        disconnect(m_project, &Project::aboutToBeDeleted, this, &GroupAllocationItemModel::projectDeleted);

        disconnect(m_project, &Project::resourceGroupChanged, this, &GroupAllocationItemModel::slotResourceGroupChanged);
        disconnect(m_project, &Project::resourceGroupToBeAdded, this, &GroupAllocationItemModel::slotResourceGroupToBeInserted);
        disconnect(m_project, &Project::resourceGroupAdded, this, &GroupAllocationItemModel::slotResourceGroupInserted);
        disconnect(m_project, &Project::resourceGroupToBeRemoved, this, &GroupAllocationItemModel::slotResourceGroupToBeRemoved);
        disconnect(m_project, &Project::resourceGroupRemoved, this, &GroupAllocationItemModel::slotResourceGroupRemoved);

        for (ResourceGroup *g : m_project->resourceGroups()) {
            connectSignals(g, false);
        }
    }
    m_project = project;
    if (m_project) {
        connect(m_project, &Project::aboutToBeDeleted, this, &GroupAllocationItemModel::projectDeleted);
        
        connect(m_project, &Project::resourceGroupChanged, this, &GroupAllocationItemModel::slotResourceGroupChanged);
        connect(m_project, &Project::resourceGroupToBeAdded, this, &GroupAllocationItemModel::slotResourceGroupToBeInserted);
        connect(m_project, &Project::resourceGroupAdded, this, &GroupAllocationItemModel::slotResourceGroupInserted);
        connect(m_project, &Project::resourceGroupToBeRemoved, this, &GroupAllocationItemModel::slotResourceGroupToBeRemoved);
        connect(m_project, &Project::resourceGroupRemoved, this, &GroupAllocationItemModel::slotResourceGroupRemoved);
        
        for (ResourceGroup *g : m_project->resourceGroups()) {
            connectSignals(g, true);
        }
    }
    m_model.setProject(m_project);
    endResetModel();
}

void GroupAllocationItemModel::connectSignals(ResourceGroup *group, bool enable)
{
    if (enable) {
        connect(group, &ResourceGroup::resourceToBeAdded, this, &GroupAllocationItemModel::slotResourceToBeAdded);
        connect(group, &ResourceGroup::resourceAdded, this, &GroupAllocationItemModel::slotResourceAdded);
        connect(group, &ResourceGroup::resourceToBeRemoved, this, &GroupAllocationItemModel::slotResourceToBeRemoved);
        connect(group, &ResourceGroup::resourceRemoved, this, &GroupAllocationItemModel::slotResourceRemoved);
    } else {
        disconnect(group, &ResourceGroup::resourceToBeAdded, this, &GroupAllocationItemModel::slotResourceToBeAdded);
        disconnect(group, &ResourceGroup::resourceAdded, this, &GroupAllocationItemModel::slotResourceAdded);
        disconnect(group, &ResourceGroup::resourceToBeRemoved, this, &GroupAllocationItemModel::slotResourceToBeRemoved);
        disconnect(group, &ResourceGroup::resourceRemoved, this, &GroupAllocationItemModel::slotResourceRemoved);
    }
    for (Resource *r : group->resources()) {
        connectSignals(r, enable);
    }
}

void GroupAllocationItemModel::connectSignals(Resource *resource, bool enable)
{
    if (enable) {
        connect(resource, &Resource::dataChanged, this, &GroupAllocationItemModel::slotResourceChanged, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
    } else {
        disconnect(resource, &Resource::dataChanged, this, &GroupAllocationItemModel::slotResourceChanged);
    }
}

void GroupAllocationItemModel::setTask(Task *task)
{
    if (task == m_model.task()) {
        return;
    }
    if (m_model.task() == nullptr) {
        beginResetModel();
        filldata(task);
        m_model.setTask(task);
        endResetModel();
        return;
    }
    if (task) {
        emit layoutAboutToBeChanged();
        filldata(task);
        m_model.setTask(task);
        emit layoutChanged();
    }
}

void GroupAllocationItemModel::filldata(Task *task)
{
    qDeleteAll(m_groupCache);
    m_groupCache.clear();
    if (m_project && task) {
        foreach (const ResourceGroupRequest *gr, task->requests().requests()) {
            m_groupCache[gr->group()] = new ResourceGroupRequest(*gr);
        }
    }
}

Qt::ItemFlags GroupAllocationItemModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = ItemModelBase::flags(index);
    if (!m_readWrite) {
        //debugPlan<<"read only"<<flags;
        return flags &= ~Qt::ItemIsEditable;
    }
    if (!index.isValid()) {
        //debugPlan<<"invalid"<<flags;
        return flags;
    }
    switch (index.column()) {
        case GroupAllocationModel::RequestAllocation:
            flags |= Qt::ItemIsEditable;
            break;
        default:
            flags &= ~Qt::ItemIsEditable;
            break;
    }
    return flags;
}

QModelIndex GroupAllocationItemModel::parent(const QModelIndex &index) const
{
    if (!index.isValid() || m_project == 0) {
        return QModelIndex();
    }
    //debugPlan<<index.internalPointer()<<":"<<index.row()<<","<<index.column();

    Resource *r = qobject_cast<Resource*>(object(index));
    if (r && r->parentGroups().value(0)) {
        // only resources have parent
        int row = m_project->indexOf(r->parentGroups().value(0));
        return createIndex(row, 0, r->parentGroups().value(0));
    }

    return QModelIndex();
}

QModelIndex GroupAllocationItemModel::index(int row, int column, const QModelIndex &parent) const
{
    if (m_project == 0 || column < 0 || column >= columnCount() || row < 0) {
        return QModelIndex();
    }
    if (! parent.isValid()) {
        if (row < m_project->numResourceGroups()) {
            return createIndex(row, column, m_project->resourceGroupAt(row));
        }
        return QModelIndex();
    }
    QObject *p = object(parent);
    ResourceGroup *g = qobject_cast<ResourceGroup*>(p);
    if (g) {
        if (row < g->numResources()) {
            return createIndex(row, column, g->resourceAt(row));
        }
        return QModelIndex();
    }
    return QModelIndex();
}

QModelIndex GroupAllocationItemModel::index(const ResourceGroup *group) const
{
    if (m_project == 0 || group == 0) {
        return QModelIndex();
    }
    ResourceGroup *g = const_cast<ResourceGroup*>(group);
    int row = m_project->indexOf(g);
    return createIndex(row, 0, g);

}

int GroupAllocationItemModel::columnCount(const QModelIndex &/*parent*/) const
{
    return m_model.propertyCount();
}

int GroupAllocationItemModel::rowCount(const QModelIndex &parent) const
{
    if (m_project == 0 || m_model.task() == 0) {
        return 0;
    }
    if (! parent.isValid()) {
        return m_project->numResourceGroups();
    }
    return 0;
}

int GroupAllocationItemModel::requestedResources(const ResourceGroup *group) const
{
    int c = 0;
//     foreach (const Resource *r, group->resources()) {
//         if (m_resourceCache.contains(r) &&  m_resourceCache[r]->units() > 0) {
//             ++c;
//         }
//     }
    return c;
}

QVariant GroupAllocationItemModel::allocation(const ResourceGroup *group, int role) const
{
    if (m_model.task() == nullptr) {
        return QVariant();
    }
    if (!m_groupCache.contains(group)) {
        return m_model.allocation(group, role);
    }
    switch (role) {
        case Qt::DisplayRole:
            return QString(" %1 (%2)")
                        .arg(qMax(m_groupCache[group]->units(), allocation(group, Role::Minimum).toInt()))
                        .arg(requestedResources(group));
        case Qt::EditRole:
            return std::max(m_groupCache[group]->units(), allocation(group, Role::Minimum).toInt());
        case Qt::ToolTipRole: {
            QString s1 = i18ncp("@info:tooltip",
                                "%1 resource requested for dynamic allocation",
                                "%1 resources requested for dynamic allocation",
                                allocation(group, Qt::EditRole).toInt());
            QString s2 = i18ncp("@info:tooltip",
                                "%1 resource allocated",
                                "%1 resources allocated",
                                requestedResources(group));

            return xi18nc("@info:tooltip", "%1<nl/>%2", s1, s2);
        }
        case Qt::WhatsThisRole: {
            return xi18nc("@info:whatsthis",
                          "<title>Group allocations</title>"
                          "<para>You can allocate a number of resources from a group and let"
                          " the scheduler select from the available resources at the time of scheduling.</para>"
                          " These dynamically allocated resources will be in addition to any resource you have allocated specifically.");
        }
        case Role::Minimum: {
            return 0;
        }
        case Role::Maximum: {
            return group->numResources() - requestedResources(group);
        }
        default:
            return m_model.allocation(group, role);
    }
    return QVariant();
}

bool GroupAllocationItemModel::setAllocation(ResourceGroup *group, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole:
            if (!m_groupCache.contains(group)) {
                m_groupCache[group] = new ResourceGroupRequest(group, 0);
            }
            m_groupCache[group]->setUnits(value.toInt());
            emit dataChanged(index(group), index(group));
            return true;
    }
    return false;
}

QVariant GroupAllocationItemModel::maximum(const ResourceGroup *group, int role) const
{
    switch (role) {
        case Qt::DisplayRole: {
            int c = group->numResources() - requestedResources(group);
            if (m_groupCache.contains(group)) {
                c -= m_groupCache[group]->units();
            }
            return i18nc("1: free resources, 2: number of resources", "%1 of %2", c, group->numResources());
        }
        case Qt::ToolTipRole:
            return xi18ncp("@info:tooltip", "There is %1 resource available in this group", "There are %1 resources available in this group", group->numResources());
        default:
            return m_model.maximum(group, role);
    }
    return QVariant();
}

QVariant GroupAllocationItemModel::notUsed(const ResourceGroup *, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            return QString(" ");
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::EditRole:
        case Qt::ToolTipRole:
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant GroupAllocationItemModel::data(const QModelIndex &index, int role) const
{
    QVariant result;
    QObject *obj = object(index);
    if (obj == nullptr) {
        return QVariant();
    }
    if (role == Qt::TextAlignmentRole) {
        // use same alignment as in header (headers always horizontal)
        return headerData(index.column(), Qt::Horizontal, role);
    }
    ResourceGroup *g = qobject_cast<ResourceGroup*>(obj);
    if (g) {
        switch (index.column()) {
            case GroupAllocationModel::RequestAllocation:
                result = allocation(g, role);
                break;
            case GroupAllocationModel::RequestMaximum:
                result = maximum(g, role);
                break;
            default:
                result = m_model.data(g, index.column(), role);
                break;
        }
    }
    return result;
}

bool GroupAllocationItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (! index.isValid()) {
        return ItemModelBase::setData(index, value, role);
    }
    qInfo()<<Q_FUNC_INFO<<index<<value<<role;
    if (!(flags(index) & Qt::ItemIsEditable)) {
        return false;
    }
    QObject *obj = object(index);
    ResourceGroup *g = qobject_cast<ResourceGroup*>(obj);
    if (g) {
        switch (index.column()) {
            case GroupAllocationModel::RequestAllocation:
                if (setAllocation(g, value, role)) {
                    emit dataChanged(index, index);
                    return true;
                }
                return false;
            default:
                //qWarning("data: invalid display value column %d", index.column());
                return false;
        }
    }
    return false;
}

QVariant GroupAllocationItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if (role == Qt::DisplayRole) {
            return m_model.headerData(section, role);
        }
        if (role == Qt::TextAlignmentRole) {
            switch (section) {
                case 0: return QVariant();
                default: return Qt::AlignCenter;
            }
            return Qt::AlignCenter;
        }
    }
    return m_model.headerData(section, role);
}

QAbstractItemDelegate *GroupAllocationItemModel::createDelegate(int col, QWidget *parent) const
{
    switch (col) {
        case GroupAllocationModel::RequestAllocation: return new SpinBoxDelegate(parent);
        default: break;
    }
    return nullptr;
}

QObject *GroupAllocationItemModel::object(const QModelIndex &index) const
{
    QObject *o = 0;
    if (index.isValid()) {
        o = static_cast<QObject*>(index.internalPointer());
        Q_ASSERT(o);
    }
    return o;
}

void GroupAllocationItemModel::slotResourceChanged(Resource *res)
{
//     for (ResourceGroup *g : res->parentGroups()) {
//         int row = g->indexOf(res);
//         emit dataChanged(createIndex(row, 0, res), createIndex(row, columnCount() - 1, res));
//     }
}

void GroupAllocationItemModel::slotResourceGroupChanged(ResourceGroup *group)
{
    int row = m_project->indexOf(group);
    emit dataChanged(createIndex(row, 0, group), createIndex(row, columnCount() - 1, group));
}
