/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2006-2010, 2012 Dag Andersen <dag.andersen@kdemail.net>
  SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kpttaskeditor.h"

#include "kptglobal.h"
#include "kptcommonstrings.h"
#include "kptnodeitemmodel.h"
#include "kptcommand.h"
#include "kptproject.h"
#include <kpttask.h>
#include "kptitemviewsettup.h"
#include "kptworkpackagesenddialog.h"
#include "kptworkpackagesendpanel.h"
#include "kptdatetime.h"
#include "kptdebug.h"
#include <ResourceItemModel.h>
#include <AllocatedResourceItemModel.h>
#include "kptresourceallocationmodel.h"
#include "ResourceAllocationView.h"
#include "kpttaskdialog.h"
#include "TasksEditController.h"
#include "kpttaskdescriptiondialog.h"
#include "RelationEditorDialog.h"
#include "kpttaskdialog.h"
#include "kptsummarytaskdialog.h"
#include "kpttaskdescriptiondialog.h"
#include "kpttaskprogressdialog.h"
#include "kptmilestoneprogressdialog.h"
#include "kptdocumentsdialog.h"
#include "TasksEditDialog.h"
#include "TaskSplitDialog.h"

#include <KoXmlReader.h>
#include <KoDocument.h>
#include <KoIcon.h>
#include <KoResourcePaths.h>

#include <QItemSelectionModel>
#include <QModelIndex>
#include <QVBoxLayout>
#include <QDragMoveEvent>
#include <QAction>
#include <QHeaderView>
#include <QMenu>
#include <QClipboard>
#include <QDir>

#include <KActionMenu>
#include <KLocalizedString>
#include <KToggleAction>
#include <KActionCollection>
#include <KMessageBox>

namespace KPlato
{

//--------------------
TaskEditorItemModel::TaskEditorItemModel(QObject *parent)
: NodeItemModel(parent)
{
}

void TaskEditorItemModel::setScheduleManager(ScheduleManager *sm)
{
    if (sm != manager()) {
        NodeItemModel::setScheduleManager(sm);
    }
}

Qt::ItemFlags TaskEditorItemModel::flags(const QModelIndex &index) const
{
    if (index.column() == NodeModel::NodeType) {
        if (! m_readWrite || isColumnReadOnly(index.column())) {
            return QAbstractItemModel::flags(index);
        }
        Node *n = node(index);
        bool baselined = n ? n->isBaselined() : false;
        if (n && ! baselined && (n->type() == Node::Type_Task || n->type() == Node::Type_Milestone)) {
            return QAbstractItemModel::flags(index) | Qt::ItemIsEditable | Qt::ItemIsDropEnabled;
        }
        return QAbstractItemModel::flags(index) | Qt::ItemIsDropEnabled;
    }
    return NodeItemModel::flags(index);
}

QVariant TaskEditorItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && section == NodeModel::NodeType) {
        if (role == Qt::ToolTipRole) {
            return xi18nc("@info:tooltip", "The type of task or the estimate type of the task");
        } else if (role == Qt::WhatsThisRole) {
            return xi18nc("@info:whatsthis",
                          "<p>Indicates the type of task or the estimate type of the task.</p>"
                          "The type can be set to <emphasis>Milestone</emphasis>, <emphasis>Effort</emphasis> or <emphasis>Duration</emphasis>.<nl/>"
                          "<note>If the type is <emphasis>Summary</emphasis> or <emphasis>Project</emphasis> the type is not editable.</note>");
        }
    }
    return NodeItemModel::headerData(section, orientation, role);
}

QVariant TaskEditorItemModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::TextAlignmentRole) {
        return NodeItemModel::data(index, role);
    }
    Node *n = node(index);
    if (n != nullptr && index.column() == NodeModel::NodeType) {
        return type(n, role);
    }
    return NodeItemModel::data(index, role);
}

bool TaskEditorItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Node *n = node(index);
    if (n != nullptr && role == Qt::EditRole && index.column() == NodeModel::NodeType) {
        return setType(n, value, role);
    }
    return NodeItemModel::setData(index, value, role);
}

QVariant TaskEditorItemModel::type(const Node *node, int role) const
{
    switch (role) {
        case Qt::DisplayRole: {
            if (node->type() == Node::Type_Task) {
                return node->estimate()->typeToString(true);
            }
            return node->typeToString(true);
        }
        case Qt::EditRole:
            return node->type();
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::ToolTipRole: {
            if (node->type() == Node::Type_Task) {
                return xi18nc("@info:tooltip", "Task with estimate type: %1", node->estimate()->typeToString(true));
            }
            return xi18nc("@info:tooltip", "Task type: %1", node->typeToString(true));
        }
        case Qt::StatusTipRole:
        case Qt::WhatsThisRole:
            return QVariant();
        case Role::EnumListValue: {
            if (node->type() == Node::Type_Milestone) {
                return 0;
            }
            if (node->type() == Node::Type_Task) {
                return node->estimate()->type() + 1;
            }
            return -1;
        }
        case Role::EnumList: {
            QStringList lst;
            lst << Node::typeToString(Node::Type_Milestone, true);
            lst += Estimate::typeToStringList(true);
            return lst;
        }
    }
    return QVariant();
}

bool TaskEditorItemModel::setType(Node *node, const QVariant &value, int role)
{
    switch (role) {
        case Qt::EditRole: {
            if (node->type() == Node::Type_Summarytask) {
                return false;
            }
            int v = value.toInt();
            switch (v) {
                case 0: { // Milestone
                    NamedCommand *cmd = nullptr;
                    if (node->constraint() == Node::FixedInterval) {
                        cmd = new NodeModifyConstraintEndTimeCmd(*node, node->constraintStartTime(), kundo2_i18n("Set type to Milestone"));
                    } else {
                        cmd =  new ModifyEstimateCmd(*node, node->estimate()->expectedEstimate(), 0.0, kundo2_i18n("Set type to Milestone"));
                    }
                    Q_EMIT executeCommand(cmd);
                    return true;
                }
                default: { // Estimate
                    --v;
                    MacroCommand *m = new MacroCommand(kundo2_i18n("Set type to %1", Estimate::typeToString((Estimate::Type)v, true)));
                    m->addCommand(new ModifyEstimateTypeCmd(*node, node->estimate()->type(), v));
                    if (node->type() == Node::Type_Milestone) {
                        if (node->constraint() == Node::FixedInterval) {
                            m->addCommand(new NodeModifyConstraintEndTimeCmd(*node, node->constraintStartTime().addDays(1)));
                        } else {
                            m->addCommand(new ModifyEstimateUnitCmd(*node, node->estimate()->unit(), Duration::Unit_d));
                            m->addCommand(new ModifyEstimateCmd(*node, node->estimate()->expectedEstimate(), 1.0));
                        }
                    }
                    Q_EMIT executeCommand(m);
                    return true;
                }
            }
            break;
        }
        default: break;
    }
    return false;
}

//--------------------
TaskEditorTreeView::TaskEditorTreeView(QWidget *parent)
    : DoubleTreeViewBase(parent)
{
    setDragPixmap(koIcon("view-task").pixmap(32));
    TaskEditorItemModel *m = new TaskEditorItemModel(this);
    setModel(m);
    //setSelectionBehavior(QAbstractItemView::SelectItems);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setDefaultDropAction(Qt::CopyAction);

    createItemDelegates(m);
    setItemDelegateForColumn(NodeModel::NodeType, new EnumDelegate(this));

    connect(this, &DoubleTreeViewBase::dropAllowed, this, &TaskEditorTreeView::slotDropAllowed);
}

NodeItemModel *TaskEditorTreeView::baseModel() const
{
    NodeSortFilterProxyModel *pr = proxyModel();
    if (pr) {
        return static_cast<NodeItemModel*>(pr->sourceModel());
    }
    return static_cast<NodeItemModel*>(model());
}

void TaskEditorTreeView::slotDropAllowed(const QModelIndex &index, int dropIndicatorPosition, QDragMoveEvent *event)
{
    QModelIndex idx = index;
    NodeSortFilterProxyModel *pr = proxyModel();
    if (pr) {
        idx = pr->mapToSource(index);
    }
    event->ignore();
    if (baseModel()->dropAllowed(idx, dropIndicatorPosition, event->mimeData())) {
        event->accept();
    }
}

void TaskEditorTreeView::editPaste()
{
    QModelIndex idx = m_leftview->currentIndex();
    const QMimeData *data = QGuiApplication::clipboard()->mimeData();
    model()->dropMimeData(data, Qt::CopyAction, idx.row()+1, 0, idx.parent());
}

//--------------------
NodeTreeView::NodeTreeView(QWidget *parent)
    : DoubleTreeViewBase(parent)
{
    setDragPixmap(koIcon("view-task").pixmap(32));
    NodeItemModel *m = new NodeItemModel(this);
    setModel(m);
    //setSelectionBehavior(QAbstractItemView::SelectItems);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);

    createItemDelegates(m);

    connect(this, &DoubleTreeViewBase::dropAllowed, this, &NodeTreeView::slotDropAllowed);
}

NodeItemModel *NodeTreeView::baseModel() const
{
    NodeSortFilterProxyModel *pr = proxyModel();
    if (pr) {
        return static_cast<NodeItemModel*>(pr->sourceModel());
    }
    return static_cast<NodeItemModel*>(model());
}

void NodeTreeView::slotDropAllowed(const QModelIndex &index, int dropIndicatorPosition, QDragMoveEvent *event)
{
    QModelIndex idx = index;
    NodeSortFilterProxyModel *pr = proxyModel();
    if (pr) {
        idx = pr->mapToSource(index);
    }
    event->ignore();
    if (baseModel()->dropAllowed(idx, dropIndicatorPosition, event->mimeData())) {
        event->accept();
    }
}


//-----------------------------------
TaskEditor::TaskEditor(KoPart *part, KoDocument *doc, QWidget *parent)
    : ViewBase(part, doc, parent)
{
    debugPlan<<"----------------- Create TaskEditor ----------------------";
    setXMLFile(QStringLiteral("TaskEditorUi.rc"));

    QVBoxLayout * l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    m_view = new TaskEditorTreeView(this);
    m_doubleTreeView = m_view;
    connect(this, &ViewBase::expandAll, m_view, &DoubleTreeViewBase::slotExpand);
    connect(this, &ViewBase::collapseAll, m_view, &DoubleTreeViewBase::slotCollapse);

    l->addWidget(m_view);
    debugPlan<<m_view->actionSplitView();
    setupGui();

    m_view->setEditTriggers(m_view->editTriggers() | QAbstractItemView::EditKeyPressed);

    m_view->setDragDropMode(QAbstractItemView::DragDrop);
    m_view->setDropIndicatorShown(true);
    m_view->setDragEnabled (true);
    m_view->setAcceptDrops(true);
    m_view->setAcceptDropsOnView(true);

    QList<int> lst1; lst1 << 1 << -1; // only display column 0 (NodeName) in left view
    QList<int> show;
    show << NodeModel::NodeResponsible
            << NodeModel::NodeAllocation
            << NodeModel::NodeType
            << NodeModel::NodePriority
            << NodeModel::NodeEstimateCalendar
            << NodeModel::NodeEstimate
            << NodeModel::NodeOptimisticRatio
            << NodeModel::NodePessimisticRatio
            << NodeModel::NodeRisk
            << NodeModel::NodeConstraint
            << NodeModel::NodeConstraintStart
            << NodeModel::NodeConstraintEnd
            << NodeModel::NodeRunningAccount
            << NodeModel::NodeStartupAccount
            << NodeModel::NodeStartupCost
            << NodeModel::NodeShutdownAccount
            << NodeModel::NodeShutdownCost
            << NodeModel::NodeDescription;

    QList<int> lst2;
    for (int i = 0; i < model()->columnCount(); ++i) {
        if (! show.contains(i)) {
            lst2 << i;
        }
    }
    for (int i = 0; i < show.count(); ++i) {
        int sec = m_view->slaveView()->header()->visualIndex(show[ i ]);
        //debugPlan<<"move section:"<<i<<show[i]<<sec;
        if (i != sec) {
            m_view->slaveView()->header()->moveSection(sec, i);
        }
    }
    m_view->hideColumns(lst1, lst2);
    m_view->masterView()->setDefaultColumns(QList<int>() << NodeModel::NodeName);
    m_view->slaveView()->setDefaultColumns(show);

    connect(model(), SIGNAL(executeCommand(KUndo2Command*)), doc, SLOT(addCommand(KUndo2Command*)));

    connect(m_view, &DoubleTreeViewBase::currentChanged, this, &TaskEditor::slotCurrentChanged);

    connect(m_view, &DoubleTreeViewBase::selectionChanged, this, &TaskEditor::slotSelectionChanged);

    connect(m_view, &DoubleTreeViewBase::contextMenuRequested, this, &TaskEditor::slotContextMenuRequested);

    connect(m_view, &DoubleTreeViewBase::headerContextMenuRequested, this, &ViewBase::slotHeaderContextMenuRequested);
    connect(m_view->masterView(), &TreeViewBase::doubleClicked, this, &TaskEditor::itemDoubleClicked);
    connect(m_view->slaveView(), &TreeViewBase::doubleClicked, this, &TaskEditor::itemDoubleClicked);

    connect(baseModel(), &NodeItemModel::projectShownChanged, this, &TaskEditor::slotProjectShown);
    connect(model(), &QAbstractItemModel::rowsMoved, this, &TaskEditor::slotEnableActions);

    setWhatsThis(
              xi18nc("@info:whatsthis",
                     "<title>Task Editor</title>"
                     "<para>"
                     "The Task Editor is used to create, edit, and delete tasks. "
                     "Tasks are organized into a Work Breakdown Structure (WBS) to any depth."
                     "</para><para>"
                     "This view supports configuration and printing using the context menu."
                     "<nl/><link url='%1'>More...</link>"
                     "</para>", QStringLiteral("plan:task-editor")));
}

void TaskEditor::itemDoubleClicked(const QPersistentModelIndex &idx)
{
    if (idx.column() == NodeModel::NodeDescription) {
        auto i = idx;
        QAbstractProxyModel *pr = proxyModel();
        while (pr) {
            i = pr->mapToSource(i);
            pr = qobject_cast<QAbstractProxyModel*>(pr->sourceModel());
        }
        Node *node = m_view->baseModel()->node(i);
        if (node) {
            auto action = actionCollection()->action(QStringLiteral("task_description"));
            if (action) {
                if (node->type() == Node::Type_Project) {
                    slotOpenProjectDescription();
                } else {
                    action->trigger();
                }
            }
        }
    }
}

void TaskEditor::slotProjectShown(bool on)
{
    debugPlan<<proxyModel();
    QModelIndex idx;
    if (proxyModel()) {
        if (proxyModel()->rowCount() > 0) {
            idx = proxyModel()->index(0, 0);
            m_view->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
        }
    } else if (baseModel() && baseModel()->rowCount() > 0) {
        idx = baseModel()->index(0, 0);
        m_view->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
    }
    if (on && idx.isValid()) {
        m_view->masterView()->expand(idx);
    }
    slotEnableActions();
}

void TaskEditor::updateReadWrite(bool rw)
{
    m_view->setReadWrite(rw);
    ViewBase::updateReadWrite(rw);
    slotEnableActions();
}

void TaskEditor::setProject(Project *project_)
{
    debugPlan<<project_;
    m_view->setProject(project_);
    ViewBase::setProject(project_);
}

void TaskEditor::createDockers()
{
    // Add dockers
    DockWidget *ds = nullptr;
    {
        ds = new DockWidget(this, QStringLiteral("Allocations"), xi18nc("@title resource allocations", "Allocations"));
        QTreeView *x = new QTreeView(ds);
        AllocatedResourceItemModel *m1 = new AllocatedResourceItemModel(x);
        x->setModel(m1);
        m1->setProject(project());
        x->setRootIsDecorated(false);
        x->setSelectionBehavior(QAbstractItemView::SelectRows);
        x->setSelectionMode(QAbstractItemView::ExtendedSelection);
        x->expandAll();
        x->resizeColumnToContents(0);
        x->setDragDropMode(QAbstractItemView::DragOnly);
        x->setDragEnabled (true);
        ds->setWidget(x);
        connect(this, &ViewBase::projectChanged, m1, &AllocatedResourceItemModel::setProject);
        connect(this, &TaskEditor::taskSelected, m1, &AllocatedResourceItemModel::setTask);
        connect(m1, &AllocatedResourceItemModel::expandAll, x, &QTreeView::expandAll);
        connect(m1, &AllocatedResourceItemModel::resizeColumnToContents, x, &QTreeView::resizeColumnToContents);
        addDocker(ds);
    }

    {
        ds = new DockWidget(this, QStringLiteral("Resources"), xi18nc("@title", "Resources"));
        ds->setToolTip(xi18nc("@info:tooltip",
                            "Drag resources into the Task Editor"
                            " and drop into the allocations- or responsible column"));
        ResourceAllocationView *e = new ResourceAllocationView(part(), ds);
        ResourceItemModel *m = new ResourceItemModel(e);
        m->setReadWrite(false);
        m->setIsCheckable(false);
        m->setProject(project());
        e->setModel(m);
        QList<int> show; show << ResourceModel::ResourceName;
        for (int i = m->columnCount() - 1; i >= 0; --i) {
            e->setColumnHidden(i, ! show.contains(i));
        }
        e->setRootIsDecorated(false);
        e->setHeaderHidden(true);
        e->setSelectionBehavior(QAbstractItemView::SelectRows);
        e->setSelectionMode(QAbstractItemView::ExtendedSelection);
        e->expandAll();
        e->resizeColumnToContents(ResourceModel::ResourceName);
        e->setDragDropMode(QAbstractItemView::DragOnly);
        e->setDragEnabled (true);
        ds->setWidget(e);
        connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged, e, &ResourceAllocationView::setSelectedTasks);
        connect(this, SIGNAL(projectChanged(KPlato::Project*)), m, SLOT(setProject(KPlato::Project*)));
        connect(m, &ItemModelBase::executeCommand, part(), &KoDocument::addCommand);
        addDocker(ds);
    }

    {
        ds = new DockWidget(this, QStringLiteral("Taskmodules"), xi18nc("@title", "Task Modules"));
        ds->setToolTip(xi18nc("@info:tooltip", "Drag a task module into the <emphasis>Task Editor</emphasis> to add it to the project"));
        ds->setLocation(Qt::LeftDockWidgetArea);
        ds->setShown(false); // hide by default
        QTreeView *e = new QTreeView(ds);
        QSortFilterProxyModel *sf = new QSortFilterProxyModel(e);
        TaskModuleModel *m = new TaskModuleModel(sf);
        connect(this, &TaskEditor::projectChanged, m, &TaskModuleModel::setProject);
        sf->setSourceModel(m);
        e->setModel(sf);
        e->sortByColumn(0, Qt::AscendingOrder);
        e->setSortingEnabled(true);
        e->setHeaderHidden(true);
        e->setRootIsDecorated(false);
        e->setSelectionBehavior(QAbstractItemView::SelectRows);
        e->setSelectionMode(QAbstractItemView::SingleSelection);
//         e->resizeColumnToContents(0);
        e->setDragDropMode(QAbstractItemView::DragDrop);
        e->setAcceptDrops(true);
        e->setDragEnabled (true);
        ds->setWidget(e);
        connect(e, &QAbstractItemView::doubleClicked, this, &TaskEditor::taskModuleDoubleClicked);
        connect(m, &TaskModuleModel::saveTaskModule, this, &TaskEditor::saveTaskModule);
        connect(m, &TaskModuleModel::removeTaskModule, this, &TaskEditor::removeTaskModule);
        addDocker(ds);
        ds->setWhatsThis(xi18nc("@info:whatsthis",
                            "<title>Task Modules</title>"
                           "<para>"
                           "Task Modules are a group of tasks that can be reused across projects."
                           " This makes it possible draw on past experience and to standardize similar operations."
                           "</para><para>"
                           "A task module is a regular plan file and typically includes tasks, estimates and dependencies."
                           "</para><para>"
                           "<link url='%1'>More...</link>"
                           "</para>", QStringLiteral("plan:task-editor#task-modules-docker")));
    }
}

void TaskEditor::taskModuleDoubleClicked(QModelIndex idx)
{
    QUrl url = idx.data(Qt::UserRole).toUrl();
    if (url.isValid()) {
        Q_EMIT openDocument(url);
    }
}

void TaskEditor::setGuiActive(bool activate)
{
    debugPlan<<activate;
    ViewBase::setGuiActive(activate);
    if (activate && !m_view->selectionModel()->currentIndex().isValid() && m_view->model()->rowCount() > 0) {
        m_view->selectionModel()->setCurrentIndex(m_view->model()->index(0, 0), QItemSelectionModel::NoUpdate);
    }
    slotEnableActions();
}

void TaskEditor::slotCurrentChanged(const QModelIndex &curr, const QModelIndex &)
{
    debugPlan<<curr.row()<<","<<curr.column();
    slotEnableActions();
}

void TaskEditor::slotSelectionChanged(const QModelIndexList &list)
{
    debugPlan<<list.count();
    slotEnableActions();
    Q_EMIT taskSelected(dynamic_cast<Task*>(selectedNode()));
}

QModelIndexList TaskEditor::selectedRows() const
{
#if 0
// Qt bug?
    return m_view->selectionModel()->selectedRows();
#else
    QModelIndexList lst;
    const QModelIndexList indexes = m_view->selectionModel()->selectedIndexes();
    for (QModelIndex i : indexes) {
        if (i.column() == 0) {
            lst << i;
        }
    }
    return lst;
#endif
}

int TaskEditor::selectedRowCount() const
{
    return selectedRows().count();
}

QList<Node*> TaskEditor::selectedNodes() const {
    QList<Node*> lst;
    const QModelIndexList indexes = selectedRows();
    for (const QModelIndex &i : indexes) {
        auto idx = i;
        QAbstractProxyModel *pr = proxyModel();
        while (pr) {
            idx = pr->mapToSource(idx);
            pr = qobject_cast<QAbstractProxyModel*>(pr->sourceModel());
        }
        Node * n = m_view->baseModel()->node(idx);
        if (n != nullptr && n->type() != Node::Type_Project) {
            lst.append(n);
        }
    }
    return lst;
}

QList<Task*> TaskEditor::selectedTasks(Node::NodeTypes type) const
{
    QList<Task*> tasks;
    auto const nodes = selectedNodes();
    for (auto node : nodes) {
        if (node->type() == type) {
            tasks << static_cast<Task*>(node);
        }
    }
    return tasks;
}

Node *TaskEditor::selectedNode() const
{
    QList<Node*> lst = selectedNodes();
    if (lst.count() != 1) {
        return nullptr;
    }
    return lst.first();
}

Node *TaskEditor::currentNode() const {
    Node * n = m_view->baseModel()->node(m_view->selectionModel()->currentIndex());
    if (n == nullptr || n->type() == Node::Type_Project) {
        return nullptr;
    }
    return n;
}

void TaskEditor::slotContextMenuRequested(const QModelIndex& index, const QPoint& pos, const QModelIndexList &rows)
{
    QString name;
    if (rows.count() > 1) {
        const auto tasks = selectedTasks();
        if (tasks.isEmpty()) {
            return;
        }
        if (tasks.count() > 1) {
            editTasks(tasks, pos);
            return;
        }
        name = tasks.at(0)->isScheduled(baseModel()->id()) ? QStringLiteral("task_popup") : QStringLiteral("task_edit_popup");
    } else {
        auto node = currentNode();
        if (node == nullptr) {
            return;
        }
        switch (node->type()) {
        case Node::Type_Project:
            name = QStringLiteral("project_edit_popup");
            Q_EMIT requestPopupMenu(name, pos);
            return;
        case Node::Type_Task:
            name = node->isScheduled(baseModel()->id()) ? QStringLiteral("task_popup") : QStringLiteral("task_edit_popup");
            break;
        case Node::Type_Milestone:
            name = node->isScheduled(baseModel()->id()) ? QStringLiteral("taskeditor_milestone_popup") : QStringLiteral("task_edit_popup");
            break;
        case Node::Type_Summarytask:
            name = QStringLiteral("summarytask_popup");
            break;
        default:
            name = QStringLiteral("node_popup");
            break;
        }
    }
    m_view->setContextMenuIndex(index);
    if (name.isEmpty()) {
        slotHeaderContextMenuRequested(pos);
        m_view->setContextMenuIndex(QModelIndex());
        return;
    }
    debugPlan<<name;
//     Q_EMIT requestPopupMenu(name, pos);
    openContextMenu(name, pos);
    m_view->setContextMenuIndex(QModelIndex());
}

void TaskEditor::editTasks(const QList<Task*> &tasks, const QPoint &pos)
{
    QList<QAction*> lst;
    QAction tasksEdit(i18n("Edit..."), nullptr);
    if (!tasks.isEmpty()) {
        TasksEditController *ted = new TasksEditController(*project(), tasks, this);
        connect(&tasksEdit, &QAction::triggered, ted, &TasksEditController::activate);
        connect(ted, &TasksEditController::addCommand, koDocument(), &KoDocument::addCommand);
        lst << &tasksEdit;
    }
    lst += contextActionList();
    if (!lst.isEmpty()) {
        QMenu::exec(lst, pos, lst.first());
    }

}

void TaskEditor::setScheduleManager(ScheduleManager *sm)
{
    if (!sm && scheduleManager()) {
        // we should only get here if the only schedule manager is scheduled,
        // or when last schedule manager is deleted
        m_domdoc.clear();
        QDomElement element = m_domdoc.createElement(QStringLiteral("expanded"));
        m_domdoc.appendChild(element);
        m_view->masterView()->saveExpanded(element);
    }
    bool tryexpand = sm && !scheduleManager();
    QDomDocument doc;
    bool expand = sm && scheduleManager();
    if (expand) {
        m_view->masterView()->setObjectName(QStringLiteral("TaskEditor"));
        QDomElement element = doc.createElement(QStringLiteral("expanded"));
        doc.appendChild(element);
        m_view->masterView()->saveExpanded(element);
    }
    ViewBase::setScheduleManager(sm);
    m_view->baseModel()->setScheduleManager(sm);

    if (expand) {
        m_view->masterView()->doExpand(doc);
    } else if (tryexpand) {
        m_view->masterView()->doExpand(m_domdoc);
    }
}

void TaskEditor::slotEnableActions()
{
    updateActionsEnabled(isReadWrite());
}

Node *TaskEditor::newIndentParent(const QList<Node*> nodes) const
{
    Node *node = nullptr;
    if (!nodes.isEmpty()) {
        int level = nodes.at(0)->level();
        const auto parent = nodes.at(0)->parentNode();
        int firstPos = parent->numChildren();
        for (Node *n : nodes) {
            if (n->level() != level || n->isBaselined()) {
                return nullptr;
            }
            firstPos = std::min(firstPos, parent->indexOf(n));
        }
        if (firstPos < parent->numChildren()) {
            node = parent->childNode(firstPos - 1);
        }
    }
    return node;
}

void TaskEditor::updateActionsEnabled(bool on)
{
    //debugPlan<<selectedRowCount()<<selectedNode()<<currentNode();
    if (! on) {
        menuAddTask->setEnabled(false);
        actionAddTask->setEnabled(false);
        actionAddMilestone->setEnabled(false);
        menuAddSubTask->setEnabled(false);
        actionAddSubtask->setEnabled(false);
        actionAddSubMilestone->setEnabled(false);
        actionDeleteTask->setEnabled(false);
        actionLinkTask->setEnabled(false);
        actionMoveTaskUp->setEnabled(false);
        actionMoveTaskDown->setEnabled(false);
        actionIndentTask->setEnabled(false);
        actionUnindentTask->setEnabled(false);
        if (auto a = actionCollection()->action(QStringLiteral("node_properties"))) a->setEnabled(false);
        if (auto a = actionCollection()->action(QStringLiteral("split_task"))) { a->setEnabled(false); }
        return;
    }

    int selCount = selectedRowCount();
    if (selCount == 0) {
        if (currentNode()) {
            // there are tasks but none is selected
            menuAddTask->setEnabled(false);
            actionAddTask->setEnabled(false);
            actionAddMilestone->setEnabled(false);
            menuAddSubTask->setEnabled(false);
            actionAddSubtask->setEnabled(false);
            actionAddSubMilestone->setEnabled(false);
            actionDeleteTask->setEnabled(false);
            actionLinkTask->setEnabled(false);
            actionMoveTaskUp->setEnabled(false);
            actionMoveTaskDown->setEnabled(false);
            actionIndentTask->setEnabled(false);
            actionUnindentTask->setEnabled(false);
            if (auto a = actionCollection()->action(QStringLiteral("node_properties"))) a->setEnabled(false);
            if (auto a = actionCollection()->action(QStringLiteral("split_task"))) { a->setEnabled(false); }
        } else {
            // we need to be able to add the first task
            menuAddTask->setEnabled(true);
            actionAddTask->setEnabled(true);
            actionAddMilestone->setEnabled(true);
            menuAddSubTask->setEnabled(false);
            actionAddSubtask->setEnabled(false);
            actionAddSubMilestone->setEnabled(false);
            actionDeleteTask->setEnabled(false);
            actionLinkTask->setEnabled(false);
            actionMoveTaskUp->setEnabled(false);
            actionMoveTaskDown->setEnabled(false);
            actionIndentTask->setEnabled(false);
            actionUnindentTask->setEnabled(false);
            if (auto a = actionCollection()->action(QStringLiteral("node_properties"))) a->setEnabled(false);
            if (auto a = actionCollection()->action(QStringLiteral("split_task"))) { a->setEnabled(false); }
        }
        return;
    }
    Node *n = selectedNode(); // 0 if not a single task, summarytask or milestone
    if (selCount == 1 && n == nullptr) {
        // only project selected
        menuAddTask->setEnabled(true);
        actionAddTask->setEnabled(true);
        actionAddMilestone->setEnabled(true);
        menuAddSubTask->setEnabled(true);
        actionAddSubtask->setEnabled(true);
        actionAddSubMilestone->setEnabled(true);
        actionDeleteTask->setEnabled(false);
        actionLinkTask->setEnabled(false);
        actionMoveTaskUp->setEnabled(false);
        actionMoveTaskDown->setEnabled(false);
        actionIndentTask->setEnabled(false);
        actionUnindentTask->setEnabled(false);
        if (auto a = actionCollection()->action(QStringLiteral("node_properties"))) a->setEnabled(false);
        if (auto a = actionCollection()->action(QStringLiteral("split_task"))) { a->setEnabled(false); }
        return;
    }
    bool baselined = false;
    Project *p = m_view->project();
    if (p && p->isBaselined()) {
        const QList<Node*> nodes = selectedNodes();
        for (Node *n : nodes) {
            if (n->isBaselined()) {
                baselined = true;
                break;
            }
        }
    }
    if (selCount == 1) {
        menuAddTask->setEnabled(true);
        actionAddTask->setEnabled(true);
        actionAddMilestone->setEnabled(true);
        menuAddSubTask->setEnabled(! baselined || n->type() == Node::Type_Summarytask);
        actionAddSubtask->setEnabled(! baselined || n->type() == Node::Type_Summarytask);
        actionAddSubMilestone->setEnabled(! baselined || n->type() == Node::Type_Summarytask);
        actionDeleteTask->setEnabled(! baselined);
        actionLinkTask->setEnabled(! baselined);
        actionMoveTaskUp->setEnabled(project()->canMoveTaskUp(n));
        actionMoveTaskDown->setEnabled(project()->canMoveTaskDown(n));
        actionIndentTask->setEnabled(project()->canIndentTask(n) && !baselined && !n->siblingBefore()->isBaselined());
        actionUnindentTask->setEnabled(project()->canUnindentTask(n) && !baselined);
        if (auto a = actionCollection()->action(QStringLiteral("node_properties"))) a->setEnabled(true);
        if (auto a = actionCollection()->action(QStringLiteral("split_task"))) { a->setEnabled(n->type() == Node::Type_Task && !n->isBaselined()); }
        return;
    }
    // selCount > 1
    menuAddTask->setEnabled(false);
    actionAddTask->setEnabled(false);
    actionAddMilestone->setEnabled(false);
    menuAddSubTask->setEnabled(false);
    actionAddSubtask->setEnabled(false);
    actionAddSubMilestone->setEnabled(false);
    actionDeleteTask->setEnabled(! baselined);
    actionLinkTask->setEnabled(false);
    actionMoveTaskUp->setEnabled(false);
    actionMoveTaskDown->setEnabled(false);
    if (auto a = actionCollection()->action(QStringLiteral("node_properties"))) a->setEnabled(!selectedTasks().isEmpty());
    if (auto a = actionCollection()->action(QStringLiteral("split_task"))) { a->setEnabled(false); }

    const QList<Node*> nodes = selectedNodes();
    const auto indentParent = newIndentParent(nodes);
    actionIndentTask->setEnabled(indentParent);
    if (indentParent) {
        for (Node *n : nodes) {
            if (!project()->canMoveTask(n, indentParent)) {
                actionIndentTask->setEnabled(false);
                break;
            }
        }
    }
    actionUnindentTask->setEnabled(true);
    for (Node *n : nodes) {
        int level = nodes.at(0)->level();
        if (n->isBaselined() || n->level() != level || !project()->canUnindentTask(n)) {
            actionUnindentTask->setEnabled(false);
            break;
        }
    }
}

void TaskEditor::setupGui()
{
    menuAddTask = new KActionMenu(koIcon("view-task-add"), i18n("Add Task"), this);
    actionCollection()->addAction(QStringLiteral("add_task"), menuAddTask);
    connect(menuAddTask, &QAction::triggered, this, &TaskEditor::slotAddTask);

    actionAddTask  = new QAction(i18n("Add Task"), this);
    actionAddTask->setShortcut(Qt::CTRL | Qt::Key_I);
    connect(actionAddTask, &QAction::triggered, this, &TaskEditor::slotAddTask);
    menuAddTask->addAction(actionAddTask);

    actionAddMilestone  = new QAction(i18n("Add Milestone"), this);
    actionAddMilestone->setShortcut(Qt::CTRL | Qt::ALT | Qt::Key_I);
    connect(actionAddMilestone, &QAction::triggered, this, &TaskEditor::slotAddMilestone);
    menuAddTask->addAction(actionAddMilestone);


    menuAddSubTask = new KActionMenu(koIcon("view-task-child-add"), i18n("Add Sub-Task"), this);
    actionCollection()->addAction(QStringLiteral("add_subtask"), menuAddSubTask);
    connect(menuAddSubTask, &QAction::triggered, this, &TaskEditor::slotAddSubtask);

    actionAddSubtask  = new QAction(i18n("Add Sub-Task"), this);
    actionAddSubtask->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_I);
    connect(actionAddSubtask, &QAction::triggered, this, &TaskEditor::slotAddSubtask);
    menuAddSubTask->addAction(actionAddSubtask);

    actionAddSubMilestone = new QAction(i18n("Add Sub-Milestone"), this);
    actionAddSubMilestone->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::ALT | Qt::Key_I);
    connect(actionAddSubMilestone, &QAction::triggered, this, &TaskEditor::slotAddSubMilestone);
    menuAddSubTask->addAction(actionAddSubMilestone);

    actionDeleteTask  = new QAction(koIcon("edit-delete"), xi18nc("@action", "Delete"), this);
    actionCollection()->setDefaultShortcut(actionDeleteTask, Qt::Key_Delete);
    actionCollection()->addAction(QStringLiteral("delete_task"), actionDeleteTask);
    connect(actionDeleteTask, &QAction::triggered, this, &TaskEditor::slotDeleteTask);

    actionLinkTask  = new QAction(koIcon("link"), xi18nc("@action", "Link"), this);
    actionCollection()->setDefaultShortcut(actionLinkTask, Qt::CTRL | Qt::Key_L);
    actionCollection()->addAction(QStringLiteral("link_task"), actionLinkTask);
    connect(actionLinkTask, &QAction::triggered, this, &TaskEditor::slotLinkTask);

    actionIndentTask  = new QAction(koIcon("format-indent-more"), i18n("Indent Task"), this);
    actionCollection()->addAction(QStringLiteral("indent_task"), actionIndentTask);
    connect(actionIndentTask, &QAction::triggered, this, &TaskEditor::slotIndentTask);

    actionUnindentTask  = new QAction(koIcon("format-indent-less"), i18n("Unindent Task"), this);
    actionCollection()->addAction(QStringLiteral("unindent_task"), actionUnindentTask);
    connect(actionUnindentTask, &QAction::triggered, this, &TaskEditor::slotUnindentTask);

    actionMoveTaskUp  = new QAction(koIcon("arrow-up"), i18n("Move Up"), this);
    actionCollection()->addAction(QStringLiteral("move_task_up"), actionMoveTaskUp);
    connect(actionMoveTaskUp, &QAction::triggered, this, &TaskEditor::slotMoveTaskUp);

    actionMoveTaskDown  = new QAction(koIcon("arrow-down"), i18n("Move Down"), this);
    actionCollection()->addAction(QStringLiteral("move_task_down"), actionMoveTaskDown);
    connect(actionMoveTaskDown, &QAction::triggered, this, &TaskEditor::slotMoveTaskDown);

    auto actionSplit  = new QAction(koIcon("split"), i18n("Split"), this);
    actionCollection()->addAction(QStringLiteral("split_task"), actionSplit);
    connect(actionSplit, &QAction::triggered, this, &TaskEditor::slotTaskSplit);

    auto actionOpenNode  = new QAction(koIcon("document-edit"), i18n("Edit..."), this);
    actionCollection()->addAction(QStringLiteral("node_properties"), actionOpenNode);
    connect(actionOpenNode, &QAction::triggered, this, &TaskEditor::slotOpenCurrentSelection);

    auto actionTaskProgress  = new QAction(koIcon("document-edit"), i18n("Progress..."), this);
    actionCollection()->addAction(QStringLiteral("task_progress"), actionTaskProgress);
    connect(actionTaskProgress, &QAction::triggered, this, &TaskEditor::slotTaskProgress);

    auto actionTaskDescription  = new QAction(koIcon("document-edit"), i18n("Description..."), this);
    actionCollection()->addAction(QStringLiteral("task_description"), actionTaskDescription);
    connect(actionTaskDescription, &QAction::triggered, this, &TaskEditor::slotTaskDescription);

    auto actionDocuments  = new QAction(koIcon("document-edit"), i18n("Documents..."), this);
    actionCollection()->addAction(QStringLiteral("task_documents"), actionDocuments);
    connect(actionDocuments, &QAction::triggered, this, &TaskEditor::slotDocuments);

    // Add the context menu actions for the view options
    actionShowProject = new KToggleAction(i18n("Show Project"), this);
    actionCollection()->addAction(QStringLiteral("show_project"), actionShowProject);
    connect(actionShowProject, &QAction::triggered, baseModel(), &NodeItemModel::setShowProject);
    addContextAction(actionShowProject);

    actionCollection()->addAction(m_view->actionSplitView()->objectName(), m_view->actionSplitView());
    connect(m_view->actionSplitView(), &QAction::triggered, this, &TaskEditor::slotSplitView);
    addContextAction(m_view->actionSplitView());

    createOptionActions(ViewBase::OptionAll);

    createDockers();
}

void TaskEditor::slotSplitView()
{
    debugPlan;
    m_view->setViewSplitMode(! m_view->isViewSplit());
    Q_EMIT optionsModified();
}


void TaskEditor::slotOptions()
{
    debugPlan;
    SplitItemViewSettupDialog *dlg = new SplitItemViewSettupDialog(this, m_view, this);
    dlg->addPrintingOptions(sender()->objectName() == QStringLiteral("print_options"));
    connect(dlg, SIGNAL(finished(int)), SLOT(slotOptionsFinished(int)));
    dlg->open();
}

void TaskEditor::slotAddTask()
{
    debugPlan;
    if (selectedRowCount() == 0 || (selectedRowCount() == 1 && selectedNode() == nullptr)) {
        Task *t = m_view->project()->createTask(m_view->project()->taskDefaults());
        QModelIndex idx = m_view->baseModel()->insertSubtask(t, m_view->project());
        Q_ASSERT(idx.isValid());
        edit(idx);
        return;
    }
    Node *sib = selectedNode();
    if (sib == nullptr) {
        return;
    }
    Task *t = m_view->project()->createTask(m_view->project()->taskDefaults());
    QModelIndex idx = m_view->baseModel()->insertTask(t, sib);
    Q_ASSERT(idx.isValid());
    edit(idx);
}

void TaskEditor::slotAddMilestone()
{
    debugPlan;
    if (selectedRowCount() == 0  || (selectedRowCount() == 1 && selectedNode() == nullptr)) {
        // None selected or only project selected: insert under main project
        Task *t = m_view->project()->createTask();
        t->estimate()->clear();
        QModelIndex idx = m_view->baseModel()->insertSubtask(t, m_view->project());
        Q_ASSERT(idx.isValid());
        edit(idx);
        return;
    }
    Node *sib = selectedNode(); // sibling
    if (sib == nullptr) {
        return;
    }
    Task *t = m_view->project()->createTask();
    t->estimate()->clear();
    QModelIndex idx = m_view->baseModel()->insertTask(t, sib);
    Q_ASSERT(idx.isValid());
    edit(idx);
}

void TaskEditor::slotAddSubMilestone()
{
    debugPlan;
    Node *parent = selectedNode();
    if (parent == nullptr && selectedRowCount() == 1) {
        // project selected
        parent = m_view->project();
    }
    if (parent == nullptr) {
        return;
    }
    // Adding a sub-task will change current task which seems to confuse the view
    // so trigger a commitData() by changing the current index
    m_view->selectionModel()->setCurrentIndex(QModelIndex(), QItemSelectionModel::NoUpdate);
    Task *t = m_view->project()->createTask(m_view->project()->taskDefaults());
    t->estimate()->clear();
    QModelIndex idx = m_view->baseModel()->insertSubtask(t, parent);
    Q_ASSERT(idx.isValid());
    edit(idx);
}

void TaskEditor::slotAddSubtask()
{
    debugPlan;
    Node *parent = selectedNode();
    if (parent == nullptr && selectedRowCount() == 1) {
        // project selected
        parent = m_view->project();
    }
    if (parent == nullptr) {
        return;
    }
    // Adding a sub-task will change current task which seems to confuse the view
    // so trigger a commitData() by changing the current index
    m_view->selectionModel()->setCurrentIndex(QModelIndex(), QItemSelectionModel::NoUpdate);
    Task *t = m_view->project()->createTask(m_view->project()->taskDefaults());
    QModelIndex idx = m_view->baseModel()->insertSubtask(t, parent);
    Q_ASSERT(idx.isValid());
    edit(idx);
}

void TaskEditor::edit(const QModelIndex &i)
{
    if (i.isValid()) {
        m_view->selectionModel()->setCurrentIndex(i, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
        m_view->edit(i);
    }
}

void TaskEditor::slotDeleteTask()
{
    //debugPlan;
    QList<Node*> lst = selectedNodes();
    while (true) {
        // remove children of selected tasks, as parents delete their children
        Node *ch = nullptr;
        for (Node *n1 : std::as_const(lst)) {
            for (Node *n2 : std::as_const(lst)) {
                if (n2->isChildOf(n1)) {
                    ch = n2;
                    break;
                }
            }
            if (ch != nullptr) {
                break;
            }
        }
        if (ch == nullptr) {
            break;
        }
        lst.removeAt(lst.indexOf(ch));
    }
    //foreach (Node* n, lst) { debugPlan<<n->name(); }
    deleteTaskList(lst);
    QModelIndex i = m_view->selectionModel()->currentIndex();
    if (i.isValid()) {
        m_view->selectionModel()->select(i, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
        m_view->selectionModel()->setCurrentIndex(i, QItemSelectionModel::NoUpdate);
    }
}

void TaskEditor::deleteTaskList(QList<Node*> lst)
{
    //debugPlan;
    for (Node *n : std::as_const(lst)) {
        if (n->isScheduled()) {
            KMessageBox::ButtonCode res = KMessageBox::warningContinueCancel(this, i18n("A task that has been scheduled will be deleted. This will invalidate the schedule."));
            if (res == KMessageBox::Cancel) {
                return;
            }
            break;
        }
    }
    if (lst.count() == 1) {
        koDocument()->addCommand(new NodeDeleteCmd(lst.takeFirst(), kundo2_i18nc("Delete one task", "Delete task")));
        return;
    }
    int num = 0;
    MacroCommand *cmd = new MacroCommand(kundo2_i18np("Delete task", "Delete tasks", lst.count()));
    while (!lst.isEmpty()) {
        Node *node = lst.takeFirst();
        if (node == nullptr || node->parentNode() == nullptr) {
            debugPlan << (node ?"Task is main project" :"No current task");
            continue;
        }
        bool del = true;
        for (Node *n : std::as_const(lst)) {
            if (node->isChildOf(n)) {
                del = false; // node is going to be deleted when we delete n
                break;
            }
        }
        if (del) {
            //debugPlan<<num<<": delete:"<<node->name();
            cmd->addCommand(new NodeDeleteCmd(node, kundo2_i18nc("@action", "Delete task")));
            num++;
        }
    }
    if (num > 0) {
        koDocument()->addCommand(cmd);
    } else {
        delete cmd;
    }
}

void TaskEditor::slotLinkTask()
{
    //debugPlan;
    Node *n = currentNode();
    if (n && project()) {
        // In case we are editing the task, trigger commitData() by changing currentIndex
        // to get the data committed before the dialog is opened
        QModelIndex idx = m_view->selectionModel()->currentIndex();
        m_view->selectionModel()->setCurrentIndex(QModelIndex(), QItemSelectionModel::NoUpdate);
        m_view->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::NoUpdate);
        RelationEditorDialog dlg(project(), n);
        if (dlg.exec()) {
            KUndo2Command *cmd = dlg.buildCommand();
            if (cmd) {
                koDocument()->addCommand(cmd);
            }
        }
    }
}

void TaskEditor::slotIndentTask()
{
    debugPlan;
    const QList<Node*> nodes = selectedNodes();
    if (nodes.count() > 0) {
        Node *newparent = newIndentParent(nodes);
        if (newparent) {
            MacroCommand *cmd = new MacroCommand(kundo2_i18np("Indent task", "Indent %1 tasks", nodes.count()));
            for (Node *n : nodes) {
                cmd->addCommand(new NodeMoveCmd(project(), n, newparent, -1));
            }
            koDocument()->addCommand(cmd);
        }
    }
}

void TaskEditor::slotUnindentTask()
{
    debugPlan;
    const QList<Node*> nodes = selectedNodes();
    if (nodes.count() == 1) {
        if (m_proj->canUnindentTask(nodes.at(0))) {
            NodeUnindentCmd * cmd = new NodeUnindentCmd(*nodes.at(0), kundo2_i18nc("For a single task", "Unindent task"));
            koDocument()->addCommand(cmd);
            QModelIndex i = baseModel()->index(nodes.first());
            m_view->selectionModel()->select(i, QItemSelectionModel::Rows | QItemSelectionModel::Current | QItemSelectionModel::ClearAndSelect);
            m_view->selectionModel()->setCurrentIndex(i, QItemSelectionModel::NoUpdate);
        }
    } else if (nodes.count() > 1) {
        MacroCommand *cmd = new MacroCommand(kundo2_i18np("Unindent task", "Unindent %1 tasks", nodes.count()));
        Node *newparent = nodes.first()->parentNode()->parentNode();
        int pos = newparent->indexOf(nodes.first()->parentNode());
        for (Node *n : nodes) {
            cmd->addCommand(new NodeMoveCmd(project(), n, newparent, ++pos));
        }
        koDocument()->addCommand(cmd);
    }
}

void TaskEditor::slotMoveTaskUp()
{
    debugPlan;
    Node *n = selectedNode();
    if (n) {
        if (m_proj->canMoveTaskUp(n)) {
            NodeMoveUpCmd * cmd = new NodeMoveUpCmd(*n, kundo2_i18n("Move task up"));
            koDocument()->addCommand(cmd);
            QModelIndex i = baseModel()->index(n);
            m_view->selectionModel()->select(i, QItemSelectionModel::Rows | QItemSelectionModel::Current | QItemSelectionModel::ClearAndSelect);
            m_view->selectionModel()->setCurrentIndex(i, QItemSelectionModel::NoUpdate);
        }
    }
}

void TaskEditor::slotMoveTaskDown()
{
    debugPlan;
    Node *n = selectedNode();
    if (n) {
        if (m_proj->canMoveTaskDown(n)) {
            NodeMoveDownCmd * cmd = new NodeMoveDownCmd(*n, kundo2_i18n("Move task down"));
            koDocument()->addCommand(cmd);
            QModelIndex i = baseModel()->index(n);
            m_view->selectionModel()->select(i, QItemSelectionModel::Rows | QItemSelectionModel::Current | QItemSelectionModel::ClearAndSelect);
            m_view->selectionModel()->setCurrentIndex(i, QItemSelectionModel::NoUpdate);
        }
    }
}

void TaskEditor::slotTaskSplit()
{
    debugPlan;
    auto *task = qobject_cast<Task*>(selectedNode());
    if (task && task->type() == Node::Type_Task) {
        TaskSplitDialog dlg(project(), task);
        if (dlg.exec() == QDialog::Accepted) {
            auto cmd = dlg.buildCommand();
            if (cmd) {
                koDocument()->addCommand(cmd);
            }
        }
    }
}


bool TaskEditor::loadContext(const KoXmlElement &context)
{
    ViewBase::loadContext(context);
    bool show = (bool)(context.attribute(QStringLiteral("show-project"), QString::number(0)).toInt());
    actionShowProject->setChecked(show);
    baseModel()->setShowProject(show); // why is this not called by the action?
    bool res = m_view->loadContext(baseModel()->columnMap(), context);
    return res;
}

void TaskEditor::saveContext(QDomElement &context) const
{
    ViewBase::saveContext(context);
    context.setAttribute(QStringLiteral("show-project"), QString::number(baseModel()->projectShown()));
    m_view->saveContext(baseModel()->columnMap(), context);
}

KoPrintJob *TaskEditor::createPrintJob()
{
    return m_view->createPrintJob(this);
}

void TaskEditor::slotEditCopy()
{
    m_view->editCopy();
}

void TaskEditor::slotEditPaste()
{
    m_view->editPaste();
}

void TaskEditor::slotOpenCurrentSelection()
{
    //debugPlan;
    auto tasks = selectedTasks();
    if (tasks.count() > 1) {
        auto dlg = new TasksEditDialog(*project(), tasks);
        if (dlg->exec() == QDialog::Accepted) {
            auto cmd = dlg->buildCommand();
            if (cmd) {
                koDocument()->addCommand(cmd);
            }
        }
        dlg->deleteLater();
        return;
    }
    slotOpenNode(currentNode());
}

void TaskEditor::slotOpenNode(Node *node)
{
    //debugPlan;
    if (!node) {
        return ;
    }
    switch (node->type()) {
        case Node::Type_Task: {
                Task *task = static_cast<Task *>(node);
                TaskDialog *dia = new TaskDialog(*project(), *task, project()->accounts(), this);
                connect(dia, &QDialog::finished, this, &TaskEditor::slotTaskEditFinished);
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
                connect(dia, &QDialog::finished, this, &TaskEditor::slotTaskEditFinished);
                dia->open();
                break;
            }
        case Node::Type_Summarytask: {
                Task *task = dynamic_cast<Task *>(node);
                Q_ASSERT(task);
                SummaryTaskDialog *dia = new SummaryTaskDialog(*task, this);
                connect(dia, &QDialog::finished, this, &TaskEditor::slotSummaryTaskEditFinished);
                dia->open();
                break;
            }
        default:
            break;
    }
}

void TaskEditor::slotTaskEditFinished(int result)
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

void TaskEditor::slotSummaryTaskEditFinished(int result)
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

void TaskEditor::slotTaskProgress()
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
                connect(dia, &QDialog::finished, this, &TaskEditor::slotTaskProgressFinished);
                dia->open();
                break;
            }
        case Node::Type_Milestone: {
                Task *task = dynamic_cast<Task *>(node);
                Q_ASSERT(task);
                MilestoneProgressDialog *dia = new MilestoneProgressDialog(*task, this);
                connect(dia, &QDialog::finished, this, &TaskEditor::slotMilestoneProgressFinished);
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

void TaskEditor::slotTaskProgressFinished(int result)
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

void TaskEditor::slotMilestoneProgressFinished(int result)
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

void TaskEditor::slotOpenProjectDescription()
{
    TaskDescriptionDialog *dia = new TaskDescriptionDialog(*project(), this, !isReadWrite());
    connect(dia, &QDialog::finished, this, &TaskEditor::slotTaskDescriptionFinished);
    dia->open();
}

void TaskEditor::slotTaskDescription()
{
    slotOpenTaskDescription(!isReadWrite());
}

void TaskEditor::slotOpenTaskDescription(bool ro)
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
                connect(dia, &QDialog::finished, this, &TaskEditor::slotTaskDescriptionFinished);
                dia->open();
                break;
            }
        default:
            break;
    }
}

void TaskEditor::slotTaskDescriptionFinished(int result)
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

void TaskEditor::slotDocuments()
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
            connect(dia, &QDialog::finished, this, &TaskEditor::slotDocumentsFinished);
            dia->open();
            break;
        }
        default:
            break;
    }
}

void TaskEditor::slotDocumentsFinished(int result)
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

//-----------------------------------
TaskView::TaskView(KoPart *part, KoDocument *doc, QWidget *parent)
    : ViewBase(part, doc, parent)
{
    setXMLFile(QStringLiteral("TaskViewUi.rc"));

    QVBoxLayout * l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    m_view = new NodeTreeView(this);
    m_doubleTreeView = m_view;
    connect(this, &ViewBase::expandAll, m_view, &DoubleTreeViewBase::slotExpand);
    connect(this, &ViewBase::collapseAll, m_view, &DoubleTreeViewBase::slotCollapse);

    NodeSortFilterProxyModel *p = new NodeSortFilterProxyModel(m_view->baseModel(), m_view);
    m_view->setModel(p);
    l->addWidget(m_view);
    setupGui();

    //m_view->setEditTriggers(m_view->editTriggers() | QAbstractItemView::EditKeyPressed);
    m_view->setDragDropMode(QAbstractItemView::DragOnly);
    m_view->setDropIndicatorShown(false);
    m_view->setDragEnabled (true);
    m_view->setAcceptDrops(false);
    m_view->setAcceptDropsOnView(false);

    QList<int> readonly;
    readonly << NodeModel::NodeName
            << NodeModel::NodeResponsible
            << NodeModel::NodeAllocation
            << NodeModel::NodeEstimateType
            << NodeModel::NodeEstimateCalendar
            << NodeModel::NodeEstimate
            << NodeModel::NodeOptimisticRatio
            << NodeModel::NodePessimisticRatio
            << NodeModel::NodeRisk
            << NodeModel::NodeConstraint
            << NodeModel::NodeConstraintStart
            << NodeModel::NodeConstraintEnd
            << NodeModel::NodeRunningAccount
            << NodeModel::NodeStartupAccount
            << NodeModel::NodeStartupCost
            << NodeModel::NodeShutdownAccount
            << NodeModel::NodeShutdownCost
            << NodeModel::NodeDescription;
    for (int c : std::as_const(readonly)) {
        m_view->baseModel()->setReadOnly(c, true);
    }

    QList<int> lst1; lst1 << 1 << -1;
    QList<int> show;
    show << NodeModel::NodeStatus
            << NodeModel::NodeCompleted
            << NodeModel::NodeResponsible
            << NodeModel::NodeAssignments
            << NodeModel::NodePerformanceIndex
            << NodeModel::NodeBCWS
            << NodeModel::NodeBCWP
            << NodeModel::NodeACWP
            << NodeModel::NodeDescription;

    for (int s = 0; s < show.count(); ++s) {
        m_view->slaveView()->mapToSection(show[s], s);
    }
    QList<int> lst2;
    for (int i = 0; i < m_view->model()->columnCount(); ++i) {
        if (! show.contains(i)) {
            lst2 << i;
        }
    }
    m_view->hideColumns(lst1, lst2);
    m_view->masterView()->setDefaultColumns(QList<int>() << 0);
    m_view->slaveView()->setDefaultColumns(show);

    connect(m_view->baseModel(), &ItemModelBase::executeCommand, doc, &KoDocument::addCommand);

    connect(m_view, &DoubleTreeViewBase::currentChanged, this, &TaskView::slotCurrentChanged);

    connect(m_view, &DoubleTreeViewBase::selectionChanged, this, &TaskView::slotSelectionChanged);

    connect(m_view, &DoubleTreeViewBase::contextMenuRequested, this, &TaskView::slotContextMenuRequested);

    connect(m_view, &DoubleTreeViewBase::headerContextMenuRequested, this, &ViewBase::slotHeaderContextMenuRequested);

    connect(m_view->masterView(), &TreeViewBase::doubleClicked, this, &TaskView::itemDoubleClicked);
    connect(m_view->slaveView(), &TreeViewBase::doubleClicked, this, &TaskView::itemDoubleClicked);

    setWhatsThis(
              xi18nc("@info:whatsthis",
                     "<title>Task Execution View</title>"
                     "<para>"
                     "The view is used to edit and inspect task progress during project execution."
                     "</para><para>"
                     "This view supports configuration and printing using the context menu."
                     "<nl/><link url='%1'>More...</link>"
                     "</para>", QStringLiteral("plan:task-execution-view")));
}

void TaskView::itemDoubleClicked(const QPersistentModelIndex &idx)
{
    if (idx.column() == NodeModel::NodeDescription) {
        Node *node = m_view->baseModel()->node(proxyModel()->mapToSource(idx));
        if (node) {
            auto action = actionCollection()->action(QStringLiteral("task_description"));
            if (action) {
                if (node->type() == Node::Type_Project) {
                    slotOpenProjectDescription();
                } else {
                    action->trigger();
                }
            }
        }
    }
}

void TaskView::updateReadWrite(bool rw)
{
    m_view->setReadWrite(rw);
    ViewBase::updateReadWrite(rw);
}

void TaskView::draw(Project &project)
{
    m_view->setProject(&project);
}

void TaskView::draw()
{
}

void TaskView::setGuiActive(bool activate)
{
    debugPlan<<activate;
    updateActionsEnabled(true);
    ViewBase::setGuiActive(activate);
    if (activate && !m_view->selectionModel()->currentIndex().isValid() && m_view->model()->rowCount() > 0) {
        m_view->selectionModel()->setCurrentIndex(m_view->model()->index(0, 0), QItemSelectionModel::NoUpdate);
    }
}

void TaskView::slotCurrentChanged(const QModelIndex &curr, const QModelIndex &)
{
    debugPlan<<curr.row()<<","<<curr.column();
    slotEnableActions();
}

void TaskView::slotSelectionChanged(const QModelIndexList &list)
{
    debugPlan<<list.count();
    slotEnableActions();
}

int TaskView::selectedNodeCount() const
{
    QItemSelectionModel* sm = m_view->selectionModel();
    return sm->selectedRows().count();
}

QList<Node*> TaskView::selectedNodes() const {
    QList<Node*> lst;
    QItemSelectionModel* sm = m_view->selectionModel();
    if (sm == nullptr) {
        return lst;
    }
    const QModelIndexList indexes = sm->selectedRows();
    for (const QModelIndex &i : indexes) {
        Node * n = m_view->baseModel()->node(proxyModel()->mapToSource(i));
        if (n != nullptr && n->type() != Node::Type_Project) {
            lst.append(n);
        }
    }
    return lst;
}

Node *TaskView::selectedNode() const
{
    QList<Node*> lst = selectedNodes();
    if (lst.count() != 1) {
        return nullptr;
    }
    return lst.first();
}

Node *TaskView::currentNode() const {
    Node * n = m_view->baseModel()->node(proxyModel()->mapToSource(m_view->selectionModel()->currentIndex()));
    if (n == nullptr || n->type() == Node::Type_Project) {
        return nullptr;
    }
    return n;
}

void TaskView::slotContextMenuRequested(const QModelIndex& index, const QPoint& pos)
{
    QString name;
    Node *node = m_view->baseModel()->node(proxyModel()->mapToSource(index));
    if (node) {
        switch (node->type()) {
            case Node::Type_Project:
                name = QStringLiteral("project_edit_popup");
                Q_EMIT requestPopupMenu(name, pos);
                return;
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
    } else debugPlan<<"No node: "<<index;

    if (name.isEmpty()) {
        slotHeaderContextMenuRequested(pos);
    } else {
        m_view->setContextMenuIndex(index);
        openContextMenu(name, pos);
        m_view->setContextMenuIndex(QModelIndex());
    }
}

void TaskView::setScheduleManager(ScheduleManager *sm)
{
    //debugPlan<<'\n';
    if (!sm && scheduleManager()) {
        // we should only get here if the only schedule manager is scheduled,
        // or when last schedule manager is deleted
        m_domdoc.clear();
        QDomElement element = m_domdoc.createElement(QStringLiteral("expanded"));
        m_domdoc.appendChild(element);
        m_view->masterView()->saveExpanded(element);
    }
    bool tryexpand = sm && !scheduleManager();
    QDomDocument doc;
    bool expand = sm && scheduleManager() && sm != scheduleManager();
    if (expand) {
        m_view->masterView()->setObjectName(QStringLiteral("TaskEditor"));
        QDomElement element = doc.createElement(QStringLiteral("expanded"));
        doc.appendChild(element);
        m_view->masterView()->saveExpanded(element);
    }
    ViewBase::setScheduleManager(sm);
    m_view->baseModel()->setScheduleManager(sm);

    if (expand) {
        m_view->masterView()->doExpand(doc);
    } else if (tryexpand) {
        m_view->masterView()->doExpand(m_domdoc);
    }
}

void TaskView::slotEnableActions()
{
    updateActionsEnabled(true);
}

void TaskView::updateActionsEnabled(bool on)
{
    const auto node  = currentNode();
    bool enable = on && node && selectedNodeCount() < 2;

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
            default:
                if (auto a = c->action(QStringLiteral("task_progress"))) { a->setEnabled(false); }
                break;
        }
    }
}

void TaskView::setupGui()
{
    auto actionOpenNode  = new QAction(koIcon("document-edit"), i18n("Edit..."), this);
    actionCollection()->addAction(QStringLiteral("node_properties"), actionOpenNode);
    connect(actionOpenNode, &QAction::triggered, this, &TaskView::slotOpenCurrentNode);

    auto actionTaskProgress  = new QAction(koIcon("document-edit"), i18n("Progress..."), this);
    actionCollection()->addAction(QStringLiteral("task_progress"), actionTaskProgress);
    connect(actionTaskProgress, &QAction::triggered, this, &TaskView::slotTaskProgress);

    auto actionTaskDescription  = new QAction(koIcon("document-edit"), i18n("Description..."), this);
    actionCollection()->addAction(QStringLiteral("task_description"), actionTaskDescription);
    connect(actionTaskDescription, &QAction::triggered, this, &TaskView::slotTaskDescription);

    auto actionDocuments  = new QAction(koIcon("document-edit"), i18n("Documents..."), this);
    actionCollection()->addAction(QStringLiteral("task_documents"), actionDocuments);
    connect(actionDocuments, &QAction::triggered, this, &TaskView::slotDocuments);

    // Add the context menu actions for the view options
    actionShowProject = new KToggleAction(i18n("Show Project"), this);
    actionCollection()->addAction(QStringLiteral("show_project"), actionShowProject);
    connect(actionShowProject, &QAction::triggered, baseModel(), &NodeItemModel::setShowProject);
    addContextAction(actionShowProject);

    actionCollection()->addAction(m_view->actionSplitView()->objectName(), m_view->actionSplitView());
    connect(m_view->actionSplitView(), &QAction::triggered, this, &TaskView::slotSplitView);
    addContextAction(m_view->actionSplitView());

    createOptionActions(ViewBase::OptionAll);
}

void TaskView::slotSplitView()
{
    debugPlan;
    m_view->setViewSplitMode(! m_view->isViewSplit());
    Q_EMIT optionsModified();
}

void TaskView::slotOptions()
{
    debugPlan;
    SplitItemViewSettupDialog *dlg = new SplitItemViewSettupDialog(this, m_view, this);
    dlg->addPrintingOptions(sender()->objectName() == QStringLiteral("print_options"));
    connect(dlg, SIGNAL(finished(int)), SLOT(slotOptionsFinished(int)));
    dlg->open();
}

bool TaskView::loadContext(const KoXmlElement &context)
{
    ViewBase::loadContext(context);
    bool show = (bool)(context.attribute(QStringLiteral("show-project"), QString::number(0)).toInt());
    actionShowProject->setChecked(show);
    baseModel()->setShowProject(show); // why is this not called by the action?
    return m_view->loadContext(m_view->baseModel()->columnMap(), context);
}

void TaskView::saveContext(QDomElement &context) const
{
    ViewBase::saveContext(context);
    context.setAttribute(QStringLiteral("show-project"), QString::number(baseModel()->projectShown()));
    m_view->saveContext(m_view->baseModel()->columnMap(), context);
}

KoPrintJob *TaskView::createPrintJob()
{
    return m_view->createPrintJob(this);
}

void TaskView::slotEditCopy()
{
    m_view->editCopy();
}

void TaskView::slotOpenCurrentNode()
{
    //debugPlan;
    slotOpenNode(currentNode());
}

void TaskView::slotOpenNode(Node *node)
{
    //debugPlan;
    if (!node) {
        return ;
    }
    switch (node->type()) {
        case Node::Type_Task: {
                Task *task = static_cast<Task *>(node);
                TaskDialog *dia = new TaskDialog(*project(), *task, project()->accounts(), this);
                connect(dia, &QDialog::finished, this, &TaskView::slotTaskEditFinished);
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
                connect(dia, &QDialog::finished, this, &TaskView::slotTaskEditFinished);
                dia->open();
                break;
            }
        case Node::Type_Summarytask: {
                Task *task = dynamic_cast<Task *>(node);
                Q_ASSERT(task);
                SummaryTaskDialog *dia = new SummaryTaskDialog(*task, this);
                connect(dia, &QDialog::finished, this, &TaskView::slotSummaryTaskEditFinished);
                dia->open();
                break;
            }
        default:
            break;
    }
}

void TaskView::slotTaskEditFinished(int result)
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

void TaskView::slotSummaryTaskEditFinished(int result)
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

void TaskView::slotTaskProgress()
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
                connect(dia, &QDialog::finished, this, &TaskView::slotTaskProgressFinished);
                dia->open();
                break;
            }
        case Node::Type_Milestone: {
                Task *task = dynamic_cast<Task *>(node);
                Q_ASSERT(task);
                MilestoneProgressDialog *dia = new MilestoneProgressDialog(*task, this);
                connect(dia, &QDialog::finished, this, &TaskView::slotMilestoneProgressFinished);
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

void TaskView::slotTaskProgressFinished(int result)
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

void TaskView::slotMilestoneProgressFinished(int result)
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

void TaskView::slotOpenProjectDescription()
{
    TaskDescriptionDialog *dia = new TaskDescriptionDialog(*project(), this, !isReadWrite());
    connect(dia, &QDialog::finished, this, &TaskView::slotTaskDescriptionFinished);
    dia->open();
}

void TaskView::slotTaskDescription()
{
    slotOpenTaskDescription(!isReadWrite());
}

void TaskView::slotOpenTaskDescription(bool ro)
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
                connect(dia, &QDialog::finished, this, &TaskView::slotTaskDescriptionFinished);
                dia->open();
                break;
            }
        default:
            break;
    }
}

void TaskView::slotTaskDescriptionFinished(int result)
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

void TaskView::slotDocuments()
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
            connect(dia, &QDialog::finished, this, &TaskView::slotDocumentsFinished);
            dia->open();
            break;
        }
        default:
            break;
    }
}

void TaskView::slotDocumentsFinished(int result)
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

//---------------------------------
WorkPackageTreeView::WorkPackageTreeView(QWidget *parent)
    : DoubleTreeViewBase(parent)
{
    debugPlan<<"----------"<<this<<"----------";
    m = new WorkPackageProxyModel(this);
    setModel(m);
    //setSelectionBehavior(QAbstractItemView::SelectItems);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);

    createItemDelegates(baseModel());

    setSortingEnabled(true);
    sortByColumn(NodeModel::NodeWBSCode, Qt::AscendingOrder);

    connect(this, &DoubleTreeViewBase::dropAllowed, this, &WorkPackageTreeView::slotDropAllowed);
}

NodeItemModel *WorkPackageTreeView::baseModel() const
{
    return m->baseModel();
}

void WorkPackageTreeView::slotDropAllowed(const QModelIndex &index, int dropIndicatorPosition, QDragMoveEvent *event)
{
    Q_UNUSED(index);
    Q_UNUSED(dropIndicatorPosition);
    Q_UNUSED(event);
/*    QModelIndex idx = index;
    NodeSortFilterProxyModel *pr = proxyModel();
    if (pr) {
        idx = pr->mapToSource(index);
    }
    event->ignore();
    if (baseModel()->dropAllowed(idx, dropIndicatorPosition, event->mimeData())) {
        event->accept();
    }*/
}

//--------------------------------
TaskWorkPackageView::TaskWorkPackageView(KoPart *part, KoDocument *doc, QWidget *parent)
    : ViewBase(part, doc, parent),
    m_cmd(nullptr)
{
    if (doc && doc->isReadWrite()) {
        setXMLFile(QStringLiteral("WorkPackageViewUi.rc"));
    } else {
        setXMLFile(QStringLiteral("WorkPackageViewUi_readonly.rc"));
    }

    QVBoxLayout * l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    m_view = new WorkPackageTreeView(this);
    m_doubleTreeView = m_view;
    connect(this, &ViewBase::expandAll, m_view, &DoubleTreeViewBase::slotExpand);
    connect(this, &ViewBase::collapseAll, m_view, &DoubleTreeViewBase::slotCollapse);

    l->addWidget(m_view);
    setupGui();

    //m_view->setEditTriggers(m_view->editTriggers() | QAbstractItemView::EditKeyPressed);
    m_view->setDragDropMode(QAbstractItemView::DragDrop);
    m_view->setDropIndicatorShown(true);
    m_view->setDragEnabled (true);
    m_view->setAcceptDrops(doc && doc->isReadWrite());
    m_view->setAcceptDropsOnView(doc && doc->isReadWrite());
    m_view->setDefaultDropAction(Qt::CopyAction);

    QList<int> readonly;
    readonly << NodeModel::NodeName
            << NodeModel::NodeResponsible
            << NodeModel::NodeAllocation
            << NodeModel::NodeEstimateType
            << NodeModel::NodeEstimateCalendar
            << NodeModel::NodeEstimate
            << NodeModel::NodeOptimisticRatio
            << NodeModel::NodePessimisticRatio
            << NodeModel::NodeRisk
            << NodeModel::NodeConstraint
            << NodeModel::NodeConstraintStart
            << NodeModel::NodeConstraintEnd
            << NodeModel::NodeRunningAccount
            << NodeModel::NodeStartupAccount
            << NodeModel::NodeStartupCost
            << NodeModel::NodeShutdownAccount
            << NodeModel::NodeShutdownCost
            << NodeModel::NodeDescription;
        for (int c : std::as_const(readonly)) {
        m_view->baseModel()->setReadOnly(c, true);
    }

    QList<int> lst1; lst1 << 1 << -1;
    QList<int> show;
    show << NodeModel::NodeStatus
            << NodeModel::NodeCompleted
            << NodeModel::NodeResponsible
            << NodeModel::NodeAssignments
            << NodeModel::NodeDescription;

    for (int s = 0; s < show.count(); ++s) {
        m_view->slaveView()->mapToSection(show[s], s);
    }
    QList<int> lst2;
    for (int i = 0; i < m_view->model()->columnCount(); ++i) {
        if (! show.contains(i)) {
            lst2 << i;
        }
    }
    m_view->hideColumns(lst1, lst2);
    m_view->masterView()->setDefaultColumns(QList<int>() << 0);
    m_view->slaveView()->setDefaultColumns(show);

    connect(m_view->baseModel(), &ItemModelBase::executeCommand, doc, &KoDocument::addCommand);

    connect(m_view, &DoubleTreeViewBase::currentChanged, this, &TaskWorkPackageView::slotCurrentChanged);

    connect(m_view, &DoubleTreeViewBase::selectionChanged, this, &TaskWorkPackageView::slotSelectionChanged);

    connect(m_view, &DoubleTreeViewBase::contextMenuRequested, this, &TaskWorkPackageView::slotContextMenuRequested);

    connect(m_view, &DoubleTreeViewBase::headerContextMenuRequested, this, &ViewBase::slotHeaderContextMenuRequested);

    connect(m_view->proxyModel(), &WorkPackageProxyModel::loadWorkPackage, this, &TaskWorkPackageView::slotLoadWorkPackage);
    connect(m_view->masterView(), &TreeViewBase::doubleClicked, this, &TaskWorkPackageView::itemDoubleClicked);
    connect(m_view->slaveView(), &TreeViewBase::doubleClicked, this, &TaskWorkPackageView::itemDoubleClicked);

    connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged, this, &TaskWorkPackageView::slotEnableActions);
    updateActionsEnabled(false);

}

void TaskWorkPackageView::itemDoubleClicked(const QPersistentModelIndex &idx)
{
    if (idx.column() == NodeModel::NodeDescription) {
        Node *node = proxyModel()->taskFromIndex(idx);
        if (node) {
            auto action = actionCollection()->action(QStringLiteral("task_description"));
            if (action) {
                action->trigger();
            }
        }
    }
}

Project *TaskWorkPackageView::project() const
{
    return m_view->project();
}

void TaskWorkPackageView::setProject(Project *project)
{
    m_view->setProject(project);
}

WorkPackageProxyModel *TaskWorkPackageView::proxyModel() const
{
    return m_view->proxyModel();
}

void TaskWorkPackageView::updateReadWrite(bool rw)
{
    m_view->setReadWrite(rw);
    ViewBase::updateReadWrite(rw);
}

void TaskWorkPackageView::setGuiActive(bool activate)
{
    debugPlan<<activate;
    ViewBase::setGuiActive(activate);
    if (activate && !m_view->selectionModel()->currentIndex().isValid() && m_view->model()->rowCount() > 0) {
        m_view->selectionModel()->setCurrentIndex(m_view->model()->index(0, 0), QItemSelectionModel::NoUpdate);
    }
}

void TaskWorkPackageView::slotRefreshView()
{
    Q_EMIT checkForWorkPackages(false);
}

void TaskWorkPackageView::slotCurrentChanged(const QModelIndex &curr, const QModelIndex &)
{
    debugPlan<<curr.row()<<","<<curr.column();
    slotEnableActions();
}

void TaskWorkPackageView::slotSelectionChanged(const QModelIndexList &list)
{
    debugPlan<<list.count();
    slotEnableActions();
}

int TaskWorkPackageView::selectedNodeCount() const
{
    QItemSelectionModel* sm = m_view->selectionModel();
    return sm->selectedRows().count();
}

QList<Node*> TaskWorkPackageView::selectedNodes() const {
    QList<Node*> lst;
    QItemSelectionModel* sm = m_view->selectionModel();
    if (sm == nullptr) {
        return lst;
    }
    const QModelIndexList indexes = sm->selectedRows();
    for (const QModelIndex &i : indexes) {
        Node * n = proxyModel()->taskFromIndex(i);
        if (n != nullptr && n->type() != Node::Type_Project) {
            lst.append(n);
        }
    }
    return lst;
}

Node *TaskWorkPackageView::selectedNode() const
{
    QList<Node*> lst = selectedNodes();
    if (lst.count() != 1) {
        return nullptr;
    }
    return lst.first();
}

Node *TaskWorkPackageView::currentNode() const {
    Node * n = proxyModel()->taskFromIndex(m_view->selectionModel()->currentIndex());
    if (n == nullptr || n->type() == Node::Type_Project) {
        return nullptr;
    }
    return n;
}

void TaskWorkPackageView::slotContextMenuRequested(const QModelIndex& index, const QPoint& pos)
{
    QString name;
    Node *node = proxyModel()->taskFromIndex(index);
    if (node) {
        switch (node->type()) {
            case Node::Type_Task:
                name = QStringLiteral("workpackage_popup");
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
    } else debugPlan<<"No node: "<<index;
    if (name.isEmpty()) {
        slotHeaderContextMenuRequested(pos);
    } else {
        m_view->setContextMenuIndex(index);
        openContextMenu(name, pos);
        m_view->setContextMenuIndex(QModelIndex());
    }
}

void TaskWorkPackageView::setScheduleManager(ScheduleManager *sm)
{
    //debugPlan<<'\n';
    m_view->baseModel()->setScheduleManager(sm);
}

void TaskWorkPackageView::slotEnableActions()
{
    updateActionsEnabled(true);
}

void TaskWorkPackageView::updateActionsEnabled(bool on)
{
    const auto nodes = selectedNodes();
    bool o = !nodes.isEmpty();
    actionMailWorkpackage->setEnabled(o && on);

    const auto node  = currentNode();
    bool enable = on && node && nodes.count() < 2;

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
            default:
                if (auto a = c->action(QStringLiteral("task_progress"))) { a->setEnabled(false); }
                break;
        }
    }

}

void TaskWorkPackageView::setupGui()
{
    auto actionTaskProgress  = new QAction(koIcon("document-edit"), i18n("Progress..."), this);
    actionCollection()->addAction(QStringLiteral("task_progress"), actionTaskProgress);
    connect(actionTaskProgress, &QAction::triggered, this, &TaskWorkPackageView::slotTaskProgress);

    auto actionTaskDescription  = new QAction(koIcon("document-edit"), i18n("Description..."), this);
    actionCollection()->addAction(QStringLiteral("task_description"), actionTaskDescription);
    connect(actionTaskDescription, &QAction::triggered, this, &TaskWorkPackageView::slotTaskDescription);

    auto actionDocuments  = new QAction(koIcon("document-edit"), i18n("Documents..."), this);
    actionCollection()->addAction(QStringLiteral("task_documents"), actionDocuments);
    connect(actionDocuments, &QAction::triggered, this, &TaskWorkPackageView::slotDocuments);

    actionMailWorkpackage  = new QAction(koIcon("cloud-upload"), i18n("Publish..."), this);
    actionCollection()->addAction(QStringLiteral("send_workpackage"), actionMailWorkpackage);
    connect(actionMailWorkpackage, &QAction::triggered, this, &TaskWorkPackageView::slotMailWorkpackage);

    actionOpenWorkpackages = new QAction(koIcon("view-task"), i18n("Work Packages..."), this);
    actionCollection()->addAction(QStringLiteral("open_workpackages"), actionOpenWorkpackages);
    actionOpenWorkpackages->setEnabled(false);
    connect(actionOpenWorkpackages, &QAction::triggered, this, &TaskWorkPackageView::openWorkpackages);

    // Add the context menu actions for the view options
    addContextAction(m_view->actionSplitView());
    actionCollection()->addAction(m_view->actionSplitView()->objectName(), m_view->actionSplitView());
    connect(m_view->actionSplitView(), &QAction::triggered, this, &TaskWorkPackageView::slotSplitView);

    createOptionActions(ViewBase::OptionAll);
}

void TaskWorkPackageView::slotMailWorkpackage()
{
    QList<Node*> lst = selectedNodes();
    if (! lst.isEmpty()) {
        // TODO find a better way to log to avoid undo/redo
        m_cmd = new MacroCommand(kundo2_i18n("Log Send Workpackage"));
        QPointer<WorkPackageSendDialog> dlg = new WorkPackageSendDialog(lst, scheduleManager(), this);
        connect (dlg->panel(), &WorkPackageSendPanel::sendWorkpackages, this, &TaskWorkPackageView::publishWorkpackages);

        connect (dlg->panel(), &WorkPackageSendPanel::sendWorkpackages, this, &TaskWorkPackageView::slotWorkPackageSent);
        dlg->exec();
        delete dlg;
        if (! m_cmd->isEmpty()) {
            part()->addCommand(m_cmd);
            m_cmd = nullptr;
        }
        delete m_cmd;
        m_cmd = nullptr;
    }
}

void TaskWorkPackageView::slotWorkPackageSent(const QList<Node*> &nodes, Resource *resource)
{
    for (Node *n : nodes) {
        WorkPackage *wp = new WorkPackage(static_cast<Task*>(n)->workPackage());
        wp->setOwnerName(resource->name());
        wp->setOwnerId(resource->id());
        wp->setTransmitionTime(DateTime::currentDateTime());
        wp->setTransmitionStatus(WorkPackage::TS_Send);
        m_cmd->addCommand(new WorkPackageAddCmd(static_cast<Project*>(n->projectNode()), n, wp));
    }
}

void TaskWorkPackageView::slotSplitView()
{
    debugPlan;
    m_view->setViewSplitMode(! m_view->isViewSplit());
    Q_EMIT optionsModified();
}

void TaskWorkPackageView::slotOptions()
{
    debugPlan;
    SplitItemViewSettupDialog *dlg = new SplitItemViewSettupDialog(this, m_view, this);
    dlg->addPrintingOptions(sender()->objectName() == QStringLiteral("print_options"));
    connect(dlg, SIGNAL(finished(int)), SLOT(slotOptionsFinished(int)));
    dlg->open();
}

bool TaskWorkPackageView::loadContext(const KoXmlElement &context)
{
    debugPlan;
    ViewBase::loadContext(context);
    return m_view->loadContext(m_view->baseModel()->columnMap(), context);
}

void TaskWorkPackageView::saveContext(QDomElement &context) const
{
    ViewBase::saveContext(context);
    m_view->saveContext(m_view->baseModel()->columnMap(), context);
}

KoPrintJob *TaskWorkPackageView::createPrintJob()
{
    return m_view->createPrintJob(this);
}

void TaskWorkPackageView::slotEditCopy()
{
    m_view->editCopy();
}

void TaskWorkPackageView::slotWorkpackagesAvailable(bool value)
{
    actionOpenWorkpackages->setEnabled(value);
}

void TaskWorkPackageView::slotLoadWorkPackage(QList<QString> files)
{
    QList<QUrl> urls;
    for (const QString &f : files) {
        QUrl url = QUrl(f);
        if (url.isValid()) {
            urls << url;
        }
    }
    Q_EMIT loadWorkPackageUrl(m_view->project(), urls);
}

void TaskWorkPackageView::slotTaskProgress()
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
                connect(dia, &QDialog::finished, this, &TaskWorkPackageView::slotTaskProgressFinished);
                dia->open();
                break;
            }
        case Node::Type_Milestone: {
                Task *task = dynamic_cast<Task *>(node);
                Q_ASSERT(task);
                MilestoneProgressDialog *dia = new MilestoneProgressDialog(*task, this);
                connect(dia, &QDialog::finished, this, &TaskWorkPackageView::slotMilestoneProgressFinished);
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

void TaskWorkPackageView::slotTaskProgressFinished(int result)
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

void TaskWorkPackageView::slotMilestoneProgressFinished(int result)
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

void TaskWorkPackageView::slotOpenProjectDescription()
{
    TaskDescriptionDialog *dia = new TaskDescriptionDialog(*project(), this, !isReadWrite());
    connect(dia, &QDialog::finished, this, &TaskWorkPackageView::slotTaskDescriptionFinished);
    dia->open();
}

void TaskWorkPackageView::slotTaskDescription()
{
    slotOpenTaskDescription(!isReadWrite());
}

void TaskWorkPackageView::slotOpenTaskDescription(bool ro)
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
                connect(dia, &QDialog::finished, this, &TaskWorkPackageView::slotTaskDescriptionFinished);
                dia->open();
                break;
            }
        default:
            break;
    }
}

void TaskWorkPackageView::slotTaskDescriptionFinished(int result)
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

void TaskWorkPackageView::slotDocuments()
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
            connect(dia, &QDialog::finished, this, &TaskWorkPackageView::slotDocumentsFinished);
            dia->open();
            break;
        }
        default:
            break;
    }
}

void TaskWorkPackageView::slotDocumentsFinished(int result)
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

} // namespace KPlato
