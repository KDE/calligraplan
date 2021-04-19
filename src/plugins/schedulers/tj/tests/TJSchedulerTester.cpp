/* This file is part of the KDE project
 * Copyright (C) 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

// clazy:excludeall=qstring-arg
#include "TJSchedulerTester.h"
#include "TestSchedulerPluginLoader.h"

// #include "PlanTJPlugin.h"

#include "kptmaindocument.h"
#include "kptpart.h"
#include "kptcommand.h"
#include "kptcalendar.h"
#include "kptdatetime.h"
#include "kptresource.h"
#include "kptnode.h"
#include "kpttask.h"
#include "kptproject.h"
#include "kptschedule.h"
#include "kptschedulerplugin.h"
#include "SchedulingContext.h"

#include <QMultiMap>
#include <QTest>

#include "tests/DateTimeTester.h"

#include "tests/debug.cpp"

using namespace KPlato;

void TJSchedulerTester::initTestCase()
{
    const QString name("TJ Scheduler");
    TestSchedulerPluginLoader *loader = new TestSchedulerPluginLoader(this);
    const QString dir = SCHEDULERPLUGINS_DIR;
    m_scheduler = loader->loadPlugin(dir, name);
    QVERIFY(m_scheduler);
}

void TJSchedulerTester::cleanupTestCase()
{
}

void TJSchedulerTester::populateSchedulingContext(SchedulingContext &context, const QString &name, const QList<Part*> &projects, const QList<Part*> &bookings) const
{
    context.project = new Project();
    context.project->setName(name);
    context.granularity = 60*60*1000;
    context.project->setConstraintStartTime(DateTime());
    context.project->setConstraintEndTime(DateTime());
    int prio = projects.count();
    for (const auto part : qAsConst(projects)) {
        auto doc = part->document();
        auto project = doc->project();
        auto sm = new KPlato::ScheduleManager(*project, project->uniqueScheduleName());
        AddScheduleManagerCmd cmd(*project, sm);
        cmd.redo();
        sm->createSchedules();
        doc->setProperty(SCHEDULEMANAGERNAME, sm->name());
        project->initiateCalculation(*(sm->expected()));
        project->setCurrentScheduleManager(sm);
        project->setCurrentSchedule(sm->expected()->id());
        context.projects.insert(prio--, project);

        if (!context.project->constraintStartTime().isValid()) {
            context.project->setConstraintStartTime(project->constraintStartTime());
            context.project->setConstraintEndTime(project->constraintEndTime());
        }
        context.project->setConstraintStartTime(std::min(project->constraintStartTime(), context.project->constraintStartTime()));
        context.project->setConstraintEndTime(std::max(project->constraintEndTime(), context.project->constraintEndTime()));
    }
    for (const auto part : bookings) {
        auto project = part->document()->project();
        auto sm = project->scheduleManagers().value(0);
        project->setCurrentScheduleManager(sm);
        project->setCurrentSchedule(sm->expected()->id());
        context.resourceBookings.append(project);
    }
}

Part *TJSchedulerTester::loadDocument(const QString &dir, const QString &fname)
{
    const QString localFilePath = dir + fname;
    qInfo()<<"load:"<<localFilePath;
    if (!QFile::exists(localFilePath)) {
        qInfo()<<"File does not exist:"<<localFilePath;
        return nullptr;
    }
    Part *part = new Part(nullptr);
    MainDocument *doc = new MainDocument(part, false);
    part->setDocument(doc);
    doc->setProgressEnabled(false);
    doc->setSkipSharedResourcesAndProjects(true);
    if (!doc->openLocalFile(localFilePath)) {
        delete part;
        return nullptr;
    }
    return part;
}

QList<Part*> TJSchedulerTester::loadDocuments(QString &dir, QList<QString> files)
{
    QList<Part*> parts;
    for (const QString &fname : files) {
        Part *part = loadDocument(dir, fname);
        if (part) {
            parts << part;
        } else {
            qInfo()<<"Failed to load:"<<dir<<fname;
        }
    }
    return parts;
}

void TJSchedulerTester::testSingleProject()
{
    const auto projectFiles = QStringList() << "Test 1.plan";
    QString dir = QFINDTESTDATA("data/multi/schedule/");
    QList<Part*> projects = loadDocuments(dir, projectFiles);
    QCOMPARE(projects.count(), projectFiles.count());

    SchedulingContext context;
    populateSchedulingContext(context, "Test Single Project", projects);

    m_scheduler->schedule(context);
    auto project = context.projects.first();
    //Debug::print(project, "--", true);
    QCOMPARE(project->childNode(0)->startTime().date(), project->constraintStartTime().date());
    QCOMPARE(project->childNode(1)->startTime().date(), project->constraintStartTime().date().addDays(1));
    qDeleteAll(projects);
}

void TJSchedulerTester::testSingleProjectWithBookings()
{
    const auto projectFiles = QStringList() << "Test 1.plan";
    QString dir = QFINDTESTDATA("data/multi/schedule/");
    QList<Part*> projects = loadDocuments(dir, projectFiles);
    QCOMPARE(projects.count(), projectFiles.count());

    const auto bookingFiles = QStringList() << "Booking 1.plan";
    dir = QFINDTESTDATA("data/multi/bookings/");
    const QList<Part*> bookings = loadDocuments(dir, bookingFiles);
    QCOMPARE(bookings.count(), bookingFiles.count());

    SchedulingContext context;
    populateSchedulingContext(context, "Test Single Project With Bookings", projects, bookings);

    m_scheduler->schedule(context);
    auto project = context.projects.first();
//     Debug::print(project, "--", true);
    QCOMPARE(project->childNode(0)->startTime().date(), project->constraintStartTime().date().addDays(4)); // monday
    QCOMPARE(project->childNode(1)->startTime().date(), project->constraintStartTime().date().addDays(5)); // tuesday
    qDeleteAll(projects);

}

void TJSchedulerTester::testMultiple()
{
    const auto projectFiles = QStringList() << "Test 1.plan" << "Test 2.plan";
    QString dir = QFINDTESTDATA("data/multi/schedule/");
    QList<Part*> projects = loadDocuments(dir, projectFiles);
    QCOMPARE(projects.count(), 2);

    SchedulingContext context;
    populateSchedulingContext(context, "Test Multiple Project", projects);
    m_scheduler->schedule(context);
    for (const Schedule::Log &l : qAsConst(context.log)) qInfo()<<l;
    auto project = projects.value(0)->document()->project();
    QCOMPARE(project->childNode(0)->startTime().date(), project->constraintStartTime().date().addDays(0)); // thursday
    QCOMPARE(project->childNode(1)->startTime().date(), project->constraintStartTime().date().addDays(1)); // friday
    project = projects.value(1)->document()->project();
    QCOMPARE(project->childNode(0)->startTime().date(), project->constraintStartTime().date().addDays(4)); // monday
    QCOMPARE(project->childNode(1)->startTime().date(), project->constraintStartTime().date().addDays(5)); // tuesday
}

void TJSchedulerTester::testMultipleWithBookings()
{
    const auto projectFiles = QStringList() << "Test 1.plan" << "Test 2.plan";
    QString dir = QFINDTESTDATA("data/multi/schedule/");
    QList<Part*> projects = loadDocuments(dir, projectFiles);
    QCOMPARE(projects.count(), projectFiles.count());

    const auto bookingFiles = QStringList() << "Booking 1.plan" << "Booking 2.plan";
    dir = QFINDTESTDATA("data/multi/bookings/");
    QList<Part*> bookings = loadDocuments(dir, bookingFiles);
    QCOMPARE(bookings.count(), bookingFiles.count());

    SchedulingContext context;
    populateSchedulingContext(context, "Test Multiple Project With Bookings", projects, bookings);
    m_scheduler->schedule(context);
    for (const Schedule::Log &l : qAsConst(context.log)) qInfo()<<l;
    auto project = projects.value(0)->document()->project();
    QCOMPARE(project->childNode(0)->startTime().date(), project->constraintStartTime().date().addDays(6)); // wednesday
    QCOMPARE(project->childNode(1)->startTime().date(), project->constraintStartTime().date().addDays(7)); // thursday
    project = projects.value(1)->document()->project();
    QCOMPARE(project->childNode(0)->startTime().date(), project->constraintStartTime().date().addDays(8)); // friday
    QCOMPARE(project->childNode(1)->startTime().date(), project->constraintStartTime().date().addDays(11)); // monday
}

QTEST_MAIN(KPlato::TJSchedulerTester)
