/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2002 Bo Thorsen bo @sonofthor.dk
   SPDX-FileCopyrightText: 2004-2010 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "TasksEditController.h"
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

TasksEditController::TasksEditController(Project &project, const QList<Task*> &tasks, QWidget *p)
    : QObject(p)
    , m_project(project)
    , m_tasks(tasks)
    , m_dlg(nullptr)
{
}

TasksEditController::~TasksEditController()
{
    delete m_dlg;
}

void TasksEditController::activate()
{
    m_dlg = new TasksEditDialog(m_project, m_tasks, qobject_cast<QWidget*>(parent()));
    connect(m_dlg, &QDialog::finished, this, &TasksEditController::finish);
    m_dlg->open();
}

void TasksEditController::finish(int result)
{
    if (!m_dlg || sender() != m_dlg) {
        return;
    }
    if (result == QDialog::Accepted) {
        MacroCommand *m = m_dlg->buildCommand();
        if (m) {
            Q_EMIT addCommand(m);
        }
    }
    m_dlg->hide();
    deleteLater();
}

}  //KPlato namespace
