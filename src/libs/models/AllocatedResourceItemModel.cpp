/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2007 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2011, 2012 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "AllocatedResourceItemModel.h"

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

AllocatedResourceItemModel::AllocatedResourceItemModel(QObject *parent)
    : QSortFilterProxyModel(parent),
    m_task(nullptr)
{
    setDynamicSortFilter(true);
    setSourceModel(new ResourceItemModel(this));
}

int AllocatedResourceItemModel::columnCount(const QModelIndex &idx) const
{
    Q_UNUSED(idx);
    return 2;
}

Project *AllocatedResourceItemModel::project() const
{
    return static_cast<ResourceItemModel*>(sourceModel())->project();
}

void AllocatedResourceItemModel::setProject(Project *project)
{
    debugPlan<<this->project()<<"="<<project;
    Project *p =this->project();
    if (p) {
        disconnect(p, &Project::nodeChanged, this, &AllocatedResourceItemModel::slotNodeChanged);
    }
    static_cast<ResourceItemModel*>(sourceModel())->setProject(project);
    if (project) {
        connect(project, &Project::nodeChanged, this, &AllocatedResourceItemModel::slotNodeChanged);
    }
    debugPlan<<rowCount()<<":"<<sourceModel()->rowCount();
}

void AllocatedResourceItemModel::reset()
{
    beginResetModel();
    endResetModel();
    Q_EMIT expandAll();
    Q_EMIT resizeColumnToContents(0);
}

void AllocatedResourceItemModel::slotNodeChanged(Node *n)
{
    debugPlan<<(n==m_task)<<n<<n->name();
    if (n != m_task) {
        return;
    }
    reset();
}

Task *AllocatedResourceItemModel::task() const
{
    return m_task;
}

void AllocatedResourceItemModel::setTask(Task *task)
{
    debugPlan<<m_task<<"="<<task<<(task?task->name():QString());
    m_task = task;
    reset();
    debugPlan<<rowCount()<<":"<<sourceModel()->rowCount();
}

Resource *AllocatedResourceItemModel::resource(const QModelIndex &idx) const
{
    QModelIndex sidx = mapToSource(idx);
    if (sidx.isValid() && sidx.internalPointer() == nullptr) {
        return project()->resourceAt(sidx.row());
    }
    return nullptr;
}

QModelIndex AllocatedResourceItemModel::index(Resource *r) const
{
    return mapFromSource(static_cast<ResourceItemModel*>(sourceModel())->index(project()->indexOf(r), 0));
}

Qt::ItemFlags AllocatedResourceItemModel::flags(const QModelIndex & index) const
{
    Qt::ItemFlags f = QSortFilterProxyModel::flags(index);
    f &= ~Qt::ItemIsUserCheckable;
    return f;
}


QVariant AllocatedResourceItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section == 1) {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
            return xi18nc("@title:column", "Allocation");
        }
        return QVariant();
    }
    return QSortFilterProxyModel::headerData(section, orientation, role);
}

QVariant AllocatedResourceItemModel::allocation(const Resource *res, int role) const
{
    ResourceRequest *rr = m_task->requests().find(res);
    if (rr == nullptr) {
        return QVariant();
    }
    switch (role) {
        case Qt::DisplayRole: {
        case Qt::EditRole:
            // xgettext: no-c-format
            return i18nc("<value>%", "%1%",rr->units());
        }
        case Qt::ToolTipRole: {
            if (rr->units() == 0) {
                return xi18nc("@info:tooltip", "Not allocated");
            }
            break;
        }
        default:
            break;
    }
    return QVariant();
}

QVariant AllocatedResourceItemModel::data(const QModelIndex& idx, int role) const
{
    if (m_task == nullptr || role == Qt::CheckStateRole || role == Qt::DecorationRole) {
        return QVariant();
    }
    if (idx.column() == 1) {
        switch (role) {
            case Qt::TextAlignmentRole:
                return Qt::AlignLeft;
            default: {
                Resource *r = resource(idx);
                if (r) {
                    return allocation(r, role);
                }
                break;
            }
            return QVariant();
        }
    }
    return QSortFilterProxyModel::data(idx, role);
}

bool AllocatedResourceItemModel::filterAcceptsRow(int source_row, const QModelIndex & source_parent) const
{
    if (m_task == nullptr || source_parent.isValid()) {
        return false;
    }
    QModelIndex idx = sourceModel()->index(source_row, 0, source_parent);
    if (!idx.isValid()) {
        return false;
    }
    const Resource *resource = static_cast<ResourceItemModel*>(sourceModel())->resource(idx);
    if (resource == nullptr) {
        return false;
    }
    return m_task->requests().find(resource) != nullptr;
}
