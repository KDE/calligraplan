/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2020 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2006-2007, 2012 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "RequieredResourceDelegate.h"

#include "ResourceItemModel.h"
#include "ResourceItemSFModel.h"
#include "kptresourceallocationmodel.h"
#include <kptproject.h>
#include <kptdebug.h>

#include <QApplication>
#include <QComboBox>
#include <QKeyEvent>
#include <QModelIndex>
#include <QItemSelection>
#include <QStyleOptionViewItem>
#include <QTimeEdit>
#include <QPainter>
#include <QToolTip>
#include <QTreeView>
#include <QStylePainter>
#include <QMimeData>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextTableFormat>
#include <QVector>
#include <QTextLength>
#include <QTextTable>

#include <QBuffer>
#include <KoStore.h>
#include <KoXmlWriter.h>
#include <KoOdfWriteStore.h>

#include <KComboBox>
#include <KLineEdit>


using namespace KPlato;


//---------------------------
RequieredResourceDelegate::RequieredResourceDelegate(QObject *parent)
    : ItemDelegate(parent)
{
}

QWidget *RequieredResourceDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex &index) const
{
    if (index.data(Qt::CheckStateRole).toInt() == Qt::Unchecked) {
        return nullptr;
    }
    TreeComboBox *editor = new TreeComboBox(parent);
    editor->installEventFilter(const_cast<RequieredResourceDelegate*>(this));
    ResourceItemSFModel *m = new ResourceItemSFModel(editor);

    m->setFilterKeyColumn(ResourceModel::ResourceType);
    m->setFilterRole(Role::EnumListValue);
    m->setFilterRegularExpression(QString::number(Resource::Type_Material));

    editor->setModel(m);
    return editor;
}

void RequieredResourceDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    TreeComboBox *box = static_cast<TreeComboBox*>(editor);
    ResourceItemSFModel *pm = static_cast<ResourceItemSFModel*>(box->model());
    const QAbstractProxyModel *proxy = static_cast<const QAbstractProxyModel*>(index.model());
    const ResourceAllocationItemModel *model = qobject_cast<const ResourceAllocationItemModel*>(proxy->sourceModel());
    Q_ASSERT(model);
    pm->setProject(model->project());
    pm->setFilteredResources(QList<Resource*>() << model->resource(proxy->mapToSource(index)));
    QItemSelectionModel *sm = box->view()->selectionModel();
    sm->clearSelection();
    box->setCurrentIndexes(sm->selectedRows());
    box->view()->expandAll();
}

void RequieredResourceDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    TreeComboBox *box = static_cast<TreeComboBox*>(editor);

    QAbstractProxyModel *pm = static_cast<QAbstractProxyModel*>(box->model());
    ResourceItemModel *rm = qobject_cast<ResourceItemModel*>(pm->sourceModel());
    QList<Resource*> lst;
    const QList<QPersistentModelIndex> indexes = box->currentIndexes();
    for (const QModelIndex i : indexes) {
        lst << rm->resource(pm->mapToSource(i));
    }
    ResourceAllocationItemModel *mdl = qobject_cast<ResourceAllocationItemModel*>(static_cast<QAbstractProxyModel*>(model)->sourceModel());
    Q_ASSERT(mdl);
    mdl->setRequired(static_cast<QAbstractProxyModel*>(model)->mapToSource(index), lst);
}

void RequieredResourceDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    debugPlan<<editor<<":"<<option.rect<<","<<editor->sizeHint();
    QRect r = option.rect;
    r.setWidth(qMax(100, r.width()));
    editor->setGeometry(r);
}
