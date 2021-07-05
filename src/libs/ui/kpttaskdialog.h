/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2002 Bo Thorsen bo @sonofthor.dk
   SPDX-FileCopyrightText: 2004-2010 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTTASKDIALOG_H
#define KPTTASKDIALOG_H

#include "planui_export.h"

#include <kpagedialog.h>


namespace KPlato
{

class Accounts;
class TaskGeneralPanel;
class RequestResourcesPanel;
class DocumentsPanel;
class TaskCostPanel;
class TaskDescriptionPanel;
class Node;
class Task;
class Project;
class MacroCommand;

/**
 * The dialog that shows and allows you to alter any task.
 */
class PLANUI_EXPORT TaskDialog : public KPageDialog {
    Q_OBJECT
public:
    /**
     * The constructor for the task settings dialog.
     * @param project the project to show
     * @param task the task to show
     * @param accounts all defined accounts
     * @param parent parent widget
     */
    TaskDialog(Project &project, Task &task, Accounts &accounts, QWidget *parent=nullptr);

    virtual MacroCommand *buildCommand();

protected Q_SLOTS:
    void accept() override;
    void setButtonOkEnabled(bool enabled);

    void slotTaskRemoved(KPlato::Node *node);
    void slotCurrentChanged(KPageWidgetItem*, KPageWidgetItem*);

protected:
    Project &m_project;
    Node *m_node;

    TaskGeneralPanel *m_generalTab;
    RequestResourcesPanel *m_resourcesTab;
    DocumentsPanel *m_documentsTab;
    TaskCostPanel *m_costTab;
    TaskDescriptionPanel *m_descriptionTab;
};

class PLANUI_EXPORT TaskAddDialog : public TaskDialog {
    Q_OBJECT
public:
    TaskAddDialog(Project &project, Task &task, Node *currentNode, Accounts &accounts, QWidget *parent=nullptr);
    ~TaskAddDialog() override;

    MacroCommand *buildCommand() override;

protected Q_SLOTS:
    void slotNodeRemoved(KPlato::Node*);

private:
    Node *m_currentnode;
};

class PLANUI_EXPORT SubTaskAddDialog : public TaskDialog {
    Q_OBJECT
public:
    SubTaskAddDialog(Project &project, Task &task, Node *currentNode, Accounts &accounts, QWidget *parent=nullptr);
    ~SubTaskAddDialog() override;

    MacroCommand *buildCommand() override;

protected Q_SLOTS:
    void slotNodeRemoved(KPlato::Node*);

private:
    Node *m_currentnode;
};

} //KPlato namespace

#endif // TASKDIALOG_H
