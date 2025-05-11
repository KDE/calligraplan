/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2006-2007 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "kptresourceeditor.h"

#include "ResourceItemModel.h"
#include "ResourceGroupItemModel.h"
#include "kptcommand.h"
#include <RemoveResourceCmd.h>
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
#include "kptresourcedialog.h"

#include <KoDocument.h>
#include <KoIcon.h>

#include <QList>
#include <QVBoxLayout>
#include <QDragMoveEvent>
#include <QAction>
#include <QMenu>

#include <KLocalizedString>
#include <KActionCollection>
#include <KDescendantsProxyModel>
#include <KMessageBox>

using namespace KPlato;


ResourceTreeView::ResourceTreeView(QWidget *parent)
    : DoubleTreeViewBase(parent)
{
    setDragPixmap(koIcon("resource-group").pixmap(32));
//    header()->setContextMenuPolicy(Qt::CustomContextMenu);
    setStretchLastSection(false);
    ResourceItemModel *m = new ResourceItemModel(this);
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
    const auto selectedRows = selectionModel()->selectedRows();
    for (const QModelIndex &i : selectedRows) {
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
ResourceEditor::ResourceEditor(KoPart *part, KoDocument *doc, QWidget *parent)
    : ViewBase(part, doc, parent)
{
    setXMLFile(QStringLiteral("ResourceEditorUi.rc"));

    setWhatsThis(
        xi18nc("@info:whatsthis", 
               "<title>Resource Editor</title>"
               "<para>"
               "Resources are organized in a Resource Breakdown Structure. "
               "Resources can be of type <emphasis>Work</emphasis> or <emphasis>Material</emphasis>. "
               "When assigned to a task, a resource of type <emphasis>Work</emphasis> can affect the duration of the task, while a resource of type <emphasis>Material</emphasis> does not. "
               "A resource must refer to a <emphasis>Calendar</emphasis> defined in the <emphasis>Work and Vacation Editor</emphasis>."
               "<nl/><link url='%1'>More...</link>"
               "</para>", QStringLiteral("plan:resource-editor")));

    QVBoxLayout * l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
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
    
    connect(m_view, &DoubleTreeViewBase::headerContextMenuRequested, this, &ResourceEditor::slotHeaderContextMenuRequested);

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

void ResourceEditor::slotHeaderContextMenuRequested(const QPoint& pos)
{
    slotContextMenuRequested(QModelIndex(), pos);
}

void ResourceEditor::slotContextMenuRequested(const QModelIndex &index, const QPoint& pos)
{
    //debugPlan<<index.row()<<","<<index.column()<<":"<<pos;
    QString name = QStringLiteral("resourceeditor_context_popup");
    if (index.isValid()) {
        Resource *r = m_view->model()->resource(index);
        if (r && !r->isShared()) {
            name = QStringLiteral("resourceeditor_resource_popup");
        }
    }
    m_view->setContextMenuIndex(index);
    if (name.isEmpty()) {
        slotHeaderContextMenuRequested(pos);
    } else {
        openContextMenu(name, pos);
    }
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
    Q_UNUSED(idx)
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
    actionCollection()->addAction(QStringLiteral("add_resource"), actionAddResource);
    actionCollection()->setDefaultShortcut(actionAddResource, Qt::CTRL | Qt::Key_I);
    connect(actionAddResource, &QAction::triggered, this, &ResourceEditor::slotAddResource);
    
    actionDeleteSelection  = new QAction(koIcon("edit-delete"), xi18nc("@action", "Delete"), this);
    actionCollection()->addAction(QStringLiteral("delete_selection"), actionDeleteSelection);
    actionCollection()->setDefaultShortcut(actionDeleteSelection, Qt::Key_Delete);
    connect(actionDeleteSelection, &QAction::triggered, this, &ResourceEditor::slotDeleteSelection);

    auto actionEditResource  = new QAction(koIcon("document-edit"), i18n("Edit Resource..."), this);
    actionCollection()->addAction(QStringLiteral("edit_resource"), actionEditResource);
    connect(actionEditResource, &QAction::triggered, this, &ResourceEditor::slotEditCurrentResource);

    auto a = new QAction(i18nc("@action:inmenu", "Show Resource Groups"));
    a->setObjectName(QStringLiteral("resourceeditor_show_groups"));
    a->setCheckable(true);
    connect(a, &QAction::triggered, m_view->model(), &ResourceItemModel::setGroupsEnabled);
    actionCollection()->addAction(a->objectName(), a);

    a = new QAction(i18nc("@action:inmenu", "Show Team Members"));
    a->setObjectName(QStringLiteral("resourceeditor_show_teams"));
    a->setCheckable(true);
    connect(a, &QAction::triggered, m_view->model(), &ResourceItemModel::setTeamsEnabled);
    actionCollection()->addAction(a->objectName(), a);
    a->setChecked(true);
    m_view->model()->setTeamsEnabled(true);

    a = new QAction(i18nc("@action:inmenu", "Show Required Resources"));
    a->setObjectName(QStringLiteral("resourceeditor_show_required"));
    a->setCheckable(true);
    connect(a, &QAction::triggered, m_view->model(), &ResourceItemModel::setRequiredEnabled);
    actionCollection()->addAction(a->objectName(), a);
    a->setChecked(true);
    m_view->model()->setRequiredEnabled(true);

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
    dlg->addPrintingOptions(sender()->objectName() == QStringLiteral("print_options"));
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
    QList<Resource*> lst;
    // FIXME: Temporary to make the old code in kptview work
    const auto selectedResources = m_view->selectedResources();
    for (Resource *r : selectedResources) {
        lst << r;
    }
    if (! lst.isEmpty()) {
        deleteResources(lst);
        QModelIndex i = m_view->selectionModel()->currentIndex();
        if (i.isValid()) {
            m_view->selectionModel()->select(i, QItemSelectionModel::Rows | QItemSelectionModel::ClearAndSelect);
            m_view->selectionModel()->setCurrentIndex(i, QItemSelectionModel::NoUpdate);
        }
    }
}

void ResourceEditor::deleteResources(const QList<Resource*> &resources)
{
    //debugPlan;
    if (resources.isEmpty()) {
        return;
    }
    for (auto r : resources) {
        if (r->isScheduled()) {
            KMessageBox::ButtonCode res = KMessageBox::warningContinueCancel(this, i18n("A resource that has been scheduled will be deleted. This will invalidate the schedule."));
            if (res == KMessageBox::Cancel) {
                return;
            }
            break;
        }
    }
    if (resources.count() == 1) {
        deleteResource(resources.first());
        return;
    }
    MacroCommand *cmd = new MacroCommand(KUndo2MagicString());
    int num = 0;
    for (auto r : resources) {
        cmd->addCommand(new RemoveResourceCmd(r));
        ++num;
    }
    if (num == 0) {
        delete cmd;
    } else {
        cmd->setText(kundo2_i18np("Delete resource", "Delete resources", num));
        koDocument()->addCommand(cmd);
    }
}

void ResourceEditor::deleteResource(Resource *resource)
{
    koDocument()->addCommand(new RemoveResourceCmd(resource, kundo2_i18nc("@action", "Delete resource")));
}

void ResourceEditor::slotEditCurrentResource()
{
    //debugPlan;
    slotEditResource(currentResource());
}

void ResourceEditor::slotEditResource(KPlato::Resource *resource)
{
    if (resource == nullptr) {
        return ;
    }
    ResourceDialog *dia = new ResourceDialog(*project(), resource, m_view);
    connect(dia, &QDialog::finished, this, &ResourceEditor::slotEditResourceFinished);
    dia->open();
}

void ResourceEditor::slotEditResourceFinished(int result)
{
    //debugPlan;
    ResourceDialog *dia = qobject_cast<ResourceDialog*>(sender());
    if (dia == nullptr) {
        return ;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command * cmd = dia->buildCommand();
        if (cmd)
            koDocument()->addCommand(cmd);
    }
    dia->deleteLater();
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
        ds = new DockWidget(this, QStringLiteral("Resource Groups"), xi18nc("@title", "Resource Groups"));
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
