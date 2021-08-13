/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2009 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTWORKPACKAGECONFIGPANEL_H
#define KPTWORKPACKAGECONFIGPANEL_H

#include "ui_kptworkpackageconfigpanel.h"


namespace KPlato
{

class WorkPackageConfigPanel : public QWidget, public Ui_WorkPackageConfigPanel
{
    Q_OBJECT
public:
    explicit WorkPackageConfigPanel(QWidget *parent = nullptr);
};

} //KPlato namespace

#endif // KPTWORKPACKAGECONFIGPANEL_H
