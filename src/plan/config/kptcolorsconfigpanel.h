/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2011 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTCOLORSCONFIGPANEL_H
#define KPTCOLORSCONFIGPANEL_H

#include "ui_kptcolorsconfigpanel.h"


namespace KPlato
{

class ColorsConfigPanel : public QWidget, public Ui_ColorsConfigPanel
{
    Q_OBJECT
public:
    explicit ColorsConfigPanel(QWidget *parent = nullptr);

};

} //KPlato namespace

#endif // KPTCOLORSCONFIGPANEL_H
