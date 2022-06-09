/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2022 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef ScheduleManagerDelegate_H
#define ScheduleManagerDelegate_H

#include "kptitemmodelbase.h"

#include "QSortFilterProxyModel"


class ScheduleManagerSFModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    ScheduleManagerSFModel(QObject *parent);

    bool filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const override;
};

//------------------------------------
class ScheduleManagerDelegate : public KPlato::ItemDelegate
{
    Q_OBJECT
public:
    explicit ScheduleManagerDelegate(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif
