/* This file is part of the KDE project
 * Copyright (C) 2019 Dag Andersen <danders@get2net.dk>
 * Copyright (C) 2007 - 2010 Dag Andersen <danders@get2net.dk>
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

#ifndef TASKSTATUSVIEW_H
#define TASKSTATUSVIEW_H

#include "planui_export.h"

#include "kptitemmodelbase.h"

#include "kptviewbase.h"
#include "ui_kpttaskstatusviewsettingspanel.h"
#include "kptitemviewsettup.h"
#include "kptnodechartmodel.h"

#include <QSplitter>

#include <KChartBarDiagram>


class QItemSelection;

class KoDocument;
class KoPageLayoutWidget;
class PrintingHeaderFooter;

namespace KChart
{
    class CartesianCoordinatePlane;
    class CartesianAxis;
    class Legend;
};

namespace KPlato
{

class Project;
class Node;
class ScheduleManager;
class TaskStatusItemModel;
class NodeItemModel;
class PerformanceStatusBase;

typedef QList<Node*> NodeList;

class PLANUI_EXPORT TaskStatusTreeView : public DoubleTreeViewBase
{
    Q_OBJECT
public:
    explicit TaskStatusTreeView(QWidget *parent);

    //void setSelectionModel( QItemSelectionModel *selectionModel );

    TaskStatusItemModel *model() const;

    Project *project() const;
    void setProject( Project *project );

    int defaultWeekday() const { return Qt::Friday; }
    int weekday() const;
    void setWeekday( int day );

    int defaultPeriod() const { return 7; }
    int period() const;
    void setPeriod( int days );

    int defaultPeriodType() const;
    int periodType() const;
    void setPeriodType( int type );
};


class PLANUI_EXPORT TaskStatusView : public ViewBase
{
    Q_OBJECT
public:
    TaskStatusView(KoPart *part, KoDocument *doc, QWidget *parent);
    
    void setupGui();
    virtual void setProject( Project *project );
    Project *project() const { return m_view->project(); }
    using ViewBase::draw;
    virtual void draw( Project &project );

    TaskStatusItemModel *model() const { return m_view->model(); }
    
    virtual void updateReadWrite( bool readwrite );
    virtual Node *currentNode() const;
    
    /// Loads context info into this view. Reimplement.
    virtual bool loadContext( const KoXmlElement &/*context*/ );
    /// Save context info from this view. Reimplement.
    virtual void saveContext( QDomElement &/*context*/ ) const;

    KoPrintJob *createPrintJob();
    
Q_SIGNALS:
    void openNode();

public Q_SLOTS:
    /// Activate/deactivate the gui
    virtual void setGuiActive( bool activate );

    void setScheduleManager(KPlato::ScheduleManager *sm);

    virtual void slotRefreshView();

protected Q_SLOTS:
    virtual void slotOptions();

protected:
    void updateActionsEnabled( bool on );

private Q_SLOTS:
    void slotContextMenuRequested( const QModelIndex &index, const QPoint& pos );
    void slotContextMenuRequested(KPlato::Node *node, const QPoint& pos);
    void slotSplitView();
    
private:
    Project *m_project;
    int m_id;
    TaskStatusTreeView *m_view;
    QDomDocument m_domdoc;

};

//--------------------------------------
class TaskStatusViewSettingsPanel : public QWidget, public Ui::TaskStatusViewSettingsPanel
{
    Q_OBJECT
public:
    explicit TaskStatusViewSettingsPanel( TaskStatusTreeView *view, QWidget *parent = 0 );

public Q_SLOTS:
    void slotOk();
    void setDefault();

Q_SIGNALS:
    void changed();

private:
    TaskStatusTreeView *m_view;
};

class TaskStatusViewSettingsDialog : public SplitItemViewSettupDialog
{
    Q_OBJECT
public:
    explicit TaskStatusViewSettingsDialog( ViewBase *view, TaskStatusTreeView *treeview, QWidget *parent = 0 );

};

} //namespace KPlato

#endif
