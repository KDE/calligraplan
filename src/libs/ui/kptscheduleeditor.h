/* This file is part of the KDE project
  Copyright (C) 2006-2008 Dag Andersen <danders@get2net.dk>
  Copyright (C) 2006-2007 Menard Alexis <danders@get2net>
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

#ifndef KPTSCHEDULEEDITOR_H
#define KPTSCHEDULEEDITOR_H

#include "planui_export.h"

#include <kptviewbase.h>
#include "kptsplitterview.h"
#include "kptschedulemodel.h"
#include "ui_kptscheduleeditor.h"

#include <KoXmlReaderForward.h>

class KoDocument;

class KToggleAction;

class QPoint;
class QSortFilterProxyModel;

namespace KPlato
{

class Project;
class ScheduleManager;
class SchedulingRange;

class PLANUI_EXPORT ScheduleTreeView : public TreeViewBase
{
    Q_OBJECT
public:
    explicit ScheduleTreeView(QWidget *parent);

    ScheduleItemModel *model() const { return static_cast<ScheduleItemModel*>( TreeViewBase::model() ); }

    Project *project() const { return model()->project(); }
    void setProject( Project *project ) { model()->setProject( project ); }

    ScheduleManager *manager( const QModelIndex &idx ) const;
    ScheduleManager *currentManager() const;
    ScheduleManager *selectedManager() const;

    QModelIndexList selectedRows() const;

Q_SIGNALS:
    void currentChanged( const QModelIndex& );
    void currentColumnChanged( const QModelIndex&, const QModelIndex& );
    void selectionChanged( const QModelIndexList& );

protected Q_SLOTS:
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;
    void currentChanged ( const QModelIndex & current, const QModelIndex & previous ) override;
    
};

class PLANUI_EXPORT ScheduleEditor : public ViewBase
{
    Q_OBJECT
public:
    ScheduleEditor(KoPart *part, KoDocument *doc, QWidget *parent);
    
    void setupGui();
    Project *project() const override { return m_view->project(); }
    void draw( Project &project ) override;
    void draw() override;
    
    ScheduleItemModel *model() const { return m_view->model(); }
    
    void updateReadWrite( bool readwrite ) override;

    /// Loads context info into this view. Reimplement.
    bool loadContext( const KoXmlElement &/*context*/ ) override;
    /// Save context info from this view. Reimplement.
    void saveContext( QDomElement &/*context*/ ) const override;
    
    KoPrintJob *createPrintJob() override;
    
Q_SIGNALS:
    void calculateSchedule(KPlato::Project*, KPlato::ScheduleManager*);
    void baselineSchedule(KPlato::Project*, KPlato::ScheduleManager*);
    void addScheduleManager(KPlato::Project*);
    void deleteScheduleManager(KPlato::Project*, KPlato::ScheduleManager*);
    void SelectionScheduleChanged();

    /**
     * Emitted when schedule selection changes.
     * @param sm is the new schedule manager. If @p is 0, no schedule is selected.
    */
    void scheduleSelectionChanged(KPlato::ScheduleManager *sm);
    
    void moveScheduleManager(KPlato::ScheduleManager *sm, KPlato::ScheduleManager *newparent, int index);

public Q_SLOTS:
    /// Activate/deactivate the gui
    void setGuiActive( bool activate ) override;

protected Q_SLOTS:
    void slotOptions() override;

private Q_SLOTS:
    void slotContextMenuRequested( const QModelIndex &index, const QPoint& pos );

    void slotSelectionChanged( const QModelIndexList& );
    void slotCurrentChanged( const QModelIndex& );
    void updateActionsEnabled( const QModelIndex &index );
    void slotEnableActions();

    void slotCalculateSchedule();
    void slotBaselineSchedule();
    void slotAddSchedule();
    void slotAddSubSchedule();
    void slotDeleteSelection();
    void slotMoveLeft();

private:
    ScheduleTreeView *m_view;
    SchedulingRange *m_schedulingRange;

    QAction *actionCalculateSchedule;
    QAction *actionBaselineSchedule;
    QAction *actionAddSchedule;
    QAction *actionAddSubSchedule;
    QAction *actionDeleteSelection;
    QAction *actionMoveLeft;
};


//-----------------------------
class PLANUI_EXPORT ScheduleLogTreeView : public QTreeView
{
    Q_OBJECT
public:
    explicit ScheduleLogTreeView(QWidget *parent);

    Project *project() const { return logModel()->project(); }
    void setProject( Project *project ) { logModel()->setProject( project ); }

    ScheduleLogItemModel *logModel() const { return static_cast<ScheduleLogItemModel*>( m_model->sourceModel() ); }
    
    ScheduleManager *scheduleManager() const { return logModel()->manager(); }
    void setScheduleManager( ScheduleManager *manager ) { logModel()->setManager( manager ); }

    void setFilterWildcard( const QString &filter );
    QRegExp filterRegExp() const;

Q_SIGNALS:
    void currentChanged( const QModelIndex& );
    void currentColumnChanged( const QModelIndex&, const QModelIndex& );
    void selectionChanged( const QModelIndexList& );

    void contextMenuRequested( const QModelIndex&, const QPoint& );

public Q_SLOTS:
    void slotEditCopy();

protected Q_SLOTS:
    void contextMenuEvent ( QContextMenuEvent *e ) override;
    void headerContextMenuRequested( const QPoint &pos );
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;
    void currentChanged ( const QModelIndex & current, const QModelIndex & previous ) override;

    void slotShowDebug( bool );

private:
    QSortFilterProxyModel *m_model;
    KToggleAction *actionShowDebug;
};

//----------------------------------------------
class PLANUI_EXPORT ScheduleLogView : public ViewBase
{
    Q_OBJECT
public:
    ScheduleLogView(KoPart *part, KoDocument *doc, QWidget *parent);

    void setupGui();
    void setProject( Project *project ) override;
    Project *project() const override { return m_view->project(); }
    using ViewBase::draw;
    void draw( Project &project ) override;

    ScheduleLogItemModel *baseModel() const { return m_view->logModel(); }

    void updateReadWrite( bool readwrite ) override;

    /// Loads context info into this view.
    bool loadContext( const KoXmlElement &/*context*/ ) override;
    /// Save context info from this view.
    void saveContext( QDomElement &/*context*/ ) const override;

Q_SIGNALS:
    void editNode(KPlato::Node *node);
    void editResource(KPlato::Resource *resource);

public Q_SLOTS:
    /// Activate/deactivate the gui
    void setGuiActive( bool activate ) override;
    void slotEditCopy() override;
    void slotEdit();

protected Q_SLOTS:
    void slotOptions() override;

private Q_SLOTS:
    void slotContextMenuRequested( const QModelIndex &index, const QPoint& pos );
    void slotScheduleSelectionChanged(KPlato::ScheduleManager *sm);
    void slotEnableActions(const KPlato::ScheduleManager *sm);

    void slotSelectionChanged( const QModelIndexList& );
    void slotCurrentChanged( const QModelIndex& );
    void updateActionsEnabled( const QModelIndex &index );

private:
    ScheduleLogTreeView *m_view;
};


//-----------------------------
class PLANUI_EXPORT ScheduleHandlerView : public SplitterView
{
    Q_OBJECT
public:
    ScheduleHandlerView(KoPart *part, KoDocument *doc, QWidget *parent);
    
    Project *project() const override { return 0; }

    ScheduleEditor *scheduleEditor() const { return m_scheduleEditor; }
    /// Always returns this (if we are called, we are hit)
    virtual ViewBase *hitView( const QPoint &glpos );

Q_SIGNALS:
    void currentScheduleManagerChanged(KPlato::ScheduleManager*);
    void editNode(KPlato::Node *node);
    void editResource(KPlato::Resource *resource);

public Q_SLOTS:
    /// Activate/deactivate the gui (also of subviews)
    void setGuiActive( bool activate ) override;

protected Q_SLOTS:
    /// Noop, we handle subviews ourselves
    void slotGuiActivated(KPlato::ViewBase *v, bool active ) override;
    void currentTabChanged( int i ) override;

private:
    ScheduleEditor *m_scheduleEditor;
};

class SchedulingRange : public QWidget, public Ui::SchedulingRange
{
    Q_OBJECT
public:
    SchedulingRange(KoDocument *doc, QWidget *parent = 0);
 
    void setReadWrite(bool rw);

public Q_SLOTS:
    void setProject(KPlato::Project *project);
    void slotProjectChanged(KPlato::Node*);
    void slotStartChanged();
    void slotEndChanged();

protected:
    KoDocument *m_doc;
    Project *m_project;
};

}  //KPlato namespace

#endif
