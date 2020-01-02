/* This file is part of the KDE project
 * Copyright (C) 2019 Dag Andersen <danders@get2net.dk>
 * Copyright (C) 2006 - 2007, 2012 Dag Andersen <danders@get2net.dk>
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
#include "RequieredResourceDelegate.h"

#include "ResourceGroupItemModel.h"
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

#include <kcombobox.h>
#include <klineedit.h>


using namespace KPlato;


//---------------------------
RequieredResourceDelegate::RequieredResourceDelegate(QObject *parent)
    : ItemDelegate(parent)
{
}

QWidget *RequieredResourceDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex &index) const
{
    if (index.data(Qt::CheckStateRole).toInt() == Qt::Unchecked) {
        return 0;
    }
    TreeComboBox *editor = new TreeComboBox(parent);
    editor->installEventFilter(const_cast<RequieredResourceDelegate*>(this));
    ResourceItemSFModel *m = new ResourceItemSFModel(editor);
    editor->setModel(m);
    return editor;
}

void RequieredResourceDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    TreeComboBox *box = static_cast<TreeComboBox*>(editor);
    ResourceItemSFModel *pm = static_cast<ResourceItemSFModel*>(box->model());
    ResourceGroupItemModel *rm = qobject_cast<ResourceGroupItemModel*>(pm->sourceModel());
    Q_ASSERT(rm);
    const ResourceAllocationItemModel *model = qobject_cast<const ResourceAllocationItemModel*>(index.model());
    Q_ASSERT(model);
    rm->setProject(model->project());
    pm->addFilteredResource(model->resource(index));
    QItemSelectionModel *sm = box->view()->selectionModel();
    sm->clearSelection();
    foreach (const Resource *r, model->required(index)) {
        // TODO
//         QModelIndex i = pm->mapFromSource(rm->index(r));
//         sm->select(i, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }
    box->setCurrentIndexes(sm->selectedRows());
    box->view()->expandAll();
}

void RequieredResourceDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                const QModelIndex &index) const
{
    TreeComboBox *box = static_cast<TreeComboBox*>(editor);

    QAbstractProxyModel *pm = static_cast<QAbstractProxyModel*>(box->model());
    ResourceGroupItemModel *rm = qobject_cast<ResourceGroupItemModel*>(pm->sourceModel());
    QList<Resource*> lst;
    foreach (const QModelIndex &i, box->currentIndexes()) {
        lst << rm->resource(pm->mapToSource(i));
    }
    ResourceAllocationItemModel *mdl = qobject_cast<ResourceAllocationItemModel*>(model);
    Q_ASSERT(mdl);
    mdl->setRequired(index, lst);
}

void RequieredResourceDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    debugPlan<<editor<<":"<<option.rect<<","<<editor->sizeHint();
    QRect r = option.rect;
    r.setWidth(qMax(100, r.width()));
    editor->setGeometry(r);
}
