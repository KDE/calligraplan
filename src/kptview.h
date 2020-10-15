/* This file is part of the KDE project
  Copyright (C) 1998, 1999, 2000 Torben Weis <weis@kde.org>
  Copyright (C) 2002 - 2010 Dag Andersen <dag.andersen@kdemail.net>
  Copyright (C) 2019 Dag Andersen <dag.andersen@kdemail.net>
  
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

#ifndef KPTVIEW_H
#define KPTVIEW_H

#include "plan_export.h"

#include <KoView.h>

#include "kptcontext.h"
#include "kptviewbase.h"

#include <QActionGroup>
#include <QDockWidget>
#include <QMap>

#include <kconfigdialog.h>

class QMenu;
class QPrintDialog;
class QStackedWidget;
class QSplitter;
class QUrl;
class QToolButton;
class KUndo2Command;
class QAction;

class KToggleAction;
class QLabel;
class KConfigSkeleton;
class KConfigSkeletonItem;
class KPageWidgetItem;

class KoView;

namespace KPlato
{

class View;
class ViewBase;
class ViewListItem;
class ViewListWidget;
struct ViewInfo;
class AccountsView;
class ResourceCoverageView;
class GanttView;
class PertEditor;
class AccountsEditor;
class TaskEditor;
class CalendarEditor;
class ScheduleEditor;
class ScheduleManager;
class CalculateScheduleCmd;
class TaskStatusView;
class Calendar;
class MainDocument;
class Part;
class Node;
class Project;
class Task;
class MainSchedule;
class Schedule;
class Resource;
class ResourceGroup;
class Relation;
class Context;
class ViewAdaptor;
class HtmlView;
class ReportView;

class ReportDesignDialog;

class DockWidget;


class PLAN_EXPORT View : public KoView
{
    Q_OBJECT

public:
    explicit View(KoPart *part, MainDocument *doc, QWidget *parent = nullptr);
    ~View() override;

    MainDocument *getPart() const;

    KoPart *getKoPart() const;

    Project& getProject() const;

    QMenu *popupMenu(const QString& name);

    virtual bool loadContext();
    virtual void saveContext(QDomElement &context) const;

    QWidget *canvas() const;

    KoPageLayout pageLayout() const override;
    void setPageLayout(const KoPageLayout &pageLayout) override;

    ScheduleManager *currentScheduleManager() const;
    long activeScheduleId() const;
    void setActiveSchedule(long id);

    /// Returns the default view information like standard name and tooltip for view type @p type
    ViewInfo defaultViewInfo(const QString &type) const;
    /// Returns the default category information like standard name and tooltip for category type @p type
    ViewInfo defaultCategoryInfo(const QString &type) const;

    ViewBase *createTaskEditor(ViewListItem *cat, const QString &tag, const QString &name = QString(), const QString &tip = QString(), int index = -1);
    ViewBase *createResourceGroupEditor(ViewListItem *cat, const QString &tag, const QString &name = QString(), const QString &tip = QString(), int index = -1);
    ViewBase *createResourceEditor(ViewListItem *cat, const QString &tag, const QString &name = QString(), const QString &tip = QString(), int index = -1);
    ViewBase *createAccountsEditor(ViewListItem *cat, const QString &tag, const QString &name = QString(), const QString &tip = QString(), int index = -1);
    ViewBase *createCalendarEditor(ViewListItem *cat, const QString &tag, const QString &name = QString(), const QString &tip = QString(), int index = -1);
    ViewBase *createScheduleHandler(ViewListItem *cat, const QString &tag, const QString &name = QString(), const QString &tip = QString(), int index = -1);
    ScheduleEditor *createScheduleEditor(QWidget *parent);
    ViewBase *createScheduleEditor(ViewListItem *cat, const QString &tag, const QString &name = QString(), const QString &tip = QString(), int index = -1);
    ViewBase *createDependencyEditor(ViewListItem *cat, const QString &tag, const QString &name = QString(), const QString &tip = QString(), int index = -1);
    ViewBase *createPertEditor(ViewListItem *cat, const QString &tag, const QString &name = QString(), const QString &tip = QString(), int index = -1);
    ViewBase *createProjectStatusView(ViewListItem *cat, const QString &tag, const QString &name = QString(), const QString &tip = QString(), int index = -1);
    ViewBase *createPerformanceStatusView(ViewListItem *cat, const QString &tag, const QString &name = QString(), const QString &tip = QString(), int index = -1);
    ViewBase *createTaskStatusView(ViewListItem *cat, const QString &tag, const QString &name = QString(), const QString &tip = QString(), int index = -1);
    ViewBase *createTaskView(ViewListItem *cat, const QString &tag, const QString &name = QString(), const QString &tip = QString(), int index = -1);
    ViewBase *createTaskWorkPackageView(ViewListItem *cat, const QString &tag, const QString &name = QString(), const QString &tip = QString(), int index = -1);
    ViewBase *createGanttView(ViewListItem *cat, const QString &tag, const QString &name = QString(), const QString &tip = QString(), int index = -1);
    ViewBase *createMilestoneGanttView(ViewListItem *cat, const QString &tag, const QString &name = QString(), const QString &tip = QString(), int index = -1);
    ViewBase *createResourceAppointmentsView(ViewListItem *cat, const QString &tag, const QString &name = QString(), const QString &tip = QString(), int index = -1);
    ViewBase *createResourceAppointmentsGanttView(ViewListItem *cat, const QString &tag, const QString &name = QString(), const QString &tip = QString(), int index = -1);
    ViewBase *createResourceCoverageView(ViewListItem *cat, const QString &tag, const QString &name = QString(), const QString &tip = QString(), int index = -1);
    ViewBase *createAccountsView(ViewListItem *cat, const QString &tag, const QString &name = QString(), const QString &tip = QString(), int index = -1);
    ViewBase *createChartView(ViewListItem *cat, const QString &tag, const QString &name = QString(), const QString &tip = QString(), int index = -1);
    ViewBase *createReportView(ViewListItem *cat, const QString &tag, const QString &name = QString(), const QString &tip = QString(), int index = -1);
    ViewBase *createReportsGeneratorView(ViewListItem *cat, const QString &tag, const QString &name = QString(), const QString &tip = QString(), int index = -1);

    KoPrintJob * createPrintJob() override;
    QPrintDialog* createPrintDialog(KoPrintJob*, QWidget*) override;

Q_SIGNALS:
    void currentScheduleManagerChanged(KPlato::ScheduleManager *sm);
    void taskModulesChanged(const QStringList &modules);
    void workPackagesAvailable(bool);

public Q_SLOTS:
    void slotUpdate();
    void slotCreateNewProject();
    void slotEditCurrentResource();
    void slotEditResource(KPlato::Resource *resource);
    void slotEditCut();
    void slotEditCopy();
    void slotEditPaste();
    void slotRefreshView();
    void slotViewSelector(bool show);

    void slotAddTask();
    void slotAddSubTask();
    void slotAddMilestone();
    void slotAddSubMilestone();
    void slotProjectEdit();
    void slotDefineWBS();
    void slotCurrencyConfig();

    void slotCreateView();

    void slotIntroduction();

    void openRelationDialog(KPlato::Node *par, KPlato::Node *child);
    void slotEditRelation(KPlato::Relation *rel);
    void slotAddRelation(KPlato::Node *par, KPlato::Node *child, int linkType);
    void slotModifyRelation(KPlato::Relation *rel, int linkType);
    void slotModifyCurrentRelation();
    void slotDeleteRelation();

    void slotRenameNode(KPlato::Node *node, const QString& name);

    void slotPopupMenuRequested(const QString& menuname, const QPoint &pos);
    void slotPopupMenu(const QString& menuname, const QPoint &pos, KPlato::ViewListItem *item);

    void addViewListItem(const KPlato::ViewListItem *item, const KPlato::ViewListItem *parent, int index);
    void removeViewListItem(const KPlato::ViewListItem *item);

    void slotOpenReportFile();

    void slotSelectionChanged(KPlato::ScheduleManager *sm);
    void slotUpdateViewInfo(KPlato::ViewListItem *itm);

    /// Load the workpackages from @p urls into @p project.
    void loadWorkPackage(KPlato::Project *project, const QList<QUrl> &urls);

    void slotCreateTemplate();

protected Q_SLOTS:
    void slotGuiActivated(KPlato::ViewBase *view, bool);
    void slotViewActivated(KPlato::ViewListItem*, KPlato::ViewListItem*);
    void slotPlugScheduleActions();
    void slotViewSchedule(QAction *act);
    void slotScheduleAdded(const KPlato::ScheduleManager*);
    void slotScheduleRemoved(const KPlato::ScheduleManager*);
    void slotScheduleSwapped(KPlato::ScheduleManager *from, KPlato::ScheduleManager *to);
    void slotScheduleCalculated(KPlato::Project *project, KPlato::ScheduleManager *manager);

    void slotAddScheduleManager(KPlato::Project *project);
    void slotDeleteScheduleManager(KPlato::Project *project, KPlato::ScheduleManager *sm);
    void slotMoveScheduleManager(KPlato::ScheduleManager *sm, KPlato::ScheduleManager *parent, int index);
    void slotCalculateSchedule(KPlato::Project*, KPlato::ScheduleManager*);
    void slotBaselineSchedule(KPlato::Project *project, KPlato::ScheduleManager *sm);

    void slotProjectWorktime();

    void slotOpenCurrentNode();
    void slotOpenNode(KPlato::Node *node);
    void slotTaskProgress();
    void slotOpenProjectDescription();
    void slotTaskDescription();
    void slotOpenTaskDescription(bool);
    void slotDocuments();
    void slotDeleteTaskList(QList<KPlato::Node*> lst);
    void slotDeleteTask(KPlato::Node *node);
    void slotDeleteCurrentTask();
    void slotIndentTask();
    void slotUnindentTask();
    void slotMoveTaskUp();
    void slotMoveTaskDown();

    void slotConnectNode();

    void slotDeleteResource(KPlato::Resource *resource);
    void slotDeleteResourceGroup(KPlato::ResourceGroup *group);
    void slotDeleteResourceObjects(const QObjectList);
    void slotDeleteResourceGroups(const QObjectList lst);

    void slotCurrentChanged(int);
    void slotSelectDefaultView();

    void slotInsertResourcesFile(const QString&, const QUrl &projects);
    void slotInsertFile();

    void slotLoadSharedProjects();

    void slotWorkPackageLoaded();
    void slotMailWorkpackage(KPlato::Node *node, KPlato::Resource *resource = nullptr);
    void slotPublishWorkpackages(const QList<KPlato::Node*> &nodes, KPlato::Resource *resource, bool mailTo);

    void slotOpenUrlRequest(KPlato::HtmlView *v, const QUrl &url);

    void createReportView(const QDomDocument &doc);

    void saveTaskModule(const QUrl &url, KPlato::Project *project);
    void removeTaskModule(const QUrl &url);

protected:
    void guiActivateEvent(bool activated) override;
    void updateReadWrite(bool readwrite) override;

    QList<QAction*> sortedActionList();
    QAction *addScheduleAction(ScheduleManager *sch);
    void setLabel(ScheduleManager *sm = nullptr);
    Task *currentTask() const;
    Node *currentNode() const;
    Resource *currentResource();
    ResourceGroup *currentResourceGroup();
    Calendar *currentCalendar();
    void updateView(QWidget *widget);

    ViewBase *currentView() const;

    ViewBase *createIntroductionView();

private Q_SLOTS:
    void slotActionDestroyed(QObject *o);
    void slotViewListItemRemoved(KPlato::ViewListItem *item);
    void slotViewListItemInserted(KPlato::ViewListItem *item, KPlato::ViewListItem *parent, int index);

    void slotProjectEditFinished(int result);
    void slotTaskEditFinished(int result);
    void slotSummaryTaskEditFinished(int result);
    void slotEditResourceFinished(int result);
    void slotProjectWorktimeFinished(int result);
    void slotDefineWBSFinished(int result);
    void slotCurrencyConfigFinished(int result);
    void slotInsertFileFinished(int result);
    void slotAddSubTaskFinished(int result);
    void slotAddTaskFinished(int result);
    void slotAddSubMilestoneFinished(int result);
    void slotAddMilestoneFinished(int result);
    void slotTaskProgressFinished(int result);
    void slotMilestoneProgressFinished(int result);
    void slotTaskDescriptionFinished(int result);
    void slotDocumentsFinished(int result);
    void slotAddRelationFinished(int result);
    void slotModifyRelationFinished(int result);
    void slotReportDesignFinished(int result);
    void slotOpenReportFileFinished(int result);
    void slotCreateViewFinished(int result);
    void slotLoadSharedProjectsFinished(int result);
    void openWorkPackageMergeDialog();
    void workPackageMergeDialogFinished(int result);
    void slotRemoveCommands();

    void initiateViews();
    void slotViewScheduleManager(KPlato::ScheduleManager *sm);

private:
    void createViews();
    ViewBase *createView(ViewListItem *cat, const QString &type, const QString &tag, const QString &name, const QString &tip, int index = -1);

    QString standardTaskStatusReport() const;

private:
    QSplitter *m_sp;
    QStackedWidget *m_tab;

    ViewListWidget *m_viewlist;
    ViewListItem *m_viewlistItem; // requested popupmenu item

    //QDockWidget *m_toolbox;

    int m_viewGrp;
    int m_defaultFontSize;
    int m_currentEstimateType;

    bool m_updateAccountsview;
    bool m_updatePertEditor;

    QLabel *m_estlabel;
    QToolButton *m_workPackageButton;

    ViewAdaptor* m_dbus;

    QActionGroup *m_scheduleActionGroup;
    QMap<QAction*, ScheduleManager*> m_scheduleActions;

    QMultiMap<ScheduleManager*, CalculateScheduleCmd*> m_calculationcommands;
    QList<KUndo2Command*> m_undocommands;

    bool m_readWrite;
    int m_defaultView;
    QList<int> m_visitedViews;

    QList<DockWidget*> m_dockers;

    // ------ File
    QAction *actionCreateTemplate;
    QAction *actionCreateNewProject;

    // ------ Edit
    QAction *actionCut;
    QAction *actionCopy;
    QAction *actionPaste;

    // ------ View
    KToggleAction *actionViewSelector;

    // ------ Insert
    // ------ Project
    QAction *actionEditMainProject;

    // ------ Tools
    QAction *actionEditStandardWorktime;
    QAction *actionDefineWBS;
    QAction *actionInsertFile;
    QAction *actionCurrencyConfig;
    QAction *actionLoadSharedProjects;
    QAction *actionOpenReportFile;

    // ------ Settings
    QAction *actionConfigure;

    // ------ Help
    QAction *actionIntroduction;

    // ------ Popup
    QAction *actionOpenNode;
    QAction *actionTaskProgress;
    QAction *actionTaskDescription;
    QAction *actionDocuments;
    QAction *actionDeleteTask;
    QAction *actionIndentTask;
    QAction *actionUnindentTask;
    QAction *actionMoveTaskUp;
    QAction *actionMoveTaskDown;

    QAction *actionEditResource;
    QAction *actionEditRelation;
    QAction *actionDeleteRelation;

    //Test
    QAction *actNoInformation;

    QMap<ViewListItem*, QAction*> m_reportActionMap;

    KoPart *m_partpart;
};


} //Kplato namespace

#endif
