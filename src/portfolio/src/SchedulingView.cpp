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
#include <kptschedulerplugin.h>
#include <kptcommand.h>

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

    QWidget *w = new QWidget(this);
    ui.setupUi(w);
    sp->addWidget(w);

    SchedulingModel *model = new SchedulingModel(ui.schedulingView);
    model->setPortfolio(qobject_cast<MainDocument*>(doc));
    ui.schedulingView->setModel(model);
    model->setDelegates(ui.schedulingView);

    // move some column after our extracolumns
    ui.schedulingView->header()->moveSection(1, model->columnCount()-1); // target start
    ui.schedulingView->header()->moveSection(1, model->columnCount()-1); // target finish
    ui.schedulingView->header()->moveSection(1, model->columnCount()-1); // description
    connect(ui.schedulingView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &SchedulingView::selectionChanged);
    connect(ui.schedulingView, &QTreeView::customContextMenuRequested, this, &SchedulingView::slotCustomContextMenuRequested);
    connect(ui.schedulingView, &QAbstractItemView::doubleClicked, this, &SchedulingView::slotDoubleClicked);

    m_logView = new QTreeView(sp);
    m_logView->setRootIsDecorated(false);
    sp->addWidget(m_logView);
    SchedulingLogFilterModel *fm = new SchedulingLogFilterModel(m_logView);
    fm->setSeveritiesDenied(QList<int>()<<KPlato::Schedule::Log::Type_Debug);
    fm->setSourceModel(&m_logModel);
    m_logView->setModel(fm);
    updateActionsEnabled();

    updateSchedulingProperties();
    connect(static_cast<MainDocument*>(doc), &MainDocument::changed, this, &SchedulingView::updateSchedulingProperties);
}

SchedulingView::~SchedulingView()
{
}

void SchedulingView::updateSchedulingProperties()
{
    MainDocument *portfolio = static_cast<MainDocument*>(koDocument());
    if (portfolio->documents().isEmpty()) {
        ui.schedulingProperties->setEnabled(false);
        return;
    }
    ui.schedulingProperties->setEnabled(true);
    ui.schedulersCombo->clear();
    const QMap<QString, KPlato::SchedulerPlugin*> plugins = portfolio->schedulerPlugins();
    QMap<QString, KPlato::SchedulerPlugin*>::const_iterator it;
    for (it = plugins.constBegin(); it != plugins.constEnd(); ++it) {
        ui.schedulersCombo->addItem(it.key());
    }
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
    bool enable = ui.schedulingView->selectionModel() && (ui.schedulingView->selectionModel()->selectedRows().count() == 1);
    actionCollection()->action("load_projects")->setEnabled(enable);
    actionCollection()->action("calculate_schedule")->setEnabled(enable);
}

void SchedulingView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(selected)
    Q_UNUSED(deselected)

    KPlato::Project *project = nullptr;
    KoDocument *doc = ui.schedulingView->selectionModel()->selectedRows().value(0).data(DOCUMENT_ROLE).value<KoDocument*>();
    if (doc) {
        project = doc->project();
    }
    SchedulingLogModel *m = &m_logModel;
    if (m_schedulingContext.projects.values().contains(project)) {
        m_logModel.setLog(m_schedulingContext.log);
    } else if (project) {
        KPlato::ScheduleManager *sm = project->findScheduleManagerByName(doc->property(SCHEDULEMANAGERNAME).toString());
        if (sm) {
            m_logModel.setLog(sm->expected()->logs());
        } else {
            m_logModel.setLog(QVector<KPlato::Schedule::Log>());
        }
    } else {
        m_logModel.setLog(QVector<KPlato::Schedule::Log>());
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

void SchedulingView::loadProjects()
{
    MainDocument *main = qobject_cast<MainDocument*>(koDocument());
    QList<QUrl> urls;
    const auto documents = main->documents();
    for (KoDocument *document : documents) {
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

QString SchedulingView::schedulerName() const
{
    return ui.schedulersCombo->currentText();
}

void SchedulingView::calculate()
{
    MainDocument *portfolio = static_cast<MainDocument*>(koDocument());
    delete m_schedulingContext.scheduleManager();
    m_schedulingContext.clear();
    const QList<KoDocument*> docs = portfolio->documents();
    if (docs.isEmpty()) {
        return;
    }

    KPlato::SchedulerPlugin *scheduler = docs.first()->schedulerPlugins().value(schedulerName());
    if (scheduler) {
        calculateSchedule(scheduler, docs);
    } else {
        calculatePert(m_schedulingContext);
    }
    selectionChanged(QItemSelection(), QItemSelection());
}

void SchedulingView::calculateSchedule(KPlato::SchedulerPlugin *scheduler, const QList<KoDocument*> docs)
{
    if (! scheduler || docs.isEmpty()) {
        return;
    }
    MainDocument *portfolio = static_cast<MainDocument*>(koDocument());
    QList<KoDocument*> scheduledDocs;
    m_schedulingContext.project = new KPlato::Project();
    m_schedulingContext.project->setName("Project Collection");
    for (KoDocument *doc : docs) {
        int prio = doc->property(SCHEDULINGPRIORITY).isValid() ? doc->property(SCHEDULINGPRIORITY).toInt() : -1;
        KPlato::Project *project = doc->project();
        KPlato::ScheduleManager *sm = project->findScheduleManagerByName(doc->property(SCHEDULEMANAGERNAME).toString());
        if (doc->property(SCHEDULINGCONTROL).toString() == "Schedule") {
            scheduledDocs << doc;
            if (!sm) {
                sm = new KPlato::ScheduleManager(*project, project->uniqueScheduleName());
                KPlato::AddScheduleManagerCmd cmd(*project, sm);
                cmd.redo();
                portfolio->setDocumentProperty(doc, SCHEDULEMANAGERNAME, sm->name());
            } else if (sm->isBaselined() || (sm->isScheduled() && project->isStarted())) {
                auto parentManager = sm;
                sm = project->createScheduleManager(sm);
                KPlato::AddScheduleManagerCmd cmd(parentManager, sm);
                cmd.redo();
                portfolio->setDocumentProperty(doc, SCHEDULEMANAGERNAME, sm->name());
            }
            project->setCurrentScheduleManager(sm);
            m_schedulingContext.addProject(doc->project(), prio);
        } else if (doc->property(SCHEDULINGCONTROL).toString() == "Include") {
            project->setCurrentScheduleManager(sm);
            m_schedulingContext.addResourceBookings(project);
        }
    }
    QApplication::setOverrideCursor(Qt::WaitCursor);
    connect(scheduler, &KPlato::SchedulerPlugin::calculateSchedule, this, &SchedulingView::calculatePert);
    scheduler->schedule(m_schedulingContext);
    disconnect(scheduler, &KPlato::SchedulerPlugin::calculateSchedule, this, &SchedulingView::calculatePert);
    m_logModel.setLog(m_schedulingContext.log);
    for (KoDocument *doc : qAsConst(scheduledDocs)) {
        portfolio->emitDocumentChanged(doc);
    }
    QApplication::restoreOverrideCursor();
}

void SchedulingView::calculatePert(KPlato::SchedulingContext &context)
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
    context.clear();
}

void SchedulingView::schedule(KoDocument *doc, QList<KoDocument*> include)
{
    MainDocument *portfolio = static_cast<MainDocument*>(koDocument());
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
    if (!sm) {
        // create a new schedule
        sm = project->createScheduleManager();
        project->addScheduleManager(sm);
        portfolio->setDocumentProperty(doc, SCHEDULEMANAGERNAME, sm->name());
        project->setConstraintStartTime(start);
    } else if (sm->isBaselined() || (sm->isScheduled() && project->isStarted())) {
        // create a subschedule
        KPlato::ScheduleManager *parent = sm;
        sm = project->createScheduleManager(parent);
        project->addScheduleManager(sm, parent);
        portfolio->setDocumentProperty(doc, SCHEDULEMANAGERNAME, sm->name());
        sm->setRecalculate(true);
        sm->setRecalculateFrom(start);
    } if (sm->parentManager()) {
        sm->setRecalculate(true);
        sm->setRecalculateFrom(start);
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
