/* This file is part of the KDE project
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

void CheckableProxyModel::clear(const QModelIndex &parent)
{
    for (int r = 0; r < rowCount(parent); ++r) {
        QModelIndex idx = index(r, 0, parent);
        if (idx.isValid()) {
            setData(idx, Qt::Unchecked, Qt::CheckStateRole);
            clear(idx);
        }
    }
}

ResourceGroupDocker::ResourceGroupDocker(QItemSelectionModel *groupSelection, ViewBase *view, const QString &identity, const QString &title)
    : DockWidget(view, identity, title)
    , m_groupSelection(groupSelection)
    , m_group(nullptr)
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

void ResourceGroupDocker::setGroup(KPlato::ResourceGroup *group)
{
    if (m_group) {
        disconnect(m_group, &ResourceGroup::resourceAdded, this, &ResourceGroupDocker::updateCheckableModel);
        disconnect(m_group, &ResourceGroup::resourceRemoved, this, &ResourceGroupDocker::updateCheckableModel);
    }
    m_group = group;
    if (m_group) {
        connect(m_group, &ResourceGroup::resourceAdded, this, &ResourceGroupDocker::updateCheckableModel);
        connect(m_group, &ResourceGroup::resourceRemoved, this, &ResourceGroupDocker::updateCheckableModel);
    }
    updateCheckableModel();
}

void ResourceGroupDocker::updateCheckableModel()
{
    disconnect(&m_checkable, &CheckableProxyModel::dataChanged, this, &ResourceGroupDocker::slotDataChanged);
    m_checkable.clear();
    if (m_group) {
        ResourceItemModel *m = qobject_cast<ResourceItemModel*>(m_checkable.sourceModel());
        Q_ASSERT(m);
        const auto resources = m_group->resources();
        for (Resource *r : resources) {
            const QModelIndex ridx = m_checkable.mapFromSource(m->index(r));
            m_checkable.setData(ridx, Qt::Checked, Qt::CheckStateRole);
        }
        connect(&m_checkable, &CheckableProxyModel::dataChanged, this, &ResourceGroupDocker::slotDataChanged);
    }
}

QModelIndex ResourceGroupDocker::selectedGroupIndex() const
{
    QModelIndexList lst;
    const auto selectedIndexes = m_groupSelection->selectedIndexes();
    for (const QModelIndex &idx : selectedIndexes) {
        if (idx.column() == 0) {
            lst << idx;
        }
    }
    if (lst.count() == 1) {
        return lst.at(0);
    }
    return QModelIndex();
}

ResourceGroup *ResourceGroupDocker::selectedGroup() const
{
    ResourceGroup *group = nullptr;
    const QModelIndex idx = selectedGroupIndex();
    if (idx.isValid()) {
        const ResourceGroupItemModel *gm = qobject_cast<const ResourceGroupItemModel*>(idx.model());
        group = gm->group(idx);
    }
    return group;
}

void ResourceGroupDocker::slotSelectionChanged()
{
    setGroup(selectedGroup());
}

void ResourceGroupDocker::slotDataChanged(const QModelIndex &idx1, const QModelIndex &idx2)
{
    Q_UNUSED(idx2);
    if (!m_group) {
        return;
    }
    int checked = idx1.data(Qt::CheckStateRole).toInt();
    const ResourceItemModel *m = qobject_cast<const ResourceItemModel*>(m_checkable.sourceModel());
    QModelIndex idx = m_checkable.mapToSource(idx1);
    Resource *resource = m->resource(idx);
    if (!resource) {
        return;
    }
    disconnect(&m_checkable, &CheckableProxyModel::dataChanged, this, &ResourceGroupDocker::slotDataChanged);
    if (checked && !m_group->resources().contains(resource)) {
        KoDocument *doc = view->part();
        doc->addCommand(new AddParentGroupCmd(resource, m_group, kundo2_i18n("Add parent group")));
    } else if (!checked && m_group->resources().contains(resource)) {
        KoDocument *doc = view->part();
        doc->addCommand(new RemoveParentGroupCmd(resource, m_group, kundo2_i18n("Remove parent group")));
    }
    connect(&m_checkable, &CheckableProxyModel::dataChanged, this, &ResourceGroupDocker::slotDataChanged);
}
