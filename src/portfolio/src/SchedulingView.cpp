/* This file is part of the KDE project
 * Copyright (C) 2021 Dag Andersen <dag.andersen@kdemail.net>
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
#include "SchedulingView.h"
#include "Factory.h"
#include "SchedulingModel.h"
#include "MainDocument.h"
#include "PlanGroupDebug.h"

#include <KoApplication.h>
#include <KoComponentData.h>
#include <KoDocument.h>
#include <KoPart.h>
#include <KoIcon.h>

#include <kptitemmodelbase.h>
#include <kptproject.h>
#include <kptscheduleeditor.h>
#include <kptdatetime.h>

#include <KActionCollection>
#include <KXMLGUIFactory>
#include <KMessageBox>

#include <QTreeView>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QDir>
#include <QItemSelectionModel>
#include <QSplitter>
#include <QAction>
#include <QMenu>

SchedulingView::SchedulingView(KoPart *part, KoDocument *doc, QWidget *parent)
    : KoView(part, doc, parent)
    , m_readWrite(false)
{
    //debugPlan;
    if (doc && doc->isReadWrite()) {
        setXMLFile("SchedulingViewUi.rc");
    } else {
        setXMLFile("SchedulingViewUi_readonly.rc");
    }
    setupGui();

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    QSplitter *sp = new QSplitter(this);
    sp->setContextMenuPolicy(Qt::CustomContextMenu);
    sp->setOrientation(Qt::Vertical);
    layout->addWidget(sp);

    m_view = new QTreeView(this);
    m_view->setRootIsDecorated(false);
    m_view->setContextMenuPolicy(Qt::CustomContextMenu);
    sp->addWidget(m_view);

    SchedulingModel *model = new SchedulingModel(m_view);
    model->setPortfolio(qobject_cast<MainDocument*>(doc));
    m_view->setModel(model);
    model->setDelegates(m_view);

    // move some column after our extracolumns
    m_view->header()->moveSection(1, model->columnCount()-1); // target start
    m_view->header()->moveSection(1, model->columnCount()-1); // target finish
    m_view->header()->moveSection(1, model->columnCount()-1); // description
    connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged, this, &SchedulingView::selectionChanged);
    connect(m_view, &QTreeView::customContextMenuRequested, this, &SchedulingView::slotCustomContextMenuRequested);
    connect(m_view, &QAbstractItemView::doubleClicked, this, &SchedulingView::slotDoubleClicked);

    m_logView = new KPlato::ScheduleLogTreeView(sp);
    sp->addWidget(m_logView);
    updateActionsEnabled();
}

SchedulingView::~SchedulingView()
{
}

void SchedulingView::setupGui()
{
    QAction *a = new QAction(koIcon("refresh"), i18n("Update"), this);
    actionCollection()->addAction("load_projects", a);
    actionCollection()->setDefaultShortcut(a, Qt::Key_F5);
    connect(a, &QAction::triggered, this, &SchedulingView::loadProjects);

    a = new QAction(koIcon("view-time-schedule-calculus"), i18n("Calculate"), this);
    actionCollection()->addAction("calculate_schedule", a);
    connect(a, &QAction::triggered, this, &SchedulingView::calculate);

    a = new QAction(koIcon("view-time-schedule-calculus"), i18n("Open Project"), this);
    actionCollection()->addAction("open_project", a);
    connect(a, &QAction::triggered, this, &SchedulingView::openProject);
}

void SchedulingView::updateReadWrite(bool readwrite)
{
    m_readWrite = readwrite;
}

QPrintDialog *SchedulingView::createPrintDialog(KoPrintJob *printJob, QWidget *parent)
{
    return nullptr;
}

void SchedulingView::updateActionsEnabled()
{
    bool enable = m_view->selectionModel() && (m_view->selectionModel()->selectedRows().count() == 1);
    actionCollection()->action("load_projects")->setEnabled(enable);
    actionCollection()->action("calculate_schedule")->setEnabled(enable);
    actionCollection()->action("open_project")->setEnabled(enable);
}

void SchedulingView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    KPlato::Project *project = nullptr;
    KoDocument *doc = m_view->selectionModel()->selectedRows().value(0).data(DOCUMENT_ROLE).value<KoDocument*>();
    if (doc) {
        project = doc->project();
    }
    m_logView->setProject(project);
    if (project) {
        m_logView->setScheduleManager(project->findScheduleManagerByName(doc->property(SCHEDULEMANAGERNAME).toString()));
    }
    updateActionsEnabled();
}

void SchedulingView::slotDoubleClicked(const QModelIndex &idx)
{
    Q_UNUSED(idx)
}

void SchedulingView::slotCustomContextMenuRequested(const QPoint &pos)
{
    QMenu *menu = qobject_cast<QMenu*>(factory()->container("context_menu", this));
    if (menu && !menu->isEmpty()) {
        menu->exec(mapToGlobal(pos));
    }
}

void SchedulingView::openProject()
{
    QModelIndex idx = m_view->selectionModel()->selectedRows().value(0);
    KoDocument *doc = idx.data(DOCUMENT_ROLE).value<KoDocument*>();
    Q_EMIT openDocument(doc);
}

void SchedulingView::loadProjects()
{
    MainDocument *main = qobject_cast<MainDocument*>(koDocument());
    QList<QUrl> urls;
    for (KoDocument *document : main->documents()) {
        KPlato::Project *project = document->project();
        QUrl url = project->sharedProjectsUrl();
        if (!urls.contains(url)) {
            urls << url;
        }
    }
    const QList<QUrl> files = projectUrls(urls);
    for (const QUrl &url : files) {
        loadProject(url);
    }
}

QList<QUrl> SchedulingView::projectUrls(const QList<QUrl> &dirs)
{
    QList<QUrl> urls;
    for (const QUrl &url : dirs) {
        QFileInfo fi(url.path());
        if (!fi.exists()) {
            warnPlanGroup<<"Url does not exist:"<<url;
            KMessageBox::sorry(this, xi18nc("@info", "Cannot load projects.<nl/>The directory does not exist:<nl/>%1", url.toDisplayString()));
        } else if (fi.isFile()) {
            warnPlanGroup<<"Not supported: Get all projects in file:"<<url;
        } else if (fi.isDir()) {
            // Get all plan files in this directory
            debugPlanGroup<<"Get all projects in dir:"<<url;
            QDir dir = fi.dir();
            const QList<QString> files = dir.entryList(QStringList()<<"*.plan");
            for(const QString &f : files) {
                QString path = dir.canonicalPath();
                if (path.isEmpty()) {
                    warnPlanGroup<<"No path to file:"<<url;
                    continue;
                }
                path += '/' + f;
                QUrl u(path);
                u.setScheme("file");
                urls << u;
            }
        } else {
            warnPlanGroup<<"Unknown url:"<<url<<url.path()<<url.fileName();
        }
    }
    return urls;
}

void SchedulingView::loadProject(const QUrl &url)
{
    KoPart *part = KoApplication::koApplication()->getPartFromUrl(url);
    Q_ASSERT(part);
    if (part) {
        KoDocument *doc = part->createDocument(part);
        doc->setAutoSave(0);
        doc->setProperty(BLOCKSHAREDPROJECTSLOADING, true);
        connect(doc, &KoDocument::sigProgress, mainWindow(), &KoMainWindow::slotProgress);
        connect(doc, &KoDocument::completed, this, &SchedulingView::slotLoadCompleted);
        connect(doc, &KoDocument::canceled, this, &SchedulingView::slotLoadCanceled);
        doc->openUrl(url);
    }
}

void SchedulingView::slotLoadCompleted()
{
    KoDocument *doc = qobject_cast<KoDocument*>(sender());
    Q_ASSERT(doc);
    MainDocument *main = qobject_cast<MainDocument*>(koDocument());
    disconnect(doc, &KoDocument::sigProgress, mainWindow(), &KoMainWindow::slotProgress);
    doc->setReadWrite(false);
    if (main->addDocument(doc)) {
        // select a usable schedule
        const KPlato::ScheduleManager *manager = scheduleManager(doc);
        if (manager) {
            doc->setProperty(SCHEDULEMANAGERNAME, manager->name());
            doc->setProperty(SCHEDULINGCONTROL, "Include");
        } else {
            doc->setProperty(SCHEDULINGCONTROL, "Exclude");
        }
        debugPlanGroup<<"Document loaded:"<<doc->url();
    } else {
        debugPlanGroup<<"Document already exists:"<<doc->url();
        doc->deleteLater();
    }
}

void SchedulingView::slotLoadCanceled()
{
    KoDocument *doc = qobject_cast<KoDocument*>(sender());
    Q_ASSERT(doc);
    doc->deleteLater();
}

KPlato::ScheduleManager* SchedulingView::scheduleManager(const KoDocument *doc) const
{
    return static_cast<MainDocument*>(koDocument())->scheduleManager(doc);
}

void SchedulingView::calculate()
{
    QList<KoDocument*> toInclude;
    QMap<int, KoDocument*> toSchedule;
    const QList<KoDocument*> docs = static_cast<MainDocument*>(koDocument())->documents();
    for (KoDocument *doc : docs) {
        if (doc->property(SCHEDULINGCONTROL).toString() == "Schedule") {
            toSchedule.insert(doc->property(SCHEDULINGPRIORITY).toInt(), doc);
        } else if (doc->property(SCHEDULINGCONTROL).toString() == "Include") {
            toInclude << doc;
        }
    }
    qApp->setOverrideCursor(QCursor(Qt::WaitCursor));
    for (KoDocument *doc : toSchedule) {
        schedule(doc, toInclude);
        toInclude << doc;
    }
    qApp->restoreOverrideCursor();
    selectionChanged(QItemSelection(), QItemSelection());
}

void SchedulingView::schedule(KoDocument *doc, QList<KoDocument*> include)
{
    MainDocument *portfolio = static_cast<MainDocument*>(koDocument());
    QList<QUrl> files;
    for (KoDocument *d : include) {
        KPlato::Project *project = d->project();
        project->setProperty(SCHEDULEMANAGERNAME, d->property(SCHEDULEMANAGERNAME));
        bool ok = QMetaObject::invokeMethod(doc, "insertSharedResourceAssignments", Q_ARG(const KPlato::Project*, project));
    }
    KPlato::Project *project = doc->project();
    KPlato::ScheduleManager *sm = scheduleManager(doc);

    KPlato::DateTime oldstart = project->constraintStartTime();
    KPlato::DateTime start = targetStart();
    if (oldstart > start) {
        start = oldstart;
    }
    if (sm && sm->isScheduled() && project->isStarted()) {
        // create a subschedule
        KPlato::ScheduleManager *parent = sm;
        sm = project->createScheduleManager(parent);
        project->addScheduleManager(sm, parent);
        portfolio->setDocumentProperty(doc, SCHEDULEMANAGERNAME, sm->name());
        sm->setRecalculate(true);
        sm->setRecalculateFrom(start);
    } else {
        // create a new schedule
        sm = project->createScheduleManager();
        project->addScheduleManager(sm);
        portfolio->setDocumentProperty(doc, SCHEDULEMANAGERNAME, sm->name());
        project->setConstraintStartTime(start);
    }
    Q_ASSERT(sm);
    if (!sm->expected()) {
        sm->createSchedules();
    }
    Q_ASSERT(sm->expected());
    Q_ASSERT(!project->schedules().isEmpty());
    project->calculate(*sm);
    project->setConstraintStartTime(oldstart);
}

KPlato::DateTime SchedulingView::targetStart() const
{
    QDate date = QDate::currentDate();
    QTime time = QTime::currentTime();
    time = QTime(time.hour(), time.minute());
    return KPlato::DateTime(date, time);
}
