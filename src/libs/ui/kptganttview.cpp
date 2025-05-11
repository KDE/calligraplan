/* This file is part of the KDE project
 *  SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 *  SPDX-FileCopyrightText: 2005 Dag Andersen <dag.andersen@kdemail.net>
 *  SPDX-FileCopyrightText: 2006 Raphael Langerhorst <raphael.langerhorst@kdemail.net>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "kptganttview.h"
#include "FilterWidget.h"
#include "GanttViewBase.h"
#include "NodeGanttViewBase.h"
#include "DateTimeTimeLine.h"
#include "DateTimeGrid.h"
#include "kptnodeitemmodel.h"
#include "kptnode.h"
#include "kptproject.h"
#include "kpttask.h"
#include "Resource.h"
#include "kptrelation.h"
#include "kptschedule.h"
#include "kptitemviewsettup.h"
#include "kptduration.h"
#include "kptdatetime.h"
#include "kptganttitemdelegate.h"
#include "config.h"
#include "kpttaskdialog.h"
#include "kptsummarytaskdialog.h"
#include "kpttaskdescriptiondialog.h"
#include "kpttaskprogressdialog.h"
#include "kptmilestoneprogressdialog.h"
#include "kptdocumentsdialog.h"
#include "MacroCommand.h"
#include "GanttFilterOptionsWidget.h"
#include "kptdebug.h"

#include <KGanttGraphicsView>
#include <KGanttTreeViewRowController>

#include <KoDocument.h>
#include <KoXmlReader.h>
#include <KoPageLayoutWidget.h>
#include <KoIcon.h>

#include <QAction>
#include <QMenu>
#include <QPushButton>
#include <QLineEdit>
#include <QActionGroup>

#include <KToggleAction>
#include <KActionCollection>

using namespace KPlato;

class GanttItemDelegate;

GanttChartDisplayOptionsPanel::GanttChartDisplayOptionsPanel(GanttViewBase *gantt, GanttItemDelegate *delegate, QWidget *parent)
    : QWidget(parent)
    , m_delegate(delegate)
    , m_gantt(gantt)
{
    setupUi(this);
    setValues(*delegate);

    connect(ui_showTaskName, &QCheckBox::stateChanged, this, &GanttChartDisplayOptionsPanel::changed);
    connect(ui_showResourceNames, &QCheckBox::stateChanged, this, &GanttChartDisplayOptionsPanel::changed);
    connect(ui_showDependencies, &QCheckBox::stateChanged, this, &GanttChartDisplayOptionsPanel::changed);
    connect(ui_showPositiveFloat, &QCheckBox::stateChanged, this, &GanttChartDisplayOptionsPanel::changed);
    connect(ui_showNegativeFloat, &QCheckBox::stateChanged, this, &GanttChartDisplayOptionsPanel::changed);
    connect(ui_showCriticalPath, &QCheckBox::stateChanged, this, &GanttChartDisplayOptionsPanel::changed);
    connect(ui_showCriticalTasks, &QCheckBox::stateChanged, this, &GanttChartDisplayOptionsPanel::changed);
    connect(ui_showCompletion, &QCheckBox::stateChanged, this, &GanttChartDisplayOptionsPanel::changed);
    connect(ui_showSchedulingError, &QCheckBox::stateChanged, this, &GanttChartDisplayOptionsPanel::changed);
    connect(ui_showTimeConstraint, &QCheckBox::stateChanged, this, &GanttChartDisplayOptionsPanel::changed);

}

void GanttChartDisplayOptionsPanel::slotOk()
{
    m_delegate->showTaskName = ui_showTaskName->checkState() == Qt::Checked;
    m_delegate->showResources = ui_showResourceNames->checkState() == Qt::Checked;
    m_delegate->showTaskLinks = ui_showDependencies->checkState() == Qt::Checked;
    m_delegate->showPositiveFloat = ui_showPositiveFloat->checkState() == Qt::Checked;
    m_delegate->showNegativeFloat = ui_showNegativeFloat->checkState() == Qt::Checked;
    m_delegate->showCriticalPath = ui_showCriticalPath->checkState() == Qt::Checked;
    m_delegate->showCriticalTasks = ui_showCriticalTasks->checkState() == Qt::Checked;
    m_delegate->showProgress = ui_showCompletion->checkState() == Qt::Checked;
    m_delegate->showSchedulingError = ui_showSchedulingError->checkState() == Qt::Checked;
    m_delegate->showTimeConstraint = ui_showTimeConstraint->checkState() == Qt::Checked;

    DateTimeTimeLine *timeline = m_gantt->timeLine();

    timeline->setInterval(ui_timeLineInterval->value() * 60000);
    QPen pen;
    pen.setWidth(ui_timeLineStroke->value());
    pen.setColor(ui_timeLineColor->color());
    timeline->setPen(pen);

    DateTimeTimeLine::Options opt = timeline->options();
    opt.setFlag(DateTimeTimeLine::Foreground, ui_timeLineForeground->isChecked());
    opt.setFlag(DateTimeTimeLine::Background, ui_timeLineBackground->isChecked());
    opt.setFlag(DateTimeTimeLine::UseCustomPen, ui_timeLineUseCustom->isChecked());
    timeline->setOptions(opt);

    m_gantt->setShowRowSeparators(ui_showRowSeparators->checkState() == Qt::Checked);
}

void GanttChartDisplayOptionsPanel::setValues(const GanttItemDelegate &del)
{
    ui_showTaskName->setCheckState(del.showTaskName ? Qt::Checked : Qt::Unchecked);
    ui_showResourceNames->setCheckState(del.showResources ? Qt::Checked : Qt::Unchecked);
    ui_showDependencies->setCheckState(del.showTaskLinks ? Qt::Checked : Qt::Unchecked);
    ui_showPositiveFloat->setCheckState(del.showPositiveFloat ? Qt::Checked : Qt::Unchecked);
    ui_showNegativeFloat->setCheckState(del.showNegativeFloat ? Qt::Checked : Qt::Unchecked);
    ui_showCriticalPath->setCheckState(del.showCriticalPath ? Qt::Checked : Qt::Unchecked);
    ui_showCriticalTasks->setCheckState(del.showCriticalTasks ? Qt::Checked : Qt::Unchecked);
    ui_showCompletion->setCheckState(del.showProgress ? Qt::Checked : Qt::Unchecked);
    ui_showSchedulingError->setCheckState(del.showSchedulingError ? Qt::Checked : Qt::Unchecked);
    ui_showTimeConstraint->setCheckState(del.showTimeConstraint ? Qt::Checked : Qt::Unchecked);

    DateTimeTimeLine *timeline = m_gantt->timeLine();

    ui_timeLineInterval->setValue(timeline->interval() / 60000);

    QPen pen = timeline->pen();
    ui_timeLineStroke->setValue(pen.width());
    ui_timeLineColor->setColor(pen.color());

    ui_timeLineHide->setChecked(true);
    DateTimeTimeLine::Options opt = timeline->options();
    ui_timeLineBackground->setChecked(opt & DateTimeTimeLine::Background);
    ui_timeLineForeground->setChecked(opt & DateTimeTimeLine::Foreground);
    ui_timeLineUseCustom->setChecked(opt & DateTimeTimeLine::UseCustomPen);

    ui_showRowSeparators->setChecked(m_gantt->showRowSeparators());

    freedays->disconnect();
    freedays->clear();
    freedays->addItem(i18nc("@item:inlistbox", "No freedays"));
    freedays->addItem(i18nc("@item:inlistbox", "Project freedays"));
    int currentIndex = m_gantt->freedaysType();
    const auto project = m_gantt->project();
    if (project) {
        auto current = m_gantt->calendar();
        const auto calendars = project->calendars();
        for (auto *c : calendars) {
            freedays->addItem(c->name(), c->id());
            if (c == current) {
                currentIndex = freedays->count() - 1;
            }
        }
        freedays->setCurrentIndex(currentIndex);
        connect(freedays, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int idx) {
            auto box = qobject_cast<QComboBox*>(sender());
            m_gantt->setCalendar(idx, m_gantt->project()->findCalendar(box->currentData().toString()));
        });
    }
}

void GanttChartDisplayOptionsPanel::setDefault()
{
    GanttItemDelegate del;
    setValues(del);
}

GanttViewSettingsDialog::GanttViewSettingsDialog(GanttViewBase *gantt, GanttItemDelegate *delegate, ViewBase *view, bool selectPrint)
    : ItemViewSettupDialog(view, gantt->treeView(), true, view)
    , m_gantt(gantt)
{
    setFaceType(KPageDialog::Auto);
    GanttChartDisplayOptionsPanel *panel = new GanttChartDisplayOptionsPanel(gantt, delegate);
    /*KPageWidgetItem *page = */insertWidget(1, panel, i18n("Chart"), i18n("Gantt Chart Settings"));
    createPrintingOptions(selectPrint);
    connect(this, SIGNAL(accepted()), this, SLOT(slotOk()));
    connect(this, &QDialog::accepted, panel, &GanttChartDisplayOptionsPanel::slotOk);
    connect(button(QDialogButtonBox::RestoreDefaults), &QAbstractButton::clicked, panel, &GanttChartDisplayOptionsPanel::setDefault);
}

void GanttViewSettingsDialog::createPrintingOptions(bool setAsCurrent)
{
    if (! m_view) {
        return;
    }
    QTabWidget *tab = new QTabWidget();
    QWidget *w = ViewBase::createPageLayoutWidget(m_view);
    tab->addTab(w, w->windowTitle());
    m_pagelayout = w->findChild<KoPageLayoutWidget*>();
    Q_ASSERT(m_pagelayout);

    m_printingOptions = new GanttPrintingOptionsWidget(m_gantt, this);
    tab->addTab(m_printingOptions, i18n("Chart"));

    KPageWidgetItem *itm = insertWidget(-1, tab, i18n("Printing"), i18n("Printing Options"));
    if (setAsCurrent) {
        setCurrentPage(itm);
    }
}

void GanttViewSettingsDialog::slotOk()
{
    debugPlan;
    ItemViewSettupDialog::slotOk();
    m_gantt->setPrintingOptions(m_printingOptions->options());
}

GanttView::GanttView(KoPart *part, KoDocument *doc, QWidget *parent, bool readWrite)
    : ViewBase(part, doc, parent)
    , m_readWrite(readWrite)
{
    debugPlan <<" ---------------- KPlato: Creating GanttView ----------------";

    setXMLFile(QStringLiteral("GanttViewUi.rc"));

    m_gantt = new MyKGanttView(this);
    m_gantt->graphicsView()->setHeaderContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &ViewBase::expandAll, m_gantt->treeView(), &TreeViewBase::slotExpand);
    connect(this, &ViewBase::collapseAll, m_gantt->treeView(), &TreeViewBase::slotCollapse);

    m_filterOptions = new GanttFilterOptionsWidget(m_gantt->sfModel(), this);
    m_filterOptions->setMaximumWidth(m_filterOptions->sizeHint().width());
    m_filterOptions->hide();

    auto l = new QHBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    l->addWidget(m_filterOptions);
    l->addWidget(m_gantt);

    setupGui();

    updateReadWrite(readWrite);
    //connect(m_gantt->constraintModel(), SIGNAL(constraintAdded(Constraint)), this, SLOT(update()));
    debugPlan <<m_gantt->constraintModel();

    connect(m_gantt->treeView(), &TreeViewBase::contextMenuRequested, this, &GanttView::slotContextMenuRequested);

    connect(m_gantt->treeView(), &TreeViewBase::headerContextMenuRequested, this, &ViewBase::slotHeaderContextMenuRequested);

    connect(m_gantt->graphicsView(), &KGantt::GraphicsView::headerContextMenuRequested, this, &GanttView::slotGanttHeaderContextMenuRequested);

    connect(qobject_cast<KGantt::DateTimeGrid*>(m_gantt->graphicsView()->grid()), &KGantt::DateTimeGrid::gridChanged, this, &GanttView::slotDateTimeGridChanged);

    connect(m_gantt->leftView(), &GanttTreeView::doubleClicked, this, &GanttView::itemDoubleClicked);

    connect(m_gantt, &MyKGanttView::contextMenuRequested, this, &GanttView::slotContextMenuRequested);

    connect(m_gantt->selectionModel(), &QItemSelectionModel::selectionChanged, this, &GanttView::slotSelectionChanged);

    connect(m_gantt->model(), &ItemModelBase::executeCommand, doc, &KoDocument::addCommand);

    updateActionsEnabled(false);

    setWhatsThis(
              xi18nc("@info:whatsthis",
                     "<title>Gantt View</title>"
                     "<para>"
                     "Displays scheduled tasks in a Gantt diagram."
                     " The chart area can be zoomed in and out with a slider"
                     " positioned in the upper left corner of the time scale."
                     " <note>You need to hoover over it with the mouse for it to show.</note>"
                     "</para><para>"
                     "This view supports configuration and printing using the context menu of the tree view."
                     "<nl/><link url='%1'>More...</link>"
                     "</para>", QStringLiteral("plan:task-gantt-view")));
}

void GanttView::slotEditCopy()
{
    m_gantt->editCopy();
}

void GanttView::itemDoubleClicked(const QPersistentModelIndex &idx)
{
    if (idx.column() == NodeModel::NodeDescription) {
        Node *node =  m_gantt->model()->node(m_gantt->sfModel()->mapToSource(idx));
        if (node) {
            auto action = actionCollection()->action(QStringLiteral("task_description"));
            if (action) {
                action->trigger();
            }
        }
    }
}

void GanttView::slotGanttHeaderContextMenuRequested(const QPoint &pt)
{
    QMenu *menu = popupMenu(QStringLiteral("gantt_datetimegrid_popup"));
    if (menu) {
        menu->exec(pt);
    }
}

KoPrintJob *GanttView::createPrintJob()
{
    return new GanttPrintingDialog(this, m_gantt);
}

void GanttView::setZoom(double)
{
    //debugPlan <<"setting gantt zoom:" << zoom;
    //m_gantt->setZoomFactor(zoom,true); NO!!! setZoomFactor() is something else
}

void GanttView::setupGui()
{
    QAction *a;

    actionShowProject = new KToggleAction(i18n("Show Project"), this);
    actionCollection()->addAction(QStringLiteral("show_project"), actionShowProject);
    actionShowProject->setChecked(m_gantt->model()->projectShown());
    // FIXME: Dependencies depend on these methods being called in the correct order
    connect(actionShowProject, &QAction::toggled, m_gantt, &MyKGanttView::clearDependencies);
    connect(actionShowProject, &QAction::toggled, m_gantt->model(), &NodeItemModel::setShowProject);
    connect(actionShowProject, &QAction::toggled, m_gantt, &MyKGanttView::createDependencies);
    addContextAction(actionShowProject);
    m_filterOptions->addAction(actionShowProject);

    auto filterAction = new QAction(i18n("Filter Options..."));
    filterAction->setIcon(koIcon("view-filter"));
    filterAction->setObjectName(QStringLiteral("filter_options"));
    connect(filterAction, &QAction::triggered, this, &GanttView::slotFilterOptions);
    //addContextAction(filterAction);

    createOptionActions(ViewBase::OptionAll);
    const auto actionList = contextActionList();
    for (QAction *a : actionList) {
        actionCollection()->addAction(a->objectName(), a);
    }

    actionShowUnscheduled = new KToggleAction(i18n("Show Unscheduled Tasks"), this);
    actionCollection()->addAction(QStringLiteral("show_unscheduled_tasks"), actionShowUnscheduled);
    actionShowUnscheduled->setChecked(m_gantt->sfModel()->showUnscheduled());
    connect(actionShowUnscheduled, &QAction::toggled, m_gantt->sfModel(), &NodeSortFilterProxyModel::setShowUnscheduled);
    //addContextAction(actionShowUnscheduled);
    m_filterOptions->addAction(actionShowUnscheduled);

    auto showTasks = new QAction(i18n("Show Tasks"));
    actionCollection()->addAction(QStringLiteral("show_tasks"), showTasks);
    showTasks->setCheckable(true);
    showTasks->setChecked(m_gantt->sfModel()->showTasks());
    connect(showTasks, &QAction::toggled, m_gantt->sfModel(), &NodeSortFilterProxyModel::setShowTasks);
    //addContextAction(showTasks);
    m_filterOptions->addAction(showTasks);

    auto showMilestones = new QAction(i18n("Show Milestones"));
    actionCollection()->addAction(QStringLiteral("show_milestones"), showMilestones);
    showMilestones->setCheckable(true);
    showMilestones->setChecked(m_gantt->sfModel()->showMilestones());
    connect(showMilestones, &QAction::toggled, m_gantt->sfModel(), &NodeSortFilterProxyModel::setShowMilestones);
    //addContextAction(showMilestones);
    m_filterOptions->addAction(showMilestones);

    a = new QAction(i18n("Tasks and Milestones"));
    a->setToolTip(xi18nc("@info:tooltip", "Activates the Task and Milestones filter options."
                        "<nl/>Deactivating this will show all scheduled tasks and milestones."));
    actionCollection()->addAction(QStringLiteral("tasks_and_milestones_group"), a);
    a->setCheckable(true);
    a->setChecked(m_gantt->sfModel()->tasksAndMilestonesGroupEnabled());
    connect(a, &QAction::toggled, m_gantt->sfModel(), &NodeSortFilterProxyModel::setTasksAndMilestonesGroupEnabled);
    m_filterOptions->addAction(a);

    a = new QAction(i18n("Period"));
    a->setToolTip(i18nc("@info:tooltip", "Activates the Period filter options."));
    actionCollection()->addAction(QStringLiteral("period_group"), a);
    a->setCheckable(true);
    a->setChecked(m_gantt->sfModel()->periodGroupEnabled());
    connect(a, &QAction::toggled, m_gantt->sfModel(), &NodeSortFilterProxyModel::setPeriodGroupEnabled);
    m_filterOptions->addAction(a);

    a = new QAction(i18n("Show Running Tasks"));
    a->setToolTip(i18nc("@info:tooltip", "Shows tasks that has been started before or during this period."));
    actionCollection()->addAction(QStringLiteral("show_running"), a);
    a->setCheckable(true);
    a->setChecked(m_gantt->sfModel()->showRunning());
    connect(a, &QAction::toggled, m_gantt->sfModel(), &NodeSortFilterProxyModel::setShowRunning);
    m_filterOptions->addAction(a);

    a = new QAction(i18n("Show Finished Tasks"));
    a->setToolTip(i18nc("@info:tooltip", "Shows tasks and milestones that has been finished during this period."));
    actionCollection()->addAction(QStringLiteral("show_finished"), a);
    a->setCheckable(true);
    a->setChecked(m_gantt->sfModel()->showFinished());
    connect(a, &QAction::toggled, m_gantt->sfModel(), &NodeSortFilterProxyModel::setShowFinished);
    m_filterOptions->addAction(a);

    a = new QAction(i18n("Show Not Started Tasks"));
    a->setToolTip(i18nc("@info:tooltip", "Shows tasks and milestones that should have been started by now."));
    actionCollection()->addAction(QStringLiteral("show_not_started"), a);
    a->setCheckable(true);
    a->setChecked(m_gantt->sfModel()->showNotStarted());
    connect(a, &QAction::toggled, m_gantt->sfModel(), &NodeSortFilterProxyModel::setShowNotStarted);
    m_filterOptions->addAction(a);

    a = new QAction(i18n("Period:"));
    actionCollection()->addAction(QStringLiteral("enable_period"), a);
    a->setCheckable(true);
    m_filterOptions->addAction(a);

    a = new QAction(i18n("specific period:"));
    actionCollection()->addAction(QStringLiteral("enable_spesific_period"), a);
    a->setCheckable(true);
    m_filterOptions->addAction(a);

    m_scalegroup = new QActionGroup(this);
    a = new QAction(i18nc("@action:inmenu", "Auto"), this);
    a->setCheckable(true);
    a->setChecked(true);
    actionCollection()->addAction(QStringLiteral("scale_auto"), a);
    connect(a, &QAction::triggered, this, &GanttView::ganttActions);
    m_scalegroup->addAction(a);

    a = new QAction(i18nc("@action:inmenu", "Month"), this);
    actionCollection()->addAction(QStringLiteral("scale_month"), a);
    a->setCheckable(true);
    connect(a, &QAction::triggered, this, &GanttView::ganttActions);
    m_scalegroup->addAction(a);

    a = new QAction(i18nc("@action:inmenu", "Week"), this);
    actionCollection()->addAction(QStringLiteral("scale_week"), a);
    a->setCheckable(true);
    connect(a, &QAction::triggered, this, &GanttView::ganttActions);
    m_scalegroup->addAction(a);

    a = new QAction(i18nc("@action:inmenu", "Day"), this);
    a->setCheckable(true);
    actionCollection()->addAction(QStringLiteral("scale_day"), a);
    connect(a, &QAction::triggered, this, &GanttView::ganttActions);
    m_scalegroup->addAction(a);

    a = new QAction(i18nc("@action:inmenu", "Hour"), this);
    a->setCheckable(true);
    actionCollection()->addAction(QStringLiteral("scale_hour"), a);
    connect(a, &QAction::triggered, this, &GanttView::ganttActions);
    m_scalegroup->addAction(a);

    a = new QAction(i18nc("@action:inmenu", "Zoom In"), this);
    a->setIcon(koIcon("zoom-in"));
    actionCollection()->addAction(QStringLiteral("zoom_in"), a);
    connect(a, &QAction::triggered, this, &GanttView::ganttActions);

    a = new QAction(i18nc("@action:inmenu", "Zoom Out"), this);
    a->setIcon(koIcon("zoom-out"));
    actionCollection()->addAction(QStringLiteral("zoom_out"), a);
    connect(a, &QAction::triggered, this, &GanttView::ganttActions);

    auto actionOpenNode  = new QAction(koIcon("document-edit"), i18n("Edit..."), this);
    actionCollection()->addAction(QStringLiteral("node_properties"), actionOpenNode);
    connect(actionOpenNode, &QAction::triggered, this, &GanttView::slotOpenCurrentNode);

    auto actionTaskProgress  = new QAction(koIcon("document-edit"), i18n("Progress..."), this);
    actionCollection()->addAction(QStringLiteral("task_progress"), actionTaskProgress);
    connect(actionTaskProgress, &QAction::triggered, this, &GanttView::slotTaskProgress);

    auto actionTaskDescription  = new QAction(koIcon("document-edit"), i18n("Description..."), this);
    actionCollection()->addAction(QStringLiteral("task_description"), actionTaskDescription);
    connect(actionTaskDescription, &QAction::triggered, this, &GanttView::slotTaskDescription);

    auto actionDocuments  = new QAction(koIcon("document-edit"), i18n("Documents..."), this);
    actionCollection()->addAction(QStringLiteral("task_documents"), actionDocuments);
    connect(actionDocuments, &QAction::triggered, this, &GanttView::slotDocuments);

    // filter
    auto filter = new QWidgetAction(this);
    filter->setObjectName(QStringLiteral("edit_filter"));
    filter->setText(i18n("Filter"));
    auto filterWidget = new FilterWidget(this);
    filter->setDefaultWidget(filterWidget);
    actionCollection()->addAction(filter->objectName(), filter);
    connect(filterWidget->lineedit, &QLineEdit::textChanged, m_gantt->sfModel(), qOverload<const QString&>(&NodeSortFilterProxyModel::setFilterRegularExpression));
    connect(filterWidget->extendedOptions, &QToolButton::clicked, this, &GanttView::slotFilterOptions);
}

void GanttView::slotSelectionChanged()
{
    updateActionsEnabled(true);
}

void GanttView::updateActionsEnabled(bool on)
{
    const auto node  = currentNode();
    bool enable = on && node && (m_gantt->selectionModel()->selectedRows().count() < 2);

    const auto c = actionCollection();
    if (auto a = c->action(QStringLiteral("task_progress"))) { a->setEnabled(false); }
    if (auto a = c->action(QStringLiteral("task_description"))) { a->setEnabled(enable); }
    if (auto a = c->action(QStringLiteral("task_documents"))) { a->setEnabled(enable); }

    if (enable) {
        auto sid = scheduleManager() ? scheduleManager()->scheduleId() : -1;
        switch (node->type()) {
            case Node::Type_Task:
            case Node::Type_Milestone:
                if (auto a = c->action(QStringLiteral("task_progress"))) { a->setEnabled(enable && node->isScheduled(sid)); }
                break;
            case Node::Type_Summarytask:
                break;
            default:
                if (auto a = c->action(QStringLiteral("task_description"))) { a->setEnabled(false); }
                if (auto a = c->action(QStringLiteral("task_documents"))) { a->setEnabled(false); }
                break;
        }
    }
}

void GanttView::slotDateTimeGridChanged()
{
    DateTimeGrid *grid = qobject_cast<DateTimeGrid*>(m_gantt->grid());
    Q_ASSERT(grid);
    if (!grid) {
        return;
    }
    switch (grid->scale()) {
        case KGantt::DateTimeGrid::ScaleAuto: actionCollection()->action(QStringLiteral("scale_auto"))->setChecked(true); break;
        case KGantt::DateTimeGrid::ScaleHour: actionCollection()->action(QStringLiteral("scale_hour"))->setChecked(true); break;
        case KGantt::DateTimeGrid::ScaleDay: actionCollection()->action(QStringLiteral("scale_day"))->setChecked(true); break;
        case KGantt::DateTimeGrid::ScaleWeek: actionCollection()->action(QStringLiteral("scale_week"))->setChecked(true); break;
        case KGantt::DateTimeGrid::ScaleMonth: actionCollection()->action(QStringLiteral("scale_month"))->setChecked(true); break;
        default:
            warnPlan<<"Unused scale:"<<grid->scale();
            break;
    }
}

void GanttView::ganttActions()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (!action) {
        return;
    }
    DateTimeGrid *grid = qobject_cast<DateTimeGrid*>(m_gantt->grid());
    Q_ASSERT(grid);
    if (!grid) {
        return;
    }
    if (action->objectName() == QStringLiteral("scale_auto")) {
        grid->setScale(DateTimeGrid::ScaleAuto);
    } else if (action->objectName() == QStringLiteral("scale_month")) {
        grid->setScale(DateTimeGrid::ScaleMonth);
    } else if (action->objectName() == QStringLiteral("scale_week")) {
        grid->setScale(DateTimeGrid::ScaleWeek);
    } else if (action->objectName() == QStringLiteral("scale_day")) {
        grid->setScale(DateTimeGrid::ScaleDay);
    } else if (action->objectName() == QStringLiteral("scale_hour")) {
        grid->setScale(DateTimeGrid::ScaleHour);
    } else if (action->objectName() == QStringLiteral("zoom_in")) {
        grid->setDayWidth(grid->dayWidth() * 1.25);
    } else if (action->objectName() == QStringLiteral("zoom_out")) {
        // daywidth *MUST NOT* go below 1.0, it is used as an integer later on
        grid->setDayWidth(qMax<qreal>(1.0, grid->dayWidth() * 0.8));
    } else {
        warnPlan<<"Unknown gantt action:"<<action;
    }
}

void GanttView::slotOptions()
{
    debugPlan;
    GanttViewSettingsDialog *dlg = new GanttViewSettingsDialog(m_gantt, m_gantt->delegate(), this, sender()->objectName() == QStringLiteral("print_options"));
    connect(dlg, SIGNAL(finished(int)), SLOT(slotOptionsFinished(int)));
    dlg->open();
}

void GanttView::slotOptionsFinished(int result)
{
    GanttViewSettingsDialog *dlg = qobject_cast<GanttViewSettingsDialog*>(sender());
    if (dlg && result == QDialog::Accepted) {
        m_gantt->graphicsView()->updateScene();
    }
    ViewBase::slotOptionsFinished(result);
}

void GanttView::clear()
{
//    m_gantt->clear();
}

void GanttView::setShowSpecialInfo(bool on)
{
    m_gantt->model()->setShowSpecial(on);
}

bool GanttView::showSpecialInfo() const
{
    return m_gantt->model()->showSpecial();
}

void GanttView::setShowResources(bool on)
{
    m_gantt->delegate()->showResources = on;
}

void GanttView::setShowTaskName(bool on)
{
    m_gantt->delegate()->showTaskName = on;
}

void GanttView::setShowProgress(bool on)
{
    m_gantt->delegate()->showProgress = on;
}

void GanttView::setShowPositiveFloat(bool on)
{
    m_gantt->delegate()->showPositiveFloat = on;
}

void GanttView::setShowCriticalTasks(bool on)
{
    m_gantt->delegate()->showCriticalTasks = on;
}

void GanttView::setShowCriticalPath(bool on)
{
    m_gantt->delegate()->showCriticalPath = on;
}

void GanttView::setShowNoInformation(bool on)
{
    m_gantt->delegate()->showNoInformation = on;
}

void GanttView::setShowAppointments(bool on)
{
    m_gantt->delegate()->showAppointments = on;
}

void GanttView::slotFilterOptions()
{
    auto w = findChild<GanttFilterOptionsWidget*>();
    if (w->isVisible()) {
        w->hide();
    } else {
        w->show();
    }
}

void GanttView::setShowTaskLinks(bool on)
{
    m_gantt->delegate()->showTaskLinks = on;
}

void GanttView::setProject(Project *project)
{
    if (this->project()) {
        disconnect(project, &Project::scheduleManagerChanged, this, &GanttView::setScheduleManager);
    }
    m_gantt->setProject(project);
    ViewBase::setProject(project);
    if (project) {
        connect(project, &Project::scheduleManagerChanged, this, &GanttView::setScheduleManager);

    }
}

void GanttView::slotProjectCalculated(Project *project, ScheduleManager *sm)
{
    if (project == this->project() && sm == scheduleManager()) {
        // refresh view
        setScheduleManager(nullptr);
        setScheduleManager(sm);
    }
}

void GanttView::setScheduleManager(ScheduleManager *sm)
{
    if (!sm && scheduleManager()) {
        // we should only get here if the only schedule manager is scheduled,
        // or when last schedule manager is deleted
        m_domdoc.clear();
        QDomElement element = m_domdoc.createElement(QStringLiteral("expanded"));
        m_domdoc.appendChild(element);
        m_gantt->treeView()->saveExpanded(element);
    }
    bool tryexpand = sm && !scheduleManager();
    bool expand = sm && scheduleManager() && sm != scheduleManager();
    QDomDocument doc;
    if (expand) {
        QDomElement element = doc.createElement(QStringLiteral("expanded"));
        doc.appendChild(element);
        m_gantt->treeView()->saveExpanded(element);
    }
    ViewBase::setScheduleManager(sm);
    m_gantt->setScheduleManager(sm);

    if (expand) {
        m_gantt->treeView()->doExpand(doc);
    } else if (tryexpand) {
        m_gantt->treeView()->doExpand(m_domdoc);
    }
    if (sm && sm->expected()) {
        auto m = m_gantt->sfModel();
        const auto start = sm->expected()->start().date();
        const auto end = sm->expected()->end().date();
        if (m->periodStart() < start) {
            m->setPeriodStart(start);
        }
        if (!m->periodEnd().isValid() || m->periodEnd() > end) {
            m->setPeriodEnd(end);
        }
    } else if (project()) {
        auto m = m_gantt->sfModel();
        const auto start = project()->constraintStartTime().date();
        const auto end = project()->constraintEndTime().date();
        if (m->periodStart() < start) {
            m->setPeriodStart(start);
        }
        if (!m->periodEnd().isValid() || m->periodEnd() > end) {
            m->setPeriodEnd(end);
        }
    }
}

void GanttView::draw(Project &project)
{
    setProject(&project);
}

void GanttView::drawChanges(Project &project)
{
    if (m_project != &project) {
        setProject(&project);
    }
}

Node *GanttView::currentNode() const
{
    QModelIndex idx = m_gantt->treeView()->selectionModel()->currentIndex();
    return m_gantt->model()->node(m_gantt->sfModel()->mapToSource(idx));
}

void GanttView::slotContextMenuRequested(const QModelIndex &idx, const QPoint &pos)
{
    debugPlan<<idx<<idx.data();
    QString name;
    QModelIndex sidx = idx;
    if (sidx.isValid()) {
        const QAbstractProxyModel *proxy = qobject_cast<const QAbstractProxyModel*>(sidx.model());
        while (proxy != m_gantt->sfModel()) {
            sidx = proxy->mapToSource(sidx);
            proxy = qobject_cast<const QAbstractProxyModel*>(sidx.model());
        }
        if (!sidx.isValid()) {
            warnPlan<<Q_FUNC_INFO<<"Failed to find item model";
            return;
        }
    }
    //m_gantt->treeView()->selectionModel()->setCurrentIndex(sidx, QItemSelectionModel::ClearAndSelect);

    Node *node = m_gantt->model()->node(m_gantt->sfModel()->mapToSource(sidx));
    auto sid = scheduleManager() ? scheduleManager()->scheduleId() : -1;
    if (node) {
        switch (node->type()) {
            case Node::Type_Project:
                name = QStringLiteral("project_edit_popup");
                Q_EMIT requestPopupMenu(name, pos);
                return;
            case Node::Type_Task:
                name = node->isScheduled(sid) ? QStringLiteral("taskview_popup") : QStringLiteral("task_unscheduled_popup");
                break;
            case Node::Type_Milestone:
                name = node->isScheduled(sid) ? QStringLiteral("taskview_milestone_popup") : QStringLiteral("task_unscheduled_popup");
                break;
            case Node::Type_Summarytask:
                name = QStringLiteral("taskview_summary_popup");
                break;
            default:
                break;
        }
    } else debugPlan<<"No node";
    m_gantt->treeView()->setContextMenuIndex(sidx);
    if (name.isEmpty()) {
        slotHeaderContextMenuRequested(pos);
    } else {
        openContextMenu(name, pos);
    }
    m_gantt->treeView()->setContextMenuIndex(QModelIndex());
}

bool GanttView::loadContext(const KoXmlElement &settings)
{
    debugPlan;
    ViewBase::loadContext(settings);
    const auto c = actionCollection();
    bool value = (bool)(settings.attribute(actionShowProject->objectName(), QString::number(0)).toInt());
    actionShowProject->setChecked(value);

    auto a = c->action(QStringLiteral("tasks_and_milestones_group"));
    value = (bool)(settings.attribute(a->objectName(), QString::number(0)).toInt());
    a->setChecked(value);

    value = (bool)(settings.attribute(actionShowUnscheduled->objectName(), QString::number(0)).toInt());
    actionShowUnscheduled->setChecked(value);

    a = c->action(QStringLiteral("show_tasks"));
    value = (bool)(settings.attribute(a->objectName(), QString::number(1)).toInt());
    a->setChecked(value);

    a = c->action(QStringLiteral("show_milestones"));
    value = (bool)(settings.attribute(a->objectName(), QString::number(1)).toInt());
    a->setChecked(value);

    a = c->action(QStringLiteral("period_group"));
    value = (bool)(settings.attribute(a->objectName(), QString::number(0)).toInt());
    a->setChecked(value);

    a = c->action(QStringLiteral("show_running"));
    value = (bool)(settings.attribute(a->objectName(), QString::number(1)).toInt());
    a->setChecked(value);

    a = c->action(QStringLiteral("show_finished"));
    value = (bool)(settings.attribute(a->objectName(), QString::number(1)).toInt());
    a->setChecked(value);

    a = c->action(QStringLiteral("show_not_started"));
    value = (bool)(settings.attribute(a->objectName(), QString::number(1)).toInt());
    a->setChecked(value);

    a = c->action(QStringLiteral("enable_period"));
    value = (bool)(settings.attribute(a->objectName(), QString::number(1)).toInt());
    a->setChecked(value);

    auto days = settings.attribute("period-days", QString::number(7)).toInt();
    m_filterOptions->setDays(days);

    a = c->action(QStringLiteral("enable_spesific_period"));
    value = (bool)(settings.attribute(a->objectName(), QString::number(0)).toInt());
    a->setChecked(value);

    auto date = QDate::fromString(settings.attribute(QStringLiteral("period-start")));
    m_gantt->sfModel()->setPeriodStart(date);
    date = QDate::fromString(settings.attribute(QStringLiteral("period-end")));
    m_gantt->sfModel()->setPeriodEnd(date);

    return m_gantt->loadContext(settings);
}

void GanttView::saveContext(QDomElement &settings) const
{
    debugPlan;
    ViewBase::saveContext(settings);
    const auto c = actionCollection();
    settings.setAttribute(actionShowProject->objectName(), actionShowProject->isChecked());

    auto a = c->action(QStringLiteral("tasks_and_milestones_group"));
    settings.setAttribute(a->objectName(), a->isChecked());

    settings.setAttribute(actionShowUnscheduled->objectName(), actionShowUnscheduled->isChecked());

    a = c->action(QStringLiteral("show_tasks"));
    settings.setAttribute(a->objectName(), a->isChecked());

    a = c->action(QStringLiteral("show_milestones"));
    settings.setAttribute(a->objectName(), a->isChecked());

    a = c->action(QStringLiteral("period_group"));
    settings.setAttribute(a->objectName(), a->isChecked());

    a = c->action(QStringLiteral("show_running"));
    settings.setAttribute(a->objectName(), a->isChecked());

    a = c->action(QStringLiteral("show_finished"));
    settings.setAttribute(a->objectName(), a->isChecked());

    a = c->action(QStringLiteral("show_not_started"));
    settings.setAttribute(a->objectName(), a->isChecked());

    a = c->action(QStringLiteral("enable_period"));
    settings.setAttribute(a->objectName(), a->isChecked());

    settings.setAttribute(QStringLiteral("period-days"), m_filterOptions->days());

    a = c->action(QStringLiteral("enable_spesific_period"));
    qInfo()<<Q_FUNC_INFO<<a<<a->isChecked()<<a->isCheckable();
    settings.setAttribute(a->objectName(), a->isChecked());

    settings.setAttribute(QStringLiteral("period-start"), m_gantt->sfModel()->periodStart().toString(Qt::ISODate));
    settings.setAttribute(QStringLiteral("period-end"), m_gantt->sfModel()->periodEnd().toString(Qt::ISODate));

    m_gantt->saveContext(settings);
}

void GanttView::updateReadWrite(bool on)
{
    // TODO: KGanttView needs read/write mode
    m_readWrite = on;
    ViewBase::updateReadWrite(on);
    if (m_gantt->model()) {
        m_gantt->model()->setReadWrite(on);
    }
}

void GanttView::slotOpenCurrentNode()
{
    //debugPlan;
    slotOpenNode(currentNode());
}

void GanttView::slotOpenNode(Node *node)
{
    //debugPlan;
    if (!node) {
        return ;
    }
    switch (node->type()) {
        case Node::Type_Task: {
                Task *task = static_cast<Task *>(node);
                TaskDialog *dia = new TaskDialog(*project(), *task, project()->accounts(), this);
                connect(dia, &QDialog::finished, this, &GanttView::slotTaskEditFinished);
                dia->open();
                break;
            }
        case Node::Type_Milestone: {
                // Use the normal task dialog for now.
                // Maybe milestone should have it's own dialog, but we need to be able to
                // enter a duration in case we accidentally set a tasks duration to zero
                // and hence, create a milestone
                Task *task = static_cast<Task *>(node);
                TaskDialog *dia = new TaskDialog(*project(), *task, project()->accounts(), this);
                connect(dia, &QDialog::finished, this, &GanttView::slotTaskEditFinished);
                dia->open();
                break;
            }
        case Node::Type_Summarytask: {
                Task *task = dynamic_cast<Task *>(node);
                Q_ASSERT(task);
                SummaryTaskDialog *dia = new SummaryTaskDialog(*task, this);
                connect(dia, &QDialog::finished, this, &GanttView::slotSummaryTaskEditFinished);
                dia->open();
                break;
            }
        default:
            break;
    }
}

void GanttView::slotTaskEditFinished(int result)
{
    TaskDialog *dia = qobject_cast<TaskDialog*>(sender());
    if (dia == nullptr) {
        return;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command *cmd = dia->buildCommand();
        if (cmd) {
            koDocument()->addCommand(cmd);
        }
    }
    dia->deleteLater();
}

void GanttView::slotSummaryTaskEditFinished(int result)
{
    SummaryTaskDialog *dia = qobject_cast<SummaryTaskDialog*>(sender());
    if (dia == nullptr) {
        return;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command * cmd = dia->buildCommand();
        if (cmd) {
            koDocument()->addCommand(cmd);
        }
    }
    dia->deleteLater();
}

void GanttView::slotTaskProgress()
{
    //debugPlan;
    Node * node = currentNode();
    if (!node)
        return ;

    switch (node->type()) {
        case Node::Type_Project: {
                break;
            }
        case Node::Type_Subproject:
            //TODO
            break;
        case Node::Type_Task: {
                Task *task = dynamic_cast<Task *>(node);
                Q_ASSERT(task);
                TaskProgressDialog *dia = new TaskProgressDialog(*task, scheduleManager(),  project()->standardWorktime(), this);
                connect(dia, &QDialog::finished, this, &GanttView::slotTaskProgressFinished);
                dia->open();
                break;
            }
        case Node::Type_Milestone: {
                Task *task = dynamic_cast<Task *>(node);
                Q_ASSERT(task);
                MilestoneProgressDialog *dia = new MilestoneProgressDialog(*task, this);
                connect(dia, &QDialog::finished, this, &GanttView::slotMilestoneProgressFinished);
                dia->open();
                break;
            }
        case Node::Type_Summarytask: {
                // TODO
                break;
            }
        default:
            break; // avoid warnings
    }
}

void GanttView::slotTaskProgressFinished(int result)
{
    TaskProgressDialog *dia = qobject_cast<TaskProgressDialog*>(sender());
    if (dia == nullptr) {
        return;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command * m = dia->buildCommand();
        if (m) {
            koDocument()->addCommand(m);
        }
    }
    dia->deleteLater();
}

void GanttView::slotMilestoneProgressFinished(int result)
{
    MilestoneProgressDialog *dia = qobject_cast<MilestoneProgressDialog*>(sender());
    if (dia == nullptr) {
        return;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command * m = dia->buildCommand();
        if (m) {
            koDocument()->addCommand(m);
        }
    }
    dia->deleteLater();
}

void GanttView::slotOpenProjectDescription()
{
    TaskDescriptionDialog *dia = new TaskDescriptionDialog(*project(), this, !isReadWrite());
    connect(dia, &QDialog::finished, this, &GanttView::slotTaskDescriptionFinished);
    dia->open();
}

void GanttView::slotTaskDescription()
{
    slotOpenTaskDescription(!isReadWrite());
}

void GanttView::slotOpenTaskDescription(bool ro)
{
    //debugPlan;
    Node * node = currentNode();
    if (!node)
        return ;

    switch (node->type()) {
        case Node::Type_Task:
        case Node::Type_Milestone:
        case Node::Type_Summarytask: {
                TaskDescriptionDialog *dia = new TaskDescriptionDialog(*node, this, ro);
                connect(dia, &QDialog::finished, this, &GanttView::slotTaskDescriptionFinished);
                dia->open();
                break;
            }
        default:
            break;
    }
}

void GanttView::slotTaskDescriptionFinished(int result)
{
    TaskDescriptionDialog *dia = qobject_cast<TaskDescriptionDialog*>(sender());
    if (dia == nullptr) {
        return;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command * m = dia->buildCommand();
        if (m) {
            koDocument()->addCommand(m);
        }
    }
    dia->deleteLater();
}

void GanttView::slotDocuments()
{
    //debugPlan;
    Node * node = currentNode();
    if (!node) {
        return ;
    }
    switch (node->type()) {
        case Node::Type_Summarytask:
        case Node::Type_Task:
        case Node::Type_Milestone: {
            DocumentsDialog *dia = new DocumentsDialog(*node, this);
            connect(dia, &QDialog::finished, this, &GanttView::slotDocumentsFinished);
            dia->open();
            break;
        }
        default:
            break;
    }
}

void GanttView::slotDocumentsFinished(int result)
{
    DocumentsDialog *dia = qobject_cast<DocumentsDialog*>(sender());
    if (dia == nullptr) {
        return;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command * m = dia->buildCommand();
        if (m) {
            koDocument()->addCommand(m);
        }
    }
    dia->deleteLater();
}

#include "moc_kptganttview.cpp"
