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


#include "ResourceGroupDocker.h"

#include <ResourceItemModel.h>
#include <ResourceGroupItemModel.h>
#include <ResourceGroup.h>
#include <Resource.h>
#include <kptproject.h>
#include <AddParentGroupCmd.h>
#include <RemoveParentGroupCmd.h>

#include <KoDocument.h>

#include <QItemSelectionModel>

using namespace KPlato;


CheckableProxyModel::CheckableProxyModel(QObject *parent)
    : KCheckableProxyModel(parent)
{
    ResourceItemModel *m = new ResourceItemModel(this);
    m->setIsCheckable(false);
    setSourceModel(m);
    QItemSelectionModel *checkModel = new QItemSelectionModel(m, this);
    setSelectionModel(checkModel);
}

int CheckableProxyModel::columnCount(const QModelIndex &idx) const
{
    Q_UNUSED(idx)
    return 1;
}

ResourceGroupDocker::ResourceGroupDocker(QItemSelectionModel *groupSelection, ViewBase *view, const QString &identity, const QString &title)
    : DockWidget(view, identity, title)
    , m_groupSelection(groupSelection)
{
    QTreeView *x = new QTreeView(this);
    x->setModel(&m_checkable);
    x->setRootIsDecorated(false);
    x->setSelectionBehavior(QAbstractItemView::SelectRows);
    x->setSelectionMode(QAbstractItemView::ExtendedSelection);
    x->setHeaderHidden(true);
    setWidget(x);
    //         x->setDragDropMode(QAbstractItemView::DragOnly);
    //         x->setDragEnabled (true);
    connect(view, &ViewBase::projectChanged, this, &ResourceGroupDocker::setProject);
    connect(&m_checkable, &CheckableProxyModel::dataChanged, this, &ResourceGroupDocker::slotDataChanged);
    connect(m_groupSelection, &QItemSelectionModel::selectionChanged, this, &ResourceGroupDocker::slotSelectionChanged);
}

void ResourceGroupDocker::setProject(KPlato::Project *project)
{
    ResourceItemModel *m = qobject_cast<ResourceItemModel*>(m_checkable.sourceModel());
    m->setProject(project);
}

ResourceGroup *ResourceGroupDocker::selectedGroup() const
{
    ResourceGroup *group = nullptr;
    QModelIndexList lst;
    for (const QModelIndex &idx : m_groupSelection->selectedIndexes()) {
        if (idx.column() == 0) {
            lst << idx;
        }
    }
    if (lst.count() == 1) {
        const QModelIndex idx = lst.at(0);
        const ResourceGroupItemModel *gm = qobject_cast<const ResourceGroupItemModel*>(idx.model());
        group = gm->group(idx);
    }
    return group;
}

void ResourceGroupDocker::slotSelectionChanged()
{
    m_checkable.selectionModel()->clear();
    ResourceGroup *group = selectedGroup();
    if (group) {
        ResourceItemModel *m = qobject_cast<ResourceItemModel*>(m_checkable.sourceModel());
        Q_ASSERT(m);
        for (Resource *r : group->resources()) {
            const QModelIndex ridx = m_checkable.mapFromSource(m->index(r));
            m_checkable.setData(ridx, Qt::Checked, Qt::CheckStateRole);
        }
    }
}

void ResourceGroupDocker::slotDataChanged(const QModelIndex &idx1, const QModelIndex &idx2)
{
    qInfo()<<Q_FUNC_INFO<<qobject_cast<const CheckableProxyModel*>(idx1.model());
    ResourceGroup *group = selectedGroup();
    if (!group) {
        m_checkable.selectionModel()->clear();
        return;
    }
    int checked = idx1.data(Qt::CheckStateRole).toInt();
    const ResourceItemModel *m = qobject_cast<const ResourceItemModel*>(m_checkable.sourceModel());
    qInfo()<<Q_FUNC_INFO<<m;
    QModelIndex idx = m_checkable.mapToSource(idx1);
    Resource *resource = m->resource(idx);
    if (!resource) {
        return;
    }
    if (checked && !group->resources().contains(resource)) {
        KoDocument *doc = view->part();
        doc->addCommand(new AddParentGroupCmd(resource, group, kundo2_i18n("Add parent group")));
        return;
    }
    if (!checked && group->resources().contains(resource)) {
        KoDocument *doc = view->part();
        doc->addCommand(new RemoveParentGroupCmd(resource, group, kundo2_i18n("Remove parent group")));
        return;
    }
}
