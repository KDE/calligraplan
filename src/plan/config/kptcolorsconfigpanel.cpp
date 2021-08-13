/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2011 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptcolorsconfigpanel.h"
#include "calligraplansettings.h"

namespace KPlato
{

ColorsConfigPanel::ColorsConfigPanel(QWidget *p)
    : QWidget(p)
{

    setupUi(this);

    kcfg_ColorGradientType->addItem(i18n("Linear"));
    kcfg_ColorGradientType->addItem(i18n("Flat"));
}

}  //KPlato namespace
