/* This file is part of the KDE project
 * Copyright (C) 2019 Dag Andersen <danders@get2net.dk>
 * Copyright (C) 2007 - 2010, 2012 Dag Andersen <danders@get2net.dk>
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
#include "Help.h"
#include "kptnodeitemmodel.h"
#include "kptdebug.h"

#include <KoXmlReader.h>
#include "KoDocument.h"
#include "KoPageLayoutWidget.h"

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
    , m_manager(0)
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
    foreach (const QModelIndex &i, m_tree->selectionModel()->selectedIndexes()) {
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
        QDomElement element = m_domdoc.createElement("expanded");
        m_domdoc.appendChild(element);
        treeView()->saveExpanded(element);
    }
    bool tryexpand = sm && !m_manager;
    bool expand = sm && m_manager && sm != m_manager;
    QDomDocument doc;
    if (expand) {
        QDomElement element = doc.createElement("expanded");
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
    QDomElement c = context.ownerDocument().createElement("chart");
    context.appendChild(c);
    m_chart->saveContext(c);

    QDomElement t = context.ownerDocument().createElement("tree");
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
    setXMLFile("PerformanceStatusViewUi.rc");

    QVBoxLayout * l = new QVBoxLayout(this);
    l->setMargin(0);
    m_view = new PerformanceStatusTreeView(this);
    connect(this, &ViewBase::expandAll, m_view->treeView(), &TreeViewBase::slotExpand);
    connect(this, &ViewBase::collapseAll, m_view->treeView(), &TreeViewBase::slotCollapse);

    l->addWidget(m_view);

    setupGui();

    connect(m_view->chartView(), &QWidget::customContextMenuRequested, this, &PerformanceStatusView::slotChartContextMenuRequested);

    connect(m_view->treeView(), SIGNAL(contextMenuRequested(QModelIndex,QPoint,QModelIndexList)), SLOT(slotContextMenuRequested(QModelIndex,QPoint)));

    Help::add(this,
              xi18nc("@info:whatsthis", 
                     "<title>Task Performance View</title>"
                     "<para>"
                     "Displays performance data aggregated to the selected task."
                     "</para><para>"
                     "This view supports configuration and printing using the context menu."
                     "<nl/><link url='%1'>More...</link>"
                     "</para>", Help::page("Task_Performance_View")));
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
    lst << c->action("print");
    lst << c->action("print_preview");
    lst << c->action("print_pdf");
    lst << c->action("print_options");
    lst << new QAction();
    lst.last()->setSeparator(true);
    lst << c->action("configure_view");
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
    if (node == 0) {
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
    //debugPlan<<name;
    if (name.isEmpty()) {
        slotHeaderContextMenuRequested(pos);
        return;
    }
    emit requestPopupMenu(name, pos);
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
    // Add the context menu actions for the view options
    createOptionActions(ViewBase::OptionAll);
}

void PerformanceStatusView::slotOptions()
{
    debugPlan;
    PerformanceStatusViewSettingsDialog *dlg = new PerformanceStatusViewSettingsDialog(this, m_view, this, sender()->objectName() == "print_options");
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
