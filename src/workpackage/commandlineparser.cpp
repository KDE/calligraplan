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

#include <KIconLoader>
#include <KLocalizedString>
#include <KAboutData>
#include <KWindowSystem>
#include <KMessageBox>

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
    m_mainwindow->setAttribute(Qt::WA_NativeWindow, true);
    KWindowSystem::updateStartupId(m_mainwindow->windowHandle());
    KWindowSystem::activateWindow(m_mainwindow->windowHandle());
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
    const QRegularExpression withProtocolChecker(QStringLiteral("^[a-zA-Z]+:"));
    for(const QString &fileUrl : fileUrls) {
        // convert to an url
        const bool startsWithProtocol = (withProtocolChecker.match(fileUrl).hasMatch() == 0);
        const QUrl url = startsWithProtocol ?
            QUrl::fromUserInput(fileUrl) :
            QUrl::fromLocalFile(workingDirectory.absoluteFilePath(fileUrl));

        // For now create an empty document
        if (! m_mainwindow->openDocument(url)) {
            KMessageBox::error(nullptr, i18n("Failed to open document"));
        }
    }
}
