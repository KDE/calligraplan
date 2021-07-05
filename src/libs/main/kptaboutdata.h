/* This file is part of the KDE project
   SPDX-FileCopyrightText: 1998-2001 Reginald Stadlbauer <reggie@kde.org>
   SPDX-FileCopyrightText: 2004-2011 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTABOUTDATA_H
#define KPTABOUTDATA_H

#include "config.h"

#include <KAboutData>
#include <KLocalizedString>
#include <kcoreaddons_version.h>

namespace KPlato
{

KAboutData * newAboutData()
{
    KAboutData *aboutData = new KAboutData(
        QStringLiteral("calligraplan"),
        i18nc("application name", "Plan"),
        QStringLiteral(PLAN_VERSION_STRING),
        i18n("Project Planning and Management Tool"),
        KAboutLicense::GPL,
        i18n("Copyright 1998-%1, The Plan Team", QStringLiteral(PLAN_YEAR)),
        QString(),
        QStringLiteral("https://www.calligra.org/plan/"));

    aboutData->addAuthor(i18n("Dag Andersen"), QString(), "dag.andersen@kdemail.net");
    aboutData->addAuthor(i18n("Thomas Zander")); // please don't re-add, I don't like getting personal emails :)
    aboutData->addAuthor(i18n("Bo Thorsen"), QString(), "bo@sonofthor.dk");
    aboutData->addAuthor(i18n("Raphael Langerhorst"),QString(),"raphael.langerhorst@kdemail.net");

    // standard ki18n translator strings
    aboutData->setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"),
                             i18nc("EMAIL OF TRANSLATORS", "Your emails"));

    aboutData->setProductName("calligraplan"); // for bugs.kde.org
    aboutData->setOrganizationDomain("kde.org");
#if KCOREADDONS_VERSION >= 0x051600
    aboutData->setDesktopFileName(QStringLiteral("org.kde.calligraplan"));
#endif

    return aboutData;
}

}  //KPlato namespace

#endif
