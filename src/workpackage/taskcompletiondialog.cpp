/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2009, 2011, 2012 Dag Andersen <dag.andersen@kdemail.net>
   SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
   
   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "taskcompletiondialog.h"
#include "workpackage.h"

#include "kpttaskprogresspanel.h"
#include "kptusedefforteditor.h"
#include "kptcommand.h"
#include "kptitemmodelbase.h"

#include <KoIcon.h>

#include <KLocalizedString>

#include <QComboBox>
#include <QVBoxLayout>

#include "debugarea.h"

using namespace KPlatoWork;


TaskCompletionDialog::TaskCompletionDialog(KPlatoWork::WorkPackage &package, KPlato::ScheduleManager *sm, QWidget *parent)
    : KoDialog(parent)
{
    setCaption(i18n("Task Progress"));
    setButtons(Ok|Cancel);
    setDefaultButton(Ok);
    showButtonSeparator(true);
    m_panel = new TaskCompletionPanel(package, sm, this);

    setMainWidget(m_panel);

    enableButtonOk(false);

    connect(m_panel, &TaskCompletionPanel::changed, this, &TaskCompletionDialog::slotChanged);
}

void TaskCompletionDialog::slotChanged(bool state)
{
    enableButtonOk(state);
}

KUndo2Command *TaskCompletionDialog::buildCommand()
{
    //debugPlanWork;
    return m_panel->buildCommand();
}

TaskCompletionPanel::TaskCompletionPanel(WorkPackage &package, KPlato::ScheduleManager *sm, QWidget *parent)
    : QWidget(parent)
{
    Q_ASSERT(sm);
    if (package.task()->completion().entrymode() != KPlato::Completion::EnterEffortPerResource) {
        package.task()->completion().setEntrymode(KPlato::Completion::EnterEffortPerResource);
    }
    if (package.task()->completion().resources().isEmpty()) {
        const QList<KPlato::Resource*> resources = package.task()->assignedResources(sm->scheduleId());
        for (KPlato::Resource *r : resources) {
            if (r->id() == package.task()->workPackage().ownerId()) {
                package.task()->completion().addUsedEffort(r);
            }
        }
    }
    QVBoxLayout *l = new QVBoxLayout(this);
    m_panel = new KPlato::TaskProgressPanel(*(package.task()), sm, nullptr, this);
    m_panel->editModeWidget->setVisible(false);
    m_panel->addResourceWidget->setVisible(false);
    m_panel->resourceTable->verticalHeader()->hide();
    QSize size = m_panel->resourceTable->sizeHint();
    size.setHeight(120);
    m_panel->resourceTable->setSizeHint(size);
    l->addWidget(m_panel);
    connect(m_panel, &KPlato::TaskProgressPanelImpl::changed, this, &TaskCompletionPanel::slotChanged);
}

KUndo2Command *TaskCompletionPanel::buildCommand()
{
    return m_panel->buildCommand();
}

void TaskCompletionPanel::slotChanged()
{
    Q_EMIT changed(true); //FIXME
}

