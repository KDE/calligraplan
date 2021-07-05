/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2002 Bo Thorsen bo @sonofthor.dk
   SPDX-FileCopyrightText: 2004-2010 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptsummarytaskdialog.h"
#include "kptsummarytaskgeneralpanel.h"
#include "kptcommand.h"
#include "kptnode.h"
#include "kpttask.h"
#include "kptproject.h"

#include <KLocalizedString>


namespace KPlato
{

SummaryTaskDialog::SummaryTaskDialog(Task &task, QWidget *p)
    : KoDialog(p),
    m_node(&task)
{
    setCaption(i18n("Summary Task Settings"));
    setButtons(Ok|Cancel);
    setDefaultButton(Ok);
    showButtonSeparator(true);
    m_generalTab = new SummaryTaskGeneralPanel(task, this);
    setMainWidget(m_generalTab);
    enableButtonOk(false);

    connect(m_generalTab, &SummaryTaskGeneralPanel::obligatedFieldsFilled, this, &KoDialog::enableButtonOk);

    Project *proj = static_cast<Project*>(task.projectNode());
    if (proj) {
        connect(proj, &Project::nodeRemoved, this, &SummaryTaskDialog::slotTaskRemoved);
    }
}

void SummaryTaskDialog::slotTaskRemoved(Node *node)
{
    if (node == m_node) {
        reject();
    }
}


MacroCommand *SummaryTaskDialog::buildCommand() {
    MacroCommand *m = new MacroCommand(kundo2_i18n("Modify Summary Task"));
    bool modified = false;
    MacroCommand *cmd = m_generalTab->buildCommand();
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

void SummaryTaskDialog::slotButtonClicked(int button) {
    if (button == KoDialog::Ok) {
        if (!m_generalTab->ok())
            return;
        accept();
    } else {
        KoDialog::slotButtonClicked(button);
    }
}


}  //KPlato namespace
