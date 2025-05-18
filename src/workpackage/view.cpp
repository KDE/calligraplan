/* This file is part of the KDE project
  SPDX-FileCopyrightText: 1998, 1999, 2000 Torben Weis <weis@kde.org>
  SPDX-FileCopyrightText: 2002-2009, 2011, 2012 Dag Andersen <dag.andersen@kdemail.net>
  SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
  
  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "view.h"
#include "mainwindow.h"
#include "taskworkpackageview.h"
#include "workpackage.h"
#include "taskcompletiondialog.h"
#include "calligraplanworksettings.h"
#include "kpttaskeditor.h"
#include "kpttaskdescriptiondialog.h"
#include "kptcommonstrings.h"

#include "KoDocumentInfo.h"
#include <KoMainWindow.h>

#include <QApplication>
#include <QLabel>
#include <QString>
#include <QSize>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QPrinter>
#include <QPrintDialog>
#include <QDomDocument>
#include <QPointer>
#include <QMenu>
#include <QAction>
#include <QActionGroup>

#include <KLocalizedString>
#include <KToolBar>

#include <KXMLGUIFactory>
#include <KEMailClientLauncherJob>
#include <KDialogJobUiDelegate>
#include <KActionCollection>
#include <QTemporaryFile>

#include <KMessageBox>

#include <KoIcon.h>

#include "part.h"
#include "factory.h"

#include "kptviewbase.h"
#include "kptdocumentseditor.h"

#include "kptnode.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptcommand.h"
#include "kptdocuments.h"
#include "kpttaskprogressdialog.h"
#include "kptcalendar.h"

#include <assert.h>

#include "debugarea.h"

namespace KPlatoWork
{

View::View(Part *part,  QWidget *parent, KActionCollection *collection)
    : QStackedWidget(parent),
    m_part(part),
    m_scheduleActionGroup(new QActionGroup(this)),
    m_manager(nullptr)
{
    m_readWrite = part->isReadWrite();
    debugPlanWork<<m_readWrite;

    // Add sub views
    createViews();

    // The menu items
    // ------ Edit
    actionRemoveSelectedPackages  = new QAction(koIcon("edit-delete"), i18n("Remove Packages"), this);
    collection->addAction(QStringLiteral("package_remove_selected"), actionRemoveSelectedPackages);
    connect(actionRemoveSelectedPackages, &QAction::triggered, this, &View::slotRemoveSelectedPackages);

    actionRemoveCurrentPackage  = new QAction(koIcon("edit-delete"), i18n("Remove Package"), this);
    collection->addAction(QStringLiteral("package_remove_current"), actionRemoveCurrentPackage);
    connect(actionRemoveCurrentPackage, &QAction::triggered, this, &View::slotRemoveCurrentPackage);

    actionViewList  = new QAction(koIcon("view-list-tree"), i18n("List"), this);
    actionViewList->setToolTip(i18nc("@info:tooltip", "Select task list"));
    collection->addAction(QStringLiteral("view_list"), actionViewList);
    connect(actionViewList, &QAction::triggered, this, &View::slotViewList);

    actionViewGantt  = new QAction(koIcon("view-time-schedule"), i18n("Gantt"), this);
    actionViewGantt->setToolTip(i18nc("@info:tooltip", "Select timeline"));
    collection->addAction(QStringLiteral("view_gantt"), actionViewGantt);
    connect(actionViewGantt, &QAction::triggered, this, &View::slotViewGantt);

    //------ Settings
    actionConfigure  = new QAction(koIcon("configure"), i18n("Configure PlanWork..."), this);
    collection->addAction(QStringLiteral("configure"), actionConfigure);
    connect(actionConfigure, &QAction::triggered, this, &View::slotConfigure);

    //------ Popups
    actionEditDocument  = new QAction(koIcon("document-edit"), i18n("Edit..."), this);
    collection->addAction(QStringLiteral("edit_document"), actionEditDocument);
    connect(actionEditDocument, &QAction::triggered, this, QOverload<>::of(&View::slotEditDocument));

    actionViewDocument  = new QAction(koIcon("document-preview"), i18nc("@verb", "View..."), this);
    collection->addAction(QStringLiteral("view_document"), actionViewDocument);
    connect(actionViewDocument, &QAction::triggered, this, &View::slotViewDocument);

    actionRemoveDocument = new QAction(koIcon("list-remove"), i18n("Remove document"), this);
    collection->addAction(QStringLiteral("remove_document"), actionRemoveDocument);
    connect(actionRemoveDocument, &QAction::triggered, this, &View::slotRemoveDocument);

    actionSendPackage  = new QAction(koIcon("mail-send"), i18n("Send Package..."), this);
    collection->addAction(QStringLiteral("edit_sendpackage"), actionSendPackage);
    connect(actionSendPackage, &QAction::triggered, this, &View::slotSendPackage);

    actionTaskCompletion  = new QAction(koIcon("document-edit"), i18n("Edit Progress..."), this);
    collection->addAction(QStringLiteral("task_progress"), actionTaskCompletion);
    connect(actionTaskCompletion, &QAction::triggered, this, &View::slotTaskCompletion);

    actionViewDescription  = new QAction(/*koIcon("document_view"),*/ i18n("View Description..."), this);
    collection->addAction(QStringLiteral("task_description"), actionViewDescription);
    connect(actionViewDescription, &QAction::triggered, this, &View::slotTaskDescription);


    updateReadWrite(m_readWrite);
    //debugPlanWork<<" end";

    loadContext();
    slotCurrentChanged(currentIndex());
    connect(this, &QStackedWidget::currentChanged, this, &View::slotCurrentChanged);

    slotSelectionChanged();
}

View::~View()
{
    saveContext();
}

void View::slotCurrentChanged(int index)
{
    actionViewList->setEnabled(index != 0);
    actionViewGantt->setEnabled(index != 1);
    saveContext();
}

void View::slotViewList()
{
    debugPlanWork;
    setCurrentIndex(0);
}

void View::slotViewGantt()
{
    debugPlanWork;
    setCurrentIndex(1);
}

void View::createViews()
{
    QWidget *v = createTaskWorkPackageView();
    addWidget(v);
    v = createGanttView();
    addWidget(v);
}

TaskWorkPackageView *View::createTaskWorkPackageView()
{
    TaskWorkPackageView *v = new TaskWorkPackageView(part(), this);

    connect(v, &AbstractView::requestPopupMenu, this, &View::slotPopupMenu);

    connect(v, &AbstractView::selectionChanged, this, &View::slotSelectionChanged);
    v->updateReadWrite(m_readWrite);
    v->loadContext();
    return v;
}

TaskWPGanttView *View::createGanttView()
{
    TaskWPGanttView *v = new TaskWPGanttView(part(), this);

    connect(v, &AbstractView::requestPopupMenu, this, &View::slotPopupMenu);

    connect(v, &AbstractView::selectionChanged, this, &View::slotSelectionChanged);
    v->updateReadWrite(m_readWrite);
    v->loadContext();
    return v;
}

void View::setupPrinter(QPrinter &/*printer*/, QPrintDialog &/*printDialog */)
{
    //debugPlanWork;
}

void View::print(QPrinter &/*printer*/, QPrintDialog &/*printDialog*/)
{
}

void View::slotSelectionChanged()
{
    bool enable = ! currentView()->selectedNodes().isEmpty();
    actionRemoveSelectedPackages->setEnabled(enable);
    actionRemoveCurrentPackage->setEnabled(enable);
}

void View::slotEditCut()
{
    //debugPlanWork;
}

void View::slotEditCopy()
{
    //debugPlanWork;
}

void View::slotEditPaste()
{
    //debugPlanWork;
}

void View::slotProgressChanged(int)
{
}

void View::slotConfigure()
{
}

KPlato::ScheduleManager *View::currentScheduleManager() const
{
    WorkPackage *wp = m_part->findWorkPackage(currentNode());
    return wp ? wp->project()->scheduleManagers().value(0) : nullptr;
}

void View::updateReadWrite(bool readwrite)
{
    debugPlanWork<<m_readWrite<<"->"<<readwrite;
    m_readWrite = readwrite;

//    actionTaskProgress->setEnabled(readwrite);

    Q_EMIT sigUpdateReadWrite(readwrite);
}

Part *View::part() const
{
    return m_part;
}

void View::slotPopupMenu(const QString& name, const QPoint & pos)
{
    debugPlanWork<<name;
    Q_ASSERT(m_part->factory());
    if (m_part->factory() == nullptr) {
        return;
    }
    QMenu *menu = ((QMenu*) m_part->factory() ->container(name, m_part));
    if (menu == nullptr) {
        return;
    }
    QList<QAction*> lst;
    AbstractView *v = currentView();
    if (v) {
        lst = v->contextActionList();
        debugPlanWork<<lst;
        if (! lst.isEmpty()) {
            menu->addSeparator();
            for (QAction *a : std::as_const(lst)) {
                menu->addAction(a);
            }
        }
    }
    menu->exec(pos);
    for (QAction *a : std::as_const(lst)) {
        menu->removeAction(a);
    }
}

bool View::loadContext()
{
    debugPlanWork;
    setCurrentIndex(PlanWorkSettings::self()->currentView());
    return true;
}

void View::saveContext() const
{
    debugPlanWork;
    PlanWorkSettings::self()->setCurrentView(currentIndex());
    PlanWorkSettings::self()->save();
}

void View::slotEditDocument()
{
    slotEditDocument(currentDocument());
}

void View::slotEditDocument(KPlato::Document *doc)
{
    debugPlanWork<<doc;
    if (doc == nullptr) {
        debugPlanWork<<"No document";
        return;
    }
    if (doc->type() != KPlato::Document::Type_Product) {
        KMessageBox::error(nullptr, i18n("This file is not editable"));
        return;
    }
    part()->editWorkpackageDocument(doc);
}

void View::slotViewDocument()
{
    Q_EMIT viewDocument(currentDocument());
}

void View::slotRemoveDocument()
{
    part()->removeDocument(currentDocument());
}

void View::slotSendPackage()
{
    KPlato::Node *node = currentNode();
    if (node == nullptr) {
        KMessageBox::error(nullptr, i18n("No work package is selected"));
        return;
    }
    debugPlanWork<<node->name();
    WorkPackage *wp = part()->findWorkPackage(node);
    if (wp == nullptr) {
        KMessageBox::error(nullptr, i18n("Cannot find work package"));
        return;
    }
/*    if (wp->isModified()) {
        int r = KMessageBox::questionTwoActionsCancel(0, i18n("This work package has been modified.\nDo you want to save it before sending?"), node->name());
        switch (r) {
            case KMessageBox::Cancel: return;
            case KMessageBox::PrimaryAction: wp->saveToProjects(part()); break;
            default: break;
        }
    }*/

    bool wasmodified = wp->isModified();
    if (wp->sendUrl().isValid()) {
        QTemporaryFile temp(wp->sendUrl().toLocalFile() + QStringLiteral("/calligraplanwork_XXXXXX") + QStringLiteral(".planwork"));
        temp.setAutoRemove(false);
        if (! temp.open()) {
            KMessageBox::error(nullptr, i18n("Could not open file. Sending is aborted."));
            return;
        }
        wp->saveNativeFormat(part(), temp.fileName());
    } else {
        QTemporaryFile temp(QDir::tempPath() + QStringLiteral("/calligraplanwork_XXXXXX") + QStringLiteral(".planwork"));
        temp.setAutoRemove(false);
        if (! temp.open()) {
            KMessageBox::error(nullptr, i18n("Could not open temporary file. Sending is aborted."));
            return;
        }
        wp->saveNativeFormat(part(), temp.fileName());

        QList<QUrl> attachURLs;
        attachURLs << QUrl::fromUserInput(temp.fileName());

        QString to = node->projectNode()->leader();
        QString subject = i18n("Work Package: %1", node->name());
        QString body = node->projectNode()->name();

        auto job = new KEMailClientLauncherJob();
        job->setTo(QStringList()<<to);
        job->setSubject(subject);
        job->setBody(body);
        job->setAttachments(attachURLs);
        job->setUiDelegate(new KDialogJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, this));
        job->start();
    }
    wp->setModified(wasmodified);
}

void View::slotTaskDescription()
{
    KPlato::Task *node = qobject_cast<KPlato::Task*>(currentNode());
    if (node == nullptr) {
        return;
    }
    QPointer<KPlato::TaskDescriptionDialog> dlg = new KPlato::TaskDescriptionDialog(*node, this, true);
    dlg->exec();
    delete dlg;
}

AbstractView *View::currentView() const
{
    return qobject_cast<AbstractView*>(currentWidget());
}

KPlato::Node *View::currentNode() const
{
    AbstractView *v = currentView();
    return v ? v->currentNode() : nullptr;
}

KPlato::Document *View::currentDocument() const
{
    AbstractView *v = currentView();
    return v ? v->currentDocument() : nullptr;
}

void View::slotTaskProgress()
{
    debugPlanWork;
    KPlato::Task *n = qobject_cast<KPlato::Task*>(currentNode());
    if (n == nullptr) {
        return;
    }
    KPlato::StandardWorktime *w = qobject_cast<KPlato::Project*>(n->projectNode())->standardWorktime();
    QPointer<KPlato::TaskProgressDialog> dlg = new KPlato::TaskProgressDialog(*n, currentScheduleManager(), w, this);
    if (dlg->exec() == QDialog::Accepted && dlg) {
        KUndo2Command *cmd = dlg->buildCommand();
        if (cmd) {
            cmd->redo(); //FIXME m_part->addCommand(cmd);
        }
    }
}

void View::slotTaskCompletion()
{
    debugPlanWork;
    WorkPackage *wp = m_part->findWorkPackage(currentNode());
    if (wp == nullptr) {
        return;
    }
    QPointer<TaskCompletionDialog> dlg = new TaskCompletionDialog(*wp, currentScheduleManager(), this);
    if (dlg->exec() == QDialog::Accepted && dlg) {
        KUndo2Command *cmd = dlg->buildCommand();
        if (cmd) {
            m_part->addCommand(cmd);
        }
    }
    delete dlg;
}

void View::slotRemoveSelectedPackages()
{
    debugPlanWork;
    QList<KPlato::Node*> lst = currentView()->selectedNodes();
    if (lst.isEmpty()) {
        return;
    }
    m_part->removeWorkPackages(lst);
}

void View::slotRemoveCurrentPackage()
{
    debugPlanWork;
    KPlato::Node *n = currentNode();
    if (n == nullptr) {
        return;
    }
    m_part->removeWorkPackage(n);
}


}  //KPlatoWork namespace
