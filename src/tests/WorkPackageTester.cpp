/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2022 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "WorkPackageTester.h"

#include <kptpart.h>
#include <kptmaindocument.h>
#include <kptview.h>
#include <kptproject.h>
#include <kpttask.h>

#include <KoDocument.h>
#include <ExtraProperties.h>

#include <workpackage.h>
#include <part.h>

#include <QTest>

#include <tests/debug.cpp>

void WorkPackageTester::init()
{
    QStandardPaths::setTestModeEnabled(true);
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    dir.removeRecursively();
}

KPlato::Part *WorkPackageTester::loadDocument(const QString &fname)
{
    if (!QFile::exists(fname)) {
        qDebug()<<"File does not exist:"<<fname;
        return nullptr;
    }
    KPlato::Part *part = new KPlato::Part(nullptr);
    auto *doc = new KPlato::MainDocument(part, false);
    part->setDocument(doc);
    doc->setProgressEnabled(false);
    doc->setProperty(NOUI, true); // avoid possible error message
    doc->setUrl(QUrl::fromUserInput(fname));
    if (!doc->openLocalFile(fname)) {
        delete part;
        return nullptr;
    }
    return part;
}

void WorkPackageTester::loadPlan_v06()
{
    // Load workpackage produced by calligraplan file syntax version 0.6.7
    QString file = QFINDTESTDATA("data/plan_v06.planwork");
    QVERIFY(!file.isEmpty());
    KPlatoWork::Part workpart;

    QVERIFY(workpart.loadWorkPackage(file));
    QCOMPARE(workpart.workPackageCount(), 1);
    auto wp = workpart.workPackage(0);
    qDebug()<<wp;
    QCOMPARE(wp->name(), QStringLiteral("T1"));
    auto task = wp->task();
    QVERIFY(!task->isStarted());
    workpart.removeWorkPackage(wp);
    delete wp;
}

void WorkPackageTester::loadPlanWork_v06()
{
    // Load workpackage produced by calligraplanwork file syntax version 0.6.7
    auto file = QFINDTESTDATA("data/planwork_v06.planwork");
    QVERIFY(!file.isEmpty());

    auto workpart = new KPlatoWork::Part();
    QVERIFY(workpart->loadWorkPackage(file));
    QCOMPARE(workpart->workPackageCount(), 1);
    auto wp = workpart->workPackage(0);
    qDebug()<<wp;
    QCOMPARE(wp->name(), QStringLiteral("T1"));
    auto task = wp->task();
    QVERIFY(task->isStarted());
    QCOMPARE(task->workPackage().ownerName(), "R1");
    workpart->saveWorkPackages(true);
    delete workpart;

    file = QFINDTESTDATA("data/Workpackage_v06.plan");
    QVERIFY(!file.isEmpty());
    auto planpart = loadDocument(file);
    QVERIFY(planpart);
    auto doc = qobject_cast<KPlato::MainDocument*>(planpart->document());
    doc->setProperty(NOUI, true);
    task = doc->project()->allTasks().value(0);
    QCOMPARE(task->name(), "T1");
    QVERIFY(!task->isStarted());

    auto wpurl = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/projects/Workpackagev0-6/T1_20220330075604557nSNAaLl.planwork");
    QVERIFY(doc->loadWorkPackage(*(doc->project()), wpurl));
    auto package = doc->workPackages().values().value(0);
    QVERIFY(package);
    QCOMPARE(package->task->name(), "T1");
    QCOMPARE(package->task->id(), task->id());
    QCOMPARE(package->ownerName, "R1");
    QVERIFY(package->task->isStarted());
    QVERIFY(!package->task->completion().isFinished());
    QCOMPARE(package->task->completion().percentFinished(), 10);
    QCOMPARE(package->task->completion().actualEffort().toDouble(), 3.);
    QCOMPARE(package->task->completion().remainingEffort().toDouble(), 36.);

    delete planpart;
}

void WorkPackageTester::loadPlan_v07()
{
    // Load workpackage produced by calligraplan file syntax version 0.7.0
    QString file = QFINDTESTDATA("data/plan_v07.planwork");
    QVERIFY(!file.isEmpty());
    KPlatoWork::Part workpart;

    QVERIFY(workpart.loadWorkPackage(file));
    QCOMPARE(workpart.workPackageCount(), 1);
    auto wp = workpart.workPackage(0);
    qDebug()<<wp;
    QCOMPARE(wp->name(), QStringLiteral("T1"));
    auto task = wp->task();
    QCOMPARE(task->completion().entrymode(), KPlato::Completion::EnterEffortPerResource);
    QVERIFY(task->isStarted());
    QCOMPARE(task->workPackage().ownerName(), "T.R1");
    QVERIFY(!task->completion().isFinished());
    QCOMPARE(task->completion().percentFinished(), 10);
    QCOMPARE(task->completion().actualEffort().toDouble(), 3.);
    QCOMPARE(task->completion().remainingEffort().toDouble(), 7.);
}

void WorkPackageTester::loadPlanWork_v07()
{
    // Load workpackage produced by calligraplanwork file syntax version 0.7.0
    auto file = QFINDTESTDATA("data/planwork_v07.planwork");
    QVERIFY(!file.isEmpty());
    auto workpart = new KPlatoWork::Part();
    QVERIFY(workpart->loadWorkPackage(file));
    QCOMPARE(workpart->workPackageCount(), 1);
    auto wp = workpart->workPackage(0);
    qDebug()<<wp;
    QCOMPARE(wp->name(), QStringLiteral("T1"));
    auto task = wp->task();
    QVERIFY(task->isStarted());
    QVERIFY(!task->completion().isFinished());
    QCOMPARE(task->name(), "T1");
    QCOMPARE(task->id(), task->id());
    QCOMPARE(task->workPackage().ownerName(), "T.R1");
    QVERIFY(task->isStarted());
    QVERIFY(!task->completion().isFinished());
    QCOMPARE(task->completion().percentFinished(), 25);
    QCOMPARE(task->completion().actualEffort().toDouble(), 0.);
    QCOMPARE(task->completion().remainingEffort().toDouble(), 7.);

    workpart->saveWorkPackages(true);
    delete workpart;

    file = QFINDTESTDATA("data/Workpackage_v07.plan");
    QVERIFY(!file.isEmpty());
    auto planpart = loadDocument(file);
    QVERIFY(planpart);
    auto doc = qobject_cast<KPlato::MainDocument*>(planpart->document());
    doc->setProperty(NOUI, true);
    task = doc->project()->allTasks().value(0);
    QCOMPARE(task->name(), "T1");
    QVERIFY(task->isStarted());
    QVERIFY(!task->completion().isFinished());

    auto wpurl = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/projects/T/T1_20220324095733nT1McWKMbv.planwork");
    QVERIFY(doc->loadWorkPackage(*(doc->project()), wpurl));
    auto package = doc->workPackages().values().value(0);
    QVERIFY(package);
    QCOMPARE(package->task->name(), "T1");
    QCOMPARE(package->task->id(), task->id());
    QCOMPARE(package->ownerName, "T.R1");
    QVERIFY(package->task->isStarted());
    QVERIFY(!package->task->completion().isFinished());
    QCOMPARE(package->task->completion().percentFinished(), 25);
    QCOMPARE(package->task->completion().actualEffort().toDouble(), 0.);
    QCOMPARE(package->task->completion().remainingEffort().toDouble(), 7.);

    delete planpart;
}

QTEST_MAIN(WorkPackageTester)
