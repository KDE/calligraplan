/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2007 Florian Piquemal <flotueur@yahoo.fr>
  SPDX-FileCopyrightText: 2007 Alexis Ménard <darktears31@gmail.com>
  SPDX-FileCopyrightText: 2007, 2012 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptperteditor.h"
#include "kptproject.h"
#include "kptrelationeditor.h"
#include "kptdebug.h"

#include <KoDocument.h>
#include <KoPart.h>
#include <KoIcon.h>

#include <QModelIndex>


namespace KPlato
{

//-----------------------------------
PertEditor::PertEditor(KoPart *part, KoDocument *doc, QWidget *parent)
    : ViewBase(part, doc, parent),
    m_project(nullptr)
{
    debugPlan <<" ---------------- KPlato: Creating PertEditor ----------------";
    widget.setupUi(this);

    m_tasktree = widget.taskList;
    m_tasktree->setSelectionMode(QAbstractItemView::SingleSelection);

    m_availableList = widget.available;
    m_availableList->setSelectionMode(QAbstractItemView::SingleSelection);

    m_requiredList = widget.required;
    m_requiredList->hideColumn(1); // child node name
    m_requiredList->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    connect(m_requiredList->model(), &ItemModelBase::executeCommand, doc, &KoDocument::addCommand);
    updateReadWrite(doc->isReadWrite());

    widget.addBtn->setIcon(koIcon("arrow-right"));
    widget.removeBtn->setIcon(koIcon("arrow-left"));
    slotAvailableChanged(nullptr);
    slotRequiredChanged(QModelIndex());

    connect(m_tasktree, &QTreeWidget::currentItemChanged, this, &PertEditor::slotCurrentTaskChanged);
    connect(m_availableList, &QTreeWidget::currentItemChanged, this, &PertEditor::slotAvailableChanged);
    connect(m_requiredList->selectionModel(), &QItemSelectionModel::currentChanged, this, &PertEditor::slotRequiredChanged);

    connect(widget.addBtn, &QAbstractButton::clicked, this, &PertEditor::slotAddClicked);
    connect(widget.removeBtn, &QAbstractButton::clicked, this, &PertEditor::slotRemoveClicked);

    connect(this, &PertEditor::executeCommand, doc, &KoDocument::addCommand);

// TODO: need to use TreeViewBase here
//     connect(this, SIGNAL(expandAll()), m_tasktree, SLOT(slotExpand()));
//     connect(this, SIGNAL(collapseAll()), m_tasktree, SLOT(slotCollapse()));
}

void PertEditor::slotCurrentTaskChanged(QTreeWidgetItem *curr, QTreeWidgetItem *prev)
{
    //debugPlan<<curr<<prev;
    if (curr == nullptr) {
        m_availableList->clear();
        loadRequiredTasksList(nullptr);
    } else if (prev == nullptr) {
        dispAvailableTasks();
    } else {
        updateAvailableTasks();
        loadRequiredTasksList(itemToNode(curr));
    }
    slotAvailableChanged(m_availableList->currentItem());
}

void PertEditor::slotAvailableChanged(QTreeWidgetItem *item)
{
    //debugPlan<<(item?item->text(0):"nil")<<(item?item->data(0, EnabledRole).toBool():false);
    if (item == nullptr || item == m_availableList->currentItem()) {
        widget.addBtn->setEnabled(item != nullptr && item->data(0, EnabledRole).toBool());
    }
}

void PertEditor::slotRequiredChanged(const QModelIndex &item)
{
    //debugPlan<<item;
    widget.removeBtn->setEnabled(item.isValid());
}

void PertEditor::slotAddClicked()
{
    if (! isReadWrite()) {
        return;
    }
    QTreeWidgetItem *item = m_availableList->currentItem();
    //debugPlan<<item;
    addTaskInRequiredList(item);
    updateAvailableTasks(item);
}

void PertEditor::addTaskInRequiredList(QTreeWidgetItem * currentItem)
{
    //debugPlan<<currentItem;
    if (currentItem == nullptr) {
        return;
    }
    if (m_project == nullptr) {
        return;
    }
    // add the relation between the current task and the current task
    QTreeWidgetItem *selectedTask = m_tasktree->currentItem();
    if (selectedTask == nullptr) {
        return;
    }

    Node *par = itemToNode(currentItem);
    Node *child = itemToNode(selectedTask);
    if (par == nullptr || child == nullptr || ! m_project->legalToLink(par, child)) {
        return;
    }
    Relation *rel = new Relation (par, child);
    AddRelationCmd *addCmd = new AddRelationCmd(*m_project, rel, kundo2_noi18n(currentItem->text(0)));
    Q_EMIT executeCommand(addCmd);

}

void PertEditor::slotRemoveClicked()
{
    if (! isReadWrite()) {
        return;
    }
    Node *n = nullptr;
    Relation *r = m_requiredList->currentRelation();
    if (r) {
        n = r->parent();
    }
    removeTaskFromRequiredList();
    setAvailableItemEnabled(n);
}

void PertEditor::removeTaskFromRequiredList()
{
    //debugPlan;
    Relation *r = m_requiredList->currentRelation();
    if (r == nullptr) {
        return;
    }
    // remove the relation
    Q_EMIT executeCommand(new DeleteRelationCmd(*m_project, r, kundo2_i18n("Remove task dependency")));
}

void PertEditor::setProject(Project *project)
{
    if (m_project) {
        disconnect(m_project, &Project::nodeAdded, this, &PertEditor::slotNodeAdded);
        disconnect(m_project, &Project::nodeToBeRemoved, this, &PertEditor::slotNodeRemoved);
        disconnect(m_project, &Project::nodeMoved, this, &PertEditor::slotNodeMoved);
        disconnect(m_project, &Project::nodeChanged, this, &PertEditor::slotNodeChanged);
        disconnect(m_project, &Project::relationAdded, this, &PertEditor::slotRelationAdded);
        disconnect(m_project, &Project::relationRemoved, this, &PertEditor::slotRelationRemoved);
    }
    m_project = project;
    if (m_project) {
        connect(m_project, &Project::nodeAdded, this, &PertEditor::slotNodeAdded);
        connect(m_project, &Project::nodeToBeRemoved, this, &PertEditor::slotNodeRemoved);
        connect(m_project, &Project::nodeMoved, this, &PertEditor::slotNodeMoved);
        connect(m_project, &Project::nodeChanged, this, &PertEditor::slotNodeChanged);
        connect(m_project, &Project::relationAdded, this, &PertEditor::slotRelationAdded);
        connect(m_project, &Project::relationRemoved, this, &PertEditor::slotRelationRemoved);
    }
    m_requiredList->setProject(project);
    draw();
}

void PertEditor::slotRelationAdded(Relation *rel)
{
    debugPlan<<rel;
    if (rel->child() == itemToNode(m_tasktree->currentItem())) {
        QTreeWidgetItem *item = findNodeItem(rel->parent(), m_availableList->invisibleRootItem());
        updateAvailableTasks(item);
    }
}

void PertEditor::slotRelationRemoved(Relation *rel)
{
    debugPlan<<rel;
    if (rel->child() == itemToNode(m_tasktree->currentItem())) {
        QTreeWidgetItem *item = findNodeItem(rel->parent(), m_availableList->invisibleRootItem());
        updateAvailableTasks(item);
    }
}

void PertEditor::slotNodeAdded(Node *node)
{
    debugPlan<<node->name()<<node->childNodeIterator();
    Node *parent = node->parentNode();
    int index = parent->indexOf(node);
    QTreeWidgetItem *pitem = findNodeItem(parent, m_tasktree->invisibleRootItem());
    if (pitem == nullptr) {
        pitem = m_tasktree->invisibleRootItem();
    }
    QTreeWidgetItem *item = new QTreeWidgetItem();
    item->setText(0, node->name());
    item->setData(0, NodeRole, node->id());
    pitem->insertChild(index, item);

    pitem = findNodeItem(parent, m_availableList->invisibleRootItem());
    if (pitem == nullptr) {
        pitem = m_availableList->invisibleRootItem();
    }
    item = new QTreeWidgetItem();
    item->setText(0, node->name());
    item->setData(0, NodeRole, node->id());
    item->setData(0, EnabledRole, true);
    pitem->insertChild(index, item);
    setAvailableItemEnabled(item);
}

void PertEditor::slotNodeRemoved(Node *node)
{
    //debugPlan;
    QTreeWidgetItem *item = findNodeItem(node, m_tasktree->invisibleRootItem());
    if (item) {
        QTreeWidgetItem *parent = item->parent();
        if (parent == nullptr) {
            parent = m_tasktree->invisibleRootItem();
        }
        Q_ASSERT(parent);
        parent->removeChild(item);
        delete item;
    }
    item = findNodeItem(node, m_availableList->invisibleRootItem());
    if (item) {
        QTreeWidgetItem *parent = item->parent();
        if (parent == nullptr) {
            parent = m_availableList->invisibleRootItem();
        }
        Q_ASSERT(parent);
        parent->removeChild(item);
        delete item;
    }
}

void PertEditor::slotNodeMoved(Node *node)
{
    Q_UNUSED(node)
    //debugPlan;
    draw();
}

void PertEditor::slotNodeChanged(Node *node)
{
    QTreeWidgetItem *item = findNodeItem(node, m_tasktree->invisibleRootItem());
    if (item) {
        item->setText(0, node->name());
    }
    item = findNodeItem(node, m_availableList->invisibleRootItem());
    if (item) {
        item->setText(0, node->name());
    }
}

void PertEditor::draw(Project &project)
{
    setProject(&project);
    draw();
}

void PertEditor::draw()
{
    m_tasktree->clear();
    if (m_project == nullptr) {
        return;
    }
    drawSubTasksName(m_tasktree->invisibleRootItem(), m_project);
}

void PertEditor::drawSubTasksName(QTreeWidgetItem *parent, Node * currentNode)
{
    const QList<Node*> nodes = currentNode->childNodeIterator();
    for(Node * currentChild : nodes) {
        QTreeWidgetItem * item = new QTreeWidgetItem(parent);
        item->setText(0, currentChild->name());
        item->setData(0, NodeRole, currentChild->id());
        //debugPlan<<"Added task"<<currentChild->name()<<"parent"<<currentChild->parent();
        drawSubTasksName(item, currentChild);
    }
}

void PertEditor::updateReadWrite(bool rw)
{
    m_requiredList->setReadWrite(rw);
    ViewBase::updateReadWrite(rw);
}

QTreeWidgetItem *PertEditor::findNodeItem(Node *node, QTreeWidgetItem *item) {
    if (node->id() == item->data(0, NodeRole).toString()) {
        return item;
    }
    for (int i = 0; i < item->childCount(); ++i) {
        QTreeWidgetItem *itm = findNodeItem(node, item->child(i));
        if (itm != nullptr) {
            return itm;
        }
    }
    return nullptr;
}


void PertEditor::dispAvailableTasks(Relation *rel)
{
    Q_UNUSED(rel)
    dispAvailableTasks();
}

void PertEditor::dispAvailableTasks(Node *parent, Node *selectedTask)
{
    QTreeWidgetItem *pitem = findNodeItem(parent, m_availableList->invisibleRootItem());
    if (pitem == nullptr) {
        pitem = m_availableList->invisibleRootItem();
    }
    const QList<Node*> nodes = parent->childNodeIterator();
    for(Node * currentNode : nodes) {
        //debugPlan<<currentNode->name()<<"level="<<currentNode->level();
        QTreeWidgetItem *item = new QTreeWidgetItem(QStringList()<<currentNode->name());
        item->setData(0, NodeRole, currentNode->id());
        pitem->addChild(item);
        // Checks it isn't the same as the selected task in the m_tasktree
        setAvailableItemEnabled(item);
        dispAvailableTasks(currentNode, selectedTask);
    }
}

void PertEditor::dispAvailableTasks()
{
    m_availableList->clear();

    if (m_project == nullptr) {
        return;
    }
    Node *selectedTask = itemToNode(m_tasktree->currentItem());

    loadRequiredTasksList(selectedTask);

    dispAvailableTasks(m_project, selectedTask);
}

void PertEditor::updateAvailableTasks(QTreeWidgetItem *item)
{
    //debugPlan<<m_project<<item;
    if (m_project == nullptr) {
        return;
    }
    if (item == nullptr) {
        item = m_availableList->invisibleRootItem();
    } else {
        setAvailableItemEnabled(item);
    }
    for (int i=0; i < item->childCount(); ++i) {
        updateAvailableTasks(item->child(i));
    }
}

void PertEditor::setAvailableItemEnabled(QTreeWidgetItem *item)
{
    //debugPlan<<item;
    Node *node = itemToNode(item);
    if (node == nullptr) {
        return;
    }

    Node *selected = itemToNode(m_tasktree->currentItem());
    if (selected == nullptr || ! m_project->legalToLink(node, selected)) {
        //debugPlan<<"Disable:"<<node->name();
        item->setData(0, EnabledRole, false);
        QFont f = item->font(0);
        f.setItalic(true);
        item->setFont(0, f);
    } else {
        //debugPlan<<"Enable:"<<node->name();
        item->setData(0, EnabledRole, true);
        QFont f = item->font(0);
        f.setItalic(false);
        item->setFont(0, f);
    }
    slotAvailableChanged(item);
}

void PertEditor::setAvailableItemEnabled(Node *node)
{
    //debugPlan<<node->name();
    setAvailableItemEnabled(nodeToItem(node, m_availableList->invisibleRootItem()));
}

QTreeWidgetItem *PertEditor::nodeToItem(Node *node, QTreeWidgetItem *item)
{
    if (itemToNode(item) == node) {
        return item;
    }
    for (int i=0; i < item->childCount(); ++i) {
        QTreeWidgetItem *itm = nodeToItem(node, item->child(i));
        if (itm) {
            return itm;
        }
    }
    return nullptr;
}

Node * PertEditor::itemToNode(QTreeWidgetItem *item)
{
    if (m_project == nullptr || item == nullptr) {
        return nullptr;
    }
    return m_project->findNode(item->data(0, NodeRole).toString());
}

void PertEditor::loadRequiredTasksList(Node * taskNode)
{
    slotRequiredChanged(QModelIndex());
    m_requiredList->setNode(taskNode);
}

void PertEditor::slotUpdate()
{
 draw();
}

} // namespace KPlato
