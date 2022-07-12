/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2007, 2012 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "kptdocumentsdialog.h"
#include "kptdocumentspanel.h"
#include "kptnode.h"
#include "kptcommand.h"

#include <KLocalizedString>
#include <KActionCollection>

using namespace KPlato;

DocumentsDialog::DocumentsDialog(Node &node, QWidget *p, bool readOnly)
    : KoDialog(p)
    , m_panel(new DocumentsPanel(node, this))
{
    m_panel->addNodeName();
    setCaption(i18n("Task Documents"));
    if (readOnly) {
        setButtons(Close);
    } else {
        setButtons(Ok|Cancel);
        setDefaultButton(Ok);
    }
    showButtonSeparator(true);

    setMainWidget(m_panel);

    enableButtonOk(false);

    connect(m_panel, &DocumentsPanel::changed, this, &KoDialog::enableButtonOk);
}

MacroCommand *DocumentsDialog::buildCommand()
{
    return m_panel->buildCommand();
}

void DocumentsDialog::slotButtonClicked(int button)
{
    if (button == KoDialog::Ok) {
        accept();
    } else {
        KoDialog::slotButtonClicked(button);
    }
}
