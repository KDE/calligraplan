/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2022 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "ReportsTester.h"

#include <kptpart.h>
#include <kptmaindocument.h>
#include <ReportGeneratorOdt.h>

#include <ExtraProperties.h>

#include <QTest>
#include <QTemporaryFile>

#include <tests/debug.cpp>

using namespace KPlato;

void ReportsTester::init()
{
    part = nullptr;
}

void ReportsTester::cleanup()
{
    delete part;
    part = nullptr;
}

void ReportsTester::loadDocument(const QString &file)
{
    QVERIFY(QFile::exists(file));

    part = new Part(nullptr);
    MainDocument *doc = new MainDocument(part, false);
    part->setDocument(doc);
    doc->setProgressEnabled(false);
    doc->setProperty(NOUI, true); // avoid possible error message
    doc->setUrl(QUrl::fromUserInput(file));
    QVERIFY(doc->openLocalFile(file));
}

void ReportsTester::testReportGeneration()
{
    QString file = QFINDTESTDATA("data/Project 1.plan");
    loadDocument(file);
    QString templateFile = QFINDTESTDATA("data/TaskStatus.odt");
    QVERIFY(QFile::exists(templateFile));

    auto project = part->document()->project();
    auto scheduleManager = project->scheduleManagers().value(0);
    ReportGeneratorOdt report;
    report.setProject(project);
    report.setScheduleManager(scheduleManager);
    report.setTemplateFile(templateFile);
    QTemporaryFile tmp(QDir::currentPath() + QDir::separator() + QStringLiteral("taskstatusreport-XXXXXX.odt"));
    QVERIFY(tmp.open());
    const auto reportFile = tmp.fileName();
    qDebug()<<reportFile;
    tmp.close();
    report.setReportFile(reportFile);
    QVERIFY(report.initiate());
    QVERIFY(report.createReport());
}


QTEST_MAIN(ReportsTester)
