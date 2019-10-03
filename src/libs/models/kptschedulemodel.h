/* This file is part of the KDE project
  Copyright (C) 2007, 2008 Dag Andersen <danders@get2net.dk>
  Copyright (C) 2011 Dag Andersen <danders@get2net.dk>

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

#ifndef KPTSCHEDULEMODEL_H
#define KPTSCHEDULEMODEL_H

#include "planmodels_export.h"

#include "kptitemmodelbase.h"
#include "kptschedule.h"

#include <QStandardItemModel>
#include <QSortFilterProxyModel>

namespace KPlato
{

class Project;
class ScheduleManager;
class MainSchedule;
class Schedule;

class PLANMODELS_EXPORT ScheduleModel : public QObject
{
    Q_OBJECT
public:
    explicit ScheduleModel( QObject *parent = 0 );
    ~ScheduleModel() override;
    
    enum Properties {
        ScheduleName = 0,
        ScheduleState,
        ScheduleDirection,
        ScheduleOverbooking,
        ScheduleDistribution,
        SchedulePlannedStart,
        SchedulePlannedFinish,
        ScheduleScheduler,
        ScheduleGranularity,
        ScheduleScheduled,
        ScheduleMode
    };
    Q_ENUM(Properties)
    const QMetaEnum columnMap() const;
    
    int propertyCount() const;
};

class PLANMODELS_EXPORT ScheduleItemModel : public ItemModelBase
{
    Q_OBJECT
public:
    explicit ScheduleItemModel( QObject *parent = 0 );
    ~ScheduleItemModel() override;

    const QMetaEnum columnMap() const override { return m_model.columnMap(); }
    
    void setProject( Project *project ) override;

    Qt::ItemFlags flags( const QModelIndex & index ) const override;

    QModelIndex parent( const QModelIndex & index ) const override;
    QModelIndex index( int row, int column, const QModelIndex & parent = QModelIndex() ) const override;
    QModelIndex index( const ScheduleManager *manager ) const;

    int columnCount( const QModelIndex & parent = QModelIndex() ) const override; 
    int rowCount( const QModelIndex & parent = QModelIndex() ) const override; 

    QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const override; 
    bool setData( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole ) override;

    QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const override;

    QAbstractItemDelegate *createDelegate( int column, QWidget *parent ) const override;
    
    void sort( int column, Qt::SortOrder order = Qt::AscendingOrder ) override;

    QMimeData * mimeData( const QModelIndexList & indexes ) const override;
    QStringList mimeTypes () const override;

    ScheduleManager *manager( const QModelIndex &index ) const;
    
    void setFlat( bool flat );

Q_SIGNALS:
    void scheduleManagerAdded(KPlato::ScheduleManager* );

protected Q_SLOTS:
    void slotManagerChanged(KPlato::ScheduleManager *sch);
    void slotScheduleChanged(KPlato::MainSchedule *sch);

    void slotScheduleManagerToBeInserted(const KPlato::ScheduleManager *manager, int row);
    void slotScheduleManagerInserted(const KPlato::ScheduleManager *manager);
    void slotScheduleManagerToBeRemoved(const KPlato::ScheduleManager *manager);
    void slotScheduleManagerRemoved(const KPlato::ScheduleManager *manager);
    void slotScheduleManagerToBeMoved(const KPlato::ScheduleManager *manager);
    void slotScheduleManagerMoved(const KPlato::ScheduleManager *manager, int index);
    void slotScheduleToBeInserted(const KPlato::ScheduleManager *manager, int row);
    void slotScheduleInserted( const KPlato::MainSchedule *schedule);
    void slotScheduleToBeRemoved( const KPlato::MainSchedule *schedule);
    void slotScheduleRemoved( const KPlato::MainSchedule *schedule);

protected:
    int row( const Schedule *sch ) const;
    
    QVariant name( const QModelIndex &index, int role ) const;
    bool setName( const QModelIndex &index, const QVariant &value, int role );
    
    QVariant state( const QModelIndex &index, int role ) const;
    bool setState( const QModelIndex &index, const QVariant &value, int role );

    QVariant allowOverbooking( const QModelIndex &index, int role ) const;
    bool setAllowOverbooking( const QModelIndex &index, const QVariant &value, int role );
    
    QVariant usePert( const QModelIndex &index, int role ) const;
    bool setUsePert( const QModelIndex &index, const QVariant &value, int role );

    QVariant projectStart( const QModelIndex &index, int role ) const;
    QVariant projectEnd( const QModelIndex &index, int role ) const;

    QVariant schedulingDirection( const QModelIndex &index, int role ) const;
    bool setSchedulingDirection( const QModelIndex &index, const QVariant &value, int role );

    QVariant schedulingStartTime( const QModelIndex &index, int role ) const;
    bool setSchedulingStartTime( const QModelIndex &index, const QVariant &value, int role );

    QVariant scheduler( const QModelIndex &index, int role ) const;
    bool setScheduler( const QModelIndex &index, const QVariant &value, int role );

    QVariant isScheduled( const QModelIndex &index, int role ) const;

    QVariant granularity( const QModelIndex &index, int role ) const;
    bool setGranularity( const QModelIndex &index, const QVariant &value, int role );

    QVariant schedulingMode( const QModelIndex &index, int role ) const;
    bool setSchedulingMode( const QModelIndex &index, const QVariant &value, int role );

private:
    ScheduleManager *m_manager; // for sanity check
    bool m_flat;
    ScheduleModel m_model;

    QList<ScheduleManager*> m_managerlist;
    
};

//----------------------------------------
class PLANMODELS_EXPORT ScheduleSortFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit ScheduleSortFilterModel( QObject *parent = 0 );
    ~ScheduleSortFilterModel() override;

    ScheduleManager *manager( const QModelIndex &index ) const;

    QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const override;
};

//----------------------------------------
class PLANMODELS_EXPORT ScheduleLogItemModel : public QStandardItemModel
{
    Q_OBJECT
public:
    enum DataRoles { SeverityRole = Qt::UserRole + 1, IdentityRole };

    explicit ScheduleLogItemModel( QObject *parent = 0 );
    ~ScheduleLogItemModel() override;

    void setProject( Project *project );
    Project *project() const { return m_project; }
    void setManager( ScheduleManager *manager );
    ScheduleManager *manager() const { return m_manager; }

    Qt::ItemFlags flags( const QModelIndex & index ) const override;

    void refresh();
    
    QString identity( const QModelIndex &idx ) const;

protected Q_SLOTS:
    void slotManagerChanged(KPlato::ScheduleManager *sch);
    void slotScheduleChanged(KPlato::MainSchedule *sch);

    void slotScheduleManagerToBeRemoved(const KPlato::ScheduleManager *manager);
    void slotScheduleManagerRemoved(const KPlato::ScheduleManager *manager);
    void slotScheduleToBeInserted(const KPlato::ScheduleManager *manager, int row);
    void slotScheduleInserted( const KPlato::MainSchedule *schedule);
    void slotScheduleToBeRemoved( const KPlato::MainSchedule *schedule);
    void slotScheduleRemoved( const KPlato::MainSchedule *schedule);

    void slotLogInserted(KPlato::MainSchedule*, int firstrow, int lastrow );

    void projectDeleted();

protected:
    void addLogEntry( const Schedule::Log &log, int row );

private:
    Project *m_project;
    ScheduleManager *m_manager;
    MainSchedule *m_schedule;

};


}  //KPlato namespace

#endif
