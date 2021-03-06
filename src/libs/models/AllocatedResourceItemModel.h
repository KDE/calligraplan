/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2007 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef ALLOCATEDRESOURCEITEMMODEL_H
#define ALLOCATEDRESOURCEITEMMODEL_H

#include "planmodels_export.h"

#include <kptitemmodelbase.h>

#include <QSortFilterProxyModel>
#include <QMetaEnum>

class QByteArray;

namespace KIO {
    class Job;
}
class KJob;

namespace KPlato
{

class Project;
class Resource;
class Calendar;
class Task;
class Node;

class PLANMODELS_EXPORT AllocatedResourceItemModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit AllocatedResourceItemModel(QObject *parent = nullptr);

    int columnCount(const QModelIndex &idx) const override;

    Project *project() const;
    Task *task() const;
    Resource *resource(const QModelIndex &index) const;
    using QAbstractProxyModel::index;
    QModelIndex index(Resource *r) const;

    Qt::ItemFlags flags(const QModelIndex & index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex &idx, int role) const override;

public Q_SLOTS:
    void setProject(KPlato::Project *project);
    void setTask(KPlato::Task *task);

Q_SIGNALS:
    void expandAll();
    void resizeColumnToContents(int);

protected Q_SLOTS:
    void slotNodeChanged(KPlato::Node *n);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const override;
    void reset();

    QVariant allocation(const Resource *r, int role) const;

    Task *m_task;
};

}  //KPlato namespace

#endif
