/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2009 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptworkpackageconfigpanel.h"

#include "kptduration.h"
#include "kptmycombobox_p.h"
#include "calligraplansettings.h"

namespace KPlato
{

WorkPackageConfigPanel::WorkPackageConfigPanel(QWidget *p)
    : QWidget(p)
{

    setupUi(this);

    kcfg_RetrieveUrl->setMode(KFile::Directory);
    kcfg_SaveUrl->setMode(KFile::Directory);

    // Disable publish for now
    // FIXME: Enable when fully implemented
    ui_publishGroup->hide();
}

}  //KPlato namespace
