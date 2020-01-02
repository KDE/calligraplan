/* This file is part of the KDE project
  Copyright (C) 2009 Dag Andersen <danders@get2net>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#ifndef KPTRESOURCEALLOCATIONMODEL_H
#define KPTRESOURCEALLOCATIONMODEL_H

#include "planmodels_export.h"

#include <kptitemmodelbase.h>

#include <QMetaEnum>
#include <QHash>



namespace KPlato
{

class Project;
class Task;
class Resource;
class ResourceGroup;
class ResourceRequest;
class ResourceGroupRequest;

/**
 The ResourceAllocationModel gives access to resource requests
*/

class PLANMODELS_EXPORT ResourceAllocationModel : public QObject
{
    Q_OBJECT
public:
    explicit ResourceAllocationModel(QObject *parent = 0);
    ~ResourceAllocationModel() override;

    enum Properties {
        RequestName = 0,
        RequestType,
        RequestAllocation,
        RequestMaximum,
        RequestRequired
    };
    Q_ENUM(Properties)
    
    const QMetaEnum columnMap() const;
    void setProject(Project *project);
    Task *task() const { return m_task; }
    void setTask(Task *task);
    int propertyCount() const;
    QVariant data(const ResourceGroup *group, const Resource *resource, int property, int role = Qt::DisplayRole) const;
    QVariant data(const ResourceGroup *group, int property, int role = Qt::DisplayRole) const;
    static QVariant headerData(int section, int role = Qt::DisplayRole);

    QVariant name(const Resource *res, int role) const;
    QVariant type(const Resource *res, int role) const;
    QVariant allocation(const ResourceGroup *group, const Resource *res, int role) const;
    QVariant maximum(const Resource *res, int role) const;
    QVariant required(const Resource *res, int role) const;
    
    QVariant name(const ResourceGroup *res, int role) const;
    QVariant type(const ResourceGroup *res, int role) const;
    QVariant allocation(const ResourceGroup *res, int role) const;
    QVariant maximum(const ResourceGroup *res, int role) const;

private:
    Project *m_project;
    Task *m_task;
};

/**
 The ResourceAllocationItemModel facilitates viewing and modifying
 resource allocations for a task.
*/

class PLANMODELS_EXPORT ResourceAllocationItemModel : public ItemModelBase
{
    Q_OBJECT
public:
    explicit ResourceAllocationItemModel(QObject *parent = 0);
    ~ResourceAllocationItemModel() override;

    const QMetaEnum columnMap() const override { return m_model.columnMap(); }

    void setProject(Project *project) override;
    Task *task() const { return m_model.task(); }

    Qt::ItemFlags flags(const QModelIndex & index) const override;

    QModelIndex parent(const QModelIndex & index) const override;
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const override;
    QModelIndex index(const ResourceGroup *group) const;
    QModelIndex index(const Resource *resource) const;

    int columnCount(const QModelIndex & parent = QModelIndex()) const override; 
    int rowCount(const QModelIndex & parent = QModelIndex()) const override; 

    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override; 
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;


    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QAbstractItemDelegate *createDelegate(int col, QWidget *parent) const override;
    
    QObject *object(const QModelIndex &index) const;

    const QHash<const Resource*, ResourceRequest*> &resourceCache() const { return m_resourceCache; }
    const QHash<const ResourceGroup*, ResourceGroupRequest*> &groupCache() const { return m_groupCache; }
    
    Resource *resource(const QModelIndex &idx) const;
    void setRequired(const QModelIndex &idx, const QList<Resource*> &lst);
    QList<Resource*> required(const QModelIndex &idx) const;

public Q_SLOTS:
    void setTask(KPlato::Task *task);

protected Q_SLOTS:
    void slotResourceGroupChanged(KPlato::ResourceGroup*);
    void slotResourceGroupToBeInserted(KPlato::Project *project, int row);
    void slotResourceGroupInserted(KPlato::ResourceGroup *group);
    void slotResourceGroupToBeRemoved(KPlato::Project *project, int row, KPlato::ResourceGroup *group);
    void slotResourceGroupRemoved();

    void slotResourceChanged(KPlato::Resource*);
    void slotResourceToBeAdded(KPlato::ResourceGroup *group, int row);
    void slotResourceAdded(KPlato::Resource *resource);
    void slotResourceToBeRemoved(KPlato::ResourceGroup *group, int row, KPlato::Resource *resource);
    void slotResourceRemoved();
    
protected:
    void filldata(Task *task);

    QVariant notUsed(const ResourceGroup *res, int role) const;
    
    QVariant allocation(const ResourceGroup *group, const Resource *res, int role) const;
    QVariant allocation(const ResourceGroup *res, int role) const;
    bool setAllocation(ResourceGroup *res, const QVariant &value, int role);
    bool setAllocation(Resource *res, const QVariant &value, int role);

    QVariant maximum(const ResourceGroup *res, int role) const;

    bool setRequired(const QModelIndex &idx, const QVariant &value, int role);
    QVariant required(const QModelIndex &idx, int role) const;

    void connectSignals(ResourceGroup *group, bool enable);
    void connectSignals(Resource *resource, bool enable);

private:
    int requestedResources(const ResourceGroup *res) const;
    bool hasMaterialResources() const;

private:
    ResourceAllocationModel m_model;

    QHash<const Resource*, ResourceRequest*> m_resourceCache;
    QHash<const Resource*, int> m_requiredChecked;
    QHash<const ResourceGroup*, ResourceGroupRequest*> m_groupCache;
};


}  //KPlato namespace

#endif
