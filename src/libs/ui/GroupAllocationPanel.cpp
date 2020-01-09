/* This file is part of the KDE project
 * Copyright (C) 2020 Dag Andersen <danders@get2net.dk>
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
#include "GroupAllocationPanel.h"
#include "kpttask.h"
#include "kptproject.h"

#include <kptcommand.h>

#include <QTreeView>
#include <QHeaderView>
#include <QHash>
#include <QVBoxLayout>


using namespace KPlato;


GroupAllocationPanel::GroupAllocationPanel(QWidget *parent, Project &project, Task &task, bool)
    : QWidget(parent)
{
    m_model.setReadWrite(true);
    m_model.setProject(&project);
    m_model.setTask(&task);

    QVBoxLayout *l = new QVBoxLayout(this);
    l->setMargin(0);
    m_view = new QTreeView(this);
    m_view->setRootIsDecorated(false);
    m_view->setModel(&m_model);
    l->addWidget(m_view);
    m_view->header()->resizeSections(QHeaderView::ResizeToContents);
    m_view->setItemDelegateForColumn(GroupAllocationModel::RequestAllocation, m_model.createDelegate(GroupAllocationModel::RequestAllocation, m_view));

    connect(&m_model, &GroupAllocationItemModel::dataChanged, this, &GroupAllocationPanel::changed);
}

bool GroupAllocationPanel::ok()
{
    return true;
}

MacroCommand *GroupAllocationPanel::buildCommand()
{
    Task *task = m_model.task();
    if (task == nullptr) {
        return nullptr;
    }
    MacroCommand *cmd = new MacroCommand(kundo2_i18n("Modify resourcegroup allocation"));
    QHash<const ResourceGroup*, ResourceGroupRequest*> groups;
    // Add/modify
    const QHash<const ResourceGroup*, ResourceGroupRequest*> &gmap = m_model.groupCache();
    for(QHash<const ResourceGroup*, ResourceGroupRequest*>::const_iterator git = gmap.constBegin(); git != gmap.constEnd(); ++git) {
        ResourceGroupRequest *gr = task->requests().find(git.key());
        if (gr == 0) {
            if (git.value()->units() > 0) {
                gr = new ResourceGroupRequest(const_cast<ResourceGroup*>(git.key()), git.value()->units());
                cmd->addCommand(new AddResourceGroupRequestCmd(*task, gr));
                groups[ git.key() ] = gr;
            } // else nothing
        } else {
            cmd->addCommand(new ModifyResourceGroupRequestUnitsCmd(gr, gr->units(), git.value()->units()));
        }
    }
    if (cmd->isEmpty()) {
        delete cmd;
        cmd = 0;
    }
    return cmd;
}


MacroCommand *GroupAllocationPanel::buildCommand(Task *task)
{
    if (task == 0) {
        return 0;
    }
    MacroCommand *cmd = new MacroCommand(kundo2_i18n("Modify resourcegroup allocation"));

    // First remove all
    foreach(ResourceGroupRequest *g, task->requests().requests()) {
        cmd->addCommand(new RemoveResourceGroupRequestCmd(g));
    }

    QHash<const ResourceGroup*, ResourceGroupRequest*> groups;
    // Add possible requests to groups
    const QHash<const ResourceGroup*, ResourceGroupRequest*> &gmap = m_model.groupCache();
    for(QHash<const ResourceGroup*, ResourceGroupRequest*>::const_iterator git = gmap.constBegin(); git != gmap.constEnd(); ++git) {
        if (git.value()->units() > 0) {
            ResourceGroupRequest *gr = new ResourceGroupRequest(const_cast<ResourceGroup*>(git.key()), git.value()->units());
            cmd->addCommand(new AddResourceGroupRequestCmd(*task, gr));
            groups[git.key()] = gr;
        }
    }
    if (cmd->isEmpty()) {
        delete cmd;
        cmd = 0;
    }
    return cmd;
}
