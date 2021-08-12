/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2012 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTPRINTINGCONTROLPRIVATE_H
#define KPTPRINTINGCONTROLPRIVATE_H

#include <QObject>

class QPrintDialog;

namespace KPlato
{
class PrintingDialog;

class PrintingControlPrivate : public QObject
{
    Q_OBJECT
public:
    PrintingControlPrivate(PrintingDialog *job, QPrintDialog *dia);
    ~PrintingControlPrivate() override {}
public Q_SLOTS:
    void slotChanged();
private:
    PrintingDialog *m_job;
    QPrintDialog *m_dia;
};

}

#endif //KPTPRINTINGCONTROLPRIVATE_H
