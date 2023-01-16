/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2009 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "MpxjImportTester.h"

#include "ExtraProperties.h"
#include "kptpart.h"
#include "kptmaindocument.h"

#include <QTest>

#include <tests/debug.cpp>

using namespace KPlato;

Part *loadDocument(const QString &file)
{
    Part *part = new Part(nullptr);
    MainDocument *doc = new MainDocument(part, false);
    part->setDocument(doc);
    doc->setProgressEnabled(false);
    doc->setProperty(NOUI, true); // avoid possible error message
    doc->importDocument(QUrl::fromUserInput(file));
    return part;
}

void MpxjImportTester::testGanttProject()
{
    QString file = QFINDTESTDATA("../../convert/testdata/ganttproject.gan");
    Part *part = loadDocument(file);
    auto project = part->document()->project();
    Debug::print(project, "GanttProject: -----------------", true);
    delete part;
}

void MpxjImportTester::testProjectLibre()
{
    QString file = QFINDTESTDATA("../../convert/testdata/projectlibre.pod");
    Part *part = loadDocument(file);
    auto project = part->document()->project();
    auto schedule = project->schedules().values().value(0);
    QVERIFY(schedule);
    project->setCurrentSchedule(schedule->id());
    Debug::print(project, "ProjectLibre: -----------------", true);
    QCOMPARE(project->allCalendars().count(), 3); // ProjectLibre generate 3 calendars by default
    QCOMPARE(project->allResourceGroups().count(), 1); // ProjectLibre seems to generate an extra group
    QCOMPARE(project->resourceList().count(), 3); // ProjectLibre seems to generate an extra resource
    QCOMPARE(project->allNodes().count(), 6);
    delete part;
}

QTEST_MAIN(MpxjImportTester)
