/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PLANPORTFOLIO_ABOUTDATA_H
#define PLANPORTFOLIO_ABOUTDATA_H

#include "config.h"

#include <KAboutData>
#include <KLocalizedString>
#include <kcoreaddons_version.h>


KAboutData * newAboutData()
{
    KAboutData *aboutData = new KAboutData(
        QStringLiteral("calligraplanportfolio"),
        i18nc("application name", "Plan Portfolio"),
        QStringLiteral(PLAN_VERSION_STRING),
        i18n("Project Portfolio Management"),
        KAboutLicense::GPL,
        i18n("Copyright 2020-%1, The Plan Team", QStringLiteral(PLAN_YEAR)),
        QString(),
        QStringLiteral("https://www.calligra.org/plan/"));

    aboutData->addAuthor(i18n("Dag Andersen"), QString(), "dag.andersen@kdemail.net");

    // standard ki18n translator strings
    aboutData->setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"),
                             i18nc("EMAIL OF TRANSLATORS", "Your emails"));

    aboutData->setProductName("calligraplanportfolio"); // for bugs.kde.org
    aboutData->setOrganizationDomain("kde.org");
    aboutData->setDesktopFileName(QStringLiteral("org.kde.calligraplanportfolio"));

    return aboutData;
}

#endif
