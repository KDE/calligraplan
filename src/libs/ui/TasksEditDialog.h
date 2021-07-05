/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2017 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef TASKSEDITDIALOGWIDGET_H
#define TASKSEDITDIALOGWIDGET_H

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
class PLANUI_EXPORT TasksEditDialog : public KPageDialog {
    Q_OBJECT
public:
    /**
     * The constructor for the tasks settings dialog.
     * @param project the project to use
     * @param tasks the list of tasks to be edited
     * @param parent parent widget
     */
    TasksEditDialog(Project &project, const QList<Task*> &tasks, QWidget *parent=nullptr);

    virtual MacroCommand *buildCommand();

protected Q_SLOTS:
    void accept() override;
    void setButtonOkEnabled(bool enabled);

    void slotTaskRemoved(KPlato::Node *node);
    void slotCurrentChanged(KPageWidgetItem*, KPageWidgetItem*);

protected:
    Project &m_project;
    const QList<Task*> m_tasks;
    Task *m_task;

    RequestResourcesPanel *m_resourcesTab;
};

} //KPlato namespace

#endif // TASKSEDITDIALOGWIDGET_H
