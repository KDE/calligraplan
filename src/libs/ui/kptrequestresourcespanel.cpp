/* This file is part of the KDE project
 * Copyright (C) 2003 - 2009 Dag Andersen <dag.andersen@kdemail.net>
 * Copyright (C) 2020 Dag Andersen <dag.andersen@kdemail.net>
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
#include <QAbstractTableModel>
#include <QItemSelectionModel>


using namespace KPlato;

RequestResourcesPanel::RequestResourcesPanel(QWidget *parent, Project &project, Task &task, bool)
    : QWidget(parent)
{
    ui.setupUi(this);

    m_model.setReadWrite(true);
    m_model.setProject(&project);
    m_model.setTask(&task);

    QSortFilterProxyModel *m = new QSortFilterProxyModel(ui.resourcesView);
    m->setDynamicSortFilter(true);
    m->setSourceModel(&m_model);

    ui.resourcesView->setModel(m);
    ui.resourcesView->header()->resizeSections(QHeaderView::ResizeToContents);

    for (int i = 0; i < m_model.columnCount(); ++i) {
        ui.resourcesView->setItemDelegateForColumn(i, m_model.createDelegate(i, ui.resourcesView));
    }
    ui.resourcesView->sortByColumn(ResourceAllocationModel::RequestType, Qt::AscendingOrder);

    ResourceAlternativesModel *alts = new ResourceAlternativesModel(&m_model, ui.alternativsView);
    alts->setDynamicSortFilter(true);

    ui.alternativsView->setModel(alts);
    // HACK
    ui.alternativsView->setItemDelegateForColumn(1 /*Allocation*/, m_model.createDelegate(ResourceAllocationModel::RequestAllocation, ui.alternativsView));

    ResourceRequiredModel *req = new ResourceRequiredModel(&m_model, ui.requiredView);
    req->setDynamicSortFilter(true);

    ui.requiredView->setModel(req);

    connect(ui.resourcesView->selectionModel(), &QItemSelectionModel::currentChanged, this, &RequestResourcesPanel::slotCurrentChanged);
    connect(&m_model, &ResourceAllocationItemModel::dataChanged, this, &RequestResourcesPanel::changed);
}

void RequestResourcesPanel::slotCurrentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous)
    if (!current.isValid()) {
        return;
    }
    auto idx = static_cast<const QSortFilterProxyModel*>(current.model())->mapToSource(current);
    Q_ASSERT(idx.isValid());
    auto resource = m_model.resource(idx);
    Q_ASSERT(resource);
    auto m = static_cast<ResourceAlternativesModel*>(ui.alternativsView->model());
    m->setResource(resource);

    auto m2 = static_cast<ResourceRequiredModel*>(ui.requiredView->model());
    if (resource->type() == Resource::Type_Work) {
        m2->setResource(resource);
    } else {
        m2->setResource(nullptr);
    }
}

bool RequestResourcesPanel::ok()
{
    return true;
}

MacroCommand *RequestResourcesPanel::buildCommand()
{
    return buildCommand(m_model.task());
}

MacroCommand *RequestResourcesPanel::buildCommand(Task *task, bool clear)
{
    if (task == nullptr) {
        return nullptr;
    }
    MacroCommand *cmd = new MacroCommand(kundo2_i18n("Modify resource allocations"));
    const QHash<const Resource*, ResourceRequest*> &rmap = m_model.resourceCache();

    // First remove all that should be removed
    if (clear) {
        const auto resourceRequests = task->requests().resourceRequests(false);
        for (ResourceRequest *rr : resourceRequests) {
            cmd->addCommand(new RemoveResourceRequestCmd(rr));
        }
    } else {
        for(QHash<const Resource*, ResourceRequest*>::const_iterator rit = rmap.constBegin(); rit != rmap.constEnd(); ++rit) {
            if (rit.value()->units() == 0) {
                ResourceRequest *rr = task->requests().find(rit.key());
                if (rr) {
                    cmd->addCommand(new RemoveResourceRequestCmd(rr));
                }
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
                rr->setAlternativeRequests(rit.value()->alternativeRequests());
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
