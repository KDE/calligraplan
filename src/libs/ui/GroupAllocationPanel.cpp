/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2020 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
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
    l->setContentsMargins(0, 0, 0, 0);
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
    const QList<ResourceGroupRequest*> requests = task->requests().requests();
    for (ResourceGroupRequest *g : requests) {
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
