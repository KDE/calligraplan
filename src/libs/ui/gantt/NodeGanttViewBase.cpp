/* This file is part of the KDE project
 *  SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 *  SPDX-FileCopyrightText: 2005 Dag Andersen <dag.andersen@kdemail.net>
 *  SPDX-FileCopyrightText: 2006 Raphael Langerhorst <raphael.langerhorst@kdemail.net>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "NodeGanttViewBase.h"
#include "kptganttitemdelegate.h"
#include <kptproject.h>

#include <KoXmlReader.h>

#include <KGanttGraphicsView>
#include <KGanttTreeViewRowController>
#include <KGanttProxyModel>
#include <KGanttConstraintModel>
#include <KGanttDateTimeGrid>

#include <QHeaderView>

class GanttItemDelegate;

using namespace KPlato;

NodeGanttViewBase::NodeGanttViewBase(QWidget *parent)
    : GanttViewBase(parent)
    , m_ganttdelegate(new GanttItemDelegate(this))
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

    NodeSortFilterProxyModel *m = new NodeSortFilterProxyModel(&m_defaultModel, this);
    m->setRecursiveFilteringEnabled(true);
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
    GanttViewBase::setProject(project);
}

bool NodeGanttViewBase::loadContext(const KoXmlElement &settings)
{
    treeView()->loadContext(model()->columnMap(), settings);

    KoXmlElement e = settings.namedItem("ganttchart").toElement();
    if (! e.isNull()) {
        m_ganttdelegate->showTaskLinks = (bool)(e.attribute("show-dependencies", QString::number(0)).toInt());
        m_ganttdelegate->showTaskName = (bool)(e.attribute("show-taskname", QString::number(0)).toInt());
        m_ganttdelegate->showResources = (bool)(e.attribute("show-resourcenames", QString::number(0)).toInt());
        m_ganttdelegate->showProgress = (bool)(e.attribute("show-completion", QString::number(0)).toInt());
        m_ganttdelegate->showCriticalPath = (bool)(e.attribute("show-criticalpath", QString::number(0)).toInt());
        m_ganttdelegate->showCriticalTasks = (bool)(e.attribute("show-criticaltasks", QString::number(0)).toInt());
        m_ganttdelegate->showPositiveFloat = (bool)(e.attribute("show-positivefloat", QString::number(0)).toInt());
        m_ganttdelegate->showSchedulingError = (bool)(e.attribute("show-schedulingerror", QString::number(0)).toInt());
        m_ganttdelegate->showTimeConstraint = (bool)(e.attribute("show-timeconstraint", QString::number(0)).toInt());
        m_ganttdelegate->showNegativeFloat = (bool)(e.attribute("show-negativefloat", QString::number(0)).toInt());

        GanttViewBase::loadContext(e);

        m_printOptions.loadContext(e);
    }
    return true;
}

void NodeGanttViewBase::saveContext(QDomElement &settings) const
{
    debugPlan;
    treeView()->saveContext(model()->columnMap(), settings);

    QDomElement e = settings.ownerDocument().createElement(QStringLiteral("ganttchart"));
    settings.appendChild(e);
    e.setAttribute(QStringLiteral("show-dependencies"), QString::number(m_ganttdelegate->showTaskLinks));
    e.setAttribute(QStringLiteral("show-taskname"), QString::number(m_ganttdelegate->showTaskName));
    e.setAttribute(QStringLiteral("show-resourcenames"), QString::number(m_ganttdelegate->showResources));
    e.setAttribute(QStringLiteral("show-completion"), QString::number(m_ganttdelegate->showProgress));
    e.setAttribute(QStringLiteral("show-criticalpath"), QString::number(m_ganttdelegate->showCriticalPath));
    e.setAttribute(QStringLiteral("show-criticaltasks"),QString::number(m_ganttdelegate->showCriticalTasks));
    e.setAttribute(QStringLiteral("show-positivefloat"), QString::number(m_ganttdelegate->showPositiveFloat));
    e.setAttribute(QStringLiteral("show-schedulingerror"), QString::number(m_ganttdelegate->showSchedulingError));
    e.setAttribute(QStringLiteral("show-timeconstraint"), QString::number(m_ganttdelegate->showTimeConstraint));
    e.setAttribute(QStringLiteral("show-negativefloat"), QString::number(m_ganttdelegate->showNegativeFloat));

    GanttViewBase::saveContext(e);

    m_printOptions.saveContext(e);
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
