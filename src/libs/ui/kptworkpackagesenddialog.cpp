/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2007 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptworkpackagesenddialog.h"
#include "kptworkpackagesendpanel.h"
#include "kptdocumentspanel.h"
#include "kpttask.h"

#include <KLocalizedString>


using namespace KPlato;

WorkPackageSendDialog::WorkPackageSendDialog(const QList<Node*> &tasks,  ScheduleManager *sm, QWidget *p)
    : KoDialog(p)
{
    setCaption(xi18nc("@title:window", "Send Work Packages"));
    setButtons(Close);
    setDefaultButton(Close);
    showButtonSeparator(true);

    m_wp = new WorkPackageSendPanel(tasks, sm, this);
    setMainWidget(m_wp);
}

QSize WorkPackageSendDialog::sizeHint() const
{
    return QSize(350, 300);
}
