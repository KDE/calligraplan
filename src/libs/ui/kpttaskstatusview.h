/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2007-2010 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
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
}

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

    //void setSelectionModel(QItemSelectionModel *selectionModel);

    TaskStatusItemModel *model() const;

    Project *project() const;
    void setProject(Project *project);

    int defaultWeekday() const { return Qt::Friday; }
    int weekday() const;
    void setWeekday(int day);

    int defaultPeriod() const { return 7; }
    int period() const;
    void setPeriod(int days);

    int defaultPeriodType() const;
    int periodType() const;
    void setPeriodType(int type);

protected Q_SLOTS:
    void slotStateChanged(Node *node);
    void slotCurrentItemChanged(const QModelIndex &idx);
};


class PLANUI_EXPORT TaskStatusView : public ViewBase
{
    Q_OBJECT
public:
    TaskStatusView(KoPart *part, KoDocument *doc, QWidget *parent);

    void setupGui();
    /// Use the document @p doc for command execution
    void setCommandDocument(KoDocument *doc);
    void setProject(Project *project) override;
    Project *project() const override { return m_view->project(); }
    using ViewBase::draw;
    void draw(Project &project) override;

    TaskStatusItemModel *model() const { return m_view->model(); }
    
    void updateReadWrite(bool readwrite) override;
    Node *currentNode() const override;
    
    /// Loads context info into this view. Reimplement.
    bool loadContext(const KoXmlElement &/*context*/) override;
    /// Save context info from this view. Reimplement.
    void saveContext(QDomElement &/*context*/) const override;

    KoPrintJob *createPrintJob() override;
    
Q_SIGNALS:
    void openNode();

public Q_SLOTS:
    /// Activate/deactivate the gui
    void setGuiActive(bool activate) override;

    void setScheduleManager(KPlato::ScheduleManager *sm) override;

    void slotRefreshView() override;

protected Q_SLOTS:
    void slotOptions() override;
    void itemDoubleClicked(const QPersistentModelIndex &idx);

protected:
    void updateActionsEnabled(bool on);

private Q_SLOTS:
    void slotContextMenuRequested(const QModelIndex &index, const QPoint& pos);
    void slotContextMenuRequested(KPlato::Node *node, const QPoint& pos);
    void slotSplitView();

    void slotTaskProgress();
    void slotOpenProjectDescription();
    void slotTaskDescription();
    void slotOpenTaskDescription(bool);
    void slotDocuments();

    void slotTaskProgressFinished(int result);
    void slotMilestoneProgressFinished(int result);
    void slotTaskDescriptionFinished(int result);
    void slotDocumentsFinished(int result);

    void slotSelectionChanged();

private:
    Project *m_project;
    int m_id;
    TaskStatusTreeView *m_view;
    QDomDocument m_domdoc;
    KoDocument *m_commandDocument;

};

//--------------------------------------
class TaskStatusViewSettingsPanel : public QWidget, public Ui::TaskStatusViewSettingsPanel
{
    Q_OBJECT
public:
    explicit TaskStatusViewSettingsPanel(TaskStatusTreeView *view, QWidget *parent = nullptr);

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
    explicit TaskStatusViewSettingsDialog(ViewBase *view, TaskStatusTreeView *treeview, QWidget *parent = nullptr);

};

} //namespace KPlato

#endif
