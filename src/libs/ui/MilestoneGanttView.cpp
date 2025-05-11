/* This file is part of the KDE project
 *  SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 *  SPDX-FileCopyrightText: 2005 Dag Andersen <dag.andersen@kdemail.net>
 *  SPDX-FileCopyrightText: 2006 Raphael Langerhorst <raphael.langerhorst@kdemail.net>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "MilestoneGanttView.h"

#include "kptganttitemdelegate.h"
#include "DateTimeTimeLine.h"
#include "DateTimeGrid.h"
#include "kpttaskdialog.h"
#include "kptsummarytaskdialog.h"
#include "kpttaskprogressdialog.h"
#include "kptmilestoneprogressdialog.h"
#include "kpttaskdescriptiondialog.h"
#include "kptdocumentsdialog.h"
#include "FilterWidget.h"

#include <kptnode.h>
#include <kptproject.h>
#include <MacroCommand.h>

#include <KoDocument.h>
#include <KActionCollection>
#include <KoIcon.h>
#include <kundo2command.h>
#include <KoPageLayoutWidget.h>

#include <KGanttGraphicsView>
#include <KGanttProxyModel>

#include <QHeaderView>
#include <QMenu>
#include <QActionGroup>

using namespace KPlato;

MilestoneGanttChartOptionsPanel::MilestoneGanttChartOptionsPanel(NodeGanttViewBase *gantt, QWidget *parent)
    : QWidget(parent)
    , m_gantt(gantt)
{
    ui.setupUi(this);
    setValues(*gantt->delegate());
}

void MilestoneGanttChartOptionsPanel::slotOk()
{
    debugPlan;
    auto id = ui.freedays->currentData().toString();
    m_gantt->setCalendar(ui.freedays->currentIndex(), m_gantt->project()->findCalendar(id));

    m_gantt->delegate()->showTaskName = ui.showTaskName->checkState() == Qt::Checked;

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

void MilestoneGanttChartOptionsPanel::setValues(const GanttItemDelegate &del)
{
    ui.showTaskName->setCheckState(del.showTaskName ? Qt::Checked : Qt::Unchecked);

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

void MilestoneGanttChartOptionsPanel::setDefault()
{
    GanttItemDelegate del;
    setValues(del);
}

MilestoneGanttViewSettingsDialog::MilestoneGanttViewSettingsDialog(NodeGanttViewBase *gantt, ViewBase *view, bool selectPrint)
    : ItemViewSettupDialog(view, gantt->treeView(), true, view),
    m_gantt(gantt)
{
    setFaceType(KPageDialog::Auto);
    m_chartOptions = new MilestoneGanttChartOptionsPanel(gantt, this);
    insertWidget(1, m_chartOptions, i18n("Chart"), i18n("Gantt Chart Settings"));
    createPrintingOptions(selectPrint);
    connect(this, SIGNAL(accepted()), this, SLOT(slotOk()));
}

void MilestoneGanttViewSettingsDialog::createPrintingOptions(bool setAsCurrent)
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

void MilestoneGanttViewSettingsDialog::slotOk()
{
    debugPlan;
    ItemViewSettupDialog::slotOk();
    m_chartOptions->slotOk();
    m_gantt->setPrintingOptions(m_printingOptions->options());
    m_gantt->graphicsView()->updateScene();
}

//------------------------
MilestoneKGanttView::MilestoneKGanttView(QWidget *parent)
    : NodeGanttViewBase(parent)
    , m_manager(nullptr)
{
    debugPlan<<"------------------- create MilestoneKGanttView -----------------------";
    MilestoneItemModel *mm = new MilestoneItemModel(this);
    for (int i = 0; i < mm->columnCount(); ++i) {
        if (i != NodeModel::NodeCompleted) {
            mm->setReadOnly(i, true);
        }
    }
    setItemModel(mm);
    treeView()->createItemDelegates(mm);

    sfModel()->setShowMilestones(true);
    sfModel()->setShowTasks(false);
    sfModel()->setShowUnscheduled(false);
    sfModel()->setShowSummarytasks(false);
    sfModel()->setTasksAndMilestonesGroupEnabled(true);

    QList<int> show;
    show << NodeModel::NodeWBSCode
            << NodeModel::NodeName
            << NodeModel::NodeStartTime;

    treeView()->setDefaultColumns(show);
    for (int i = 0; i < model()->columnCount(); ++i) {
        if (! show.contains(i)) {
            treeView()->hideColumn(i);
        }
    }
    treeView()->header()->moveSection(NodeModel::NodeWBSCode, show.indexOf(NodeModel::NodeWBSCode));
    treeView()->setRootIsDecorated (false);

    KGantt::ProxyModel *m = static_cast<KGantt::ProxyModel*>(ganttProxyModel());

    m->setRole(KGantt::ItemTypeRole, KGantt::ItemTypeRole); // To provide correct format
    m->setRole(KGantt::StartTimeRole, Qt::EditRole); // To provide correct format
    m->setRole(KGantt::EndTimeRole, Qt::EditRole); // To provide correct format

    m->removeColumn(Qt::DisplayRole);
    m->setColumn(KGantt::ItemTypeRole, NodeModel::NodeType);
    m->setColumn(KGantt::StartTimeRole, NodeModel::NodeStartTime);
    m->setColumn(KGantt::EndTimeRole, NodeModel::NodeEndTime);
    m->setColumn(KGantt::TaskCompletionRole, NodeModel::NodeCompleted);

    KGantt::DateTimeGrid *g = static_cast<KGantt::DateTimeGrid*>(grid());
    g->setDayWidth(30);
    // TODO: extend QLocale/KGantt to support formats for hourly time display
    // see bug #349030
    // removed custom code here

    // TODO: add to context
    treeView()->sortByColumn(NodeModel::NodeWBSCode, Qt::AscendingOrder);
    treeView()->setSortingEnabled(true);

    setWhatsThis(
              xi18nc("@info:whatsthis",
                     "<title>Milestone Gantt View</title>"
                     "<para>"
                     "Displays scheduled milestones in a Gantt diagram."
                     " The chart area can be zoomed in and out with a slider"
                     " positioned in the upper left corner of the time scale."
                     " <note>You need to hoover over it with the mouse for it to show.</note>"
                     "</para><para>"
                     "This view supports configuration and printing using the context menu."
                     "<nl/><link url='%1'>More...</link>"
                     "</para>", QStringLiteral("plan:milestone-gantt-view")));
}

MilestoneItemModel *MilestoneKGanttView::model() const
{
    return static_cast<MilestoneItemModel*>(NodeGanttViewBase::model());
}

void MilestoneKGanttView::setProject(Project *proj)
{
    if (project()) {
        disconnect(project(), &Project::projectCalculated, this, &MilestoneKGanttView::slotProjectCalculated);
    }
    NodeGanttViewBase::setProject(proj);
    if (proj) {
        connect(proj, &Project::projectCalculated, this, &MilestoneKGanttView::slotProjectCalculated);
    }
}

void MilestoneKGanttView::slotProjectCalculated(ScheduleManager *sm)
{
    if (m_manager == sm) {
        setScheduleManager(sm);
    }
}

void MilestoneKGanttView::setScheduleManager(ScheduleManager *sm)
{
    //debugPlan<<id<<'\n';
    model()->setScheduleManager(nullptr);
    m_manager = sm;
    KGantt::DateTimeGrid *g = static_cast<KGantt::DateTimeGrid*>(grid());
    if (sm && m_project) {
        QDateTime start;
        const QList<Node*> nodes = model()->mileStones();
        for (Node* n : nodes) {
            QDateTime nt = n->startTime(sm->scheduleId());
            if (! nt.isValid()) {
                continue;
            }
            if (! start.isValid() || start > nt) {
                start = nt;
                debugPlan<<n->name()<<start;
            }
        }
        if (! start.isValid()) {
            start = project()->startTime(sm->scheduleId());
        }
        if (start.isValid() && g->startDateTime() !=  start) {
            g->setStartDateTime(start);
        }
    }
    if (! g->startDateTime().isValid()) {
        g->setStartDateTime(QDateTime::currentDateTime());
    }
    model()->setScheduleManager(sm);
    graphicsView()->updateScene();
}

//------------------------------------------

MilestoneGanttView::MilestoneGanttView(KoPart *part, KoDocument *doc, QWidget *parent, bool readWrite)
    : ViewBase(part, doc, parent),
        m_readWrite(readWrite),
        m_project(nullptr)
{
    debugPlan <<" ---------------- Plan: Creating Milesone GanttView ----------------";

    setXMLFile(QStringLiteral("MilestoneGanttViewUi.rc"));

    QVBoxLayout *l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    m_splitter = new QSplitter(this);
    l->addWidget(m_splitter);
    m_splitter->setOrientation(Qt::Vertical);

    m_gantt = new MilestoneKGanttView(m_splitter);
    m_gantt->graphicsView()->setHeaderContextMenuPolicy(Qt::CustomContextMenu);

    m_showTaskName = false; // FIXME
    m_showProgress = false; //FIXME
    m_showPositiveFloat = false; //FIXME
    m_showCriticalTasks = false; //FIXME
    m_showNoInformation = false; //FIXME

    updateReadWrite(readWrite);

    setupGui();

    connect(m_gantt->treeView(), &TreeViewBase::contextMenuRequested, this, &MilestoneGanttView::slotContextMenuRequested);
    connect(m_gantt, &MyKGanttView::contextMenuRequested, this, &MilestoneGanttView::slotContextMenuRequested);

    connect(m_gantt->treeView(), &TreeViewBase::headerContextMenuRequested, this, &ViewBase::slotHeaderContextMenuRequested);
    connect(m_gantt->graphicsView(), &KGantt::GraphicsView::headerContextMenuRequested, this, &MilestoneGanttView::slotGanttHeaderContextMenuRequested);
    connect(qobject_cast<KGantt::DateTimeGrid*>(m_gantt->graphicsView()->grid()), &KGantt::DateTimeGrid::gridChanged, this, &MilestoneGanttView::slotDateTimeGridChanged);

    connect(m_gantt->treeView(), &GanttTreeView::doubleClicked, this, &MilestoneGanttView::itemDoubleClicked);

    connect(m_gantt->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MilestoneGanttView::slotSelectionChanged);

    connect(m_gantt->model(), &ItemModelBase::executeCommand, doc, &KoDocument::addCommand);

    updateActionsEnabled(false);
}

void MilestoneGanttView::slotEditCopy()
{
    m_gantt->editCopy();
}

void MilestoneGanttView::itemDoubleClicked(const QPersistentModelIndex &idx)
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

void MilestoneGanttView::slotGanttHeaderContextMenuRequested(const QPoint &pt)
{
    QMenu *menu = popupMenu(QStringLiteral("gantt_datetimegrid_popup"));
    if (menu) {
        menu->exec(pt);
    }
}

void MilestoneGanttView::setZoom(double)
{
    //debugPlan <<"setting gantt zoom:" << zoom;
    //m_gantt->setZoomFactor(zoom,true); NO!!! setZoomFactor() is something else
}

void MilestoneGanttView::show()
{
}

void MilestoneGanttView::clear()
{
}

void MilestoneGanttView::setProject(Project *project)
{
    m_gantt->setProject(project);
}

void MilestoneGanttView::setScheduleManager(ScheduleManager *sm)
{
    //debugPlan<<id<<'\n';
    m_gantt->setScheduleManager(sm);
}

void MilestoneGanttView::draw(Project &project)
{
    setProject(&project);
}

void MilestoneGanttView::drawChanges(Project &project)
{
    if (m_project != &project) {
        setProject(&project);
    }
}

Node *MilestoneGanttView::currentNode() const
{
    QModelIndex idx = m_gantt->treeView()->selectionModel()->currentIndex();
    return m_gantt->model()->node(m_gantt->sfModel()->mapToSource(idx));
}

void MilestoneGanttView::setupGui()
{
    createOptionActions(ViewBase::OptionAll & ~(ViewBase::OptionExpand|ViewBase::OptionCollapse));

    const auto actionList = contextActionList();
    for (QAction *a : actionList) {
        actionCollection()->addAction(a->objectName(), a);
    }

    m_scalegroup = new QActionGroup(this);
    QAction *a = new QAction(i18nc("@action:inmenu", "Auto"), this);
    a->setCheckable(true);
    a->setChecked(true);
    actionCollection()->addAction(QStringLiteral("scale_auto"), a);
    connect(a, &QAction::triggered, this, &MilestoneGanttView::ganttActions);
    m_scalegroup->addAction(a);

    a = new QAction(i18nc("@action:inmenu", "Month"), this);
    actionCollection()->addAction(QStringLiteral("scale_month"), a);
    a->setCheckable(true);
    connect(a, &QAction::triggered, this, &MilestoneGanttView::ganttActions);
    m_scalegroup->addAction(a);

    a = new QAction(i18nc("@action:inmenu", "Week"), this);
    actionCollection()->addAction(QStringLiteral("scale_week"), a);
    a->setCheckable(true);
    connect(a, &QAction::triggered, this, &MilestoneGanttView::ganttActions);
    m_scalegroup->addAction(a);

    a = new QAction(i18nc("@action:inmenu", "Day"), this);
    a->setCheckable(true);
    actionCollection()->addAction(QStringLiteral("scale_day"), a);
    connect(a, &QAction::triggered, this, &MilestoneGanttView::ganttActions);
    m_scalegroup->addAction(a);

    a = new QAction(i18nc("@action:inmenu", "Hour"), this);
    a->setCheckable(true);
    actionCollection()->addAction(QStringLiteral("scale_hour"), a);
    connect(a, &QAction::triggered, this, &MilestoneGanttView::ganttActions);
    m_scalegroup->addAction(a);

    a = new QAction(i18nc("@action:inmenu", "Zoom In"), this);
    a->setIcon(koIcon("zoom-in"));
    actionCollection()->addAction(QStringLiteral("zoom_in"), a);
    connect(a, &QAction::triggered, this, &MilestoneGanttView::ganttActions);

    a = new QAction(i18nc("@action:inmenu", "Zoom Out"), this);
    a->setIcon(koIcon("zoom-out"));
    actionCollection()->addAction(QStringLiteral("zoom_out"), a);
    connect(a, &QAction::triggered, this, &MilestoneGanttView::ganttActions);

    auto actionOpenNode  = new QAction(koIcon("document-edit"), i18n("Edit..."), this);
    actionCollection()->addAction(QStringLiteral("node_properties"), actionOpenNode);
    connect(actionOpenNode, &QAction::triggered, this, &MilestoneGanttView::slotOpenCurrentNode);

    auto actionTaskProgress  = new QAction(koIcon("document-edit"), i18n("Progress..."), this);
    actionCollection()->addAction(QStringLiteral("task_progress"), actionTaskProgress);
    connect(actionTaskProgress, &QAction::triggered, this, &MilestoneGanttView::slotTaskProgress);

    auto actionTaskDescription  = new QAction(koIcon("document-edit"), i18n("Description..."), this);
    actionCollection()->addAction(QStringLiteral("task_description"), actionTaskDescription);
    connect(actionTaskDescription, &QAction::triggered, this, &MilestoneGanttView::slotTaskDescription);

    auto actionDocuments  = new QAction(koIcon("document-edit"), i18n("Documents..."), this);
    actionCollection()->addAction(QStringLiteral("task_documents"), actionDocuments);
    connect(actionDocuments, &QAction::triggered, this, &MilestoneGanttView::slotDocuments);

    auto filter = new QWidgetAction(this);
    filter->setObjectName(QStringLiteral("edit_filter"));
    filter->setText(i18n("Filter"));
    auto filterWidget = new FilterWidget(false, this);
    filter->setDefaultWidget(filterWidget);
    actionCollection()->addAction(filter->objectName(), filter);
    connect(filterWidget->lineedit, &QLineEdit::textChanged, m_gantt->sfModel(), qOverload<const QString&>(&NodeSortFilterProxyModel::setFilterRegularExpression));
}

void MilestoneGanttView::slotSelectionChanged()
{
    updateActionsEnabled(true);
}

void MilestoneGanttView::updateActionsEnabled(bool on)
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

void MilestoneGanttView::slotDateTimeGridChanged()
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

void MilestoneGanttView::ganttActions()
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

void MilestoneGanttView::slotContextMenuRequested(const QModelIndex &idx, const QPoint &pos)
{
    debugPlan;
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
    m_gantt->treeView()->selectionModel()->setCurrentIndex(sidx, QItemSelectionModel::ClearAndSelect);

    Node *node = m_gantt->model()->node(m_gantt->sfModel()->mapToSource(sidx));
    if (node) {
        switch (node->type()) {
            case Node::Type_Task:
                name = QStringLiteral("taskview_popup");
                break;
            case Node::Type_Milestone:
                name = QStringLiteral("taskview_milestone_popup");
                break;
            case Node::Type_Summarytask:
                name = QStringLiteral("taskview_summary_popup");
                break;
            default:
                break;
        }
    } else debugPlan<<"No node";
    m_gantt->treeView()->setContextMenuIndex(idx);
    if (name.isEmpty()) {
        slotHeaderContextMenuRequested(pos);
    } else {
        openContextMenu(name, pos);
    }
    m_gantt->treeView()->setContextMenuIndex(QModelIndex());
}

void MilestoneGanttView::slotOptions()
{
    debugPlan;
    qInfo()<<Q_FUNC_INFO<<sender();
    MilestoneGanttViewSettingsDialog *dlg =  new MilestoneGanttViewSettingsDialog(m_gantt, this, sender()->objectName() == QStringLiteral("print_options"));
    connect(dlg, SIGNAL(finished(int)), SLOT(slotOptionsFinished(int)));
    dlg->open();
}

bool MilestoneGanttView::loadContext(const KoXmlElement &settings)
{
    debugPlan;
    ViewBase::loadContext(settings);
    return m_gantt->loadContext(settings);
}

void MilestoneGanttView::saveContext(QDomElement &settings) const
{
    debugPlan;
    ViewBase::saveContext(settings);
    return m_gantt->saveContext(settings);
}

void MilestoneGanttView::updateReadWrite(bool on)
{
    m_readWrite = on;
    ViewBase::updateReadWrite(on);
    if (m_gantt->model()) {
        m_gantt->model()->setReadWrite(on);
    }
}

KoPrintJob *MilestoneGanttView::createPrintJob()
{
    return new GanttPrintingDialog(this, m_gantt);
}

void MilestoneGanttView::slotOpenCurrentNode()
{
    //debugPlan;
    slotOpenNode(currentNode());
}

void MilestoneGanttView::slotOpenNode(Node *node)
{
    //debugPlan;
    if (!node) {
        return ;
    }
    switch (node->type()) {
        case Node::Type_Task: {
                Task *task = static_cast<Task *>(node);
                TaskDialog *dia = new TaskDialog(*project(), *task, project()->accounts(), this);
                connect(dia, &QDialog::finished, this, &MilestoneGanttView::slotTaskEditFinished);
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
                connect(dia, &QDialog::finished, this, &MilestoneGanttView::slotTaskEditFinished);
                dia->open();
                break;
            }
        case Node::Type_Summarytask: {
                Task *task = dynamic_cast<Task *>(node);
                Q_ASSERT(task);
                SummaryTaskDialog *dia = new SummaryTaskDialog(*task, this);
                connect(dia, &QDialog::finished, this, &MilestoneGanttView::slotSummaryTaskEditFinished);
                dia->open();
                break;
            }
        default:
            break;
    }
}

void MilestoneGanttView::slotTaskEditFinished(int result)
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

void MilestoneGanttView::slotSummaryTaskEditFinished(int result)
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

void MilestoneGanttView::slotTaskProgress()
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
                connect(dia, &QDialog::finished, this, &MilestoneGanttView::slotTaskProgressFinished);
                dia->open();
                break;
            }
        case Node::Type_Milestone: {
                Task *task = dynamic_cast<Task *>(node);
                Q_ASSERT(task);
                MilestoneProgressDialog *dia = new MilestoneProgressDialog(*task, this);
                connect(dia, &QDialog::finished, this, &MilestoneGanttView::slotMilestoneProgressFinished);
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

void MilestoneGanttView::slotTaskProgressFinished(int result)
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

void MilestoneGanttView::slotMilestoneProgressFinished(int result)
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

void MilestoneGanttView::slotOpenProjectDescription()
{
    TaskDescriptionDialog *dia = new TaskDescriptionDialog(*project(), this, !isReadWrite());
    connect(dia, &QDialog::finished, this, &MilestoneGanttView::slotTaskDescriptionFinished);
    dia->open();
}

void MilestoneGanttView::slotTaskDescription()
{
    slotOpenTaskDescription(!isReadWrite());
}

void MilestoneGanttView::slotOpenTaskDescription(bool ro)
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
                connect(dia, &QDialog::finished, this, &MilestoneGanttView::slotTaskDescriptionFinished);
                dia->open();
                break;
            }
        default:
            break;
    }
}

void MilestoneGanttView::slotTaskDescriptionFinished(int result)
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

void MilestoneGanttView::slotDocuments()
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
            connect(dia, &QDialog::finished, this, &MilestoneGanttView::slotDocumentsFinished);
            dia->open();
            break;
        }
        default:
            break;
    }
}

void MilestoneGanttView::slotDocumentsFinished(int result)
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
