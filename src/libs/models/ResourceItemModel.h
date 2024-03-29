/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2007 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
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
 * . Resources can be members of multiple groups which can be shown as children to the top level resource
 * . A Team resource can also have the team member resources as children
 * . A resource with required resources can have these as children
 *
 * The top level resource has an index with internalPointer == nullptr.
 * A group has an index with internalPointer == parent resource.
 * A team member resource has an index with internalPointer == parent resource.
 * A required resource has an index with internalPointer == parent resource.
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

    QModelIndex insertResource(Resource *resource, Resource *after = nullptr);

    int sortRole(int column) const override;

    QModelIndex index(Resource *resource) const;
    Resource *resource(const QModelIndex &idx) const;
    ResourceGroup *group(const QModelIndex &idx) const;

    bool groupsEnabled() const;
    bool teamsEnabled() const;
    bool requiredEnabled() const;

    void setIsCheckable(bool enable);
    bool isCheckable() const;

public Q_SLOTS:
    void setGroupsEnabled(bool enable);
    void setTeamsEnabled(bool enable);
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

    void slotResourceTeamToBeAdded(KPlato::Resource *resource, int row);
    void slotResourceTeamAdded(KPlato::Resource *resource);
    void slotResourceTeamToBeRemoved(KPlato::Resource *resource, int row, KPlato::Resource *team);
    void slotResourceTeamRemoved();

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
    bool m_isCheckable;

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
