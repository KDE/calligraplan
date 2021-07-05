/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2003-2007 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTRECALCULATEDIALOG_H
#define KPTRECALCULATEDIALOG_H

#include "planui_export.h"

#include "ui_kptrecalculatedialog.h"

#include <KoDialog.h>

namespace KPlato
{

class RecalculateDialogImpl : public QWidget, public Ui_RecalculateDialog
{
    Q_OBJECT
public:
    explicit RecalculateDialogImpl (QWidget *parent);

};

class PLANUI_EXPORT RecalculateDialog : public KoDialog
{
    Q_OBJECT
public:
    explicit RecalculateDialog(QWidget *parent = nullptr);

    QDateTime dateTime() const;

private:
    RecalculateDialogImpl *dia;
};

} //KPlato namespace

#endif
