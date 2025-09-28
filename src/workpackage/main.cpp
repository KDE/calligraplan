/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001 Thomas zander <zander@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/


// clazy:excludeall=qstring-arg
#include "commandlineparser.h"

#include <KDBusService>
#include <KLocalizedString>

#include <QApplication>
#include <QLoggingCategory>
#include <QDir>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("calligraplanwork"); // activate translations
#ifdef Q_OS_MACOS
        // app.applicationName() will return "Plan Work" because of the nice name
        // set in the Info.plist. DBus doesn't like the resulting space in the 
        // service name, so reset the application name:
        app.setApplicationName(QLatin1String("calligraplanwork"));
#endif
    KDBusService service(KDBusService::Unique);
    // we come here only once...

    /**
     * Disable debug output by default, only log warnings.
     * Debug logs can be controlled by the environment variable QT_LOGGING_RULES.
     *
     * For example, to get full debug output, run the following:
     * QT_LOGGING_RULES="calligra.planwork=true" calligraplan
     *
     * See: https://doc.qt.io/qt-5/qloggingcategory.html
     */
    QLoggingCategory::setFilterRules(QStringLiteral("calligra.plan*.debug=false\n"
                                     "calligra.plan*.warning=true"));

    CommandLineParser cmd;
    QObject::connect(&service, &KDBusService::activateRequested, &cmd, &CommandLineParser::handleActivateRequest);
    cmd.handleCommandLine(QDir::current());
    return app.exec();
}
