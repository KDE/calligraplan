/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2007-2010 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
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
}

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

    void slotOpenCurrentNode();
    void slotOpenNode(KPlato::Node *node);
    void slotTaskProgress();
    void slotOpenProjectDescription();
    void slotTaskDescription();
    void slotOpenTaskDescription(bool);
    void slotDocuments();

    void slotTaskEditFinished(int result);
    void slotSummaryTaskEditFinished(int result);
    void slotTaskProgressFinished(int result);
    void slotMilestoneProgressFinished(int result);
    void slotTaskDescriptionFinished(int result);
    void slotDocumentsFinished(int result);

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
