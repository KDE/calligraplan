/* This file is part of the KDE project
 * Copyright (C) 2020 Dag Andersen <danders@get2net>
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

#ifndef RESOURCEGROUPITEMMODEL_H
#define RESOURCEGROUPITEMMODEL_H

#include "planmodels_export.h"

#include <kptitemmodelbase.h>
#include "ResourceGroupModel.h"
#include "ResourceModel.h"

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

/**
 * @class ResourceGroupItemModel
 * 
 * ResourceGroups to any depth.
 * Resources may be member of multiple groups on any level.
 * Resources only shown if m_resourcesEnabled.
 * Resources shown behind groups.
 * 
 *  * Structure:
 * --- ResourceGroup
 *  !   !- ResourceGroup (0 or more)
 *  !   !   !- ResourceGroup (0 or more)
 *  !   !   !- Resource (0 or more, if m_resourcesEnabled)
 *  !   !- Resource (0 or more, if m_resourcesEnabled)
 *  !- :
 *  !- ResourceGroup
 * 
 * QModelIndex: internalPointer() pionts to parent group, nullptr if top level group.
 */

class PLANMODELS_EXPORT ResourceGroupItemModel : public ItemModelBase
{
    Q_OBJECT
public:
    explicit ResourceGroupItemModel(QObject *parent = nullptr);
    ~ResourceGroupItemModel() override;

    const QMetaEnum columnMap() const override { return m_groupModel.columnMap(); }

    void setProject(Project *project) override;

    Qt::ItemFlags flags(const QModelIndex & index) const override;

    QModelIndex parent(const QModelIndex & index) const override;
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const override;

    QModelIndex index(const ResourceGroup *group, int column = 0) const;

    int columnCount(const QModelIndex & parent = QModelIndex()) const override; 
    int rowCount(const QModelIndex & parent = QModelIndex()) const override; 

    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override; 
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;


    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QStringList mimeTypes () const override;
    Qt::DropActions supportedDropActions() const override;
    bool dropAllowed(const QModelIndex &index, int dropIndicatorPosition, const QMimeData *data) override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    QMimeData *mimeData(const QModelIndexList & indexes) const override;
    
    QAbstractItemDelegate *createDelegate(int col, QWidget *parent) const override;

    ResourceGroup *group(const QModelIndex &index) const;
    Resource *resource(const QModelIndex &index) const;
    QModelIndex insertGroup(ResourceGroup *g, ResourceGroup *parent = nullptr);
    QModelIndex insertResource(ResourceGroup *g, Resource *r, Resource *after = 0);

    int sortRole(int column) const override;

    void setResourcesEnabled(bool enable);
    bool resourcesEnabled() const;

Q_SIGNALS:
    void resourceAdded(KPlato::Resource *resource);
    void resourceRemoved();

protected Q_SLOTS:
    void slotResourceGroupChanged(KPlato::ResourceGroup *group);
    void slotResourceGroupToBeAdded(Project *project, ResourceGroup *parent, int row);
    void slotResourceGroupAdded(KPlato::ResourceGroup *group);
    void slotResourceGroupToBeRemoved(Project *project, ResourceGroup *parent, int row, KPlato::ResourceGroup *group);
    void slotResourceGroupRemoved();

    void slotResourceChanged(KPlato::Resource *resource);
    void slotResourceToBeAdded(KPlato::ResourceGroup *group, int row);
    void slotResourceAdded(KPlato::Resource *resource);
    void slotResourceToBeRemoved(KPlato::ResourceGroup *group, int row, KPlato::Resource *resource);
    void slotResourceRemoved();

    void slotDataArrived(KIO::Job *job, const QByteArray &data  );
    void slotJobFinished(KJob *job);

protected:
    QVariant notUsed(const ResourceGroup *res, int role) const;
    
    QVariant name(const ResourceGroup *res, int role) const;
    bool setName(ResourceGroup *res, const QVariant &value, int role);
    
    QVariant type(const ResourceGroup *res, int role) const;
    bool setType(ResourceGroup *res, const QVariant &value, int role);
    bool setUnits(ResourceGroup *res, const QVariant &value, int role);
    bool setCoordinator(ResourceGroup *res, const QVariant &value, int role);

    QList<Resource*> resourceList(QDataStream &stream);

    bool createResources(ResourceGroup *group, const QByteArray &data);

    void connectSignals(ResourceGroup *group, bool connect);
    void connectSignals(Resource *resource, bool connect);

private:
    ResourceGroupModel m_groupModel;
    ResourceModel m_resourceModel;
    bool m_resourcesEnabled;

    struct DropData {
        Qt::DropAction action;
        int row;
        int column;
        QModelIndex parent;
        QByteArray data;
    } m_dropData;
    QMap<KJob*, DropData> m_dropDataMap;
};


class PLANMODELS_EXPORT ParentGroupItemModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit ParentGroupItemModel(QObject *parent = nullptr);
    ~ParentGroupItemModel() override;

    int columnCount(const QModelIndex &idx = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex &idx) const override;
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;

    void setGroupIsCheckable(bool checkable);
    bool groupIsCheckable() const;

    void setResourcesEnabled(bool enable);
    bool resourcesEnabled() const;

public Q_SLOTS:
    void setProject(Project *project);
    void setResource(Resource *resource);

protected Q_SLOTS:
    void slotResourceAdded(KPlato::ResourceGroup *group);
    void slotResourceRemoved();

Q_SIGNALS:
    void executeCommand(KUndo2Command *cmd);

private:
    ResourceGroupItemModel *m_model;
    Resource *m_resource;
    bool m_groupIsCheckable;
};

}  //KPlato namespace

#endif
