/* This file is part of the KDE project
 *  SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 *  SPDX-FileCopyrightText: 2005 Dag Andersen <dag.andersen@kdemail.net>
 *  SPDX-FileCopyrightText: 2006 Raphael Langerhorst <raphael.langerhorst@kdemail.net>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "ResourceAppointmentsGanttView.h"

#include <kptresourceappointmentsmodel.h>

#include "kptnodeitemmodel.h"
#include "kptappointment.h"
#include "kptnode.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptresource.h"
#include "kptrelation.h"
#include "kptschedule.h"
#include "kptviewbase.h"
#include "kptitemviewsettup.h"
#include "kptduration.h"
#include "kptdatetime.h"
#include "kptresourceappointmentsmodel.h"
#include "kptdebug.h"
#include "DateTimeTimeLine.h"
#include "DateTimeGrid.h"
#include "kptganttitemdelegate.h"
#include "config.h"
#include "kptcommand.h"
#include "kpttaskdialog.h"
#include "kptsummarytaskdialog.h"
#include "kpttaskdescriptiondialog.h"
#include "kpttaskprogressdialog.h"
#include "kptmilestoneprogressdialog.h"
#include "kptdocumentsdialog.h"

#include <KGanttProxyModel>
#include <KGanttGraphicsView>
#include <KGanttTreeViewRowController>

#include <KoDocument.h>
#include <KoXmlReader.h>
#include <KoPageLayoutWidget.h>
#include <KoIcon.h>

#include <QHeaderView>
#include <QTabWidget>
#include <QPushButton>
#include <QAction>
#include <QActionGroup>
#include <QMenu>

#include <KToggleAction>
#include <KActionCollection>


using namespace KPlato;

ResourceAppointmentsGanttChartOptionsPanel::ResourceAppointmentsGanttChartOptionsPanel(GanttViewBase *gantt, QWidget *parent)
    : QWidget(parent)
    , m_gantt(gantt)
{
    ui.setupUi(this);
    setValues();
}

void ResourceAppointmentsGanttChartOptionsPanel::slotOk()
{
    debugPlan;
    auto id = ui.freedays->currentData().toString();
    m_gantt->setCalendar(ui.freedays->currentIndex(), m_gantt->project()->findCalendar(id));

    DateTimeTimeLine *timeline = m_gantt->timeLine();

    timeline->setInterval(ui.timeLineInterval->value() * 60000);
    QPen pen;
    pen.setWidth(ui.timeLineStroke->value());
    pen.setColor(ui.timeLineColor->color());
    timeline->setPen(pen);

    DateTimeTimeLine::Options opt = timeline->options();
    opt.setFlag(DateTimeTimeLine::Foreground, ui.timeLineForeground->isChecked());
    opt.setFlag(DateTimeTimeLine::Background, ui.timeLineBackground->isChecked());
    opt.setFlag(DateTimeTimeLine::UseCustomPen, ui.timeLineUseCustom->isChecked());
    timeline->setOptions(opt);

    m_gantt->setShowRowSeparators(ui.showRowSeparators->checkState() == Qt::Checked);
}

void ResourceAppointmentsGanttChartOptionsPanel::setValues()
{
    DateTimeTimeLine *timeline = m_gantt->timeLine();

    ui.timeLineInterval->setValue(timeline->interval() / 60000);

    QPen pen = timeline->pen();
    ui.timeLineStroke->setValue(pen.width());
    ui.timeLineColor->setColor(pen.color());

    ui.timeLineHide->setChecked(true);
    DateTimeTimeLine::Options opt = timeline->options();
    ui.timeLineBackground->setChecked(opt & DateTimeTimeLine::Background);
    ui.timeLineForeground->setChecked(opt & DateTimeTimeLine::Foreground);
    ui.timeLineUseCustom->setChecked(opt & DateTimeTimeLine::UseCustomPen);

    ui.showRowSeparators->setChecked(m_gantt->showRowSeparators());

    ui.freedays->disconnect();
    ui.freedays->clear();
    ui.freedays->addItem(i18nc("@item:inlistbox", "No freedays"));
    ui.freedays->addItem(i18nc("@item:inlistbox", "Project freedays"));
    int currentIndex = m_gantt->freedaysType();
    const auto project = m_gantt->project();
    if (project) {
        auto current = m_gantt->calendar();
        const auto calendars = project->calendars();
        for (auto *c : calendars) {
            ui.freedays->addItem(c->name(), c->id());
            if (c == current) {
                currentIndex = ui.freedays->count() - 1;
            }
        }
        ui.freedays->setCurrentIndex(currentIndex);
        connect(ui.freedays, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int idx) {
            auto box = qobject_cast<QComboBox*>(sender());
            m_gantt->setCalendar(idx, m_gantt->project()->findCalendar(box->currentData().toString()));
        });
    }
}

void ResourceAppointmentsGanttChartOptionsPanel::setDefault()
{
    setValues();
}

ResourceAppointmentsGanttViewSettingsDialog::ResourceAppointmentsGanttViewSettingsDialog(GanttViewBase *gantt,  ViewBase *view, bool selectPrint)
    : ItemViewSettupDialog(view, gantt->treeView(), true, view)
    , m_gantt(gantt)
{
    setFaceType(KPageDialog::Auto);
    m_chartOptions = new ResourceAppointmentsGanttChartOptionsPanel(gantt, this);
    insertWidget(1, m_chartOptions, i18n("Chart"), i18n("Gantt Chart Settings"));
    createPrintingOptions(selectPrint);
    connect(this, SIGNAL(accepted()), this, SLOT(slotOk()));
}

void ResourceAppointmentsGanttViewSettingsDialog::createPrintingOptions(bool setAsCurrent)
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

void ResourceAppointmentsGanttViewSettingsDialog::slotOk()
{
    debugPlan;
    ItemViewSettupDialog::slotOk();
    m_chartOptions->slotOk();
    m_gantt->setPrintingOptions(m_printingOptions->options());
    m_gantt->graphicsView()->updateScene();
}

//------------------------------------------

ResourceAppointmentsGanttView::ResourceAppointmentsGanttView(KoPart *part, KoDocument *doc, QWidget *parent, bool readWrite)
    : ViewBase(part, doc, parent),
    m_project(nullptr),
    m_model(new ResourceAppointmentsGanttModel(this))
{
    debugPlan <<" ---------------- KPlato: Creating ResourceAppointmentsGanttView ----------------";

    setXMLFile(QStringLiteral("ResourceAppointmentsGanttViewUi.rc"));

    m_gantt = new GanttViewBase(this);
    m_gantt->graphicsView()->setHeaderContextMenuPolicy(Qt::CustomContextMenu);
    m_gantt->graphicsView()->setItemDelegate(new ResourceGanttItemDelegate(m_gantt));

    GanttTreeView *tv = new GanttTreeView(m_gantt);
    tv->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    tv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    tv->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel); // needed since qt 4.2
    m_gantt->setLeftView(tv);
    connect(this, &ViewBase::expandAll, tv, &TreeViewBase::slotExpand);
    connect(this, &ViewBase::collapseAll, tv, &TreeViewBase::slotCollapse);
    m_rowController = new KGantt::TreeViewRowController(tv, m_gantt->ganttProxyModel());
    m_gantt->setRowController(m_rowController);
    tv->header()->setStretchLastSection(true);

    tv->setTreePosition(-1);

    KGantt::ProxyModel *m = static_cast<KGantt::ProxyModel*>(m_gantt->ganttProxyModel());
    m->setRole(KGantt::ItemTypeRole, KGantt::ItemTypeRole);
    m->setRole(KGantt::StartTimeRole, KGantt::StartTimeRole);
    m->setRole(KGantt::EndTimeRole, KGantt::EndTimeRole);
    m->setRole(KGantt::TaskCompletionRole, KGantt::TaskCompletionRole);

    m_gantt->setModel(m_model);

    QVBoxLayout *l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    l->addWidget(m_gantt);

    setupGui();

    updateReadWrite(readWrite);

    connect(m_gantt->leftView(), SIGNAL(contextMenuRequested(QModelIndex,QPoint,QModelIndexList)), SLOT(slotContextMenuRequested(QModelIndex,QPoint)));
    connect(m_gantt, &GanttViewBase::contextMenuRequested, this, &ResourceAppointmentsGanttView::slotContextMenuRequestedFromGantt);

    connect(m_gantt->leftView(), SIGNAL(headerContextMenuRequested(QPoint)), SLOT(slotHeaderContextMenuRequested(QPoint)));
    connect(m_gantt->graphicsView(), &KGantt::GraphicsView::headerContextMenuRequested, this, &ResourceAppointmentsGanttView::slotGanttHeaderContextMenuRequested);
    connect(qobject_cast<KGantt::DateTimeGrid*>(m_gantt->graphicsView()->grid()), &KGantt::DateTimeGrid::gridChanged, this, &ResourceAppointmentsGanttView::slotDateTimeGridChanged);
    connect(m_gantt->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ResourceAppointmentsGanttView::slotSelectionChanged);

    updateActionsEnabled(false);

    setWhatsThis(
              xi18nc("@info:whatsthis",
                     "<title>Resource Assignments (Gantt)</title>"
                     "<para>"
                     "Displays the scheduled resource - task assignments in a Gantt diagram."
                     " The chart area can be zoomed in and out with a slider"
                     " positioned in the upper left corner of the time scale."
                     " <note>You need to hoover over it with the mouse for it to show.</note>"
                     "</para><para>"
                     "This view supports configuration and printing using the context menu."
                     "<nl/><link url='%1'>More...</link>"
                     "</para>", QStringLiteral("plan:resource-assignment-gantt-view")));
}

ResourceAppointmentsGanttView::~ResourceAppointmentsGanttView()
{
    delete m_rowController;
}

void ResourceAppointmentsGanttView::slotEditCopy()
{
    m_gantt->editCopy();
}

void ResourceAppointmentsGanttView::slotGanttHeaderContextMenuRequested(const QPoint &pt)
{
    QMenu *menu = popupMenu(QStringLiteral("gantt_datetimegrid_popup"));
    if (menu) {
        menu->exec(pt);
    }
}

void ResourceAppointmentsGanttView::setZoom(double)
{
    //debugPlan <<"setting gantt zoom:" << zoom;
    //m_gantt->setZoomFactor(zoom,true); NO!!! setZoomFactor() is something else
}

Project *ResourceAppointmentsGanttView::project() const
{
    return m_model->project();
}

void ResourceAppointmentsGanttView::setProject(Project *project)
{
    m_gantt->setProject(project);
    m_model->setProject(project);
    ViewBase::setProject(project);
}

void ResourceAppointmentsGanttView::setScheduleManager(ScheduleManager *sm)
{
    //debugPlan<<id<<'\n';
    if (!sm && scheduleManager()) {
        // we should only get here if the only schedule manager is scheduled,
        // or when last schedule manager is deleted
        m_domdoc.clear();
        QDomElement element = m_domdoc.createElement(QStringLiteral("expanded"));
        m_domdoc.appendChild(element);
        treeView()->saveExpanded(element);
    }
    bool tryexpand = sm && !scheduleManager();
    bool expand = sm && scheduleManager() && sm != scheduleManager();
    QDomDocument doc;
    if (expand) {
        QDomElement element = doc.createElement(QStringLiteral("expanded"));
        doc.appendChild(element);
        treeView()->saveExpanded(element);
    }
    ViewBase::setScheduleManager(sm);
    m_model->setScheduleManager(sm);

    if (expand) {
        treeView()->doExpand(doc);
    } else if (tryexpand) {
        treeView()->doExpand(m_domdoc);
    }
    m_gantt->graphicsView()->updateScene();
}

void ResourceAppointmentsGanttView::setupGui()
{
    createOptionActions(ViewBase::OptionAll);

    const auto actionList = contextActionList();
    for (QAction *a : actionList) {
        actionCollection()->addAction(a->objectName(), a);
    }

    m_scalegroup = new QActionGroup(this);
    QAction *a = new QAction(i18nc("@action:inmenu", "Auto"), this);
    a->setCheckable(true);
    a->setChecked(true);
    actionCollection()->addAction(QStringLiteral("scale_auto"), a);
    connect(a, &QAction::triggered, this, &ResourceAppointmentsGanttView::ganttActions);
    m_scalegroup->addAction(a);

    a = new QAction(i18nc("@action:inmenu", "Month"), this);
    actionCollection()->addAction(QStringLiteral("scale_month"), a);
    a->setCheckable(true);
    connect(a, &QAction::triggered, this, &ResourceAppointmentsGanttView::ganttActions);
    m_scalegroup->addAction(a);

    a = new QAction(i18nc("@action:inmenu", "Week"), this);
    actionCollection()->addAction(QStringLiteral("scale_week"), a);
    a->setCheckable(true);
    connect(a, &QAction::triggered, this, &ResourceAppointmentsGanttView::ganttActions);
    m_scalegroup->addAction(a);

    a = new QAction(i18nc("@action:inmenu", "Day"), this);
    a->setCheckable(true);
    actionCollection()->addAction(QStringLiteral("scale_day"), a);
    connect(a, &QAction::triggered, this, &ResourceAppointmentsGanttView::ganttActions);
    m_scalegroup->addAction(a);

    a = new QAction(i18nc("@action:inmenu", "Hour"), this);
    a->setCheckable(true);
    actionCollection()->addAction(QStringLiteral("scale_hour"), a);
    connect(a, &QAction::triggered, this, &ResourceAppointmentsGanttView::ganttActions);
    m_scalegroup->addAction(a);

    a = new QAction(i18nc("@action:inmenu", "Zoom In"), this);
    a->setIcon(koIcon("zoom-in"));
    actionCollection()->addAction(QStringLiteral("zoom_in"), a);
    connect(a, &QAction::triggered, this, &ResourceAppointmentsGanttView::ganttActions);

    a = new QAction(i18nc("@action:inmenu", "Zoom Out"), this);
    a->setIcon(koIcon("zoom-out"));
    actionCollection()->addAction(QStringLiteral("zoom_out"), a);
    connect(a, &QAction::triggered, this, &ResourceAppointmentsGanttView::ganttActions);

    auto actionOpenNode  = new QAction(koIcon("document-edit"), i18n("Edit..."), this);
    actionCollection()->addAction(QStringLiteral("node_properties"), actionOpenNode);
    connect(actionOpenNode, &QAction::triggered, this, &ResourceAppointmentsGanttView::slotOpenCurrentNode);

    auto actionTaskProgress  = new QAction(koIcon("document-edit"), i18n("Progress..."), this);
    actionCollection()->addAction(QStringLiteral("task_progress"), actionTaskProgress);
    connect(actionTaskProgress, &QAction::triggered, this, &ResourceAppointmentsGanttView::slotTaskProgress);

    auto actionTaskDescription  = new QAction(koIcon("document-edit"), i18n("Description..."), this);
    actionCollection()->addAction(QStringLiteral("task_description"), actionTaskDescription);
    connect(actionTaskDescription, &QAction::triggered, this, &ResourceAppointmentsGanttView::slotTaskDescription);

    auto actionDocuments  = new QAction(koIcon("document-edit"), i18n("Documents..."), this);
    actionCollection()->addAction(QStringLiteral("task_documents"), actionDocuments);
    connect(actionDocuments, &QAction::triggered, this, &ResourceAppointmentsGanttView::slotDocuments);
}

void ResourceAppointmentsGanttView::slotSelectionChanged()
{
    updateActionsEnabled(true);
}

void ResourceAppointmentsGanttView::updateActionsEnabled(bool on)
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

void ResourceAppointmentsGanttView::slotDateTimeGridChanged()
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

void ResourceAppointmentsGanttView::ganttActions()
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

Node *ResourceAppointmentsGanttView::currentNode() const
{
    QModelIndex idx = treeView()->selectionModel()->currentIndex();
    return m_model->node(idx);
}

void ResourceAppointmentsGanttView::slotContextMenuRequestedFromGantt(const QModelIndex &idx, const QPoint &pos)
{
    QModelIndex sidx = idx;
    if (sidx.isValid()) {
        const QAbstractProxyModel *proxy = qobject_cast<const QAbstractProxyModel*>(sidx.model());
        while (proxy != nullptr) {
            sidx = proxy->mapToSource(sidx);
            proxy = qobject_cast<const QAbstractProxyModel*>(sidx.model());
        }
        if (!sidx.isValid()) {
            warnPlan<<Q_FUNC_INFO<<"Failed to find item model";
            return;
        }
    }
    if (sidx.isValid() && m_model->node(sidx.parent())) {
        // we get the individual appointment interval, task is its parent
        sidx = sidx.parent();
    }
    m_gantt->treeView()->selectionModel()->setCurrentIndex(sidx, QItemSelectionModel::ClearAndSelect);
    slotContextMenuRequested(sidx, pos);
}

void ResourceAppointmentsGanttView::slotContextMenuRequested(const QModelIndex &idx, const QPoint &pos)
{
    debugPlan<<idx;
    QString name;
    if (idx.isValid()) {
        Node *n = m_model->node(idx);
        if (n) {
            name = QStringLiteral("taskview_popup");
        }
    }
    m_gantt->treeView()->setContextMenuIndex(idx);
    if (name.isEmpty()) {
        slotHeaderContextMenuRequested(pos);
    } else {
        openContextMenu(name, pos);
    }
    m_gantt->treeView()->setContextMenuIndex(QModelIndex());
}

void ResourceAppointmentsGanttView::slotOptions()
{
    debugPlan;
    qInfo()<<Q_FUNC_INFO<<sender();
    ItemViewSettupDialog *dlg = new ResourceAppointmentsGanttViewSettingsDialog(m_gantt, this, sender()->objectName() == QStringLiteral("print_options"));
    connect(dlg, SIGNAL(finished(int)), SLOT(slotOptionsFinished(int)));
    dlg->open();
}

bool ResourceAppointmentsGanttView::loadContext(const KoXmlElement &settings)
{
    debugPlan;
    ViewBase::loadContext(settings);
    m_gantt->loadContext(settings);
    return treeView()->loadContext(m_model->columnMap(), settings);
}

void ResourceAppointmentsGanttView::saveContext(QDomElement &settings) const
{
    debugPlan;
    ViewBase::saveContext(settings);
    m_gantt->saveContext(settings);
    treeView()->saveContext(m_model->columnMap(), settings);
}

void ResourceAppointmentsGanttView::updateReadWrite(bool on)
{
    m_readWrite = on;
}

KoPrintJob *ResourceAppointmentsGanttView::createPrintJob()
{
    return new GanttPrintingDialog(this, m_gantt);
}

void ResourceAppointmentsGanttView::slotOpenCurrentNode()
{
    //debugPlan;
    slotOpenNode(currentNode());
}

void ResourceAppointmentsGanttView::slotOpenNode(Node *node)
{
    //debugPlan;
    if (!node) {
        return ;
    }
    switch (node->type()) {
        case Node::Type_Task: {
                Task *task = static_cast<Task *>(node);
                TaskDialog *dia = new TaskDialog(*project(), *task, project()->accounts(), this);
                connect(dia, &QDialog::finished, this, &ResourceAppointmentsGanttView::slotTaskEditFinished);
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
                connect(dia, &QDialog::finished, this, &ResourceAppointmentsGanttView::slotTaskEditFinished);
                dia->open();
                break;
            }
        case Node::Type_Summarytask: {
                Task *task = dynamic_cast<Task *>(node);
                Q_ASSERT(task);
                SummaryTaskDialog *dia = new SummaryTaskDialog(*task, this);
                connect(dia, &QDialog::finished, this, &ResourceAppointmentsGanttView::slotSummaryTaskEditFinished);
                dia->open();
                break;
            }
        default:
            break;
    }
}

void ResourceAppointmentsGanttView::slotTaskEditFinished(int result)
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

void ResourceAppointmentsGanttView::slotSummaryTaskEditFinished(int result)
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

void ResourceAppointmentsGanttView::slotTaskProgress()
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
                connect(dia, &QDialog::finished, this, &ResourceAppointmentsGanttView::slotTaskProgressFinished);
                dia->open();
                break;
            }
        case Node::Type_Milestone: {
                Task *task = dynamic_cast<Task *>(node);
                Q_ASSERT(task);
                MilestoneProgressDialog *dia = new MilestoneProgressDialog(*task, this);
                connect(dia, &QDialog::finished, this, &ResourceAppointmentsGanttView::slotMilestoneProgressFinished);
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

void ResourceAppointmentsGanttView::slotTaskProgressFinished(int result)
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

void ResourceAppointmentsGanttView::slotMilestoneProgressFinished(int result)
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

void ResourceAppointmentsGanttView::slotOpenProjectDescription()
{
    TaskDescriptionDialog *dia = new TaskDescriptionDialog(*project(), this, !isReadWrite());
    connect(dia, &QDialog::finished, this, &ResourceAppointmentsGanttView::slotTaskDescriptionFinished);
    dia->open();
}

void ResourceAppointmentsGanttView::slotTaskDescription()
{
    slotOpenTaskDescription(!isReadWrite());
}

void ResourceAppointmentsGanttView::slotOpenTaskDescription(bool ro)
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
                connect(dia, &QDialog::finished, this, &ResourceAppointmentsGanttView::slotTaskDescriptionFinished);
                dia->open();
                break;
            }
        default:
            break;
    }
}

void ResourceAppointmentsGanttView::slotTaskDescriptionFinished(int result)
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

void ResourceAppointmentsGanttView::slotDocuments()
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
            connect(dia, &QDialog::finished, this, &ResourceAppointmentsGanttView::slotDocumentsFinished);
            dia->open();
            break;
        }
        default:
            break;
    }
}

void ResourceAppointmentsGanttView::slotDocumentsFinished(int result)
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

#include "moc_ResourceAppointmentsGanttView.cpp"
