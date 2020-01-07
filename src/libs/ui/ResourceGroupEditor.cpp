/* This file is part of the KDE project
 * Copyright (C) 2020 Dag Andersen <danders@get2net.dk>
 * Copyright (C) 2019 Dag Andersen <danders@get2net.dk>
 * Copyright (C) 2006 - 2011, 2012 Dag Andersen <danders@get2net.dk>
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
#include "ResourceGroupEditor.h"

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

using namespace KPlato;


ResourceGroupTreeView::ResourceGroupTreeView(QWidget *parent)
    : DoubleTreeViewBase(parent)
{
    setDragPixmap(koIcon("resource-group").pixmap(32));
//    header()->setContextMenuPolicy(Qt::CustomContextMenu);
    setStretchLastSection(false);
    setRootIsDecorated(false);
    ResourceGroupItemModel *m = new ResourceGroupItemModel(this);
    m->setResourcesEnabled(true);
    setModel(m);
    
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);

    createItemDelegates(m);

    connect(this, &DoubleTreeViewBase::dropAllowed, this, &ResourceGroupTreeView::slotDropAllowed);

}

ResourceGroupItemModel *ResourceGroupTreeView::model() const
{
    return static_cast<ResourceGroupItemModel*>(DoubleTreeViewBase::model());
}

Project *ResourceGroupTreeView::project() const
{
    return model()->project();
}

void ResourceGroupTreeView::setProject(Project *project)
{
    model()->setProject(project);
}

void ResourceGroupTreeView::slotDropAllowed(const QModelIndex &index, int dropIndicatorPosition, QDragMoveEvent *event)
{
    event->ignore();
    if (model()->dropAllowed(index, dropIndicatorPosition, event->mimeData())) {
        event->accept();
    }
}

QObject *ResourceGroupTreeView::currentObject() const
{
    const ResourceGroupItemModel *m = model();
    const QModelIndex &i = selectionModel()->currentIndex();
    QObject *o = m->group(i);
    if (!o) {
        o = m->resource(i);
    }
    return o;
}

QList<QObject*> ResourceGroupTreeView::selectedObjects() const
{
    QList<QObject*> lst;
//     foreach (const QModelIndex &i, selectionModel()->selectedRows()) {
//         lst << static_cast<QObject*>(i.internalPointer());
//     }
    return lst;
}

QList<ResourceGroup*> ResourceGroupTreeView::selectedGroups() const
{
    QList<ResourceGroup*> gl;
    const ResourceGroupItemModel *m = model();
    for (const QModelIndex &i : selectionModel()->selectedRows()) {
        ResourceGroup *g = m->group(i);
        if (g) {
            gl << g;
        }
    }
    return gl;
}

QList<Resource*> ResourceGroupTreeView::selectedResources() const
{
    QList<Resource*> rl;
    const ResourceGroupItemModel *m = model();
    for (const QModelIndex &i : selectionModel()->selectedRows()) {
        Resource *r = m->resource(i);
        if (r) {
            rl << r;
        }
    }
    return rl;
}

//-----------------------------------
ResourceGroupEditor::ResourceGroupEditor(KoPart *part, KoDocument *doc, QWidget *parent)
    : ViewBase(part, doc, parent)
{
    if (doc && doc->isReadWrite()) {
        setXMLFile("ResourceGroupEditorUi.rc");
    } else {
        setXMLFile("ResourceGroupEditorUi_readonly.rc");
    }

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
    m_view = new ResourceGroupTreeView(this);
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

    connect(m_view, &DoubleTreeViewBase::currentChanged, this, &ResourceGroupEditor::slotCurrentChanged);

    connect(m_view, &DoubleTreeViewBase::selectionChanged, this, &ResourceGroupEditor::slotSelectionChanged);

    connect(m_view, &DoubleTreeViewBase::contextMenuRequested, this, &ResourceGroupEditor::slotContextMenuRequested);
    
    connect(m_view, &DoubleTreeViewBase::headerContextMenuRequested, this, &ViewBase::slotHeaderContextMenuRequested);

}

void ResourceGroupEditor::updateReadWrite(bool readwrite)
{
    m_view->setReadWrite(readwrite);
}

void ResourceGroupEditor::setProject(Project *project)
{
    debugPlan<<project;
    m_view->setProject(project);
    ViewBase::setProject(project);
}

void ResourceGroupEditor::setGuiActive(bool activate)
{
    debugPlan<<activate;
    updateActionsEnabled(true);
    ViewBase::setGuiActive(activate);
    if (activate && !m_view->selectionModel()->currentIndex().isValid()) {
        m_view->selectionModel()->setCurrentIndex(m_view->model()->index(0, 0), QItemSelectionModel::NoUpdate);
    }
}

void ResourceGroupEditor::slotContextMenuRequested(const QModelIndex &index, const QPoint& pos)
{
    //debugPlan<<index.row()<<","<<index.column()<<":"<<pos;
    QString name;
    if (index.isValid()) {
        ResourceGroup *g = m_view->model()->group(index);
        if (g) {
            name = "resourcegroupeditor_group_popup";
        }
    }
    m_view->setContextMenuIndex(index);
    if (name.isEmpty()) {
        slotHeaderContextMenuRequested(pos);
        m_view->setContextMenuIndex(QModelIndex());
        return;
    }
    emit requestPopupMenu(name, pos);
    m_view->setContextMenuIndex(QModelIndex());
}

ResourceGroup *ResourceGroupEditor::currentResourceGroup() const
{
    return qobject_cast<ResourceGroup*>(m_view->currentObject());
}

void ResourceGroupEditor::slotCurrentChanged(const QModelIndex &)
{
    //debugPlan<<curr.row()<<","<<curr.column();
//    slotEnableActions();
}

void ResourceGroupEditor::slotSelectionChanged(const QModelIndexList&)
{
    //debugPlan<<list.count();
    updateActionsEnabled();
}

void ResourceGroupEditor::slotEnableActions(bool on)
{
    updateActionsEnabled(on);
}

void ResourceGroupEditor::updateActionsEnabled(bool on)
{
    bool o = on && m_view->project();

    QList<ResourceGroup*> groupList = m_view->selectedGroups();
    bool nogroup = groupList.isEmpty();
    bool onegroup = groupList.count() == 1;

    actionAddGroup->setEnabled(o);

    if (o && !nogroup) {
        foreach (ResourceGroup *g, groupList) {
            if (g->isBaselined()) {
                o = false;
                break;
            }
        }
    }
    actionDeleteSelection->setEnabled(o && !nogroup);
}

void ResourceGroupEditor::setupGui()
{
    actionAddGroup  = new QAction(koIcon("resource-group-new"), i18n("Add Resource Group"), this);
    actionCollection()->addAction("add_group", actionAddGroup);
    actionCollection()->setDefaultShortcut(actionAddGroup, Qt::CTRL + Qt::Key_I);
    connect(actionAddGroup, &QAction::triggered, this, &ResourceGroupEditor::slotAddGroup);
    
    actionDeleteSelection  = new QAction(koIcon("edit-delete"), xi18nc("@action", "Delete"), this);
    actionCollection()->addAction("delete_selection", actionDeleteSelection);
    actionCollection()->setDefaultShortcut(actionDeleteSelection, Qt::Key_Delete);
    connect(actionDeleteSelection, &QAction::triggered, this, &ResourceGroupEditor::slotDeleteSelection);
    
    // Add the context menu actions for the view options
    actionCollection()->addAction(m_view->actionSplitView()->objectName(), m_view->actionSplitView());
    connect(m_view->actionSplitView(), &QAction::triggered, this, &ResourceGroupEditor::slotSplitView);
    addContextAction(m_view->actionSplitView());
    
    createOptionActions(ViewBase::OptionAll);
}

void ResourceGroupEditor::slotSplitView()
{
    debugPlan;
    m_view->setViewSplitMode(! m_view->isViewSplit());
    emit optionsModified();
}

void ResourceGroupEditor::slotOptions()
{
    debugPlan;
    SplitItemViewSettupDialog *dlg = new SplitItemViewSettupDialog(this, m_view, this);
    dlg->addPrintingOptions(sender()->objectName() == "print_options");
    connect(dlg, SIGNAL(finished(int)), SLOT(slotOptionsFinished(int)));
    dlg->open();
}

void ResourceGroupEditor::slotAddGroup()
{
    //debugPlan;
    ResourceGroup *g = new ResourceGroup();
    QModelIndex i = m_view->model()->insertGroup(g);
    if (i.isValid()) {
        m_view->selectionModel()->select(i, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
        m_view->selectionModel()->setCurrentIndex(i, QItemSelectionModel::NoUpdate);
        m_view->edit(i);
    }
}

void ResourceGroupEditor::slotDeleteSelection()
{
    QObjectList lst = m_view->selectedObjects();
    //debugPlan<<lst.count()<<" objects";
    if (! lst.isEmpty()) {
        emit deleteObjectList(lst);
        QModelIndex i = m_view->selectionModel()->currentIndex();
        if (i.isValid()) {
            m_view->selectionModel()->select(i, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
            m_view->selectionModel()->setCurrentIndex(i, QItemSelectionModel::NoUpdate);
        }
    }
}

bool ResourceGroupEditor::loadContext(const KoXmlElement &context)
{
    debugPlan<<objectName();
    ViewBase::loadContext(context);
    return m_view->loadContext(model()->columnMap(), context);
}

void ResourceGroupEditor::saveContext(QDomElement &context) const
{
    debugPlan<<objectName();
    ViewBase::saveContext(context);
    m_view->saveContext(model()->columnMap(), context);
}

KoPrintJob *ResourceGroupEditor::createPrintJob()
{
    return m_view->createPrintJob(this);
}

void ResourceGroupEditor::slotEditCopy()
{
    m_view->editCopy();
}