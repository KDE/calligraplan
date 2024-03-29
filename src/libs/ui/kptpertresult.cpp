/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2007 Florian Piquemal <flotueur@yahoo.fr>
  SPDX-FileCopyrightText: 2007 Alexis Ménard <darktears31@gmail.com>
  SPDX-FileCopyrightText: 2007, 2012 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/
// clazy:excludeall=qstring-arg
#include "kptpertresult.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptnode.h"
#include "kptschedule.h"
#include "kptitemviewsettup.h"
#include "kptdebug.h"
#include <kptcommand.h>
#include "kpttaskdialog.h"
#include "kptsummarytaskdialog.h"

#include <KoDocument.h>
#include <KoIcon.h>

#include <KActionCollection>

#include <QAbstractItemView>
#include <QMenu>
#include <QAction>
#include <QLocale>

#include <math.h>

namespace KPlato
{
  
class Project;
class Node;
class Task;


static const double dist[][2] = {
    {0.00, 0.5000}, {0.02, 0.5080}, {0.04, 0.5160}, {0.06, 0.5239}, {0.08, 0.5319},
    {0.10, 0.5398}, {0.12, 0.5478}, {0.14, 0.5557}, {0.16, 0.5636}, {0.18, 0.5714},
    {0.20, 0.5793}, {0.22, 0.5871}, {0.24, 0.5948}, {0.26, 0.6026}, {0.28, 0.6103},
    {0.30, 0.6179}, {0.32, 0.6255}, {0.34, 0.6331}, {0.36, 0.6406}, {0.38, 0.6480},
    {0.40, 0.6554}, {0.42, 0.6628}, {0.44, 0.6700}, {0.46, 0.6772}, {0.48, 0.6844},
    {0.50, 0.6915}, {0.52, 0.6985}, {0.54, 0.7054}, {0.56, 0.7123}, {0.58, 0.7190},
    {0.60, 0.7257}, {0.62, 0.7324}, {0.64, 0.7389}, {0.66, 0.7454}, {0.68, 0.7517},
    {0.70, 0.7580}, {0.72, 0.7642}, {0.74, 0.7704}, {0.76, 0.7764}, {0.78, 0.7823},
    {0.80, 0.7881}, {0.82, 0.7939}, {0.84, 0.7995}, {0.86, 0.8051}, {0.88, 0.8106},
    {0.90, 0.8159}, {0.92, 0.8212}, {0.94, 0.8264}, {0.96, 0.8315}, {0.98, 0.8365},
    {1.00, 0.8413}, {1.02, 0.8461}, {1.04, 0.8508}, {1.06, 0.8554}, {1.08, 0.8599},
    {1.10, 0.8643}, {1.12, 0.8686}, {1.14, 0.8729}, {1.16, 0.8770}, {1.18, 0.8810},
    {1.20, 0.8849}, {1.22, 0.8888}, {1.24, 0.8925}, {1.26, 0.8962}, {1.28, 0.8997},
    {1.30, 0.9032}, {1.32, 0.9066}, {1.34, 0.9099}, {1.36, 0.9131}, {1.38, 0.9162},
    {1.40, 0.9192}, {1.42, 0.9222}, {1.44, 0.9251}, {1.46, 0.9279}, {1.48, 0.9306},
    {1.50, 0.9332}, {1.52, 0.9357}, {1.54, 0.9382}, {1.56, 0.9406}, {1.58, 0.9429},
    {1.60, 0.9452}, {1.62, 0.9474}, {1.64, 0.9495}, {1.66, 0.9515}, {1.68, 0.9535},
    {1.70, 0.9554}, {1.72, 0.9573}, {1.74, 0.9591}, {1.76, 0.9608}, {1.78, 0.9625},
    {1.80, 0.9641}, {1.82, 0.9656}, {1.84, 0.9671}, {1.86, 0.9686}, {1.88, 0.9699},
    {1.90, 0.9713}, {1.92, 0.9726}, {1.94, 0.9738}, {1.96, 0.9750}, {1.98, 0.9761},
    {2.00, 0.9772}, {2.02, 0.9783}, {2.34, 0.9793}, {2.36, 0.9803}, {2.38, 0.9812},
    {2.10, 0.9821}, {2.12, 0.9830}, {2.34, 0.9838}, {2.36, 0.9846}, {2.38, 0.9854},
    {2.20, 0.9861}, {2.22, 0.9868}, {2.34, 0.9875}, {2.36, 0.9881}, {2.38, 0.9887},
    {2.30, 0.9893}, {2.32, 0.9898}, {2.34, 0.9904}, {2.36, 0.9909}, {2.38, 0.9913},
    {2.40, 0.9918}, {2.42, 0.9922}, {2.44, 0.9927}, {2.46, 0.9931}, {2.48, 0.9934},
    {2.50, 0.9938}, {2.52, 0.9941}, {2.54, 0.9945}, {2.56, 0.9948}, {2.58, 0.9951},
    {2.60, 0.9953}, {2.62, 0.9956}, {2.64, 0.9959}, {2.66, 0.9961}, {2.68, 0.9963},
    {2.70, 0.9965}, {2.72, 0.9967}, {2.74, 0.9969}, {2.76, 0.9971}, {2.78, 0.9973},
    {2.80, 0.9974}, {2.82, 0.9976}, {2.84, 0.9977}, {2.86, 0.9979}, {2.88, 0.9980},
    {2.90, 0.9981}, {2.92, 0.9982}, {2.94, 0.9984}, {2.96, 0.9985}, {2.98, 0.9986},
    {3.00, 0.9987}
};

//-----------------------------------
PertResult::PertResult(KoPart *part, KoDocument *doc, QWidget *parent)
    : ViewBase(part, doc, parent),
    m_node(nullptr),
    m_project(nullptr),
    current_schedule(nullptr)
{
    debugPlan << " ---------------- KPlato: Creating PertResult ----------------";
    setXMLFile(QStringLiteral("PertResultUi.rc"));
    widget.setupUi(this);
    PertResultItemModel *m = new PertResultItemModel(widget.treeWidgetTaskResult);
    widget.treeWidgetTaskResult->setModel(m);
    widget.treeWidgetTaskResult->setStretchLastSection(false);
    widget.treeWidgetTaskResult->setSelectionMode(QAbstractItemView::ExtendedSelection);

    setupGui();
    
    QList<int> lst1; lst1 << 1 << -1; // only display column 0 (NodeName) in left view
    QList<int> show;
    show << NodeModel::NodeEarlyStart
            << NodeModel::NodeEarlyFinish
            << NodeModel::NodeLateStart
            << NodeModel::NodeLateFinish
            << NodeModel::NodePositiveFloat
            << NodeModel::NodeFreeFloat
            << NodeModel::NodeNegativeFloat
            << NodeModel::NodeStartFloat
            << NodeModel::NodeFinishFloat;

    QList<int> lst2; 
    for (int i = 0; i < m->columnCount(); ++i) {
        if (! show.contains(i)) {
            lst2 << i;
        }
    }
    widget.treeWidgetTaskResult->hideColumns(lst1, lst2);
    widget.treeWidgetTaskResult->masterView()->setDefaultColumns(QList<int>() << 0);
    widget.treeWidgetTaskResult->slaveView()->setDefaultColumns(show);
    
    connect(widget.treeWidgetTaskResult, &DoubleTreeViewBase::contextMenuRequested, this, &PertResult::slotContextMenuRequested);
    
    connect(widget.treeWidgetTaskResult, SIGNAL(headerContextMenuRequested(QPoint)), SLOT(slotHeaderContextMenuRequested(QPoint)));

    connect(this, &ViewBase::expandAll, widget.treeWidgetTaskResult, &DoubleTreeViewBase::slotExpand);
    connect(this, &ViewBase::collapseAll, widget.treeWidgetTaskResult, &DoubleTreeViewBase::slotCollapse);
}

void PertResult::draw(Project &project)
{
    setProject(&project);
    //draw();
}
  
void PertResult::draw()
{
    debugPlan<<m_project;
    widget.scheduleName->setText(i18n("None"));
    widget.totalFloat->clear();
    if (m_project && model()->manager() && model()->manager()->isScheduled()) {
        long id = model()->manager()->scheduleId();
        if (id == -1) {
            return;
        }
        widget.scheduleName->setText(model()->manager()->name());
        Duration f;
        const QList<Node*> nodes = m_project->allNodes();
        for (Node *n : nodes) {
            if (n->type() == Node::Type_Task || n->type() == Node::Type_Milestone) {
                f += static_cast<Task*>(n)->positiveFloat(id);
            }
        }
        widget.totalFloat->setText(QLocale().toString(f.toDouble(Duration::Unit_h), 'f', 2));
    }
}

void PertResult::setupGui()
{
    auto actionOpenNode  = new QAction(koIcon("document-edit"), i18n("Edit..."), this);
    actionCollection()->addAction(QStringLiteral("node_properties"), actionOpenNode);
    connect(actionOpenNode, &QAction::triggered, this, &PertResult::slotOpenCurrentNode);

    // Add the context menu actions for the view options
    actionCollection()->addAction(widget.treeWidgetTaskResult->actionSplitView()->objectName(), widget.treeWidgetTaskResult->actionSplitView());
    connect(widget.treeWidgetTaskResult->actionSplitView(), &QAction::triggered, this, &PertResult::slotSplitView);
    addContextAction(widget.treeWidgetTaskResult->actionSplitView());

    createOptionActions(ViewBase::OptionExpand | ViewBase::OptionCollapse | ViewBase::OptionViewConfig);
}

void PertResult::slotSplitView()
{
    debugPlan;
    widget.treeWidgetTaskResult->setViewSplitMode(! widget.treeWidgetTaskResult->isViewSplit());
    Q_EMIT optionsModified();
}

Node *PertResult::currentNode() const
{
    return model()->node(widget.treeWidgetTaskResult->selectionModel()->currentIndex());
}

void PertResult::slotContextMenuRequested(const QModelIndex& index, const QPoint& pos)
{
    debugPlan<<index<<pos;
    Node *node = model()->node(index);
    if (node == nullptr) {
        slotHeaderContextMenuRequested(pos);
        return;
    }
    debugPlan<<node->name()<<" :"<<pos;
    QString name;
    switch (node->type()) {
        case Node::Type_Task:
            name = QStringLiteral("task_edit_popup");
            break;
        case Node::Type_Milestone:
            name = QStringLiteral("taskeditor_milestone_popup");
            break;
        default:
            break;
    }
    if (name.isEmpty()) {
        slotHeaderContextMenuRequested(pos);
    } else {
        openPopupMenu(name, pos);
    }
}

void PertResult::slotHeaderContextMenuRequested(const QPoint &pos)
{
    debugPlan;
    QList<QAction*> lst = contextActionList();
    if (! lst.isEmpty()) {
        QMenu::exec(lst, pos,  lst.first());
    }
}

void PertResult::slotOptions()
{
    debugPlan;
    SplitItemViewSettupDialog *dlg = new SplitItemViewSettupDialog(this, widget.treeWidgetTaskResult, this);
    // Note: printing needs fixes in SplitterView/ScheduleHandlerView
    //dlg->addPrintingOptions(sender()->objectName() == "print_options");
    connect(dlg, SIGNAL(finished(int)), SLOT(slotOptionsFinished(int)));
    dlg->open();
}

void PertResult::slotUpdate(){

    draw();
}

void PertResult::slotScheduleSelectionChanged(ScheduleManager *sm)
{
    current_schedule = sm;
    model()->setManager(sm);
    draw();
}

void PertResult::slotProjectCalculated(KPlato::ScheduleManager *sm)
{
    if (sm && sm == model()->manager()) {
        //draw();
        slotScheduleSelectionChanged(sm);
    }
}

void PertResult::slotScheduleManagerToBeRemoved(const ScheduleManager *sm)
{
    if (sm == model()->manager()) {
        current_schedule = nullptr;
        model()->setManager(nullptr);
    }
}

void PertResult::slotScheduleManagerChanged(ScheduleManager *sm)
{
    if (current_schedule && current_schedule == sm) {
        slotScheduleSelectionChanged(sm);
    }
}

void PertResult::setProject(Project *project)
{
    if (m_project) {
        disconnect(m_project, &Project::nodeChanged, this, &PertResult::slotUpdate);
        disconnect(m_project, &Project::projectCalculated, this, &PertResult::slotProjectCalculated);
        disconnect(m_project, &Project::scheduleManagerToBeRemoved, this, &PertResult::slotScheduleManagerToBeRemoved);
        disconnect(m_project, &Project::scheduleManagerChanged, this, &PertResult::slotScheduleManagerChanged);
    }
    m_project = project;
    model()->setProject(m_project);
    if (m_project) {
        connect(m_project, &Project::nodeChanged, this, &PertResult::slotUpdate);
        connect(m_project, &Project::projectCalculated, this, &PertResult::slotProjectCalculated);
        connect(m_project, &Project::scheduleManagerToBeRemoved, this, &PertResult::slotScheduleManagerToBeRemoved);
        connect(m_project, &Project::scheduleManagerChanged, this, &PertResult::slotScheduleManagerChanged);
    }
    draw();
}

bool PertResult::loadContext(const KoXmlElement &context)
{
    debugPlan;
    ViewBase::loadContext(context);
    return widget.treeWidgetTaskResult->loadContext(model()->columnMap(), context);
}

void PertResult::saveContext(QDomElement &context) const
{
    ViewBase::saveContext(context);
    widget.treeWidgetTaskResult->saveContext(model()->columnMap(), context);
}

KoPrintJob *PertResult::createPrintJob()
{
    return widget.treeWidgetTaskResult->createPrintJob(this);
}

void PertResult::slotOpenCurrentNode()
{
    //debugPlan;
    slotOpenNode(currentNode());
}

void PertResult::slotOpenNode(Node *node)
{
    //debugPlan;
    if (!node) {
        return ;
    }
    switch (node->type()) {
        case Node::Type_Task: {
                Task *task = static_cast<Task *>(node);
                TaskDialog *dia = new TaskDialog(*project(), *task, project()->accounts(), this);
                connect(dia, &QDialog::finished, this, &PertResult::slotTaskEditFinished);
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
                connect(dia, &QDialog::finished, this, &PertResult::slotTaskEditFinished);
                dia->open();
                break;
            }
        default:
            break;
    }
}

void PertResult::slotTaskEditFinished(int result)
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

void PertResult::slotSummaryTaskEditFinished(int result)
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

//--------------------
PertCpmView::PertCpmView(KoPart *part, KoDocument *doc, QWidget *parent)
    : ViewBase(part, doc, parent),
    m_project(nullptr),
    current_schedule(nullptr),
    block(false)
{
    debugPlan << " ---------------- KPlato: Creating PertCpmView ----------------";
    widget.setupUi(this);
    widget.cpmTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    widget.probabilityFrame->setVisible(false);

    widget.cpmTable->setStretchLastSection (false);
    CriticalPathItemModel *m = new CriticalPathItemModel(widget.cpmTable);
    widget.cpmTable->setModel(m);
    
    setupGui();
    
    QList<int> lst1; lst1 << 1 << -1; // only display first column (NodeName) in left view
    widget.cpmTable->masterView()->setDefaultColumns(QList<int>() << 0);
    
    QList<int> show;
    show << NodeModel::NodeDuration
            << NodeModel::NodeVarianceDuration
            << NodeModel::NodeOptimisticDuration
            << NodeModel::NodePessimisticDuration
            << NodeModel::NodeEstimate
            << NodeModel::NodeExpected
            << NodeModel::NodeVarianceEstimate
            << NodeModel::NodeOptimistic
            << NodeModel::NodePessimistic;

    QList<int> lst2;
    for (int i = 0; i < m->columnCount(); ++i) {
        if (! show.contains(i)) {
            lst2 << i;
        }
    }
    widget.cpmTable->hideColumns(lst1, lst2);
    
    for (int s = 0; s < show.count(); ++s) {
        widget.cpmTable->slaveView()->mapToSection(show[s], s);
    }
    widget.cpmTable->slaveView()->setDefaultColumns(show);
    
    connect(widget.cpmTable, &DoubleTreeViewBase::contextMenuRequested, this, &PertCpmView::slotContextMenuRequested);

    connect(widget.cpmTable, SIGNAL(headerContextMenuRequested(QPoint)), SLOT(slotHeaderContextMenuRequested(QPoint)));
    
    connect(widget.finishTime, &QDateTimeEdit::dateTimeChanged, this, &PertCpmView::slotFinishTimeChanged);
    
    connect(widget.probability, SIGNAL(valueChanged(int)), SLOT(slotProbabilityChanged(int)));
}

void PertCpmView::setupGui()
{
    // Add the context menu actions for the view options
    connect(widget.cpmTable->actionSplitView(), &QAction::triggered, this, &PertCpmView::slotSplitView);
    addContextAction(widget.cpmTable->actionSplitView());

    createOptionActions(ViewBase::OptionExpand | ViewBase::OptionCollapse | ViewBase::OptionViewConfig);
}

void PertCpmView::slotSplitView()
{
    debugPlan;
    widget.cpmTable->setViewSplitMode(! widget.cpmTable->isViewSplit());
    Q_EMIT optionsModified();
}

Node *PertCpmView::currentNode() const
{
    return model()->node(widget.cpmTable->selectionModel()->currentIndex());
}

void PertCpmView::slotContextMenuRequested(const QModelIndex& index, const QPoint& pos)
{
    debugPlan<<index<<pos;
    Node *node = model()->node(index);
    if (node == nullptr) {
        slotHeaderContextMenuRequested(pos);
        return;
    }
    debugPlan<<node->name()<<" :"<<pos;
    QString name;
    switch (node->type()) {
        case Node::Type_Task:
            name = QStringLiteral("task_popup");
            break;
        case Node::Type_Milestone:
            name = QStringLiteral("taskeditor_milestone_popup");
            break;
        case Node::Type_Summarytask:
            name = QStringLiteral("summarytask_popup");
            break;
        case Node::Type_Project:
            break;
        default:
            name = QStringLiteral("node_popup");
            break;
    }
    if (name.isEmpty()) {
        slotHeaderContextMenuRequested(pos);
        return;
    }
    debugPlan<<name;
    Q_EMIT requestPopupMenu(name, pos);
}

void PertCpmView::slotHeaderContextMenuRequested(const QPoint &pos)
{
    debugPlan;
    QList<QAction*> lst = contextActionList();
    if (! lst.isEmpty()) {
        QMenu::exec(lst, pos,  lst.first());
    }
}

void PertCpmView::slotOptions()
{
    debugPlan;
    SplitItemViewSettupDialog *dlg = new SplitItemViewSettupDialog(this, widget.cpmTable, this);
    // Note: printing needs fixes in SplitterView/ScheduleHandlerView
    //dlg->addPrintingOptions(sender()->objectName() == "print_options");
    connect(dlg, SIGNAL(finished(int)), SLOT(slotOptionsFinished(int)));
    dlg->open();
}

void PertCpmView::slotScheduleSelectionChanged(ScheduleManager *sm)
{
    bool enbl = sm && sm->isScheduled() && sm->usePert();
    debugPlan<<sm<<(sm?sm->isScheduled():false)<<(sm?sm->usePert():false)<<enbl;
    widget.probabilityFrame->setVisible(enbl);
    current_schedule = sm;
    model()->setManager(sm);
    draw();
}

void PertCpmView::slotProjectCalculated(ScheduleManager *sm)
{
    if (sm && sm == model()->manager()) {
        slotScheduleSelectionChanged(sm);
    }
}

void PertCpmView::slotScheduleManagerChanged(ScheduleManager *sm)
{
    if (current_schedule == sm) {
        slotScheduleSelectionChanged(sm);
    }
}

void PertCpmView::slotScheduleManagerToBeRemoved(const ScheduleManager *sm)
{
    if (sm == current_schedule) {
        current_schedule = nullptr;
        model()->setManager(nullptr);
        widget.probabilityFrame->setVisible(false);
    }
}

void PertCpmView::setProject(Project *project)
{
    if (m_project) {
        disconnect(m_project, &Project::nodeChanged, this, &PertCpmView::slotUpdate);
        disconnect(m_project, &Project::projectCalculated, this, &PertCpmView::slotProjectCalculated);
        disconnect(m_project, &Project::scheduleManagerToBeRemoved, this, &PertCpmView::slotScheduleManagerToBeRemoved);
        disconnect(m_project, &Project::scheduleManagerChanged, this, &PertCpmView::slotScheduleManagerChanged);
    }
    m_project = project;
    model()->setProject(m_project);
    if (m_project) {
        connect(m_project, &Project::nodeChanged, this, &PertCpmView::slotUpdate);
        connect(m_project, &Project::projectCalculated, this, &PertCpmView::slotProjectCalculated);
        connect(m_project, &Project::scheduleManagerToBeRemoved, this, &PertCpmView::slotScheduleManagerToBeRemoved);
        connect(m_project, &Project::scheduleManagerChanged, this, &PertCpmView::slotScheduleManagerChanged);
    }
    draw();
}

void PertCpmView::draw(Project &project)
{
    setProject(&project);
    // draw()
}

void PertCpmView::draw()
{
    widget.scheduleName->setText(i18n("None"));
    bool enbl = m_project && current_schedule && current_schedule->isScheduled() && current_schedule->usePert();
    widget.probabilityFrame->setVisible(enbl);
    if (m_project && current_schedule && current_schedule->isScheduled()) {
        long id = current_schedule->scheduleId();
        if (id == -1) {
            return;
        }
        widget.scheduleName->setText(current_schedule->name());
        widget.finishTime->setDateTime(m_project->endTime(id));
        bool ro = model()->variance(Qt::EditRole).toDouble() == 0.0;
        if (ro) {
            widget.probability->setValue(50);
        }
        widget.finishTime->setReadOnly(ro);
        widget.probability->setEnabled(! ro);
    }
}

void PertCpmView::slotFinishTimeChanged(const QDateTime &dt)
{
    debugPlan<<dt;
    if (block || m_project == nullptr || current_schedule == nullptr) {
        return;
    }
    block = true;
    double var = model()->variance(Qt::EditRole).toDouble();
    double dev = sqrt(var);
    DateTime et = m_project->endTime(current_schedule->scheduleId());
    DateTime t = DateTime(dt);
    double d = (et - t).toDouble();
    d = t < et ? -d : d;
    double z = d / dev;
    double v = probability(z);
    widget.probability->setValue((int)(v * 100));
    //debugPlan<<z<<", "<<v;
    block = false;
}

void PertCpmView::slotProbabilityChanged(int value)
{
    debugPlan<<value;
    if (value == 0 || block || m_project == nullptr || current_schedule == nullptr) {
        return;
    }
    block = true;
    double var = model()->variance(Qt::EditRole).toDouble();
    double dev = sqrt(var);
    DateTime et = m_project->endTime(current_schedule->scheduleId());
    double p = valueZ(value);
    DateTime t = et + Duration(qint64(p * dev));
    widget.finishTime->setDateTime(t);
    //debugPlan<<p<<", "<<t.toString();
    block = false;
}

double PertCpmView::probability(double z) const
{
    double p = 1.0;
    int i = 1;
    for (; i < 151; ++i) {
        if (qAbs(z) <= dist[i][0]) {
            break;
        }
    }
    p = dist[i-1][1] + ((dist[i][1] - dist[i-1][1]) * ((qAbs(z) - dist[i-1][0]) / (dist[i][0] - dist[i-1][0])));
    //debugPlan<<i<<":"<<z<<dist[i][0]<<dist[i][1]<<"="<<p;
    return z < 0 ? 1 - p : p;
}

double PertCpmView::valueZ(double pr) const
{
    double p = (pr >= 50.0 ? pr : 100.0 - pr) / 100.0;
    double z = 3.0;
    int i = 1;
    for (; i < 151; ++i) {
        if (p < dist[i][1]) {
            break;
        }
    }
    z = dist[i-1][0] + ((dist[i][0] - dist[i-1][0]) * ((p - dist[i-1][1]) / (dist[i][1] - dist[i-1][1])));
    //debugPlan<<i<<":"<<pr<<p<<dist[i][0]<<dist[i][1]<<"="<<z;
    return pr < 50.0 ? -z : z;
}

void PertCpmView::slotUpdate()
{
    draw();
}

bool PertCpmView::loadContext(const KoXmlElement &context)
{
    debugPlan<<objectName();
    ViewBase::loadContext(context);
    return widget.cpmTable->loadContext(model()->columnMap(), context);
}

void PertCpmView::saveContext(QDomElement &context) const
{
    ViewBase::saveContext(context);
    widget.cpmTable->saveContext(model()->columnMap(), context);
}

KoPrintJob *PertCpmView::createPrintJob()
{
    return widget.cpmTable->createPrintJob(this);
}

} // namespace KPlato
