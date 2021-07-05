/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005-2007 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptmilestoneprogressdialog.h"
#include "kptmilestoneprogresspanel.h"

#include "kptnode.h"
#include "kptproject.h"

#include <KLocalizedString>


namespace KPlato
{

class MacroCommand;

MilestoneProgressDialog::MilestoneProgressDialog(Task &task, QWidget *p)
    : KoDialog(p),
    m_node(&task)
{
    setCaption(i18n("Milestone Progress"));
    setButtons(Ok|Cancel);
    setDefaultButton(Ok);
    showButtonSeparator(true);
    m_panel = new MilestoneProgressPanel(task, this);

    setMainWidget(m_panel);

    enableButtonOk(false);

    connect(m_panel, &MilestoneProgressPanelImpl::changed, this, &MilestoneProgressDialog::slotChanged);
    Project *proj = static_cast<Project*>(task.projectNode());
    if (proj) {
        connect(proj, &Project::nodeRemoved, this, &MilestoneProgressDialog::slotNodeRemoved);
    }
}

void MilestoneProgressDialog::slotNodeRemoved(Node *node)
{
    if (m_node == node) {
        reject();
    }
}

void MilestoneProgressDialog::slotChanged() {
    enableButtonOk(true);
}

MacroCommand *MilestoneProgressDialog::buildCommand() {
    return m_panel->buildCommand();
}


}  //KPlato namespace
