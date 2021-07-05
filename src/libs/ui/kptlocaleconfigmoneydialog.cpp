/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2009 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptlocaleconfigmoneydialog.h"
#include "locale/localemon.h"

#include "kptcommand.h"
#include "kptlocale.h"


namespace KPlato
{

LocaleConfigMoneyDialog::LocaleConfigMoneyDialog(Locale *locale, QWidget *p)
    : KoDialog(p)
{
    setCaption(i18n("Currency Settings"));
    setButtons(Ok|Cancel);
    showButtonSeparator(true);
    m_panel = new LocaleConfigMoney(locale, this);

    setMainWidget(m_panel);

    enableButtonOk(false);

    connect(m_panel, &LocaleConfigMoney::localeChanged, this, &LocaleConfigMoneyDialog::slotChanged);
}

void LocaleConfigMoneyDialog::slotChanged() {
    enableButtonOk(true);
}

KUndo2Command *LocaleConfigMoneyDialog::buildCommand(Project &project) {
    MacroCommand *m = new ModifyProjectLocaleCmd(project, kundo2_i18n("Modify currency settings"));
    MacroCommand *cmd = m_panel->buildCommand();
    if (cmd) {
        m->addCommand(cmd);
    }
    if (m->isEmpty()) {
        delete m;
        return nullptr;
    }
    return m;
}


}  //KPlato namespace
