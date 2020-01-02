 /* This file is part of the KDE project
   Copyright (C) 2005 - 2007, 2011 Dag Andersen <danders@get2net>

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
 * Boston, MA 02110-1301, USA.
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

/**
    The ResourceAppointmentsItemModel organizes appointments
    as hours booked per day (or week, month).
    It handles both internal and external appointments.
*/
class PLANMODELS_EXPORT ResourceAppointmentsItemModel : public ItemModelBase
{
    Q_OBJECT
public:
    explicit ResourceAppointmentsItemModel(QObject *parent = 0);
    ~ResourceAppointmentsItemModel() override;

    void setProject(Project *project) override;
    long id() const;

    Qt::ItemFlags flags(const QModelIndex & index) const override;

    QModelIndex parent(const QModelIndex & index) const override;
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const override;
    QModelIndex index(Resource *resource) const;

    int columnCount(const QModelIndex & parent = QModelIndex()) const override; 
    int rowCount(const QModelIndex & parent = QModelIndex()) const override; 

    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override; 
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    Node *node(const QModelIndex &index) const;
    Appointment *appointment(const QModelIndex &index) const;
    QModelIndex createAppointmentIndex(int row, int col, void *ptr) const;
    Appointment *externalAppointment(const QModelIndex &index) const;
    QModelIndex createExternalAppointmentIndex(int row, int col, void *ptr) const;
    Resource *resource(const QModelIndex &index) const;
    QModelIndex createResourceIndex(int row, int col, Resource *ptr) const;

    void refresh() override;
    void refreshData();

    QDate startDate() const;
    QDate endDate() const;

    Resource *parent(const Appointment *a) const;
    int rowNumber(Resource *res, Appointment *a) const;
    void setShowInternalAppointments(bool show);
    bool showInternalAppointments() const { return m_showInternal; }
    void setShowExternalAppointments(bool show);
    bool showExternalAppointments() const { return m_showExternal; }

Q_SIGNALS:
    void refreshed();
    void appointmentInserted(KPlato::Resource*, KPlato::Appointment*);
    
public Q_SLOTS:
    void setScheduleManager(KPlato::ScheduleManager *sm) override;

protected Q_SLOTS:
    void slotResourceChanged(KPlato::Resource*);
    void slotResourceToBeInserted(KPlato::Project *project, int row);
    void slotResourceInserted(KPlato::Resource *resource);
    void slotResourceToBeRemoved(KPlato::Project *project, int row, Resource *resource);
    void slotResourceRemoved();

    void slotCalendarChanged(KPlato::Calendar* cal);
    void slotProjectCalculated(KPlato::ScheduleManager *sm);
    
    void slotAppointmentToBeInserted(KPlato::Resource *r, int row);
    void slotAppointmentInserted(KPlato::Resource*, KPlato::Appointment*);
    void slotAppointmentToBeRemoved(KPlato::Resource *r, int row);
    void slotAppointmentRemoved();
    void slotAppointmentChanged(KPlato::Resource *r, KPlato::Appointment *a);
    
protected:
    QVariant notUsed(const ResourceGroup *res, int role) const;

    QVariant name(const Resource *res, int role) const;
    QVariant name(const Node *node, int role) const;
    QVariant name(const Appointment *appointment, int role) const;

    QVariant total(const Resource *res, int role) const;
    QVariant total(const Resource *res, const QDate &date, int role) const;
    QVariant total(const Appointment *a, int role) const;

    QVariant assignment(const Appointment *a, const QDate &date, int role) const;

    void connectSignals(ResourceGroup *group, bool connect);
    void connectSignals(Resource *resource, bool connect);

private:
    int m_columnCount;
    QHash<const Appointment*, EffortCostMap> m_effortMap;
    QHash<const Appointment*, EffortCostMap> m_externalEffortMap;
    QDate m_start;
    QDate m_end;
    
    ResourceGroup *m_group; // Used for sanity checks
    Resource *m_resource; // Used for sanity checks
    
    bool m_showInternal;
    bool m_showExternal;
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

    explicit ResourceAppointmentsRowModel(QObject *parent = 0);
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
    explicit ResourceAppointmentsGanttModel(QObject *parent = 0);
    ~ResourceAppointmentsGanttModel() override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

protected:
    QVariant data(const Resource *r, int column, int role = Qt::DisplayRole) const; 
    QVariant data(const Appointment *a, int column, int role = Qt::DisplayRole) const; 
    QVariant data(const AppointmentInterval *a, int column, int role = Qt::DisplayRole) const;
};

}  //KPlato namespace

#endif // KPTRESOURCEAPPOINTMENTSMODEL_H
