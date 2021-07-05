/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2017 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "TasksEditDialog.h"
#include "kpttaskcostpanel.h"
#include "kpttaskgeneralpanel.h"
#include "kptrequestresourcespanel.h"
#include "kptdocumentspanel.h"
#include "kpttaskdescriptiondialog.h"
#include "kptcommand.h"
#include "kptnode.h"
#include "kpttask.h"
#include "kptproject.h"

#include <KoVBox.h>

#include <KLocalizedString>

namespace KPlato
{

TasksEditDialog::TasksEditDialog(Project &project, const QList<Task*> &tasks, QWidget *p)
    : KPageDialog(p)
    , m_project(project)
    , m_tasks(tasks)
{
    m_task = new Task(project.taskDefaults());

    setWindowTitle(i18n("Tasks Settings"));
    setFaceType(KPageDialog::Tabbed);

    KoVBox *page;

    // Create all the tabs.
    page =  new KoVBox();
    addPage(page, i18n("&Resources"));
    m_resourcesTab = new RequestResourcesPanel(page, project, *m_task);

    resize(size().expandedTo(QSize(200, 75)));
}

void TasksEditDialog::setButtonOkEnabled(bool enabled)
{
    buttonBox()->button(QDialogButtonBox::Ok)->setEnabled(enabled);
}

void TasksEditDialog::slotCurrentChanged(KPageWidgetItem *current, KPageWidgetItem *prev)
{
    Q_UNUSED(current)
    Q_UNUSED(prev)
}

void TasksEditDialog::slotTaskRemoved(Node *node)
{
    if (node->type() == Node::Type_Task && m_tasks.contains(static_cast<Task*>(node))) {
        reject();
    }
}

MacroCommand *TasksEditDialog::buildCommand()
{
    MacroCommand *m = new MacroCommand(kundo2_i18n("Modify tasks"));
    bool modified = false;

    for (Task *t : m_tasks) {
        MacroCommand *c = m_resourcesTab->buildCommand(t, true);
        if (c) {
            m->addCommand(c);
            modified = true;
        }
    }
    if (!modified) {
        delete m;
        m = nullptr;
    }
    return m;
}

void TasksEditDialog::accept()
{
    KPageDialog::accept();
}

}  //KPlato namespace
