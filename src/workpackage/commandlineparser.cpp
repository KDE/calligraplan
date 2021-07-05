/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "commandlineparser.h"
#include "part.h"
#include "mainwindow.h"
#include "aboutdata.h"

#include <kiconloader.h>
#include <KLocalizedString>
#include <KAboutData>
#include <KStartupInfo>
#include <KWindowSystem>
#include <KMessageBox>
#include <kwindowsystem_version.h>

#include <QApplication>
#include <QDir>

#include "debugarea.h"

CommandLineParser::CommandLineParser()
    : QObject(),
    m_mainwindow(nullptr)
{
    KAboutData *aboutData = KPlatoWork::newAboutData();
    KAboutData::setApplicationData(*aboutData);
    qApp->setWindowIcon(QIcon::fromTheme(QStringLiteral("calligraplanwork"), qApp->windowIcon()));

    aboutData->setupCommandLine(&m_commandLineParser);
    m_commandLineParser.addPositionalArgument(QStringLiteral("[file]"), i18n("File to open"));

    m_commandLineParser.process(*qApp);

    aboutData->processCommandLine(&m_commandLineParser);

    delete aboutData;
}

CommandLineParser::~CommandLineParser()
{
}

void CommandLineParser::handleActivateRequest(const QStringList &arguments, const QString &workingDirectory)
{
    m_commandLineParser.parse(arguments);

    handleCommandLine(QDir(workingDirectory));

    // terminate startup notification and activate the mainwindow
#if KWINDOWSYSTEM_VERSION >= QT_VERSION_CHECK(5,62,0)
    m_mainwindow->setAttribute(Qt::WA_NativeWindow, true);
    KStartupInfo::setNewStartupId(m_mainwindow->windowHandle(), KStartupInfo::startupId());
#else
    KStartupInfo::setNewStartupId(m_mainwindow, KStartupInfo::startupId());
#endif
    KWindowSystem::forceActiveWindow(m_mainwindow->winId());

}

void CommandLineParser::handleCommandLine(const QDir &workingDirectory)
{
    QList<KMainWindow*> lst = KMainWindow::memberList();
    if (lst.count() > 1) {
        warnPlanWork<<"windows count > 1:"<<lst.count();
        return; // should never happen
    }
    if (lst.isEmpty()) {
        Q_ASSERT(m_mainwindow == nullptr);
    }
    if (m_mainwindow == nullptr) {
        m_mainwindow = new KPlatoWork_MainWindow();
        m_mainwindow->show();
    }    
    // Get the command line arguments which we have to parse
    const QStringList fileUrls = m_commandLineParser.positionalArguments();
    // TODO: remove once Qt has proper handling itself
    const QRegExp withProtocolChecker(QStringLiteral("^[a-zA-Z]+:"));
    for(const QString &fileUrl : qAsConst(fileUrls)) {
        // convert to an url
        const bool startsWithProtocol = (withProtocolChecker.indexIn(fileUrl) == 0);
        const QUrl url = startsWithProtocol ?
            QUrl::fromUserInput(fileUrl) :
            QUrl::fromLocalFile(workingDirectory.absoluteFilePath(fileUrl));

        // For now create an empty document
        if (! m_mainwindow->openDocument(url)) {
            KMessageBox::error(nullptr, i18n("Failed to open document"));
        }
    }
}
