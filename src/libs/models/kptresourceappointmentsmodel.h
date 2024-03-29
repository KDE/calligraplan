 /* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005-2007, 2011 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTRESOURCEAPPOINTMENTSMODEL_H
#define KPTRESOURCEAPPOINTMENTSMODEL_H

#include "planmodels_export.h"

#include <kptitemmodelbase.h>
#include "kpteffortcostmap.h"


namespace KPlato
{

class Project;
class Node;
class Appointment;
class AppointmentInterval;
class Resource;
class ResourceGroup;
class ScheduleManager;
class MainSchedule;
class Calendar;

class ItemData;

/**
    The ResourceAppointmentsItemModel organizes appointments as hours booked per day.

    All resources are listed under a 'Project' group.

    Resources belonging to resourcegroup(s) are also listed under these group(s).

    Team resources are not included.
*/
class PLANMODELS_EXPORT ResourceAppointmentsItemModel : public ItemModelBase
{
    Q_OBJECT
public:
    explicit ResourceAppointmentsItemModel(QObject *parent = nullptr);
    ~ResourceAppointmentsItemModel() override;

    void setProject(Project *project) override;
    long id() const;

    Qt::ItemFlags flags(const QModelIndex & index) const override;

    QModelIndex parent(const QModelIndex & index) const override;
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const override;

    int columnCount(const QModelIndex & parent = QModelIndex()) const override; 
    int rowCount(const QModelIndex & parent = QModelIndex()) const override; 

    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override; 
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    Node *node(const QModelIndex &index) const;
    Appointment *appointment(const QModelIndex &index) const;
    Resource *resource(const QModelIndex &index) const;

    QDate startDate() const;
    QDate endDate() const;

    Resource *parent(const Appointment *a) const;

Q_SIGNALS:
    void refreshed();
    void appointmentInserted(KPlato::Resource*, KPlato::Appointment*);
    
public Q_SLOTS:
    void setScheduleManager(KPlato::ScheduleManager *sm) override;

protected Q_SLOTS:
    void refresh() override;

    void slotResourceGroupInserted(KPlato::ResourceGroup *group);
    void slotResourceGroupToBeRemoved(KPlato::Project *project, KPlato::ResourceGroup *parent, int row, KPlato::ResourceGroup *group);

    void slotResourceChanged(KPlato::Resource*);
    void slotResourceToBeInserted(KPlato::Project *project, int row);
    void slotResourceInserted(KPlato::Resource *resource);
    void slotResourceToBeRemoved(KPlato::Project *project, int row, KPlato::Resource *resource);
    void slotResourceRemoved();

    void slotCalendarChanged(KPlato::Calendar* cal);
    void slotProjectCalculated(KPlato::ScheduleManager *sm);
    
    void slotAppointmentToBeInserted(KPlato::Resource *r, int row);
    void slotAppointmentInserted(KPlato::Resource*, KPlato::Appointment*);
    void slotAppointmentToBeRemoved(KPlato::Resource *r, int row);
    void slotAppointmentRemoved();
    void slotAppointmentChanged(KPlato::Resource *r, KPlato::Appointment *a);
    
protected:
    void refreshData();

    QVariant total(const ItemData *item, int role) const;
    QVariant total(const ItemData *item, const QDate &date, int role) const;

    QVariant total(const ResourceGroup *group, int role) const;
    QVariant total(const ResourceGroup *group, const QDate &date, int role) const;
    QVariant total(const Resource *res, int role) const;
    QVariant total(const Resource *res, const QDate &date, int role) const;
    QVariant total(const Appointment *a, int role) const;

    QVariant assignment(const Appointment *a, const QDate &date, int role) const;

    void connectSignals(ResourceGroup *group, bool connect);
    void connectSignals(Resource *resource, bool connect);

    void addResource(Resource *resource, ItemData *parentItem);
    void addGroup(ResourceGroup *group, ItemData *parentItem);

private:
    ItemData *m_rootItem;
    int m_columnCount;
    QHash<const Appointment*, EffortCostMap> m_effortMap;
    QDate m_start;
    QDate m_end;
};

/**
    The ResourceAppointmentsRowModel returns each appointment interval as a new row.
*/
class PLANMODELS_EXPORT ResourceAppointmentsRowModel : public ItemModelBase
{
    Q_OBJECT
public:
    enum Properties {
        Name = 0,
        Type,
        StartTime,
        EndTime,
        Load
    };
    Q_ENUM(Properties)
    const QMetaEnum columnMap() const override;

    explicit ResourceAppointmentsRowModel(QObject *parent = nullptr);
    ~ResourceAppointmentsRowModel() override;

    void setProject(Project *project) override;
    long id() const;

    int columnCount(const QModelIndex & parent = QModelIndex()) const override; 
    int rowCount(const QModelIndex & parent = QModelIndex()) const override; 

    QModelIndex parent(const QModelIndex &idx = QModelIndex()) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override; 
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    /// If @p idx is an appointment, return it's parent resource, else 0
    Resource *parentResource(const QModelIndex &idx) const;
    /// If @p idx is a resource, return it, else 0
    Resource *resource(const QModelIndex &idx) const;
    /// If @p idx is an appointment interval, return it's parent appointment, else 0
    Appointment *parentAppointment(const QModelIndex &idx) const;
    /// If @p idx is an appointment, return it, else 0
    Appointment *appointment(const QModelIndex &idx) const;
    /// If @p idx is an appointment interval, return it, else 0
    AppointmentInterval *interval(const QModelIndex &idx) const;

    QModelIndex index(Resource *r) const;
    QModelIndex index(Appointment *a) const;

    /// If @p idx is an appointment, return the node, else 0
    Node *node(const QModelIndex &idx) const;
    
    /// Return the sortorder to be used for @p column
    int sortRole(int column) const override;

    class Private;

public Q_SLOTS:
    void setScheduleManager(KPlato::ScheduleManager *sm) override;

protected Q_SLOTS:
    void slotResourceToBeInserted(KPlato::Project *project, int row);
    void slotResourceInserted(KPlato::Resource *resource);
    void slotResourceToBeRemoved(KPlato::Project *project, int row, KPlato::Resource *resource);
    void slotResourceRemoved();

    void slotAppointmentToBeInserted(KPlato::Resource *r, int row);
    void slotAppointmentInserted(KPlato::Resource *r, KPlato::Appointment *a);
    void slotAppointmentToBeRemoved(KPlato::Resource *r, int row);
    void slotAppointmentRemoved();
    void slotAppointmentChanged(KPlato::Resource *r, KPlato::Appointment *a);
    void slotProjectCalculated(KPlato::ScheduleManager *sm);

protected:
    QModelIndex createResourceIndex(int row, int column);
    QModelIndex createAppointmentIndex(int row, int column, Resource *r);
    QModelIndex createIntervalIndex(int row, int column, Appointment *a);

    void connectSignals(Resource *resource, bool connect);

protected:
    QMap<void*, Private*> m_datamap;
    MainSchedule *m_schedule;
};

/**
    The ResourceAppointmentsGanttModel specialized for use by KGantt
*/
class PLANMODELS_EXPORT ResourceAppointmentsGanttModel : public ResourceAppointmentsRowModel
{
    Q_OBJECT
public:
    explicit ResourceAppointmentsGanttModel(QObject *parent = nullptr);
    ~ResourceAppointmentsGanttModel() override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

protected:
    QVariant data(const Resource *r, int column, int role = Qt::DisplayRole) const; 
    QVariant data(const Appointment *a, int column, int role = Qt::DisplayRole) const; 
    QVariant data(const AppointmentInterval *a, int column, int role = Qt::DisplayRole) const;
};

}  //KPlato namespace

#endif // KPTRESOURCEAPPOINTMENTSMODEL_H
