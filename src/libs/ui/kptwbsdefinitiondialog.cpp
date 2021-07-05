/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptwbsdefinitiondialog.h"
#include "kptwbsdefinitionpanel.h"
#include "kptwbsdefinition.h"
#include <kptcommand.h>

#include <KLocalizedString>


namespace KPlato
{

WBSDefinitionDialog::WBSDefinitionDialog(Project &project, WBSDefinition &def, QWidget *p)
    : KoDialog(p)
{
    setCaption(i18n("WBS Definition"));
    setButtons(Ok|Cancel);
    setDefaultButton(Ok);
    showButtonSeparator(true);

    m_panel = new WBSDefinitionPanel(project, def, this);
    setMainWidget(m_panel);
    enableButtonOk(false);
    connect(m_panel, &WBSDefinitionPanel::changed, this, &KoDialog::enableButtonOk);
    connect(this, &KoDialog::okClicked, this, &WBSDefinitionDialog::slotOk);
}


KUndo2Command *WBSDefinitionDialog::buildCommand() {
    return m_panel->buildCommand();
}

void WBSDefinitionDialog::slotOk() {
    if (!m_panel->ok())
        return;
    accept();
}


}  //KPlato namespace
