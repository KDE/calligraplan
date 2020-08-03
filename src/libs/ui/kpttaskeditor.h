/* This file is part of the KDE project
  Copyright (C) 2006 -20010 Dag Andersen <dag.andersen@kdemail.net>
  Copyright (C) 2019 Dag Andersen <dag.andersen@kdemail.net>
  
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version..

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
* Boston, MA 02110-1301, USA.
*/

#ifndef KPTTASKEDITOR_H
#define KPTTASKEDITOR_H

#include "planui_export.h"

#include "kptglobal.h"
#include "kptnodeitemmodel.h"
#include "kptviewbase.h"

class KoDocument;

class KActionMenu;

namespace KPlato
{

class Project;
class Node;
class NodeItemModel;
class MacroCommand;

class PLANUI_EXPORT TaskEditorItemModel : public NodeItemModel
{
    Q_OBJECT
public:
    explicit TaskEditorItemModel(QObject *parent = 0);
    
    Qt::ItemFlags flags(const QModelIndex & index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override; 
    bool setData(const QModelIndex & index, const QVariant &value, int role = Qt::EditRole) override;

public Q_SLOTS:
    void setScheduleManager(KPlato::ScheduleManager *sm) override;

protected:
    QVariant type(const Node *node, int role) const;
    bool setType(Node *node, const QVariant &value, int role) override;

};

class PLANUI_EXPORT TaskEditorTreeView : public DoubleTreeViewBase
{
    Q_OBJECT
public:
    explicit TaskEditorTreeView(QWidget *parent);

    //void setSelectionModel(QItemSelectionModel *selectionModel);

    NodeItemModel *baseModel() const;
    NodeSortFilterProxyModel *proxyModel() const { return qobject_cast<NodeSortFilterProxyModel*>(model()); }

    Project *project() const { return baseModel()->project(); }
    void setProject(Project *project) { baseModel()->setProject(project); }

    void editPaste() override;

Q_SIGNALS:
    void currentColumnChanged(const QModelIndex&, const QModelIndex&);

protected Q_SLOTS:
    void slotDropAllowed(const QModelIndex &index, int dropIndicatorPosition, QDragMoveEvent *event);
};

class PLANUI_EXPORT NodeTreeView : public DoubleTreeViewBase
{
    Q_OBJECT
public:
    explicit NodeTreeView(QWidget *parent);

    //void setSelectionModel(QItemSelectionModel *selectionModel);

    NodeItemModel *baseModel() const;
    NodeSortFilterProxyModel *proxyModel() const { return qobject_cast<NodeSortFilterProxyModel*>(model()); }

    Project *project() const { return baseModel()->project(); }
    void setProject(Project *project) { baseModel()->setProject(project); }
    
Q_SIGNALS:
    void currentColumnChanged(const QModelIndex&, const QModelIndex&);

protected Q_SLOTS:
    void slotDropAllowed(const QModelIndex &index, int dropIndicatorPosition, QDragMoveEvent *event);
};

class PLANUI_EXPORT TaskEditor : public ViewBase
{
    Q_OBJECT
public:
    TaskEditor(KoPart *part, KoDocument *doc, QWidget *parent);
    
    void setupGui();
    void setProject(Project *project) override;
    Project *project() const override { return m_view->project(); }

    Node *currentNode() const override;
    QList<Node*> selectedNodes() const ;
    Node *selectedNode() const;

    void updateReadWrite(bool readwrite) override;

    NodeItemModel *baseModel() const { return m_view->baseModel(); }
    NodeSortFilterProxyModel *proxyModel() const { return m_view->proxyModel(); }
    QAbstractItemModel *model() const { return m_view->model(); }

    /// Loads context info into this view. Reimplement.
    bool loadContext(const KoXmlElement &/*context*/) override;
    /// Save context info from this view. Reimplement.
    void saveContext(QDomElement &/*context*/) const override;

    KoPrintJob *createPrintJob() override;

Q_SIGNALS:
    void taskSelected(KPlato::Task *task);
    void openNode();
    void addTask();
    void addMilestone();
    void addSubtask();
    void addSubMilestone();
    void deleteTaskList(const QList<KPlato::Node*>&);
    void moveTaskUp();
    void moveTaskDown();
    void indentTask();
    void unindentTask();

    void saveTaskModule(const QUrl &url, KPlato::Project *project);
    void removeTaskModule(const QUrl &url);

public Q_SLOTS:
    /// Activate/deactivate the gui
    void setGuiActive(bool activate) override;

    void setScheduleManager(KPlato::ScheduleManager *sm) override;

    void slotEditCopy() override;
    void slotEditPaste() override;

protected:
    void updateActionsEnabled(bool on);
    int selectedRowCount() const;
    QModelIndexList selectedRows() const;
    void editTasks(const QList<Task*> &tasks, const QPoint &pos);
    void createDockers();

protected Q_SLOTS:
    void slotOptions() override;
    void itemDoubleClicked(const QPersistentModelIndex &idx);

private Q_SLOTS:
    void slotSelectionChanged(const QModelIndexList&);
    void slotCurrentChanged(const QModelIndex&, const QModelIndex&);
    void slotContextMenuRequested(const QModelIndex &index, const QPoint& pos , const QModelIndexList &rows);

    void slotEnableActions();

    void slotAddTask();
    void slotAddSubtask();
    void slotAddMilestone();
    void slotAddSubMilestone();
    void slotDeleteTask();
    void slotLinkTask();
    void slotIndentTask();
    void slotUnindentTask();
    void slotMoveTaskUp();
    void slotMoveTaskDown();

    void slotSplitView();
    void slotProjectShown(bool);

    void taskModuleDoubleClicked(QModelIndex idx);

private:
    void edit(const QModelIndex &index);

private:
    TaskEditorTreeView *m_view;

    KActionMenu *menuAddTask;
    KActionMenu *menuAddSubTask;
    QAction *actionAddTask;
    QAction *actionAddMilestone;
    QAction *actionAddSubtask;
    QAction *actionAddSubMilestone;
    QAction *actionDeleteTask;
    QAction *actionLinkTask;
    QAction *actionMoveTaskUp;
    QAction *actionMoveTaskDown;
    QAction *actionIndentTask;
    QAction *actionUnindentTask;

    QAction *actionShowProject;
    QDomDocument m_domdoc;
};

class PLANUI_EXPORT TaskView : public ViewBase
{
    Q_OBJECT
public:
    TaskView(KoPart *part, KoDocument *doc, QWidget *parent);

    void setupGui();
    Project *project() const override { return m_view->project(); }
    void draw(Project &project) override;
    void draw() override;

    NodeItemModel *baseModel() const { return m_view->baseModel(); }
    NodeSortFilterProxyModel *proxyModel() const { return m_view->proxyModel(); }

    Node *currentNode() const override;
    QList<Node*> selectedNodes() const ;
    Node *selectedNode() const;

    void updateReadWrite(bool readwrite) override;

    /// Loads context info into this view. Reimplement.
    bool loadContext(const KoXmlElement &/*context*/) override;
    /// Save context info from this view. Reimplement.
    void saveContext(QDomElement &/*context*/) const override;

    KoPrintJob *createPrintJob() override;
    
Q_SIGNALS:
    void openNode();

public Q_SLOTS:
    /// Activate/deactivate the gui
    void setGuiActive(bool activate) override;

    void setScheduleManager(KPlato::ScheduleManager *sm) override;
    void slotEditCopy() override;

protected:
    void updateActionsEnabled(bool on);
    int selectedNodeCount() const;

protected Q_SLOTS:
    void slotOptions() override;
    void itemDoubleClicked(const QPersistentModelIndex &idx);

private Q_SLOTS:
    void slotSelectionChanged(const QModelIndexList&);
    void slotCurrentChanged(const QModelIndex&, const QModelIndex&);
    void slotContextMenuRequested(const QModelIndex &index, const QPoint& pos);

    void slotEnableActions();

    void slotSplitView();

private:
    NodeTreeView *m_view;
    QAction *actionShowProject;
    QDomDocument m_domdoc;
};

//-----------------------------------
class WorkPackageTreeView : public DoubleTreeViewBase
{
    Q_OBJECT
public:
    explicit WorkPackageTreeView(QWidget *parent);

    //void setSelectionModel(QItemSelectionModel *selectionModel);

    NodeItemModel *baseModel() const;
    WorkPackageProxyModel *proxyModel() const { return m; }

    Project *project() const { return baseModel()->project(); }
    void setProject(Project *project) { m->setProject(project); }

    ScheduleManager *scheduleManager() const { return baseModel()->manager(); }

Q_SIGNALS:
    void currentColumnChanged(const QModelIndex&, const QModelIndex&);

protected Q_SLOTS:
    void slotDropAllowed(const QModelIndex &index, int dropIndicatorPosition, QDragMoveEvent *event);
protected:
    WorkPackageProxyModel *m;
};

class PLANUI_EXPORT TaskWorkPackageView : public ViewBase
{
    Q_OBJECT
public:
    TaskWorkPackageView(KoPart *part, KoDocument *doc, QWidget *parent);

    void setupGui();
    Project *project() const override;
    void setProject(Project *project) override;
    ScheduleManager *scheduleManager() const override { return m_view->scheduleManager(); }

    WorkPackageProxyModel *proxyModel() const;

    Node *currentNode() const override;
    QList<Node*> selectedNodes() const ;
    Node *selectedNode() const;

    void updateReadWrite(bool readwrite) override;

    /// Loads context info into this view. Reimplement.
    bool loadContext(const KoXmlElement &/*context*/) override;
    /// Save context info from this view. Reimplement.
    void saveContext(QDomElement &/*context*/) const override;

    KoPrintJob *createPrintJob() override;

Q_SIGNALS:
    void mailWorkpackage(KPlato::Node *n, KPlato::Resource *r = 0);
    void publishWorkpackages(const QList<KPlato::Node*> &nodes, KPlato::Resource *r, bool mailTo);
    void openWorkpackages();
    void checkForWorkPackages(bool);
    void loadWorkPackageUrl(KPlato::Project *project, QList<QUrl> &urls);

public Q_SLOTS:
    /// Activate/deactivate the gui
    void setGuiActive(bool activate) override;
    void slotRefreshView() override;
    void setScheduleManager(KPlato::ScheduleManager *sm) override;
    void slotEditCopy() override;
    void slotWorkpackagesAvailable(bool value);

protected:
    void updateActionsEnabled(bool on);
    int selectedNodeCount() const;

protected Q_SLOTS:
    void slotOptions() override;
    void slotMailWorkpackage();
    void slotWorkPackageSent(const QList<KPlato::Node*> &nodes, KPlato::Resource *resource);
    void slotLoadWorkPackage(QList<QString>);

private Q_SLOTS:
    void slotSelectionChanged(const QModelIndexList&);
    void slotCurrentChanged(const QModelIndex&, const QModelIndex&);
    void slotContextMenuRequested(const QModelIndex &index, const QPoint& pos);
    void itemDoubleClicked(const QPersistentModelIndex &idx);

    void slotEnableActions();

    void slotSplitView();

private:
    WorkPackageTreeView *m_view;
    MacroCommand *m_cmd;

    QAction *actionMailWorkpackage;
    QAction *actionOpenWorkpackages;
};

} //namespace KPlato


#endif
