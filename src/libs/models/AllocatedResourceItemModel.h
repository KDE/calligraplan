/* This file is part of the KDE project
 * Copyright (C) 2019 Dag Andersen <danders@get2net>
 * Copyright (C) 2007 Dag Andersen <danders@get2net>
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
class ResourceGroup;
class Calendar;
class Task;
class Node;

class PLANMODELS_EXPORT AllocatedResourceItemModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit AllocatedResourceItemModel(QObject *parent = 0);

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
    QObject *object(const QModelIndex &idx) const;

    QVariant allocation(const Resource *r, int role) const;
    QVariant allocation(const ResourceGroup *g, int role) const;

    Task *m_task;
};

}  //KPlato namespace

#endif
