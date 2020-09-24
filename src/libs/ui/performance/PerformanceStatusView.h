/* This file is part of the KDE project
 * Copyright (C) 2019 Dag Andersen <dag.andersen@kdemail.net>
 * Copyright (C) 2007 - 2010 Dag Andersen <dag.andersen@kdemail.net>
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

#ifndef PERFORMANCESTATUSVIEW_H
#define PERFORMANCESTATUSVIEW_H

#include "planui_export.h"

#include "PerformanceStatusBase.h"

#include "kptitemmodelbase.h"

#include "kptviewbase.h"
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


class PerformanceStatusTreeView : public QSplitter
{
    Q_OBJECT
public:
    explicit PerformanceStatusTreeView(QWidget *parent);

    NodeItemModel *nodeModel() const;
    Project *project() const;
    void setProject(Project *project);
    void setScheduleManager(ScheduleManager *sm);

    /// Loads context info into this view.
    virtual bool loadContext(const KoXmlElement &context);
    /// Save context info from this view.
    virtual void saveContext(QDomElement &context) const;

    TreeViewBase *treeView() const { return m_tree; }
    PerformanceStatusBase *chartView() const { return m_chart; }
    
    KoPrintJob *createPrintJob(ViewBase *view);

    void editCopy();

protected Q_SLOTS:
    void slotSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected);
    void resizeSplitters();

private:
    TreeViewBase *m_tree;
    PerformanceStatusBase *m_chart;
    ScheduleManager *m_manager;
    QDomDocument m_domdoc;
};


//----------------------------------
class PLANUI_EXPORT PerformanceStatusView : public ViewBase
{
    Q_OBJECT
public:
    PerformanceStatusView(KoPart *part, KoDocument *doc, QWidget *parent);

    void setupGui();
    Project *project() const override { return m_view->project(); }
    void setProject(Project *project) override;

    /// Loads context info into this view. Reimplement.
    bool loadContext(const KoXmlElement &context) override;
    /// Save context info from this view. Reimplement.
    void saveContext(QDomElement &context) const override;

    Node *currentNode() const override;

    KoPrintJob *createPrintJob() override;

public Q_SLOTS:
    /// Activate/deactivate the gui
    void setGuiActive(bool activate) override;

    void setScheduleManager(KPlato::ScheduleManager *sm) override;

    void slotEditCopy() override;

protected Q_SLOTS:
    void slotOptions() override;

protected:
    void updateActionsEnabled(bool on);
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private Q_SLOTS:
    void slotContextMenuRequested(KPlato::Node *node, const QPoint& pos);
    void slotContextMenuRequested(const QModelIndex &index, const QPoint& pos);
    void slotChartContextMenuRequested(const QPoint& pos);
    void slotTableContextMenuRequested(const QPoint& pos);

private:
    PerformanceStatusTreeView *m_view;
    QPoint m_dragStartPosition;
};


class PerformanceStatusViewSettingsDialog : public ItemViewSettupDialog
{
    Q_OBJECT
public:
    explicit PerformanceStatusViewSettingsDialog(PerformanceStatusView *view, PerformanceStatusTreeView *treeview, QWidget *parent = nullptr, bool selectPrint = false);

};


} //namespace KPlato


#endif
