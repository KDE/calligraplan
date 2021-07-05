/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2017 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "ResourceAllocationView.h"

#include "kptnode.h"
#include "kptresource.h"
#include "kptnodeitemmodel.h"
#include "ResourceGroupItemModel.h"
#include "kptcommand.h"

#include <KoDocument.h>

#include <KLocalizedString>

#include <QAction>
#include <QMenu>
#include <QModelIndexList>
#include <QContextMenuEvent>
#include <QDebug>


namespace KPlato
{

ResourceAllocationView::ResourceAllocationView(KoDocument *doc, QWidget *parent)
    : QTreeView(parent)
    , m_doc(doc)
{
    m_allocateAction = new QAction(i18n("Allocate"), this);
    connect(m_allocateAction, &QAction::triggered, this, &ResourceAllocationView::slotAllocate);
}

QList<Resource*> ResourceAllocationView::selectedResources() const
{
    QList<Resource*> resources;
    ResourceGroupItemModel *m = qobject_cast<ResourceGroupItemModel*>(model());
    if (m) {
        const QModelIndexList indexes = selectionModel()->selectedRows();
        for(const QModelIndex &idx : indexes) {
            Resource *r = m->resource(idx);
            if (r) {
                resources << r;
            }
        }
    }
    return resources;
}

void ResourceAllocationView::setSelectedTasks(const QItemSelection &selected, const QItemSelection &deselected)
{
    for (QModelIndex &idx : deselected.indexes()) {
        if (m_tasks.contains(idx)) {
            m_tasks.removeAt(m_tasks.indexOf(idx));
        }
    }
    const QModelIndexList tasks = selected.indexes();
    if (tasks.isEmpty()) {
        return;
    }
    const NodeItemModel *m = qobject_cast<const NodeItemModel*>(tasks.first().model());
    if (!m) {
        return;
    }
    for (const QModelIndex &idx : tasks) {
        if (idx.column() != NodeModel::NodeAllocation) {
            continue;
        }
        Node *n = m->node(idx);
        if (n->type() != Node::Type_Task) {
            continue;
        }
        m_tasks << QPersistentModelIndex(idx);
    }
}

void ResourceAllocationView::contextMenuEvent(QContextMenuEvent *event)
{
    if (m_tasks.isEmpty()) {
        return;
    }
    if (selectedResources().isEmpty()) {
        return;
    }
    QMenu menu;
    menu.move(event->globalPos());
    menu.addAction(m_allocateAction);
    menu.exec();
    return;    
}

void ResourceAllocationView::slotAllocate()
{
    if (!m_doc) {
        warnPlan<<"ResourceAllocationView has no document, commands cannot be executed";
        return;
    }
    QList<QPersistentModelIndex> lst;
    for (QPersistentModelIndex &idx : m_tasks) {
        if (idx.isValid()) {
            lst << idx;
        }
    }
    if (lst.isEmpty()) {
        return;
    }
    const QList<Resource*> resources = selectedResources();
    if (resources.isEmpty()) {
        return;
    }
    const NodeItemModel *m = qobject_cast<const NodeItemModel*>(lst.first().model());
    MacroCommand *cmd = new MacroCommand();
    for (QPersistentModelIndex &idx : lst) {
        Node *n = m->node(idx);
        if (!n || n->type() != Node::Type_Task) {
            continue;
        }
        Task *t = static_cast<Task*>(n);
        // remove any requests before adding new ones
        const QList<ResourceRequest*> requests = t->requests().resourceRequests();
        for (ResourceRequest *r : requests) {
            RemoveResourceRequestCmd *c = new RemoveResourceRequestCmd(r);
            c->execute(); // need to remove everything before we add anything
            cmd->addCommand(c);
        }
        for (Resource *r : resources) {
            ResourceRequest *rr = new ResourceRequest(r);
            rr->setUnits(100); // defaults to 100%
            AddResourceRequestCmd *c = new AddResourceRequestCmd(&n->requests(), rr);
            c->execute();
            cmd->addCommand(c);
        }
    }
    if (cmd->isEmpty()) {
        delete cmd;
    } else {
        MacroCommand *m = new MacroCommand(kundo2_i18n("Modify resource allocations"));
        m_doc->addCommand(m);
        m->addCommand(cmd);
    }
}

} // namespace KPlato
