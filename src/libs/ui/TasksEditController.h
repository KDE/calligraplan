/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2002 Bo Thorsen bo @sonofthor.dk
   SPDX-FileCopyrightText: 2004-2010 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef TASKSEDITCONTROLLER_H
#define TASKSEDITCONTROLLER_H

#include "planui_export.h"

#include <QObject>

class KUndo2Command;

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

class TasksEditDialog;

/**
 * The dialog that allows you to alter multiple tasks.
 */
class PLANUI_EXPORT TasksEditController : public QObject
{
    Q_OBJECT
public:
    TasksEditController(Project &project, const QList<Task*> &tasks, QWidget *parent = nullptr);
    ~TasksEditController() override;

public Q_SLOTS:
    void activate();

Q_SIGNALS:
    void addCommand(KUndo2Command *cmd);

private Q_SLOTS:
    void finish(int result);

private:
    Project &m_project;
    const QList<Task*> m_tasks;
    TasksEditDialog *m_dlg;
};


} //KPlato namespace

#endif // TASKSEDITCONTROLLER_H
