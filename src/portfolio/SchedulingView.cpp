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
#include "ScheduleManagerInfoDialog.h"

#include <KoApplication.h>
#include <KoComponentData.h>
#include <KoDocument.h>
#include <KoPart.h>
#include <KoIcon.h>
#include <ExtraProperties.h>

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
#include <QProgressDialog>

SchedulingView::SchedulingView(KoPart *part, KoDocument *doc, QWidget *parent)
    : KoView(part, doc, parent)
    , m_readWrite(false)
{
    //debugPlan;
    setupGui();

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
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
    model->setCalculateFrom(ui.calculationDateTime->dateTime());

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
    ui.todayRB->setChecked(true);
    slotTodayToggled(true);
    QTimer::singleShot(0, this, &SchedulingView::calculateFromChanged); // update model

    connect(model, &QAbstractItemModel::dataChanged, this, &SchedulingView::updateActionsEnabled);
    connect(model, &QAbstractItemModel::rowsInserted, this, &SchedulingView::updateSchedulingProperties);
    connect(model, &QAbstractItemModel::rowsRemoved, this, &SchedulingView::updateSchedulingProperties);
    //connect(model, &QAbstractItemModel::modelReset, this, &SchedulingView::updateSchedulingProperties);
    connect(ui.schedulersCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(slotSchedulersComboChanged(int)));
    connect(ui.granularities, SIGNAL(currentIndexChanged(int)), this, SLOT(slotGranularitiesChanged(int)));
    connect(ui.sequential, &QRadioButton::toggled, this, &SchedulingView::slotSequentialChanged);
    connect(ui.todayRB, &QRadioButton::toggled, this, &SchedulingView::slotTodayToggled);
    connect(ui.tomorrowRB, &QRadioButton::toggled, this, &SchedulingView::slotTomorrowToggled);
    connect(ui.timeRB, &QRadioButton::toggled, this, &SchedulingView::slotTimeToggled);
    connect(ui.calculate, &QPushButton::clicked, this, &SchedulingView::calculate);
    connect(ui.calculationDateTime, &QDateTimeEdit::dateTimeChanged, this, &SchedulingView::calculateFromChanged);
    connect(ui.timeRB, &QRadioButton::toggled, ui.calculationDateTime, &QDateTimeEdit::setEnabled);

    connect(static_cast<MainDocument*>(doc), &MainDocument::documentInserted, this, &SchedulingView::portfolioChanged);
    connect(static_cast<MainDocument*>(doc), &MainDocument::documentRemoved, this, &SchedulingView::portfolioChanged);

    setWhatsThis(xi18nc("@info:whatsthis",
        "<title>Scheduling</title>"
        "<para>"
        "The Scheduling view lets you schedule the projects in your portfolio. The projects are scheduled according to their priority."
        "</para><para>"
        "Set <emphasis>Control</emphasis> to <emphasis>Schedule</emphasis> for the project or projects you want to schedule."
        "<nl/>Set <emphasis>Control</emphasis> to <emphasis>Include</emphasis> to have resource assignments included in resource leveling."
        "<nl/>Set <emphasis>Control</emphasis> to <emphasis>Exclude</emphasis> if you want to exclude the project."
        "</para><para>"
        "<note>If your projects share resources with projects that are not part of your portfolio, these projects needs to be included for resource leveling to work properly.</note>"
        "</para><para>"
        "<link url='%1'>More...</link>"
        "</para>", QStringLiteral("portfolio:scheduling")));
}

SchedulingView::~SchedulingView()
{
}

void SchedulingView::portfolioChanged()
{
    ui.schedulingProperties->setDisabled(static_cast<MainDocument*>(koDocument())->documents().isEmpty());
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
            ui.granularities->addItem(i18ncp("@label:listbox range: 0-60", "%1 minute", "%1 minutes", v/(60*1000)));
        }
        ui.granularities->setCurrentIndex(scheduler->granularityIndex());
        ui.sequential->setChecked(!scheduler->scheduleInParallel());
        ui.sequential->setEnabled(true);
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
    setXMLFile(QStringLiteral("Portfolio_SchedulingViewUi.rc"));

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
    const auto portfolio = static_cast<MainDocument*>(koDocument());
    const auto docs = portfolio->documents();
    const auto calculateFrom = ui.calculationDateTime->dateTime();
    bool enable = false;
    for (auto doc : docs) {
        if (doc->property(SCHEDULINGCONTROL).toString() == QStringLiteral("Schedule")) {
            if (calculateFrom > doc->project()->constraintEndTime()) {
                ui.calculate->setEnabled(false);
                return;
            }
            enable = true;
        }
    }
    ui.calculate->setEnabled(enable);
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
    if (m_schedulingContext.project || m_schedulingContext.projects.key(doc, -1) == -1) {
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

void SchedulingView::calculateFromChanged()
{
    auto model = static_cast<SchedulingModel*>(ui.schedulingView->model());
    model->setCalculateFrom(ui.calculationDateTime->dateTime());
    updateActionsEnabled();
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
    m_logModel.setLog(m_schedulingContext.log);
    const auto key = schedulerKey();
    if (calculateSchedule(portfolio->schedulerPlugin(key))) {
        selectionChanged(QItemSelection(), QItemSelection());
        if (!m_schedulingContext.cancelScheduling && !m_schedulingContext.projects.isEmpty()) {
            portfolio->setModified(true);
        }
    }
}

bool SchedulingView::calculateSchedule(KPlato::SchedulerPlugin *scheduler)
{
    auto portfolio = static_cast<MainDocument*>(koDocument());
    auto docs = portfolio->documents();

    // Populate scheduling context
    m_schedulingContext.scheduler = scheduler;
    m_schedulingContext.project = new KPlato::Project();
    m_schedulingContext.project->setName(QStringLiteral("Project Collection"));
    m_schedulingContext.calculateFrom = calculationTime();
    m_schedulingContext.log.clear();
    m_logModel.setLog(m_schedulingContext.log);
    if (!scheduler) {
        warnPortfolio<<"No scheduler plugin"<<schedulerKey();
        KPlato::Schedule::Log log(m_schedulingContext.project, KPlato::Schedule::Log::Type_Error, i18n("Internal error. No scheduler plugin found."));
        m_logModel.setLog(QVector<KPlato::Schedule::Log>() << log);
        m_logView->resizeColumnToContents(0);
        return false;
    }
    if (docs.isEmpty()) {
        warnPortfolio<<"Nothing to schedule";
        KPlato::Schedule::Log log(m_schedulingContext.project, KPlato::Schedule::Log::Type_Warning, i18n("Nothing to schedule"));
        m_logModel.setLog(QVector<KPlato::Schedule::Log>() << log);
        m_logView->resizeColumnToContents(0);
        return false;
    }
    for (KoDocument *doc : qAsConst(docs)) {
        int prio = doc->property(SCHEDULINGPRIORITY).isValid() ? doc->property(SCHEDULINGPRIORITY).toInt() : -1;
        if (doc->property(SCHEDULINGCONTROL).toString() == QStringLiteral("Schedule")) {
            m_schedulingContext.addProject(doc, prio);
        } else if (doc->property(SCHEDULINGCONTROL).toString() == QStringLiteral("Include")) {
            m_schedulingContext.addResourceBookings(doc);
        }
    }
    if (m_schedulingContext.projects.isEmpty()) {
        warnPortfolio<<"Nothing to schedule";
        KPlato::Schedule::Log log(m_schedulingContext.project, KPlato::Schedule::Log::Type_Warning, i18n("Nothing to schedule"));
        m_logModel.setLog(QVector<KPlato::Schedule::Log>() << log);
        if (QApplication::overrideCursor()) {
            QApplication::restoreOverrideCursor();
        }
        return false;
    }
    ScheduleManagerInfoDialog dlg(m_schedulingContext.projects.values());
    if (dlg.exec() != QDialog::Accepted) {
        return false;
    }
    for (const auto &info : dlg.scheduleManagerInfoList()) {
        if (!info.manager) {
            info.project->setProperty(SCHEDULEMANAGERNAME, info.document->property(SCHEDULEMANAGERNAME));
            continue;
        }
        info.project->addScheduleManager(info.manager, info.parentManager);
        info.project->setProperty(SCHEDULEMANAGERNAME, info.manager->name());
    }
    QApplication::setOverrideCursor(Qt::WaitCursor); // FIXME: workaround because progress dialog shown late, why?

    KPlato::DateTime targetEnd;
    QMultiMap<int, KoDocument*>::const_iterator it = m_schedulingContext.projects.constBegin();
    for (; it != m_schedulingContext.projects.constEnd(); ++it) {
        const auto p = it.value()->project();
        const auto end = p->constraintEndTime();
        if (end < m_schedulingContext.calculateFrom) {
            KPlato::Schedule::Log log(p, KPlato::Schedule::Log::Type_Error, i18n("Scheduling not possible. Project target end time must be later than calculation time."));
            m_logModel.setLog(QVector<KPlato::Schedule::Log>() << log);
            if (QApplication::overrideCursor()) {
                QApplication::restoreOverrideCursor();
            }
            return false;
        }
        targetEnd = std::max(targetEnd, end);
    }
    m_schedulingContext.project->setConstraintEndTime(targetEnd);

    m_progress = new QProgressDialog(this);
    m_progress->setLabelText(i18n("Scheduling projects"));
    m_progress->setWindowModality(Qt::WindowModal);
    m_progress->setMinimumDuration(0);
    connect(scheduler, &KPlato::SchedulerPlugin::progressChanged, this, [this](int value, KPlato::ScheduleManager*) {
        if (QApplication::overrideCursor()) {
            QApplication::restoreOverrideCursor();
        }
        if (!m_progress->wasCanceled()) {
            m_progress->setValue(value);
        }
    });
    connect(m_progress, &QProgressDialog::canceled, this, [this]() {
        m_schedulingContext.scheduler->cancelScheduling(m_schedulingContext);
    });
    scheduler->schedule(m_schedulingContext);
    m_logModel.setLog(m_schedulingContext.log);

    for (auto it = m_schedulingContext.projects.constBegin(); it != m_schedulingContext.projects.constEnd(); ++it) {
        portfolio->emitDocumentChanged(it.value());
        Q_EMIT projectCalculated(it.value()->project(), it.value()->project()->findScheduleManagerByName(it.value()->property(SCHEDULEMANAGERNAME).toString()));
    }
    m_progress->deleteLater();
    if (QApplication::overrideCursor()) {
        QApplication::restoreOverrideCursor();
    }
    return true;
}

void SchedulingView::saveSettings(QDomElement &settings) const
{

    settings.setAttribute(QStringLiteral("scheduler"), ui.schedulersCombo->currentIndex());

    settings.setAttribute(QStringLiteral("calculation-method"), ui.parallel->isChecked() ? QStringLiteral("parallel") : QStringLiteral("sequensial"));
    settings.setAttribute(QStringLiteral("granularity"), ui.granularities->currentIndex());
    QString s;
    if (ui.timeRB->isChecked()) {
        s = QStringLiteral("time");
        settings.setAttribute(QStringLiteral("calculate-from"), ui.calculationDateTime->dateTime().toString(Qt::ISODate));
    } else if (ui.todayRB->isChecked()) {
        s = QStringLiteral("today");
    } else if (ui.tomorrowRB->isChecked()) {
        s = QStringLiteral("tomorrow");
    }
    settings.setAttribute(QStringLiteral("time-option"), s);

}

void SchedulingView::loadSettings(KoXmlElement &settings)
{
    ui.schedulersCombo->setCurrentIndex(settings.attribute(QStringLiteral("scheduler")).toInt());
    auto s = settings.attribute(QStringLiteral("calculation-method"));
    if (s == QStringLiteral("parallel")) {
        ui.parallel->setChecked(true);
    }
    ui.granularities->setCurrentIndex(settings.attribute(QStringLiteral("granularity")).toInt());
    s = settings.attribute(QStringLiteral("time-option"));
    if (s == QStringLiteral("time")) {
        ui.timeRB->setChecked(true);
        ui.calculationDateTime->setDateTime(QDateTime::fromString(settings.attribute(QStringLiteral("calculate-from")), Qt::ISODate));
    } else if (s == QStringLiteral("today")) {
        ui.todayRB->setChecked(true);
    } else if (s == QStringLiteral("tomorrow")) {
        ui.tomorrowRB->setChecked(true);
    }
    ui.calculationDateTime->setEnabled(ui.timeRB->isChecked());
}
