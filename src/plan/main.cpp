/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001 Thomas zander <zander@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptfactory.h"
#include "kptaboutdata.h"
#include "kptmaindocument.h"
#include "calligraplansettings.h"

#include <KoApplication.h>
#include <Help.h>

#include <QApplication>
#include <QLoggingCategory>

int main(int argc, char **argv)
{
    /**
     * Disable debug output by default, only log warnings.
     * Debug logs can be controlled by the environment variable QT_LOGGING_RULES.
     *
     * For example, to get full debug output, run the following:
     * QT_LOGGING_RULES="calligra.*=true" calligraplan
     *
     * See: https://doc.qt.io/qt-5/qloggingcategory.html
     */
    QLoggingCategory::setFilterRules(QStringLiteral("calligra.*.debug=false\n"
                                     "calligra.*.warning=true"));

    QApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);
    KoApplication app(PLAN_MIME_TYPE.latin1(), QStringLiteral("calligraplan"), KPlato::newAboutData, argc, argv);
    KLocalizedString::setApplicationDomain("calligraplan"); // activate translations

    if (!app.start(KPlato::Factory::global())) {
        return 1;
    }
    return app.exec();
}
