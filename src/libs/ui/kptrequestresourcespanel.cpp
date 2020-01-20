/* This file is part of the KDE project
 * Copyright (C) 2003 - 2009 Dag Andersen <danders@get2net.dk>
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
#include "kptrequestresourcespanel.h"
#include <kpttask.h>
#include <kptproject.h>
#include <kptresource.h>
#include <AddParentGroupCmd.h>
#include <ModifyResourceRequestAlternativeCmd.h>
#include <kptcommand.h>

#include <QTreeView>
#include <QHeaderView>
#include <QHash>
#include <QVBoxLayout>
#include <QSortFilterProxyModel>


using namespace KPlato;


RequestResourcesPanel::RequestResourcesPanel(QWidget *parent, Project &project, Task &task, bool)
    : QWidget(parent)
{
    m_model.setReadWrite(true);
    m_model.setProject(&project);
    m_model.setTask(&task);

    QVBoxLayout *l = new QVBoxLayout(this);
    l->setMargin(0);
    m_view = new QTreeView(this);
    QSortFilterProxyModel *m = new QSortFilterProxyModel(m_view);
    m->setDynamicSortFilter(true);
    m->setSourceModel(&m_model);
    m_view->setModel(m);
    m_view->setSortingEnabled(true);
    m_view->setRootIsDecorated(false);
    l->addWidget(m_view);
    m_view->header()->resizeSections(QHeaderView::ResizeToContents);

    for (int i = 0; i < m_model.columnCount(); ++i) {
        m_view->setItemDelegateForColumn(i, m_model.createDelegate(i, m_view));
    }
    connect(&m_model, &ResourceAllocationItemModel::dataChanged, this, &RequestResourcesPanel::changed);
    m_view->sortByColumn(ResourceAllocationModel::RequestType, Qt::AscendingOrder);
}

bool RequestResourcesPanel::ok()
{
    return true;
}

MacroCommand *RequestResourcesPanel::buildCommand()
{
    return buildCommand(m_model.task());
}

MacroCommand *RequestResourcesPanel::buildCommand(Task *task)
{
    if (task == nullptr) {
        return nullptr;
    }
    MacroCommand *cmd = new MacroCommand(kundo2_i18n("Modify resource allocations"));
    const QHash<const Resource*, ResourceRequest*> &rmap = m_model.resourceCache();

    // First remove all that should be removed
    for(QHash<const Resource*, ResourceRequest*>::const_iterator rit = rmap.constBegin(); rit != rmap.constEnd(); ++rit) {
        if (rit.value()->units() == 0) {
            ResourceRequest *rr = task->requests().find(rit.key());
            if (rr) {
                cmd->addCommand(new RemoveResourceRequestCmd(rr));
            }
        }
    }
    for(QHash<const Resource*, ResourceRequest*>::const_iterator rit = rmap.constBegin(); rit != rmap.constEnd(); ++rit) {
        Resource *resource = const_cast<Resource*>(rit.key());
        if (rit.value()->units() > 0) {
            ResourceRequest *rr = task->requests().find(resource);
            if (rr == nullptr) {
                ResourceRequest *rr = new ResourceRequest(resource, rit.value()->units());
                rr->setRequiredResources(rit.value()->requiredResources());
                cmd->addCommand(new AddResourceRequestCmd(&task->requests(), rr));
            } else {
                if (rit.value()->units() != rr->units()) {
                    cmd->addCommand(new ModifyResourceRequestUnitsCmd(rr, rr->units(), rit.value()->units()));
                }
                if (rit.value()->requiredResources() != rr->requiredResources()) {
                    cmd->addCommand(new ModifyResourceRequestRequiredCmd(rr, rit.value()->requiredResources()));
                }
                if (rit.value()->alternativeRequests() != rr->alternativeRequests()) {
                    cmd->addCommand(new ModifyResourceRequestAlternativeCmd(rr, rit.value()->alternativeRequests()));
                }
            }
        }
    }
    if (cmd->isEmpty()) {
        delete cmd;
        cmd = nullptr;
    }
    return cmd;
}
