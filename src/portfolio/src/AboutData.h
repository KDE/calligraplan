/* This file is part of the KDE project
   Copyright (C) 1998 - 2001 Reginald Stadlbauer <reggie@kde.org>
   Copyright (C) 2004 - 2011 Dag Andersen <dag.andersen@kdemail.net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#ifndef PLANPORTFOLIO_ABOUTDATA_H
#define PLANPORTFOLIO_ABOUTDATA_H

#include "config.h"

#include <KAboutData>
#include <KLocalizedString>
#include <kcoreaddons_version.h>

namespace Portfolio
{

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
    aboutData->setDesktopFileName(QStringLiteral("org.kde.calligraplan"));

    return aboutData;
}
} // namespace Portfolio
#endif
