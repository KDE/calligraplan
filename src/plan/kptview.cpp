/* This file is part of the KDE project
  SPDX-FileCopyrightText: 1998, 1999, 2000 Torben Weis <weis@kde.org>
  SPDX-FileCopyrightText: 2002-2011 Dag Andersen <dag.andersen@kdemail.net>
  SPDX-FileCopyrightText: 2012 Dag Andersen <dag.andersen@kdemail.net>
  SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
  
  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptview.h"
#include "ui_CreateReportTemplateDialog.h"

#include <KMessageBox>
#include <KRecentFilesAction>

#include "KoDocumentInfo.h"
#include "KoMainWindow.h"
#include <KoIcon.h>
#include <KoResourcePaths.h>
#include <KoFileDialog.h>

#include <QAction>
#include <QApplication>
#include <QLabel>
#include <QString>
#include <QStringList>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QPrintDialog>
#include <QDomDocument>
#include <QDomElement>
#include <kundo2command.h>
#include <QTimer>
#include <QDockWidget>
#include <QMenu>
#include <QTemporaryFile>
#include <QFileDialog>
#include <QStatusBar>

#include <KActionCollection>
#include <KActionMenu>
#include <KStandardAction>
#include <KToolBar>
#include <KXMLGUIFactory>
#include <KToggleAction>

#include <KEMailClientLauncherJob>
#include <KDialogJobUiDelegate>
#include <KIO/OpenUrlJob>
#include <KIO/JobUiDelegate>
#include <KIO/JobUiDelegateFactory>

#include <KoPart.h>
#include <KoComponentData.h>
#include <KoPluginLoader.h>

#include "kptlocale.h"
#include "kptviewbase.h"
#include "kptaccountsview.h"
#include "kptaccountseditor.h"
#include "kptcalendareditor.h"
#include "kptfactory.h"
#include "kptmilestoneprogressdialog.h"
#include "kpttaskdescriptiondialog.h"
#include "kptdocumentsdialog.h"
#include "kptnode.h"
#include "kptmaindocument.h"
#include "kptproject.h"
#include "kptmainprojectdialog.h"
#include "kpttask.h"
#include "kptsummarytaskdialog.h"
#include "kpttaskdialog.h"
#include "kpttaskprogressdialog.h"
#include "kptganttview.h"
#include <MilestoneGanttView.h>
#include <ResourceAppointmentsGanttView.h>
#include "kpttaskeditor.h"
#include "kptdependencyeditor.h"
#include "kptperteditor.h"
#include "kptdatetime.h"
#include "kptcommand.h"
#include <RemoveResourceCmd.h>
#include "kptrelation.h"
#include "kptrelationdialog.h"
#include "kptresourceappointmentsview.h"
#include "ResourceGroupEditor.h"
#include "kptresourceeditor.h"
#include "kptscheduleeditor.h"
#include "kptresourcedialog.h"
#include "kptresource.h"
#include "kptstandardworktimedialog.h"
#include "kptwbsdefinitiondialog.h"
#include "kpttaskstatusview.h"
#include "kptsplitterview.h"
#include "kptpertresult.h"
#include "kptinsertfiledlg.h"
#include "kptlocaleconfigmoneydialog.h"
#include "kptflatproxymodel.h"
#include "kpttaskstatusmodel.h"
#include "kptworkpackagemergedialog.h"
#include "ResourceCoverageView.h"

#include "performance/PerformanceStatusView.h"
#include "performance/ProjectStatusView.h"

#include "reportsgenerator/ReportsGeneratorView.h"

#ifdef PLAN_USE_KREPORT
#include "reports/reportview.h"
#include "reports/reportdata.h"
#endif

#include "kptviewlistdialog.h"
#include "kptviewlistdocker.h"
#include "kptviewlist.h"
#include "kptschedulesdocker.h"
#include "kptpart.h"
#include "kptdebug.h"

#include "calligraplansettings.h"
#include "kptprintingcontrolprivate.h"

// #include "KPtViewAdaptor.h"

#include <assert.h>

using namespace KPlato;

View::View(KoPart *part, MainDocument *doc, QWidget *parent)
        : KoView(part, doc, true, parent),
        m_currentEstimateType(Estimate::Use_Expected),
        m_scheduleActionGroup(new QActionGroup(this)),
        m_readWrite(false),
        m_defaultView(1),
        m_partpart (part)
{
    //debugPlan;

    doc->registerView(this);

    setComponentName(Factory::global().componentName(), Factory::global().componentDisplayName());
    setXMLFile(QStringLiteral("calligraplan.rc"));

//     new ViewAdaptor(this);

    m_sp = new QSplitter(this);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_sp);

    ViewListDocker *docker = nullptr;
    if (mainWindow() == nullptr) {
        // Don't use docker if embedded
        m_viewlist = new ViewListWidget(doc, m_sp);
        m_viewlist->setProject(&(getProject()));
        connect(m_viewlist, &ViewListWidget::selectionChanged, this, &View::slotSelectionChanged);
        connect(this, &View::currentScheduleManagerChanged, m_viewlist, &ViewListWidget::setSelectedSchedule);
        connect(m_viewlist, &ViewListWidget::updateViewInfo, this, &View::slotUpdateViewInfo);
    } else {
        ViewListDockerFactory vl(this);
        docker = static_cast<ViewListDocker *>(mainWindow()->createDockWidget(&vl));
        if (docker->view() != this) {
            docker->setView(this);
        }
        m_viewlist = docker->viewList();
#if 0        //SchedulesDocker
        SchedulesDockerFactory sdf;
        SchedulesDocker *sd = dynamic_cast<SchedulesDocker*>(createDockWidget(&sdf));
        Q_ASSERT(sd);

        sd->setProject(&getProject());
        connect(sd, SIGNAL(selectionChanged(KPlato::ScheduleManager*)), SLOT(slotSelectionChanged(KPlato::ScheduleManager*)));
        connect(this, &View::currentScheduleManagerChanged, sd, SLOT(setSelectedSchedule(KPlato::ScheduleManager*)));
#endif
    }

    m_tab = new QStackedWidget(m_sp);

////////////////////////////////////////////////////////////////////////////////////////////////////

    // Add sub views

    // The menu items
    // ------ File
    actionCreateTemplate = new QAction(koIcon("document-save-as-template"), i18n("Create Project Template..."), this);
    actionCollection()->addAction(QStringLiteral("file_createtemplate"), actionCreateTemplate);
    connect(actionCreateTemplate, SIGNAL(triggered(bool)), SLOT(slotCreateTemplate()));

    actionCreateNewProject = new QAction(i18n("Create New Project..."), this);
    actionCollection()->addAction(QStringLiteral("file_createnewproject"), actionCreateNewProject);
    connect(actionCreateNewProject, &QAction::triggered, this, &View::slotCreateNewProject);

    // ------ Edit
    actionCut = actionCollection()->addAction(KStandardAction::Cut,  QStringLiteral("edit_cut"), this, SLOT(slotEditCut()));
    actionCopy = actionCollection()->addAction(KStandardAction::Copy,  QStringLiteral("edit_copy"), this, SLOT(slotEditCopy()));
    actionPaste = actionCollection()->addAction(KStandardAction::Paste,  QStringLiteral("edit_paste"), this, SLOT(slotEditPaste()));

    // ------ View
    actionCollection()->addAction(KStandardAction::Redisplay, QStringLiteral("view_refresh") , this, SLOT(slotRefreshView()));

    actionViewSelector  = new KToggleAction(i18n("Show Selector"), this);
    actionCollection()->addAction(QStringLiteral("view_show_selector"), actionViewSelector);
    connect(actionViewSelector, &QAction::triggered, this, &View::slotViewSelector);

    // ------ Insert

    // ------ Project
    actionEditMainProject  = new QAction(koIcon("view-time-schedule-edit"), i18n("Edit..."), this);
    actionCollection()->addAction(QStringLiteral("project_edit"), actionEditMainProject);
    connect(actionEditMainProject, &QAction::triggered, this, &View::slotProjectEdit);

    actionEditStandardWorktime  = new QAction(koIcon("configure"), i18n("Define Estimate Conversions..."), this);
    actionCollection()->addAction(QStringLiteral("project_worktime"), actionEditStandardWorktime);
    connect(actionEditStandardWorktime, &QAction::triggered, this, &View::slotProjectWorktime);

    actionDefineWBS  = new QAction(koIcon("configure"), i18n("Define WBS Pattern..."), this);
    actionCollection()->addAction(QStringLiteral("tools_define_wbs"), actionDefineWBS);
    connect(actionDefineWBS, &QAction::triggered, this, &View::slotDefineWBS);

    actionCurrencyConfig  = new QAction(koIcon("configure"), i18n("Define Currency..."), this);
    actionCollection()->addAction(QStringLiteral("config_currency"), actionCurrencyConfig);
    connect(actionCurrencyConfig, &QAction::triggered, this, &View::slotCurrencyConfig);

    QAction *actionProjectDescription = new QAction(koIcon("document-edit"), i18n("Edit Description..."), this);
    actionCollection()->addAction(QStringLiteral("edit_project_description"), actionProjectDescription);
    connect(actionProjectDescription, &QAction::triggered, this, &View::slotOpenProjectDescription);

    // ------ Tools
    actionInsertFile  = new QAction(koIcon("document-import"), i18n("Insert Project File..."), this);
    actionCollection()->addAction(QStringLiteral("insert_file"), actionInsertFile);
    connect(actionInsertFile, &QAction::triggered, this, &View::slotInsertFile);

    auto a = new QAction(koIcon("view-refresh"), i18n("Update Shared Resources"), this);
    actionCollection()->addAction(QStringLiteral("load_shared_resources"), a);
    connect(a, &QAction::triggered, this, &View::slotUpdateSharedResources);

#ifdef PLAN_USE_KREPORT
    actionOpenReportFile  = new QAction(koIcon("document-open"), i18n("Open Report Definition File..."), this);
    actionCollection()->addAction(QStringLiteral("reportdesigner_open_file"), actionOpenReportFile);
    connect(actionOpenReportFile, QAction::triggered, this, &View::slotOpenReportFile);
#endif

    actionCreateReportTemplate = new QAction(koIcon("document-edit"), i18n("Create Report Template..."), this);
    actionCollection()->addAction(QStringLiteral("create_report_template"), actionCreateReportTemplate);
    connect(actionCreateReportTemplate, &QAction::triggered, this, &View::slotCreateReportTemplate);
    // Settings

    // ------ Popup
    auto actionOpenNode  = new QAction(koIcon("document-edit"), i18n("Edit..."), this);
    actionCollection()->addAction(QStringLiteral("project_properties"), actionOpenNode);
    connect(actionOpenNode, &QAction::triggered, this, &View::slotOpenCurrentNode);

    // Viewlist popup
    connect(m_viewlist, &ViewListWidget::createView, this, &View::slotCreateView);

    m_workPackageButton = new QToolButton(this);
    m_workPackageButton->hide();
    m_workPackageButton->setIcon(koIcon("application-x-vnd.kde.plan.work"));
    m_workPackageButton->setText(i18n("Work Packages..."));
    m_workPackageButton->setToolTip(i18nc("@info:tooltip", "Work packages available"));
    m_workPackageButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    connect(m_workPackageButton, &QToolButton::clicked, this, &View::openWorkPackageMergeDialog);
    m_estlabel = new QLabel(QLatin1String(), nullptr);
    if (statusBar()) {
        addStatusBarItem(m_estlabel, 0, true);
    }

    connect(&getProject(), &Project::scheduleManagerAdded, this, &View::slotScheduleAdded);
    connect(&getProject(), &Project::scheduleManagerRemoved, this, &View::slotScheduleRemoved);
    connect(&getProject(), &Project::scheduleManagersSwapped, this, &View::slotScheduleSwapped);
    connect(&getProject(), &Project::sigCalculationFinished, this, &View::slotScheduleCalculated);
    slotPlugScheduleActions();

    connect(doc, &MainDocument::changed, this, &View::slotUpdate);

    connect(m_scheduleActionGroup, &QActionGroup::triggered, this, &View::slotViewSchedule);


    connect(mainDocument(), &MainDocument::workPackageLoaded, this, &View::slotWorkPackageLoaded);

    // views take time for large projects
    QTimer::singleShot(0, this, &View::initiateViews);

    const QList<KPluginFactory *> pluginFactories =
        KoPluginLoader::instantiatePluginFactories(QStringLiteral("calligraplan/extensions"));

    for (KPluginFactory* factory : pluginFactories) {
        QObject *object = factory->create<QObject>(this, QVariantList());
        KXMLGUIClient *clientPlugin = dynamic_cast<KXMLGUIClient*>(object);
        if (clientPlugin) {
            insertChildClient(clientPlugin);
        } else {
            // not our/valid plugin, so delete the created object
            object->deleteLater();
        }
    }
    //debugPlan<<" end";
}

View::~View()
{
    // Disconnect and delete so we do not get called by destroyed() signal
    const QMap<QAction*, ScheduleManager*> map = m_scheduleActions; // clazy:exclude=qmap-with-pointer-key
    QMap<QAction*, ScheduleManager*>::const_iterator it;
    for (it = map.constBegin(); it != map.constEnd(); ++it) {
        disconnect(it.key(), &QObject::destroyed, this, &View::slotActionDestroyed);
        m_scheduleActionGroup->removeAction(it.key());
        delete it.key();
    }
    ViewBase *view = currentView();
    if (view) {
        // deactivate view to remove dockers etc
        slotGuiActivated(view, false);
    }
    /*    removeStatusBarItem(m_estlabel);
    delete m_estlabel;*/
}

void View::initiateViews()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    createViews();

    connect(m_viewlist, &ViewListWidget::activated, this, &View::slotViewActivated);
    // after createViews() !!
    connect(m_viewlist, &ViewListWidget::viewListItemRemoved, this, &View::slotViewListItemRemoved);
    // after createViews() !!
    connect(m_viewlist, &ViewListWidget::viewListItemInserted, this, &View::slotViewListItemInserted);

    ViewListDocker *docker = qobject_cast<ViewListDocker*>(m_viewlist->parent());
    if (docker) {
        // after createViews() !!
        connect(m_viewlist, &ViewListWidget::modified, docker, &ViewListDocker::slotModified);
        connect(m_viewlist, &ViewListWidget::modified, mainDocument(), &MainDocument::slotViewlistModified);
        connect(mainDocument(), &MainDocument::viewlistModified, docker, &ViewListDocker::updateWindowTitle);
    }
    connect(m_tab, &QStackedWidget::currentChanged, this, &View::slotCurrentChanged);

    slotSelectDefaultView();

    loadContext();

    QApplication::restoreOverrideCursor();
}

void View::slotCreateNewProject()
{
    debugPlan;
    if (KMessageBox::Continue == KMessageBox::warningContinueCancel(this,
                      xi18nc("@info",
                             "<note>This action cannot be undone.</note><nl/><nl/>"
                             "Create a new Project from the current project "
                             "with new project- and task identities.<nl/>"
                             "Resource- and calendar identities are not changed.<nl/>"
                             "All scheduling information is removed.<nl/>"
                             "<nl/>Do you want to continue?")))
    {
        Q_EMIT currentScheduleManagerChanged(nullptr);
        mainDocument()->createNewProject();
        slotOpenNode(&getProject());
    }
}

void View::slotCreateTemplate()
{
    debugPlan;
    KoFileDialog dlg(nullptr, KoFileDialog::SaveFile, QStringLiteral("Create Template"));
    dlg.setNameFilters(QStringList()<<QStringLiteral("Plan Template (*.plant)"));
    QString file = dlg.filename();
    if (!file.isEmpty()) {
        QTemporaryDir dir;
        dir.setAutoRemove(false);
        QString tmpfile = dir.path() + QLatin1Char('/') + QUrl(file).fileName();
        tmpfile.replace(QStringLiteral(".plant"), QStringLiteral(".plan"));
        Part *part = new Part(this);
        MainDocument *doc = new MainDocument(part);
        part->setDocument(doc);
        doc->disconnect(); // doc shall not handle feedback from openUrl()
        doc->setAutoSave(0); //disable
        doc->setSkipSharedResourcesAndProjects(true); // crashes if shared resources are not skipped
        bool ok = koDocument()->exportDocument(QUrl::fromUserInput(QStringLiteral("file:/") + tmpfile));
        ok &= doc->loadNativeFormat(tmpfile);
        if (ok) {
            // strip unused data
            Project *project = doc->project();
            const auto scheduleManagers = project->scheduleManagers();
            for (ScheduleManager *sm : scheduleManagers) {
                DeleteScheduleManagerCmd c(*project, sm);
                c.redo();
            }
        }
        doc->setSavingTemplate(true);
        doc->saveNativeFormat(file);
        part->deleteLater();
    }
}

void View::createViews()
{
    Context *ctx = mainDocument()->context();
    if (ctx && ctx->isLoaded()) {
        debugPlan<<"isLoaded";
        KoXmlNode n = ctx->context().namedItem("categories");
        if (n.isNull()) {
            warnPlan<<"No categories";
        } else {
            n = n.firstChild();
            for (; ! n.isNull(); n = n.nextSibling()) {
                if (! n.isElement()) {
                    continue;
                }
                KoXmlElement e = n.toElement();
                if (e.tagName() != QStringLiteral("category")) {
                    continue;
                }
                debugPlan<<"category: "<<e.attribute("tag");
                ViewListItem *cat;
                QString cn = e.attribute("name");
                QString ct = e.attribute("tag");
                if (cn.isEmpty()) {
                    cn = defaultCategoryInfo(ct).name;
                }
                cat = m_viewlist->addCategory(ct, cn);
                KoXmlNode n1 = e.firstChild();
                for (; ! n1.isNull(); n1 = n1.nextSibling()) {
                    if (! n1.isElement()) {
                        continue;
                    }
                    KoXmlElement e1 = n1.toElement();
                    if (e1.tagName() != QStringLiteral("view")) {
                        continue;
                    }
                    ViewBase *v = nullptr;
                    QString type = e1.attribute("viewtype");
                    QString tag = e1.attribute("tag");
                    QString name = e1.attribute("name");
                    QString tip = e1.attribute("tooltip");
                    v = createView(cat, type, tag, name, tip);
                    //KoXmlNode settings = e1.namedItem("settings "); ????
                    KoXmlNode settings = e1.firstChild();
                    for (; ! settings.isNull(); settings = settings.nextSibling()) {
                        if (settings.nodeName() == QStringLiteral("settings")) {
                            break;
                        }
                    }
                    if (v && settings.isElement()) {
                        debugPlan<<" settings";
                        v->loadContext(settings.toElement());
                    }
                }
            }
        }
        if (ctx->version() == 0) {
            // when opening a file created with an earlier version, new views are not shown by default
            const auto ct = QStringLiteral("Editors");
            if (!m_viewlist->findView(QStringLiteral("ResourceGroupEditor"))) {
                const auto cat = m_viewlist->addCategory(ct, defaultCategoryInfo(ct).name); // make sure category exists
                const int idx = m_viewlist->indexOf(ct, QStringLiteral("ResourceEditor")); // insert before resource editor
                createResourceGroupEditor(cat, QStringLiteral("ResourceGroupEditor"), QString(), QLatin1String(TIP_USE_DEFAULT_TEXT), idx);
            }
        }
    } else {
        debugPlan<<"Default";
        ViewBase *v = nullptr;
        ViewListItem *cat;
        QString ct = QStringLiteral("Editors");
        cat = m_viewlist->addCategory(ct, defaultCategoryInfo(ct).name);

        createCalendarEditor(cat, QStringLiteral("CalendarEditor"), QString(), QLatin1String(TIP_USE_DEFAULT_TEXT));

        createAccountsEditor(cat, QStringLiteral("AccountsEditor"), QString(), QLatin1String(TIP_USE_DEFAULT_TEXT));

        createResourceGroupEditor(cat, QStringLiteral("ResourceGroupEditor"), QString(), QLatin1String(TIP_USE_DEFAULT_TEXT));

        createResourceEditor(cat, QStringLiteral("ResourceEditor"), QString(), QLatin1String(TIP_USE_DEFAULT_TEXT));

        v = createTaskEditor(cat, QStringLiteral("TaskEditor"), QString(), QLatin1String(TIP_USE_DEFAULT_TEXT));
        m_defaultView = m_tab->count() - 1;
        v->showColumns(QList<int>() << NodeModel::NodeName
                                    << NodeModel::NodeType
                                    << NodeModel::NodeAllocation
                                    << NodeModel::NodeEstimateCalendar
                                    << NodeModel::NodeEstimate
                                    << NodeModel::NodeOptimisticRatio
                                    << NodeModel::NodePessimisticRatio
                                    << NodeModel::NodeRisk
                                    << NodeModel::NodeResponsible
                                    << NodeModel::NodeDescription
                    );

        v = createTaskEditor(cat, QStringLiteral("TaskConstraintEditor"), i18n("Task Constraints"), i18n("Edit task scheduling constraints"));
        v->showColumns(QList<int>() << NodeModel::NodeName
                                    << NodeModel::NodeType
                                    << NodeModel::NodePriority
                                    << NodeModel::NodeConstraint
                                    << NodeModel::NodeConstraintStart
                                    << NodeModel::NodeConstraintEnd
                                    << NodeModel::NodeDescription
                      );

        v = createTaskEditor(cat, QStringLiteral("TaskCostEditor"), i18n("Task Cost"), i18n("Edit task cost"));
        v->showColumns(QList<int>() << NodeModel::NodeName
                                    << NodeModel::NodeType
                                    << NodeModel::NodeRunningAccount
                                    << NodeModel::NodeStartupAccount
                                    << NodeModel::NodeStartupCost
                                    << NodeModel::NodeShutdownAccount
                                    << NodeModel::NodeShutdownCost
                                    << NodeModel::NodeDescription
                      );

        createDependencyEditor(cat, QStringLiteral("DependencyEditor"), QString(), QLatin1String(TIP_USE_DEFAULT_TEXT));

        // Do not show by default
        // createPertEditor(cat, QStringLiteral("PertEditor"), QString(), QLatin1String(TIP_USE_DEFAULT_TEXT));

        createScheduleHandler(cat, QStringLiteral("ScheduleHandlerView"), QString(), QLatin1String(TIP_USE_DEFAULT_TEXT));

        ct = QStringLiteral("Views");
        cat = m_viewlist->addCategory(ct, defaultCategoryInfo(ct).name);

        createGanttView(cat, QStringLiteral("GanttView"), QString(), QLatin1String(TIP_USE_DEFAULT_TEXT));

        createMilestoneGanttView(cat, QStringLiteral("MilestoneGanttView"), QString(), QLatin1String(TIP_USE_DEFAULT_TEXT));

        createResourceAppointmentsView(cat, QStringLiteral("ResourceAppointmentsView"), QString(), QLatin1String(TIP_USE_DEFAULT_TEXT));

        createResourceAppointmentsGanttView(cat, QStringLiteral("ResourceAppointmentsGanttView"), QString(), QLatin1String(TIP_USE_DEFAULT_TEXT));

        createAccountsView(cat, QStringLiteral("AccountsView"), QString(), QLatin1String(TIP_USE_DEFAULT_TEXT));

        createResourceCoverageView(cat, QStringLiteral("ResourceCoverageView"), QString(), QLatin1String(TIP_USE_DEFAULT_TEXT));

        ct = QStringLiteral("Execution");
        cat = m_viewlist->addCategory(ct, defaultCategoryInfo(ct).name);

        createProjectStatusView(cat, QStringLiteral("ProjectStatusView"), QString(), QLatin1String(TIP_USE_DEFAULT_TEXT));

        createPerformanceStatusView(cat, QStringLiteral("PerformanceStatusView"), QString(), QLatin1String(TIP_USE_DEFAULT_TEXT));

        createTaskStatusView(cat, QStringLiteral("TaskStatusView"), QString(), QLatin1String(TIP_USE_DEFAULT_TEXT));

        createTaskView(cat, QStringLiteral("TaskView"), QString(), QLatin1String(TIP_USE_DEFAULT_TEXT));

        createTaskWorkPackageView(cat, QStringLiteral("TaskWorkPackageView"), QString(), QLatin1String(TIP_USE_DEFAULT_TEXT));

        ct = QStringLiteral("Reports");
        cat = m_viewlist->addCategory(ct, defaultCategoryInfo(ct).name);

        createReportsGeneratorView(cat, QStringLiteral("ReportsGeneratorView"), i18n("Generate reports"), QLatin1String(TIP_USE_DEFAULT_TEXT));

#ifdef PLAN_USE_KREPORT
        // Let user add reports explicitly, we prefer reportsgenerator now
        // A little hack to get the user started...
#if 0
        ReportView *rv = qobject_cast<ReportView*>(createReportView(cat, QStringLiteral("ReportView"), i18n("Task Status Report"), QLatin1String(TIP_USE_DEFAULT_TEXT)));
        if (rv) {
            QDomDocument doc;
            doc.setContent(standardTaskStatusReport());
            rv->loadXML(doc);
        }
#endif
#endif
    }
}

ViewBase *View::createView(ViewListItem *cat, const QString &type, const QString &tag, const QString &name, const QString &tip, int index)
{
    ViewBase *v = nullptr;
    //NOTE: type is the same as classname (so if it is changed...)
    if (type == QStringLiteral("CalendarEditor")) {
        v = createCalendarEditor(cat, tag, name, tip, index);
    } else if (type == QStringLiteral("AccountsEditor")) {
        v = createAccountsEditor(cat, tag, name, tip, index);
    } else if (type == QStringLiteral("ResourceGroupEditor")) {
        v = createResourceGroupEditor(cat, tag, name, tip, index);
    } else if (type == QStringLiteral("ResourceEditor")) {
        v = createResourceEditor(cat, tag, name, tip, index);
    } else if (type == QStringLiteral("TaskEditor")) {
        v = createTaskEditor(cat, tag, name, tip, index);
    } else if (type == QStringLiteral("DependencyEditor")) {
        v = createDependencyEditor(cat, tag, name, tip, index);
    } else if (type == QStringLiteral("PertEditor")) {
        v = createPertEditor(cat, tag, name, tip, index);
    } else if (type == QStringLiteral("ScheduleEditor")) {
        v = createScheduleEditor(cat, tag, name, tip, index);
    } else if (type == QStringLiteral("ScheduleHandlerView")) {
        v = createScheduleHandler(cat, tag, name, tip, index);
    } else if (type == QStringLiteral("ProjectStatusView")) {
        v = createProjectStatusView(cat, tag, name, tip, index);
    } else if (type == QStringLiteral("TaskStatusView")) {
        v = createTaskStatusView(cat, tag, name, tip, index);
    } else if (type == QStringLiteral("TaskView")) {
        v = createTaskView(cat, tag, name, tip, index);
    } else if (type == QStringLiteral("TaskWorkPackageView")) {
        v = createTaskWorkPackageView(cat, tag, name, tip, index);
    } else if (type == QStringLiteral("GanttView")) {
        v = createGanttView(cat, tag, name, tip, index);
    } else if (type == QStringLiteral("MilestoneGanttView")) {
        v = createMilestoneGanttView(cat, tag, name, tip, index);
    } else if (type == QStringLiteral("ResourceAppointmentsView")) {
        v = createResourceAppointmentsView(cat, tag, name, tip, index);
    } else if (type == QStringLiteral("ResourceAppointmentsGanttView")) {
        v = createResourceAppointmentsGanttView(cat, tag, name, tip, index);
    } else if (type == QStringLiteral("ResourceCoverageView")) {
        v = createResourceCoverageView(cat, tag, name, tip, index);
    } else if (type == QStringLiteral("AccountsView")) {
        v = createAccountsView(cat, tag, name, tip, index);
    } else if (type == QStringLiteral("PerformanceStatusView")) {
        v = createPerformanceStatusView(cat, tag, name, tip, index);
    } else if (type == QStringLiteral("ReportsGeneratorView")) {
        v = createReportsGeneratorView(cat, tag, name, tip, index);
    } else if (type == QStringLiteral("ReportView")) {
#ifdef PLAN_USE_KREPORT
        v = createReportView(cat, tag, name, tip, index);
#endif
    } else  {
        warnPlan<<"Unknown viewtype: "<<type;
    }
    return v;
}

void View::slotUpdateViewInfo(ViewListItem *itm)
{
    if (itm->type() == ViewListItem::ItemType_SubView) {
        itm->setViewInfo(defaultViewInfo(itm->viewType()));
    } else if (itm->type() == ViewListItem::ItemType_Category) {
        ViewInfo vi = defaultCategoryInfo(itm->tag());
        itm->setViewInfo(vi);
    }
}

ViewInfo View::defaultViewInfo(const QString &type) const
{
    ViewInfo vi;
    if (type == QStringLiteral("CalendarEditor")) {
        vi.name = i18n("Work & Vacation");
        vi.tip = xi18nc("@info:tooltip", "Edit working- and vacation days for resources");
    } else if (type == QStringLiteral("AccountsEditor")) {
        vi.name = i18n("Cost Breakdown Structure");
        vi.tip = xi18nc("@info:tooltip", "Edit cost breakdown structure.");
    } else if (type == QStringLiteral("ResourceGroupEditor")) {
        vi.name = i18n("Resource Breakdown Structure");
        vi.tip = xi18nc("@info:tooltip", "Edit resource breakdown structure");
    } else if (type == QStringLiteral("ResourceEditor")) {
        vi.name = i18n("Resources");
        vi.tip = xi18nc("@info:tooltip", "Edit resources");
    } else if (type == QStringLiteral("TaskEditor")) {
        vi.name = i18n("Tasks");
        vi.tip = xi18nc("@info:tooltip", "Edit work breakdown structure");
    } else if (type == QStringLiteral("DependencyEditor")) {
        vi.name = i18n("Dependencies (Graphic)");
        vi.tip = xi18nc("@info:tooltip", "Edit task dependencies");
    } else if (type == QStringLiteral("PertEditor")) {
        vi.name = i18n("Dependencies (List)");
        vi.tip = xi18nc("@info:tooltip", "Edit task dependencies");
    } else if (type == QStringLiteral("ScheduleEditor")) {
        // This view is not used stand-alone atm
        vi.name = i18n("Schedules");
    } else if (type == QStringLiteral("ScheduleHandlerView")) {
        vi.name = i18n("Schedules");
        vi.tip = xi18nc("@info:tooltip", "Calculate and analyze project schedules");
    } else if (type == QStringLiteral("ProjectStatusView")) {
        vi.name = i18n("Project Performance Chart");
        vi.tip = xi18nc("@info:tooltip", "View project status information");
    } else if (type == QStringLiteral("TaskStatusView")) {
        vi.name = i18n("Task Status");
        vi.tip = xi18nc("@info:tooltip", "View task progress information");
    } else if (type == QStringLiteral("TaskView")) {
        vi.name = i18n("Task Execution");
        vi.tip = xi18nc("@info:tooltip", "View task execution information");
    } else if (type == QStringLiteral("TaskWorkPackageView")) {
        vi.name = i18n("Work Package View");
        vi.tip = xi18nc("@info:tooltip", "View task work package information");
    } else if (type == QStringLiteral("GanttView")) {
        vi.name = i18n("Gantt");
        vi.tip = xi18nc("@info:tooltip", "View Gantt chart");
    } else if (type == QStringLiteral("MilestoneGanttView")) {
        vi.name = i18n("Milestone Gantt");
        vi.tip = xi18nc("@info:tooltip", "View milestone Gantt chart");
    } else if (type == QStringLiteral("ResourceAppointmentsView")) {
        vi.name = i18n("Resource Assignments");
        vi.tip = xi18nc("@info:tooltip", "View resource assignments in a table");
    } else if (type == QStringLiteral("ResourceAppointmentsGanttView")) {
        vi.name = i18n("Resource Assignments (Gantt)");
        vi.tip = xi18nc("@info:tooltip", "View resource assignments in Gantt chart");
    } else if (type == QStringLiteral("ResourceCoverageView")) {
        vi.name = i18n("Resource Coverage");
        vi.tip = xi18nc("@info:tooltip", "Inspect resource coverage");
    } else if (type == QStringLiteral("AccountsView")) {
        vi.name = i18n("Cost Breakdown");
        vi.tip = xi18nc("@info:tooltip", "View planned and actual cost");
    } else if (type == QStringLiteral("PerformanceStatusView")) {
        vi.name = i18n("Tasks Performance Chart");
        vi.tip = xi18nc("@info:tooltip", "View tasks performance status information");
    } else if (type == QStringLiteral("ReportsGeneratorView")) {
        vi.name = i18n("Reports Generator");
        vi.tip = xi18nc("@info:tooltip", "Generate reports");
    } else if (type == QStringLiteral("ReportView")) {
        vi.name = i18n("Report");
        vi.tip = xi18nc("@info:tooltip", "View report");
    } else  {
        warnPlan<<"Unknown viewtype: "<<type;
    }
    return vi;
}

ViewInfo View::defaultCategoryInfo(const QString &type) const
{
    ViewInfo vi;
    if (type == QStringLiteral("Editors")) {
        vi.name = i18n("Editors");
    } else if (type == QStringLiteral("Views")) {
        vi.name = i18n("Views");
    } else if (type == QStringLiteral("Execution")) {
        vi.name = i18nc("Project execution views", "Execution");
    } else if (type == QStringLiteral("Reports")) {
        vi.name = i18n("Reports");
    }
    return vi;
}

ViewBase *View::createResourceAppointmentsGanttView(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    ResourceAppointmentsGanttView *v = new ResourceAppointmentsGanttView(getKoPart(), mainDocument(), m_tab);
    m_tab->addWidget(v);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, v, mainDocument(), QLatin1String(), index);
    ViewInfo vi = defaultViewInfo(QStringLiteral("ResourceAppointmentsGanttView"));
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == QLatin1String(TIP_USE_DEFAULT_TEXT)) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }
    connect(v, &ViewBase::requestPopupMenu, this, &View::slotPopupMenuRequested);
    connect(v, &ViewBase::guiActivated, this, &View::slotGuiActivated);
    connect(this, &View::currentScheduleManagerChanged, v, &ResourceAppointmentsGanttView::setScheduleManager);

    v->setProject(&(getProject()));
    v->setScheduleManager(currentScheduleManager());
    v->updateReadWrite(true); // always allow editing
    return v;
}

ViewBase *View::createResourceCoverageView(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    ResourceCoverageView *v = new ResourceCoverageView(getKoPart(), mainDocument(), m_tab);
    m_tab->addWidget(v);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, v, mainDocument(), QLatin1String(), index);
    ViewInfo vi = defaultViewInfo(QStringLiteral("ResourceCoverageView"));
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == QLatin1String(TIP_USE_DEFAULT_TEXT)) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }
    connect(v, &ViewBase::requestPopupMenu, this, &View::slotPopupMenuRequested);
    connect(v, &ViewBase::guiActivated, this, &View::slotGuiActivated);
    connect(this, &View::currentScheduleManagerChanged, v, &ResourceCoverageView::setScheduleManager);

    v->setProject(&(getProject()));
    v->setScheduleManager(currentScheduleManager());
    v->updateReadWrite(true); // always allow editing
    return v;
}

ViewBase *View::createResourceAppointmentsView(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    ResourceAppointmentsView *v = new ResourceAppointmentsView(getKoPart(), mainDocument(), m_tab);
    m_tab->addWidget(v);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, v, mainDocument(), QLatin1String(), index);
    ViewInfo vi = defaultViewInfo(QStringLiteral("ResourceAppointmentsView"));
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == QLatin1String(TIP_USE_DEFAULT_TEXT)) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }
    connect(v, &ViewBase::requestPopupMenu, this, &View::slotPopupMenuRequested);
    connect(v, &ViewBase::guiActivated, this, &View::slotGuiActivated);
    connect(this, &View::currentScheduleManagerChanged, v, &ResourceAppointmentsView::setScheduleManager);

    v->setProject(&(getProject()));
    v->setScheduleManager(currentScheduleManager());
    v->updateReadWrite(true); // always allow editing
    return v;
}

ViewBase *View::createResourceGroupEditor(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    ResourceGroupEditor *e = new ResourceGroupEditor(getKoPart(), mainDocument(), m_tab);
    e->setViewSplitMode(false);
    m_tab->addWidget(e);
    e->setProject(&(getProject()));
    
    ViewListItem *i = m_viewlist->addView(cat, tag, name, e, mainDocument(), QLatin1String(), index);
    ViewInfo vi = defaultViewInfo(QStringLiteral("ResourceGroupEditor"));
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == QLatin1String(TIP_USE_DEFAULT_TEXT)) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }
    connect(e, &ViewBase::requestPopupMenu, this, &View::slotPopupMenuRequested);
    connect(e, &ViewBase::guiActivated, this, &View::slotGuiActivated);

    e->updateReadWrite(true); // always allow editing
    return e;
}

ViewBase *View::createResourceEditor(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    ResourceEditor *resourceeditor = new ResourceEditor(getKoPart(), mainDocument(), m_tab);
    resourceeditor->setViewSplitMode(false);
    m_tab->addWidget(resourceeditor);
    resourceeditor->setProject(&(getProject()));

    ViewListItem *i = m_viewlist->addView(cat, tag, name, resourceeditor, mainDocument(), QLatin1String(), index);
    ViewInfo vi = defaultViewInfo(QStringLiteral("ResourceEditor"));
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == QLatin1String(TIP_USE_DEFAULT_TEXT)) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }
    connect(resourceeditor, &ViewBase::requestPopupMenu, this, &View::slotPopupMenuRequested);
    connect(resourceeditor, &ViewBase::guiActivated, this, &View::slotGuiActivated);

    resourceeditor->updateReadWrite(true); // always allow editing
    return resourceeditor;
}

ViewBase *View::createTaskEditor(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    TaskEditor *taskeditor = new TaskEditor(getKoPart(), mainDocument(), m_tab);
    taskeditor->setViewSplitMode(false);
    m_tab->addWidget(taskeditor);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, taskeditor, mainDocument(), QLatin1String(), index);
    ViewInfo vi = defaultViewInfo(QStringLiteral("TaskEditor"));
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == QLatin1String(TIP_USE_DEFAULT_TEXT)) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }

    taskeditor->setProject(&(getProject()));
    taskeditor->setScheduleManager(currentScheduleManager());

    connect(this, &View::currentScheduleManagerChanged, taskeditor, &TaskEditor::setScheduleManager);

    connect(taskeditor, &ViewBase::requestPopupMenu, this, &View::slotPopupMenuRequested);
    connect(taskeditor, &ViewBase::guiActivated, this, &View::slotGuiActivated);

    connect(taskeditor, &TaskEditor::saveTaskModule, this, &View::saveTaskModule);
    connect(taskeditor, &TaskEditor::removeTaskModule, this, &View::removeTaskModule);
    connect(taskeditor, &ViewBase::openDocument, static_cast<KPlato::Part*>(m_partpart), &Part::openTaskModule);

    taskeditor->updateReadWrite(true); // always allow editing

    return taskeditor;
}

ViewBase *View::createAccountsEditor(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    AccountsEditor *ae = new AccountsEditor(getKoPart(), mainDocument(), m_tab);
    m_tab->addWidget(ae);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, ae, mainDocument(), QLatin1String(), index);
    ViewInfo vi = defaultViewInfo(QStringLiteral("AccountsEditor"));
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == QLatin1String(TIP_USE_DEFAULT_TEXT)) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }

    ae->draw(getProject());

    connect(ae, &ViewBase::requestPopupMenu, this, &View::slotPopupMenuRequested);
    connect(ae, &ViewBase::guiActivated, this, &View::slotGuiActivated);

    ae->updateReadWrite(true); // always allow editing
    return ae;
}

ViewBase *View::createCalendarEditor(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    CalendarEditor *calendareditor = new CalendarEditor(getKoPart(), mainDocument(), m_tab);
    m_tab->addWidget(calendareditor);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, calendareditor, mainDocument(), QLatin1String(), index);
    ViewInfo vi = defaultViewInfo(QStringLiteral("CalendarEditor"));
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == QLatin1String(TIP_USE_DEFAULT_TEXT)) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }

    calendareditor->draw(getProject());

    connect(calendareditor, &ViewBase::requestPopupMenu, this, &View::slotPopupMenuRequested);
    connect(calendareditor, &ViewBase::guiActivated, this, &View::slotGuiActivated);

    calendareditor->updateReadWrite(true); // always allow editing
    return calendareditor;
}

ViewBase *View::createScheduleHandler(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    ScheduleHandlerView *handler = new ScheduleHandlerView(getKoPart(), mainDocument(), m_tab);
    m_tab->addWidget(handler);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, handler, mainDocument(), QLatin1String(), index);
    ViewInfo vi = defaultViewInfo(QStringLiteral("ScheduleHandlerView"));
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == QLatin1String(TIP_USE_DEFAULT_TEXT)) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }

    connect(handler, &ViewBase::requestPopupMenu, this, &View::slotPopupMenuRequested);
    connect(handler, &ViewBase::guiActivated, this, &View::slotGuiActivated);

    connect(this, &View::currentScheduleManagerChanged, handler, &ScheduleHandlerView::currentScheduleManagerChanged);

    handler->draw(getProject());
    handler->updateReadWrite(true); // always allow editing
    return handler;
}

ScheduleEditor *View::createScheduleEditor(QWidget *parent)
{
    ScheduleEditor *scheduleeditor = new ScheduleEditor(getKoPart(), mainDocument(), parent);

    scheduleeditor->updateReadWrite(true); // always allow editing
    return scheduleeditor;
}

ViewBase *View::createScheduleEditor(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    ScheduleEditor *scheduleeditor = new ScheduleEditor(getKoPart(), mainDocument(), m_tab);
    m_tab->addWidget(scheduleeditor);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, scheduleeditor, mainDocument(), QLatin1String(), index);
    ViewInfo vi = defaultViewInfo(QStringLiteral("ScheduleEditor"));
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == QLatin1String(TIP_USE_DEFAULT_TEXT)) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }

    scheduleeditor->setProject(&(getProject()));

    connect(scheduleeditor, &ViewBase::requestPopupMenu, this, &View::slotPopupMenuRequested);
    connect(scheduleeditor, &ViewBase::guiActivated, this, &View::slotGuiActivated);

    scheduleeditor->updateReadWrite(true); // always allow editing
    return scheduleeditor;
}


ViewBase *View::createDependencyEditor(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    DependencyEditor *editor = new DependencyEditor(getKoPart(), mainDocument(), m_tab);
    m_tab->addWidget(editor);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, editor, mainDocument(), QLatin1String(), index);
    ViewInfo vi = defaultViewInfo(QStringLiteral("DependencyEditor"));
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == QLatin1String(TIP_USE_DEFAULT_TEXT)) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }

    editor->draw(getProject());

    connect(editor, &ViewBase::requestPopupMenu, this, &View::slotPopupMenuRequested);
    connect(editor, &ViewBase::guiActivated, this, &View::slotGuiActivated);

    connect(this, &View::currentScheduleManagerChanged, editor, &DependencyEditor::setScheduleManager);

    editor->updateReadWrite(true); // always allow editing
    editor->setScheduleManager(currentScheduleManager());
    return editor;
}

ViewBase *View::createPertEditor(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    PertEditor *perteditor = new PertEditor(getKoPart(), mainDocument(), m_tab);
    m_tab->addWidget(perteditor);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, perteditor, mainDocument(), QLatin1String(), index);
    ViewInfo vi = defaultViewInfo(QStringLiteral("PertEditor"));
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == QLatin1String(TIP_USE_DEFAULT_TEXT)) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }

    perteditor->draw(getProject());

    connect(perteditor, &ViewBase::requestPopupMenu, this, &View::slotPopupMenuRequested);
    connect(perteditor, &ViewBase::guiActivated, this, &View::slotGuiActivated);

    m_updatePertEditor = true;
    perteditor->updateReadWrite(true); // always allow editing
    return perteditor;
}

ViewBase *View::createProjectStatusView(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    ProjectStatusView *v = new ProjectStatusView(getKoPart(), mainDocument(), m_tab);
    m_tab->addWidget(v);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, v, mainDocument(), QLatin1String(), index);
    ViewInfo vi = defaultViewInfo(QStringLiteral("ProjectStatusView"));
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == QLatin1String(TIP_USE_DEFAULT_TEXT)) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }
    connect(v, &ViewBase::requestPopupMenu, this, &View::slotPopupMenuRequested);
    connect(v, &ViewBase::guiActivated, this, &View::slotGuiActivated);

    connect(this, &View::currentScheduleManagerChanged, v, &ProjectStatusView::setScheduleManager);

    v->updateReadWrite(true); // always allow editing
    v->setProject(&getProject());
    v->setScheduleManager(currentScheduleManager());
    return v;
}

ViewBase *View::createPerformanceStatusView(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    PerformanceStatusView *v = new PerformanceStatusView(getKoPart(), mainDocument(), m_tab);
    m_tab->addWidget(v);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, v, mainDocument(), QLatin1String(), index);
    ViewInfo vi = defaultViewInfo(QStringLiteral("PerformanceStatusView"));
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == QLatin1String(TIP_USE_DEFAULT_TEXT)) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }
    connect(v, &ViewBase::requestPopupMenu, this, &View::slotPopupMenuRequested);
    connect(v, &ViewBase::guiActivated, this, &View::slotGuiActivated);

    connect(this, &View::currentScheduleManagerChanged, v, &PerformanceStatusView::setScheduleManager);

    v->updateReadWrite(true); // always allow editing
    v->setProject(&getProject());
    v->setScheduleManager(currentScheduleManager());
    return v;
}


ViewBase *View::createTaskStatusView(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    TaskStatusView *taskstatusview = new TaskStatusView(getKoPart(), mainDocument(), m_tab);
    taskstatusview->setViewSplitMode(false);
    m_tab->addWidget(taskstatusview);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, taskstatusview, mainDocument(), QLatin1String(), index);
    ViewInfo vi = defaultViewInfo(QStringLiteral("TaskStatusView"));
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == QLatin1String(TIP_USE_DEFAULT_TEXT)) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }
    connect(taskstatusview, &ViewBase::requestPopupMenu, this, &View::slotPopupMenuRequested);
    connect(taskstatusview, &ViewBase::guiActivated, this, &View::slotGuiActivated);

    connect(this, &View::currentScheduleManagerChanged, taskstatusview, &TaskStatusView::setScheduleManager);

    taskstatusview->updateReadWrite(true); // always allow editing
    taskstatusview->draw(getProject());
    taskstatusview->setScheduleManager(currentScheduleManager());
    return taskstatusview;
}

ViewBase *View::createTaskView(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    TaskView *v = new TaskView(getKoPart(), mainDocument(), m_tab);
    v->setViewSplitMode(false);
    m_tab->addWidget(v);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, v, mainDocument(), QLatin1String(), index);
    ViewInfo vi = defaultViewInfo(QStringLiteral("TaskView"));
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == QLatin1String(TIP_USE_DEFAULT_TEXT)) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }

    v->draw(getProject());
    v->setScheduleManager(currentScheduleManager());

    connect(this, &View::currentScheduleManagerChanged, v, &TaskView::setScheduleManager);

    connect(v, &ViewBase::requestPopupMenu, this, &View::slotPopupMenuRequested);
    connect(v, &ViewBase::guiActivated, this, &View::slotGuiActivated);

    v->updateReadWrite(true); // always allow editing
    return v;
}

ViewBase *View::createTaskWorkPackageView(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    TaskWorkPackageView *v = new TaskWorkPackageView(getKoPart(), mainDocument(), m_tab);
    v->setViewSplitMode(false);
    m_tab->addWidget(v);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, v, mainDocument(), QLatin1String(), index);
    ViewInfo vi = defaultViewInfo(QStringLiteral("TaskWorkPackageView"));
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == QLatin1String(TIP_USE_DEFAULT_TEXT)) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }

    v->setProject(&getProject());
    v->setScheduleManager(currentScheduleManager());

    connect(this, &View::currentScheduleManagerChanged, v, &TaskWorkPackageView::setScheduleManager);

    connect(v, &ViewBase::requestPopupMenu, this, &View::slotPopupMenuRequested);
    connect(v, &ViewBase::guiActivated, this, &View::slotGuiActivated);

    connect(v, &TaskWorkPackageView::publishWorkpackages, this, &View::slotPublishWorkpackages);
    connect(v, &TaskWorkPackageView::openWorkpackages, this, &View::openWorkPackageMergeDialog);
    connect(this, &View::workPackagesAvailable, v, &TaskWorkPackageView::slotWorkpackagesAvailable);
    connect(v, &TaskWorkPackageView::checkForWorkPackages, mainDocument(), &MainDocument::checkForWorkPackages);
    connect(v, &TaskWorkPackageView::loadWorkPackageUrl, this, &View::loadWorkPackage);
    v->updateReadWrite(true); // always allow editing

    return v;
}

ViewBase *View::createGanttView(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    GanttView *ganttview = new GanttView(getKoPart(), mainDocument(), m_tab, true); // always allow editing
    m_tab->addWidget(ganttview);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, ganttview, mainDocument(), QLatin1String(), index);
    ViewInfo vi = defaultViewInfo(QStringLiteral("GanttView"));
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == QLatin1String(TIP_USE_DEFAULT_TEXT)) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }

    ganttview->setProject(&(getProject()));
    ganttview->setScheduleManager(currentScheduleManager());

    connect(ganttview, &ViewBase::requestPopupMenu, this, &View::slotPopupMenuRequested);
    connect(ganttview, &ViewBase::guiActivated, this, &View::slotGuiActivated);

    connect(this, &View::currentScheduleManagerChanged, ganttview, &GanttView::setScheduleManager);

    ganttview->updateReadWrite(true); // always allow editing

    return ganttview;
}

ViewBase *View::createMilestoneGanttView(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    MilestoneGanttView *ganttview = new MilestoneGanttView(getKoPart(), mainDocument(), m_tab, true); // always allow editing
    m_tab->addWidget(ganttview);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, ganttview, mainDocument(), QLatin1String(), index);
    ViewInfo vi = defaultViewInfo(QStringLiteral("MilestoneGanttView"));
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == QLatin1String(TIP_USE_DEFAULT_TEXT)) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }

    ganttview->setProject(&(getProject()));
    ganttview->setScheduleManager(currentScheduleManager());

    connect(ganttview, &ViewBase::requestPopupMenu, this, &View::slotPopupMenuRequested);
    connect(ganttview, &ViewBase::guiActivated, this, &View::slotGuiActivated);

    connect(this, &View::currentScheduleManagerChanged, ganttview, &MilestoneGanttView::setScheduleManager);

    ganttview->updateReadWrite(true); // always allow editing

    return ganttview;
}


ViewBase *View::createAccountsView(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    AccountsView *accountsview = new AccountsView(getKoPart(), &getProject(), mainDocument(), m_tab);
    m_tab->addWidget(accountsview);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, accountsview, mainDocument(), QLatin1String(), index);
    ViewInfo vi = defaultViewInfo(QStringLiteral("AccountsView"));
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == QLatin1String(TIP_USE_DEFAULT_TEXT)) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }

    accountsview->setScheduleManager(currentScheduleManager());

    connect(this, &View::currentScheduleManagerChanged, accountsview, &AccountsView::setScheduleManager);

    connect(accountsview, &ViewBase::requestPopupMenu, this, &View::slotPopupMenuRequested);
    connect(accountsview, &ViewBase::guiActivated, this, &View::slotGuiActivated);

    accountsview->updateReadWrite(true); // always allow editing
    return accountsview;
}

ViewBase *View::createReportsGeneratorView(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    ReportsGeneratorView *v = new ReportsGeneratorView(getKoPart(), mainDocument(), m_tab);
    m_tab->addWidget(v);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, v, mainDocument(), QLatin1String(), index);
    ViewInfo vi = defaultViewInfo(QStringLiteral("ReportsGeneratorView"));
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == QLatin1String(TIP_USE_DEFAULT_TEXT)) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }

    v->setProject(&getProject());

    connect(this, &View::currentScheduleManagerChanged, v, &ViewBase::setScheduleManager);
    connect(this, &View::currentScheduleManagerChanged, v, &ViewBase::slotRefreshView);
    v->setScheduleManager(currentScheduleManager());

    connect(v, &ViewBase::requestPopupMenu, this, &View::slotPopupMenuRequested);
    connect(v, &ViewBase::guiActivated, this, &View::slotGuiActivated);

    v->updateReadWrite(true); // always allow editing
    return v;
}

ViewBase *View::createReportView(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
#ifdef PLAN_USE_KREPORT
    ReportView *v = new ReportView(getKoPart(), mainDocument(), m_tab);
    m_tab->addWidget(v);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, v, mainDocument(), QLatin1String(), index);
    ViewInfo vi = defaultViewInfo(QStringLiteral("ReportView"));
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == QLatin1String(TIP_USE_DEFAULT_TEXT)) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }

    v->setProject(&getProject());

    connect(this, &View::currentScheduleManagerChanged, v, &ReportView::setScheduleManager);
    connect(this, &View::currentScheduleManagerChanged, v, SLOT(slotRefreshView()));
    v->setScheduleManager(currentScheduleManager());

    connect(v, &ReportView::guiActivated, this, &View::slotGuiActivated);
    v->updateReadWrite(true); // always allow editing
    return v;
#else
    Q_UNUSED(cat)
    Q_UNUSED(tag)
    Q_UNUSED(name)
    Q_UNUSED(tip)
    Q_UNUSED(index)
    return nullptr;
#endif
}

Project& View::getProject() const
{
    return mainDocument() ->getProject();
}

KoPrintJob * View::createPrintJob()
{
    KoView *v = qobject_cast<KoView*>(canvas());
    if (v == nullptr) {
        return nullptr;
    }
    return v->createPrintJob();
}

ViewBase *View::currentView() const
{
    return qobject_cast<ViewBase*>(m_tab->currentWidget());
}

void View::slotEditCut()
{
    ViewBase *v = currentView();
    if (v) {
        v->slotEditCut();
    }
}

void View::slotEditCopy()
{
    ViewBase *v = currentView();
    if (v) {
        v->slotEditCopy();
    }
}

void View::slotEditPaste()
{
    ViewBase *v = currentView();
    if (v) {
        v->slotEditPaste();
    }
}

void View::slotRefreshView()
{
    ViewBase *v = currentView();
    if (v) {
        debugPlan<<v;
        v->slotRefreshView();
    }
}

void View::slotViewSelector(bool show)
{
    //debugPlan;
    m_viewlist->setVisible(show);
}

void View::slotInsertResourcesFile(const QString &file)
{
    mainDocument()->insertResourcesFile(QUrl(file));
}

void View::slotInsertFile()
{
    InsertFileDialog *dlg = new InsertFileDialog(getProject(), currentTask(), this);
    connect(dlg, &QDialog::finished, this, &View::slotInsertFileFinished);
    dlg->open();
}

void View::slotInsertFileFinished(int result)
{
    InsertFileDialog *dlg = qobject_cast<InsertFileDialog*>(sender());
    if (dlg == nullptr) {
        return;
    }
    if (result == QDialog::Accepted) {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        mainDocument()->insertFile(dlg->url(), dlg->parentNode(), dlg->afterNode());
        QApplication::restoreOverrideCursor();
    }
    dlg->deleteLater();
}

void View::slotUpdateSharedResources()
{
    auto doc = mainDocument();
    auto project = doc->project();
    if (project->useSharedResources() && !project->sharedResourcesFile().isEmpty()) {
        doc->insertResourcesFile(QUrl::fromUserInput(project->sharedResourcesFile()));
    }
}

void View::slotCreateReportTemplate()
{
    class CreateReportTemplateDialog : public QDialog
    {
        ::Ui::CreateReportTemplateDialog ui;
        QMap<QString, QUrl> files;
    public:

        CreateReportTemplateDialog(KoPart *part, QWidget *parent = nullptr)
        : QDialog(parent)
        {
            ui.setupUi(this);
            QString path = QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("reports"), QStandardPaths::LocateDirectory);
            if (!path.isEmpty()) {
                QDir dir(path);
                const auto entries = dir.entryList(QDir::Files|QDir::QDir::NoDotAndDotDot);
                for(auto &file : entries) {
                    QUrl url = QUrl::fromLocalFile((path + QLatin1Char('/') + file));
                    files.insert(url.fileName(), url);
                }
            }
            KConfigGroup cfgGrp(part->componentData().config(), "Report Templates");
            if (cfgGrp.exists()) {
                const auto templates = cfgGrp.readEntry(QStringLiteral("ReportTemplatePaths")).split(QLatin1Char(','));
                for (auto &path : templates) {
                    QDir dir(path);
                    const auto entries = dir.entryList(QDir::Files|QDir::QDir::NoDotAndDotDot);
                    for(auto &file : entries) {
                        QUrl url = QUrl::fromLocalFile(path + QLatin1Char('/') + file);
                        files.insert(url.fileName(), url);
                    }
                }
            }
            ui.templates->insertItems(0, files.keys());
            ui.templates->setCurrentIndex(0);
            ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
            connect(ui.fileName, &KUrlRequester::textChanged, this, [this](const QString &text) {
                ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!text.isEmpty());
            });
        }
        QUrl url() const {
            const auto tmp = files.value(ui.templates->currentText());
            const auto url = ui.fileName->url();
            if (url.isEmpty() || tmp == url) {
                return url;
            }
            if (QFile::exists(url.toLocalFile())) {
                if (KMessageBox::warningTwoActions(nullptr,
                                                   i18n("The file '%1' already exists, do you want to overwrite it?", url.fileName()),
                                                   i18nc("@window:title","Report Template"),
                                                   KStandardGuiItem::overwrite(),
                                                   KStandardGuiItem::cancel()) == KMessageBox::SecondaryAction) {
                    return QUrl();
                }
            }
            if (!tmp.isEmpty()) {
                QFile::remove(url.toLocalFile());
                QFile file(tmp.toLocalFile());
                file.copy(url.toLocalFile());
                Q_ASSERT(QFile::exists(url.toLocalFile()));
            }
            return url;
        }
        void openAssistant(QWidget *parent = nullptr) const {
            if (ui.openAssistant->isChecked()) {
                auto url = QUrl::fromLocalFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("calligraplan/data/ReportTemplateAssistant.odt")));
                if (!url.isValid()) {
                    return;
                }
                auto job = new KIO::OpenUrlJob(url);
                job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, parent));
                job->start();
            }
        }
    };
    CreateReportTemplateDialog dlg(getKoPart());
    if (dlg.exec() == QDialog::Accepted) {
        QUrl url = dlg.url();
        if (!url.isEmpty()) {
            auto job = new KIO::OpenUrlJob(url);
            job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, window()));
            job->start();
        }
        dlg.openAssistant(window());
    }
}

void View::slotProjectEdit()
{
    slotOpenNode(&getProject());
}

void View::slotProjectWorktime()
{
    StandardWorktimeDialog *dia = new StandardWorktimeDialog(getProject(), this);
    connect(dia, &QDialog::finished, this, &View::slotProjectWorktimeFinished);
    dia->open();
}

void View::slotProjectWorktimeFinished(int result)
{
    StandardWorktimeDialog *dia = qobject_cast<StandardWorktimeDialog*>(sender());
    if (dia == nullptr) {
        return;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command * cmd = dia->buildCommand();
        if (cmd) {
            //debugPlan<<"Modifying calendar(s)";
            mainDocument() ->addCommand(cmd); //also executes
        }
    }
    dia->deleteLater();
}

void View::slotSelectionChanged(ScheduleManager *sm) {
    debugPlan<<sm;
    if (sm == nullptr) {
        return;
    }
    QAction *a = m_scheduleActions.key(sm);
    if (!a) {
        debugPlan<<sm<<"could not find action for schedule:"<<sm;
        return;
    }
    a->setChecked(true); // this doesn't trigger QActionGroup
    slotViewSchedule(a);
}

QList<QAction*> View::sortedActionList()
{
    QMap<QString, QAction*> lst;
    const QMap<QAction*, ScheduleManager*> map = m_scheduleActions; // clazy:exclude=qmap-with-pointer-key
    QMap<QAction*, ScheduleManager*>::const_iterator it;
    for (it = map.constBegin(); it != map.constEnd(); ++it) {
        lst.insert(it.key()->objectName(), it.key());
    }
    return lst.values();
}

void View::slotScheduleSwapped(ScheduleManager *from,  ScheduleManager *to)
{
    if (currentScheduleManager() == from) {
        QAction *a = m_scheduleActions.key(to);
        if (a) {
            a->setChecked(true);
        }
    }
}

void View::slotScheduleRemoved(const ScheduleManager *sch)
{
    debugPlan<<sch<<sch->name();
    QAction *a = nullptr;
    QAction *checked = m_scheduleActionGroup->checkedAction();
    QMapIterator<QAction*, ScheduleManager*> i(m_scheduleActions);
    while (i.hasNext()) {
        i.next();
        if (i.value() == sch) {
            a = i.key();
            break;
        }
    }
    if (a) {
        unplugActionList(QStringLiteral("view_schedule_list"));
        delete a;
        plugActionList(QStringLiteral("view_schedule_list"), sortedActionList());
        if (checked && checked != a) {
            checked->setChecked(true);
        } else if (! m_scheduleActions.isEmpty()) {
            m_scheduleActions.firstKey()->setChecked(true);
        }
    }
    slotViewSchedule(m_scheduleActionGroup->checkedAction());
}

void View::slotScheduleAdded(const ScheduleManager *sch)
{
    ScheduleManager *s = const_cast<ScheduleManager*>(sch);
    unplugActionList(QStringLiteral("view_schedule_list"));
    QAction *act = addScheduleAction(s);
    plugActionList(QStringLiteral("view_schedule_list"), sortedActionList());
    if (!currentScheduleManager()) {
        if (act) {
            act->setChecked(true);
        } else if (! m_scheduleActions.isEmpty()) {
            m_scheduleActions.firstKey()->setChecked(true);
        }
        slotViewSchedule(m_scheduleActionGroup->checkedAction());
    }
}

void View::slotScheduleCalculated(Project *project, ScheduleManager *manager)
{
    Q_UNUSED(project);
    if (manager == currentScheduleManager()) {
        slotViewScheduleManager(manager);
    }
}

QAction *View::addScheduleAction(ScheduleManager *sch)
{
    QAction *act = nullptr;
    QString n = sch->name();
    act = new KToggleAction(n, this);
    actionCollection()->addAction(n, act);
    m_scheduleActions.insert(act, sch);
    m_scheduleActionGroup->addAction(act);
    //debugPlan<<"Add:"<<n;
    connect(act, &QObject::destroyed, this, &View::slotActionDestroyed);
    return act;
}

void View::slotViewScheduleManager(ScheduleManager *sm)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    setLabel(sm);
    Q_EMIT currentScheduleManagerChanged(sm);
    QApplication::restoreOverrideCursor();
}

void View::slotViewSchedule(QAction *act)
{
    //debugPlan<<act;
    ScheduleManager *sm = nullptr;
    if (act != nullptr) {
        sm = m_scheduleActions.value(act, nullptr);
    }
    setLabel(nullptr);
    slotViewScheduleManager(sm);
}

void View::slotActionDestroyed(QObject *o)
{
    //debugPlan<<o->name();
    m_scheduleActions.remove(static_cast<QAction*>(o));
}

void View::slotPlugScheduleActions()
{
    ScheduleManager *current = currentScheduleManager();
    unplugActionList(QStringLiteral("view_schedule_list"));
    const QMap<QAction*, ScheduleManager*> map = m_scheduleActions; // clazy:exclude=qmap-with-pointer-key
    QMap<QAction*, ScheduleManager*>::const_iterator it;
    for (it = map.constBegin(); it != map.constEnd(); ++it) {
        m_scheduleActionGroup->removeAction(it.key());
        delete it.key();
    }
    m_scheduleActions.clear();
    QAction *ca = nullptr;
    const QList<ScheduleManager*> managers = getProject().allScheduleManagers();
    for (ScheduleManager *sm : managers) {
        QAction *act = addScheduleAction(sm);
        if (sm == current) {
            ca = act;
        }
    }
    plugActionList(QStringLiteral("view_schedule_list"), sortedActionList());
    if (ca == nullptr && m_scheduleActionGroup->actions().count() > 0) {
        ca = m_scheduleActionGroup->actions().constFirst();
    }
    if (ca) {
        ca->setChecked(true);
    }
    slotViewSchedule(ca);
}

void View::slotRemoveCommands()
{
    while (! m_undocommands.isEmpty()) {
        m_undocommands.last()->undo();
        delete m_undocommands.takeLast();
    }
}

void View::slotDefineWBS()
{
    //debugPlan;
    Project &p = getProject();
    WBSDefinitionDialog *dia = new WBSDefinitionDialog(p, p.wbsDefinition(), this);
    connect(dia, &QDialog::finished, this, &View::slotDefineWBSFinished);
    dia->open();
}

void View::slotDefineWBSFinished(int result)
{
    //debugPlan;
    WBSDefinitionDialog *dia = qobject_cast<WBSDefinitionDialog*>(sender());
    if (dia == nullptr) {
        return;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command *cmd = dia->buildCommand();
        if (cmd) {
            mainDocument()->addCommand(cmd);
        }
    }
    dia->deleteLater();
}

Calendar *View::currentCalendar()
{
    ViewBase *v = dynamic_cast<ViewBase*>(m_tab->currentWidget());
    if (v == nullptr) {
        return nullptr;
    }
    return v->currentCalendar();
}

Node *View::currentNode() const
{
    ViewBase *v = dynamic_cast<ViewBase*>(m_tab->currentWidget());
    if (v == nullptr) {
        return nullptr;
    }
    Node * task = v->currentNode();
    if (nullptr != task) {
        return task;
    }
    return &(getProject());
}

Task *View::currentTask() const
{
    ViewBase *v = dynamic_cast<ViewBase*>(m_tab->currentWidget());
    if (v == nullptr) {
        return nullptr;
    }
    Node * task = v->currentNode();
    if (task) {
        return dynamic_cast<Task*>(task);
    }
    return nullptr;
}

Resource *View::currentResource()
{
    ViewBase *v = dynamic_cast<ViewBase*>(m_tab->currentWidget());
    if (v == nullptr) {
        return nullptr;
    }
    return v->currentResource();
}

ResourceGroup *View::currentResourceGroup()
{
    ViewBase *v = dynamic_cast<ViewBase*>(m_tab->currentWidget());
    if (v == nullptr) {
        return nullptr;
    }
    return v->currentResourceGroup();
}


void View::slotOpenCurrentNode()
{
    //debugPlan;
    Node * node = currentNode();
    slotOpenNode(node);
}

void View::slotOpenNode(Node *node)
{
    //debugPlan;
    if (!node)
        return ;

    switch (node->type()) {
        case Node::Type_Project: {
                Project * project = static_cast<Project *>(node);
                MainProjectDialog *dia = new MainProjectDialog(*project, this);
                connect(dia, &MainProjectDialog::dialogFinished, this, &View::slotProjectEditFinished);
                dia->open();
                break;
            }
        default:
            break; // avoid warnings
    }
}

void View::slotProjectEditFinished(int result)
{
    MainProjectDialog *dia = qobject_cast<MainProjectDialog*>(sender());
    if (dia == nullptr) {
        return;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command * cmd = dia->buildCommand();
        if (cmd) {
            mainDocument()->addCommand(cmd);
            if (dia->updateSharedResources()) {
                int answer = QMessageBox::question(this, i18nc("ªtitle:window", "Shared Resources"), xi18nc("@info", "Use shared resources has been activated.<nl/>Shall resources be updated?"));
                if (answer == QMessageBox::Yes) {
                    slotUpdateSharedResources();
                }
            }
        }
    }
    dia->deleteLater();
}

ScheduleManager *View::currentScheduleManager() const
{
    return m_scheduleActions.value(m_scheduleActionGroup->checkedAction());
}

long View::activeScheduleId() const
{
    ScheduleManager *s = m_scheduleActions.value(m_scheduleActionGroup->checkedAction());
    return s == nullptr || s->expected() == nullptr ? -1 : s->expected()->id();
}

void View::setActiveSchedule(long id)
{
    if (id != -1) {
        QMap<QAction*, ScheduleManager*>::const_iterator it = m_scheduleActions.constBegin();
        for (; it != m_scheduleActions.constEnd(); ++it) {
            int mid = it.value()->expected() == nullptr ? -1 : it.value()->expected()->id();
            if (mid == id) {
                it.key()->setChecked(true);
                slotViewSchedule(it.key()); // signal not emitted from group, so trigger it here
                break;
            }
        }
    }
}

void View::slotOpenProjectDescription()
{
    debugPlan<<koDocument()->isReadWrite();
    const ViewBase *cv = currentView();
    TaskDescriptionDialog *dia = new TaskDescriptionDialog(getProject(), this, (cv && !cv->isReadWrite()));
    connect(dia, &QDialog::finished, this, &View::slotProjectDescriptionFinished);
    dia->open();
}
void View::slotProjectDescriptionFinished(int result)
{
    TaskDescriptionDialog *dia = qobject_cast<TaskDescriptionDialog*>(sender());
    if (dia == nullptr) {
        return;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command * m = dia->buildCommand();
        if (m) {
            mainDocument() ->addCommand(m);
        }
    }
    dia->deleteLater();
}

void View::updateReadWrite(bool readwrite)
{
    m_readWrite = readwrite;
    m_viewlist->setReadWrite(readwrite);
}

MainDocument *View::mainDocument() const
{
    return (MainDocument *) koDocument();
}

KoPart *View::getKoPart() const
{
    return m_partpart;
}

void View::slotConnectNode()
{
    //debugPlan;
    /*    NodeItem *curr = ganttview->currentItem();
        if (curr) {
            debugPlan<<"node="<<curr->getNode().name();
        }*/
}

QMenu * View::popupMenu(const QString& name)
{
    //debugPlan;
    if (factory()) {
        return ((QMenu*) factory() ->container(name, this));
    }
    debugPlan<<"No factory";
    return nullptr;
}

void View::slotUpdate()
{
    //debugPlan<<"calculate="<<calculate;

//    m_updateResourceview = true;
    m_updatePertEditor = true;
    updateView(m_tab->currentWidget());
}

void View::slotGuiActivated(ViewBase *view, bool activate)
{
    if (activate) {
        const QList<DockWidget*> dockers = view->dockers();
        for (DockWidget *ds : dockers) {
            m_dockers.append(ds);
            ds->activate(mainWindow());
        }
        if (!m_dockers.isEmpty()) {debugPlan<<"Added dockers:"<<view<<m_dockers;}
    } else {
        if (!m_dockers.isEmpty()) {debugPlan<<"Remove dockers:"<<view<<m_dockers;}
        while (! m_dockers.isEmpty()) {
            m_dockers.takeLast()->deactivate(mainWindow());
        }
    }
}

void View::guiActivateEvent(bool activated)
{
    if (activated) {
        // plug my own actionlists, they may be gone
        slotPlugScheduleActions();
    }
    // propagate to sub-view
    ViewBase *v = dynamic_cast<ViewBase*>(m_tab->currentWidget());
    if (v) {
        v->setGuiActive(activated);
    }
}

void View::slotViewListItemRemoved(ViewListItem *item)
{
    mainDocument()->removeViewListItem(this, item);
}

void View::removeViewListItem(const ViewListItem *item)
{
    if (item == nullptr) {
        return;
    }
    ViewListItem *itm = m_viewlist->findItem(item->tag());
    if (itm == nullptr) {
        return;
    }
    m_viewlist->removeViewListItem(itm);
    return;
}

void View::slotViewListItemInserted(ViewListItem *item, ViewListItem *parent, int index)
{
    mainDocument()->insertViewListItem(this, item, parent, index);
}

void View::addViewListItem(const ViewListItem *item, const ViewListItem *parent, int index)
{
    if (item == nullptr) {
        return;
    }
    if (parent == nullptr) {
        if (item->type() != ViewListItem::ItemType_Category) {
            return;
        }
        m_viewlist->blockSignals(true);
        ViewListItem *cat = m_viewlist->addCategory(item->tag(), item->text(0));
        cat->setToolTip(0, item->toolTip(0));
        m_viewlist->blockSignals(false);
        return;
    }
    ViewListItem *cat = m_viewlist->findCategory(parent->tag());
    if (cat == nullptr) {
        return;
    }
    m_viewlist->blockSignals(true);
    createView(cat, item->viewType(), item->tag(), item->text(0), item->toolTip(0), index);
    m_viewlist->blockSignals(false);
}

void View::createReportView(const QDomDocument &doc)
{
#ifdef PLAN_USE_KREPORT
    QPointer<ViewListReportsDialog> vd = new ViewListReportsDialog(this, *m_viewlist, doc, this);
    vd->exec(); // FIXME  make non-crash
    delete vd;
#else
    Q_UNUSED(doc)
#endif
}

void View::slotOpenReportFile()
{
#ifdef PLAN_USE_KREPORT
    QFileDialog *dlg = new QFileDialog(this);
    connect(dlg, &QDialog::finished, &View::slotOpenReportFileFinished(int)));
    dlg->open();
#endif
}

void View::slotOpenReportFileFinished(int result)
{
#ifdef PLAN_USE_KREPORT
    QFileDialog *fdlg = qobject_cast<QFileDialog*>(sender());
    if (fdlg == 0 || result != QDialog::Accepted) {
        return;
    }
    QString fn = fdlg->selectedFiles().value(0);
    if (fn.isEmpty()) {
        return;
    }
    QFile file(fn);
    if (! file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        KMessageBox::error(this, xi18nc("@info", "Cannot open file:<br/><filename>%1</filename>", fn));
        return;
    }
    QDomDocument doc;
    doc.setContent(&file);
    createReportView(doc);
#else
    Q_UNUSED(result)
#endif
}

void View::slotReportDesignFinished(int /*result */)
{
#ifdef PLAN_USE_KREPORT
    if (sender()) {
        sender()->deleteLater();
    }
#endif
}

void View::slotCreateView()
{
    ViewListDialog *dlg = new ViewListDialog(this, *m_viewlist, this);
    connect(dlg, &QDialog::finished, this, &View::slotCreateViewFinished);
    dlg->open();
}

void View::slotCreateViewFinished(int)
{
    if (sender()) {
        sender()->deleteLater();
    }
}

void View::slotViewActivated(ViewListItem *item, ViewListItem *prev)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    if (prev && prev->type() == ViewListItem::ItemType_Category && m_viewlist->previousViewItem()) {
        // A view is shown anyway...
        ViewBase *v = qobject_cast<ViewBase*>(m_viewlist->previousViewItem()->view());
        if (v) {
            factory()->removeClient(v);
            v->setGuiActive(false);
        }
    } else if (prev && prev->type() == ViewListItem::ItemType_SubView) {
        ViewBase *v = qobject_cast<ViewBase*>(prev->view());
        if (v) {
            factory()->removeClient(v);
            v->setGuiActive(false);
        }
    }
    if (item && item->type() == ViewListItem::ItemType_SubView) {
        //debugPlan<<"Activate:"<<item;
        m_tab->setCurrentWidget(item->view());
        // Add sub-view specific gui
        ViewBase *v = dynamic_cast<ViewBase*>(m_tab->currentWidget());
        if (v) {
            factory()->addClient(v);
            v->setGuiActive(true);
        }
    }
    QApplication::restoreOverrideCursor();
}

QWidget *View::canvas() const
{
    return m_tab->currentWidget();//KoView::canvas();
}

KoPageLayout View::pageLayout() const
{
    return currentView()->pageLayout();
}

void View::setPageLayout(const KoPageLayout &pageLayout)
{
    currentView()->setPageLayout(pageLayout);
}

QPrintDialog *View::createPrintDialog(KoPrintJob *printJob, QWidget *parent)
{
    debugPlan<<printJob;
    KoPrintingDialog *job = dynamic_cast<KoPrintingDialog*>(printJob);
    if (! job) {
        return nullptr;
    }
    QPrintDialog *dia = KoView::createPrintDialog(job, parent);

    PrintingDialog *j = dynamic_cast<PrintingDialog*>(job);
    if (j) {
        new PrintingControlPrivate(j, dia);
    }
    return dia;
}

void View::slotCurrentChanged(int view)
{
    m_visitedViews << view;
    ViewListItem *item = m_viewlist->findItem(qobject_cast<ViewBase*>(m_tab->currentWidget()));
    m_viewlist->setCurrentItem(item);
}

void View::slotSelectDefaultView()
{
    m_tab->setCurrentIndex(qMin(m_defaultView, m_tab->count()-1));
}

void View::updateView(QWidget *)
{
}

void View::slotRenameNode(Node *node, const QString& name)
{
    //debugPlan<<name;
    if (node) {
        KUndo2MagicString s = kundo2_i18n("Modify name");
        switch(node->type()) {
            case Node::Type_Task: s = kundo2_i18n("Modify task name"); break;
            case Node::Type_Milestone: s = kundo2_i18n("Modify milestone name"); break;
            case Node::Type_Summarytask: s = kundo2_i18n("Modify summarytask name"); break;
            case Node::Type_Project: s = kundo2_i18n("Modify project name"); break;
        }
        NodeModifyNameCmd * cmd = new NodeModifyNameCmd(*node, name, s);
        mainDocument() ->addCommand(cmd);
    }
}

void View::slotPopupMenuRequested(const QString& menuname, const QPoint & pos)
{
    QMenu * menu = this->popupMenu(menuname);
    if (menu) {
        //debugPlan<<menu<<":"<<menu->actions().count();
        ViewBase *v = qobject_cast<ViewBase*>(m_tab->currentWidget());
        //debugPlan<<v<<menuname;
        QList<QAction*> lst;
        if (v) {
            lst = v->contextActionList();
            debugPlan<<lst;
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
}

void View::slotPopupMenu(const QString& menuname, const QPoint &pos, ViewListItem *item)
{
    //debugPlan<<menuname;
    m_viewlistItem = item;
    slotPopupMenuRequested(menuname, pos);
}

bool View::loadContext()
{
    Context *ctx = mainDocument()->context();
    if (ctx == nullptr || ! ctx->isLoaded()) {
        return false;
    }
    KoXmlElement n = ctx->context();
    QString cv = n.attribute("current-view");
    if (! cv.isEmpty()) {
        m_viewlist->setSelected(m_viewlist->findItem(cv));
    } else debugPlan<<"No current view";

    long id = n.attribute("current-schedule", QString::number(-1)).toLong();
    if (id != -1) {
        setActiveSchedule(id);
    } else debugPlan<<"No current schedule";

    return true;
}

void View::saveContext(QDomElement &me) const
{
    //debugPlan;
    long id = activeScheduleId();
    if (id != -1) {
        me.setAttribute(QStringLiteral("current-schedule"), QString::number((qlonglong)id));
    }
    ViewListItem *item = m_viewlist->findItem(qobject_cast<ViewBase*>(m_tab->currentWidget()));
    if (item) {
        me.setAttribute(QStringLiteral("current-view"), item->tag());
    }
    m_viewlist->save(me);
}

void View::loadWorkPackage(Project *project, const QList<QUrl> &urls)
{
    bool loaded = false;
    for (const QUrl &url : urls) {
        loaded |= mainDocument()->loadWorkPackage(*project, url);
    }
    if (loaded) {
        slotWorkPackageLoaded();
    }
}

void View::setLabel(ScheduleManager *sm)
{
    //debugPlan;
    Schedule *s = sm == nullptr ? nullptr : sm->expected();
    if (s && !s->isDeleted() && s->isScheduled()) {
        m_estlabel->setText(sm->name());
        return;
    }
    m_estlabel->setText(xi18nc("@info:status", "Not scheduled"));
}

void View::slotWorkPackageLoaded()
{
    debugPlan<<mainDocument()->workPackages();
    addStatusBarItem(m_workPackageButton, 0, true);
    Q_EMIT workPackagesAvailable(true);
}

void View::openWorkPackageMergeDialog()
{
    WorkPackageMergeDialog *dlg = new WorkPackageMergeDialog(&getProject(), mainDocument()->workPackages(), this);
    connect(dlg, &QDialog::finished, this, &View::workPackageMergeDialogFinished);
    connect(dlg, SIGNAL(terminateWorkPackage(const KPlato::Package*)), mainDocument(), SLOT(terminateWorkPackage(const KPlato::Package*)));
    connect(dlg, &WorkPackageMergeDialog::executeCommand, koDocument(), &KoDocument::addCommand);
    dlg->open();
    removeStatusBarItem(m_workPackageButton);
    Q_EMIT workPackagesAvailable(false);
}

void View::workPackageMergeDialogFinished(int result)
{
    debugPlanWp<<"result:"<<result<<"sender:"<<sender();
    WorkPackageMergeDialog *dlg = qobject_cast<WorkPackageMergeDialog*>(sender());
    Q_ASSERT(dlg);
    if (!mainDocument()->workPackages().isEmpty()) {
        slotWorkPackageLoaded();
    }
    if (dlg) {
        dlg->deleteLater();
    }
}

void View::slotPublishWorkpackages(const QList<Node*> &nodes, Resource *resource, bool mailTo)
{
    debugPlanWp<<resource<<nodes;
    if (resource == nullptr) {
        warnPlan<<"No resource, we don't handle node->leader() yet";
        return;
    }
    QList<QUrl> attachURLs = mainDocument()->publishWorkpackages(nodes, resource, activeScheduleId());
    if (!mainDocument()->errorMessage().isEmpty()) {
         KMessageBox::error(nullptr, mainDocument()->errorMessage());
         return;
    }
    if (mailTo) {
        QString body;
        for (const auto n : nodes) {
            body += n->name() + QLatin1Char('\n');
        }

        debugPlanWp<<attachURLs;
        QString to = resource->name() + QStringLiteral(" <") + resource->email() + QLatin1Char('>');
        QString subject = i18n("Work Package for project: %1", getProject().name());

        auto job = new KEMailClientLauncherJob();
        job->setTo(QStringList()<<to);
        job->setSubject(subject);
        job->setBody(body);
        job->setAttachments(attachURLs);
        job->setUiDelegate(new KDialogJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, this));
        job->start();
    }
}

void View::slotCurrencyConfig()
{
    LocaleConfigMoneyDialog *dlg = new LocaleConfigMoneyDialog(getProject().locale(), this);
    connect(dlg, &QDialog::finished, this, &View::slotCurrencyConfigFinished);
    dlg->open();
}

void View::slotCurrencyConfigFinished(int result)
{
    LocaleConfigMoneyDialog *dlg = qobject_cast<LocaleConfigMoneyDialog*>(sender());
    if (dlg == nullptr) {
        return;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command *c = dlg->buildCommand(getProject());
        if (c) {
            mainDocument()->addCommand(c);
        }
    }
    dlg->deleteLater();
}

void View::saveTaskModule(const QUrl &url, Project *project)
{
    // NOTE: workaround: KoResourcePaths::saveLocation(QStringLiteral("calligraplan_taskmodules"); does not work
    const QString dir = KoResourcePaths::saveLocation("appdata", QStringLiteral("taskmodules/"));
    debugPlan<<"dir="<<dir;
    if (! dir.isEmpty()) {
        Part *part = new Part(this);
        MainDocument *doc = new MainDocument(part);
        part->setDocument(doc);
        doc->disconnect(); // doc shall not handle feedback from openUrl()
        doc->setAutoSave(0); //disable
        doc->insertProject(*project, nullptr, nullptr); // FIXME: destroys project, find better way
        doc->getProject().setName(project->name());
        doc->getProject().setLeader(project->leader());
        doc->getProject().setDescription(project->description());
        doc->saveNativeFormat(dir + url.fileName());
        part->deleteLater(); // also deletes document
        debugPlan<<dir + url.fileName();
    } else {
        debugPlan<<"Could not find a location";
    }
}

void View::removeTaskModule(const QUrl &url)
{
    debugPlan<<url;
}

QString View::standardTaskStatusReport() const
{
    QString s;
#ifdef PLAN_USE_KREPORT
    s = QString::fromLatin1(
        "<planreportdefinition version=\"1.0\" mime=\"application/x-vnd.kde.plan.report.definition\" editor=\"Plan\" >"
        "<data-source select-from=\"taskstatus\" ></data-source>"
        "<report:content xmlns:report=\"http://kexi-project.org/report/2.0\" xmlns:fo=\"urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0\" xmlns:svg=\"urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0\" >"
        "<report:title>%1</report:title>"
        "<report:script report:script-interpreter=\"javascript\" ></report:script>"
        "<report:grid report:grid-divisions=\"4\" report:grid-snap=\"1\" report:page-unit=\"cm\" report:grid-visible=\"1\" />"
        "<report:page-style report:print-orientation=\"portrait\" fo:margin-bottom=\"1cm\" fo:margin-top=\"1cm\" fo:margin-left=\"1cm\" fo:margin-right=\"1cm\" report:page-size=\"A4\" >predefined</report:page-style>"
        "<report:body>"
        "<report:section svg:height=\"1.75cm\" fo:background-color=\"#ffffff\" report:section-type=\"header-page-any\" >"
        "<report:field report:name=\"field16\" report:horizontal-align=\"left\" report:item-data-source=\"#project.manager\" svg:x=\"13cm\" svg:width=\"5.9714cm\" svg:y=\"0.4cm\" report:vertical-align=\"bottom\" svg:height=\"0.6cm\" report:z-index=\"0\" >"
        "<report:text-style fo:font-weight=\"bold\" fo:letter-spacing=\"0%\" style:letter-kerning=\"true\" fo:font-size=\"10\" fo:foreground-color=\"#000000\" fo:font-family=\"DejaVu Sans\" fo:background-color=\"#ffffff\" fo:background-opacity=\"100%\" />"
        "<report:line-style report:line-style=\"nopen\" report:line-weight=\"1\" report:line-color=\"#000000\" />"
        "</report:field>"
        "<report:label report:name=\"label16\" report:horizontal-align=\"left\" svg:x=\"13cm\" svg:width=\"5.9714cm\" svg:y=\"0cm\" report:caption=\"%2\" report:vertical-align=\"center\" svg:height=\"0.4cm\" report:z-index=\"1\" >"
        "<report:text-style fo:letter-spacing=\"0%\" style:letter-kerning=\"true\" fo:font-size=\"8\" fo:foreground-color=\"#000000\" fo:font-family=\"DejaVu Sans\" fo:font-style=\"italic\" fo:background-color=\"#ffffff\" fo:background-opacity=\"100%\" />"
        "<report:line-style report:line-style=\"nopen\" report:line-weight=\"1\" report:line-color=\"#000000\" />"
        "</report:label>"
        "<report:field report:name=\"field17\" report:horizontal-align=\"left\" report:item-data-source=\"#project.name\" svg:x=\"0cm\" svg:width=\"13cm\" svg:y=\"0.4cm\" report:vertical-align=\"bottom\" svg:height=\"0.6cm\" report:z-index=\"1\" >"
        "<report:text-style fo:font-weight=\"bold\" fo:letter-spacing=\"0%\" style:letter-kerning=\"true\" fo:font-size=\"10\" fo:foreground-color=\"#000000\" fo:font-family=\"DejaVu Sans\" fo:background-color=\"#ffffff\" fo:background-opacity=\"0%\" />"
        "<report:line-style report:line-style=\"nopen\" report:line-weight=\"1\" report:line-color=\"#000000\" />"
        "</report:field>"
        "<report:label report:name=\"label18\" report:horizontal-align=\"left\" svg:x=\"0cm\" svg:width=\"13cm\" svg:y=\"0cm\" report:caption=\"%3\" report:vertical-align=\"center\" svg:height=\"0.4cm\" report:z-index=\"0\" >"
        "<report:text-style fo:letter-spacing=\"0%\" style:letter-kerning=\"true\" fo:font-size=\"8\" fo:foreground-color=\"#000000\" fo:font-family=\"DejaVu Sans\" fo:font-style=\"italic\" fo:background-color=\"#ffffff\" fo:background-opacity=\"100%\" />"
        "<report:line-style report:line-style=\"nopen\" report:line-weight=\"1\" report:line-color=\"#000000\" />"
        "</report:label>"
        "<report:line report:name=\"line15\" svg:y1=\"1.2229cm\" svg:x1=\"0cm\" svg:y2=\"1.2229cm\" svg:x2=\"18.9715cm\" report:z-index=\"0\" >"
        "<report:line-style report:line-style=\"solid\" report:line-weight=\"1\" report:line-color=\"#000000\" />"
        "</report:line>"
        "</report:section>"
        "<report:section svg:height=\"1.50cm\" fo:background-color=\"#ffffff\" report:section-type=\"header-report\" >"
        "<report:label report:name=\"label17\" report:horizontal-align=\"left\" svg:x=\"0cm\" svg:width=\"18.97cm\" svg:y=\"0cm\" report:caption=\"%4\" report:vertical-align=\"center\" svg:height=\"1.25cm\" report:z-index=\"0\" >"
        "<report:text-style fo:letter-spacing=\"0%\" style:letter-kerning=\"true\" fo:font-size=\"10\" fo:foreground-color=\"#000000\" fo:font-family=\"DejaVu Sans\" fo:background-color=\"#ffffff\" fo:background-opacity=\"100%\" />"
        "<report:line-style report:line-style=\"nopen\" report:line-weight=\"1\" report:line-color=\"#000000\" />"
        "</report:label>"
        "</report:section>"
        "<report:section svg:height=\"2.50cm\" fo:background-color=\"#ffffff\" report:section-type=\"footer-page-any\" >"
        "<report:field report:name=\"field10\" report:horizontal-align=\"right\" report:item-data-source=\"=constants.PageNumber()\" svg:x=\"6.75cm\" svg:width=\"0.75cm\" svg:y=\"0.25cm\" report:vertical-align=\"center\" svg:height=\"0.75cm\" report:z-index=\"0\" >"
        "<report:text-style fo:letter-spacing=\"0%\" style:letter-kerning=\"true\" fo:font-size=\"8\" fo:foreground-color=\"#000000\" fo:font-family=\"DejaVu Sans\" fo:background-color=\"#ffffff\" fo:background-opacity=\"100%\" />"
        "<report:line-style report:line-style=\"nopen\" report:line-weight=\"1\" report:line-color=\"#000000\" />"
        "</report:field>"
        "<report:field report:name=\"field11\" report:horizontal-align=\"left\" report:item-data-source=\"=constants.PageTotal()\" svg:x=\"8.25cm\" svg:width=\"3cm\" svg:y=\"0.25cm\" report:vertical-align=\"center\" svg:height=\"0.75cm\" report:z-index=\"0\" >"
        "<report:text-style fo:letter-spacing=\"0%\" style:letter-kerning=\"true\" fo:font-size=\"8\" fo:foreground-color=\"#000000\" fo:font-family=\"DejaVu Sans\" fo:background-color=\"#ffffff\" fo:background-opacity=\"100%\" />"
        "<report:line-style report:line-style=\"nopen\" report:line-weight=\"1\" report:line-color=\"#000000\" />"
        "</report:field>"
        "<report:label report:name=\"label12\" report:horizontal-align=\"center\" svg:x=\"7.5cm\" svg:width=\"0.75cm\" svg:y=\"0.25cm\" report:caption=\"%5\" report:vertical-align=\"center\" svg:height=\"0.75cm\" report:z-index=\"0\" >"
        "<report:text-style fo:letter-spacing=\"0%\" style:letter-kerning=\"true\" fo:font-size=\"8\" fo:foreground-color=\"#000000\" fo:font-family=\"DejaVu Sans\" fo:font-style=\"italic\" fo:background-color=\"#ffffff\" fo:background-opacity=\"100%\" />"
        "<report:line-style report:line-style=\"nopen\" report:line-weight=\"1\" report:line-color=\"#000000\" />"
        "</report:label>"
        "<report:label report:name=\"label13\" report:horizontal-align=\"right\" svg:x=\"5.75cm\" svg:width=\"1cm\" svg:y=\"0.25cm\" report:caption=\"%6\" report:vertical-align=\"center\" svg:height=\"0.75cm\" report:z-index=\"0\" >"
        "<report:text-style fo:letter-spacing=\"0%\" style:letter-kerning=\"true\" fo:font-size=\"8\" fo:foreground-color=\"#000000\" fo:font-family=\"DejaVu Sans\" fo:font-style=\"italic\" fo:background-color=\"#ffffff\" fo:background-opacity=\"100%\" />"
        "<report:line-style report:line-style=\"nopen\" report:line-weight=\"1\" report:line-color=\"#000000\" />"
        "</report:label>"
        "<report:line report:name=\"line14\" svg:y1=\"0.2195cm\" svg:x1=\"0cm\" svg:y2=\"0.2195cm\" svg:x2=\"18.9715cm\" report:z-index=\"0\" >"
        "<report:line-style report:line-style=\"solid\" report:line-weight=\"1\" report:line-color=\"#000000\" />"
        "</report:line>"
        "</report:section>"
        "<report:detail>"
        "<report:group report:group-sort=\"ascending\" report:group-column=\"Parent\" >"
        "<report:section svg:height=\"2.50cm\" fo:background-color=\"#ffffff\" report:section-type=\"group-header\" >"
        "<report:label report:name=\"label6\" report:horizontal-align=\"left\" svg:x=\"0.5cm\" svg:width=\"3.75cm\" svg:y=\"1.75cm\" report:caption=\"%7\" report:vertical-align=\"center\" svg:height=\"0.75cm\" report:z-index=\"0\" >"
        "<report:text-style fo:letter-spacing=\"0%\" style:letter-kerning=\"true\" fo:font-size=\"8\" fo:foreground-color=\"#000000\" fo:font-family=\"DejaVu Sans\" fo:background-color=\"#ffffff\" fo:background-opacity=\"100%\" />"
        "<report:line-style report:line-style=\"nopen\" report:line-weight=\"1\" report:line-color=\"#000000\" />"
        "</report:label>"
        "<report:field report:name=\"field8\" report:horizontal-align=\"left\" report:item-data-source=\"Parent\" svg:x=\"0.5cm\" svg:width=\"8cm\" svg:y=\"1cm\" report:vertical-align=\"center\" svg:height=\"0.689cm\" report:z-index=\"0\" >"
        "<report:text-style fo:font-weight=\"bold\" fo:letter-spacing=\"0%\" style:letter-kerning=\"true\" fo:font-size=\"8\" fo:foreground-color=\"#000000\" fo:font-family=\"DejaVu Sans\" fo:background-color=\"#ffffff\" fo:background-opacity=\"100%\" />"
        "<report:line-style report:line-style=\"nopen\" report:line-weight=\"1\" report:line-color=\"#000000\" />"
        "</report:field>"
        "<report:label report:name=\"label8\" report:horizontal-align=\"center\" svg:x=\"4.25cm\" svg:width=\"4.25cm\" svg:y=\"1.75cm\" report:caption=\"%8\" report:vertical-align=\"center\" svg:height=\"0.75cm\" report:z-index=\"0\" >"
        "<report:text-style fo:letter-spacing=\"0%\" style:letter-kerning=\"true\" fo:font-size=\"8\" fo:foreground-color=\"#000000\" fo:font-family=\"DejaVu Sans\" fo:background-color=\"#ffffff\" fo:background-opacity=\"100%\" />"
        "<report:line-style report:line-style=\"nopen\" report:line-weight=\"1\" report:line-color=\"#000000\" />"
        "</report:label>"
        "</report:section>"
        "</report:group>"
        "<report:section svg:height=\"0.50cm\" fo:background-color=\"#ffffff\" report:section-type=\"detail\" >"
        "<report:field report:name=\"field7\" report:horizontal-align=\"left\" report:item-data-source=\"NodeName\" svg:x=\"0.5cm\" svg:width=\"3.75cm\" svg:y=\"0cm\" report:vertical-align=\"center\" svg:height=\"0.5cm\" report:z-index=\"0\" >"
        "<report:text-style fo:letter-spacing=\"0%\" style:letter-kerning=\"true\" fo:font-size=\"8\" fo:foreground-color=\"#000000\" fo:font-family=\"DejaVu Sans\" fo:background-color=\"#ffffff\" fo:background-opacity=\"100%\" />"
        "<report:line-style report:line-style=\"nopen\" report:line-weight=\"1\" report:line-color=\"#000000\" />"
        "</report:field>"
        "<report:field report:name=\"field9\" report:horizontal-align=\"center\" report:item-data-source=\"NodeCompleted\" svg:x=\"4.25cm\" svg:width=\"4.25cm\" svg:y=\"0cm\" report:vertical-align=\"center\" svg:height=\"0.5cm\" report:z-index=\"0\" >"
        "<report:text-style fo:letter-spacing=\"0%\" style:letter-kerning=\"true\" fo:font-size=\"8\" fo:foreground-color=\"#000000\" fo:font-family=\"DejaVu Sans\" fo:background-color=\"#ffffff\" fo:background-opacity=\"100%\" />"
        "<report:line-style report:line-style=\"nopen\" report:line-weight=\"1\" report:line-color=\"#000000\" />"
        "</report:field>"
        "</report:section>"
        "</report:detail>"
        "</report:body>"
        "</report:content>"
        "</planreportdefinition>")
        .arg(
            i18n("Report"),
            i18nc("Project manager", "Manager:"),
            i18n("Project:"),
            i18n("Task Status Report"),
            i18nc("As in: Page 1 of 2", "of"),
            i18n("Page"),
            i18nc("Task name", "Name"),
            i18nc("Task completion", "Completion (%)")
        );
#endif
    return s;
}
