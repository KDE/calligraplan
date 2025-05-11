/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005-2010, 2012 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptresourceappointmentsview.h"

#include "kptappointment.h"
#include "kptcommand.h"
#include "kpteffortcostmap.h"
#include "kptitemmodelbase.h"
#include "kptcalendar.h"
#include "kptduration.h"
#include "kptnode.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptresource.h"
#include "kptdatetime.h"
#include "kptitemviewsettup.h"
#include "kptviewbase.h"
#include "kptdebug.h"
#include "kpttaskdialog.h"
#include "kptsummarytaskdialog.h"
#include "kpttaskdescriptiondialog.h"
#include "kpttaskprogressdialog.h"
#include "kptmilestoneprogressdialog.h"
#include "kptdocumentsdialog.h"

#include "KoPageLayoutWidget.h"
#include <KoDocument.h>
#include <KoXmlReader.h>
#include <KoIcon.h>

#include <KLocalizedString>
#include <KActionCollection>

#include <QList>
#include <QVBoxLayout>
#include <QTabWidget>


namespace KPlato
{

ResourceAppointmentsConfigDialog::ResourceAppointmentsConfigDialog(ViewBase *view, ResourceAppointmentsTreeView *treeview, QWidget *p, bool selectPrint)
    : KPageDialog(p)
    , m_view(view)
    , m_treeview(treeview)
{
    setWindowTitle(i18n("Settings"));
    setFaceType(KPageDialog::Plain); // only one page, KPageDialog will use margins

    QTabWidget *tab = new QTabWidget();

    QWidget *w = ViewBase::createPageLayoutWidget(view);
    tab->addTab(w, w->windowTitle());
    m_pagelayout = w->findChild<KoPageLayoutWidget*>();
    Q_ASSERT(m_pagelayout);

    m_headerfooter = ViewBase::createHeaderFooterWidget(view);
    m_headerfooter->setOptions(view->printingOptions());
    tab->addTab(m_headerfooter, m_headerfooter->windowTitle());

    KPageWidgetItem *page = addPage(tab, i18n("Printing"));
    page->setHeader(i18n("Printing Options"));
    if (selectPrint) {
        setCurrentPage(page);
    }
    connect(this, &QDialog::accepted, this, [this]() {
        m_view->setPageLayout(m_pagelayout->pageLayout());
        m_view->setPrintingOptions(m_headerfooter->options());
    });
}

ResourceAppointmentsTreeView::ResourceAppointmentsTreeView(QWidget *parent)
    : DoubleTreeViewBase(true, parent)
{
//    header()->setContextMenuPolicy(Qt::CustomContextMenu);
    m_rightview->setStretchLastSection(false);

    ResourceAppointmentsItemModel *m = new ResourceAppointmentsItemModel(this);
    setModel(m);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    QList<int> lst1; lst1 << 2 << -1;
    QList<int> lst2; lst2 << 0 << 1;
    hideColumns(lst1, lst2);

    m_leftview->resizeColumnToContents (1);
    connect(m, &QAbstractItemModel::modelReset, this, &ResourceAppointmentsTreeView::slotRefreshed);

    m_rightview->setObjectName(QStringLiteral("ResourceAppointments"));
}

bool ResourceAppointmentsTreeView::loadContext(const KoXmlElement &context)
{
    debugPlan;
    DoubleTreeViewBase::loadContext(QMetaEnum(), context);
    return true;
}

void ResourceAppointmentsTreeView::saveContext(QDomElement &settings) const
{
    debugPlan;
    DoubleTreeViewBase::saveContext(QMetaEnum(), settings);
}

void ResourceAppointmentsTreeView::slotRefreshed()
{
    //debugPlan<<model()->columnCount()<<", "<<m_leftview->header()->count()<<", "<<m_rightview->header()->count()<<", "<<m_leftview->header()->hiddenSectionCount()<<", "<<m_rightview->header()->hiddenSectionCount();
    ResourceAppointmentsItemModel *m = model();
    setModel(nullptr);
    setModel(m);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    QList<int> lst1; lst1 << 2 << -1;
    QList<int> lst2; lst2 << 0 << 1;
    hideColumns(lst1, lst2);
}

QModelIndex ResourceAppointmentsTreeView::currentIndex() const
{
    return selectionModel()->currentIndex();
}

//-----------------------------------

ResourceAppointmentsView::ResourceAppointmentsView(KoPart *part, KoDocument *doc, QWidget *parent)
    : ViewBase(part, doc, parent)
{
    debugPlan<<"------------------- ResourceAppointmentsView -----------------------";
    setXMLFile(QStringLiteral("ResourceAppointmentsViewUi.rc"));
    setupGui();

    QVBoxLayout * l = new QVBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    m_view = new ResourceAppointmentsTreeView(this);
    connect(this, &ViewBase::expandAll, m_view, &DoubleTreeViewBase::slotExpand);
    connect(this, &ViewBase::collapseAll, m_view, &DoubleTreeViewBase::slotCollapse);

    l->addWidget(m_view);

    m_view->setEditTriggers(m_view->editTriggers() | QAbstractItemView::EditKeyPressed);

    connect(model(), &ItemModelBase::executeCommand, doc, &KoDocument::addCommand);

    connect(m_view, &DoubleTreeViewBase::currentChanged, this, &ResourceAppointmentsView::slotCurrentChanged);

    connect(m_view, &DoubleTreeViewBase::selectionChanged, this, &ResourceAppointmentsView::slotSelectionChanged);

    connect(m_view, &DoubleTreeViewBase::contextMenuRequested, this, &ResourceAppointmentsView::slotContextMenuRequested);

    connect(m_view, &DoubleTreeViewBase::headerContextMenuRequested, this, &ViewBase::slotHeaderContextMenuRequested);

    setWhatsThis(
              xi18nc("@info:whatsthis",
                     "<title>Resource Assignments View</title>"
                     "<para>"
                     "Displays the scheduled resource - task assignments."
                     "</para><para>"
                     "This view supports configuration and printing using the context menu."
                     "<nl/><link url='%1'>More...</link>"
                     "</para>", QStringLiteral("plan:resource-assignment-view")));
}

void ResourceAppointmentsView::draw(Project &project)
{
    setProject(&project);
}

void ResourceAppointmentsView::setProject(Project *project)
{
    m_view->setProject(project);
}

void ResourceAppointmentsView::setScheduleManager(ScheduleManager *sm)
{
    if (!sm && scheduleManager()) {
        // we should only get here if the only schedule manager is scheduled,
        // or when last schedule manager is deleted
        m_domdoc.clear();
        QDomElement element = m_domdoc.createElement(QStringLiteral("expanded"));
        m_domdoc.appendChild(element);
        m_view->masterView()->saveExpanded(element);
    }
    bool tryexpand = sm && !scheduleManager();
    bool expand = sm && scheduleManager() && sm != scheduleManager();
    QDomDocument doc;
    if (expand) {
        QDomElement element = doc.createElement(QStringLiteral("expanded"));
        doc.appendChild(element);
        m_view->masterView()->saveExpanded(element);
    }
    ViewBase::setScheduleManager(sm);
    m_view->setScheduleManager(sm);

    if (expand) {
        m_view->masterView()->doExpand(doc);
    } else if (tryexpand) {
        m_view->masterView()->doExpand(m_domdoc);
    }
}

void ResourceAppointmentsView::draw()
{
}

void ResourceAppointmentsView::setGuiActive(bool activate)
{
    debugPlan<<activate;
    updateActionsEnabled(true);
    ViewBase::setGuiActive(activate);
    if (activate && !m_view->selectionModel()->currentIndex().isValid()) {
        m_view->selectionModel()->setCurrentIndex(m_view->model()->index(0, 0), QItemSelectionModel::NoUpdate);
    }
}

void ResourceAppointmentsView::slotContextMenuRequested(const QModelIndex &index, const QPoint& pos)
{
    debugPlan<<index<<pos;
    QString name;
    if (index.isValid()) {
        Node *n = m_view->model()->node(index);
        if (n) {
            name = QStringLiteral("taskview_popup");
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

Node *ResourceAppointmentsView::currentNode() const
{
    return m_view->model()->node(m_view->currentIndex());
}

Resource *ResourceAppointmentsView::currentResource() const
{
    //return qobject_cast<Resource*>(m_view->currentObject());
    return nullptr;
}

ResourceGroup *ResourceAppointmentsView::currentResourceGroup() const
{
    //return qobject_cast<ResourceGroup*>(m_view->currentObject());
    return nullptr;
}

void ResourceAppointmentsView::slotCurrentChanged(const QModelIndex &)
{
    //debugPlan<<curr.row()<<", "<<curr.column();
//    slotEnableActions();
}

void ResourceAppointmentsView::slotSelectionChanged(const QModelIndexList&)
{
    //debugPlan<<list.count();
    updateActionsEnabled();
}

void ResourceAppointmentsView::slotEnableActions(bool on)
{
    updateActionsEnabled(on);
}

void ResourceAppointmentsView::updateActionsEnabled(bool on)
{
/*    bool o = on && m_view->project();

    QList<ResourceGroup*> groupList = m_view->selectedGroups();
    bool nogroup = groupList.isEmpty();
    bool group = groupList.count() == 1;
    QList<Resource*> resourceList = m_view->selectedResources();
    bool noresource = resourceList.isEmpty();
    bool resource = resourceList.count() == 1;

    bool any = !nogroup || !noresource;

    actionAddResource->setEnabled(o && ((group  && noresource) || (resource && nogroup)));
    actionAddGroup->setEnabled(o);
    actionDeleteSelection->setEnabled(o && any);*/

    const auto node  = currentNode();
    bool enable = on && node && (m_view->selectionModel()->selectedRows().count() == 1);

    const auto c = actionCollection();
    if (auto a = c->action(QStringLiteral("task_progress"))) { a->setEnabled(false); }
    if (auto a = c->action(QStringLiteral("task_description"))) { a->setEnabled(enable); }
    if (auto a = c->action(QStringLiteral("task_documents"))) { a->setEnabled(enable); }

    if (enable) {
        auto sid = scheduleManager() ? scheduleManager()->scheduleId() : -1;
        switch (node->type()) {
            case Node::Type_Task:
            case Node::Type_Milestone:
                if (auto a = c->action(QStringLiteral("task_progress"))) { a->setEnabled(enable && node->isScheduled(sid)); }
                break;
            default:
                if (auto a = c->action(QStringLiteral("task_progress"))) { a->setEnabled(false); }
                break;
        }
    }
}

void ResourceAppointmentsView::setupGui()
{
    // Add the context menu actions for the view options
    createOptionActions(ViewBase::OptionAll & ~OptionViewConfig);

    auto actionTaskProgress  = new QAction(koIcon("document-edit"), i18n("Progress..."), this);
    actionCollection()->addAction(QStringLiteral("task_progress"), actionTaskProgress);
    connect(actionTaskProgress, &QAction::triggered, this, &ResourceAppointmentsView::slotTaskProgress);

    auto actionTaskDescription  = new QAction(koIcon("document-edit"), i18n("Description..."), this);
    actionCollection()->addAction(QStringLiteral("task_description"), actionTaskDescription);
    connect(actionTaskDescription, &QAction::triggered, this, &ResourceAppointmentsView::slotTaskDescription);

    auto actionDocuments  = new QAction(koIcon("document-edit"), i18n("Documents..."), this);
    actionCollection()->addAction(QStringLiteral("task_documents"), actionDocuments);
    connect(actionDocuments, &QAction::triggered, this, &ResourceAppointmentsView::slotDocuments);
}

void ResourceAppointmentsView::slotOptions()
{
    auto dlg = new ResourceAppointmentsConfigDialog(this, m_view, this);
    connect(dlg, &ResourceAppointmentsConfigDialog::finished, this, &ResourceAppointmentsView::slotOptionsFinished);
    dlg->open();
}


void ResourceAppointmentsView::slotAddResource()
{
    //debugPlan;
/*    QList<ResourceGroup*> gl = m_view->selectedGroups();
    if (gl.count() > 1) {
        return;
    }
    ResourceGroup *g = 0;
    if (!gl.isEmpty()) {
        g = gl.first();
    } else {
        QList<Resource*> rl = m_view->selectedResources();
        if (rl.count() != 1) {
            return;
        }
        g = rl.first()->parentGroup();
    }
    if (g == 0) {
        return;
    }
    Resource *r = new Resource();
    QModelIndex i = m_view->model()->insertResource(g, r);
    if (i.isValid()) {
        m_view->selectionModel()->setCurrentIndex(i, QItemSelectionModel::NoUpdate);
        m_view->edit(i);
    }
*/
}

void ResourceAppointmentsView::slotAddGroup()
{
    //debugPlan;
/*    ResourceGroup *g = new ResourceGroup();
    QModelIndex i = m_view->model()->insertGroup(g);
    if (i.isValid()) {
        m_view->selectionModel()->setCurrentIndex(i, QItemSelectionModel::NoUpdate);
        m_view->edit(i);
    }*/
}

void ResourceAppointmentsView::slotDeleteSelection()
{
/*    QObjectList lst = m_view->selectedObjects();
    //debugPlan<<lst.count()<<" objects";
    if (! lst.isEmpty()) {
        Q_EMIT deleteObjectList(lst);
    }*/
}

bool ResourceAppointmentsView::loadContext(const KoXmlElement &context)
{
    ViewBase::loadContext(context);
    return m_view->loadContext(context);
}

void ResourceAppointmentsView::saveContext(QDomElement &context) const
{
    ViewBase::saveContext(context);
    m_view->saveContext(context);
}

KoPrintJob *ResourceAppointmentsView::createPrintJob()
{
    return m_view->createPrintJob(this);
}

void ResourceAppointmentsView::slotTaskProgress()
{
    //debugPlan;
    Node * node = currentNode();
    if (!node)
        return ;

    switch (node->type()) {
        case Node::Type_Task: {
                Task *task = dynamic_cast<Task *>(node);
                Q_ASSERT(task);
                TaskProgressDialog *dia = new TaskProgressDialog(*task, scheduleManager(),  project()->standardWorktime(), this);
                connect(dia, &QDialog::finished, this, &ResourceAppointmentsView::slotTaskProgressFinished);
                dia->open();
                break;
            }
        default:
            break;
    }
}

void ResourceAppointmentsView::slotTaskProgressFinished(int result)
{
    TaskProgressDialog *dia = qobject_cast<TaskProgressDialog*>(sender());
    if (dia == nullptr) {
        return;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command * m = dia->buildCommand();
        if (m) {
            koDocument()->addCommand(m);
        }
    }
    dia->deleteLater();
}

void ResourceAppointmentsView::slotTaskDescription()
{
    slotOpenTaskDescription(!isReadWrite());
}

void ResourceAppointmentsView::slotOpenTaskDescription(bool ro)
{
    //debugPlan;
    Node * node = currentNode();
    if (!node)
        return ;

    switch (node->type()) {
        case Node::Type_Task: {
                TaskDescriptionDialog *dia = new TaskDescriptionDialog(*node, this, ro);
                connect(dia, &QDialog::finished, this, &ResourceAppointmentsView::slotTaskDescriptionFinished);
                dia->open();
                break;
            }
        default:
            break;
    }
}

void ResourceAppointmentsView::slotTaskDescriptionFinished(int result)
{
    TaskDescriptionDialog *dia = qobject_cast<TaskDescriptionDialog*>(sender());
    if (dia == nullptr) {
        return;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command * m = dia->buildCommand();
        if (m) {
            koDocument()->addCommand(m);
        }
    }
    dia->deleteLater();
}

void ResourceAppointmentsView::slotDocuments()
{
    //debugPlan;
    Node * node = currentNode();
    if (!node) {
        return ;
    }
    switch (node->type()) {
        case Node::Type_Task: {
            DocumentsDialog *dia = new DocumentsDialog(*node, this);
            connect(dia, &QDialog::finished, this, &ResourceAppointmentsView::slotDocumentsFinished);
            dia->open();
            break;
        }
        default:
            break;
    }
}

void ResourceAppointmentsView::slotDocumentsFinished(int result)
{
    DocumentsDialog *dia = qobject_cast<DocumentsDialog*>(sender());
    if (dia == nullptr) {
        return;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command * m = dia->buildCommand();
        if (m) {
            koDocument()->addCommand(m);
        }
    }
    dia->deleteLater();
}

} // namespace KPlato
