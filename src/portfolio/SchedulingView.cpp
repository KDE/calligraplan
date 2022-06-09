/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "SchedulingView.h"
#include "SchedulingModel.h"
#include "MainDocument.h"
#include "PlanGroupDebug.h"

#include <KoApplication.h>
#include <KoComponentData.h>
#include <KoDocument.h>
#include <KoPart.h>
#include <KoIcon.h>

#include <kptproject.h>
#include <kptitemmodelbase.h>
#include <kptproject.h>
#include <kptscheduleeditor.h>
#include <kptdatetime.h>
#include <kptschedulerplugin.h>
#include <kptcommand.h>
#include <kpttaskdescriptiondialog.h>
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
        setXMLFile(QStringLiteral("Portfolio_SchedulingViewUi.rc"));
    } else {
        setXMLFile(QStringLiteral("Portfolio_SchedulingViewUi_readonly.rc"));
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
    connect(ui.schedulingView, &QTreeView::customContextMenuRequested, this, &SchedulingView::slotContextMenuRequested);
    connect(ui.schedulingView, &QAbstractItemView::doubleClicked, this, &SchedulingView::itemDoubleClicked);

    m_logView = new QTreeView(sp);
    m_logView->setContextMenuPolicy(Qt::ActionsContextMenu);
    m_logView->header()->setContextMenuPolicy(Qt::ActionsContextMenu);
    auto a = new QAction(QStringLiteral("Debug"), m_logView);
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
    connect(model, &QAbstractItemModel::rowsInserted, this, &SchedulingView::updateSchedulingProperties);
    connect(model, &QAbstractItemModel::rowsRemoved, this, &SchedulingView::updateSchedulingProperties);
    connect(model, &QAbstractItemModel::modelReset, this, &SchedulingView::updateSchedulingProperties);
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
            ui.granularities->addItem(QStringLiteral("%1 min").arg(v/(60*1000)), (qint64)v);
        }
        ui.granularities->setCurrentIndex(scheduler->granularityIndex());
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
        scheduler->setGranularityIndex(idx);
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
    auto a  = new QAction(koIcon("document-edit"), i18n("Description..."), this);
    actionCollection()->addAction(QStringLiteral("project_description"), a);
    connect(a, &QAction::triggered, this, &SchedulingView::slotDescription);

}

void SchedulingView::itemDoubleClicked(const QPersistentModelIndex &idx)
{
    debugPortfolio<<idx;
    if (idx.column() == 3 /*Description*/) {
        slotDescription();
    }
}

void SchedulingView::slotContextMenuRequested(const QPoint &pos)
{
    debugPortfolio<<"Context menu"<<pos;
    if (!factory()) {
        debugPortfolio<<"No factory";
        return;
    }
    if (!ui.schedulingView->indexAt(pos).isValid()) {
        debugPortfolio<<"Nothing selected";
        return;
    }
    auto menu = static_cast<QMenu*>(factory()->container(QStringLiteral("schedulingview_popup"), this));
    Q_ASSERT(menu);
    if (menu->isEmpty()) {
        debugPortfolio<<"Menu is empty";
        return;
    }
    menu->exec(ui.schedulingView->viewport()->mapToGlobal(pos));
}

void SchedulingView::slotDescription()
{
    auto idx = ui.schedulingView->selectionModel()->currentIndex();
    if (!idx.isValid()) {
        debugPortfolio<<"No current project";
        return;
    }
    auto doc = ui.schedulingView->model()->data(idx, DOCUMENT_ROLE).value<KoDocument*>();
    auto project = doc->project();
    KPlato::TaskDescriptionDialog dia(*project, this, m_readWrite);
    if (dia.exec() == QDialog::Accepted) {
        auto m = dia.buildCommand();
        if (m) {
            doc->addCommand(m);
        }
    }
}

void SchedulingView::updateReadWrite(bool readwrite)
{
    m_readWrite = readwrite;
}

KoPrintJob *SchedulingView::createPrintJob()
{
    return nullptr;
}

void SchedulingView::updateActionsEnabled()
{
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
    if (m_schedulingContext.projects.key(doc, -1) == -1) {
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
    QVariantList managerNames;
    {const auto docs = portfolio->documents();
    for (auto doc : docs) {
        managerNames << doc->property(SCHEDULEMANAGERNAME);
    }}
    m_schedulingContext.clear();
    const auto key = schedulerKey();
    auto scheduler = portfolio->schedulerPlugin(key);
    if (scheduler) {
        calculateSchedule(scheduler);
        selectionChanged(QItemSelection(), QItemSelection());
    } else {
        warnPortfolio<<Q_FUNC_INFO<<"No scheduler plugin"<<key;
    }
    {const auto docs = portfolio->documents();
    Q_ASSERT(managerNames.count() == docs.count());
    for (int i = 1; i < docs.count(); ++i) {
        if (docs.at(i)->property(SCHEDULEMANAGERNAME) != managerNames.value(i)) {
            portfolio->setModified(true);
            break;
        }
    }}
}

void SchedulingView::calculateSchedule(KPlato::SchedulerPlugin *scheduler)
{
    auto portfolio = static_cast<MainDocument*>(koDocument());
    auto docs = portfolio->documents();
    if (docs.isEmpty()) {
        warnPortfolio<<Q_FUNC_INFO<<"Nothing to shcedule";
        return;
    }
    // Populate scheduling context
    m_schedulingContext.project = new KPlato::Project();
    m_schedulingContext.project->setName(QStringLiteral("Project Collection"));
    m_schedulingContext.calculateFrom = calculationTime();
    for (KoDocument *doc : qAsConst(docs)) {
        int prio = doc->property(SCHEDULINGPRIORITY).isValid() ? doc->property(SCHEDULINGPRIORITY).toInt() : -1;
        if (doc->property(SCHEDULINGCONTROL).toString() == QStringLiteral("Schedule")) {
            m_schedulingContext.addProject(doc, prio);
        } else if (doc->property(SCHEDULINGCONTROL).toString() == QStringLiteral("Include")) {
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
