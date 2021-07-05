/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2009, 2012 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptresourceallocationmodel.h"

#include "AlternativeResourceDelegate.h"
#include "RequieredResourceDelegate.h"
#include <kptresourcerequest.h>
#include "kptcommonstrings.h"
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

ResourceAlternativesModel::ResourceAlternativesModel(ResourceAllocationItemModel *dataModel, QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_resource(nullptr)
{
    Q_ASSERT(dataModel);
    setSourceModel(dataModel);
    connect(dataModel, &ResourceAllocationItemModel::dataChanged, this, &ResourceAlternativesModel::slotDataChanged);
}

void ResourceAlternativesModel::slotDataChanged()
{
    beginResetModel();
    endResetModel();
}

void ResourceAlternativesModel::setResource(Resource *resource)
{
    beginResetModel();
    m_resource = resource;
    endResetModel();
}

Resource *ResourceAlternativesModel::resource() const
{
    return m_resource;
}

Resource *ResourceAlternativesModel::resource(const QModelIndex &idx) const
{
    QVariant v = QSortFilterProxyModel::data(idx, Role::Object);
    return v.value<Resource*>();
}

ResourceRequest *ResourceAlternativesModel::request(const QModelIndex &idx) const
{
    const ResourceAllocationItemModel *m = static_cast<ResourceAllocationItemModel*>(sourceModel());
    return m->alternativeRequest(m_resource, resource(idx));
}

bool ResourceAlternativesModel::filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const
{
    Q_UNUSED(source_parent)
    switch (source_column) {
        case ResourceAllocationModel::RequestName:
        case ResourceAllocationModel::RequestAllocation:
        case ResourceAllocationModel::RequestMaximum:
            return true;
        default:
            break;
    }
    return false;
}

bool ResourceAlternativesModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if (!m_resource || source_parent.isValid()) {
        return false;
    }
    auto m = static_cast<ResourceAllocationItemModel*>(sourceModel());
    const QModelIndex idx = m->index(source_row, 0);
    auto r = m->resource(idx);
    return r && (r != m_resource) && (r->type() == m_resource->type());
}

Qt::ItemFlags ResourceAlternativesModel::flags(const QModelIndex &idx) const
{
    if (!m_resource) {
        return Qt::NoItemFlags;
    }
    const ResourceAllocationItemModel *m = static_cast<ResourceAllocationItemModel*>(sourceModel());
    if (!m->resourceCache().contains(m_resource)) {
        return Qt::NoItemFlags;
    }
    return QSortFilterProxyModel::flags(idx);
}

QVariant ResourceAlternativesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
//     if (orientation == Qt::Vertical) {
//         return QVariant();
//     }
//     if (role == Qt::DisplayRole) {
//         switch (section) {
//             case 0: return i18n("Alternative");
//             default: break;
//         }
//     }
    return QSortFilterProxyModel::headerData(section, orientation, role);
}

QVariant ResourceAlternativesModel::data(const QModelIndex &idx, int role) const
{
    QModelIndex aIdx = mapToSource(idx);
    switch (aIdx.column()) {
        case ResourceAllocationModel::RequestAllocation: {
            ResourceRequest *alternative = request(idx);
            if (role == Qt::CheckStateRole) {
                return alternative ? Qt::Checked : Qt::Unchecked;
            } else if (role == Qt::DisplayRole) {
                int units = alternative ? alternative->units() : 0;
                // xgettext: no-c-format
                return i18nc("<value>%", "%1%", units);
            } else if (role == Qt::EditRole) {
                return alternative ? alternative->units() : 0;
            } else if (role == Role::Minimum) {
                break;
            } else if (role == Role::Maximum) {
                break;
            } if (role == Qt::ToolTipRole) {
                int units = alternative ? alternative->units() : 0;
                if (units == 0) {
                    return xi18nc("@info:tooltip", "Not allocated");
                }
                QString max = mapToSource(idx).siblingAtColumn(ResourceAllocationModel::RequestMaximum).data(Qt::DisplayRole).toString();
                QString alloced = i18nc("<value>%", "%1%", units);
                return xi18nc("@info:tooltip", "%1 allocated out of %2 available", alloced, max);
            }
            break;
        }
    }
    return QSortFilterProxyModel::data(idx, role);
}

bool ResourceAlternativesModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    if (mapToSource(idx).column() == ResourceAllocationModel::RequestAllocation) {
        if (role == Qt::CheckStateRole) {
            Resource *alternative = resource(idx);
            if (alternative) {
                ResourceAllocationItemModel *m = static_cast<ResourceAllocationItemModel*>(sourceModel());
                if (value.toInt() == Qt::Checked) {
                    ResourceRequest *rr = m->alternativeRequest(m_resource, alternative);
                    if (!rr) {
                        int max = mapToSource(idx).siblingAtColumn(ResourceAllocationModel::RequestAllocation).data(Role::Maximum).toInt();
                        rr = new ResourceRequest(alternative, max);
                    }
                    m->addAlternativeRequest(m_resource, rr);
                    Q_EMIT dataChanged(idx, idx);
                    return true;
                } else if (m->removeAlternativeRequest(m_resource, alternative)) {
                    Q_EMIT dataChanged(idx, idx);
                    return true;
                }
            }
            return false;
        } else if (role == Qt::EditRole) {
            Resource *alternative = resource(idx);
            if (alternative) {
                ResourceAllocationItemModel *m = static_cast<ResourceAllocationItemModel*>(sourceModel());
                ResourceRequest *rr = m->alternativeRequest(m_resource, alternative);
                if (rr) {
                    rr->setUnits(value.toInt());
                    Q_EMIT dataChanged(idx, idx);
                    return true;
                }
            }
            return false;
        }
    }
    return QSortFilterProxyModel::setData(idx, value, role);
}

//--------------------------------------

ResourceRequiredModel::ResourceRequiredModel(ResourceAllocationItemModel *dataModel, QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_resource(nullptr)
{
    Q_ASSERT(dataModel);
    setSourceModel(dataModel);
    connect(dataModel, &ResourceAllocationItemModel::dataChanged, this, &ResourceRequiredModel::slotDataChanged);
}

void ResourceRequiredModel::slotDataChanged()
{
    beginResetModel();
    endResetModel();
}

void ResourceRequiredModel::setResource(Resource *resource)
{
    beginResetModel();
    m_resource = resource;
    endResetModel();
}

Resource *ResourceRequiredModel::resource() const
{
    return m_resource;
}

Resource *ResourceRequiredModel::resource(const QModelIndex &idx) const
{
    QVariant v = QSortFilterProxyModel::data(idx, Role::Object);
    return v.value<Resource*>();
}

bool ResourceRequiredModel::filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const
{
    Q_UNUSED(source_parent)
    switch (source_column) {
        case ResourceAllocationModel::RequestName:
            return true;
        case ResourceAllocationModel::RequestAllocation:
        case ResourceAllocationModel::RequestMaximum:
        default:
            break;
    }
    return false;
}

bool ResourceRequiredModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if (!m_resource || source_parent.isValid()) {
        return false;
    }
    auto m = static_cast<ResourceAllocationItemModel*>(sourceModel());
    const QModelIndex idx = m->index(source_row, 0);
    auto r = m->resource(idx);
    return r && (r != m_resource) && (r->type() == Resource::Type_Material);
}

Qt::ItemFlags ResourceRequiredModel::flags(const QModelIndex &idx) const
{
    if (!m_resource) {
        return Qt::NoItemFlags;
    }
    const auto m = static_cast<ResourceAllocationItemModel*>(sourceModel());
    if (!m->resourceCache().contains(m_resource)) {
        return Qt::NoItemFlags;
    }
    if (idx.column() == 0) {
        return Qt::ItemIsUserCheckable | Qt::ItemIsEditable | QSortFilterProxyModel::flags(idx);
    }
    return QSortFilterProxyModel::flags(idx);
}

QVariant ResourceRequiredModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return QSortFilterProxyModel::headerData(section, orientation, role);
}

QVariant ResourceRequiredModel::data(const QModelIndex &idx, int role) const
{
    const auto aIdx = mapToSource(idx);
    switch (aIdx.column()) {
        case ResourceAllocationModel::RequestName: {
            if (role == Qt::CheckStateRole) {
                auto required = resource(idx);
                const auto m = static_cast<ResourceAllocationItemModel*>(sourceModel());
                auto request = m->resourceCache().value(m_resource);
                if (request && request->requiredResources().contains(required)) {
                    return Qt::Checked;
                }
                return Qt::Unchecked;
            }
            break;
        }
    }
    return QSortFilterProxyModel::data(idx, role);
}

bool ResourceRequiredModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    if (role == Qt::CheckStateRole) {
        if (mapToSource(idx).column() == ResourceAllocationModel::RequestName) {
            auto required = resource(idx);
            const auto m = static_cast<ResourceAllocationItemModel*>(sourceModel());
            auto request = m->resourceCache().value(m_resource);
            Q_ASSERT(request);
            if (!request) {
                return false;
            }
            if (value.toInt() == Qt::Checked) {
                if (!request->requiredResources().contains(required)) {
                    request->addRequiredResource(required);
                    Q_EMIT dataChanged(idx, idx);
                    return true;
                }
            } else if (value.toInt() == Qt::Unchecked) {
                if (request->requiredResources().contains(required)) {
                    request->removeRequiredResource(required);
                    Q_EMIT dataChanged(idx, idx);
                    return true;
                }
            }
        }
    }
    return false;
}

//--------------------------------------

ResourceAllocationModel::ResourceAllocationModel(QObject *parent)
    : QObject(parent),
    m_project(nullptr),
    m_task(nullptr)
{
}

ResourceAllocationModel::~ResourceAllocationModel()
{
}

const QMetaEnum ResourceAllocationModel::columnMap() const
{
    return metaObject()->enumerator(metaObject()->indexOfEnumerator("Properties"));
}

void ResourceAllocationModel::setProject(Project *project)
{
    m_project = project;
}

void ResourceAllocationModel::setTask(Task *task)
{
    m_task = task;
}

int ResourceAllocationModel::propertyCount() const
{
    return columnMap().keyCount();
}

QVariant ResourceAllocationModel::name(const Resource *res, int role) const
{
    //debugPlan<<res->name()<<","<<role;
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case Qt::ToolTipRole:
            return res->name();
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

QVariant ResourceAllocationModel::type(const Resource *res, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            return res->typeToString(true);
        case Qt::EditRole:
            return res->typeToString(false);
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

QVariant ResourceAllocationModel::allocation(const Resource *res, int role) const
{
    if (m_project == nullptr || m_task == nullptr) {
        return QVariant();
    }
    const ResourceRequest *rr = m_task->requests().find(res);
    switch (role) {
        case Qt::DisplayRole: {
            int units = rr ? rr->units() : 0;
            // xgettext: no-c-format
            return i18nc("<value>%", "%1%", units);
        }
        case Qt::EditRole:
            return rr ? rr->units() : 0;
        case Qt::ToolTipRole: {
            int units = rr ? rr->units() : 0;
            if (units == 0) {
                return xi18nc("@info:tooltip", "Not allocated");
            }
            // xgettext: no-c-format
            return xi18nc("@info:tooltip", "Allocated units: %1%", units);
        }
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Role::Minimum:
            return 0;
        case Role::Maximum:
            return 100;
        case Qt::CheckStateRole:
            return Qt::Unchecked;
        default:
            break;
    }
    return QVariant();
}

QVariant ResourceAllocationModel::maximum(const Resource *res, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            // xgettext: no-c-format
            return i18nc("<value>%", "%1%", res->units());
        case Qt::EditRole:
            return res->units();
        case Qt::ToolTipRole:
            // xgettext: no-c-format
            return i18n("Maximum units available: %1%", res->units());
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
    }
    return QVariant();
}

QVariant ResourceAllocationModel::required(const Resource *res, int role) const
{
    switch (role) {
        case Qt::DisplayRole: {
            QStringList lst;
            const QList<Resource*> resources = res->requiredResources();
            for (Resource *r : resources) {
                lst << r->name();
            }
            return lst.join(",");
        }
        case Qt::EditRole:
            return QVariant();//Not used
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::WhatsThisRole:
            return xi18nc("@info:whatsthis", "<title>Required Resources</title>"
            "<para>A working resource can be assigned to one or more required resources."
            " A required resource is a material resource that the working resource depends on"
            " in order to do the work.</para>");
    }
    return QVariant();
}

QVariant ResourceAllocationModel::alternative(const Resource *res, int role) const
{
    switch (role) {
        case Qt::DisplayRole: {
            QStringList lst;
            if (m_task) {
                ResourceRequest *rr = m_task->requests().find(res);
                if (rr) {
                    const auto alternativeRequests = rr->alternativeRequests();
                    for (ResourceRequest *r : alternativeRequests) {
                        lst << r->resource()->name();
                    }
                }
            }
            return lst.join(",");
        }
        case Qt::EditRole:
            return QVariant();//Not used
        case Qt::ToolTipRole:
            return QVariant();
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::StatusTipRole:
            return QVariant();
        case Qt::CheckStateRole:
            break;
        case Qt::WhatsThisRole:
            return xi18nc("@info:whatsthis", "<title>Alternative Resource Allocation</title>"
            "<para>A resource can have one or more alternative resource assigned to it."
            " The scheduling software will then select the most appropriate resource to use.</para>");
    }
    return QVariant();
}

QVariant ResourceAllocationModel::data(const Resource *resource, int property, int role) const
{
    QVariant result;
    if (resource == nullptr) {
        return result;
    }
    switch (property) {
        case RequestName: result = name(resource, role); break;
        case RequestType: result = type(resource, role); break;
        case RequestAllocation: result = allocation(resource, role); break;
        case RequestMaximum: result = maximum(resource, role); break;
        case RequestAlternative: result = alternative(resource, role); break;
        case RequestRequired: result = required(resource, role); break;
        default:
            debugPlan<<"data: invalid display value: property="<<property;
            break;
    }
    return result;
}

QVariant ResourceAllocationModel::headerData(int section, int role)
{
    if (role == Qt::DisplayRole) {
        switch (section) {
            case RequestName: return i18n("Name");
            case RequestType: return i18n("Type");
            case RequestAllocation: return i18n("Allocation");
            case RequestMaximum: return xi18nc("@title:column", "Available");
            case RequestAlternative: return xi18nc("@title:column", "Alternative Resources");
            case RequestRequired: return xi18nc("@title:column", "Required Resources");
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
            case RequestMaximum: return xi18nc("@info:tooltip", "Available resources or resource units");
            case RequestAlternative: return xi18nc("@info:tooltip", "List of resources that can be used as an alternative");
            case RequestRequired: return xi18nc("@info:tooltip", "Required material resources");
            default: return QVariant();
        }
    } else if (role == Qt::WhatsThisRole) {
        switch (section) {
            case RequestAlternative:
                return xi18nc("@info:whatsthis", "<title>Alternative Resource Allocation</title>"
                "<para>A resource can have one or more alternative resource assigned to it."
                " The scheduling software will then select the most appropriate resource to use.</para>");
            case RequestRequired:
                return xi18nc("@info:whatsthis", "<title>Required Resources</title>"
                "<para>A working resource can be assigned to one or more required resources."
                " A required resource is a material resource that the working resource depends on"
                " in order to do the work.</para>");
            default: return QVariant();
        }
    }
    return QVariant();
}
//--------------------------------------

ResourceAllocationItemModel::ResourceAllocationItemModel(QObject *parent)
    : ItemModelBase(parent)
{
}

ResourceAllocationItemModel::~ResourceAllocationItemModel()
{
}

void ResourceAllocationItemModel::slotResourceToBeAdded(KPlato::Project *project, int row)
{
    Q_UNUSED(project)
    beginInsertRows(QModelIndex(), row, row);
}

void ResourceAllocationItemModel::slotResourceAdded(KPlato::Resource *resource)
{
    Q_UNUSED(resource)
    endInsertRows();
}

void ResourceAllocationItemModel::slotResourceToBeRemoved(KPlato::Project *project, int row, KPlato::Resource *resource)
{
    Q_UNUSED(project)
    Q_UNUSED(resource)
    beginRemoveRows(QModelIndex(), row, row);
}

void ResourceAllocationItemModel::slotResourceRemoved()
{
    endRemoveRows();
}

void ResourceAllocationItemModel::setProject(Project *project)
{
    beginResetModel();
    if (m_project) {
        disconnect(m_project, &Project::aboutToBeDeleted, this, &ResourceAllocationItemModel::projectDeleted);

        disconnect(m_project, &Project::resourceChanged, this, &ResourceAllocationItemModel::slotResourceChanged);
        disconnect(m_project, &Project::resourceToBeAdded, this, &ResourceAllocationItemModel::slotResourceToBeAdded);
        disconnect(m_project, &Project::resourceAdded, this, &ResourceAllocationItemModel::slotResourceAdded);
        disconnect(m_project, &Project::resourceToBeRemoved, this, &ResourceAllocationItemModel::slotResourceToBeRemoved);
        disconnect(m_project, &Project::resourceRemoved, this, &ResourceAllocationItemModel::slotResourceRemoved);
    }
    m_project = project;
    if (m_project) {
        disconnect(m_project, &Project::aboutToBeDeleted, this, &ResourceAllocationItemModel::projectDeleted);

        disconnect(m_project, &Project::resourceChanged, this, &ResourceAllocationItemModel::slotResourceChanged);
        disconnect(m_project, &Project::resourceToBeAdded, this, &ResourceAllocationItemModel::slotResourceToBeAdded);
        disconnect(m_project, &Project::resourceAdded, this, &ResourceAllocationItemModel::slotResourceAdded);
        disconnect(m_project, &Project::resourceToBeRemoved, this, &ResourceAllocationItemModel::slotResourceToBeRemoved);
        disconnect(m_project, &Project::resourceRemoved, this, &ResourceAllocationItemModel::slotResourceRemoved);
    }
    m_model.setProject(m_project);
    endResetModel();
}

void ResourceAllocationItemModel::setTask(Task *task)
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
        Q_EMIT layoutAboutToBeChanged();
        filldata(task);
        m_model.setTask(task);
        Q_EMIT layoutChanged();
    }
}

void ResourceAllocationItemModel::filldata(Task *task)
{
    qDeleteAll(m_resourceCache);
    m_resourceCache.clear();
    m_requiredChecked.clear();
    if (m_project && task) {
        const auto resourceRequests = task->requests().resourceRequests();
        for (ResourceRequest *rr : resourceRequests) {
            const Resource *r = rr->resource();
            m_resourceCache[r] = new ResourceRequest(*rr);
            if (!m_resourceCache[r]->requiredResources().isEmpty()) {
                m_requiredChecked[r] = Qt::Checked;
            }
            if (!m_resourceCache[r]->alternativeRequests().isEmpty()) {
                m_alternativeChecked[r] = Qt::Checked;
            }
        }
    }
}

bool ResourceAllocationItemModel::hasMaterialResources() const
{
    if (!m_project) {
        return false;
    }
    const QList<Resource*> resources = m_project->resourceList();
    for (const Resource *r : resources) {
        if (r->type() == Resource::Type_Material) {
            return true;
        }
    }
    return false;
}

Qt::ItemFlags ResourceAllocationItemModel::flags(const QModelIndex &index) const
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
        case ResourceAllocationModel::RequestAllocation:
            flags |= (Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
            break;
        case ResourceAllocationModel::RequestAlternative: {
            Resource *r = resource(index);
            if (r && m_alternativeChecked.value(r) == Qt::Checked) {
                flags |= (Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
            } else {
                flags |= Qt::ItemIsUserCheckable;
                flags &= ~Qt::ItemIsEditable;
            }
            break;
        }
        case ResourceAllocationModel::RequestRequired: {
            Resource *r = resource(index);
            if (r && r->type() != Resource::Type_Work) {
                flags &= ~(Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
            } else if (m_resourceCache.contains(r) && m_resourceCache[r]->units() > 0) {
                flags |= (Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
                if (!hasMaterialResources()) {
                    flags &= ~Qt::ItemIsEnabled;
                }
            }
            break;
        }
        default:
            flags &= ~Qt::ItemIsEditable;
            break;
    }
    return flags;
}


QModelIndex ResourceAllocationItemModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return QModelIndex();
}

QModelIndex ResourceAllocationItemModel::index(int row, int column, const QModelIndex &parent) const
{
    if (m_project == nullptr || column < 0 || column >= columnCount() || row < 0) {
        return QModelIndex();
    }
    if (parent.isValid()) {
        return QModelIndex();
    }
    Resource *r = m_project->resourceAt(row);
    if (r) {
        return createIndex(row, column, r);
    }
    return QModelIndex();
}

QModelIndex ResourceAllocationItemModel::index(Resource *resource) const
{
    if (m_project == nullptr || resource == nullptr) {
        return QModelIndex();
    }
    int row = m_project->indexOf(resource);
    if (row >= 0) {
        return createIndex(row, 0, resource);
    }
    return QModelIndex();
}

int ResourceAllocationItemModel::columnCount(const QModelIndex &/*parent*/) const
{
    return 4;//m_model.propertyCount();
}

int ResourceAllocationItemModel::rowCount(const QModelIndex &parent) const
{
    if (m_project == nullptr || m_model.task() == nullptr) {
        return 0;
    }
    return parent.isValid() ? 0 : m_project->resourceCount();
}

QVariant ResourceAllocationItemModel::allocation(const Resource *res, int role) const
{
    if (m_model.task() == nullptr) {
        return QVariant();
    }
    if (!m_resourceCache.contains(res)) {
        if (role == Qt::EditRole) {
            ResourceRequest *req = m_model.task()->requests().find(res);
            if (req == nullptr) {
                req = new ResourceRequest(const_cast<Resource*>(res), 0);
            }
            const_cast<ResourceAllocationItemModel*>(this)->m_resourceCache.insert(res, req);
            return req->units();
        }
        return m_model.allocation(res, role);
    }
    switch (role) {
        case Qt::DisplayRole: {
            // xgettext: no-c-format
            return i18nc("<value>%", "%1%", m_resourceCache[res]->units());
        }
        case Qt::EditRole:
            return m_resourceCache[res]->units();
        case Qt::ToolTipRole: {
            if (res->units() == 0) {
                return xi18nc("@info:tooltip", "Not allocated");
            }
            return xi18nc("@info:tooltip", "%1 allocated out of %2 available", allocation(res, Qt::DisplayRole).toString(), m_model.maximum(res, Qt::DisplayRole).toString());
        }
        case Qt::CheckStateRole:
            return m_resourceCache[res]->units() == 0 ? Qt::Unchecked : Qt::Checked;
        default:
            return m_model.allocation(res, role);
    }
    return QVariant();
}

bool ResourceAllocationItemModel::setAllocation(Resource *res, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole: {
            m_resourceCache[res]->setUnits(value.toInt());
            QModelIndex idx = index(res);
            if (value.toInt() == 0) {
                setAlternative(idx.sibling(idx.row(), ResourceAllocationModel::RequestAlternative), Qt::Unchecked, Qt::CheckStateRole);
            }
            Q_EMIT dataChanged(idx, idx.sibling(idx.row(), ResourceAllocationModel::RequestAllocation));
            return true;
        }
        case Qt::CheckStateRole: {
            QModelIndex idx = index(res);
            if (!m_resourceCache.contains(res)) {
                m_resourceCache[res] = new ResourceRequest(res, 0);
            }
            if (m_resourceCache[res]->units() == 0) {
                m_resourceCache[res]->setUnits(100);
            } else {
                m_resourceCache[res]->setUnits(0);
                setAlternative(idx.sibling(idx.row(), ResourceAllocationModel::RequestAlternative), Qt::Unchecked, Qt::CheckStateRole);
            }
            Q_EMIT dataChanged(idx, idx);
            return true;
        }
    }
    return false;
}

void ResourceAllocationItemModel::addRequiredResource(Resource *resource, Resource *required)
{
    ResourceRequest *request = m_resourceCache[resource];
    if (!request->requiredResources().contains(required)) {
        request->addRequiredResource(required);
    }
}

bool ResourceAllocationItemModel::removeRequiredResource(Resource *resource, Resource *required)
{
    if (!m_resourceCache.contains(resource)) {
        return false;
    }
    ResourceRequest *request = m_resourceCache[resource];
    if (request->requiredResources().contains(required)) {
        request->removeRequiredResource(required);
        return true;
    }
    return false;
}

QVariant ResourceAllocationItemModel::required(const QModelIndex &idx, int role) const
{
    if (m_model.task() == nullptr) {
        return QVariant();
    }
    Resource *res = resource(idx);
    if (res == nullptr) {
        return QVariant();
    }
    switch (role) {
        case Qt::DisplayRole: {
            if (res->type() == Resource::Type_Work) {
                QStringList lst;
                if (m_requiredChecked[res]) {
                    const QList<Resource*> resources = required(idx);
                    for (const Resource *r : resources) {
                        lst << r->name();
                    }
                }
                return lst.isEmpty() ? i18n("None") : lst.join(",");
            }
            break;
        }
        case Qt::EditRole: break;
        case Qt::ToolTipRole:
            switch (res->type()) {
                case Resource::Type_Work: {
                    if (!hasMaterialResources()) {
                        return xi18nc("@info:tooltip", "No material resources available");
                    }
                    QStringList lst;
                    if (m_requiredChecked[res]) {
                        const QList<Resource*> resources = required(idx);
                        for (const Resource *r : resources) {
                            lst << r->name();
                        }
                    }
                    return lst.isEmpty() ? xi18nc("@info:tooltip", "No required resources") : lst.join("\n");
                }
                case Resource::Type_Material:
                    return xi18nc("@info:tooltip", "Material resources cannot have required resources");
                case Resource::Type_Team:
                    return xi18nc("@info:tooltip", "Team resources cannot have required resources");
            }
            break;
        case Qt::CheckStateRole:
            if (res->type() == Resource::Type_Work) {
                return m_requiredChecked[res];
            }
            break;
        default:
            return m_model.required(res, role);
    }
    return QVariant();
}

bool ResourceAllocationItemModel::setRequired(const QModelIndex &idx, const QVariant &value, int role)
{
    Resource *res = resource(idx);
    if (res == nullptr) {
        return false;
    }
    switch (role) {
        case Qt::CheckStateRole:
            m_requiredChecked[res] = value.toInt();
            if (value.toInt() == Qt::Unchecked) {
                m_resourceCache[res]->setRequiredResources(QList<Resource*>());
            }
            Q_EMIT dataChanged(idx, idx);
            return true;
    }
    return false;
}

QVariant ResourceAllocationItemModel::alternative(const QModelIndex &idx, int role) const
{
    if (m_model.task() == nullptr) {
        return QVariant();
    }
    Resource *res = resource(idx);
    if (res == nullptr) {
        return QVariant();
    }
    switch (role) {
        case Qt::DisplayRole: {
            QStringList lst;
            if (m_alternativeChecked[res] == Qt::Checked) {
                ResourceRequest *rr = m_resourceCache.value(res);
                if (rr) {
                        const auto alternativeRequests = rr->alternativeRequests();
                        for (ResourceRequest *r : alternativeRequests) {
                        lst << r->resource()->name();
                    }
                }
            }
            return lst.isEmpty() ? i18n("None") : lst.join(",");
        }
        case Qt::EditRole: break; // not used
        case Qt::CheckStateRole:
            return m_alternativeChecked[res];
        default:
            return m_model.alternative(res, role);
    }
    return QVariant();
}

bool ResourceAllocationItemModel::setAlternative(const QModelIndex &idx, const QVariant &value, int role)
{
    Resource *res = resource(idx);
    if (res == nullptr) {
        return false;
    }
    switch (role) {
        case Qt::CheckStateRole:
            m_alternativeChecked[res] = value.toInt();
            if (value.toInt() == Qt::Unchecked) {
                ResourceRequest *rr = m_resourceCache.value(res);
                if (rr) {
                    rr->setAlternativeRequests(QList<ResourceRequest*>());
                }
            }
            Q_EMIT dataChanged(idx, idx);
            return true;
    }
    return false;
}

void ResourceAllocationItemModel::addAlternativeRequest(Resource *resource, ResourceRequest *alternative)
{
    removeAlternativeRequest(resource, alternative->resource());
    ResourceRequest *request = m_resourceCache[resource];
    request->addAlternativeRequest(alternative);
}

bool ResourceAllocationItemModel::removeAlternativeRequest(Resource *resource, Resource *alternative)
{
    if (!m_resourceCache.contains(resource)) {
        return false;
    }
    ResourceRequest *request = m_resourceCache[resource];
    ResourceRequest *alt = nullptr;
    const auto alternativeRequests = request->alternativeRequests();
    for (ResourceRequest *rr : alternativeRequests) {
        if (rr->resource() == alternative) {
            alt = rr;
            break;
        }
    }
    if (alt) {
        request->removeAlternativeRequest(alt);
        return true;
    }
    return false;
}

ResourceRequest *ResourceAllocationItemModel::alternativeRequest(Resource *resource, Resource *alternative) const
{
    if (!m_resourceCache.contains(resource)) {
        return nullptr;
    }
    ResourceRequest *request = m_resourceCache[resource];
    const auto alternativeRequests = request->alternativeRequests();
    for (ResourceRequest *rr : alternativeRequests) {
        if (rr->resource() == alternative) {
            return rr;
        }
    }
    return nullptr;
}

QVariant ResourceAllocationItemModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::TextAlignmentRole) {
        // use same alignment as in header (headers always horizontal)
        return headerData(index.column(), Qt::Horizontal, role);
    }
    QVariant result;
    Resource *r = resource(index);
    if (role == Role::Object) {
        return QVariant::fromValue(r);
    }
    if (r) {
        if (index.column() == ResourceAllocationModel::RequestAllocation) {
            return allocation(r, role);
        }
        if (index.column() == ResourceAllocationModel::RequestRequired) {
            return required(index, role);
        }
        if (index.column() == ResourceAllocationModel::RequestAlternative) {
            return alternative(index, role);
        }
        result = m_model.data(r, index.column(), role);
    }
    return result;
}

bool ResourceAllocationItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid()) {
        return ItemModelBase::setData(index, value, role);
    }
    Resource *r = resource(index);
    if (r) {
        switch (index.column()) {
            case ResourceAllocationModel::RequestAllocation:
                if (setAllocation(r, value, role)) {
                    Q_EMIT dataChanged(index, index);
                    QModelIndex idx = this->index(index.row(), ResourceAllocationModel::RequestAllocation, parent(parent(index)));
                    Q_EMIT dataChanged(idx, idx);
                    return true;
                }
                return false;
            case ResourceAllocationModel::RequestAlternative:
                return setAlternative(index, value, role);
            case ResourceAllocationModel::RequestRequired:
                return setRequired(index, value, role);
            default:
                //qWarning("data: invalid display value column %d", index.column());
                return false;
        }
    }
    return false;
}

QVariant ResourceAllocationItemModel::headerData(int section, Qt::Orientation orientation, int role) const
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

QAbstractItemDelegate *ResourceAllocationItemModel::createDelegate(int col, QWidget *parent) const
{
    QAbstractItemDelegate *del = nullptr;
    switch (col) {
        case ResourceAllocationModel::RequestAllocation: del = new SpinBoxDelegate(parent); break;
        case ResourceAllocationModel::RequestAlternative: del = new AlternativeResourceDelegate(parent); break;
        case ResourceAllocationModel::RequestRequired: del = new RequieredResourceDelegate(parent); break;
        default: break;
    }
    return del;
}

QObject *ResourceAllocationItemModel::object(const QModelIndex &index) const
{
    QObject *o = nullptr;
    if (index.isValid()) {
        o = static_cast<QObject*>(index.internalPointer());
        Q_ASSERT(o);
    }
    return o;
}

void ResourceAllocationItemModel::slotResourceChanged(Resource *res)
{
    QModelIndex idx = index(res);
    Q_EMIT dataChanged(idx, idx.sibling(idx.row(), columnCount()-1));
}

Resource *ResourceAllocationItemModel::resource(const QModelIndex &idx) const
{
    return qobject_cast<Resource*>(object(idx));
}

void ResourceAllocationItemModel::setRequired(const QModelIndex &idx, const QList<Resource*> &lst)
{
    Resource *r = resource(idx);
    Q_ASSERT(r);
    if (m_resourceCache.contains(r)) {
        m_resourceCache[r]->setRequiredResources(lst);
        Q_EMIT dataChanged(idx, idx);
    }
}

QList<Resource*> ResourceAllocationItemModel::required(const QModelIndex &idx) const
{
    Resource *r = resource(idx);
    Q_ASSERT(r);
    if (m_resourceCache.contains(r)) {
        ResourceRequest* request = m_resourceCache[r];
        return request->requiredResources();
    }
    return r->requiredResources();
}

void ResourceAllocationItemModel::setAlternativeRequests(const QModelIndex &idx, const QList<Resource*> &lst)
{
    Resource *r = resource(idx);
    Q_ASSERT(r);
    if (m_resourceCache.contains(r)) {
        QList<ResourceRequest*> requests;
        for (Resource *r : lst) {
            requests << new ResourceRequest(r, 100);
        }
        m_resourceCache[r]->setAlternativeRequests(requests);
        Q_EMIT dataChanged(idx, idx);
    }
}

QList<ResourceRequest*> ResourceAllocationItemModel::alternativeRequests(const QModelIndex &idx) const
{
    QList<ResourceRequest*> alternatives;
    ResourceRequest *request = m_resourceCache.value(resource(idx));
    if (request) {
        alternatives = request->alternativeRequests();
    }
    return alternatives;
}
