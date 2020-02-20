/* This file is part of the KDE project
  Copyright (C) 1998, 1999, 2000 Torben Weis <weis@kde.org>
  Copyright (C) 2002 - 2011 Dag Andersen <danders@get2net.dk>
  Copyright (C) 2012 Dag Andersen <danders@get2net.dk>
  Copyright (C) 2019 Dag Andersen <danders@get2net.dk>
  
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
* Boston, MA 02110-1301, USA.
*/

// clazy:excludeall=qstring-arg
#include "kptview.h"

#include <kmessagebox.h>
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

#include <kactioncollection.h>
#include <kactionmenu.h>
#include <kstandardaction.h>
#include <ktoolbar.h>
#include <kxmlguifactory.h>
#include <ktoggleaction.h>
#include <ktoolinvocation.h>
#include <krun.h>
#include <khelpclient.h>

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
#include "kptloadsharedprojectsdialog.h"
#include "kpthtmlview.h"
#include "about/aboutpage.h"
#include "kptlocaleconfigmoneydialog.h"
#include "kptflatproxymodel.h"
#include "kpttaskstatusmodel.h"
#include "kptworkpackagemergedialog.h"
#include "Help.h"

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
        : KoView(part, doc, parent),
        m_currentEstimateType(Estimate::Use_Expected),
        m_scheduleActionGroup(new QActionGroup(this)),
        m_readWrite(false),
        m_defaultView(1),
        m_partpart (part)
{
    //debugPlan;
    new Help(KPlatoSettings::contextPath(), KPlatoSettings::contextLanguage());

    doc->registerView(this);

    setComponentName(Factory::global().componentName(), Factory::global().componentDisplayName());
    if (!doc->isReadWrite())
        setXMLFile("calligraplan_readonly.rc");
    else
        setXMLFile("calligraplan.rc");

//     new ViewAdaptor(this);

    m_sp = new QSplitter(this);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(m_sp);

    ViewListDocker *docker = 0;
    if (mainWindow() == 0) {
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
    createIntroductionView();

    // The menu items
    // ------ File
    actionCreateTemplate = new QAction(koIcon("document-save-as-template"), i18n("Create Project Template..."), this);
    actionCollection()->addAction("file_createtemplate", actionCreateTemplate);
    connect(actionCreateTemplate, SIGNAL(triggered(bool)), SLOT(slotCreateTemplate()));

    actionCreateNewProject = new QAction(i18n("Create New Project..."), this);
    actionCollection()->addAction("file_createnewproject", actionCreateNewProject);
    connect(actionCreateNewProject, &QAction::triggered, this, &View::slotCreateNewProject);

    // ------ Edit
    actionCut = actionCollection()->addAction(KStandardAction::Cut,  "edit_cut", this, SLOT(slotEditCut()));
    actionCopy = actionCollection()->addAction(KStandardAction::Copy,  "edit_copy", this, SLOT(slotEditCopy()));
    actionPaste = actionCollection()->addAction(KStandardAction::Paste,  "edit_paste", this, SLOT(slotEditPaste()));

    // ------ View
    actionCollection()->addAction(KStandardAction::Redisplay, "view_refresh" , this, SLOT(slotRefreshView()));

    actionViewSelector  = new KToggleAction(i18n("Show Selector"), this);
    actionCollection()->addAction("view_show_selector", actionViewSelector);
    connect(actionViewSelector, &QAction::triggered, this, &View::slotViewSelector);

    // ------ Insert

    // ------ Project
    actionEditMainProject  = new QAction(koIcon("view-time-schedule-edit"), i18n("Edit..."), this);
    actionCollection()->addAction("project_edit", actionEditMainProject);
    connect(actionEditMainProject, &QAction::triggered, this, &View::slotProjectEdit);

    actionEditStandardWorktime  = new QAction(koIcon("configure"), i18n("Define Estimate Conversions..."), this);
    actionCollection()->addAction("project_worktime", actionEditStandardWorktime);
    connect(actionEditStandardWorktime, &QAction::triggered, this, &View::slotProjectWorktime);

    actionDefineWBS  = new QAction(koIcon("configure"), i18n("Define WBS Pattern..."), this);
    actionCollection()->addAction("tools_define_wbs", actionDefineWBS);
    connect(actionDefineWBS, &QAction::triggered, this, &View::slotDefineWBS);

    actionCurrencyConfig  = new QAction(koIcon("configure"), i18n("Define Currency..."), this);
    actionCollection()->addAction("config_currency", actionCurrencyConfig);
    connect(actionCurrencyConfig, &QAction::triggered, this, &View::slotCurrencyConfig);

    QAction *actionProjectDescription = new QAction(koIcon("document-edit"), i18n("Edit Description..."), this);
    actionCollection()->addAction("edit_project_description", actionProjectDescription);
    connect(actionProjectDescription, &QAction::triggered, this, &View::slotOpenProjectDescription);

    // ------ Tools
    actionInsertFile  = new QAction(koIcon("document-import"), i18n("Insert Project File..."), this);
    actionCollection()->addAction("insert_file", actionInsertFile);
    connect(actionInsertFile, &QAction::triggered, this, &View::slotInsertFile);

    actionLoadSharedProjects  = new QAction(koIcon("document-import"), i18n("Load Shared Projects..."), this);
    actionCollection()->addAction("load_shared_projects", actionLoadSharedProjects);
    connect(actionLoadSharedProjects, &QAction::triggered, this, &View::slotLoadSharedProjects);

#ifdef PLAN_USE_KREPORT
    actionOpenReportFile  = new QAction(koIcon("document-open"), i18n("Open Report Definition File..."), this);
    actionCollection()->addAction("reportdesigner_open_file", actionOpenReportFile);
    connect(actionOpenReportFile, QAction::triggered, this, &View::slotOpenReportFile);
#endif

    // ------ Help
    actionIntroduction  = new QAction(koIcon("dialog-information"), i18n("Introduction to Plan"), this);
    actionCollection()->addAction("plan_introduction", actionIntroduction);
    connect(actionIntroduction, &QAction::triggered, this, &View::slotIntroduction);

    // ------ Popup
    actionOpenNode  = new QAction(koIcon("document-edit"), i18n("Edit..."), this);
    actionCollection()->addAction("node_properties", actionOpenNode);
    connect(actionOpenNode, &QAction::triggered, this, &View::slotOpenCurrentNode);
    actionTaskProgress  = new QAction(koIcon("document-edit"), i18n("Progress..."), this);
    actionCollection()->addAction("task_progress", actionTaskProgress);
    connect(actionTaskProgress, &QAction::triggered, this, &View::slotTaskProgress);
    actionDeleteTask  = new QAction(koIcon("edit-delete"), i18n("Delete Task"), this);
    actionCollection()->addAction("delete_task", actionDeleteTask);
    connect(actionDeleteTask, &QAction::triggered, this, &View::slotDeleteCurrentTask);
    actionTaskDescription  = new QAction(koIcon("document-edit"), i18n("Description..."), this);
    actionCollection()->addAction("task_description", actionTaskDescription);
    connect(actionTaskDescription, &QAction::triggered, this, &View::slotTaskDescription);
    actionDocuments  = new QAction(koIcon("document-edit"), i18n("Documents..."), this);
    actionCollection()->addAction("task_documents", actionDocuments);
    connect(actionDocuments, &QAction::triggered, this, &View::slotDocuments);
    actionIndentTask = new QAction(koIcon("format-indent-more"), i18n("Indent Task"), this);
    actionCollection()->addAction("indent_task", actionIndentTask);
    connect(actionIndentTask, &QAction::triggered, this, &View::slotIndentTask);
    actionUnindentTask= new QAction(koIcon("format-indent-less"), i18n("Unindent Task"), this);
    actionCollection()->addAction("unindent_task", actionUnindentTask);
    connect(actionUnindentTask, &QAction::triggered, this, &View::slotUnindentTask);
    actionMoveTaskUp = new QAction(koIcon("arrow-up"), i18n("Move Task Up"), this);
    actionCollection()->addAction("move_task_up", actionMoveTaskUp);
    connect(actionMoveTaskUp, &QAction::triggered, this, &View::slotMoveTaskUp);
    actionMoveTaskDown = new QAction(koIcon("arrow-down"), i18n("Move Task Down"), this);
    actionCollection()->addAction("move_task_down", actionMoveTaskDown);
    connect(actionMoveTaskDown, &QAction::triggered, this, &View::slotMoveTaskDown);

    actionEditResource  = new QAction(koIcon("document-edit"), i18n("Edit Resource..."), this);
    actionCollection()->addAction("edit_resource", actionEditResource);
    connect(actionEditResource, &QAction::triggered, this, &View::slotEditCurrentResource);

    actionEditRelation  = new QAction(koIcon("document-edit"), i18n("Edit Dependency..."), this);
    actionCollection()->addAction("edit_dependency", actionEditRelation);
    connect(actionEditRelation, &QAction::triggered, this, &View::slotModifyCurrentRelation);
    actionDeleteRelation  = new QAction(koIcon("edit-delete"), i18n("Delete Dependency"), this);
    actionCollection()->addAction("delete_dependency", actionDeleteRelation);
    connect(actionDeleteRelation, &QAction::triggered, this, &View::slotDeleteRelation);

    // Viewlist popup
    connect(m_viewlist, &ViewListWidget::createView, this, &View::slotCreateView);

    m_workPackageButton = new QToolButton(this);
    m_workPackageButton->hide();
    m_workPackageButton->setIcon(koIcon("application-x-vnd.kde.plan.work"));
    m_workPackageButton->setText(i18n("Work Packages..."));
    m_workPackageButton->setToolTip(i18nc("@info:tooltip", "Work packages available"));
    m_workPackageButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    connect(m_workPackageButton, &QToolButton::clicked, this, &View::openWorkPackageMergeDialog);
    m_estlabel = new QLabel("", 0);
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


    connect(getPart(), &MainDocument::workPackageLoaded, this, &View::slotWorkPackageLoaded);

    // views take time for large projects
    QTimer::singleShot(0, this, &View::initiateViews);

    const QList<KPluginFactory *> pluginFactories =
        KoPluginLoader::instantiatePluginFactories(QStringLiteral("calligraplan/extensions"));

    foreach (KPluginFactory* factory, pluginFactories) {
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
        connect(m_viewlist, &ViewListWidget::modified, getPart(), &MainDocument::slotViewlistModified);
        connect(getPart(), &MainDocument::viewlistModified, docker, &ViewListDocker::updateWindowTitle);
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
        emit currentScheduleManagerChanged(0);
        getPart()->createNewProject();
        slotOpenNode(&getProject());
    }
}

void View::slotCreateTemplate()
{
    debugPlan;
    KoFileDialog dlg(nullptr, KoFileDialog::SaveFile, "Create Template");
    dlg.setNameFilters(QStringList()<<"Plan Template (*.plant)");
    QString file = dlg.filename();
    if (!file.isEmpty()) {
        QTemporaryDir dir;
        dir.setAutoRemove(false);
        QString tmpfile = dir.path() + '/' + QUrl(file).fileName();
        tmpfile.replace(".plant", ".plan");
        Part *part = new Part(this);
        MainDocument *doc = new MainDocument(part);
        part->setDocument(doc);
        doc->disconnect(); // doc shall not handle feedback from openUrl()
        doc->setAutoSave(0); //disable
        bool ok = koDocument()->exportDocument(QUrl::fromUserInput("file:/" + tmpfile));
        ok &= doc->loadNativeFormat(tmpfile);
        if (ok) {
            // strip unused data
            Project *project = doc->project();
            for (ScheduleManager *sm : project->scheduleManagers()) {
                DeleteScheduleManagerCmd c(*project, sm);
                c.redo();
            }
        }
        ok &= doc->saveNativeFormat(file);

        part->deleteLater();
    }
}

void View::createViews()
{
    Context *ctx = getPart()->context();
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
                if (e.tagName() != "category") {
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
                    if (e1.tagName() != "view") {
                        continue;
                    }
                    ViewBase *v = 0;
                    QString type = e1.attribute("viewtype");
                    QString tag = e1.attribute("tag");
                    QString name = e1.attribute("name");
                    QString tip = e1.attribute("tooltip");
                    v = createView(cat, type, tag, name, tip);
                    //KoXmlNode settings = e1.namedItem("settings "); ????
                    KoXmlNode settings = e1.firstChild();
                    for (; ! settings.isNull(); settings = settings.nextSibling()) {
                        if (settings.nodeName() == "settings") {
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
    } else {
        debugPlan<<"Default";
        ViewBase *v = 0;
        ViewListItem *cat;
        QString ct = "Editors";
        cat = m_viewlist->addCategory(ct, defaultCategoryInfo(ct).name);

        createCalendarEditor(cat, "CalendarEditor", QString(), TIP_USE_DEFAULT_TEXT);

        createAccountsEditor(cat, "AccountsEditor", QString(), TIP_USE_DEFAULT_TEXT);

        v = createResourceGroupEditor(cat, "ResourceGroupEditor", QString(), TIP_USE_DEFAULT_TEXT);

        v = createResourceEditor(cat, "ResourceEditor", QString(), TIP_USE_DEFAULT_TEXT);

        v = createTaskEditor(cat, "TaskEditor", QString(), TIP_USE_DEFAULT_TEXT);
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

        v = createTaskEditor(cat, "TaskConstraintEditor", i18n("Task Constraints"), i18n("Edit task scheduling constraints"));
        v->showColumns(QList<int>() << NodeModel::NodeName
                                    << NodeModel::NodeType
                                    << NodeModel::NodePriority
                                    << NodeModel::NodeConstraint
                                    << NodeModel::NodeConstraintStart
                                    << NodeModel::NodeConstraintEnd
                                    << NodeModel::NodeDescription
                      );

        v = createTaskEditor(cat, "TaskCostEditor", i18n("Task Cost"), i18n("Edit task cost"));
        v->showColumns(QList<int>() << NodeModel::NodeName
                                    << NodeModel::NodeType
                                    << NodeModel::NodeRunningAccount
                                    << NodeModel::NodeStartupAccount
                                    << NodeModel::NodeStartupCost
                                    << NodeModel::NodeShutdownAccount
                                    << NodeModel::NodeShutdownCost
                                    << NodeModel::NodeDescription
                      );

        createDependencyEditor(cat, "DependencyEditor", QString(), TIP_USE_DEFAULT_TEXT);

        // Do not show by default
        // createPertEditor(cat, "PertEditor", QString(), TIP_USE_DEFAULT_TEXT);

        createScheduleHandler(cat, "ScheduleHandlerView", QString(), TIP_USE_DEFAULT_TEXT);

        ct = "Views";
        cat = m_viewlist->addCategory(ct, defaultCategoryInfo(ct).name);

        createGanttView(cat, "GanttView", QString(), TIP_USE_DEFAULT_TEXT);

        createMilestoneGanttView(cat, "MilestoneGanttView", QString(), TIP_USE_DEFAULT_TEXT);

        createResourceAppointmentsView(cat, "ResourceAppointmentsView", QString(), TIP_USE_DEFAULT_TEXT);

        createResourceAppointmentsGanttView(cat, "ResourceAppointmentsGanttView", QString(), TIP_USE_DEFAULT_TEXT);

        createAccountsView(cat, "AccountsView", QString(), TIP_USE_DEFAULT_TEXT);

        ct = "Execution";
        cat = m_viewlist->addCategory(ct, defaultCategoryInfo(ct).name);

        createProjectStatusView(cat, "ProjectStatusView", QString(), TIP_USE_DEFAULT_TEXT);

        createPerformanceStatusView(cat, "PerformanceStatusView", QString(), TIP_USE_DEFAULT_TEXT);

        v = createTaskStatusView(cat, "TaskStatusView", QString(), TIP_USE_DEFAULT_TEXT);

        v = createTaskView(cat, "TaskView", QString(), TIP_USE_DEFAULT_TEXT);

        v = createTaskWorkPackageView(cat, "TaskWorkPackageView", QString(), TIP_USE_DEFAULT_TEXT);

        ct = "Reports";
        cat = m_viewlist->addCategory(ct, defaultCategoryInfo(ct).name);

        createReportsGeneratorView(cat, "ReportsGeneratorView", i18n("Generate reports"), TIP_USE_DEFAULT_TEXT);

#ifdef PLAN_USE_KREPORT
        // Let user add reports explicitly, we prefer reportsgenerator now
        // A little hack to get the user started...
#if 0
        ReportView *rv = qobject_cast<ReportView*>(createReportView(cat, "ReportView", i18n("Task Status Report"), TIP_USE_DEFAULT_TEXT));
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
    ViewBase *v = 0;
    //NOTE: type is the same as classname (so if it is changed...)
    if (type == "CalendarEditor") {
        v = createCalendarEditor(cat, tag, name, tip, index);
    } else if (type == "AccountsEditor") {
        v = createAccountsEditor(cat, tag, name, tip, index);
    } else if (type == "ResourceGroupEditor") {
        v = createResourceGroupEditor(cat, tag, name, tip, index);
    } else if (type == "ResourceEditor") {
        v = createResourceEditor(cat, tag, name, tip, index);
    } else if (type == "TaskEditor") {
        v = createTaskEditor(cat, tag, name, tip, index);
    } else if (type == "DependencyEditor") {
        v = createDependencyEditor(cat, tag, name, tip, index);
    } else if (type == "PertEditor") {
        v = createPertEditor(cat, tag, name, tip, index);
    } else if (type == "ScheduleEditor") {
        v = createScheduleEditor(cat, tag, name, tip, index);
    } else if (type == "ScheduleHandlerView") {
        v = createScheduleHandler(cat, tag, name, tip, index);
    } else if (type == "ProjectStatusView") {
        v = createProjectStatusView(cat, tag, name, tip, index);
    } else if (type == "TaskStatusView") {
        v = createTaskStatusView(cat, tag, name, tip, index);
    } else if (type == "TaskView") {
        v = createTaskView(cat, tag, name, tip, index);
    } else if (type == "TaskWorkPackageView") {
        v = createTaskWorkPackageView(cat, tag, name, tip, index);
    } else if (type == "GanttView") {
        v = createGanttView(cat, tag, name, tip, index);
    } else if (type == "MilestoneGanttView") {
        v = createMilestoneGanttView(cat, tag, name, tip, index);
    } else if (type == "ResourceAppointmentsView") {
        v = createResourceAppointmentsView(cat, tag, name, tip, index);
    } else if (type == "ResourceAppointmentsGanttView") {
        v = createResourceAppointmentsGanttView(cat, tag, name, tip, index);
    } else if (type == "AccountsView") {
        v = createAccountsView(cat, tag, name, tip, index);
    } else if (type == "PerformanceStatusView") {
        v = createPerformanceStatusView(cat, tag, name, tip, index);
    } else if (type == "ReportsGeneratorView") {
        v = createReportsGeneratorView(cat, tag, name, tip, index);
    } else if (type == "ReportView") {
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
    if (type == "CalendarEditor") {
        vi.name = i18n("Work & Vacation");
        vi.tip = xi18nc("@info:tooltip", "Edit working- and vacation days for resources");
    } else if (type == "AccountsEditor") {
        vi.name = i18n("Cost Breakdown Structure");
        vi.tip = xi18nc("@info:tooltip", "Edit cost breakdown structure.");
    } else if (type == "ResourceGroupEditor") {
        vi.name = i18n("Resource Breakdown Structure");
        vi.tip = xi18nc("@info:tooltip", "Edit resource breakdown structure");
    } else if (type == "ResourceEditor") {
        vi.name = i18n("Resources");
        vi.tip = xi18nc("@info:tooltip", "Edit resources");
    } else if (type == "TaskEditor") {
        vi.name = i18n("Tasks");
        vi.tip = xi18nc("@info:tooltip", "Edit work breakdown structure");
    } else if (type == "DependencyEditor") {
        vi.name = i18n("Dependencies (Graphic)");
        vi.tip = xi18nc("@info:tooltip", "Edit task dependencies");
    } else if (type == "PertEditor") {
        vi.name = i18n("Dependencies (List)");
        vi.tip = xi18nc("@info:tooltip", "Edit task dependencies");
    } else if (type == "ScheduleEditor") {
        // This view is not used stand-alone atm
        vi.name = i18n("Schedules");
    } else if (type == "ScheduleHandlerView") {
        vi.name = i18n("Schedules");
        vi.tip = xi18nc("@info:tooltip", "Calculate and analyze project schedules");
    } else if (type == "ProjectStatusView") {
        vi.name = i18n("Project Performance Chart");
        vi.tip = xi18nc("@info:tooltip", "View project status information");
    } else if (type == "TaskStatusView") {
        vi.name = i18n("Task Status");
        vi.tip = xi18nc("@info:tooltip", "View task progress information");
    } else if (type == "TaskView") {
        vi.name = i18n("Task Execution");
        vi.tip = xi18nc("@info:tooltip", "View task execution information");
    } else if (type == "TaskWorkPackageView") {
        vi.name = i18n("Work Package View");
        vi.tip = xi18nc("@info:tooltip", "View task work package information");
    } else if (type == "GanttView") {
        vi.name = i18n("Gantt");
        vi.tip = xi18nc("@info:tooltip", "View Gantt chart");
    } else if (type == "MilestoneGanttView") {
        vi.name = i18n("Milestone Gantt");
        vi.tip = xi18nc("@info:tooltip", "View milestone Gantt chart");
    } else if (type == "ResourceAppointmentsView") {
        vi.name = i18n("Resource Assignments");
        vi.tip = xi18nc("@info:tooltip", "View resource assignments in a table");
    } else if (type == "ResourceAppointmentsGanttView") {
        vi.name = i18n("Resource Assignments (Gantt)");
        vi.tip = xi18nc("@info:tooltip", "View resource assignments in Gantt chart");
    } else if (type == "AccountsView") {
        vi.name = i18n("Cost Breakdown");
        vi.tip = xi18nc("@info:tooltip", "View planned and actual cost");
    } else if (type == "PerformanceStatusView") {
        vi.name = i18n("Tasks Performance Chart");
        vi.tip = xi18nc("@info:tooltip", "View tasks performance status information");
    } else if (type == "ReportsGeneratorView") {
        vi.name = i18n("Reports Generator");
        vi.tip = xi18nc("@info:tooltip", "Generate reports");
    } else if (type == "ReportView") {
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
    if (type == "Editors") {
        vi.name = i18n("Editors");
    } else if (type == "Views") {
        vi.name = i18n("Views");
    } else if (type == "Execution") {
        vi.name = i18nc("Project execution views", "Execution");
    } else if (type == "Reports") {
        vi.name = i18n("Reports");
    }
    return vi;
}

void View::slotOpenUrlRequest(HtmlView *v, const QUrl &url)
{
    debugPlan<<url;
    if (url.scheme() == QLatin1String("about")) {
        if (url.url() == QLatin1String("about:close")) {
            int view = m_visitedViews.count() < 2 ? qMin(m_defaultView, m_tab->count()-1) : m_visitedViews.at(m_visitedViews.count() - 2);
            debugPlan<<"Prev:"<<view<<m_visitedViews;
            m_tab->setCurrentIndex(view);
            return;
        }
        if (url.url().startsWith(QLatin1String("about:plan"))) {
            getPart()->aboutPage().generatePage(v->htmlPart(), url);
            return;
        }
    }
    if (url.scheme() == QLatin1String("help")) {
        KHelpClient::invokeHelp("", url.fileName());
        return;
    }
    // try to open the url
    debugPlan<<url<<"is external, try to open";
    new KRun(url, mainWindow());
}

ViewBase *View::createIntroductionView()
{
    HtmlView *v = new HtmlView(getKoPart(), getPart(), m_tab);
    v->htmlPart().setJScriptEnabled(false);
    v->htmlPart().setJavaEnabled(false);
    v->htmlPart().setMetaRefreshEnabled(false);
    v->htmlPart().setPluginsEnabled(false);

    slotOpenUrlRequest(v, QUrl("about:plan/main"));

    connect(v, &HtmlView::openUrlRequest, this, &View::slotOpenUrlRequest);

    m_tab->addWidget(v);
    return v;
}

ViewBase *View::createResourceAppointmentsGanttView(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    ResourceAppointmentsGanttView *v = new ResourceAppointmentsGanttView(getKoPart(), getPart(), m_tab);
    m_tab->addWidget(v);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, v, getPart(), "", index);
    ViewInfo vi = defaultViewInfo("ResourceAppointmentsGanttView");
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == TIP_USE_DEFAULT_TEXT) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }


    connect(v, &ViewBase::guiActivated, this, &View::slotGuiActivated);

    connect(this, &View::currentScheduleManagerChanged, v, &ResourceAppointmentsGanttView::setScheduleManager);

    connect(v, &ResourceAppointmentsGanttView::requestPopupMenu, this, &View::slotPopupMenuRequested);

    v->setProject(&(getProject()));
    v->setScheduleManager(currentScheduleManager());
    v->updateReadWrite(m_readWrite);
    return v;
}


ViewBase *View::createResourceAppointmentsView(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    ResourceAppointmentsView *v = new ResourceAppointmentsView(getKoPart(), getPart(), m_tab);
    m_tab->addWidget(v);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, v, getPart(), "", index);
    ViewInfo vi = defaultViewInfo("ResourceAppointmentsView");
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == TIP_USE_DEFAULT_TEXT) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }

    connect(v, &ViewBase::guiActivated, this, &View::slotGuiActivated);

    connect(this, &View::currentScheduleManagerChanged, v, &ResourceAppointmentsView::setScheduleManager);

    connect(v, &ResourceAppointmentsView::requestPopupMenu, this, &View::slotPopupMenuRequested);

    v->setProject(&(getProject()));
    v->setScheduleManager(currentScheduleManager());
    v->updateReadWrite(m_readWrite);
    return v;
}

ViewBase *View::createResourceGroupEditor(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    ResourceGroupEditor *e = new ResourceGroupEditor(getKoPart(), getPart(), m_tab);
    e->setViewSplitMode(false);
    m_tab->addWidget(e);
    e->setProject(&(getProject()));
    
    ViewListItem *i = m_viewlist->addView(cat, tag, name, e, getPart(), "", index);
    ViewInfo vi = defaultViewInfo("ResourceGroupEditor");
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == TIP_USE_DEFAULT_TEXT) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }
    
    connect(e, &ViewBase::guiActivated, this, &View::slotGuiActivated);
    
    connect(e, &ResourceGroupEditor::deleteObjectList, this, &View::slotDeleteResourceObjects);
    
    connect(e, &ResourceGroupEditor::requestPopupMenu, this, &View::slotPopupMenuRequested);
    e->updateReadWrite(m_readWrite);
    return e;
}

ViewBase *View::createResourceEditor(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    ResourceEditor *resourceeditor = new ResourceEditor(getKoPart(), getPart(), m_tab);
    resourceeditor->setViewSplitMode(false);
    m_tab->addWidget(resourceeditor);
    resourceeditor->setProject(&(getProject()));

    ViewListItem *i = m_viewlist->addView(cat, tag, name, resourceeditor, getPart(), "", index);
    ViewInfo vi = defaultViewInfo("ResourceEditor");
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == TIP_USE_DEFAULT_TEXT) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }

    connect(resourceeditor, &ViewBase::guiActivated, this, &View::slotGuiActivated);

    connect(resourceeditor, &ResourceEditor::deleteObjectList, this, &View::slotDeleteResourceObjects);

    connect(resourceeditor, &ResourceEditor::requestPopupMenu, this, &View::slotPopupMenuRequested);
    resourceeditor->updateReadWrite(m_readWrite);
    return resourceeditor;
}

ViewBase *View::createTaskEditor(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    TaskEditor *taskeditor = new TaskEditor(getKoPart(), getPart(), m_tab);
    taskeditor->setViewSplitMode(false);
    m_tab->addWidget(taskeditor);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, taskeditor, getPart(), "", index);
    ViewInfo vi = defaultViewInfo("TaskEditor");
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == TIP_USE_DEFAULT_TEXT) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }

    taskeditor->setProject(&(getProject()));
    taskeditor->setScheduleManager(currentScheduleManager());

    connect(this, &View::currentScheduleManagerChanged, taskeditor, &TaskEditor::setScheduleManager);

    connect(taskeditor, &ViewBase::guiActivated, this, &View::slotGuiActivated);

    connect(taskeditor, &TaskEditor::addTask, this, &View::slotAddTask);
    connect(taskeditor, &TaskEditor::addMilestone, this, &View::slotAddMilestone);
    connect(taskeditor, &TaskEditor::addSubtask, this, &View::slotAddSubTask);
    connect(taskeditor, &TaskEditor::addSubMilestone, this, &View::slotAddSubMilestone);
    connect(taskeditor, &TaskEditor::deleteTaskList, this, &View::slotDeleteTaskList);
    connect(taskeditor, &TaskEditor::moveTaskUp, this, &View::slotMoveTaskUp);
    connect(taskeditor, &TaskEditor::moveTaskDown, this, &View::slotMoveTaskDown);
    connect(taskeditor, &TaskEditor::indentTask, this, &View::slotIndentTask);
    connect(taskeditor, &TaskEditor::unindentTask, this, &View::slotUnindentTask);

    connect(taskeditor, &TaskEditor::saveTaskModule, this, &View::saveTaskModule);
    connect(taskeditor, &TaskEditor::removeTaskModule, this, &View::removeTaskModule);
    connect(taskeditor, &ViewBase::openDocument, static_cast<KPlato::Part*>(m_partpart), &Part::openTaskModule);

    connect(taskeditor, &TaskEditor::requestPopupMenu, this, &View::slotPopupMenuRequested);
    connect(taskeditor, &TaskEditor::openTaskDescription, this, &View::slotOpenTaskDescription);
    taskeditor->updateReadWrite(m_readWrite);

    return taskeditor;
}

ViewBase *View::createAccountsEditor(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    AccountsEditor *ae = new AccountsEditor(getKoPart(), getPart(), m_tab);
    m_tab->addWidget(ae);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, ae, getPart(), "", index);
    ViewInfo vi = defaultViewInfo("AccountsEditor");
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == TIP_USE_DEFAULT_TEXT) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }

    ae->draw(getProject());

    connect(ae, &ViewBase::guiActivated, this, &View::slotGuiActivated);
    ae->updateReadWrite(m_readWrite);
    return ae;
}

ViewBase *View::createCalendarEditor(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    CalendarEditor *calendareditor = new CalendarEditor(getKoPart(), getPart(), m_tab);
    m_tab->addWidget(calendareditor);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, calendareditor, getPart(), "", index);
    ViewInfo vi = defaultViewInfo("CalendarEditor");
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == TIP_USE_DEFAULT_TEXT) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }

    calendareditor->draw(getProject());

    connect(calendareditor, &ViewBase::guiActivated, this, &View::slotGuiActivated);

    connect(calendareditor, &CalendarEditor::requestPopupMenu, this, &View::slotPopupMenuRequested);
    calendareditor->updateReadWrite(m_readWrite);
    return calendareditor;
}

ViewBase *View::createScheduleHandler(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    ScheduleHandlerView *handler = new ScheduleHandlerView(getKoPart(), getPart(), m_tab);
    m_tab->addWidget(handler);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, handler, getPart(), "", index);
    ViewInfo vi = defaultViewInfo("ScheduleHandlerView");
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == TIP_USE_DEFAULT_TEXT) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }

    connect(handler->scheduleEditor(), &ScheduleEditor::addScheduleManager, this, &View::slotAddScheduleManager);
    connect(handler->scheduleEditor(), &ScheduleEditor::deleteScheduleManager, this, &View::slotDeleteScheduleManager);
    connect(handler->scheduleEditor(), &ScheduleEditor::moveScheduleManager, this, &View::slotMoveScheduleManager);

    connect(handler->scheduleEditor(), &ScheduleEditor::calculateSchedule, this, &View::slotCalculateSchedule);

    connect(handler->scheduleEditor(), &ScheduleEditor::baselineSchedule, this, &View::slotBaselineSchedule);


    connect(handler, &ViewBase::guiActivated, this, &View::slotGuiActivated);

    connect(this, &View::currentScheduleManagerChanged, handler, &ScheduleHandlerView::currentScheduleManagerChanged);

    connect(handler, &ScheduleHandlerView::requestPopupMenu, this, &View::slotPopupMenuRequested);

    connect(handler, &ScheduleHandlerView::editNode, this, &View::slotOpenNode);
    connect(handler, &ScheduleHandlerView::editResource, this, &View::slotEditResource);

    handler->draw(getProject());
    handler->updateReadWrite(m_readWrite);
    return handler;
}

ScheduleEditor *View::createScheduleEditor(QWidget *parent)
{
    ScheduleEditor *scheduleeditor = new ScheduleEditor(getKoPart(), getPart(), parent);

    connect(scheduleeditor, &ScheduleEditor::addScheduleManager, this, &View::slotAddScheduleManager);
    connect(scheduleeditor, &ScheduleEditor::deleteScheduleManager, this, &View::slotDeleteScheduleManager);

    connect(scheduleeditor, &ScheduleEditor::calculateSchedule, this, &View::slotCalculateSchedule);

    connect(scheduleeditor, &ScheduleEditor::baselineSchedule, this, &View::slotBaselineSchedule);

    scheduleeditor->updateReadWrite(m_readWrite);
    return scheduleeditor;
}

ViewBase *View::createScheduleEditor(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    ScheduleEditor *scheduleeditor = new ScheduleEditor(getKoPart(), getPart(), m_tab);
    m_tab->addWidget(scheduleeditor);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, scheduleeditor, getPart(), "", index);
    ViewInfo vi = defaultViewInfo("ScheduleEditor");
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == TIP_USE_DEFAULT_TEXT) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }

    scheduleeditor->setProject(&(getProject()));

    connect(scheduleeditor, &ViewBase::guiActivated, this, &View::slotGuiActivated);

    connect(scheduleeditor, &ScheduleEditor::addScheduleManager, this, &View::slotAddScheduleManager);

    connect(scheduleeditor, &ScheduleEditor::deleteScheduleManager, this, &View::slotDeleteScheduleManager);

    connect(scheduleeditor, &ScheduleEditor::calculateSchedule, this, &View::slotCalculateSchedule);

    connect(scheduleeditor, &ScheduleEditor::baselineSchedule, this, &View::slotBaselineSchedule);

    scheduleeditor->updateReadWrite(m_readWrite);
    return scheduleeditor;
}


ViewBase *View::createDependencyEditor(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    DependencyEditor *editor = new DependencyEditor(getKoPart(), getPart(), m_tab);
    m_tab->addWidget(editor);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, editor, getPart(), "", index);
    ViewInfo vi = defaultViewInfo("DependencyEditor");
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == TIP_USE_DEFAULT_TEXT) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }

    editor->draw(getProject());

    connect(editor, &ViewBase::guiActivated, this, &View::slotGuiActivated);

    connect(editor, &DependencyEditor::addRelation, this, &View::slotAddRelation);
    connect(editor, &DependencyEditor::modifyRelation, this, &View::slotModifyRelation);
    connect(editor, &DependencyEditor::editRelation, this, &View::slotEditRelation);

    connect(editor, &DependencyEditor::editNode, this, &View::slotOpenNode);
    connect(editor, &DependencyEditor::addTask, this, &View::slotAddTask);
    connect(editor, &DependencyEditor::addMilestone, this, &View::slotAddMilestone);
    connect(editor, &DependencyEditor::addSubMilestone, this, &View::slotAddSubMilestone);
    connect(editor, &DependencyEditor::addSubtask, this, &View::slotAddSubTask);
    connect(editor, &DependencyEditor::deleteTaskList, this, &View::slotDeleteTaskList);

    connect(this, &View::currentScheduleManagerChanged, editor, &DependencyEditor::setScheduleManager);

    connect(editor, &DependencyEditor::requestPopupMenu, this, &View::slotPopupMenuRequested);
    editor->updateReadWrite(m_readWrite);
    editor->setScheduleManager(currentScheduleManager());
    return editor;
}

ViewBase *View::createPertEditor(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    PertEditor *perteditor = new PertEditor(getKoPart(), getPart(), m_tab);
    m_tab->addWidget(perteditor);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, perteditor, getPart(), "", index);
    ViewInfo vi = defaultViewInfo("PertEditor");
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == TIP_USE_DEFAULT_TEXT) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }

    perteditor->draw(getProject());

    connect(perteditor, &ViewBase::guiActivated, this, &View::slotGuiActivated);
    m_updatePertEditor = true;
    perteditor->updateReadWrite(m_readWrite);
    return perteditor;
}

ViewBase *View::createProjectStatusView(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    ProjectStatusView *v = new ProjectStatusView(getKoPart(), getPart(), m_tab);
    m_tab->addWidget(v);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, v, getPart(), "", index);
    ViewInfo vi = defaultViewInfo("ProjectStatusView");
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == TIP_USE_DEFAULT_TEXT) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }

    connect(v, &ViewBase::guiActivated, this, &View::slotGuiActivated);

    connect(this, &View::currentScheduleManagerChanged, v, &ProjectStatusView::setScheduleManager);

    v->updateReadWrite(m_readWrite);
    v->setProject(&getProject());
    v->setScheduleManager(currentScheduleManager());
    return v;
}

ViewBase *View::createPerformanceStatusView(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    PerformanceStatusView *v = new PerformanceStatusView(getKoPart(), getPart(), m_tab);
    m_tab->addWidget(v);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, v, getPart(), "", index);
    ViewInfo vi = defaultViewInfo("PerformanceStatusView");
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == TIP_USE_DEFAULT_TEXT) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }

    connect(v, &ViewBase::guiActivated, this, &View::slotGuiActivated);

    connect(this, &View::currentScheduleManagerChanged, v, &PerformanceStatusView::setScheduleManager);

    connect(v, &PerformanceStatusView::requestPopupMenu, this, &View::slotPopupMenuRequested);

    v->updateReadWrite(m_readWrite);
    v->setProject(&getProject());
    v->setScheduleManager(currentScheduleManager());
    return v;
}


ViewBase *View::createTaskStatusView(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    TaskStatusView *taskstatusview = new TaskStatusView(getKoPart(), getPart(), m_tab);
    taskstatusview->setViewSplitMode(false);
    m_tab->addWidget(taskstatusview);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, taskstatusview, getPart(), "", index);
    ViewInfo vi = defaultViewInfo("TaskStatusView");
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == TIP_USE_DEFAULT_TEXT) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }

    connect(taskstatusview, &ViewBase::guiActivated, this, &View::slotGuiActivated);

    connect(this, &View::currentScheduleManagerChanged, taskstatusview, &TaskStatusView::setScheduleManager);

    connect(taskstatusview, &TaskStatusView::requestPopupMenu, this, &View::slotPopupMenuRequested);
    connect(taskstatusview, &TaskStatusView::openTaskDescription, this, &View::slotOpenTaskDescription);

    taskstatusview->updateReadWrite(m_readWrite);
    taskstatusview->draw(getProject());
    taskstatusview->setScheduleManager(currentScheduleManager());
    return taskstatusview;
}

ViewBase *View::createTaskView(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    TaskView *v = new TaskView(getKoPart(), getPart(), m_tab);
    v->setViewSplitMode(false);
    m_tab->addWidget(v);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, v, getPart(), "", index);
    ViewInfo vi = defaultViewInfo("TaskView");
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == TIP_USE_DEFAULT_TEXT) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }

    v->draw(getProject());
    v->setScheduleManager(currentScheduleManager());

    connect(this, &View::currentScheduleManagerChanged, v, &TaskView::setScheduleManager);

    connect(v, &ViewBase::guiActivated, this, &View::slotGuiActivated);

    connect(v, &TaskView::requestPopupMenu, this, &View::slotPopupMenuRequested);
    connect(v, &TaskView::openTaskDescription, this, &View::slotOpenTaskDescription);
    v->updateReadWrite(m_readWrite);
    return v;
}

ViewBase *View::createTaskWorkPackageView(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    TaskWorkPackageView *v = new TaskWorkPackageView(getKoPart(), getPart(), m_tab);
    v->setViewSplitMode(false);
    m_tab->addWidget(v);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, v, getPart(), "", index);
    ViewInfo vi = defaultViewInfo("TaskWorkPackageView");
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == TIP_USE_DEFAULT_TEXT) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }

    v->setProject(&getProject());
    v->setScheduleManager(currentScheduleManager());

    connect(this, &View::currentScheduleManagerChanged, v, &TaskWorkPackageView::setScheduleManager);

    connect(v, &ViewBase::guiActivated, this, &View::slotGuiActivated);

    connect(v, &TaskWorkPackageView::requestPopupMenu, this, &View::slotPopupMenuRequested);

    connect(v, &TaskWorkPackageView::mailWorkpackage, this, &View::slotMailWorkpackage);
    connect(v, &TaskWorkPackageView::publishWorkpackages, this, &View::slotPublishWorkpackages);
    connect(v, &TaskWorkPackageView::openWorkpackages, this, &View::openWorkPackageMergeDialog);
    connect(this, &View::workPackagesAvailable, v, &TaskWorkPackageView::slotWorkpackagesAvailable);
    connect(v, &TaskWorkPackageView::checkForWorkPackages, getPart(), &MainDocument::checkForWorkPackages);
    connect(v, &TaskWorkPackageView::loadWorkPackageUrl, this, &View::loadWorkPackage);
    connect(v, &TaskWorkPackageView::openTaskDescription, this, &View::slotOpenTaskDescription);
    v->updateReadWrite(m_readWrite);

    return v;
}

ViewBase *View::createGanttView(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    GanttView *ganttview = new GanttView(getKoPart(), getPart(), m_tab, koDocument()->isReadWrite());
    m_tab->addWidget(ganttview);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, ganttview, getPart(), "", index);
    ViewInfo vi = defaultViewInfo("GanttView");
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == TIP_USE_DEFAULT_TEXT) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }

    ganttview->setProject(&(getProject()));
    ganttview->setScheduleManager(currentScheduleManager());

    connect(ganttview, &ViewBase::guiActivated, this, &View::slotGuiActivated);
/*  TODO: Review these
    connect(ganttview, SIGNAL(addRelation(KPlato::Node*,KPlato::Node*,int)), SLOT(slotAddRelation(KPlato::Node*,KPlato::Node*,int)));
    connect(ganttview, SIGNAL(modifyRelation(KPlato::Relation*,int)), SLOT(slotModifyRelation(KPlato::Relation*,int)));
    connect(ganttview, SIGNAL(modifyRelation(KPlato::Relation*)), SLOT(slotModifyRelation(KPlato::Relation*)));
    connect(ganttview, SIGNAL(itemDoubleClicked()), SLOT(slotOpenNode()));
    connect(ganttview, SIGNAL(itemRenamed(KPlato::Node*,QString)), this, SLOT(slotRenameNode(KPlato::Node*,QString)));*/

    connect(this, &View::currentScheduleManagerChanged, ganttview, &GanttView::setScheduleManager);

    connect(ganttview, &GanttView::requestPopupMenu, this, &View::slotPopupMenuRequested);
    connect(ganttview, &GanttView::openTaskDescription, this, &View::slotOpenTaskDescription);
    ganttview->updateReadWrite(m_readWrite);

    return ganttview;
}

ViewBase *View::createMilestoneGanttView(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    MilestoneGanttView *ganttview = new MilestoneGanttView(getKoPart(), getPart(), m_tab, koDocument()->isReadWrite());
    m_tab->addWidget(ganttview);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, ganttview, getPart(), "", index);
    ViewInfo vi = defaultViewInfo("MilestoneGanttView");
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == TIP_USE_DEFAULT_TEXT) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }

    ganttview->setProject(&(getProject()));
    ganttview->setScheduleManager(currentScheduleManager());

    connect(ganttview, &ViewBase::guiActivated, this, &View::slotGuiActivated);

    connect(this, &View::currentScheduleManagerChanged, ganttview, &MilestoneGanttView::setScheduleManager);

    connect(ganttview, &MilestoneGanttView::requestPopupMenu, this, &View::slotPopupMenuRequested);
    connect(ganttview, &MilestoneGanttView::openTaskDescription, this, &View::slotOpenTaskDescription);
    ganttview->updateReadWrite(m_readWrite);

    return ganttview;
}


ViewBase *View::createAccountsView(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    AccountsView *accountsview = new AccountsView(getKoPart(), &getProject(), getPart(), m_tab);
    m_tab->addWidget(accountsview);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, accountsview, getPart(), "", index);
    ViewInfo vi = defaultViewInfo("AccountsView");
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == TIP_USE_DEFAULT_TEXT) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }

    accountsview->setScheduleManager(currentScheduleManager());

    connect(this, &View::currentScheduleManagerChanged, accountsview, &AccountsView::setScheduleManager);

    connect(accountsview, &ViewBase::guiActivated, this, &View::slotGuiActivated);
    accountsview->updateReadWrite(m_readWrite);
    return accountsview;
}

ViewBase *View::createReportsGeneratorView(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
    ReportsGeneratorView *v = new ReportsGeneratorView(getKoPart(), getPart(), m_tab);
    m_tab->addWidget(v);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, v, getPart(), "", index);
    ViewInfo vi = defaultViewInfo("ReportsGeneratorView");
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == TIP_USE_DEFAULT_TEXT) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }

    v->setProject(&getProject());

    connect(this, &View::currentScheduleManagerChanged, v, &ViewBase::setScheduleManager);
    connect(this, &View::currentScheduleManagerChanged, v, &ViewBase::slotRefreshView);
    v->setScheduleManager(currentScheduleManager());

    connect(v, &ViewBase::guiActivated, this, &View::slotGuiActivated);
    connect(v, &ReportsGeneratorView::requestPopupMenu, this, &View::slotPopupMenuRequested);

    v->updateReadWrite(m_readWrite);
    return v;
}

ViewBase *View::createReportView(ViewListItem *cat, const QString &tag, const QString &name, const QString &tip, int index)
{
#ifdef PLAN_USE_KREPORT
    ReportView *v = new ReportView(getKoPart(), getPart(), m_tab);
    m_tab->addWidget(v);

    ViewListItem *i = m_viewlist->addView(cat, tag, name, v, getPart(), "", index);
    ViewInfo vi = defaultViewInfo("ReportView");
    if (name.isEmpty()) {
        i->setText(0, vi.name);
    }
    if (tip == TIP_USE_DEFAULT_TEXT) {
        i->setToolTip(0, vi.tip);
    } else {
        i->setToolTip(0, tip);
    }

    v->setProject(&getProject());

    connect(this, &View::currentScheduleManagerChanged, v, &ReportView::setScheduleManager);
    connect(this, &View::currentScheduleManagerChanged, v, SLOT(slotRefreshView()));
    v->setScheduleManager(currentScheduleManager());

    connect(v, &ReportView::guiActivated, this, &View::slotGuiActivated);
    v->updateReadWrite(m_readWrite);
    return v;
#else
    Q_UNUSED(cat)
    Q_UNUSED(tag)
    Q_UNUSED(name)
    Q_UNUSED(tip)
    Q_UNUSED(index)
    return 0;
#endif
}

Project& View::getProject() const
{
    return getPart() ->getProject();
}

KoPrintJob * View::createPrintJob()
{
    KoView *v = qobject_cast<KoView*>(canvas());
    if (v == 0) {
        return 0;
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

void View::slotInsertResourcesFile(const QString &file, const QUrl &projects)
{
    getPart()->insertResourcesFile(QUrl(file), projects);
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
    if (dlg == 0) {
        return;
    }
    if (result == QDialog::Accepted) {
        getPart()->insertFile(dlg->url(), dlg->parentNode(), dlg->afterNode());
    }
    dlg->deleteLater();
}

void View::slotLoadSharedProjects()
{
    LoadSharedProjectsDialog *dlg = new LoadSharedProjectsDialog(getProject(), getPart()->url(), this);
    connect(dlg, &QDialog::finished, this, &View::slotLoadSharedProjectsFinished);
    dlg->open();
}

void View::slotLoadSharedProjectsFinished(int result)
{
    LoadSharedProjectsDialog *dlg = qobject_cast<LoadSharedProjectsDialog*>(sender());
    if (dlg == 0) {
        return;
    }
    if (result == QDialog::Accepted) {
        getPart()->insertSharedProjects(dlg->urls());
    }
    dlg->deleteLater();
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
    if (dia == 0) {
        return;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command * cmd = dia->buildCommand();
        if (cmd) {
            //debugPlan<<"Modifying calendar(s)";
            getPart() ->addCommand(cmd); //also executes
        }
    }
    dia->deleteLater();
}

void View::slotSelectionChanged(ScheduleManager *sm) {
    debugPlan<<sm;
    if (sm == 0) {
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
    QAction *a = 0;
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
        unplugActionList("view_schedule_list");
        delete a;
        plugActionList("view_schedule_list", sortedActionList());
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
    QAction *checked = m_scheduleActionGroup->checkedAction();
    unplugActionList("view_schedule_list");
    QAction *act = addScheduleAction(s);
    plugActionList("view_schedule_list", sortedActionList());
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
    QAction *act = 0;
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
    emit currentScheduleManagerChanged(sm);
    QApplication::restoreOverrideCursor();
}

void View::slotViewSchedule(QAction *act)
{
    //debugPlan<<act;
    ScheduleManager *sm = 0;
    if (act != 0) {
        ScheduleManager *sch = m_scheduleActions.value(act, 0);
        sm = m_scheduleActions.value(act, 0);
    }
    setLabel(0);
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
    unplugActionList("view_schedule_list");
    const QMap<QAction*, ScheduleManager*> map = m_scheduleActions; // clazy:exclude=qmap-with-pointer-key
    QMap<QAction*, ScheduleManager*>::const_iterator it;
    for (it = map.constBegin(); it != map.constEnd(); ++it) {
        m_scheduleActionGroup->removeAction(it.key());
        delete it.key();
    }
    m_scheduleActions.clear();
    QAction *ca = 0;
    foreach(ScheduleManager *sm, getProject().allScheduleManagers()) {
        QAction *act = addScheduleAction(sm);
        if (sm == current) {
            ca = act;
        }
    }
    plugActionList("view_schedule_list", sortedActionList());
    if (ca == 0 && m_scheduleActionGroup->actions().count() > 0) {
        ca = m_scheduleActionGroup->actions().constFirst();
    }
    if (ca) {
        ca->setChecked(true);
    }
    slotViewSchedule(ca);
}

void View::slotCalculateSchedule(Project *project, ScheduleManager *sm)
{
    if (project == 0 || sm == 0) {
        return;
    }
    if (sm->parentManager() && ! sm->parentManager()->isScheduled()) {
        // the parent must be scheduled
        return;
    }
    CalculateScheduleCmd *cmd =  new CalculateScheduleCmd(*project, sm, kundo2_i18nc("@info:status 1=schedule name", "Calculate %1", sm->name()));
    getPart() ->addCommand(cmd);
    slotUpdate();
}

void View::slotRemoveCommands()
{
    while (! m_undocommands.isEmpty()) {
        m_undocommands.last()->undo();
        delete m_undocommands.takeLast();
    }
}

void View::slotBaselineSchedule(Project *project, ScheduleManager *sm)
{
    if (project == 0 || sm == 0) {
        return;
    }
    if (! sm->isBaselined() && project->isBaselined()) {
        KMessageBox::sorry(this, i18n("Cannot baseline. The project is already baselined."));
        return;
    }
    MacroCommand *cmd = nullptr;
    if (sm->isBaselined()) {
        KMessageBox::ButtonCode res = KMessageBox::warningContinueCancel(this, i18n("This schedule is baselined. Do you want to remove the baseline?"));
        if (res == KMessageBox::Cancel) {
            return;
        }
        cmd = new MacroCommand(kundo2_i18n("Reset baseline %1", sm->name()));
        cmd->addCommand(new ResetBaselineScheduleCmd(*sm));
    } else {
        cmd = new MacroCommand(kundo2_i18n("Baseline %1", sm->name()));
        if (sm->schedulingMode() == ScheduleManager::AutoMode) {
            cmd->addCommand(new ModifyScheduleManagerSchedulingModeCmd(*sm, ScheduleManager::ManualMode));
        }
        cmd->addCommand(new BaselineScheduleCmd(*sm, kundo2_i18n("Baseline %1", sm->name())));
    }
    getPart() ->addCommand(cmd);
}

void View::slotAddScheduleManager(Project *project)
{
    if (project == 0) {
        return;
    }
    ScheduleManager *sm = project->createScheduleManager();
    AddScheduleManagerCmd *cmd =  new AddScheduleManagerCmd(*project, sm, -1, kundo2_i18n("Add schedule %1", sm->name()));
    getPart() ->addCommand(cmd);
}

void View::slotDeleteScheduleManager(Project *project, ScheduleManager *sm)
{
    if (project == 0 || sm == 0) {
        return;
    }
    DeleteScheduleManagerCmd *cmd =  new DeleteScheduleManagerCmd(*project, sm, kundo2_i18n("Delete schedule %1", sm->name()));
    getPart() ->addCommand(cmd);
}

void View::slotMoveScheduleManager(ScheduleManager *sm, ScheduleManager *parent, int index)
{
    if (sm == 0) {
        return;
    }
    MoveScheduleManagerCmd *cmd =  new MoveScheduleManagerCmd(sm, parent, index, kundo2_i18n("Move schedule %1", sm->name()));
    getPart() ->addCommand(cmd);
}

void View::slotAddSubTask()
{
    Task * node = getProject().createTask(getPart() ->config().taskDefaults());
    SubTaskAddDialog *dia = new SubTaskAddDialog(getProject(), *node, currentNode(), getProject().accounts(), this);
    connect(dia, &QDialog::finished, this, &View::slotAddSubTaskFinished);
    dia->open();
}

void View::slotAddSubTaskFinished(int result)
{
    SubTaskAddDialog *dia = qobject_cast<SubTaskAddDialog*>(sender());
    if (dia == 0) {
        return;
    }
    if (result  == QDialog::Accepted) {
        KUndo2Command *m = dia->buildCommand();
        getPart() ->addCommand(m); // add task to project
    }
    dia->deleteLater();
}

void View::slotAddTask()
{
    Task * node = getProject().createTask(getPart() ->config().taskDefaults());
    TaskAddDialog *dia = new TaskAddDialog(getProject(), *node, currentNode(), getProject().accounts(), this);
    connect(dia, &QDialog::finished, this, &View::slotAddTaskFinished);
    dia->open();
}

void View::slotAddTaskFinished(int result)
{
    TaskAddDialog *dia = qobject_cast<TaskAddDialog*>(sender());
    if (dia == 0) {
        return;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command *m = dia->buildCommand();
        getPart() ->addCommand(m); // add task to project
    }
    dia->deleteLater();
}

void View::slotAddMilestone()
{
    Task * node = getProject().createTask();
    node->estimate() ->clear();

    TaskAddDialog *dia = new TaskAddDialog(getProject(), *node, currentNode(), getProject().accounts(), this);
    connect(dia, &QDialog::finished, this, &View::slotAddMilestoneFinished);
    dia->open();
}

void View::slotAddMilestoneFinished(int result)
{
    TaskAddDialog *dia = qobject_cast<TaskAddDialog*>(sender());
    if (dia == 0) {
        return;
    }
    if (result == QDialog::Accepted) {
        MacroCommand *c = new MacroCommand(kundo2_i18n("Add milestone"));
        c->addCommand(dia->buildCommand());
        getPart() ->addCommand(c); // add task to project
    }
    dia->deleteLater();
}

void View::slotAddSubMilestone()
{
    Task * node = getProject().createTask();
    node->estimate() ->clear();

    SubTaskAddDialog *dia = new SubTaskAddDialog(getProject(), *node, currentNode(), getProject().accounts(), this);
    connect(dia, &QDialog::finished, this, &View::slotAddSubMilestoneFinished);
    dia->open();
}

void View::slotAddSubMilestoneFinished(int result)
{
    SubTaskAddDialog *dia = qobject_cast<SubTaskAddDialog*>(sender());
    if (dia == 0) {
        return;
    }
    if (result == QDialog::Accepted) {
        MacroCommand *c = new MacroCommand(kundo2_i18n("Add sub-milestone"));
        c->addCommand(dia->buildCommand());
        getPart() ->addCommand(c); // add task to project
    }
    dia->deleteLater();
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
    if (dia == 0) {
        return;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command *cmd = dia->buildCommand();
        if (cmd) {
            getPart()->addCommand(cmd);
        }
    }
    dia->deleteLater();
}

void View::slotIntroduction()
{
    m_tab->setCurrentIndex(0);
}


Calendar *View::currentCalendar()
{
    ViewBase *v = dynamic_cast<ViewBase*>(m_tab->currentWidget());
    if (v == 0) {
        return 0;
    }
    return v->currentCalendar();
}

Node *View::currentNode() const
{
    ViewBase *v = dynamic_cast<ViewBase*>(m_tab->currentWidget());
    if (v == 0) {
        return 0;
    }
    Node * task = v->currentNode();
    if (0 != task) {
        return task;
    }
    return &(getProject());
}

Task *View::currentTask() const
{
    ViewBase *v = dynamic_cast<ViewBase*>(m_tab->currentWidget());
    if (v == 0) {
        return 0;
    }
    Node * task = v->currentNode();
    if (task) {
        return dynamic_cast<Task*>(task);
    }
    return 0;
}

Resource *View::currentResource()
{
    ViewBase *v = dynamic_cast<ViewBase*>(m_tab->currentWidget());
    if (v == 0) {
        return 0;
    }
    return v->currentResource();
}

ResourceGroup *View::currentResourceGroup()
{
    ViewBase *v = dynamic_cast<ViewBase*>(m_tab->currentWidget());
    if (v == 0) {
        return 0;
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
                connect(dia, &MainProjectDialog::sigLoadSharedResources, this, &View::slotInsertResourcesFile);
                connect(dia, &MainProjectDialog::loadResourceAssignments, getPart(), &MainDocument::loadResourceAssignments);
                connect(dia, &MainProjectDialog::clearResourceAssignments, getPart(), &MainDocument::clearResourceAssignments);
                dia->open();
                break;
            }
        case Node::Type_Subproject:
            //TODO
            break;
        case Node::Type_Task: {
                Task *task = static_cast<Task *>(node);
                TaskDialog *dia = new TaskDialog(getProject(), *task, getProject().accounts(), this);
                connect(dia, &QDialog::finished, this, &View::slotTaskEditFinished);
                dia->open();
                break;
            }
        case Node::Type_Milestone: {
                // Use the normal task dialog for now.
                // Maybe milestone should have it's own dialog, but we need to be able to
                // enter a duration in case we accidentally set a tasks duration to zero
                // and hence, create a milestone
                Task *task = static_cast<Task *>(node);
                TaskDialog *dia = new TaskDialog(getProject(), *task, getProject().accounts(), this);
                connect(dia, &QDialog::finished, this, &View::slotTaskEditFinished);
                dia->open();
                break;
            }
        case Node::Type_Summarytask: {
                Task *task = dynamic_cast<Task *>(node);
                Q_ASSERT(task);
                SummaryTaskDialog *dia = new SummaryTaskDialog(*task, this);
                connect(dia, &QDialog::finished, this, &View::slotSummaryTaskEditFinished);
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
    if (dia == 0) {
        return;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command * cmd = dia->buildCommand();
        if (cmd) {
            getPart() ->addCommand(cmd);
        }
    }
    dia->deleteLater();
}

void View::slotTaskEditFinished(int result)
{
    TaskDialog *dia = qobject_cast<TaskDialog*>(sender());
    if (dia == 0) {
        return;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command * cmd = dia->buildCommand();
        if (cmd) {
            getPart() ->addCommand(cmd);
        }
    }
    dia->deleteLater();
}

void View::slotSummaryTaskEditFinished(int result)
{
    SummaryTaskDialog *dia = qobject_cast<SummaryTaskDialog*>(sender());
    if (dia == 0) {
        return;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command * cmd = dia->buildCommand();
        if (cmd) {
            getPart() ->addCommand(cmd);
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

void View::slotTaskProgress()
{
    //debugPlan;
    Node * node = currentNode();
    if (!node)
        return ;

    switch (node->type()) {
        case Node::Type_Project: {
                break;
            }
        case Node::Type_Subproject:
            //TODO
            break;
        case Node::Type_Task: {
                Task *task = dynamic_cast<Task *>(node);
                Q_ASSERT(task);
                TaskProgressDialog *dia = new TaskProgressDialog(*task, currentScheduleManager(),  getProject().standardWorktime(), this);
                connect(dia, &QDialog::finished, this, &View::slotTaskProgressFinished);
                dia->open();
                break;
            }
        case Node::Type_Milestone: {
                Task *task = dynamic_cast<Task *>(node);
                Q_ASSERT(task);
                MilestoneProgressDialog *dia = new MilestoneProgressDialog(*task, this);
                connect(dia, &QDialog::finished, this, &View::slotMilestoneProgressFinished);
                dia->open();
                break;
            }
        case Node::Type_Summarytask: {
                // TODO
                break;
            }
        default:
            break; // avoid warnings
    }
}

void View::slotTaskProgressFinished(int result)
{
    TaskProgressDialog *dia = qobject_cast<TaskProgressDialog*>(sender());
    if (dia == 0) {
        return;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command * m = dia->buildCommand();
        if (m) {
            getPart() ->addCommand(m);
        }
    }
    dia->deleteLater();
}

void View::slotMilestoneProgressFinished(int result)
{
    MilestoneProgressDialog *dia = qobject_cast<MilestoneProgressDialog*>(sender());
    if (dia == 0) {
        return;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command * m = dia->buildCommand();
        if (m) {
            getPart() ->addCommand(m);
        }
    }
    dia->deleteLater();
}

void View::slotOpenProjectDescription()
{
    debugPlan<<koDocument()->isReadWrite();
    TaskDescriptionDialog *dia = new TaskDescriptionDialog(getProject(), this, !koDocument()->isReadWrite());
    connect(dia, &QDialog::finished, this, &View::slotTaskDescriptionFinished);
    dia->open();
}

void View::slotTaskDescription()
{
    slotOpenTaskDescription(!koDocument()->isReadWrite());
}

void View::slotOpenTaskDescription(bool ro)
{
    //debugPlan;
    Node * node = currentNode();
    if (!node)
        return ;

    switch (node->type()) {
        case Node::Type_Subproject:
            //TODO
            break;
        case Node::Type_Project:
        case Node::Type_Task:
        case Node::Type_Milestone:
        case Node::Type_Summarytask: {
                TaskDescriptionDialog *dia = new TaskDescriptionDialog(*node, this, ro);
                connect(dia, &QDialog::finished, this, &View::slotTaskDescriptionFinished);
                dia->open();
                break;
            }
        default:
            break; // avoid warnings
    }
}

void View::slotTaskDescriptionFinished(int result)
{
    TaskDescriptionDialog *dia = qobject_cast<TaskDescriptionDialog*>(sender());
    if (dia == 0) {
        return;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command * m = dia->buildCommand();
        if (m) {
            getPart() ->addCommand(m);
        }
    }
    dia->deleteLater();
}

void View::slotDocuments()
{
    //debugPlan;
    Node * node = currentNode();
    if (!node) {
        return ;
    }
    switch (node->type()) {
        case Node::Type_Subproject:
            //TODO
            break;
        case Node::Type_Project:
        case Node::Type_Summarytask:
        case Node::Type_Task:
        case Node::Type_Milestone: {
            DocumentsDialog *dia = new DocumentsDialog(*node, this);
            connect(dia, &QDialog::finished, this, &View::slotDocumentsFinished);
            dia->open();
            break;
        }
        default:
            break; // avoid warnings
    }
}

void View::slotDocumentsFinished(int result)
{
    DocumentsDialog *dia = qobject_cast<DocumentsDialog*>(sender());
    if (dia == 0) {
        return;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command * m = dia->buildCommand();
        if (m) {
            getPart()->addCommand(m);
        }
    }
    dia->deleteLater();
}

void View::slotDeleteTaskList(QList<Node*> lst)
{
    //debugPlan;
    foreach (Node *n, lst) {
        if (n->isScheduled()) {
            KMessageBox::ButtonCode res = KMessageBox::warningContinueCancel(this, i18n("A task that has been scheduled will be deleted. This will invalidate the schedule."));
            if (res == KMessageBox::Cancel) {
                return;
            }
            break;
        }
    }
    if (lst.count() == 1) {
        getPart()->addCommand(new NodeDeleteCmd(lst.takeFirst(), kundo2_i18n("Delete task")));
        return;
    }
    int num = 0;
    MacroCommand *cmd = new MacroCommand(kundo2_i18np("Delete task", "Delete tasks", lst.count()));
    while (!lst.isEmpty()) {
        Node *node = lst.takeFirst();
        if (node == 0 || node->parentNode() == 0) {
            debugPlan << (node ?"Task is main project" :"No current task");
            continue;
        }
        bool del = true;
        foreach (Node *n, lst) {
            if (node->isChildOf(n)) {
                del = false; // node is going to be deleted when we delete n
                break;
            }
        }
        if (del) {
            //debugPlan<<num<<": delete:"<<node->name();
            cmd->addCommand(new NodeDeleteCmd(node, kundo2_i18n("Delete task")));
            num++;
        }
    }
    if (num > 0) {
        getPart()->addCommand(cmd);
    } else {
        delete cmd;
    }
}

void View::slotDeleteTask(Node *node)
{
    //debugPlan;
    if (node == 0 || node->parentNode() == 0) {
        debugPlan << (node ?"Task is main project" :"No current task");
        return ;
    }
    if (node->isScheduled()) {
        KMessageBox::ButtonCode res = KMessageBox::warningContinueCancel(this, i18n("This task has been scheduled. This will invalidate the schedule."));
        if (res == KMessageBox::Cancel) {
            return;
        }
    }
    NodeDeleteCmd *cmd = new NodeDeleteCmd(node, kundo2_i18n("Delete task"));
    getPart() ->addCommand(cmd);
}

void View::slotDeleteCurrentTask()
{
    //debugPlan;
    return slotDeleteTask(currentNode());
}

void View::slotIndentTask()
{
    //debugPlan;
    Node * node = currentNode();
    if (node == 0 || node->parentNode() == 0) {
        debugPlan << (node ?"Task is main project" :"No current task");
        return ;
    }
    if (getProject().canIndentTask(node)) {
        NodeIndentCmd * cmd = new NodeIndentCmd(*node, kundo2_i18n("Indent task"));
        getPart() ->addCommand(cmd);
    }
}

void View::slotUnindentTask()
{
    //debugPlan;
    Node * node = currentNode();
    if (node == 0 || node->parentNode() == 0) {
        debugPlan << (node ?"Task is main project" :"No current task");
        return ;
    }
    if (getProject().canUnindentTask(node)) {
        NodeUnindentCmd * cmd = new NodeUnindentCmd(*node, kundo2_i18n("Unindent task"));
        getPart() ->addCommand(cmd);
    }
}

void View::slotMoveTaskUp()
{
    //debugPlan;

    Node * task = currentNode();
    if (0 == task) {
        // is always != 0. At least we would get the Project, but you never know who might change that
        // so better be careful
        errorPlan << "No current task" << endl;
        return ;
    }

    if (Node::Type_Project == task->type()) {
        debugPlan <<"The root node cannot be moved up";
        return ;
    }
    if (getProject().canMoveTaskUp(task)) {
        NodeMoveUpCmd * cmd = new NodeMoveUpCmd(*task, kundo2_i18n("Move task up"));
        getPart() ->addCommand(cmd);
    }
}

void View::slotMoveTaskDown()
{
    //debugPlan;

    Node * task = currentNode();
    if (0 == task) {
        // is always != 0. At least we would get the Project, but you never know who might change that
        // so better be careful
        return ;
    }

    if (Node::Type_Project == task->type()) {
        debugPlan <<"The root node cannot be moved down";
        return ;
    }
    if (getProject().canMoveTaskDown(task)) {
        NodeMoveDownCmd * cmd = new NodeMoveDownCmd(*task, kundo2_i18n("Move task down"));
        getPart() ->addCommand(cmd);
    }
}

void View::openRelationDialog(Node *par, Node *child)
{
    //debugPlan;
    Relation * rel = new Relation(par, child);
    AddRelationDialog *dia = new AddRelationDialog(getProject(), rel, this);
    connect(dia, &QDialog::finished, this, &View::slotAddRelationFinished);
    dia->open();
}

void View::slotAddRelationFinished(int result)
{
    AddRelationDialog *dia = qobject_cast<AddRelationDialog*>(sender());
    if (dia == 0) {
        return;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command * m = dia->buildCommand();
        if (m) {
            getPart() ->addCommand(m);
        }
    }
    dia->deleteLater();
}

void View::slotAddRelation(Node *par, Node *child, int linkType)
{
    //debugPlan;
    if (linkType == Relation::FinishStart ||
            linkType == Relation::StartStart ||
            linkType == Relation::FinishFinish) {
        Relation * rel = new Relation(par, child, static_cast<Relation::Type>(linkType));
        getPart() ->addCommand(new AddRelationCmd(getProject(), rel, kundo2_i18n("Add task dependency")));
    } else {
        openRelationDialog(par, child);
    }
}

void View::slotEditRelation(Relation *rel)
{
    //debugPlan;
    ModifyRelationDialog *dia = new ModifyRelationDialog(getProject(), rel, this);
    connect(dia, &QDialog::finished, this, &View::slotModifyRelationFinished);
    dia->open();
}

void View::slotModifyRelationFinished(int result)
{
    ModifyRelationDialog *dia = qobject_cast<ModifyRelationDialog*>(sender());
    if (dia == 0) {
        return ;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command *cmd = dia->buildCommand();
        if (cmd) {
            getPart() ->addCommand(cmd);
        }
    }
    dia->deleteLater();
}

void View::slotModifyRelation(Relation *rel, int linkType)
{
    //debugPlan;
    if (linkType == Relation::FinishStart ||
            linkType == Relation::StartStart ||
            linkType == Relation::FinishFinish) {
        getPart() ->addCommand(new ModifyRelationTypeCmd(rel, static_cast<Relation::Type>(linkType)));
    } else {
        slotEditRelation(rel);
    }
}

void View::slotModifyCurrentRelation()
{
    ViewBase *v = dynamic_cast<ViewBase*>(m_tab->currentWidget());
    if (v == 0) {
        return;
    }
    Relation *rel = v->currentRelation();
    if (rel) {
        slotEditRelation(rel);
    }
}

void View::slotDeleteRelation()
{
    ViewBase *v = dynamic_cast<ViewBase*>(m_tab->currentWidget());
    if (v == 0) {
        return;
    }
    Relation *rel = v->currentRelation();
    if (rel) {
        getPart()->addCommand(new DeleteRelationCmd(getProject(), rel, kundo2_i18n("Delete task dependency")));
    }
}

void View::slotEditCurrentResource()
{
    //debugPlan;
    slotEditResource(currentResource());
}

void View::slotEditResource(Resource *resource)
{
    if (resource == 0) {
        return ;
    }
    ResourceDialog *dia = new ResourceDialog(getProject(), resource, this);
    connect(dia, &QDialog::finished, this, &View::slotEditResourceFinished);
    dia->open();
}

void View::slotEditResourceFinished(int result)
{
    //debugPlan;
    ResourceDialog *dia = qobject_cast<ResourceDialog*>(sender());
    if (dia == 0) {
        return ;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command * cmd = dia->buildCommand();
        if (cmd)
            getPart() ->addCommand(cmd);
    }
    dia->deleteLater();
}

void View::slotDeleteResource(Resource *resource)
{
    getPart()->addCommand(new RemoveResourceCmd(resource, kundo2_i18n("Delete resource")));
}

void View::slotDeleteResourceGroup(ResourceGroup *group)
{
    getPart()->addCommand(new RemoveResourceGroupCmd(group->project(), group, kundo2_i18n("Delete resourcegroup")));
}

void View::slotDeleteResourceObjects(QObjectList lst)
{
    //debugPlan;
    foreach (QObject *o, lst) {
        Resource *r = qobject_cast<Resource*>(o);
        if (r && r->isScheduled()) {
            KMessageBox::ButtonCode res = KMessageBox::warningContinueCancel(this, i18n("A resource that has been scheduled will be deleted. This will invalidate the schedule."));
            if (res == KMessageBox::Cancel) {
                return;
            }
            break;
        }
        ResourceGroup *g = qobject_cast<ResourceGroup*>(o);
        if (g && g->isScheduled()) {
            KMessageBox::ButtonCode res = KMessageBox::warningContinueCancel(this, i18n("A resource that has been scheduled will be deleted. This will invalidate the schedule."));
            if (res == KMessageBox::Cancel) {
                return;
            }
            break;
        }
    }
    if (lst.count() == 1) {
        Resource *r = qobject_cast<Resource*>(lst.first());
        if (r) {
            slotDeleteResource(r);
        } else {
            ResourceGroup *g = qobject_cast<ResourceGroup*>(lst.first());
            if (g) {
                slotDeleteResourceGroup(g);
            }
        }
        return;
    }
//    int num = 0;
    MacroCommand *cmd = 0, *rc = 0, *gc = 0;
    foreach (QObject *o, lst) {
        Resource *r = qobject_cast<Resource*>(o);
        if (r) {
            if (rc == 0)  rc = new MacroCommand(KUndo2MagicString());
            rc->addCommand(new RemoveResourceCmd(r));
            continue;
        }
        ResourceGroup *g = qobject_cast<ResourceGroup*>(o);
        if (g) {
            if (gc == 0)  gc = new MacroCommand(KUndo2MagicString());
            gc->addCommand(new RemoveResourceGroupCmd(g->project(), g));
        }
    }
    if (rc || gc) {
        KUndo2MagicString s;
        if (rc && gc) {
            s = kundo2_i18n("Delete resourcegroups and resources");
        } else if (rc) {
            s = kundo2_i18np("Delete resource", "Delete resources", lst.count());
        } else {
            s = kundo2_i18np("Delete resourcegroup", "Delete resourcegroups", lst.count());
        }
        cmd = new MacroCommand(s);
    }
    if (rc)
        cmd->addCommand(rc);
    if (gc)
        cmd->addCommand(gc);
    if (cmd)
        getPart()->addCommand(cmd);
}


void View::updateReadWrite(bool readwrite)
{
    m_readWrite = readwrite;
    m_viewlist->setReadWrite(readwrite);
}

MainDocument *View::getPart() const
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
    return 0L;
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
        foreach (DockWidget *ds, view->dockers()) {
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
    getPart()->removeViewListItem(this, item);
}

void View::removeViewListItem(const ViewListItem *item)
{
    if (item == 0) {
        return;
    }
    ViewListItem *itm = m_viewlist->findItem(item->tag());
    if (itm == 0) {
        return;
    }
    m_viewlist->removeViewListItem(itm);
    return;
}

void View::slotViewListItemInserted(ViewListItem *item, ViewListItem *parent, int index)
{
    getPart()->insertViewListItem(this, item, parent, index);
}

void View::addViewListItem(const ViewListItem *item, const ViewListItem *parent, int index)
{
    if (item == 0) {
        return;
    }
    if (parent == 0) {
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
    if (cat == 0) {
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
        KMessageBox::sorry(this, xi18nc("@info", "Cannot open file:<br/><filename>%1</filename>", fn));
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
        return 0;
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
        getPart() ->addCommand(cmd);
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
                foreach (QAction *a, lst) {
                    menu->addAction(a);
                }
            }
        }
        menu->exec(pos);
        foreach (QAction *a, lst) {
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
    Context *ctx = getPart()->context();
    if (ctx == 0 || ! ctx->isLoaded()) {
        return false;
    }
    KoXmlElement n = ctx->context();
    QString cv = n.attribute("current-view");
    if (! cv.isEmpty()) {
        m_viewlist->setSelected(m_viewlist->findItem(cv));
    } else debugPlan<<"No current view";

    long id = n.attribute("current-schedule", "-1").toLong();
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
        me.setAttribute("current-schedule", QString::number((qlonglong)id));
    }
    ViewListItem *item = m_viewlist->findItem(qobject_cast<ViewBase*>(m_tab->currentWidget()));
    if (item) {
        me.setAttribute("current-view", item->tag());
    }
    m_viewlist->save(me);
}

void View::loadWorkPackage(Project *project, const QList<QUrl> &urls)
{
    bool loaded = false;
    for (const QUrl &url : urls) {
        loaded |= getPart()->loadWorkPackage(*project, url);
    }
    if (loaded) {
        slotWorkPackageLoaded();
    }
}

void View::setLabel(ScheduleManager *sm)
{
    //debugPlan;
    Schedule *s = sm == 0 ? 0 : sm->expected();
    if (s && !s->isDeleted() && s->isScheduled()) {
        m_estlabel->setText(sm->name());
        return;
    }
    m_estlabel->setText(xi18nc("@info:status", "Not scheduled"));
}

void View::slotWorkPackageLoaded()
{
    debugPlan<<getPart()->workPackages();
    addStatusBarItem(m_workPackageButton, 0, true);
    emit workPackagesAvailable(true);
}

void View::openWorkPackageMergeDialog()
{
    WorkPackageMergeDialog *dlg = new WorkPackageMergeDialog(&getProject(), getPart()->workPackages(), this);
    connect(dlg, &QDialog::finished, this, &View::workPackageMergeDialogFinished);
    connect(dlg, SIGNAL(terminateWorkPackage(const KPlato::Package*)), getPart(), SLOT(terminateWorkPackage(const KPlato::Package*)));
    connect(dlg, &WorkPackageMergeDialog::executeCommand, koDocument(), &KoDocument::addCommand);
    dlg->open();
    removeStatusBarItem(m_workPackageButton);
    emit workPackagesAvailable(false);
}

void View::workPackageMergeDialogFinished(int result)
{
    debugPlanWp<<"result:"<<result<<"sender:"<<sender();
    WorkPackageMergeDialog *dlg = qobject_cast<WorkPackageMergeDialog*>(sender());
    Q_ASSERT(dlg);
    if (!getPart()->workPackages().isEmpty()) {
        slotWorkPackageLoaded();
    }
    if (dlg) {
        dlg->deleteLater();
    }
}


void View::slotMailWorkpackage(Node *node, Resource *resource)
{
    debugPlan;
    QTemporaryFile tmpfile(QDir::tempPath() + QLatin1String("/calligraplanwork_XXXXXX") + QLatin1String(".planwork"));
    tmpfile.setAutoRemove(false);
    if (! tmpfile.open()) {
        debugPlan<<"Failed to open file";
        KMessageBox::error(0, i18n("Failed to open temporary file"));
        return;
    }
    QUrl url = QUrl::fromLocalFile(tmpfile.fileName());
    if (! getPart()->saveWorkPackageUrl(url, node, activeScheduleId(), resource)) {
        debugPlan<<"Failed to save to file";
        KMessageBox::error(0, xi18nc("@info", "Failed to save to temporary file:<br/> <filename>%1</filename>", url.url()));
        return;
    }
    QStringList attachURLs;
    attachURLs << url.url();
    QString to = resource == 0 ? node->leader() : (resource->name() + " <" + resource->email() + '>');
    QString cc;
    QString bcc;
    QString subject = i18n("Work Package: %1", node->name());
    QString body = i18nc("1=project name, 2=task name", "%1\n%2", getProject().name(), node->name());
    QString messageFile;

    KToolInvocation::invokeMailer(to, cc, bcc, subject, body, messageFile, attachURLs);
}

void View::slotPublishWorkpackages(const QList<Node*> &nodes, Resource *resource, bool mailTo)
{
    debugPlanWp<<resource<<nodes;
    if (resource == 0) {
        warnPlan<<"No resource, we don't handle node->leader() yet";
        return;
    }
    bool mail = mailTo;
    QString body;
    QStringList attachURLs;

    QString path;
    if (getProject().workPackageInfo().publishUrl.isValid()) {
        path = getProject().workPackageInfo().publishUrl.path();
        debugPlanWp<<"publish:"<<path;
    } else {
        path = QDir::tempPath();
        mail = true;
    }
    foreach (Node *n, nodes) {
        QTemporaryFile tmpfile(path + QLatin1String("/calligraplanwork_XXXXXX") + QLatin1String(".planwork"));
        tmpfile.setAutoRemove(false);
        if (! tmpfile.open()) {
            debugPlanWp<<"Failed to open file";
            KMessageBox::error(0, i18n("Failed to open work package file"));
            return;
        }
        QUrl url = QUrl::fromLocalFile(tmpfile.fileName());
        debugPlanWp<<url;
        if (! getPart()->saveWorkPackageUrl(url, n, activeScheduleId(), resource)) {
            debugPlan<<"Failed to save to file";
            KMessageBox::error(0, xi18nc("@info", "Failed to save to temporary file:<br/><filename>%1</filename>", url.url()));
            return;
        }
        attachURLs << url.url();
        body += n->name() + '\n';
    }
    if (mail) {
        debugPlanWp<<attachURLs;
        QString to = resource->name() + " <" + resource->email() + '>';
        QString subject = i18n("Work Package for project: %1", getProject().name());
        QString cc;
        QString bcc;
        QString messageFile;

        KToolInvocation::invokeMailer(to, cc, bcc, subject, body, messageFile, attachURLs);
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
    if (dlg == 0) {
        return;
    }
    if (result == QDialog::Accepted) {
        KUndo2Command *c = dlg->buildCommand(getProject());
        if (c) {
            getPart()->addCommand(c);
        }
    }
    dlg->deleteLater();
}

void View::saveTaskModule(const QUrl &url, Project *project)
{
    // NOTE: workaround: KoResourcePaths::saveLocation("calligraplan_taskmodules"); does not work
    const QString dir = KoResourcePaths::saveLocation("appdata", "taskmodules/");
    debugPlan<<"dir="<<dir;
    if (! dir.isEmpty()) {
        Part *part = new Part(this);
        MainDocument *doc = new MainDocument(part);
        part->setDocument(doc);
        doc->disconnect(); // doc shall not handle feedback from openUrl()
        doc->setAutoSave(0); //disable
        doc->insertProject(*project, 0, 0); // FIXME: destroys project, find better way
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
