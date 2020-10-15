/* This file is part of the KDE project
 *  Copyright (C) 2019 Dag Andersen <dag.andersen@kdemail.net>
 *  Copyright (C) 2005 Dag Andersen <dag.andersen@kdemail.net>
 *  Copyright (C) 2006 Raphael Langerhorst <raphael.langerhorst@kdemail.net>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

// clazy:excludeall=qstring-arg
#include "kptganttview.h"
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
#include "Help.h"
#include "kptdebug.h"
#include "DateTimeTimeLine.h"
#include "DateTimeGrid.h"
#include "kptganttitemdelegate.h"
#include "config.h"

#include <KGanttProxyModel>
#include <KGanttConstraintModel>
#include <KGanttConstraint>
#include <KGanttGraphicsView>
#include <KGanttTreeViewRowController>

#include <KoDocument.h>
#include <KoXmlReader.h>
#include <KoPageLayoutWidget.h>
#include <KoIcon.h>

#include <QSplitter>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QDateTime>
#include <QModelIndex>
#include <QPainter>
#include <QTabWidget>
#include <QPushButton>
#include <QLocale>
#include <QAction>
#include <QMenu>
#include <QHoverEvent>
#include <QScrollBar>
#include <QDrag>
#include <QClipboard>
#include <QAbstractSlider>

#include <ktoggleaction.h>
#include <KActionCollection>

#include <KGanttGlobal>
#include <KGanttStyleOptionGanttItem>


/// The main namespace
namespace KPlato
{

class GanttItemDelegate;


//-------------------------------------------
NodeGanttViewBase::NodeGanttViewBase(QWidget *parent)
    : GanttViewBase(parent),
    m_project(nullptr),
    m_ganttdelegate(new GanttItemDelegate(this))
{
    debugPlan<<"------------------- create NodeGanttViewBase -----------------------";
    graphicsView()->setItemDelegate(m_ganttdelegate);
    GanttTreeView *tv = new GanttTreeView(this);
    tv->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    tv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    tv->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel); // needed since qt 4.2
    setLeftView(tv);
    m_rowController = new KGantt::TreeViewRowController(tv, ganttProxyModel());
    setRowController(m_rowController);
    tv->header()->setStretchLastSection(true);

    NodeSortFilterProxyModel *m = new NodeSortFilterProxyModel(&m_defaultModel, this, true);
    KGantt::View::setModel(m);
}

NodeGanttViewBase::~NodeGanttViewBase()
{
    delete m_rowController;
}

NodeSortFilterProxyModel *NodeGanttViewBase::sfModel() const
{
    return static_cast<NodeSortFilterProxyModel*>(KGantt::View::model());
}

void NodeGanttViewBase::setItemModel(ItemModelBase *model)
{
    sfModel()->setSourceModel(model);
}

ItemModelBase *NodeGanttViewBase::model() const
{
    return sfModel()->itemModel();
}

void NodeGanttViewBase::setProject(Project *project)
{
    model()->setProject(project);
    m_project = project;
}

bool NodeGanttViewBase::loadContext(const KoXmlElement &settings)
{
    treeView()->loadContext(model()->columnMap(), settings);

    KoXmlElement e = settings.namedItem("ganttchart").toElement();
    if (! e.isNull()) {
        m_ganttdelegate->showTaskLinks = (bool)(e.attribute("show-dependencies", "0").toInt());
        m_ganttdelegate->showTaskName = (bool)(e.attribute("show-taskname", "0").toInt());
        m_ganttdelegate->showResources = (bool)(e.attribute("show-resourcenames", "0").toInt());
        m_ganttdelegate->showProgress = (bool)(e.attribute("show-completion", "0").toInt());
        m_ganttdelegate->showCriticalPath = (bool)(e.attribute("show-criticalpath", "0").toInt());
        m_ganttdelegate->showCriticalTasks = (bool)(e.attribute("show-criticaltasks", "0").toInt());
        m_ganttdelegate->showPositiveFloat = (bool)(e.attribute("show-positivefloat", "0").toInt());
        m_ganttdelegate->showSchedulingError = (bool)(e.attribute("show-schedulingerror", "0").toInt());
        m_ganttdelegate->showTimeConstraint = (bool)(e.attribute("show-timeconstraint", "0").toInt());
        m_ganttdelegate->showNegativeFloat = (bool)(e.attribute("show-negativefloat", "0").toInt());

        GanttViewBase::loadContext(e);

        m_printOptions.loadContext(e);
    }
    return true;
}

void NodeGanttViewBase::saveContext(QDomElement &settings) const
{
    debugPlan;
    treeView()->saveContext(model()->columnMap(), settings);

    QDomElement e = settings.ownerDocument().createElement("ganttchart");
    settings.appendChild(e);
    e.setAttribute("show-dependencies", QString::number(m_ganttdelegate->showTaskLinks));
    e.setAttribute("show-taskname", QString::number(m_ganttdelegate->showTaskName));
    e.setAttribute("show-resourcenames", QString::number(m_ganttdelegate->showResources));
    e.setAttribute("show-completion", QString::number(m_ganttdelegate->showProgress));
    e.setAttribute("show-criticalpath", QString::number(m_ganttdelegate->showCriticalPath));
    e.setAttribute("show-criticaltasks",QString::number(m_ganttdelegate->showCriticalTasks));
    e.setAttribute("show-positivefloat", QString::number(m_ganttdelegate->showPositiveFloat));
    e.setAttribute("show-schedulingerror", QString::number(m_ganttdelegate->showSchedulingError));
    e.setAttribute("show-timeconstraint", QString::number(m_ganttdelegate->showTimeConstraint));
    e.setAttribute("show-negativefloat", QString::number(m_ganttdelegate->showNegativeFloat));

    GanttViewBase::saveContext(e);

    m_printOptions.saveContext(e);
}

void NodeGanttViewBase::setShowUnscheduledTasks(bool show)
{
    NodeSortFilterProxyModel *m = qobject_cast<NodeSortFilterProxyModel*>(KGantt::View::model());
    if (m) {
        m->setFilterUnscheduled(!show);
    }
}

//-------------------------------------------
MyKGanttView::MyKGanttView(QWidget *parent)
    : NodeGanttViewBase(parent),
    m_manager(nullptr)
{
    debugPlan<<"------------------- create MyKGanttView -----------------------";
    GanttItemModel *gm = new GanttItemModel(this);
    for (int i = 0; i < gm->columnCount(); ++i) {
        if (i != NodeModel::NodeCompleted) {
            gm->setReadOnly(i, true);
        }
    }
    setItemModel(gm);
    treeView()->createItemDelegates(gm);

    QList<int> show;
    show << NodeModel::NodeName
            << NodeModel::NodeCompleted
            << NodeModel::NodeStartTime
            << NodeModel::NodeEndTime;

    treeView()->setDefaultColumns(show);
    for (int i = 0; i < model()->columnCount(); ++i) {
        if (! show.contains(i)) {
            treeView()->hideColumn(i);
        }
    }

    setConstraintModel(new KGantt::ConstraintModel(this));
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

    connect(model(), &NodeItemModel::nodeInserted, this, &MyKGanttView::slotNodeInserted);
}

GanttItemModel *MyKGanttView::model() const
{
    return static_cast<GanttItemModel*>(NodeGanttViewBase::model());
}

void MyKGanttView::setProject(Project *proj)
{
    clearDependencies();
    if (project()) {
        disconnect(project(), &Project::relationToBeModified, this, &MyKGanttView::removeDependency);
        disconnect(project(), &Project::relationModified, this, &MyKGanttView::addDependency);
        disconnect(project(), &Project::relationAdded, this, &MyKGanttView::addDependency);
        disconnect(project(), &Project::relationToBeRemoved, this, &MyKGanttView::removeDependency);
        disconnect(project(), &Project::projectCalculated, this, &MyKGanttView::slotProjectCalculated);
    }
    NodeGanttViewBase::setProject(proj);
    if (proj) {
        connect(project(), &Project::relationToBeModified, this, &MyKGanttView::removeDependency);
        connect(project(), &Project::relationModified, this, &MyKGanttView::addDependency);
        connect(proj, &Project::relationAdded, this, &MyKGanttView::addDependency);
        connect(proj, &Project::relationToBeRemoved, this, &MyKGanttView::removeDependency);
        connect(proj, &Project::projectCalculated, this, &MyKGanttView::slotProjectCalculated);
    }

    createDependencies();
}

void MyKGanttView::slotProjectCalculated(ScheduleManager *sm)
{
    if (m_manager == sm) {
        setScheduleManager(sm);
    }
}

void MyKGanttView::setScheduleManager(ScheduleManager *sm)
{
    constraintModel()->clear();
    m_manager = sm;
    KGantt::DateTimeGrid *g = static_cast<KGantt::DateTimeGrid*>(grid());
    if (sm && project()) {
        QDateTime start = project()->startTime(sm->scheduleId());
        if (start.isValid() && g->startDateTime() != start) {
            g->setStartDateTime(start);
        }
    }
    if (! g->startDateTime().isValid()) {
        g->setStartDateTime(QDateTime::currentDateTime());
    }
    model()->setScheduleManager(sm);
    createDependencies();
    graphicsView()->updateScene();
}

void MyKGanttView::slotNodeInserted(Node *node)
{
    const QList<Relation*> relations = node->dependChildNodes();
    for (Relation *r : relations) {
        addDependency(r);
    }
    const QList<Relation*> relations2 = node->dependParentNodes();
    for (Relation *r : relations2) {
        addDependency(r);
    }
}

void MyKGanttView::addDependency(Relation *rel)
{
    QModelIndex par = sfModel()->mapFromSource(model()->index(rel->parent()));
    QModelIndex ch = sfModel()->mapFromSource(model()->index(rel->child()));
//    debugPlan<<"addDependency() "<<model()<<par.model();
    if (par.isValid() && ch.isValid()) {
        KGantt::Constraint con(par, ch, KGantt::Constraint::TypeSoft,
                                 static_cast<KGantt::Constraint::RelationType>(rel->type())/*NOTE!!*/
                               );
        if (! constraintModel()->hasConstraint(con)) {
            constraintModel()->addConstraint(con);
        }
    }
}

void MyKGanttView::removeDependency(Relation *rel)
{
    QModelIndex par = sfModel()->mapFromSource(model()->index(rel->parent()));
    QModelIndex ch = sfModel()->mapFromSource(model()->index(rel->child()));
    KGantt::Constraint con(par, ch, KGantt::Constraint::TypeSoft,
                             static_cast<KGantt::Constraint::RelationType>(rel->type())/*NOTE!!*/
                           );
    constraintModel()->removeConstraint(con);
}

void MyKGanttView::clearDependencies()
{
    constraintModel()->clear();
    // Remove old deps from view
    // NOTE: This should be handled by KGantt
    graphicsView()->updateScene();
}

void MyKGanttView::createDependencies()
{
    clearDependencies();
    if (project() == nullptr || m_manager == nullptr) {
        return;
    }
    const QList<Node*> nodes = project()->allNodes();
    for (Node* n : nodes) {
        const QList<Relation*> relations = n->dependChildNodes();
        for (Relation *r : relations) {
            addDependency(r);
        }
    }
}

//------------------------------------------
GanttView::GanttView(KoPart *part, KoDocument *doc, QWidget *parent, bool readWrite)
    : ViewBase(part, doc, parent),
    m_readWrite(readWrite),
    m_project(nullptr)
{
    debugPlan <<" ---------------- KPlato: Creating GanttView ----------------";

    setXMLFile("GanttViewUi.rc");

    QVBoxLayout *l = new QVBoxLayout(this);
    l->setMargin(0);
    m_splitter = new QSplitter(this);
    l->addWidget(m_splitter);
    m_splitter->setOrientation(Qt::Vertical);

    m_gantt = new MyKGanttView(m_splitter);
    m_gantt->graphicsView()->setHeaderContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &ViewBase::expandAll, m_gantt->treeView(), &TreeViewBase::slotExpand);
    connect(this, &ViewBase::collapseAll, m_gantt->treeView(), &TreeViewBase::slotCollapse);

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

    Help::add(this,
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
                     "</para>", Help::page("Task_Gantt_View")));
}

void GanttView::slotEditCopy()
{
    m_gantt->editCopy();
}

void GanttView::itemDoubleClicked(const QPersistentModelIndex &idx)
{
    if (idx.column() == NodeModel::NodeDescription) {
        emit openTaskDescription(isReadWrite() && (idx.flags() & Qt::ItemIsEditable));
    }
}

void GanttView::slotGanttHeaderContextMenuRequested(const QPoint &pt)
{
    QMenu *menu = popupMenu("gantt_datetimegrid_popup");
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
    // create context menu actions
    actionShowProject = new KToggleAction(i18n("Show Project"), this);
    actionCollection()->addAction("show_project", actionShowProject);
    // FIXME: Dependencies depend on these methods being called in the correct order
    connect(actionShowProject, &QAction::triggered, m_gantt, &MyKGanttView::clearDependencies);
    connect(actionShowProject, &QAction::triggered, m_gantt->model(), &NodeItemModel::setShowProject);
    connect(actionShowProject, &QAction::triggered, m_gantt, &MyKGanttView::createDependencies);
    addContextAction(actionShowProject);

    actionShowUnscheduled = new KToggleAction(i18n("Show Unscheduled Tasks"), this);
    actionCollection()->addAction("show_unscheduled_tasks", actionShowUnscheduled);
    connect(actionShowUnscheduled, &QAction::triggered, m_gantt, &MyKGanttView::setShowUnscheduledTasks);
    addContextAction(actionShowUnscheduled);

    createOptionActions(ViewBase::OptionAll);
    for (QAction *a : contextActionList()) {
        actionCollection()->addAction(a->objectName(), a);
    }

    m_scalegroup = new QActionGroup(this);
    QAction *a = new QAction(i18nc("@action:inmenu", "Auto"), this);
    a->setCheckable(true);
    a->setChecked(true);
    actionCollection()->addAction("scale_auto", a);
    connect(a, &QAction::triggered, this, &GanttView::ganttActions);
    m_scalegroup->addAction(a);

    a = new QAction(i18nc("@action:inmenu", "Month"), this);
    actionCollection()->addAction("scale_month", a);
    a->setCheckable(true);
    connect(a, &QAction::triggered, this, &GanttView::ganttActions);
    m_scalegroup->addAction(a);

    a = new QAction(i18nc("@action:inmenu", "Week"), this);
    actionCollection()->addAction("scale_week", a);
    a->setCheckable(true);
    connect(a, &QAction::triggered, this, &GanttView::ganttActions);
    m_scalegroup->addAction(a);

    a = new QAction(i18nc("@action:inmenu", "Day"), this);
    a->setCheckable(true);
    actionCollection()->addAction("scale_day", a);
    connect(a, &QAction::triggered, this, &GanttView::ganttActions);
    m_scalegroup->addAction(a);

    a = new QAction(i18nc("@action:inmenu", "Hour"), this);
    a->setCheckable(true);
    actionCollection()->addAction("scale_hour", a);
    connect(a, &QAction::triggered, this, &GanttView::ganttActions);
    m_scalegroup->addAction(a);

    a = new QAction(i18nc("@action:inmenu", "Zoom In"), this);
    a->setIcon(koIcon("zoom-in"));
    actionCollection()->addAction("zoom_in", a);
    connect(a, &QAction::triggered, this, &GanttView::ganttActions);

    a = new QAction(i18nc("@action:inmenu", "Zoom Out"), this);
    a->setIcon(koIcon("zoom-out"));
    actionCollection()->addAction("zoom_out", a);
    connect(a, &QAction::triggered, this, &GanttView::ganttActions);
}

void GanttView::slotDateTimeGridChanged()
{
    DateTimeGrid *grid = qobject_cast<DateTimeGrid*>(m_gantt->grid());
    Q_ASSERT(grid);
    if (!grid) {
        return;
    }
    QAction *a = m_scalegroup->checkedAction();
    switch (grid->scale()) {
        case KGantt::DateTimeGrid::ScaleAuto: actionCollection()->action("scale_auto")->setChecked(true); break;
        case KGantt::DateTimeGrid::ScaleHour: actionCollection()->action("scale_hour")->setChecked(true); break;
        case KGantt::DateTimeGrid::ScaleDay: actionCollection()->action("scale_day")->setChecked(true); break;
        case KGantt::DateTimeGrid::ScaleWeek: actionCollection()->action("scale_week")->setChecked(true); break;
        case KGantt::DateTimeGrid::ScaleMonth: actionCollection()->action("scale_month")->setChecked(true); break;
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
    if (action->objectName() == "scale_auto") {
        grid->setScale(DateTimeGrid::ScaleAuto);
    } else if (action->objectName() == "scale_month") {
        grid->setScale(DateTimeGrid::ScaleMonth);
    } else if (action->objectName() == "scale_week") {
        grid->setScale(DateTimeGrid::ScaleWeek);
    } else if (action->objectName() == "scale_day") {
        grid->setScale(DateTimeGrid::ScaleDay);
    } else if (action->objectName() == "scale_hour") {
        grid->setScale(DateTimeGrid::ScaleHour);
    } else if (action->objectName() == "zoom_in") {
        grid->setDayWidth(grid->dayWidth() * 1.25);
    } else if (action->objectName() == "zoom_out") {
        // daywidth *MUST NOT* go below 1.0, it is used as an integer later on
        grid->setDayWidth(qMax<qreal>(1.0, grid->dayWidth() * 0.8));
    } else {
        warnPlan<<"Unknown gantt action:"<<action;
    }
}

void GanttView::slotOptions()
{
    debugPlan;
    GanttViewSettingsDialog *dlg = new GanttViewSettingsDialog(m_gantt, m_gantt->delegate(), this, sender()->objectName() == "print_options");
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
    if (project) {
        connect(project, &Project::scheduleManagerChanged, this, &GanttView::setScheduleManager);
    }
}

void GanttView::setScheduleManager(ScheduleManager *sm)
{
    if (!sm && scheduleManager()) {
        // we should only get here if the only schedule manager is scheduled,
        // or when last schedule manager is deleted
        m_domdoc.clear();
        QDomElement element = m_domdoc.createElement("expanded");
        m_domdoc.appendChild(element);
        m_gantt->treeView()->saveExpanded(element);
    }
    bool tryexpand = sm && !scheduleManager();
    bool expand = sm && scheduleManager() && sm != scheduleManager();
    QDomDocument doc;
    if (expand) {
        QDomElement element = doc.createElement("expanded");
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
    m_gantt->treeView()->selectionModel()->setCurrentIndex(sidx, QItemSelectionModel::ClearAndSelect);

    Node *node = m_gantt->model()->node(m_gantt->sfModel()->mapToSource(sidx));
    if (node) {
        switch (node->type()) {
            case Node::Type_Project:
                name = "taskview_project_popup";
                break;
            case Node::Type_Task:
                name = "taskview_popup";
                break;
            case Node::Type_Milestone:
                name = "taskview_milestone_popup";
                break;
            case Node::Type_Summarytask:
                name = "taskview_summary_popup";
                break;
            default:
                break;
        }
    } else debugPlan<<"No node";

    m_gantt->treeView()->setContextMenuIndex(sidx);
    if (name.isEmpty()) {
        slotHeaderContextMenuRequested(pos);
        m_gantt->treeView()->setContextMenuIndex(QModelIndex());
        debugPlan<<"No menu";
        return;
    }
    emit requestPopupMenu(name, pos);
    m_gantt->treeView()->setContextMenuIndex(QModelIndex());
}

bool GanttView::loadContext(const KoXmlElement &settings)
{
    debugPlan;
    ViewBase::loadContext(settings);
    bool show = (bool)(settings.attribute("show-project", "0").toInt());
    actionShowProject->setChecked(show);
    m_gantt->model()->setShowProject(show); // why is this not called by the action?
    show = (bool)(settings.attribute("show-unscheduled", "1").toInt());
    actionShowUnscheduled->setChecked(show);
    m_gantt->setShowUnscheduledTasks(show);

    return m_gantt->loadContext(settings);
}

void GanttView::saveContext(QDomElement &settings) const
{
    debugPlan;
    ViewBase::saveContext(settings);
    settings.setAttribute("show-project", QString::number(actionShowProject->isChecked()));
    settings.setAttribute("show-unscheduled", QString::number(actionShowUnscheduled->isChecked()));

    m_gantt->saveContext(settings);

}

void GanttView::updateReadWrite(bool on)
{
    // TODO: KGanttView needs read/write mode
    m_readWrite = on;
    if (m_gantt->model()) {
        m_gantt->model()->setReadWrite(on);
    }
}

//----
MilestoneGanttViewSettingsDialog::MilestoneGanttViewSettingsDialog(GanttViewBase *gantt, ViewBase *view, bool selectPrint)
    : ItemViewSettupDialog(view, gantt->treeView(), true, view),
    m_gantt(gantt)
{
    QTabWidget *tab = new QTabWidget();
    QWidget *w = ViewBase::createPageLayoutWidget(view);
    tab->addTab(w, w->windowTitle());
    m_pagelayout = w->findChild<KoPageLayoutWidget*>();
    Q_ASSERT(m_pagelayout);

    m_printingoptions = new GanttPrintingOptionsWidget(this);
    m_printingoptions->setOptions(gantt->printingOptions());
    tab->addTab(m_printingoptions, m_printingoptions->windowTitle());
    KPageWidgetItem *page = insertWidget(-1, tab, i18n("Printing"), i18n("Printing Options"));
    if (selectPrint) {
        setCurrentPage(page);
    }
    connect(this, SIGNAL(accepted()), this, SLOT(slotOk()));
}

void MilestoneGanttViewSettingsDialog::slotOk()
{
    debugPlan;
    m_gantt->setPrintingOptions(m_printingoptions->options());
    ItemViewSettupDialog::slotOk();
}

//------------------------
MilestoneKGanttView::MilestoneKGanttView(QWidget *parent)
    : NodeGanttViewBase(parent),
    m_manager(nullptr)
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

    sfModel()->setFilterRole (Qt::EditRole);
    sfModel()->setFilterFixedString(QString::number(Node::Type_Milestone));
    sfModel()->setFilterKeyColumn(NodeModel::NodeType);

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

    Help::add(this,
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
                     "</para>", Help::page("Milestone_Gantt_View")));
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

    setXMLFile("GanttViewUi.rc");

    QVBoxLayout *l = new QVBoxLayout(this);
    l->setMargin(0);
    m_splitter = new QSplitter(this);
    l->addWidget(m_splitter);
    m_splitter->setOrientation(Qt::Vertical);

    setupGui();

    m_gantt = new MilestoneKGanttView(m_splitter);
    m_gantt->graphicsView()->setHeaderContextMenuPolicy(Qt::CustomContextMenu);

    m_showTaskName = false; // FIXME
    m_showProgress = false; //FIXME
    m_showPositiveFloat = false; //FIXME
    m_showCriticalTasks = false; //FIXME
    m_showNoInformation = false; //FIXME

    updateReadWrite(readWrite);

    connect(m_gantt->treeView(), &TreeViewBase::contextMenuRequested, this, &MilestoneGanttView::slotContextMenuRequested);
    connect(m_gantt, &MyKGanttView::contextMenuRequested, this, &MilestoneGanttView::slotContextMenuRequested);

    connect(m_gantt->treeView(), &TreeViewBase::headerContextMenuRequested, this, &ViewBase::slotHeaderContextMenuRequested);
    connect(m_gantt->graphicsView(), &KGantt::GraphicsView::headerContextMenuRequested, this, &MilestoneGanttView::slotGanttHeaderContextMenuRequested);
    connect(qobject_cast<KGantt::DateTimeGrid*>(m_gantt->graphicsView()->grid()), &KGantt::DateTimeGrid::gridChanged, this, &MilestoneGanttView::slotDateTimeGridChanged);

    connect(m_gantt->treeView(), &GanttTreeView::doubleClicked, this, &MilestoneGanttView::itemDoubleClicked);
}

void MilestoneGanttView::slotEditCopy()
{
    m_gantt->editCopy();
}

void MilestoneGanttView::itemDoubleClicked(const QPersistentModelIndex &idx)
{
    if (idx.column() == NodeModel::NodeDescription) {
        emit openTaskDescription(isReadWrite() && (idx.flags() & Qt::ItemIsEditable));
    }
}

void MilestoneGanttView::slotGanttHeaderContextMenuRequested(const QPoint &pt)
{
    QMenu *menu = popupMenu("gantt_datetimegrid_popup");
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

    for (QAction *a : contextActionList()) {
        actionCollection()->addAction(a->objectName(), a);
    }

    m_scalegroup = new QActionGroup(this);
    QAction *a = new QAction(i18nc("@action:inmenu", "Auto"), this);
    a->setCheckable(true);
    a->setChecked(true);
    actionCollection()->addAction("scale_auto", a);
    connect(a, &QAction::triggered, this, &MilestoneGanttView::ganttActions);
    m_scalegroup->addAction(a);

    a = new QAction(i18nc("@action:inmenu", "Month"), this);
    actionCollection()->addAction("scale_month", a);
    a->setCheckable(true);
    connect(a, &QAction::triggered, this, &MilestoneGanttView::ganttActions);
    m_scalegroup->addAction(a);

    a = new QAction(i18nc("@action:inmenu", "Week"), this);
    actionCollection()->addAction("scale_week", a);
    a->setCheckable(true);
    connect(a, &QAction::triggered, this, &MilestoneGanttView::ganttActions);
    m_scalegroup->addAction(a);

    a = new QAction(i18nc("@action:inmenu", "Day"), this);
    a->setCheckable(true);
    actionCollection()->addAction("scale_day", a);
    connect(a, &QAction::triggered, this, &MilestoneGanttView::ganttActions);
    m_scalegroup->addAction(a);

    a = new QAction(i18nc("@action:inmenu", "Hour"), this);
    a->setCheckable(true);
    actionCollection()->addAction("scale_hour", a);
    connect(a, &QAction::triggered, this, &MilestoneGanttView::ganttActions);
    m_scalegroup->addAction(a);

    a = new QAction(i18nc("@action:inmenu", "Zoom In"), this);
    a->setIcon(koIcon("zoom-in"));
    actionCollection()->addAction("zoom_in", a);
    connect(a, &QAction::triggered, this, &MilestoneGanttView::ganttActions);

    a = new QAction(i18nc("@action:inmenu", "Zoom Out"), this);
    a->setIcon(koIcon("zoom-out"));
    actionCollection()->addAction("zoom_out", a);
    connect(a, &QAction::triggered, this, &MilestoneGanttView::ganttActions);
}

void MilestoneGanttView::slotDateTimeGridChanged()
{
    DateTimeGrid *grid = qobject_cast<DateTimeGrid*>(m_gantt->grid());
    Q_ASSERT(grid);
    if (!grid) {
        return;
    }
    QAction *a = m_scalegroup->checkedAction();
    switch (grid->scale()) {
        case KGantt::DateTimeGrid::ScaleAuto: actionCollection()->action("scale_auto")->setChecked(true); break;
        case KGantt::DateTimeGrid::ScaleHour: actionCollection()->action("scale_hour")->setChecked(true); break;
        case KGantt::DateTimeGrid::ScaleDay: actionCollection()->action("scale_day")->setChecked(true); break;
        case KGantt::DateTimeGrid::ScaleWeek: actionCollection()->action("scale_week")->setChecked(true); break;
        case KGantt::DateTimeGrid::ScaleMonth: actionCollection()->action("scale_month")->setChecked(true); break;
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
    if (action->objectName() == "scale_auto") {
        grid->setScale(DateTimeGrid::ScaleAuto);
    } else if (action->objectName() == "scale_month") {
        grid->setScale(DateTimeGrid::ScaleMonth);
    } else if (action->objectName() == "scale_week") {
        grid->setScale(DateTimeGrid::ScaleWeek);
    } else if (action->objectName() == "scale_day") {
        grid->setScale(DateTimeGrid::ScaleDay);
    } else if (action->objectName() == "scale_hour") {
        grid->setScale(DateTimeGrid::ScaleHour);
    } else if (action->objectName() == "zoom_in") {
        grid->setDayWidth(grid->dayWidth() * 1.25);
    } else if (action->objectName() == "zoom_out") {
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
                name = "taskview_popup";
                break;
            case Node::Type_Milestone:
                name = "taskview_milestone_popup";
                break;
            case Node::Type_Summarytask:
                name = "taskview_summary_popup";
                break;
            default:
                break;
        }
    } else debugPlan<<"No node";
    m_gantt->treeView()->setContextMenuIndex(idx);
    if (name.isEmpty()) {
        debugPlan<<"No menu";
        slotHeaderContextMenuRequested(pos);
        m_gantt->treeView()->setContextMenuIndex(QModelIndex());
        return;
    }
    emit requestPopupMenu(name, pos);
    m_gantt->treeView()->setContextMenuIndex(QModelIndex());
}

void MilestoneGanttView::slotOptions()
{
    debugPlan;
    MilestoneGanttViewSettingsDialog *dlg =  new MilestoneGanttViewSettingsDialog(m_gantt, this, sender()->objectName() == "print_options");
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
    if (m_gantt->model()) {
        m_gantt->model()->setReadWrite(on);
    }
}

KoPrintJob *MilestoneGanttView::createPrintJob()
{
    return new GanttPrintingDialog(this, m_gantt);
}

//--------------------
ResourceAppointmentsGanttViewSettingsDialog::ResourceAppointmentsGanttViewSettingsDialog(GanttViewBase *gantt,  ViewBase *view, bool selectPrint)
    : ItemViewSettupDialog(view, gantt->treeView(), true, view)
    , m_gantt(gantt)
{
    QTabWidget *tab = new QTabWidget();
    QWidget *w = ViewBase::createPageLayoutWidget(view);
    tab->addTab(w, w->windowTitle());
    m_pagelayout = w->findChild<KoPageLayoutWidget*>();
    Q_ASSERT(m_pagelayout);
    m_printingoptions = new GanttPrintingOptionsWidget(this);
    m_printingoptions->setOptions(gantt->printingOptions());
    tab->addTab(m_printingoptions, m_printingoptions->windowTitle());
    KPageWidgetItem *page = insertWidget(-1, tab, i18n("Printing"), i18n("Printing Options"));
    if (selectPrint) {
        setCurrentPage(page);
    }
    connect(this, SIGNAL(accepted()), this, SLOT(slotOk()));
}

void ResourceAppointmentsGanttViewSettingsDialog::slotOk()
{
    debugPlan;
    m_gantt->setPrintingOptions(m_printingoptions->options());
    ItemViewSettupDialog::slotOk();
}

//------------------------------------------

ResourceAppointmentsGanttView::ResourceAppointmentsGanttView(KoPart *part, KoDocument *doc, QWidget *parent, bool readWrite)
    : ViewBase(part, doc, parent),
    m_project(nullptr),
    m_model(new ResourceAppointmentsGanttModel(this))
{
    debugPlan <<" ---------------- KPlato: Creating ResourceAppointmentsGanttView ----------------";

    setXMLFile("GanttViewUi.rc");

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
    l->setMargin(0);
    l->addWidget(m_gantt);

    setupGui();

    updateReadWrite(readWrite);

    connect(m_gantt->leftView(), SIGNAL(contextMenuRequested(QModelIndex,QPoint,QModelIndexList)), SLOT(slotContextMenuRequested(QModelIndex,QPoint)));
    connect(m_gantt, &GanttViewBase::contextMenuRequested, this, &ResourceAppointmentsGanttView::slotContextMenuRequestedFromGantt);

    connect(m_gantt->leftView(), SIGNAL(headerContextMenuRequested(QPoint)), SLOT(slotHeaderContextMenuRequested(QPoint)));
    connect(m_gantt->graphicsView(), &KGantt::GraphicsView::headerContextMenuRequested, this, &ResourceAppointmentsGanttView::slotGanttHeaderContextMenuRequested);
    connect(qobject_cast<KGantt::DateTimeGrid*>(m_gantt->graphicsView()->grid()), &KGantt::DateTimeGrid::gridChanged, this, &ResourceAppointmentsGanttView::slotDateTimeGridChanged);

    Help::add(this,
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
                     "</para>", Help::page("Resource_Assignment_Gantt_View")));
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
    QMenu *menu = popupMenu("gantt_datetimegrid_popup");
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
    m_model->setProject(project);
}

void ResourceAppointmentsGanttView::setScheduleManager(ScheduleManager *sm)
{
    //debugPlan<<id<<'\n';
    if (!sm && scheduleManager()) {
        // we should only get here if the only schedule manager is scheduled,
        // or when last schedule manager is deleted
        m_domdoc.clear();
        QDomElement element = m_domdoc.createElement("expanded");
        m_domdoc.appendChild(element);
        treeView()->saveExpanded(element);
    }
    bool tryexpand = sm && !scheduleManager();
    bool expand = sm && scheduleManager() && sm != scheduleManager();
    QDomDocument doc;
    if (expand) {
        QDomElement element = doc.createElement("expanded");
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

    for (QAction *a : contextActionList()) {
        actionCollection()->addAction(a->objectName(), a);
    }

    m_scalegroup = new QActionGroup(this);
    QAction *a = new QAction(i18nc("@action:inmenu", "Auto"), this);
    a->setCheckable(true);
    a->setChecked(true);
    actionCollection()->addAction("scale_auto", a);
    connect(a, &QAction::triggered, this, &ResourceAppointmentsGanttView::ganttActions);
    m_scalegroup->addAction(a);

    a = new QAction(i18nc("@action:inmenu", "Month"), this);
    actionCollection()->addAction("scale_month", a);
    a->setCheckable(true);
    connect(a, &QAction::triggered, this, &ResourceAppointmentsGanttView::ganttActions);
    m_scalegroup->addAction(a);

    a = new QAction(i18nc("@action:inmenu", "Week"), this);
    actionCollection()->addAction("scale_week", a);
    a->setCheckable(true);
    connect(a, &QAction::triggered, this, &ResourceAppointmentsGanttView::ganttActions);
    m_scalegroup->addAction(a);

    a = new QAction(i18nc("@action:inmenu", "Day"), this);
    a->setCheckable(true);
    actionCollection()->addAction("scale_day", a);
    connect(a, &QAction::triggered, this, &ResourceAppointmentsGanttView::ganttActions);
    m_scalegroup->addAction(a);

    a = new QAction(i18nc("@action:inmenu", "Hour"), this);
    a->setCheckable(true);
    actionCollection()->addAction("scale_hour", a);
    connect(a, &QAction::triggered, this, &ResourceAppointmentsGanttView::ganttActions);
    m_scalegroup->addAction(a);

    a = new QAction(i18nc("@action:inmenu", "Zoom In"), this);
    a->setIcon(koIcon("zoom-in"));
    actionCollection()->addAction("zoom_in", a);
    connect(a, &QAction::triggered, this, &ResourceAppointmentsGanttView::ganttActions);

    a = new QAction(i18nc("@action:inmenu", "Zoom Out"), this);
    a->setIcon(koIcon("zoom-out"));
    actionCollection()->addAction("zoom_out", a);
    connect(a, &QAction::triggered, this, &ResourceAppointmentsGanttView::ganttActions);
}

void ResourceAppointmentsGanttView::slotDateTimeGridChanged()
{
    DateTimeGrid *grid = qobject_cast<DateTimeGrid*>(m_gantt->grid());
    Q_ASSERT(grid);
    if (!grid) {
        return;
    }
    QAction *a = m_scalegroup->checkedAction();
    switch (grid->scale()) {
        case KGantt::DateTimeGrid::ScaleAuto: actionCollection()->action("scale_auto")->setChecked(true); break;
        case KGantt::DateTimeGrid::ScaleHour: actionCollection()->action("scale_hour")->setChecked(true); break;
        case KGantt::DateTimeGrid::ScaleDay: actionCollection()->action("scale_day")->setChecked(true); break;
        case KGantt::DateTimeGrid::ScaleWeek: actionCollection()->action("scale_week")->setChecked(true); break;
        case KGantt::DateTimeGrid::ScaleMonth: actionCollection()->action("scale_month")->setChecked(true); break;
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
    if (action->objectName() == "scale_auto") {
        grid->setScale(DateTimeGrid::ScaleAuto);
    } else if (action->objectName() == "scale_month") {
        grid->setScale(DateTimeGrid::ScaleMonth);
    } else if (action->objectName() == "scale_week") {
        grid->setScale(DateTimeGrid::ScaleWeek);
    } else if (action->objectName() == "scale_day") {
        grid->setScale(DateTimeGrid::ScaleDay);
    } else if (action->objectName() == "scale_hour") {
        grid->setScale(DateTimeGrid::ScaleHour);
    } else if (action->objectName() == "zoom_in") {
        grid->setDayWidth(grid->dayWidth() * 1.25);
    } else if (action->objectName() == "zoom_out") {
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
            name = "taskview_popup";
        }
    }
    m_gantt->treeView()->setContextMenuIndex(idx);
    if (name.isEmpty()) {
        slotHeaderContextMenuRequested(pos);
        m_gantt->treeView()->setContextMenuIndex(QModelIndex());
        return;
    }
    emit requestPopupMenu(name, pos);
    m_gantt->treeView()->setContextMenuIndex(QModelIndex());
}

void ResourceAppointmentsGanttView::slotOptions()
{
    debugPlan;
    ItemViewSettupDialog *dlg = new ResourceAppointmentsGanttViewSettingsDialog(m_gantt, this, sender()->objectName() == "print_options");
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

}  //KPlato namespace

#include "moc_kptganttview.cpp"
