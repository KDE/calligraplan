/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005-2010 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kpttaskprogressdialog.h"
#include "kpttaskprogresspanel.h"
#include "kptcommand.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptnode.h"

#include <KLocalizedString>

namespace KPlato
{

TaskProgressDialog::TaskProgressDialog(Task &task, ScheduleManager *sm, StandardWorktime *workTime, QWidget *p)
    : KoDialog(p),
    m_node(&task)
{
    setCaption(i18n("Task Progress"));
    setButtons(Ok|Cancel);
    setDefaultButton(Ok);
    showButtonSeparator(true);
    m_panel = new TaskProgressPanel(task, sm, workTime, this);

    setMainWidget(m_panel);

    enableButtonOk(false);

    connect(m_panel, &TaskProgressPanelImpl::changed, this, &TaskProgressDialog::slotChanged);
    Project *proj = static_cast<Project*>(task.projectNode());
    if (proj) {
        connect(proj, &Project::nodeRemoved, this, &TaskProgressDialog::slotNodeRemoved);
    }
    setWhatsThis(
              xi18nc("@info:whatsthis",
                     "<title>Edit Task Progress</title>"
                     "<para>"
                     "This dialog consists of the following parts:"
                     "<list>"
                     "<item>Edit mode control to define the behavior of the dialog.</item>"
                     "<item>Controls to mark the task as started and finished.</item>"
                     "<item>Table to enter used effort for each resource.</item>"
                     "<item>Table for entry of task completion and remaining effort.</item>"
                     "</list></para><para>"
                     "Edit modes:"
                     "<list>"
                     "<item><emphasis>Per resource</emphasis> requires you to enter used effort for each resource assigned to this task."
                     " This enables the most detailed tracking of effort- and cost usage.</item>"
                     "<item><emphasis>Per task</emphasis> enables you to enter the minimum of information. When completion is changed,"
                     " used effort and remaining effort is automatically calculated, but can of course be manually modified if needed.</item>"
                     "</list>"
                     "<note>You should select the desired edit mode when starting the task.</note>"
                     "<nl/><link url='%1'>More...</link>"
                     "</para>", QStringLiteral("plan:task-progress-dialog")));

}

void TaskProgressDialog::slotNodeRemoved(Node *node)
{
    if (m_node == node) {
        reject();
    }
}

void TaskProgressDialog::slotChanged() {
    enableButtonOk(true);
}

MacroCommand *TaskProgressDialog::buildCommand() {
    MacroCommand *m = new MacroCommand(kundo2_i18n("Modify Task Progress"));
    bool modified = false;
    MacroCommand *cmd = m_panel->buildCommand();
    if (cmd) {
        m->addCommand(cmd);
        modified = true;
    }
    if (!modified) {
        delete m;
        return nullptr;
    }
    return m;
}


}  //KPlato namespace
