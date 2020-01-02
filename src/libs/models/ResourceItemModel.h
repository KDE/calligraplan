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

#ifndef RESOURCEITEMMODEL_H
#define RESOURCEITEMMODEL_H

#include "planmodels_export.h"

#include <kptitemmodelbase.h>

#include "ResourceModel.h"
#include "ResourceGroupModel.h"

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
 * The ResourceItemModel is organized as follows:
 * . Resource are the top level index
 * . Resources can be members of multiple groups which are shown as children to the top level resource
 * . A Team resource can also have the team member resources as children
 * 
 * The top level resource has an index with internalPointer == nullptr.
 * A group has an index with internalPointer == parent resource.
 * A team member resource has an index with internalPointer == parent resource.
 */

class PLANMODELS_EXPORT ResourceItemModel : public ItemModelBase
{
    Q_OBJECT
public:
    explicit ResourceItemModel(QObject *parent = nullptr);
    ~ResourceItemModel() override;

    const QMetaEnum columnMap() const override { return m_resourceModel.columnMap(); }

    void setProject(Project *project) override;

    Qt::ItemFlags flags(const QModelIndex & index) const override;

    QModelIndex parent(const QModelIndex & index) const override;
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const override;

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

    QModelIndex insertResource(Resource *resource, Resource *after = 0);

    int sortRole(int column) const override;

    Resource *resource(const QModelIndex &idx) const;
    ResourceGroup *group(const QModelIndex &idx) const;

    bool groupsEnabled() const;
    void setGroupsEnabled(bool enable);
    bool teamsEnabled() const;
    void setTeamsEnabled(bool enable);
    bool requiredEnabled() const;
    void setRequiredEnabled(bool enable);

protected Q_SLOTS:
    void slotResourceChanged(KPlato::Resource*);
    void slotResourceGroupChanged(KPlato::ResourceGroup *);

    void slotResourceGroupToBeAdded(KPlato::Resource *resource, int row);
    void slotResourceGroupAdded(KPlato::ResourceGroup *group);
    void slotResourceGroupToBeRemoved(KPlato::Resource *resource, int row, KPlato::ResourceGroup *group);
    void slotResourceGroupRemoved();

    void slotResourceToBeAdded(KPlato::Project *project, int row);
    void slotResourceAdded(KPlato::Resource *resource);
    void slotResourceToBeRemoved(KPlato::Project *project, int row, KPlato::Resource *resource);
    void slotResourceRemoved();

    void slotCalendarChanged(KPlato::Calendar* cal);

    void slotDataArrived(KIO::Job *job, const QByteArray &data  );
    void slotJobFinished(KJob *job);

protected:
    QVariant notUsed(const ResourceGroup *res, int role) const;
    
    QVariant name(const ResourceGroup *res, int role) const;
    bool setName(Resource *res, const QVariant &value, int role);
    bool setName(ResourceGroup *res, const QVariant &value, int role);
    
    QVariant type(const ResourceGroup *res, int role) const;
    bool setType(Resource *res, const QVariant &value, int role);
    bool setType(ResourceGroup *res, const QVariant &value, int role);

    bool setInitials(Resource *res, const QVariant &value, int role);
    bool setEmail(Resource *res, const QVariant &value, int role);
    bool setCalendar(Resource *res, const QVariant &value, int role);
    bool setUnits(Resource *res, const QVariant &value, int role);
    bool setAvailableFrom(Resource *res, const QVariant &value, int role);
    bool setAvailableUntil(Resource *res, const QVariant &value, int role);
    bool setNormalRate(Resource *res, const QVariant &value, int role);
    bool setOvertimeRate(Resource *res, const QVariant &value, int role);
    bool setAccount(Resource *res, const QVariant &value, int role);

    QList<Resource*> resourceList(QDataStream &stream);

    bool createResources(ResourceGroup *group, const QByteArray &data);

    void connectSignals(Resource *resource, bool enable);
    void connectSignals(ResourceGroup *group, bool enable);

private:
    ResourceModel m_resourceModel;
    ResourceGroupModel m_groupModel;
    bool m_groupsEnabled;
    bool m_teamsEnabled;
    bool m_requiredEnabled;

    struct DropData {
        Qt::DropAction action;
        int row;
        int column;
        QModelIndex parent;
        QByteArray data;
    } m_dropData;
    QMap<KJob*, DropData> m_dropDataMap;
};

}  //KPlato namespace

#endif
