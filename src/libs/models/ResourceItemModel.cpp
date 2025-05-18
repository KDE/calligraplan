/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2007 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2011, 2012 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "ResourceItemModel.h"

#include "kptlocale.h"
#include "kptcommonstrings.h"
#include <AddResourceCmd.h>
#include "kptcommand.h"
#include "kptitemmodelbase.h"
#include "kptcalendar.h"
#include "kptduration.h"
#include "kptnode.h"
#include "kptproject.h"
#include "kpttask.h"
#include <ResourceGroup.h>
#include <Resource.h>
#include "kptdatetime.h"
#include "kptdebug.h"

#include <KoIcon.h>

#include <QMimeData>
#include <QMimeDatabase>
#include <QStringList>
#include <QLocale>

#include <KIO/TransferJob>
#include <KIO/StatJob>

#ifdef PLAN_KCONTACTS_FOUND
#include <KContacts/Addressee>
#include <KContacts/VCardConverter>
#endif


using namespace KPlato;


ResourceItemModel::ResourceItemModel(QObject *parent)
    : ItemModelBase(parent)
    , m_groupsEnabled(false)
    , m_teamsEnabled(false)
    , m_requiredEnabled(false)
    , m_isCheckable(true)
{
}

ResourceItemModel::~ResourceItemModel()
{
}

void ResourceItemModel::setIsCheckable(bool enable)
{
    beginResetModel();
    m_isCheckable = enable;
    endResetModel();
}

bool ResourceItemModel::isCheckable() const
{
    return m_isCheckable;
}

void ResourceItemModel::slotResourceToBeAdded(Project *project, int row)
{
    Q_UNUSED(project)
    debugPlan<<sender()<<row;
    beginInsertRows(QModelIndex(), row, row);
}

void ResourceItemModel::slotResourceAdded(Resource* resource)
{
    connectSignals(resource, true);
    endInsertRows();
}

void ResourceItemModel::slotResourceToBeRemoved(Project *project, int row, Resource* resource)
{
    Q_UNUSED(project)
    debugPlan<<sender()<<row<<resource;
    beginRemoveRows(QModelIndex(), row, row);
    connectSignals(resource, false);
}

void ResourceItemModel::slotResourceRemoved()
{
    endRemoveRows();
}

void ResourceItemModel::slotResourceGroupToBeAdded(Resource* resource, int row)
{
    debugPlan<<sender()<<resource<<row;
    QModelIndex idx = createIndex(m_project->indexOf(resource), 0);
    beginInsertRows(idx, row, row);
}

void ResourceItemModel::slotResourceGroupAdded(ResourceGroup *group)
{
    connectSignals(group, true);
    endInsertRows();
}

void ResourceItemModel::slotResourceGroupToBeRemoved(Resource *resource, int row, KPlato::ResourceGroup *group)
{
    //debugPlan<<group->name();
    QModelIndex idx = createIndex(m_project->indexOf(resource), 0);
    beginRemoveRows(idx, row, row);
    connectSignals(group, false);
}

void ResourceItemModel::slotResourceGroupRemoved()
{
    //debugPlan<<group->name();
    endRemoveRows();
}

void ResourceItemModel::slotResourceTeamToBeAdded(Resource* resource, int row)
{
    debugPlan<<sender()<<resource<<row;
    QModelIndex idx = createIndex(m_project->indexOf(resource), 0);
    int r = row;
    if (m_groupsEnabled) {
        r += resource->groupCount();
    }
    beginInsertRows(idx, r, r);
}

void ResourceItemModel::slotResourceTeamAdded(Resource* resource)
{
    Q_UNUSED(resource)
    endInsertRows();
}

void ResourceItemModel::slotResourceTeamToBeRemoved(KPlato::Resource *resource, int row, KPlato::Resource *team)
{
    Q_UNUSED(team)
    //debugPlan<<group->name();
    QModelIndex idx = createIndex(m_project->indexOf(resource), 0);
    int r = row;
    if (m_groupsEnabled) {
        r += resource->groupCount();
    }
    beginRemoveRows(idx, r, r);
}

void ResourceItemModel::slotResourceTeamRemoved()
{
    //debugPlan<<group->name();
    endRemoveRows();
}

void ResourceItemModel::setProject(Project *project)
{
    beginResetModel();
    if (m_project) {
        disconnect(m_project, &Project::aboutToBeDeleted, this, &ResourceItemModel::projectDeleted);
        disconnect(m_project, &Project::localeChanged, this, &ResourceItemModel::slotLayoutChanged);
        disconnect(m_project, &Project::defaultCalendarChanged, this, &ResourceItemModel::slotCalendarChanged);

        disconnect(m_project, &Project::resourceChanged, this, &ResourceItemModel::slotResourceChanged);
        disconnect(m_project, &Project::resourceToBeAdded, this, &ResourceItemModel::slotResourceToBeAdded);
        disconnect(m_project, &Project::resourceAdded, this, &ResourceItemModel::slotResourceAdded);
        disconnect(m_project, &Project::resourceToBeRemoved, this, &ResourceItemModel::slotResourceToBeRemoved);
        disconnect(m_project, &Project::resourceRemoved, this, &ResourceItemModel::slotResourceRemoved);

        const auto resourceList =  m_project->resourceList();
        for (Resource *r : resourceList) {
            connectSignals(r, false);
        }
    }
    m_project = project;
    if (m_project) {
        connect(m_project, &Project::aboutToBeDeleted, this, &ResourceItemModel::projectDeleted);
        connect(m_project, &Project::localeChanged, this, &ResourceItemModel::slotLayoutChanged);
        connect(m_project, &Project::defaultCalendarChanged, this, &ResourceItemModel::slotCalendarChanged);

        connect(m_project, &Project::resourceChanged, this, &ResourceItemModel::slotResourceChanged);
        connect(m_project, &Project::resourceToBeAdded, this, &ResourceItemModel::slotResourceToBeAdded);
        connect(m_project, &Project::resourceAdded, this, &ResourceItemModel::slotResourceAdded);
        connect(m_project, &Project::resourceToBeRemoved, this, &ResourceItemModel::slotResourceToBeRemoved);
        connect(m_project, &Project::resourceRemoved, this, &ResourceItemModel::slotResourceRemoved);

        const auto resourceList = m_project->resourceList();
        for (Resource *r : resourceList) {
            connectSignals(r, true);
        }
    }
    m_resourceModel.setProject(m_project);
    endResetModel();
}

void ResourceItemModel::connectSignals(Resource *resource, bool enable)
{
    if (enable) {
        connect(resource, &Resource::resourceGroupToBeAdded, this, &ResourceItemModel::slotResourceGroupToBeAdded);
        connect(resource, &Resource::resourceGroupAdded, this, &ResourceItemModel::slotResourceGroupAdded);
        connect(resource, &Resource::resourceGroupToBeRemoved, this, &ResourceItemModel::slotResourceGroupToBeRemoved);
        connect(resource, &Resource::resourceGroupRemoved, this, &ResourceItemModel::slotResourceGroupRemoved);

        connect(resource, &Resource::teamToBeAdded, this, &ResourceItemModel::slotResourceTeamToBeAdded);
        connect(resource, &Resource::teamAdded, this, &ResourceItemModel::slotResourceTeamAdded);
        connect(resource, &Resource::teamToBeRemoved, this, &ResourceItemModel::slotResourceTeamToBeRemoved);
        connect(resource, &Resource::teamRemoved, this, &ResourceItemModel::slotResourceTeamRemoved);
    } else {
        disconnect(resource, &Resource::resourceGroupToBeAdded, this, &ResourceItemModel::slotResourceGroupToBeAdded);
        disconnect(resource, &Resource::resourceGroupAdded, this, &ResourceItemModel::slotResourceGroupAdded);
        disconnect(resource, &Resource::resourceGroupToBeRemoved, this, &ResourceItemModel::slotResourceGroupToBeRemoved);
        disconnect(resource, &Resource::resourceGroupRemoved, this, &ResourceItemModel::slotResourceGroupRemoved);

        disconnect(resource, &Resource::teamToBeAdded, this, &ResourceItemModel::slotResourceTeamToBeAdded);
        disconnect(resource, &Resource::teamAdded, this, &ResourceItemModel::slotResourceTeamAdded);
        disconnect(resource, &Resource::teamToBeRemoved, this, &ResourceItemModel::slotResourceTeamToBeRemoved);
        disconnect(resource, &Resource::teamRemoved, this, &ResourceItemModel::slotResourceTeamRemoved);
    }
    const auto parentGroups = resource->parentGroups();
    for (ResourceGroup *g : parentGroups) {
        connectSignals(g, enable);
    }
}

void ResourceItemModel::connectSignals(ResourceGroup *group, bool enable)
{
    if (enable) {
        connect(group, &ResourceGroup::dataChanged, this, &ResourceItemModel::slotResourceGroupChanged, Qt::ConnectionType(Qt::AutoConnection|Qt::UniqueConnection));
    } else {
        disconnect(group, &ResourceGroup::dataChanged, this, &ResourceItemModel::slotResourceGroupChanged);
    }
}

Qt::ItemFlags ResourceItemModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = ItemModelBase::flags(index);
    if (!m_readWrite) {
        //debugPlan<<"read only"<<flags;
        if (index.isValid()) {
            flags |= Qt::ItemIsDragEnabled;
        }
        return flags &= ~Qt::ItemIsEditable;
    }
    if (!index.isValid()) {
        //debugPlan<<"invalid"<<flags;
        return flags;
    }
    Resource *r = resource(index);
    if (r != nullptr) {
        flags |= Qt::ItemIsDragEnabled;
        if (r->isShared()) {
            flags &= ~Qt::ItemIsEditable;
            if (index.column() == ResourceModel::ResourceName) {
                if (m_isCheckable) {
                    flags |= Qt::ItemIsUserCheckable;
                }
            } else if (index.column() == ResourceModel::ResourceAccount) {
                if (!r->isBaselined()) {
                    flags |= Qt::ItemIsEditable;
                }
            }
            return flags;
        }
        switch (index.column()) {
            case ResourceModel::ResourceName:
                flags |= Qt::ItemIsEditable;
                if (m_isCheckable) {
                    flags |= Qt::ItemIsUserCheckable;
                }
                break;
            case ResourceModel::ResourceOrigin:
                flags &= ~Qt::ItemIsEditable;
                break;
            case ResourceModel::ResourceType:
                if (! r->isBaselined()) {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            case ResourceModel::ResourceAccount:
                if (! r->isBaselined()) {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            case ResourceModel::ResourceNormalRate:
                if (! r->isBaselined()) {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            case ResourceModel::ResourceOvertimeRate:
                if (! r->isBaselined()) {
                    flags |= Qt::ItemIsEditable;
                }
                break;
            default:
                flags |= Qt::ItemIsEditable;
        }
        //debugPlan<<"resource"<<flags;
    } else {
        ResourceGroup *g = group(index);
        if (g) {
            flags |= Qt::ItemIsDragEnabled;
            if (g->isShared()) {
                flags &= ~Qt::ItemIsEditable;
                return flags;
            }
            flags |= Qt::ItemIsDropEnabled;
            switch (index.column()) {
                case ResourceModel::ResourceName: flags |= Qt::ItemIsEditable; break;
                case ResourceModel::ResourceType: flags |= Qt::ItemIsEditable; break;
                default: flags &= ~Qt::ItemIsEditable;
            }
            //debugPlan<<"group"<<flags;
        }
    }
    return flags;
}


QModelIndex ResourceItemModel::parent(const QModelIndex &index) const
{
    if (!index.isValid() || m_project == nullptr) {
        return QModelIndex();
    }
    //debugPlan<<index.internalPointer()<<":"<<index.row()<<","<<index.column();
    if (index.internalPointer() == nullptr) {
        // top level resource, no parent
        return QModelIndex();
    }
    Resource *r = static_cast<Resource*>(index.internalPointer());
    Q_ASSERT(r);
    int row = m_project->indexOf(r);
    return createIndex(row, index.column());
}

QModelIndex ResourceItemModel::index(int row, int column, const QModelIndex &parent) const
{
    if (m_project == nullptr || column < 0 || column >= columnCount() || row < 0) {
        return QModelIndex();
    }
    if (!parent.isValid()) {
        // top level resource
        if (row < m_project->resourceCount()) {
            return createIndex(row, column);
        }
        return QModelIndex();
    }
    Q_ASSERT(parent.internalPointer() == nullptr);
    if (parent.internalPointer()) {
        return QModelIndex();
    }
    Resource *r = resource(parent);
    Q_ASSERT(r);
    return createIndex(row, column, r);
}

int ResourceItemModel::columnCount(const QModelIndex &/*parent*/) const
{
    return m_resourceModel.propertyCount();
}

int ResourceItemModel::rowCount(const QModelIndex &parent) const
{
    int rows = 0;
    if (m_project == nullptr) {
        return rows;
    }
    if (parent.isValid()) {
        if (parent.internalPointer() == nullptr) {
            Resource *r = m_project->resourceAt(parent.row());
            if (r) {
                if (m_groupsEnabled) {
                    rows = r->groupCount();
                }
                if (m_teamsEnabled) {
                    rows += r->teamCount();
                }
                if (m_requiredEnabled) {
                    rows += r->requiredResources().count();
                }
            }
        } else {
#if 0
            // FIXME: Model needs further modifications to handle this
            if (m_requiredEnabled) {
                auto resource = static_cast<Resource*>(parent.internalPointer());
                rows = resource->requiredResources().count();
            }
#endif
        }
    } else {
        rows = m_project->resourceCount();
    }
    return rows;
}

QVariant ResourceItemModel::name(const  ResourceGroup *res, int role) const
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

bool ResourceItemModel::setName(Resource *res, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole:
            if (value.toString() == res->name()) {
                return false;
            }
            Q_EMIT executeCommand(new ModifyResourceNameCmd(res, value.toString(), kundo2_i18n("Modify resource name")));
            return true;
        case Qt::CheckStateRole:
            Q_EMIT executeCommand(new ModifyResourceAutoAllocateCmd(res, value.toBool(), kundo2_i18n("Modify resource auto allocate")));
            return true;
    }
    return false;
}

bool ResourceItemModel::setType(Resource *res, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole: {
            Resource::Type v;
            QStringList lst = res->typeToStringList(false);
            if (lst.contains(value.toString())) {
                v = static_cast<Resource::Type>(lst.indexOf(value.toString()));
            } else {
                v = static_cast<Resource::Type>(value.toInt());
            }
            if (v == res->type()) {
                return false;
            }
            Q_EMIT executeCommand(new ModifyResourceTypeCmd(res, v, kundo2_i18n("Modify resource type")));
            return true;
        }
    }
    return false;
}

bool ResourceItemModel::setInitials(Resource *res, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole:
            if (value.toString() == res->initials()) {
                return false;
            }
            Q_EMIT executeCommand(new ModifyResourceInitialsCmd(res, value.toString(), kundo2_i18n("Modify resource initials")));
            return true;
    }
    return false;
}

bool ResourceItemModel::setEmail(Resource *res, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole:
            if (value.toString() == res->email()) {
                return false;
            }
            Q_EMIT executeCommand(new ModifyResourceEmailCmd(res, value.toString(), kundo2_i18n("Modify resource email")));
            return true;
    }
    return false;
}

bool ResourceItemModel::setCalendar(Resource *res, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole:
        {
            Calendar *c = nullptr;
            if (value.toInt() > 0) {
                QStringList lst = m_resourceModel.calendar(res, Role::EnumList).toStringList();
                if (value.toInt() < lst.count()) {
                    c = m_project->calendarByName(lst.at(value.toInt()));
                }
            }
            if (c == res->calendar(true)) {
                return false;
            }
            Q_EMIT executeCommand(new ModifyResourceCalendarCmd(res, c, kundo2_i18n("Modify resource calendar")));
            return true;
        }
    }
    return false;
}


bool ResourceItemModel::setUnits(Resource *res, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole:
            if (value.toInt() == res->units()) {
                return false;
            }
            Q_EMIT executeCommand(new ModifyResourceUnitsCmd(res, value.toInt(), kundo2_i18n("Modify resource available units")));
            return true;
    }
    return false;
}

bool ResourceItemModel::setAvailableFrom(Resource *res, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole:
            if (value.toDateTime() == res->availableFrom()) {
                return false;
            }
            Q_EMIT executeCommand(new ModifyResourceAvailableFromCmd(res, value.toDateTime(), kundo2_i18n("Modify resource available from")));
            return true;
    }
    return false;
}

bool ResourceItemModel::setAvailableUntil(Resource *res, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole:
            if (value.toDateTime() == res->availableUntil()) {
                return false;
            }
            Q_EMIT executeCommand(new ModifyResourceAvailableUntilCmd(res, value.toDateTime(), kundo2_i18n("Modify resource available until")));
            return true;
    }
    return false;
}

bool ResourceItemModel::setNormalRate(Resource *res, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole:
            if (value.toDouble() == res->normalRate()) {
                return false;
            }
            Q_EMIT executeCommand(new ModifyResourceNormalRateCmd(res, value.toDouble(), kundo2_i18n("Modify resource normal rate")));
            return true;
    }
    return false;
}

bool ResourceItemModel::setOvertimeRate(Resource *res, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole:
            if (value.toDouble() == res->overtimeRate()) {
                return false;
            }
            Q_EMIT executeCommand(new ModifyResourceOvertimeRateCmd(res, value.toDouble(), kundo2_i18n("Modify resource overtime rate")));
            return true;
    }
    return false;
}

bool ResourceItemModel::setAccount(Resource *res, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole: {
            Account *a = nullptr;
            if (value.typeId() == QMetaType::Int) {
                QStringList lst = m_resourceModel.account(res, Role::EnumList).toStringList();
                if (value.toInt() >= lst.count()) {
                    return false;
                }
                a = m_project->accounts().findAccount(lst.at(value.toInt()));
            } else if (value.typeId() == QMetaType::QString) {
                a = m_project->accounts().findAccount(value.toString());
            }
            Account *old = res->account();
            if (old != a) {
                Q_EMIT executeCommand(new ResourceModifyAccountCmd(*res, old, a, kundo2_i18n("Modify resource account")));
                return true;
            }
        }
        default:
            break;
    }
    return false;
}

QVariant ResourceItemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }
    if (!m_isCheckable && role == Qt::CheckStateRole) {
        return QVariant();
    }
    if (index.internalPointer() == nullptr) {
        // top level resource
        Resource *r = m_project->resourceAt(index.row());
        return m_resourceModel.data(r, index.column(), role);
    }
    if (m_groupsEnabled) {
        Resource *r = static_cast<Resource*>(index.internalPointer());
        int groupCount = r->groupCount();
        if (index.row() < groupCount) {
            ResourceGroup *g = r->parentGroups().value(index.row());
            Q_ASSERT(g);
            return m_groupModel.data(g, index.column(), role);
        }
    }
    if (m_teamsEnabled) {
        Resource *r = static_cast<Resource*>(index.internalPointer());
        if (r->type() == Resource::Type_Team) {
            int row = index.row();
            if (m_groupsEnabled) {
                row -= r->groupCount();
            }
            return m_resourceModel.data(r->teamMembers().value(row), index.column(), role);
        }
    }
    if (m_requiredEnabled) {
        Resource *r = static_cast<Resource*>(index.internalPointer());
        if (r->type() == Resource::Type_Work) {
            int row = index.row();
            if (m_groupsEnabled) {
                row -= r->groupCount();
            }
            return m_resourceModel.data(r->requiredResources().value(row), index.column(), role);
        }
    }
    return QVariant();
}

bool ResourceItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (ItemModelBase::setData(index, value, role)) {
        return true;
    }
    if (! index.isValid()) {
        return false;
    }
    if (role != Qt::EditRole && role != Qt::CheckStateRole) {
        return false;
    }
    if ((flags(index) & (Qt::ItemIsEditable | Qt::ItemIsUserCheckable)) == 0) {
        return false;
    }
    Resource *r =resource(index);
    if (r) {
        switch (index.column()) {
            case ResourceModel::ResourceName: return setName(r, value, role);
            case ResourceModel::ResourceOrigin: return false; // Not editable
            case ResourceModel::ResourceType: return setType(r, value, role);
            case ResourceModel::ResourceInitials: return setInitials(r, value, role);
            case ResourceModel::ResourceEmail: return setEmail(r, value, role);
            case ResourceModel::ResourceCalendar: return setCalendar(r, value, role);
            case ResourceModel::ResourceLimit: return setUnits(r, value, role);
            case ResourceModel::ResourceAvailableFrom: return setAvailableFrom(r, value, role);
            case ResourceModel::ResourceAvailableUntil: return setAvailableUntil(r, value, role);
            case ResourceModel::ResourceNormalRate: return setNormalRate(r, value, role);
            case ResourceModel::ResourceOvertimeRate: return setOvertimeRate(r, value, role);
            case ResourceModel::ResourceAccount: return setAccount(r, value, role);
            default:
                qWarning("data: invalid display value column %d", index.column());
                return false;
        }
    }
    return false;
}

QVariant ResourceItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if (role == Qt::DisplayRole || role == Qt::TextAlignmentRole) {
            return m_resourceModel.headerData(section, role);
        }
    }
    if (role == Qt::ToolTipRole) {
        return m_resourceModel.headerData(section, role);
    }
    return ItemModelBase::headerData(section, orientation, role);
}

QAbstractItemDelegate *ResourceItemModel::createDelegate(int col, QWidget *parent) const
{
    switch (col) {
        case ResourceModel::ResourceType: return new EnumDelegate(parent);
        case ResourceModel::ResourceCalendar: return new EnumDelegate(parent);
        case ResourceModel::ResourceAvailableFrom: return new DateTimeCalendarDelegate(parent);
        case ResourceModel::ResourceAvailableUntil: return new DateTimeCalendarDelegate(parent);
        case ResourceModel::ResourceAccount: return new EnumDelegate(parent);
        default: break;
    }
    return nullptr;
}

ResourceGroup *ResourceItemModel::group(const QModelIndex &index) const
{
    if (index.internalPointer() == nullptr) {
        return nullptr;
    }
    Resource *r = static_cast<Resource*>(index.internalPointer());
    return r->parentGroups().value(index.row());
}

QModelIndex ResourceItemModel::index(Resource *resource) const
{
    QModelIndex idx;
    int row = m_project->indexOf(resource);
    if (row >= 0) {
        idx = index(row, 0);
    }
    return idx;
}

Resource *ResourceItemModel::resource(const QModelIndex &index) const
{
    if (index.internalPointer() == nullptr) {
        return m_project->resourceAt(index.row());
    }
    if (m_teamsEnabled) {
        Resource *r = static_cast<Resource*>(index.internalPointer());
        int row = index.row();
        if (m_groupsEnabled) {
            row -= r->groupCount();
        }
        return r->teamMembers().value(row);
    }
    return nullptr;
}

void ResourceItemModel::slotCalendarChanged(Calendar*)
{
    const QList<Resource*> resources = m_project->resourceList();
    for (Resource *r : resources) {
        if (r->calendar(true) == nullptr) {
            slotResourceChanged(r);
        }
    }
}

void ResourceItemModel::slotResourceChanged(Resource *resource)
{
    int row = m_project->indexOf(resource);
    Q_EMIT dataChanged(createIndex(row, 0), createIndex(row, columnCount() - 1));
    if (m_teamsEnabled) {
        int offset = 0;
        if (m_groupsEnabled) {
            offset = resource->groupCount();
        }
        const auto resourceList = m_project->resourceList();
        for (Resource *r : resourceList) {
            if (r->type() == Resource::Type_Team && r->teamMembers().contains(resource)) {
                int row = r->teamMembers().indexOf(resource) + offset;
                Q_EMIT dataChanged(createIndex(row, 0, resource), createIndex(row, columnCount() - 1, resource));
            }
        }
    }
}

void ResourceItemModel::slotResourceGroupChanged(ResourceGroup *group)
{
    const auto resourceList = group->resources();
    for (Resource *r : resourceList) {
        int row = r->groupIndexOf(group);
        Q_EMIT dataChanged(createIndex(row, 0, r), createIndex(row, columnCount() - 1, r));
    }
}

bool ResourceItemModel::groupsEnabled() const
{
    return m_groupsEnabled;
}

void ResourceItemModel::setGroupsEnabled(bool enable)
{
    beginResetModel();
    m_groupsEnabled = enable;
    endResetModel();
}

bool ResourceItemModel::teamsEnabled() const
{
    return m_teamsEnabled;
}

void ResourceItemModel::setTeamsEnabled(bool enable)
{
    beginResetModel();
    m_teamsEnabled = enable;
    endResetModel();
}

bool ResourceItemModel::requiredEnabled() const
{
    return m_requiredEnabled;
}

void ResourceItemModel::setRequiredEnabled(bool enable)
{
    beginResetModel();
    m_requiredEnabled = enable;
    endResetModel();
}

Qt::DropActions ResourceItemModel::supportedDropActions() const
{
    return Qt::MoveAction | Qt::CopyAction;
}

bool ResourceItemModel::dropAllowed(const QModelIndex &index, int dropIndicatorPosition, const QMimeData *data)
{
    if (data->hasFormat(QStringLiteral("application/x-vnd.kde.plan.resourceitemmodel.internal"))) {
        QByteArray encodedData = data->data(QStringLiteral("application/x-vnd.kde.plan.resourceitemmodel.internal"));
        QDataStream stream(&encodedData, QIODevice::ReadOnly);

        const QList<Resource*> resources = resourceList(stream);
        for (const Resource *r : resources) {
            if (r->isShared()) {
                return false;
            }
        }
    }

    //debugPlan<<index<<data;
    // TODO: if internal, don't allow dropping on my own parent
    switch (dropIndicatorPosition) {
        case ItemModelBase::OnItem:
            return group(index); // Allow only on group
        default:
            break;
    }
    return false;
}

QStringList ResourceItemModel::mimeTypes() const
{
    return ItemModelBase::mimeTypes()
#ifdef PLAN_KDEPIMLIBS_FOUND
            <<QStringLiteral("text/x-vcard")
            <<QStringLiteral("text/directory")
            <<QStringLiteral("text/uri-list")
#endif
           <<QStringLiteral("text/html")
           <<QStringLiteral("text/plain")
           <<QStringLiteral("application/x-vnd.kde.plan.resourceitemmodel.internal");
}

void ResourceItemModel::slotDataArrived(KIO::Job *job, const QByteArray &data)
{
    if (m_dropDataMap.contains(job)) {
        m_dropDataMap[ job ].data += data;
    }
}

void ResourceItemModel::slotJobFinished(KJob *job)
{
    Q_UNUSED(job);
//     if (job->error() || ! m_dropDataMap.contains(job)) {
//         debugPlan<<(job->error() ? "Job error":"Error: no such job");
//     } else if (QMimeDatabase().mimeTypeForData(m_dropDataMap[ job ].data).inherits(QStringLiteral("text/x-vcard"))) {
//         ResourceGroup *g = 0;
//         if (m_dropDataMap[ job ].parent.isValid()) {
//             g = qobject_cast<ResourceGroup*>(object(m_dropDataMap[ job ].parent));
//         } else {
//             g = qobject_cast<ResourceGroup*>(object(index(m_dropDataMap[ job ].row, m_dropDataMap[ job ].column, m_dropDataMap[ job ].parent)));
//         }
//         if (g == 0) {
//             debugPlan<<"No group"<<m_dropDataMap[ job ].row<<m_dropDataMap[ job ].column<<m_dropDataMap[ job ].parent;
//         } else {
//             createResources(g, m_dropDataMap[ job ].data);
//         }
//     }
//     if (m_dropDataMap.contains(job)) {
//         m_dropDataMap.remove(job);
//     }
//     disconnect(job, &KJob::result, this, &ResourceItemModel::slotJobFinished);
}

bool ResourceItemModel::createResources(ResourceGroup *group, const QByteArray &data)
{
#ifdef PLAN_KCONTACTS_FOUND
    KContacts::VCardConverter vc;
    KContacts::Addressee::List lst = vc.parseVCards(data);
    MacroCommand *m = new MacroCommand(kundo2_i18np("Add resource from address book", "Add %1 resources from address book", lst.count()));
    // Note: windows needs this type of iteration
    for (int a = 0; a < lst.count(); ++a) {
        Resource *r = new Resource();
        QString uid = lst[a].uid();
        if (! m_project->findResource(uid)) {
            r->setId(uid);
        }
        r->setName(lst[a].formattedName());
        r->setEmail(lst[a].preferredEmail());
        m->addCommand(new AddResourceCmd(group, r));
    }
    if (m->isEmpty()) {
        delete m;
        return false;
    }
    Q_EMIT executeCommand(m);
    return true;
#else
    Q_UNUSED(group);
    Q_UNUSED(data);
    return false;
#endif
}

bool ResourceItemModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(data);
    Q_UNUSED(action);
    Q_UNUSED(row);
    Q_UNUSED(column);
    Q_UNUSED(parent);
#if 0
    debugPlan<<row<<column<<parent;
    if (action == Qt::IgnoreAction) {
        return true;
    }
    if (column > 0) {
        return false;
    }
    ResourceGroup *g = 0;
    if (parent.isValid()) {
        g = qobject_cast<ResourceGroup*>(object(parent));
    } else {
        g = qobject_cast<ResourceGroup*>(object(index(row, column, parent)));
    }
    if (g == 0) {
        debugPlan<<"No group"<<row<<column<<parent;
        return false;
    }
    debugPlan<<data->formats()<<g->name();
    if (data->hasFormat("application/x-vnd.kde.plan.resourceitemmodel.internal")) {
        debugPlan<<action<<Qt::MoveAction;
        if (action == Qt::MoveAction) {
            MacroCommand *m = 0;
            QByteArray encodedData = data->data("application/x-vnd.kde.plan.resourceitemmodel.internal");
            QDataStream stream(&encodedData, QIODevice::ReadOnly);
            int i = 0;
            const QList<Resource*> resources = resourceList(stream);
            for (const Resource *r : resources) {
                if (r->parentGroups().value(0) == g) {
                    continue;
                }
                if (m == 0) m = new MacroCommand(KUndo2MagicString());
                m->addCommand(new MoveResourceCmd(g, r));
                ++i;
            }
            if (m) {
                KUndo2MagicString msg = kundo2_i18np("Move resource", "Move %1 resources", i);
                MacroCommand *c = new MacroCommand(msg);
                c->addCommand(m);
                Q_EMIT executeCommand(c);
            }
            return true;
        }
        if (action == Qt::CopyAction) {
            MacroCommand *m = 0;
            QByteArray encodedData = data->data("application/x-vnd.kde.plan.resourceitemmodel.internal");
            QDataStream stream(&encodedData, QIODevice::ReadOnly);
            int i = 0;
            const QList<Resource*> resources = resourceList(stream);
            for (const Resource *r : resources) {
                Resource *nr = new Resource(r);
                if (m == 0) m = new MacroCommand(KUndo2MagicString());
                m->addCommand(new AddResourceCmd(g, nr));
                ++i;
            }
            if (m) {
                KUndo2MagicString msg = kundo2_i18np("Copy resource", "Copy %1 resources", i);
                MacroCommand *c = new MacroCommand(msg);
                c->addCommand(m);
                Q_EMIT executeCommand(c);
            }
            return true;
        }
        return true;
    }
    if (data->hasFormat("text/x-vcard") || data->hasFormat("text/directory")) {
        if (action != Qt::CopyAction) {
            return false;
        }
        QString f = data->hasFormat("text/x-vcard") ? "text/x-vcard" : "text/directory";
        return createResources(g, data->data(f));
    }
    if (data->hasFormat("text/uri-list")) {
        const QList<QUrl> urls = data->urls();
        if (urls.isEmpty()) {
            return false;
        }
        bool result = false;
        for (const QUrl &url : urls) {
            if (url.scheme() != "akonadi") {
                debugPlan<<url<<"is not 'akonadi'";
                continue;
            }
            // TODO: KIO::get will find out about this as well, no?
            KIO::StatJob* statJob = KIO::stat(url);
            statJob->setSide(KIO::StatJob::SourceSide);

            const bool isUrlReadable = statJob->exec();
            if (! isUrlReadable) {
                debugPlan<<url<<"does not exist";
                continue;
            }

            KIO::TransferJob *job = KIO::get(url, KIO::NoReload, KIO::HideProgressInfo);
            bool res = connect(job, &KIO::TransferJob::data, this, &ResourceItemModel::slotDataArrived);
            Q_ASSERT(res);
	    Q_UNUSED(res);
            res = connect(job, &KJob::result, this, &ResourceItemModel::slotJobFinished);
            Q_ASSERT(res);

            m_dropDataMap[ job ].action = action;
            m_dropDataMap[ job ].row = row;
            m_dropDataMap[ job ].column = column;
            m_dropDataMap[ job ].parent = parent;

            job->start();
            result = true;
        }
        return result;
    }
#endif
    return false;
}

QList<Resource*> ResourceItemModel::resourceList(QDataStream &stream)
{
    QList<Resource*> lst;
    while (!stream.atEnd()) {
        QString id;
        stream >> id;
        Resource *r = m_project->findResource(id);
        if (r) {
            lst << r;
        }
    }
    debugPlan<<lst;
    return lst;
}

QMimeData *ResourceItemModel::mimeData(const QModelIndexList & indexes) const
{
    QMimeData *m = ItemModelBase::mimeData(indexes);
    QByteArray encodedData;
    QDataStream stream(&encodedData, QIODevice::WriteOnly);
    QList<int> rows;
    for (const QModelIndex &index : indexes) {
        if (index.isValid() && !rows.contains(index.row())) {
            //debugPlan<<index.row();
            Resource *r = resource(index);
            if (r) {
                rows << index.row();
                stream << r->id();
            }
        }
    }
    if (!rows.isEmpty()) {
        m->setData(QStringLiteral("application/x-vnd.kde.plan.resourceitemmodel.internal"), encodedData);
    }
    return m;
}

QModelIndex ResourceItemModel::insertResource(Resource *resource, Resource * /*after*/)
{
    //debugPlan;
    Q_EMIT executeCommand(new AddResourceCmd(m_project, resource, kundo2_i18n("Add resource")));
    int row = m_project->indexOf(resource);
    if (row != -1) {
        return createIndex(row, ResourceModel::ResourceName);
    }
    return QModelIndex();
}

int ResourceItemModel::sortRole(int column) const
{
    switch (column) {
        case ResourceModel::ResourceAvailableFrom:
        case ResourceModel::ResourceAvailableUntil:
            return Qt::EditRole;
        default:
            break;
    }
    return Qt::DisplayRole;
}
