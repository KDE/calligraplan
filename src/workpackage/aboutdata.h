/* This file is part of the KDE project
   SPDX-FileCopyrightText: 1998-2001 Reginald Stadlbauer <reggie@kde.org>
   SPDX-FileCopyrightText: 2007-2011 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPLATOWORK_ABOUTDATA
#define KPLATOWORK_ABOUTDATA

#include <KAboutData>
#include <KLocalizedString>
#include <kcoreaddons_version.h>

#include <config.h>

namespace KPlatoWork
{

static const char PLANWORK_DESCRIPTION[] = "PlanWork - Work Package handler for the Plan Project Planning Tool";
static const char PLANWORK_VERSION[] = PLAN_VERSION_STRING;

KAboutData * newAboutData()
{
    KAboutData * aboutData = new KAboutData(
        QStringLiteral("calligraplanwork"),
        i18nc("application name", "Plan WorkPackage Handler"),
        QStringLiteral(PLAN_VERSION_STRING),
        i18n("PlanWork - Work Package handler for the Plan Project Planning Tool"),
        KAboutLicense::GPL,
        i18n("Copyright 1998-%1, The Plan Team", QStringLiteral(PLAN_YEAR)),
        QString(),
        QStringLiteral("https://www.calligra.org/plan/"));

    aboutData->addAuthor(i18n("Dag Andersen"), QString(), QStringLiteral("dag.andersen@kdemail.net"));
    // standard ki18n translator strings
    aboutData->setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"),
                             i18nc("EMAIL OF TRANSLATORS", "Your emails"));

    aboutData->setProductName("calligraplan/work");
    aboutData->setDesktopFileName(QStringLiteral("org.kde.calligraplanwork"));

    return aboutData;
}

}  //KPlatoWork namespace

#endif
