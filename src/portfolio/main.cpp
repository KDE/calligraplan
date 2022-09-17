/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "PortfolioFactory.h"
#include "AboutData.h"
#include "MainDocument.h"

#include <MimeTypes.h>
#include <KoApplication.h>

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
    KoApplication app(PLANPORTFOLIO_MIME_TYPE.latin1(), QStringLiteral("calligraplanportfolio"), newAboutData, argc, argv);

    if (!app.start(PortfolioFactory::global())) {
        return 1;
    }
    return app.exec();
}
