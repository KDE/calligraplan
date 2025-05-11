/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2007-2010, 2012 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "PerformanceStatusView.h"
#include "PerformanceStatusBase.h"

#include "kptglobal.h"
#include "kptlocale.h"
#include "kptcommonstrings.h"
#include "kptcommand.h"
#include "kptproject.h"
#include "kptschedule.h"
#include "kpteffortcostmap.h"
#include "kptnodeitemmodel.h"
#include "kptdebug.h"
#include "kpttaskdialog.h"
#include "kptsummarytaskdialog.h"
#include "kpttaskdescriptiondialog.h"
#include "kpttaskprogressdialog.h"
#include "kptmilestoneprogressdialog.h"
#include "kptdocumentsdialog.h"

#include <KoXmlReader.h>
#include "KoDocument.h"
#include "KoPageLayoutWidget.h"
#include <KoIcon.h>

#include <KActionCollection>

#include <QModelIndex>
#include <QItemSelection>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QAbstractButton>
#include <QTimer>
#include <QDrag>
#include <QMimeData>
#include <QPixmap>
#include <QMouseEvent>
#include <QClipboard>
#include <QMenu>

using namespace KChart;

using namespace KPlato;


//-----------------------------------
PerformanceStatusTreeView::PerformanceStatusTreeView(QWidget *parent)
    : QSplitter(parent)
    , m_manager(nullptr)
{
    m_tree = new TreeViewBase(this);

    NodeItemModel *m = new NodeItemModel(m_tree);
    m_tree->setModel(m);
    QList<int> lst1; lst1 << 1 << -1; // only display column 0 (NodeName) in tree view
    m_tree->setDefaultColumns(QList<int>() << 0);
    m_tree->setColumnsHidden(lst1);
    m_tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    addWidget(m_tree);
    m_tree->setTreePosition(-1);

    m_chart = new PerformanceStatusBase(this);
    addWidget(m_chart);

    connect(m_tree->selectionModel(), &QItemSelectionModel::selectionChanged, this, &PerformanceStatusTreeView::slotSelectionChanged);

    QTimer::singleShot(0, this, &PerformanceStatusTreeView::resizeSplitters);
}

void PerformanceStatusTreeView::slotSelectionChanged(const QItemSelection&, const QItemSelection&)
{
    //debugPlan;
    QList<Node*> nodes;
    const QModelIndexList indexes = m_tree->selectionModel()->selectedIndexes();
    for (const QModelIndex &i : indexes) {
        Node *n = nodeModel()->node(i);
        if (! nodes.contains(n)) {
            nodes.append(n);
        }
    }
    m_chart->setNodes(nodes);
}

NodeItemModel *PerformanceStatusTreeView::nodeModel() const
{
    return static_cast<NodeItemModel*>(m_tree->model());
}

void PerformanceStatusTreeView::setScheduleManager(ScheduleManager *sm)
{
    if (!sm && m_manager) {
        // we should only get here if the only schedule manager is scheduled,
        // or when last schedule manager is deleted
        m_domdoc.clear();
        QDomElement element = m_domdoc.createElement(QStringLiteral("expanded"));
        m_domdoc.appendChild(element);
        treeView()->saveExpanded(element);
    }
    bool tryexpand = sm && !m_manager;
    bool expand = sm && m_manager && sm != m_manager;
    QDomDocument doc;
    if (expand) {
        QDomElement element = doc.createElement(QStringLiteral("expanded"));
        doc.appendChild(element);
        treeView()->saveExpanded(element);
    }
    m_manager = sm;
    nodeModel()->setScheduleManager(sm);
    m_chart->setScheduleManager(sm);

    if (expand) {
        treeView()->doExpand(doc);
    } else if (tryexpand) {
        treeView()->doExpand(m_domdoc);
    }
}

Project *PerformanceStatusTreeView::project() const
{
    return nodeModel()->project();
}

void PerformanceStatusTreeView::setProject(Project *project)
{
    nodeModel()->setProject(project);
    m_chart->setProject(project);
    m_chart->setNodes(QList<Node*>() << project);
}

bool PerformanceStatusTreeView::loadContext(const KoXmlElement &context)
{
    debugPlan;

    bool res = false;
    res = m_chart->loadContext(context.namedItem("chart").toElement());
    res &= m_tree->loadContext(nodeModel()->columnMap(), context.namedItem("tree").toElement());
    return res;
}

void PerformanceStatusTreeView::saveContext(QDomElement &context) const
{
    QDomElement c = context.ownerDocument().createElement(QStringLiteral("chart"));
    context.appendChild(c);
    m_chart->saveContext(c);

    QDomElement t = context.ownerDocument().createElement(QStringLiteral("tree"));
    context.appendChild(t);
    m_tree->saveContext(nodeModel()->columnMap(), t);
}

KoPrintJob *PerformanceStatusTreeView::createPrintJob(ViewBase *view)
{
    return m_chart->createPrintJob(view);
}

// hackish way to get reasonable initial splitter sizes
void PerformanceStatusTreeView::resizeSplitters()
{
    int x1 = sizes().value(0);
    int x2 = sizes().value(1);
    if (x1 == 0 && x2 == 0) {
        // not shown yet, try later
        QTimer::singleShot(100, this, &PerformanceStatusTreeView::resizeSplitters);
        return;
    }
    if (x1 == 0 || x2 == 0) {
        // one is hidden, do nothing
        return;
    }
    int tot = x1 + x2;
    x1 = qMax(x1, qMin((tot) / 2, 150));
    setSizes(QList<int>() << x1 << (tot - x1));
}

void PerformanceStatusTreeView::editCopy()
{
    QMimeData *mimeData = new QMimeData;
    QPixmap pixmap(size());
    render(&pixmap);
    mimeData->setImageData(pixmap);
    QGuiApplication::clipboard()->setMimeData(mimeData);
}

//-----------------------------------
PerformanceStatusView::PerformanceStatusView(KoPart *part, KoDocument *doc, QWidget *parent)
    : ViewBase(part, doc, parent)
{
    debugPlan<<"-------------------- creating PerformanceStatusView -------------------";
    setXMLFile(QStringLiteral("PerformanceStatusViewUi.rc"));

    QVBoxLayout * l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    m_view = new PerformanceStatusTreeView(this);
    connect(this, &ViewBase::expandAll, m_view->treeView(), &TreeViewBase::slotExpand);
    connect(this, &ViewBase::collapseAll, m_view->treeView(), &TreeViewBase::slotCollapse);

    l->addWidget(m_view);

    setupGui();

    connect(m_view->chartView(), &QWidget::customContextMenuRequested, this, &PerformanceStatusView::slotChartContextMenuRequested);

    connect(m_view->treeView(), &TreeViewBase::contextMenuRequested, this, [this](const QModelIndex &idx, const QPoint pos, const QModelIndexList&) {
        slotContextMenuRequested(idx, pos);
    });

    connect(m_view->treeView()->header(), &QHeaderView::customContextMenuRequested, this, [this](const QPoint &pos) {
        slotChartContextMenuRequested(mapToGlobal(pos));
    });

    setWhatsThis(
              xi18nc("@info:whatsthis", 
                     "<title>Task Performance View</title>"
                     "<para>"
                     "Displays performance data aggregated to the selected task."
                     "</para><para>"
                     "This view supports configuration and printing using the context menu."
                     "<nl/><link url='%1'>More...</link>"
                     "</para>", QStringLiteral("plan:task-performance-view")));
}

void PerformanceStatusView::slotEditCopy()
{
    m_view->editCopy();
}


void PerformanceStatusView::slotChartContextMenuRequested(const QPoint& pos)
{
    debugPlan<<pos;
    QList<QAction*> lst;
    const KActionCollection *c = actionCollection();
    lst << c->action(QStringLiteral("print"));
    lst << c->action(QStringLiteral("print_preview"));
    lst << c->action(QStringLiteral("print_pdf"));
    lst << c->action(QStringLiteral("print_options"));
    lst << new QAction();
    lst.last()->setSeparator(true);
    lst << c->action(QStringLiteral("configure_view"));
    if (!lst.isEmpty()) {
        QMenu::exec(lst, pos, lst.first());
    }
}

void PerformanceStatusView::slotTableContextMenuRequested(const QPoint& pos)
{
    debugPlan<<pos;
}

void PerformanceStatusView::slotContextMenuRequested(const QModelIndex &index, const QPoint& pos)
{
    debugPlan<<index<<pos;
    m_view->treeView()->setContextMenuIndex(index);
    if (! index.isValid()) {
        slotHeaderContextMenuRequested(pos);
        return;
    }
    Node *node = m_view->nodeModel()->node(index);
    if (node == nullptr) {
        slotHeaderContextMenuRequested(pos);
        m_view->treeView()->setContextMenuIndex(QModelIndex());
        return;
    }
    slotContextMenuRequested(node, pos);
    m_view->treeView()->setContextMenuIndex(QModelIndex());
}

Node *PerformanceStatusView::currentNode() const
{
    return m_view->nodeModel()->node(m_view->treeView()->selectionModel()->currentIndex());
}

void PerformanceStatusView::slotContextMenuRequested(Node *node, const QPoint& pos)
{
    debugPlan<<node->name()<<" :"<<pos;
    QString name;
    auto sid = scheduleManager() ? scheduleManager()->scheduleId() : -1;
    switch (node->type()) {
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
    //debugPlan<<name;
    if (name.isEmpty()) {
        slotHeaderContextMenuRequested(pos);
    } else {
        openContextMenu(name, pos);
    }
}


void PerformanceStatusView::setScheduleManager(ScheduleManager *sm)
{
    //debugPlan;
    ViewBase::setScheduleManager(sm);
    m_view->setScheduleManager(sm);
}

void PerformanceStatusView::setProject(Project *project)
{
    m_view->setProject(project);
}

void PerformanceStatusView::setGuiActive(bool activate)
{
    debugPlan<<activate;
//    updateActionsEnabled(true);
    ViewBase::setGuiActive(activate);
}

void PerformanceStatusView::setupGui()
{
    auto actionOpenNode  = new QAction(koIcon("document-edit"), i18n("Edit..."), this);
    actionCollection()->addAction(QStringLiteral("node_properties"), actionOpenNode);
    connect(actionOpenNode, &QAction::triggered, this, &PerformanceStatusView::slotOpenCurrentNode);

    auto actionTaskProgress  = new QAction(koIcon("document-edit"), i18n("Progress..."), this);
    actionCollection()->addAction(QStringLiteral("task_progress"), actionTaskProgress);
    connect(actionTaskProgress, &QAction::triggered, this, &PerformanceStatusView::slotTaskProgress);

    auto actionTaskDescription  = new QAction(koIcon("document-edit"), i18n("Description..."), this);
    actionCollection()->addAction(QStringLiteral("task_description"), actionTaskDescription);
    connect(actionTaskDescription, &QAction::triggered, this, &PerformanceStatusView::slotTaskDescription);

    auto actionDocuments  = new QAction(koIcon("document-edit"), i18n("Documents..."), this);
    actionCollection()->addAction(QStringLiteral("task_documents"), actionDocuments);
    connect(actionDocuments, &QAction::triggered, this, &PerformanceStatusView::slotDocuments);

    // Add the context menu actions for the view options
    createOptionActions(ViewBase::OptionAll);
}

void PerformanceStatusView::slotOptions()
{
    debugPlan;
    PerformanceStatusViewSettingsDialog *dlg = new PerformanceStatusViewSettingsDialog(this, m_view, this, sender()->objectName() == QStringLiteral("print_options"));
    connect(dlg, SIGNAL(finished(int)), SLOT(slotOptionsFinished(int)));
    dlg->open();
}

bool PerformanceStatusView::loadContext(const KoXmlElement &context)
{
    debugPlan;
    ViewBase::loadContext(context);
    return m_view->loadContext(context);
}

void PerformanceStatusView::saveContext(QDomElement &context) const
{
    ViewBase::saveContext(context);
    m_view->saveContext(context);
}

KoPrintJob *PerformanceStatusView::createPrintJob()
{
    return m_view->createPrintJob(this);
}

void PerformanceStatusView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragStartPosition = event->pos();
    }
    ViewBase::mousePressEvent(event);
}

void PerformanceStatusView::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton)) {
        ViewBase::mouseMoveEvent(event);
        return;
    }
    if ((event->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance()) {
        ViewBase::mouseMoveEvent(event);
        return;
    }
    event->accept();
    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;
    QPixmap pixmap(m_view->size());
    m_view->render(&pixmap);
    mimeData->setImageData(pixmap);
    drag->setMimeData(mimeData);
    drag->exec(Qt::CopyAction);
}

void PerformanceStatusView::slotOpenCurrentNode()
{
    //debugPlan;
    slotOpenNode(currentNode());
}

void PerformanceStatusView::slotOpenNode(Node *node)
{
    //debugPlan;
    if (!node) {
        return ;
    }
    switch (node->type()) {
        case Node::Type_Task: {
                Task *task = static_cast<Task *>(node);
                TaskDialog *dia = new TaskDialog(*project(), *task, project()->accounts(), this);
                connect(dia, &QDialog::finished, this, &PerformanceStatusView::slotTaskEditFinished);
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
                connect(dia, &QDialog::finished, this, &PerformanceStatusView::slotTaskEditFinished);
                dia->open();
                break;
            }
        case Node::Type_Summarytask: {
                Task *task = dynamic_cast<Task *>(node);
                Q_ASSERT(task);
                SummaryTaskDialog *dia = new SummaryTaskDialog(*task, this);
                connect(dia, &QDialog::finished, this, &PerformanceStatusView::slotSummaryTaskEditFinished);
                dia->open();
                break;
            }
        default:
            break;
    }
}

void PerformanceStatusView::slotTaskEditFinished(int result)
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

void PerformanceStatusView::slotSummaryTaskEditFinished(int result)
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

void PerformanceStatusView::slotTaskProgress()
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
                connect(dia, &QDialog::finished, this, &PerformanceStatusView::slotTaskProgressFinished);
                dia->open();
                break;
            }
        case Node::Type_Milestone: {
                Task *task = dynamic_cast<Task *>(node);
                Q_ASSERT(task);
                MilestoneProgressDialog *dia = new MilestoneProgressDialog(*task, this);
                connect(dia, &QDialog::finished, this, &PerformanceStatusView::slotMilestoneProgressFinished);
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

void PerformanceStatusView::slotTaskProgressFinished(int result)
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

void PerformanceStatusView::slotMilestoneProgressFinished(int result)
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

void PerformanceStatusView::slotOpenProjectDescription()
{
    TaskDescriptionDialog *dia = new TaskDescriptionDialog(*project(), this, !isReadWrite());
    connect(dia, &QDialog::finished, this, &PerformanceStatusView::slotTaskDescriptionFinished);
    dia->open();
}

void PerformanceStatusView::slotTaskDescription()
{
    slotOpenTaskDescription(!isReadWrite());
}

void PerformanceStatusView::slotOpenTaskDescription(bool ro)
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
                connect(dia, &QDialog::finished, this, &PerformanceStatusView::slotTaskDescriptionFinished);
                dia->open();
                break;
            }
        default:
            break;
    }
}

void PerformanceStatusView::slotTaskDescriptionFinished(int result)
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

void PerformanceStatusView::slotDocuments()
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
            connect(dia, &QDialog::finished, this, &PerformanceStatusView::slotDocumentsFinished);
            dia->open();
            break;
        }
        default:
            break;
    }
}

void PerformanceStatusView::slotDocumentsFinished(int result)
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

//-----------------
PerformanceStatusViewSettingsDialog::PerformanceStatusViewSettingsDialog(PerformanceStatusView *view, PerformanceStatusTreeView *treeview, QWidget *parent, bool selectPrint)
    : ItemViewSettupDialog(view, treeview->treeView(), true, parent)
{
    PerformanceStatusViewSettingsPanel *panel = new PerformanceStatusViewSettingsPanel(treeview->chartView(), this);
    KPageWidgetItem *page = insertWidget(0, panel, i18n("Chart"), i18n("Chart Settings"));
    setCurrentPage(page);
    addPrintingOptions(selectPrint);
    //connect(panel, SIGNAL(changed(bool)), this, SLOT(enableButtonOk(bool)));

    connect(this, &QDialog::accepted, panel, &PerformanceStatusViewSettingsPanel::slotOk);
    connect(button(QDialogButtonBox::RestoreDefaults), &QAbstractButton::clicked, panel, &PerformanceStatusViewSettingsPanel::setDefault);
}
