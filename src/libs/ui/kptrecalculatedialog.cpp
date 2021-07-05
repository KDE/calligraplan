/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2003-2007 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptrecalculatedialog.h"


namespace KPlato
{

RecalculateDialogImpl::RecalculateDialogImpl (QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);

    // Set tomorrow as default
    QDateTime ct = QDateTime::currentDateTime().addDays(1);
    ct.setTime(QTime());
    dateTimeEdit->setDateTime(ct);
    btnCurrent->setChecked(true);
    dateTimeEdit->setEnabled(false);
    connect(btnFrom, &QAbstractButton::toggled, dateTimeEdit, &QWidget::setEnabled);
}



//////////////////  ResourceDialog  ////////////////////////

RecalculateDialog::RecalculateDialog(QWidget *parent)
    : KoDialog(parent)
{
    setCaption(i18n("Re-calculate Schedule"));
    setButtons(Ok|Cancel);
    setDefaultButton(Ok);
    showButtonSeparator(true);
    dia = new RecalculateDialogImpl(this);
    setMainWidget(dia);
}

QDateTime RecalculateDialog::dateTime() const {
    return dia->btnFrom->isChecked() ? dia->dateTimeEdit->dateTime() : QDateTime::currentDateTime();
}


}  //KPlato namespace
