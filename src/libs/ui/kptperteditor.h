/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2007 Florian Piquemal <flotueur@yahoo.fr>
  SPDX-FileCopyrightText: 2007 Alexis Ménard <darktears31@gmail.com>
  SPDX-FileCopyrightText: 2007 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTPERTEDITOR_H
#define KPTPERTEDITOR_H

#include "planui_export.h"

#include "kptviewbase.h"
#include "kptitemmodelbase.h"
#include "kpttaskeditor.h"
#include <ui_kptperteditor.h>

#include "kptcommand.h"
#include "kptnode.h"

#include <QList>


class KoDocument;

class QTreeWidgetItem;
class QTableWidgetItem;
class QModelIndex;
class KUndo2Command;

namespace KPlato
{

class View;
class Project;
class RelationTreeView;

class PLANUI_EXPORT PertEditor : public ViewBase
{
    Q_OBJECT
public:

    enum Roles { NodeRole = Qt::UserRole + 1, EnabledRole };
    
    PertEditor(KoPart *part, KoDocument *doc, QWidget *parent);
    void updateReadWrite(bool readwrite) override;
    void setProject(Project *project) override;
    Project *project() const override { return m_project; }
    void draw(Project &project) override;
    void draw() override;
    void drawSubTasksName(QTreeWidgetItem *parent,Node * currentNode);
    void clearRequiredList();
    void loadRequiredTasksList(Node * taskNode);
    Node *itemToNode(QTreeWidgetItem *item);
    QTreeWidgetItem *nodeToItem(Node *node, QTreeWidgetItem *item);
    QList<Node*> listNodeNotView(Node * node);

    void updateAvailableTasks(QTreeWidgetItem *item = nullptr);
    void setAvailableItemEnabled(QTreeWidgetItem *item);
    void setAvailableItemEnabled(Node *node);
    
Q_SIGNALS:
    void executeCommand(KUndo2Command*);

protected:
    bool isInRequiredList(Node *node);
    QTreeWidgetItem *findNodeItem(Node *node, QTreeWidgetItem *item);
    QTableWidgetItem *findRequiredItem(Node *node);
    
private Q_SLOTS:
    void slotNodeAdded(KPlato::Node*);
    void slotNodeRemoved(KPlato::Node*);
    void slotNodeMoved(KPlato::Node*);
    void slotNodeChanged(KPlato::Node*);
    void slotRelationAdded(KPlato::Relation *rel);
    void slotRelationRemoved(KPlato::Relation *rel);
    
    void dispAvailableTasks();
    void dispAvailableTasks(KPlato::Node *parent, KPlato::Node *selectedTask);
    void dispAvailableTasks(KPlato::Relation *rel);
    void addTaskInRequiredList(QTreeWidgetItem * currentItem);
    void removeTaskFromRequiredList();
    void slotUpdate();

    void slotCurrentTaskChanged(QTreeWidgetItem *curr, QTreeWidgetItem *prev);
    void slotAvailableChanged(QTreeWidgetItem *item);
    void slotRequiredChanged(const QModelIndex &index);
    void slotAddClicked();
    void slotRemoveClicked();

private:
    Project * m_project;
    QTreeWidget *m_tasktree;
    QTreeWidget *m_availableList;
    RelationTreeView *m_requiredList;
    
    Ui::PertEditor widget;
};

}  //KPlato namespace

#endif
