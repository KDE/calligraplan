/* This file is part of the KDE project
  SPDX-FileCopyrightText: 1998, 1999, 2000 Torben Weis <weis@kde.org>
  SPDX-FileCopyrightText: 2002-2011 Dag Andersen <dag.andersen@kdemail.net>
  SPDX-FileCopyrightText: 2012 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptprintingcontrolprivate.h"

#include <kptviewbase.h>

#include <QSpinBox>
#include <QPrintDialog>

namespace KPlato
{

PrintingControlPrivate::PrintingControlPrivate(PrintingDialog *job, QPrintDialog *dia)
    : QObject(dia),
    m_job(job),
    m_dia(dia)
{
    connect(job, SIGNAL(changed()), SLOT(slotChanged()));
}

void PrintingControlPrivate::slotChanged()
{
    if (! m_job || ! m_dia) {
        return;
    }
    QSpinBox *to = m_dia->findChild<QSpinBox*>(QStringLiteral("to"));
    QSpinBox *from = m_dia->findChild<QSpinBox*>(QStringLiteral("from"));
    if (to && from) {
        from->setMinimum(m_job->documentFirstPage());
        from->setMaximum(m_job->documentLastPage());
        from->setValue(from->minimum());
        to->setMinimum(from->minimum());
        to->setMaximum(from->maximum());
        to->setValue(to->maximum());
    }
}

}  //KPlato namespace
