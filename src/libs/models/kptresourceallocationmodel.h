/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2009 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTRESOURCEALLOCATIONMODEL_H
#define KPTRESOURCEALLOCATIONMODEL_H

#include "planmodels_export.h"

#include <kptitemmodelbase.h>

#include <QSortFilterProxyModel>
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
class ResourceAllocationItemModel;

class PLANMODELS_EXPORT ResourceAlternativesModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit ResourceAlternativesModel(ResourceAllocationItemModel *dataModel, QObject *parent = nullptr);

    void setResource(Resource *resource);
    Resource *resource() const;
    Resource *resource(const QModelIndex &idx) const;
    ResourceRequest *request(const QModelIndex &idx) const;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &idx, const QVariant &value, int role = Qt::EditRole) override;

protected:
    bool filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const override;
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private Q_SLOTS:
    void slotDataChanged();

private:
    Resource *m_resource;
    QHash<Resource*, QList<Resource*> > m_alternatives;
};

class PLANMODELS_EXPORT ResourceRequiredModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit ResourceRequiredModel(ResourceAllocationItemModel *dataModel, QObject *parent = nullptr);

    void setResource(Resource *resource);
    Resource *resource() const;
    Resource *resource(const QModelIndex &idx) const;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &idx, const QVariant &value, int role = Qt::EditRole) override;

protected:
    bool filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const override;
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private Q_SLOTS:
    void slotDataChanged();

private:
    Resource *m_resource;
    QHash<Resource*, QList<Resource*> > m_alternatives;
};

/**
 The ResourceAllocationModel gives access to resource requests
*/

class PLANMODELS_EXPORT ResourceAllocationModel : public QObject
{
    Q_OBJECT
public:
    explicit ResourceAllocationModel(QObject *parent = nullptr);
    ~ResourceAllocationModel() override;

    enum Properties {
        RequestName = 0,
        RequestType,
        RequestAllocation,
        RequestMaximum,
        RequestAlternative,
        RequestRequired
    };
    Q_ENUM(Properties)
    
    const QMetaEnum columnMap() const;
    void setProject(Project *project);
    Task *task() const { return m_task; }
    void setTask(Task *task);
    int propertyCount() const;
    QVariant data(const Resource *resource, int property, int role = Qt::DisplayRole) const;
    static QVariant headerData(int section, int role = Qt::DisplayRole);

    QVariant name(const Resource *res, int role) const;
    QVariant type(const Resource *res, int role) const;
    QVariant allocation(const Resource *res, int role) const;
    QVariant maximum(const Resource *res, int role) const;
    QVariant required(const Resource *res, int role) const;
    QVariant alternative(const Resource *res, int role) const;

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
    explicit ResourceAllocationItemModel(QObject *parent = nullptr);
    ~ResourceAllocationItemModel() override;

    const QMetaEnum columnMap() const override { return m_model.columnMap(); }

    void setProject(Project *project) override;
    Task *task() const { return m_model.task(); }

    Qt::ItemFlags flags(const QModelIndex & index) const override;

    QModelIndex parent(const QModelIndex & index) const override;
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const override;
    QModelIndex index(Resource *resource) const;

    int columnCount(const QModelIndex & parent = QModelIndex()) const override; 
    int rowCount(const QModelIndex & parent = QModelIndex()) const override; 

    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override; 
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;


    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QAbstractItemDelegate *createDelegate(int col, QWidget *parent) const override;
    
    QObject *object(const QModelIndex &index) const;

    const QHash<const Resource*, ResourceRequest*> &resourceCache() const { return m_resourceCache; }
    
    Resource *resource(const QModelIndex &idx) const;

    void addRequiredResource(Resource *resource, Resource *required);
    bool removeRequiredResource(Resource *resource, Resource *required);

    void setRequired(const QModelIndex &idx, const QList<Resource*> &lst);
    QList<Resource*> required(const QModelIndex &idx) const;

    void addAlternativeRequest(Resource *resource, ResourceRequest *alternative);
    bool removeAlternativeRequest(Resource *resource, Resource *alternative);
    ResourceRequest *alternativeRequest(Resource *resource, Resource *alternative) const;

    void setAlternativeRequests(const QModelIndex &idx, const QList<Resource*> &lst);
    QList<ResourceRequest*> alternativeRequests(const QModelIndex &idx) const;

public Q_SLOTS:
    void setTask(KPlato::Task *task);

protected Q_SLOTS:
    void slotResourceChanged(KPlato::Resource*);
    void slotResourceToBeAdded(KPlato::Project *project, int row);
    void slotResourceAdded(KPlato::Resource *resource);
    void slotResourceToBeRemoved(KPlato::Project *project, int row, KPlato::Resource *resource);
    void slotResourceRemoved();

protected:
    void filldata(Task *task);

    QVariant allocation(const Resource *res, int role) const;
    bool setAllocation(Resource *res, const QVariant &value, int role);

    bool setRequired(const QModelIndex &idx, const QVariant &value, int role);
    QVariant required(const QModelIndex &idx, int role) const;

    bool setAlternative(const QModelIndex &idx, const QVariant &value, int role);
    QVariant alternative(const QModelIndex &idx, int role) const;

private:
    int requestedResources(const ResourceGroup *res) const;
    bool hasMaterialResources() const;

private:
    ResourceAllocationModel m_model;

    QHash<const Resource*, ResourceRequest*> m_resourceCache;
    QHash<const Resource*, int> m_requiredChecked;
    QHash<const Resource*, int> m_alternativeChecked;
};


}  //KPlato namespace

#endif
