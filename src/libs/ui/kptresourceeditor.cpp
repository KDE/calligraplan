/* This file is part of the KDE project
 * Copyright (C) 2019 Dag Andersen <dag.andersen@kdemail.net>
 * Copyright (C) 2006 - 2007 Dag Andersen <dag.andersen@kdemail.net>
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
#include "kptresourceeditor.h"

#include "ResourceItemModel.h"
#include "ResourceGroupItemModel.h"
#include "kptcommand.h"
#include "kptitemmodelbase.h"
#include "kptcalendar.h"
#include "kptduration.h"
#include "kptnode.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptresource.h"
#include "kptdatetime.h"
#include "kptitemviewsettup.h"
#include "Help.h"
#include "kptdebug.h"

#include <KoDocument.h>
#include <KoIcon.h>

#include <QList>
#include <QVBoxLayout>
#include <QDragMoveEvent>
#include <QAction>
#include <QMenu>

#include <KLocalizedString>
#include <kactioncollection.h>
#include <KDescendantsProxyModel>

using namespace KPlato;


ResourceTreeView::ResourceTreeView(QWidget *parent)
    : DoubleTreeViewBase(parent)
{
    setDragPixmap(koIcon("resource-group").pixmap(32));
//    header()->setContextMenuPolicy(Qt::CustomContextMenu);
    setStretchLastSection(false);
    ResourceItemModel *m = new ResourceItemModel(this);
    m->setTeamsEnabled(true);
    setModel(m);
    
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);

    createItemDelegates(m);

    connect(this, &DoubleTreeViewBase::dropAllowed, this, &ResourceTreeView::slotDropAllowed);

}

ResourceItemModel *ResourceTreeView::model() const
{
    return static_cast<ResourceItemModel*>(DoubleTreeViewBase::model());
}

Project *ResourceTreeView::project() const
{
    return model()->project();
}

void ResourceTreeView::setProject(Project *project)
{
    model()->setProject(project);
}

void ResourceTreeView::slotDropAllowed(const QModelIndex &index, int dropIndicatorPosition, QDragMoveEvent *event)
{
    event->ignore();
    if (model()->dropAllowed(index, dropIndicatorPosition, event->mimeData())) {
        event->accept();
    }
}

QObject *ResourceTreeView::currentObject() const
{
    QObject *o = model()->resource(selectionModel()->currentIndex());
    if (!o) {
        o = model()->group(selectionModel()->currentIndex());
    }
    return o;
}

QList<QObject*> ResourceTreeView::selectedObjects() const
{
    QList<QObject*> lst;
//     foreach (const QModelIndex &i, selectionModel()->selectedRows()) {
//         lst << static_cast<QObject*>(i.internalPointer());
//     }
    return lst;
}

QList<ResourceGroup*> ResourceTreeView::selectedGroups() const
{
    QList<ResourceGroup*> gl;
    const ResourceItemModel *m = model();
    for (const QModelIndex &i : selectionModel()->selectedRows()) {
        ResourceGroup *g = m->group(i);
        if (g) {
            gl << g;
        }
    }
    return gl;
}

QList<Resource*> ResourceTreeView::selectedResources() const
{
    QList<Resource*> rl;
    const ResourceItemModel *m = model();
    for (const QModelIndex &i : selectionModel()->selectedRows()) {
        Resource *r = m->resource(i);
        if (r) {
            rl << r;
        }
    }
    return rl;
}

//-----------------------------------
ResourceEditor::ResourceEditor(KoPart *part, KoDocument *doc, QWidget *parent)
    : ViewBase(part, doc, parent)
{
    setXMLFile("ResourceEditorUi.rc");

    Help::add(this,
        xi18nc("@info:whatsthis", 
               "<title>Resource Editor</title>"
               "<para>"
               "Resources are organized in a Resource Breakdown Structure. "
               "Resources can be of type <emphasis>Work</emphasis> or <emphasis>Material</emphasis>. "
               "When assigned to a task, a resource of type <emphasis>Work</emphasis> can affect the duration of the task, while a resource of type <emphasis>Material</emphasis> does not. "
               "A resource must refer to a <emphasis>Calendar</emphasis> defined in the <emphasis>Work and Vacation Editor</emphasis>."
               "<nl/><link url='%1'>More...</link>"
               "</para>", Help::page("Resource_Editor")));

    QVBoxLayout * l = new QVBoxLayout(this);
    l->setMargin(0);
    m_view = new ResourceTreeView(this);
    m_doubleTreeView = m_view;
    connect(this, &ViewBase::expandAll, m_view, &DoubleTreeViewBase::slotExpand);
    connect(this, &ViewBase::collapseAll, m_view, &DoubleTreeViewBase::slotCollapse);

    l->addWidget(m_view);
    setupGui();
    
    m_view->setEditTriggers(m_view->editTriggers() | QAbstractItemView::EditKeyPressed);
    m_view->setDragDropMode(QAbstractItemView::DragDrop);
    m_view->setDropIndicatorShown(true);
    m_view->setDragEnabled (true);
    m_view->setAcceptDrops(true);
//    m_view->setAcceptDropsOnView(true);


    QList<int> lst1; lst1 << 1 << -1;
    QList<int> lst2; lst2 << 0 << ResourceModel::ResourceOvertimeRate;
    m_view->hideColumns(lst1, lst2);
    
    m_view->masterView()->setDefaultColumns(QList<int>() << 0);
    QList<int> show;
    for (int c = 1; c < model()->columnCount(); ++c) {
        if (c != ResourceModel::ResourceOvertimeRate) {
            show << c;
        }
    }
    m_view->slaveView()->setDefaultColumns(show);

    connect(model(), &ItemModelBase::executeCommand, doc, &KoDocument::addCommand);

    connect(m_view, &DoubleTreeViewBase::currentChanged, this, &ResourceEditor::slotCurrentChanged);

    connect(m_view, &DoubleTreeViewBase::selectionChanged, this, &ResourceEditor::slotSelectionChanged);

    connect(m_view, &DoubleTreeViewBase::contextMenuRequested, this, &ResourceEditor::slotContextMenuRequested);
    
    connect(m_view, &DoubleTreeViewBase::headerContextMenuRequested, this, &ViewBase::slotHeaderContextMenuRequested);

    createDockers();
}

void ResourceEditor::updateReadWrite(bool readwrite)
{
    m_view->setReadWrite(readwrite);
}

void ResourceEditor::setProject(Project *project)
{
    debugPlan<<project;
    m_view->setProject(project);
    ViewBase::setProject(project);
}

void ResourceEditor::setGuiActive(bool activate)
{
    debugPlan<<activate;
    updateActionsEnabled(true);
    ViewBase::setGuiActive(activate);
    if (activate && !m_view->selectionModel()->currentIndex().isValid()) {
        m_view->selectionModel()->setCurrentIndex(m_view->model()->index(0, 0), QItemSelectionModel::NoUpdate);
    }
}

void ResourceEditor::slotContextMenuRequested(const QModelIndex &index, const QPoint& pos)
{
    //debugPlan<<index.row()<<","<<index.column()<<":"<<pos;
    QString name;
    if (index.isValid()) {
        Resource *r = m_view->model()->resource(index);
        if (r && !r->isShared()) {
            name = "resourceeditor_resource_popup";
        }
    }
    m_view->setContextMenuIndex(index);
    if (name.isEmpty()) {
        slotHeaderContextMenuRequested(pos);
        m_view->setContextMenuIndex(QModelIndex());
        return;
    }
    Q_EMIT requestPopupMenu(name, pos);
    m_view->setContextMenuIndex(QModelIndex());
}

Resource *ResourceEditor::currentResource() const
{
    return qobject_cast<Resource*>(m_view->currentObject());
}

Resource *ResourceEditor::resource(const QModelIndex &idx) const
{
    return m_proj ? m_proj->resourceAt(idx.row()) : nullptr;
}

void ResourceEditor::slotCurrentChanged(const QModelIndex &idx)
{
}

void ResourceEditor::slotSelectionChanged(const QModelIndexList &lst)
{
    QModelIndexList idxs;
    for (const QModelIndex &idx : lst) {
        if (idx.column() == 0) {
            idxs << idx;
        }
    }
    Resource *r = nullptr;
    if (idxs.count() == 1) {
        r = resource(idxs.at(0));
    }
    Q_EMIT resourceSelected(r);

    //debugPlan<<list.count();
    updateActionsEnabled();
}

void ResourceEditor::slotEnableActions(bool on)
{
    updateActionsEnabled(on);
}

void ResourceEditor::updateActionsEnabled(bool on)
{
    bool o = on && m_view->project();

    const QList<Resource*> resourceList = m_view->selectedResources();
    bool resource = resourceList.count() == 1;

    actionAddResource->setEnabled(o);

    if (o && !resourceList.isEmpty()) {
        for (Resource *r : resourceList) {
            if (r->isBaselined()) {
                o = false;
                break;
            }
        }
    }
    actionDeleteSelection->setEnabled(o && !resourceList.isEmpty());
}

void ResourceEditor::setupGui()
{
    actionAddResource  = new QAction(koIcon("list-add-user"), i18n("Add Resource"), this);
    actionCollection()->addAction("add_resource", actionAddResource);
    actionCollection()->setDefaultShortcut(actionAddResource, Qt::CTRL + Qt::Key_I);
    connect(actionAddResource, &QAction::triggered, this, &ResourceEditor::slotAddResource);
    
    actionDeleteSelection  = new QAction(koIcon("edit-delete"), xi18nc("@action", "Delete"), this);
    actionCollection()->addAction("delete_selection", actionDeleteSelection);
    actionCollection()->setDefaultShortcut(actionDeleteSelection, Qt::Key_Delete);
    connect(actionDeleteSelection, &QAction::triggered, this, &ResourceEditor::slotDeleteSelection);
    
    // Add the context menu actions for the view options
    actionCollection()->addAction(m_view->actionSplitView()->objectName(), m_view->actionSplitView());
    connect(m_view->actionSplitView(), &QAction::triggered, this, &ResourceEditor::slotSplitView);
    addContextAction(m_view->actionSplitView());
    
    createOptionActions(ViewBase::OptionAll);
}

void ResourceEditor::slotSplitView()
{
    debugPlan;
    m_view->setViewSplitMode(! m_view->isViewSplit());
    Q_EMIT optionsModified();
}

void ResourceEditor::slotOptions()
{
    debugPlan;
    SplitItemViewSettupDialog *dlg = new SplitItemViewSettupDialog(this, m_view, this);
    dlg->addPrintingOptions(sender()->objectName() == "print_options");
    connect(dlg, SIGNAL(finished(int)), SLOT(slotOptionsFinished(int)));
    dlg->open();
}


void ResourceEditor::slotAddResource()
{
    //debugPlan;
    QModelIndex i = m_view->model()->insertResource(new Resource());
    if (i.isValid()) {
        m_view->selectionModel()->select(i, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
        m_view->selectionModel()->setCurrentIndex(i, QItemSelectionModel::NoUpdate);
        m_view->edit(i);
    }

}

void ResourceEditor::slotDeleteSelection()
{
    QObjectList lst;
    // FIXME: Temporary to make the old code in kptview work
    for (Resource *r : m_view->selectedResources()) {
        lst << r;
    }
    //debugPlan<<lst.count()<<" objects";
    if (! lst.isEmpty()) {
        Q_EMIT deleteObjectList(lst);
        QModelIndex i = m_view->selectionModel()->currentIndex();
        if (i.isValid()) {
            m_view->selectionModel()->select(i, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
            m_view->selectionModel()->setCurrentIndex(i, QItemSelectionModel::NoUpdate);
        }
    }
}

bool ResourceEditor::loadContext(const KoXmlElement &context)
{
    debugPlan<<objectName();
    ViewBase::loadContext(context);
    return m_view->loadContext(model()->columnMap(), context);
}

void ResourceEditor::saveContext(QDomElement &context) const
{
    debugPlan<<objectName();
    ViewBase::saveContext(context);
    m_view->saveContext(model()->columnMap(), context);
}

KoPrintJob *ResourceEditor::createPrintJob()
{
    return m_view->createPrintJob(this);
}

void ResourceEditor::slotEditCopy()
{
    m_view->editCopy();
}

void ResourceEditor::createDockers()
{
    // Add dockers
    DockWidget *ds = nullptr;
    {
        ds = new DockWidget(this, "Resource Groups", xi18nc("@title", "Resource Groups"));
        QTreeView *x = new QTreeView(ds);
        ParentGroupItemModel *m1 = new ParentGroupItemModel(x);
        m1->setGroupIsCheckable(true);
        m1->setResourcesEnabled(false);
        KDescendantsProxyModel *m2 = new KDescendantsProxyModel(x);
        m2->setSourceModel(m1);
        x->setModel(m2);
        m1->setProject(project());
        x->setRootIsDecorated(false);
        x->setHeaderHidden(true);
        x->setSelectionBehavior(QAbstractItemView::SelectRows);
        x->setSelectionMode(QAbstractItemView::ExtendedSelection);
        x->expandAll();
        x->resizeColumnToContents(0);
//         x->setDragDropMode(QAbstractItemView::DragOnly);
//         x->setDragEnabled (true);
        connect(m1, &ParentGroupItemModel::executeCommand, koDocument(), &KoDocument::addCommand);
        ds->setWidget(x);
        connect(this, &ViewBase::projectChanged, m1, &ParentGroupItemModel::setProject);
        connect(this, &ResourceEditor::resourceSelected, m1, &ParentGroupItemModel::setResource);
//         connect(m1, &ParentGroupItemModel::resizeColumnToContents, x, &QTreeView::resizeColumnToContents);
        addDocker(ds);
    }
}
