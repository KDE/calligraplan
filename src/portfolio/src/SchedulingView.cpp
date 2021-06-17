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
#include <QComboBox>

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
    m_logView->setContextMenuPolicy(Qt::ActionsContextMenu);
    m_logView->header()->setContextMenuPolicy(Qt::ActionsContextMenu);
    auto a = new QAction(QString("Debug"), m_logView);
    a->setCheckable(true);
    m_logView->insertAction(nullptr, a);
    m_logView->header()->insertAction(nullptr, a);
    connect(a, &QAction::toggled, this, &SchedulingView::updateLogFilter);
    m_logView->setRootIsDecorated(false);
    sp->addWidget(m_logView);
    SchedulingLogFilterModel *fm = new SchedulingLogFilterModel(m_logView);
    fm->setSeveritiesDenied(QList<int>() << KPlato::Schedule::Log::Type_Debug);
    fm->setSourceModel(&m_logModel);
    m_logView->setModel(fm);
    updateActionsEnabled();

    updateSchedulingProperties();

    connect(model, &QAbstractItemModel::dataChanged, this, &SchedulingView::updateActionsEnabled);
    connect(ui.schedulersCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(slotSchedulersComboChanged(int)));
    connect(ui.granularities, SIGNAL(currentIndexChanged(int)), this, SLOT(slotGranularitiesChanged(int)));
    connect(ui.sequential, &QRadioButton::toggled, this, &SchedulingView::slotSequentialChanged);
    connect(ui.todayRB, &QRadioButton::toggled, this, &SchedulingView::slotTodayToggled);
    connect(ui.tomorrowRB, &QRadioButton::toggled, this, &SchedulingView::slotTomorrowToggled);
    connect(ui.timeRB, &QRadioButton::toggled, this, &SchedulingView::slotTimeToggled);
    connect(ui.calculate, &QPushButton::clicked, this, &SchedulingView::calculate);
}

SchedulingView::~SchedulingView()
{
}

void SchedulingView::updateLogFilter()
{
    auto a = qobject_cast<QAction*>(sender());
    if (!a) {
        return;
    }
    auto fm = qobject_cast<SchedulingLogFilterModel*>(m_logView->model());
    QList<int> filter;
    if (!a->isChecked()) {
        filter << KPlato::Schedule::Log::Type_Debug;
    }
    fm->setSeveritiesDenied(filter);
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
        ui.schedulersCombo->addItem(it.value()->name(), it.key());
    }
    slotSchedulersComboChanged(ui.schedulersCombo->currentIndex());
    ui.todayRB->setChecked(true);
    slotTodayToggled(true);
}

void SchedulingView::slotSchedulersComboChanged(int idx)
{
    ui.granularities->blockSignals(true);
    ui.granularities->clear();
    MainDocument *portfolio = static_cast<MainDocument*>(koDocument());
    const auto scheduler = portfolio->schedulerPlugin(ui.schedulersCombo->itemData(idx).toString());
    if (scheduler) {
        const auto lst = scheduler->granularities();
        for (auto v : lst) {
            ui.granularities->addItem(QString("%1 min").arg(v/(60*1000)), (qint64)v);
        }
        ui.granularities->setCurrentIndex(scheduler->granularity());
        ui.sequential->setChecked(!scheduler->scheduleInParallel());
        ui.parallel->setChecked(scheduler->scheduleInParallel());
        ui.parallel->setEnabled(scheduler->capabilities() & KPlato::SchedulerPlugin::ScheduleInParallel);
        ui.schedulersCombo->setWhatsThis(scheduler->comment());
    } else {
        ui.sequential->setEnabled(false);
        ui.parallel->setEnabled(false);
        ui.schedulersCombo->setWhatsThis(QString());
    }
    ui.granularities->blockSignals(false);
}

void SchedulingView::slotGranularitiesChanged(int idx)
{
    MainDocument *portfolio = static_cast<MainDocument*>(koDocument());
    const auto scheduler = portfolio->schedulerPlugin(ui.schedulersCombo->currentData().toString());
    if (scheduler) {
        scheduler->setGranularity(idx);
    }
}

void SchedulingView::slotSequentialChanged(bool state)
{
    MainDocument *portfolio = static_cast<MainDocument*>(koDocument());
    const auto scheduler = portfolio->schedulerPlugin(ui.schedulersCombo->currentData().toString());
    if (scheduler) {
        scheduler->setScheduleInParallel(!state);
    }
}

void SchedulingView::slotTodayToggled(bool state)
{
    if (state) {
        ui.calculationDateTime->setDateTime(QDateTime(QDate::currentDate(), QTime()));
    }
}

void SchedulingView::slotTomorrowToggled(bool state)
{
    if (state) {
        ui.calculationDateTime->setDateTime(QDateTime(QDate::currentDate().addDays(1), QTime()));
    }
}

void SchedulingView::slotTimeToggled(bool state)
{
    ui.calculationDateTime->setEnabled(state);
}

void SchedulingView::setupGui()
{
    QAction *a = new QAction(koIcon("refresh"), i18n("Update"), this);
    actionCollection()->addAction("load_projects", a);
    actionCollection()->setDefaultShortcut(a, Qt::Key_F5);
    connect(a, &QAction::triggered, this, &SchedulingView::loadProjects);
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
    qInfo()<<Q_FUNC_INFO;
    bool enable = ui.schedulingView->selectionModel() && (ui.schedulingView->selectionModel()->selectedRows().count() == 1);
    actionCollection()->action("load_projects")->setEnabled(enable);

    ui.calculate->setEnabled(false);
    const auto portfolio = static_cast<MainDocument*>(koDocument());
    const auto docs = portfolio->documents();
    for (auto doc : docs) {
        if (doc->property(SCHEDULINGCONTROL).toString() == QStringLiteral("Schedule")) {
            ui.calculate->setEnabled(true);
            break;
        }
    }
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
    if (m_schedulingContext.projects.values().contains(doc)) {
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

QString SchedulingView::schedulerKey() const
{
    return ui.schedulersCombo->currentData().toString();
}

QDateTime SchedulingView::calculationTime() const
{
    return ui.calculationDateTime->dateTime();
}

void SchedulingView::calculate()
{
    MainDocument *portfolio = static_cast<MainDocument*>(koDocument());
    m_schedulingContext.clear();
    const auto key = schedulerKey();
    auto scheduler = portfolio->schedulerPlugin(key);
    if (scheduler) {
        calculateSchedule(scheduler);
        selectionChanged(QItemSelection(), QItemSelection());
    } else {
        warnPlanGroup<<Q_FUNC_INFO<<"No scheduler plugin"<<key;
    }
}

void SchedulingView::calculateSchedule(KPlato::SchedulerPlugin *scheduler)
{
    auto portfolio = static_cast<MainDocument*>(koDocument());
    auto docs = portfolio->documents();
    if (docs.isEmpty()) {
        warnPlanGroup<<Q_FUNC_INFO<<"Nothing to shcedule";
        return;
    }
    // Populate scheduling context
    m_schedulingContext.project = new KPlato::Project();
    m_schedulingContext.project->setName("Project Collection");
    m_schedulingContext.calculateFrom = calculationTime();
    for (KoDocument *doc : docs) {
        int prio = doc->property(SCHEDULINGPRIORITY).isValid() ? doc->property(SCHEDULINGPRIORITY).toInt() : -1;
        if (doc->property(SCHEDULINGCONTROL).toString() == "Schedule") {
            m_schedulingContext.addProject(doc, prio);
        } else if (doc->property(SCHEDULINGCONTROL).toString() == "Include") {
            m_schedulingContext.addResourceBookings(doc);
        }
    }
    QApplication::setOverrideCursor(Qt::WaitCursor);
    scheduler->schedule(m_schedulingContext);
    m_logModel.setLog(m_schedulingContext.log);
    for (QMap<int, KoDocument*>::const_iterator it = m_schedulingContext.projects.constBegin(); it != m_schedulingContext.projects.constEnd(); ++it) {
        portfolio->emitDocumentChanged(it.value());
    }
    QApplication::restoreOverrideCursor();
}
