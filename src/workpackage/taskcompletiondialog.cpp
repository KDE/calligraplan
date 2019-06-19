/* This file is part of the KDE project
   Copyright (C) 2009, 2011, 2012 Dag Andersen <danders@get2net.dk>
   Copyright (C) 2019 Dag Andersen <danders@get2net.dk>
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
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


TaskCompletionDialog::TaskCompletionDialog(WorkPackage &package, ScheduleManager *sm, QWidget *parent)
    : KoDialog(parent)
{
    setCaption(i18n("Task Progress") );
    setButtons( Ok|Cancel );
    setDefaultButton( Ok );
    showButtonSeparator( true );
    m_panel = new TaskCompletionPanel( package, sm, this);

    setMainWidget(m_panel);

    enableButtonOk(false);

    connect(m_panel, &TaskCompletionPanel::changed, this, &TaskCompletionDialog::slotChanged);
}

void TaskCompletionDialog::slotChanged( bool state )
{
    enableButtonOk( state );
}

KUndo2Command *TaskCompletionDialog::buildCommand()
{
    //debugPlanWork;
    return m_panel->buildCommand();
}

TaskCompletionPanel::TaskCompletionPanel(WorkPackage &package, ScheduleManager *sm, QWidget *parent)
    : QWidget(parent)
{
    Q_ASSERT(sm);
    if (package.task()->completion().entrymode() != KPlato::Completion::EnterEffortPerResource) {
        package.task()->completion().setEntrymode(KPlato::Completion::EnterEffortPerResource);
    }
    if (package.task()->completion().resources().isEmpty()) {
        foreach (Resource *r, package.task()->assignedResources(sm->scheduleId())) {
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
    emit changed( true ); //FIXME
}

