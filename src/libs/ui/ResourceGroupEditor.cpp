/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2020 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2006-2011, 2012 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "ResourceGroupEditor.h"

#include "ResourceGroupDocker.h"
#include "ResourceGroupItemModel.h"
#include "ResourceItemModel.h"
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
#include "kptdebug.h"

#include <KoDocument.h>
#include <KoIcon.h>

#include <QList>
#include <QVBoxLayout>
#include <QDragMoveEvent>
#include <QAction>
#include <QMenu>

#include <KLocalizedString>
#include <KActionCollection>
#include <KCheckableProxyModel>

using namespace KPlato;


ResourceGroupTreeView::ResourceGroupTreeView(QWidget *parent)
    : DoubleTreeViewBase(parent)
{
    setDragPixmap(koIcon("resource-group").pixmap(32));
//    header()->setContextMenuPolicy(Qt::CustomContextMenu);
    setStretchLastSection(false);
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
    const auto groups = selectedGroups();
    for (ResourceGroup *g : groups) {
        lst << g;
    }
    return lst;
}

QList<ResourceGroup*> ResourceGroupTreeView::selectedGroups() const
{
    QList<ResourceGroup*> gl;
    const ResourceGroupItemModel *m = model();
    const auto selectedRows = selectionModel()->selectedRows();
    for (const QModelIndex &i : selectedRows) {
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
    const auto selectedRows = selectionModel()->selectedRows();
    for (const QModelIndex &i : selectedRows) {
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
    setXMLFile(QStringLiteral("ResourceGroupEditorUi.rc"));

    setWhatsThis(
        xi18nc("@info:whatsthis", 
               "<title>Resource Breakdown Structure</title>"
               "<para>"
               "Resources can be organized in a Resource Breakdown Structure. "
               "The structure is purely organizational and has no impact on resource allocations."
               "<nl/><link url='%1'>More...</link>"
               "</para>", QStringLiteral("plan:resource-breakdown-structure")));

    QVBoxLayout * l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
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
    QList<int> lst2; lst2 << 0;
    m_view->hideColumns(lst1, lst2);

    m_view->masterView()->setDefaultColumns(QList<int>() << 0);
    QList<int> show;
    for (int c = 1; c < model()->columnCount(); ++c) {
        show << c;
    }
    m_view->slaveView()->setDefaultColumns(show);

    connect(model(), &ItemModelBase::executeCommand, doc, &KoDocument::addCommand);

    connect(m_view, &DoubleTreeViewBase::currentChanged, this, &ResourceGroupEditor::slotCurrentChanged);

    connect(m_view, &DoubleTreeViewBase::selectionChanged, this, &ResourceGroupEditor::slotSelectionChanged);

    connect(m_view, &DoubleTreeViewBase::contextMenuRequested, this, &ResourceGroupEditor::slotContextMenuRequested);
    
    connect(m_view, &DoubleTreeViewBase::headerContextMenuRequested, this, &ViewBase::slotHeaderContextMenuRequested);

    createDockers();
}

void ResourceGroupEditor::updateReadWrite(bool readwrite)
{
    m_view->setReadWrite(readwrite);
    ViewBase::updateReadWrite(readwrite);
    slotEnableActions(isReadWrite());
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
    ViewBase::setGuiActive(activate);
    if (activate && !m_view->selectionModel()->currentIndex().isValid()) {
        m_view->selectionModel()->setCurrentIndex(m_view->model()->index(0, 0), QItemSelectionModel::NoUpdate);
    }
    updateActionsEnabled(isReadWrite());
}

void ResourceGroupEditor::slotContextMenuRequested(const QModelIndex &index, const QPoint& pos)
{
    //debugPlan<<index.row()<<","<<index.column()<<":"<<pos;
    QString name;
    if (index.isValid()) {
        ResourceGroup *g = m_view->model()->group(index);
        if (g) {
            // No menu atm
            //name = "resourcegroupeditor_group_popup";
        }
    }
    m_view->setContextMenuIndex(index);
    if (name.isEmpty()) {
        slotHeaderContextMenuRequested(pos);
    } else {
        Q_EMIT requestPopupMenu(name, pos);
    }
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
    updateActionsEnabled(isReadWrite());
}

void ResourceGroupEditor::slotEnableActions(bool on)
{
    updateActionsEnabled(on);
}

void ResourceGroupEditor::updateActionsEnabled(bool on)
{
    bool o = on && m_view->project();

    const QList<ResourceGroup*> groupList = m_view->selectedGroups();
    bool nogroup = groupList.isEmpty();
    bool onegroup = groupList.count() == 1;

    actionAddGroup->setEnabled(o);
    actionAddSubGroup->setEnabled(o && onegroup);

    if (o && !nogroup) {
        for (ResourceGroup *g : groupList) {
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
    actionAddGroup = new QAction(koIcon("resource-group-new"), i18n("Add Resource Group"), this);
    actionCollection()->addAction(QStringLiteral("add_group"), actionAddGroup);
    actionCollection()->setDefaultShortcut(actionAddGroup, Qt::CTRL | Qt::Key_I);
    connect(actionAddGroup, &QAction::triggered, this, &ResourceGroupEditor::slotAddGroup);

    actionAddSubGroup = new QAction(koIcon("resource-group-new"), i18n("Add Child Resource Group"), this);
    actionCollection()->addAction(QStringLiteral("add_subgroup"), actionAddSubGroup);
    actionCollection()->setDefaultShortcut(actionAddSubGroup, Qt::SHIFT | Qt::CTRL | Qt::Key_I);
    connect(actionAddSubGroup, &QAction::triggered, this, &ResourceGroupEditor::slotAddSubGroup);
    
    actionDeleteSelection  = new QAction(koIcon("edit-delete"), xi18nc("@action", "Delete"), this);
    actionCollection()->addAction(QStringLiteral("delete_selection"), actionDeleteSelection);
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
    Q_EMIT optionsModified();
}

void ResourceGroupEditor::slotOptions()
{
    debugPlan;
    SplitItemViewSettupDialog *dlg = new SplitItemViewSettupDialog(this, m_view, this);
    dlg->addPrintingOptions(sender()->objectName() == QStringLiteral("print_options"));
    connect(dlg, SIGNAL(finished(int)), SLOT(slotOptionsFinished(int)));
    dlg->open();
}

void ResourceGroupEditor::slotAddGroup()
{
    //debugPlan;
    ResourceGroup *current = currentResourceGroup();
    ResourceGroup *parent = current ? current->parentGroup() : nullptr;
    ResourceGroup *g = new ResourceGroup();
    QModelIndex i = m_view->model()->insertGroup(g, parent);
    if (i.isValid()) {
        m_view->selectionModel()->setCurrentIndex(i, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
        m_view->edit(i);
    }
}

void ResourceGroupEditor::slotAddSubGroup()
{
    //debugPlan;
    ResourceGroup *parent = currentResourceGroup();
    ResourceGroup *g = new ResourceGroup();
    QModelIndex i = m_view->model()->insertGroup(g, parent);
    if (i.isValid()) {
        m_view->selectionModel()->setCurrentIndex(i, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
        m_view->edit(i);
    }
}

void ResourceGroupEditor::slotDeleteSelection()
{
    deleteResourceGroups(m_view->selectedGroups());
}

void ResourceGroupEditor::deleteResourceGroup(ResourceGroup *group)
{
    koDocument()->addCommand(new RemoveResourceGroupCmd(group->project(), group, kundo2_i18nc("@action", "Delete resourcegroup")));
}

void ResourceGroupEditor::deleteResourceGroups(const QList<ResourceGroup*> &groups)
{
    //debugPlan;
    if (groups.isEmpty()) {
        return;
    }
    if (groups.count() == 1) {
        deleteResourceGroup(groups.first());
        return;
    }
    MacroCommand *cmd = new MacroCommand(KUndo2MagicString());
    int num = 0;
    for (auto *g : groups) {
        cmd->addCommand(new RemoveResourceGroupCmd(project(), g));
        ++num;
    }
    if (num == 0) {
        delete cmd;
    } else {
        cmd->setText(kundo2_i18np("Delete resourcegroup", "Delete resourcegroups", num));
        koDocument()->addCommand(cmd);
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

void ResourceGroupEditor::createDockers()
{
    // Add dockers
    DockWidget *ds = new ResourceGroupDocker(m_view->selectionModel(), this, QStringLiteral("Resources"), xi18nc("@title", "Resources"));
    addDocker(ds);
}
