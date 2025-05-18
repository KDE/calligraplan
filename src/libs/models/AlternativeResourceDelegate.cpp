/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2020 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2006-2007, 2012 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "AlternativeResourceDelegate.h"

#include "ResourceItemModel.h"
#include "ResourceItemSFModel.h"
#include "kptresourceallocationmodel.h"
#include <kptproject.h>
#include <kptdebug.h>


using namespace KPlato;


//---------------------------
AlternativeResourceDelegate::AlternativeResourceDelegate(QObject *parent)
    : ItemDelegate(parent)
{
}

QWidget *AlternativeResourceDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex &index) const
{
    Q_UNUSED(index)

    TreeComboBox *editor = new TreeComboBox(parent);
    editor->installEventFilter(const_cast<AlternativeResourceDelegate*>(this));
    ResourceItemSFModel *m = new ResourceItemSFModel(editor);
    editor->setModel(m);
    return editor;
}

void AlternativeResourceDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    TreeComboBox *box = static_cast<TreeComboBox*>(editor);
    ResourceItemSFModel *pm = static_cast<ResourceItemSFModel*>(box->model());
    const QAbstractProxyModel *proxy = static_cast<const QAbstractProxyModel*>(index.model());
    const ResourceAllocationItemModel *model = qobject_cast<const ResourceAllocationItemModel*>(proxy->sourceModel());
    Q_ASSERT(model);
    pm->setProject(model->project());
    pm->setFilteredResources(QList<Resource*>() << model->resource(proxy->mapToSource(index)));
    pm->setFilterKeyColumn(ResourceModel::ResourceType);
    pm->setFilterRole(Qt::EditRole);
    pm->setFilterRegularExpression(index.sibling(index.row(), ResourceAllocationModel::RequestType).data(Qt::EditRole).toString());
}

void AlternativeResourceDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    TreeComboBox *box = static_cast<TreeComboBox*>(editor);

    ResourceItemSFModel *pm = static_cast<ResourceItemSFModel*>(box->model());
    QList<Resource*> lst;
    const QList<QPersistentModelIndex> indexes = box->currentIndexes();
    for (const QModelIndex i : indexes) {
        lst << pm->resource(i);
    }
    ResourceAllocationItemModel *mdl = qobject_cast<ResourceAllocationItemModel*>(static_cast<QAbstractProxyModel*>(model)->sourceModel());
    Q_ASSERT(mdl);
    mdl->setAlternativeRequests(static_cast<QAbstractProxyModel*>(model)->mapToSource(index), lst);
}

void AlternativeResourceDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    debugPlan<<editor<<":"<<option.rect<<","<<editor->sizeHint();
    QRect r = option.rect;
    r.setWidth(qMax(100, r.width()));
    editor->setGeometry(r);
}
