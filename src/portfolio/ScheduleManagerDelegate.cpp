/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2022 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "ScheduleManagerDelegate.h"
#include "SchedulingModel.h"

#include "kptschedulemodel.h"
#include <kptproject.h>

#include <QTreeView>

using namespace KPlato;

ScheduleManagerSFModel::ScheduleManagerSFModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(new ScheduleItemModel(this));
}

bool ScheduleManagerSFModel::filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const
{
    Q_UNUSED(source_parent)
    return source_column == ScheduleModel::ScheduleName;
}

//---------------------------
ScheduleManagerDelegate::ScheduleManagerDelegate(QObject *parent)
    : ItemDelegate(parent)
{
}

QWidget *ScheduleManagerDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)
    TreeComboBox *editor = new TreeComboBox(parent);
    editor->installEventFilter(const_cast<ScheduleManagerDelegate*>(this));
    auto m = new ScheduleManagerSFModel(editor);

    editor->setModel(m);
    return editor;
}

void ScheduleManagerDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    TreeComboBox *box = static_cast<TreeComboBox*>(editor);
    box->setSelectionMode(QAbstractItemView::SingleSelection);

    auto pm = static_cast<ScheduleManagerSFModel*>(box->model());
    auto sim = static_cast<ScheduleItemModel*>(pm->sourceModel());

    auto project = static_cast<KPlato::Project*>(index.data(PROJECT_ROLE).value<void*>());
    sim->setProject(project);

    QItemSelectionModel *sm = box->view()->selectionModel();
    sm->clearSelection();
    box->setCurrentIndexes(sm->selectedRows());
    box->view()->expandAll();
}

void ScheduleManagerDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    auto box = static_cast<TreeComboBox*>(editor);
    model->setData(index, box->currentText(), Qt::EditRole);
}

void ScheduleManagerDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    debugPlan<<editor<<":"<<option.rect<<","<<editor->sizeHint();
    QRect r = option.rect;
    r.setWidth(qMax(100, r.width()));
    editor->setGeometry(r);
}
