/* This file is part of the KDE project
  SPDX-FileCopyrightText: 1998, 1999, 2000 Torben Weis <weis@kde.org>
  SPDX-FileCopyrightText: 2002-2010 Dag Andersen <dag.andersen@kdemail.net>
  SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
  
  SPDX-License-Identifier: LGPL-2.0-or-later
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
    void slotEditCut();
    void slotEditCopy();
    void slotEditPaste();
    void slotRefreshView();
    void slotViewSelector(bool show);

    void slotProjectEdit();
    void slotDefineWBS();
    void slotCurrencyConfig();

    void slotCreateView();

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
    /// Publish workpackages
    void slotPublishWorkpackages(const QList<KPlato::Node*> &nodes, KPlato::Resource *resource, bool mailTo);

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

    void slotProjectWorktime();

    void slotOpenCurrentNode();
    void slotOpenNode(KPlato::Node *node);
    void slotOpenProjectDescription();

    void slotConnectNode();
    void slotCurrentChanged(int);
    void slotSelectDefaultView();

    void slotInsertResourcesFile(const QString &file);
    void slotInsertFile();
    void slotUpdateSharedResources();

    void slotWorkPackageLoaded();

    void createReportView(const QDomDocument &doc);

    void saveTaskModule(const QUrl &url, KPlato::Project *project);
    void removeTaskModule(const QUrl &url);

    void slotCreateReportTemplate();

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

private Q_SLOTS:
    void slotActionDestroyed(QObject *o);
    void slotViewListItemRemoved(KPlato::ViewListItem *item);
    void slotViewListItemInserted(KPlato::ViewListItem *item, KPlato::ViewListItem *parent, int index);

    void slotProjectEditFinished(int result);
    void slotProjectDescriptionFinished(int result);
    void slotProjectWorktimeFinished(int result);
    void slotDefineWBSFinished(int result);
    void slotCurrencyConfigFinished(int result);
    void slotInsertFileFinished(int result);

    void slotReportDesignFinished(int result);
    void slotOpenReportFileFinished(int result);
    void slotCreateViewFinished(int result);
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

    // ------ Project
    QAction *actionEditMainProject;

    // ------ Tools
    QAction *actionEditStandardWorktime;
    QAction *actionDefineWBS;
    QAction *actionInsertFile;
    QAction *actionCurrencyConfig;
    QAction *actionOpenReportFile;
    QAction *actionCreateReportTemplate;

    // ------ Settings
    QAction *actionConfigure;

    //Test
    QAction *actNoInformation;

    QMap<ViewListItem*, QAction*> m_reportActionMap;

    KoPart *m_partpart;
};


} //Kplato namespace

#endif
