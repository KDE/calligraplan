/* This file is part of the KDE project
 * Copyright (C) 2007 Dag Andersen <dag.andersen@kdemail.net>
 * Copyright (C) 2011, 2012 Dag Andersen <dag.andersen@kdemail.net>
 * Copyright (C) 2016 Dag Andersen <dag.andersen@kdemail.net>
 * Copyright (C) 2019 Dag Andersen <dag.andersen@kdemail.net>
 * Copyright (C) 2020 Dag Andersen <dag.andersen@kdemail.net>
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
#include "ResourceGroupItemModel.h"

#include <AddResourceCmd.h>
#include "AddParentGroupCmd.h"
#include "RemoveParentGroupCmd.h"
#include <ResourceGroupModifyCoordinatorCmd.h>
#include "kptlocale.h"
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

ResourceGroupItemModel::ResourceGroupItemModel(QObject *parent)
    : ItemModelBase(parent)
    , m_resourcesEnabled(false)
{
}

ResourceGroupItemModel::~ResourceGroupItemModel()
{
}

void ResourceGroupItemModel::setResourcesEnabled(bool enable)
{
    beginResetModel();
    m_resourcesEnabled = enable;
    endResetModel();
}

bool ResourceGroupItemModel::resourcesEnabled() const
{
    return m_resourcesEnabled;
}

void ResourceGroupItemModel::slotResourceToBeAdded(ResourceGroup *group, int row)
{
    debugPlan<<group<<","<<row;
    if (m_resourcesEnabled) {
        beginInsertRows(index(group), row, row);
    }
}

void ResourceGroupItemModel::slotResourceAdded(KPlato::Resource *resource)
{
    connectSignals(resource, true);
    if (m_resourcesEnabled) {
        endInsertRows();
    }
}

void ResourceGroupItemModel::slotResourceToBeRemoved(ResourceGroup *group, int row, KPlato::Resource *resource)
{
    if (m_resourcesEnabled) {
        beginRemoveRows(index(group), row, row);
    }
    connectSignals(resource, false);
}

void ResourceGroupItemModel::slotResourceRemoved()
{
    if (m_resourcesEnabled) {
        endRemoveRows();
    }
}

void ResourceGroupItemModel::slotResourceGroupToBeAdded(Project *project, ResourceGroup *parent, int row)
{
    Q_UNUSED(project)
    debugPlan<<row;
    beginInsertRows(index(parent), row, row);
}

void ResourceGroupItemModel::slotResourceGroupAdded(ResourceGroup *group)
{
    Q_UNUSED(group)
    connectSignals(group, true);
    endInsertRows();
}

void ResourceGroupItemModel::slotResourceGroupToBeRemoved(Project *project, ResourceGroup *parent, int row, ResourceGroup *group)
{
    Q_UNUSED(project)
    beginRemoveRows(index(parent), row, row);
    connectSignals(group, false);
}

void ResourceGroupItemModel::slotResourceGroupRemoved()
{
    endRemoveRows();
}

void ResourceGroupItemModel::setProject(Project *project)
{
    debugPlan<<project;
    beginResetModel();
    if (m_project) {
        disconnect(m_project, &Project::aboutToBeDeleted, this, &ResourceGroupItemModel::projectDeleted);
        disconnect(m_project, &Project::localeChanged, this, &ResourceGroupItemModel::slotLayoutChanged);

        disconnect(m_project, &Project::resourceGroupChanged, this, &ResourceGroupItemModel::slotResourceGroupChanged);
        disconnect(m_project, &Project::resourceGroupToBeAdded, this, &ResourceGroupItemModel::slotResourceGroupToBeAdded);
        disconnect(m_project, &Project::resourceGroupAdded, this, &ResourceGroupItemModel::slotResourceGroupAdded);
        disconnect(m_project, &Project::resourceGroupToBeRemoved, this, &ResourceGroupItemModel::slotResourceGroupToBeRemoved);
        disconnect(m_project, &Project::resourceGroupRemoved, this, &ResourceGroupItemModel::slotResourceGroupRemoved);

        for (ResourceGroup *g : m_project->resourceGroups()) {
            connectSignals(g, false);
        }
    }
    m_project = project;
    if (m_project) {
        connect(m_project, &Project::aboutToBeDeleted, this, &ResourceGroupItemModel::projectDeleted);
        connect(m_project, &Project::localeChanged, this, &ResourceGroupItemModel::slotLayoutChanged);

        connect(m_project, &Project::resourceGroupChanged, this, &ResourceGroupItemModel::slotResourceGroupChanged, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
        connect(m_project, &Project::resourceGroupToBeAdded, this, &ResourceGroupItemModel::slotResourceGroupToBeAdded, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
        connect(m_project, &Project::resourceGroupAdded, this, &ResourceGroupItemModel::slotResourceGroupAdded, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
        connect(m_project, &Project::resourceGroupToBeRemoved, this, &ResourceGroupItemModel::slotResourceGroupToBeRemoved, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
        connect(m_project, &Project::resourceGroupRemoved, this, &ResourceGroupItemModel::slotResourceGroupRemoved, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));

        for (ResourceGroup *g : m_project->resourceGroups()) {
            connectSignals(g, true);
        }
    }
    m_groupModel.setProject(m_project);
    m_resourceModel.setProject(m_project);
    endResetModel();
}

void ResourceGroupItemModel::connectSignals(ResourceGroup *group, bool enable)
{
    if (enable) {
        connect(group, &ResourceGroup::dataChanged, this, &ResourceGroupItemModel::slotResourceGroupChanged, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
        connect(group, &ResourceGroup::groupToBeAdded, this, &ResourceGroupItemModel::slotResourceGroupToBeAdded, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
        connect(group, &ResourceGroup::groupAdded, this, &ResourceGroupItemModel::slotResourceGroupAdded, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
        connect(group, &ResourceGroup::groupToBeRemoved, this, &ResourceGroupItemModel::slotResourceGroupToBeRemoved, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
        connect(group, &ResourceGroup::groupRemoved, this, &ResourceGroupItemModel::slotResourceGroupRemoved, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));

        connect(group, &ResourceGroup::resourceToBeAdded, this, &ResourceGroupItemModel::slotResourceToBeAdded, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
        connect(group, &ResourceGroup::resourceAdded, this, &ResourceGroupItemModel::slotResourceAdded, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
        connect(group, &ResourceGroup::resourceToBeRemoved, this, &ResourceGroupItemModel::slotResourceToBeRemoved, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
        connect(group, &ResourceGroup::resourceRemoved, this, &ResourceGroupItemModel::slotResourceRemoved, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
    } else {
        disconnect(group, &ResourceGroup::dataChanged, this, &ResourceGroupItemModel::slotResourceGroupChanged);
        disconnect(group, &ResourceGroup::groupToBeAdded, this, &ResourceGroupItemModel::slotResourceGroupToBeAdded);
        disconnect(group, &ResourceGroup::groupAdded, this, &ResourceGroupItemModel::slotResourceGroupAdded);
        disconnect(group, &ResourceGroup::groupToBeRemoved, this, &ResourceGroupItemModel::slotResourceGroupToBeRemoved);
        disconnect(group, &ResourceGroup::groupRemoved, this, &ResourceGroupItemModel::slotResourceGroupRemoved);

        disconnect(group, &ResourceGroup::resourceToBeAdded, this, &ResourceGroupItemModel::slotResourceToBeAdded);
        disconnect(group, &ResourceGroup::resourceAdded, this, &ResourceGroupItemModel::slotResourceAdded);
        disconnect(group, &ResourceGroup::resourceToBeRemoved, this, &ResourceGroupItemModel::slotResourceToBeRemoved);
        disconnect(group, &ResourceGroup::resourceRemoved, this, &ResourceGroupItemModel::slotResourceRemoved);
    }
    for (ResourceGroup *g : group->childGroups()) {
        connectSignals(g, enable);
    }
    for (Resource *r : group->resources()) {
        connectSignals(r, enable);
    }
}

void ResourceGroupItemModel::connectSignals(Resource *resource, bool enable)
{
    if (m_resourcesEnabled) {
        if (enable) {
            connect(resource, &Resource::dataChanged, this, &ResourceGroupItemModel::slotResourceChanged, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
        } else {
            disconnect(resource, &Resource::dataChanged, this, &ResourceGroupItemModel::slotResourceChanged);
        }
    }
}

Qt::ItemFlags ResourceGroupItemModel::flags(const QModelIndex &index) const
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
    ResourceGroup *g = group(index);
    if (g) {
        flags |= Qt::ItemIsDragEnabled;
        if (g->isShared()) {
            flags &= ~Qt::ItemIsEditable;
            return flags;
        }
        flags |= Qt::ItemIsDropEnabled;
        switch (index.column()) {
            case ResourceGroupModel::Name: flags |= Qt::ItemIsEditable; break;
            case ResourceGroupModel::Type: flags |= Qt::ItemIsEditable; break;
            case ResourceGroupModel::Coordinator: flags |= Qt::ItemIsEditable; break;
            default: flags &= ~Qt::ItemIsEditable;
        }
    } else if (m_resourcesEnabled) {
        Resource *r = resource(index);
        if (r) {
            flags |= Qt::ItemIsDragEnabled;
            flags &= ~Qt::ItemIsEditable;
        }
    }
    return flags;
}


QModelIndex ResourceGroupItemModel::parent(const QModelIndex &index) const
{
    if (!index.isValid() || m_project == nullptr) {
        return QModelIndex();
    }
    //debugPlan<<index.internalPointer()<<":"<<index.row()<<","<<index.column();
    if (index.internalPointer() == nullptr) {
        return QModelIndex();
    }
    ResourceGroup *p = static_cast<ResourceGroup*>(index.internalPointer());
    if (!p) {
        return QModelIndex();
    }
    ResourceGroup *gp = p ? p->parentGroup() : nullptr;
    int row = gp ? gp->indexOf(p) : m_project->indexOf(p);
    return createIndex(row, 0, gp);
}

QModelIndex ResourceGroupItemModel::index(int row, int column, const QModelIndex &parent) const
{
    if (m_project == nullptr || column < 0 || column >= columnCount() || row < 0) {
        return QModelIndex();
    }
    if (!parent.isValid()) {
        if (row < m_project->numResourceGroups()) {
            return createIndex(row, column);
        }
        return QModelIndex();
    }
    ResourceGroup *g = group(parent);
    if (g) {
        if (row < g->numChildGroups()) {
            return createIndex(row, column, g);
        } else if (m_resourcesEnabled) {
            return createIndex(row, column, g);
        }
    }
    return QModelIndex();
}

QModelIndex ResourceGroupItemModel::index(const ResourceGroup *group, int column) const
{
    if (m_project == nullptr || group == nullptr) {
        return QModelIndex();
    }
    int row = -1;
    if (group->parentGroup()) {
        row = group->parentGroup()->indexOf(const_cast<ResourceGroup*>(group));
    } else {
        row = m_project->indexOf(const_cast<ResourceGroup*>(group));
    }
    if (row < 0) {
        return QModelIndex();
    }
    return createIndex(row, column, group->parentGroup());
}

int ResourceGroupItemModel::columnCount(const QModelIndex &/*parent*/) const
{
    return m_groupModel.propertyCount();
}

int ResourceGroupItemModel::rowCount(const QModelIndex &parent) const
{
    if (m_project == nullptr) {
        return 0;
    }
    if (!parent.isValid()) {
        return m_project->numResourceGroups();
    }
    int rows = 0;
    ResourceGroup *g = group(parent);
    if (g) {
        rows = g->numChildGroups();
        if (m_resourcesEnabled) {
            rows += g->numResources();
        }
    } // else a resource so no children
    return rows;
}

QVariant ResourceGroupItemModel::name(const  ResourceGroup *res, int role) const
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

bool ResourceGroupItemModel::setName(ResourceGroup *res, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole:
            if (value.toString() == res->name()) {
                return false;
            }
            Q_EMIT executeCommand(new ModifyResourceGroupNameCmd(res, value.toString(), kundo2_i18n("Modify resourcegroup name")));
            return true;
    }
    return false;
}

QVariant ResourceGroupItemModel::type(const ResourceGroup *res, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
        case Qt::EditRole:
            return res->type();
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
    }
    return QVariant();
}

bool ResourceGroupItemModel::setType(ResourceGroup *res, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole: {
            Q_EMIT executeCommand(new ModifyResourceGroupTypeCmd(res, value.toString(), kundo2_i18n("Modify resourcegroup type")));
            return true;
        }
    }
    return false;
}

bool ResourceGroupItemModel::setUnits(ResourceGroup *res, const QVariant &value, int role)
{
    return false;
}

bool ResourceGroupItemModel::setCoordinator(ResourceGroup *res, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole: {
            Q_EMIT executeCommand(new ResourceGroupModifyCoordinatorCmd(res, value.toString(), kundo2_i18n("Modify resourcegroup coordinator")));
            return true;
        }
    }
    return false;
}

QVariant ResourceGroupItemModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::TextAlignmentRole) {
        // use same alignment as in header (headers always horizontal)
        return headerData(index.column(), Qt::Horizontal, role);
    }
    ResourceGroup *g = group(index);
    if (g) {
        return m_groupModel.data(g, index.column(), role);
    }
    if (m_resourcesEnabled) {
        Resource *r = resource(index);
        if (r) {
            switch (index.column()) {
                case ResourceGroupModel::Name: return m_resourceModel.data(r, ResourceModel::ResourceName, role);
                case ResourceGroupModel::Origin: return m_resourceModel.data(r, ResourceModel::ResourceOrigin, role);
                case ResourceGroupModel::Type: return m_resourceModel.data(r, ResourceModel::ResourceType, role);
                case ResourceGroupModel::Units: return m_resourceModel.data(r, ResourceModel::ResourceLimit, role);
                case ResourceGroupModel::Coordinator: return QVariant();
                default:
                    break;
            }
        }
    }
    return QVariant();
}

bool ResourceGroupItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (ItemModelBase::setData(index, value, role)) {
        return true;
    }
    if (!index.isValid()) {
        return false;
    }
    if (role != Qt::EditRole && role != Qt::CheckStateRole) {
        return false;
    }
    if ((flags(index) & (Qt::ItemIsEditable | Qt::ItemIsUserCheckable)) == 0) {
        return false;
    }
    ResourceGroup *g = group(index);
    if (g) {
        bool result = false;
        switch (index.column()) {
            case ResourceGroupModel::Name:  result = setName(g, value, role); break;
            case ResourceGroupModel::Origin: return false; // Not editable
            case ResourceGroupModel::Type: result = setType(g, value, role); break;
            case ResourceGroupModel::Units: result = setUnits(g, value, role); break;
            case ResourceGroupModel::Coordinator: result = setCoordinator(g, value, role); break;
            default:
                qWarning("data: invalid display value column %d", index.column());
                return false;
        }
        if (result) {
            Q_EMIT dataChanged(index, index);
        }
    }
    return false;
}

QVariant ResourceGroupItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if (role == Qt::DisplayRole || role == Qt::TextAlignmentRole) {
            return m_groupModel.headerData(section, role);
        }
    }
    if (role == Qt::ToolTipRole) {
        return m_groupModel.headerData(section, role);
    }
    return ItemModelBase::headerData(section, orientation, role);
}

QAbstractItemDelegate *ResourceGroupItemModel::createDelegate(int col, QWidget *parent) const
{
    Q_UNUSED(col)
    Q_UNUSED(parent)
    return nullptr;
}

ResourceGroup *ResourceGroupItemModel::group(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return nullptr;
    }
    ResourceGroup *parent = static_cast<ResourceGroup*>(index.internalPointer());
    if (parent) {
        return parent->childGroupAt(index.row());
    }
    return m_project->resourceGroupAt(index.row());
}

Resource *ResourceGroupItemModel::resource(const QModelIndex &index) const
{
    if (!m_resourcesEnabled || !index.isValid()) {
        return nullptr;
    }
    ResourceGroup *parent = static_cast<ResourceGroup*>(index.internalPointer());
    if (!parent) {
        return nullptr;
    }
    int row = index.row() - parent->numChildGroups();
    return parent->resourceAt(row);
}

void ResourceGroupItemModel::slotResourceChanged(Resource *res)
{
    for (ResourceGroup *g : res->parentGroups()) {
        int row = g->indexOf(res) + g->numChildGroups();
        Q_EMIT dataChanged(createIndex(row, 0, g), createIndex(row, columnCount() - 1, g));
    }
}

void ResourceGroupItemModel::slotResourceGroupChanged(ResourceGroup *group)
{
    ResourceGroup *parent = group->parentGroup();
    QModelIndex idx = index(group);
    Q_EMIT dataChanged(idx, idx.sibling(idx.row(), columnCount() - 1));
}

Qt::DropActions ResourceGroupItemModel::supportedDropActions() const
{
    return Qt::MoveAction | Qt::CopyAction;
}

bool ResourceGroupItemModel::dropAllowed(const QModelIndex &index, int dropIndicatorPosition, const QMimeData *data)
{
    if (data->hasFormat("application/x-vnd.kde.plan.resourcegroupitemmodel.internal")) {
        QByteArray encodedData = data->data("application/x-vnd.kde.plan.resourcegroupitemmodel.internal");
        QDataStream stream(&encodedData, QIODevice::ReadOnly);
        int i = 0;
        const QList<Resource*> resources = resourceList(stream);
        for (Resource *r : resources) {
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

QStringList ResourceGroupItemModel::mimeTypes() const
{
    return ItemModelBase::mimeTypes()
#ifdef PLAN_KDEPIMLIBS_FOUND
            << "text/x-vcard"
            << "text/directory"
            << "text/uri-list"
#endif
            << "application/x-vnd.kde.plan.resourcegroupitemmodel.internal";
}

void ResourceGroupItemModel::slotDataArrived(KIO::Job *job, const QByteArray &data)
{
    if (m_dropDataMap.contains(job)) {
        m_dropDataMap[ job ].data += data;
    }
}

void ResourceGroupItemModel::slotJobFinished(KJob *job)
{
    if (job->error() || ! m_dropDataMap.contains(job)) {
        debugPlan<<(job->error() ? "Job error":"Error: no such job");
    } else if (QMimeDatabase().mimeTypeForData(m_dropDataMap[ job ].data).inherits(QStringLiteral("text/x-vcard"))) {
        ResourceGroup *g = nullptr;
        if (m_dropDataMap[ job ].parent.isValid()) {
            g = group(m_dropDataMap[ job ].parent);
        } else {
            g = group(index(m_dropDataMap[ job ].row, m_dropDataMap[ job ].column, m_dropDataMap[ job ].parent));
        }
        if (g == nullptr) {
            debugPlan<<"No group"<<m_dropDataMap[ job ].row<<m_dropDataMap[ job ].column<<m_dropDataMap[ job ].parent;
        } else {
            createResources(g, m_dropDataMap[ job ].data);
        }
    }
    if (m_dropDataMap.contains(job)) {
        m_dropDataMap.remove(job);
    }
    disconnect(job, &KJob::result, this, &ResourceGroupItemModel::slotJobFinished);
}

bool ResourceGroupItemModel::createResources(ResourceGroup *group, const QByteArray &data)
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

bool ResourceGroupItemModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    debugPlan<<row<<column<<parent;
    if (action == Qt::IgnoreAction) {
        return true;
    }
    if (column > 0) {
        return false;
    }
    ResourceGroup *g = nullptr;
    if (parent.isValid()) {
        g = group(parent);
    } else {
        g = group(index(row, column, parent));
    }
    if (g == nullptr) {
        debugPlan<<"No group"<<row<<column<<parent;
        return false;
    }
    debugPlan<<data->formats()<<g->name();
    if (data->hasFormat("application/x-vnd.kde.plan.resourcegroupitemmodel.internal")) {
        debugPlan<<action<<Qt::MoveAction;
        if (action == Qt::MoveAction) {
            MacroCommand *m = nullptr;
            QByteArray encodedData = data->data("application/x-vnd.kde.plan.resourcegroupitemmodel.internal");
            QDataStream stream(&encodedData, QIODevice::ReadOnly);
            int i = 0;
            const QList<Resource*> resources = resourceList(stream);
            for (Resource *r : resources) {
                if (r->parentGroups().value(0) == g) {
                    continue;
                }
                if (m == nullptr) m = new MacroCommand(KUndo2MagicString());
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
            MacroCommand *m = nullptr;
            QByteArray encodedData = data->data("application/x-vnd.kde.plan.resourcegroupitemmodel.internal");
            QDataStream stream(&encodedData, QIODevice::ReadOnly);
            int i = 0;
            const QList<Resource*> resources = resourceList(stream);
            for (Resource *r : resources) {
                Resource *nr = new Resource(r);
                if (m == nullptr) m = new MacroCommand(KUndo2MagicString());
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
            bool res = connect(job, &KIO::TransferJob::data, this, &ResourceGroupItemModel::slotDataArrived);
            Q_ASSERT(res);
	    Q_UNUSED(res);
            res = connect(job, &KJob::result, this, &ResourceGroupItemModel::slotJobFinished);
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
    return false;
}

QList<Resource*> ResourceGroupItemModel::resourceList(QDataStream &stream)
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

QMimeData *ResourceGroupItemModel::mimeData(const QModelIndexList & indexes) const
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
            } else if (group(index)) {
                rows.clear();
                break;
            }
        }
    }
    if (!rows.isEmpty()) {
        m->setData("application/x-vnd.kde.plan.resourcegroupitemmodel.internal", encodedData);
    }
    return m;
}

QModelIndex ResourceGroupItemModel::insertGroup(ResourceGroup *group, ResourceGroup *parent)
{
    //debugPlan;
    if (parent) {
        Q_EMIT executeCommand(new AddResourceGroupCmd(m_project, parent, group, kundo2_i18n("Add resource group")));
        QModelIndex idx = index(group);
        return idx;
    }
    Q_EMIT executeCommand(new AddResourceGroupCmd(m_project, group, kundo2_i18n("Add resource group")));
    return index(group);
}

QModelIndex ResourceGroupItemModel::insertResource(ResourceGroup *parent, Resource *r, Resource * /*after*/)
{
    //debugPlan;
    Q_EMIT executeCommand(new AddParentGroupCmd(r, parent, kundo2_i18n("Add resource")));
    int row = parent->indexOf(r) + parent->numChildGroups();
    if (row != -1) {
        return createIndex(row, 0, parent);
    }
    return QModelIndex();
}

int ResourceGroupItemModel::sortRole(int column) const
{
    return Qt::DisplayRole;
}

//----------
ParentGroupItemModel::ParentGroupItemModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_model(new ResourceGroupItemModel(this))
    , m_resource(nullptr)
    , m_groupIsCheckable(false)
{
    setSourceModel(m_model);
    m_model->setReadWrite(true);
    m_model->setResourcesEnabled(true);
}

ParentGroupItemModel::~ParentGroupItemModel()
{
}

int ParentGroupItemModel::columnCount(const QModelIndex &idx) const
{
    Q_UNUSED(idx);
    return 1;
}

Qt::ItemFlags ParentGroupItemModel::flags(const QModelIndex &idx) const
{
    QModelIndex index = mapToSource(idx);
    Qt::ItemFlags f = m_model->flags(index);
    if (m_resource && m_groupIsCheckable && m_model->group(index)) {
        f |= Qt::ItemIsUserCheckable;
    }
    return f;
}

QVariant ParentGroupItemModel::data(const QModelIndex &idx, int role) const
{
    if (m_resource && m_groupIsCheckable && role == Qt::CheckStateRole) {
        ResourceGroup *group = m_model->group(mapToSource(idx));
        if (group) {
            return m_resource->groupIndexOf(group) >= 0 ? Qt::Checked : Qt::Unchecked;
        }
    }
    return QSortFilterProxyModel::data(idx, role);
}

bool ParentGroupItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::CheckStateRole && m_resource) {
        ResourceGroup *g = m_model->group(mapToSource(index));
        if (g) {
            if (value.toInt() == Qt::Checked) {
                Q_EMIT executeCommand(new AddParentGroupCmd(m_resource, g, kundo2_i18n("Add parent group")));
            } else {
                Q_EMIT executeCommand(new RemoveParentGroupCmd(m_resource, g, kundo2_i18n("Remove parent group")));
            }
            return true;
        }
        return false;
    }
    return QSortFilterProxyModel::setData(index, value, role);
}


void ParentGroupItemModel::setProject(Project *project)
{
    m_model->setProject(project);
}

void ParentGroupItemModel::setResource(Resource *resource)
{
    beginResetModel();
    if (m_resource) {
        disconnect(resource, &Resource::resourceGroupAdded, this, &ParentGroupItemModel::slotResourceAdded);
        disconnect(resource, &Resource::resourceGroupRemoved, this, &ParentGroupItemModel::slotResourceRemoved);
    }
    m_resource = resource;
    if (m_resource) {
        connect(resource, &Resource::resourceGroupAdded, this, &ParentGroupItemModel::slotResourceAdded);
        connect(resource, &Resource::resourceGroupRemoved, this, &ParentGroupItemModel::slotResourceRemoved);
        for (ResourceGroup *g : m_resource->parentGroups()) {
            m_model->setData(m_model->index(g), Qt::Checked, Qt::CheckStateRole);
        }
    }
    endResetModel();
}
    
void ParentGroupItemModel::setGroupIsCheckable(bool checkable)
{
    m_groupIsCheckable = checkable;
}

bool ParentGroupItemModel::groupIsCheckable() const
{
    return m_groupIsCheckable;
}

void ParentGroupItemModel::slotResourceAdded(KPlato::ResourceGroup *group)
{
    QModelIndex idx = mapFromSource(m_model->index(group));
    Q_EMIT dataChanged(idx, idx);
}

void ParentGroupItemModel::slotResourceRemoved()
{
    beginResetModel();
    endResetModel();
}

void ParentGroupItemModel::setResourcesEnabled(bool enable)
{
    m_model->setResourcesEnabled(enable);
}

bool ParentGroupItemModel::resourcesEnabled() const
{
    return m_model->resourcesEnabled();
}
