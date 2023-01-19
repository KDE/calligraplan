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
    QScopedPointer<Part> part(loadDocument(file));
    auto project = part->document()->project();

    auto schedule = project->schedules().values().value(0);
    QVERIFY(schedule);
    project->setCurrentSchedule(schedule->id());

//    Debug::print(project, "GanttProject: -----------------", true);
    QEXPECT_FAIL("", "GanttProject importer does not set project description", Continue);
    QVERIFY(project->description().contains(QStringLiteral("This is a test of Gantt Project importer.")));

    QCOMPARE(project->allCalendars().count(), 1);
    QCOMPARE(project->allResourceGroups().count(), 0);
    QCOMPARE(project->resourceList().count(), 2);

    const auto nodes = project->allNodes(true);
    QCOMPARE(nodes.count(), 8);

    QVERIFY(nodes.value(0)->parentNode() == project);

    QVERIFY(nodes.value(1)->parentNode() == project);

    QVERIFY(nodes.value(2)->parentNode() == project);

    QVERIFY(nodes.value(3)->parentNode() == project);

    QVERIFY(nodes.value(4)->parentNode() == nodes.value(3));

    QVERIFY(nodes.value(5)->parentNode() == nodes.value(3));

    QVERIFY(nodes.value(6)->parentNode() == nodes.value(5));

    QVERIFY(nodes.value(7)->parentNode() == project);

    const auto tasks = project->allTasks();
    QCOMPARE(tasks.count(), 6);

    QCOMPARE(tasks.value(0)->priority(), 100);

    QCOMPARE(tasks.value(1)->priority(), 400);
    QVERIFY(tasks.value(1)->completion().isStarted());
    QCOMPARE(tasks.value(1)->completion().percentFinished(), 30);

    QCOMPARE(tasks.value(2)->priority(), 500);
    QVERIFY(tasks.value(2)->completion().isFinished());
    QCOMPARE(tasks.value(2)->completion().percentFinished(), 100);

    QCOMPARE(tasks.value(3)->priority(), 600);

    QCOMPARE(tasks.value(4)->priority(), 900);

    QCOMPARE(tasks.value(5)->priority(), 500);
    QCOMPARE(tasks.value(5)->type(), Node::Type_Milestone);

    QEXPECT_FAIL("", "GanttProject importer does not set task description", Continue);
    QVERIFY(tasks.value(5)->description().contains(QStringLiteral("This is a description")));
}

void MpxjImportTester::testProjectLibre()
{
    QString file = QFINDTESTDATA("../../convert/testdata/projectlibre.pod");
    QScopedPointer<Part> part(loadDocument(file));
    auto project = part->document()->project();
    auto schedule = project->schedules().values().value(0);
    QVERIFY(schedule);
    project->setCurrentSchedule(schedule->id());

//    Debug::print(project, "ProjectLibre: -----------------", true);
    QCOMPARE(project->leader(), "Project Manager");
    QEXPECT_FAIL("", "ProjectLibre importer does not set project description", Continue);
    QVERIFY(project->description().contains(QStringLiteral("This is the project description.")));

    QCOMPARE(project->allCalendars().count(), 3); // ProjectLibre generates 3 calendars by default
    QCOMPARE(project->allResourceGroups().count(), 1); // ProjectLibre seems to generate an extra group
    QCOMPARE(project->resourceList().count(), 3); // ProjectLibre seems to generate an extra resource
    const auto nodes = project->allNodes(true);
    QCOMPARE(nodes.count(), 7);

    QVERIFY(nodes.value(0)->parentNode() == project);

    QVERIFY(nodes.value(1)->parentNode() == project);

    QCOMPARE(nodes.value(2)->type(), Node::Type_Summarytask);
    QVERIFY(nodes.value(2)->parentNode() == project);

    QVERIFY(nodes.value(3)->parentNode() == nodes.value(2));

    QCOMPARE(nodes.value(4)->type(), Node::Type_Summarytask);
    QVERIFY(nodes.value(4)->parentNode() == nodes.value(2));

    QVERIFY(nodes.value(5)->parentNode() == nodes.value(4));

    QVERIFY(nodes.value(6)->parentNode() == project);

    const auto tasks = project->allTasks();
    QCOMPARE(tasks.count(), 5);

    QCOMPARE(tasks.value(0)->type(), Node::Type_Task);
    QCOMPARE(tasks.value(0)->priority(), 500);
    QVERIFY(tasks.value(0)->completion().isFinished());
    QCOMPARE(tasks.value(0)->completion().percentFinished(), 100);

    QCOMPARE(tasks.value(1)->type(), Node::Type_Task);
    QCOMPARE(tasks.value(1)->priority(), 500);
    QVERIFY(tasks.value(1)->completion().isStarted());
    QCOMPARE(tasks.value(1)->completion().percentFinished(), 30);

    QCOMPARE(tasks.value(2)->type(), Node::Type_Task);
    QCOMPARE(tasks.value(2)->priority(), 500);

    QCOMPARE(tasks.value(3)->type(), Node::Type_Task);
    QCOMPARE(tasks.value(3)->priority(), 500);

    QCOMPARE(tasks.value(4)->type(), Node::Type_Milestone);
    QCOMPARE(tasks.value(4)->priority(), 500);
    QVERIFY(tasks.value(4)->description().contains(QStringLiteral("This is a description")));
}

QTEST_MAIN(MpxjImportTester)
