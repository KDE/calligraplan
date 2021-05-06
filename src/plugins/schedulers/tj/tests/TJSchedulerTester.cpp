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

#include "PlanTJScheduler.h"

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

void TJSchedulerTester::init()
{
    m_scheduler = new PlanTJScheduler();
}

void TJSchedulerTester::cleanup()
{
    delete m_scheduler;
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
        ScheduleManager *sm = nullptr;
        ScheduleManager *parentManager = nullptr;
        if (project->isStarted()) {
            parentManager = project->scheduleManagers().value(0);
            sm = project->createScheduleManager(parentManager);
            sm->setParentManager(parentManager);
        } else {
            sm = new KPlato::ScheduleManager(*project, project->uniqueScheduleName());
        }
        AddScheduleManagerCmd cmd(*project, sm);
        cmd.redo();
        sm->createSchedules();
        doc->setProperty(SCHEDULEMANAGERNAME, sm->name());
        project->initiateCalculation(*(sm->expected()));
        project->setCurrentScheduleManager(sm);
        project->setCurrentSchedule(sm->expected()->id());
        if (parentManager) {
            project->setParentSchedule(parentManager->expected());
        }
        context.projects.insert(prio--, project);

        if (!context.project->constraintStartTime().isValid()) {
            context.project->setConstraintStartTime(project->constraintStartTime());
            context.project->setConstraintEndTime(project->constraintEndTime());
        } else {
            context.project->setConstraintStartTime(std::min(project->constraintStartTime(), context.project->constraintStartTime()));
            context.project->setConstraintEndTime(std::max(project->constraintEndTime(), context.project->constraintEndTime()));
        }
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
    if (!QFile::exists(localFilePath)) {
        qDebug()<<"File does not exist:"<<localFilePath;
        return nullptr;
    }
    Part *part = new Part(nullptr);
    MainDocument *doc = new MainDocument(part, false);
    part->setDocument(doc);
    doc->setProgressEnabled(false);
    doc->setProperty(NOUI, true); // avoid possible error message
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
            qDebug()<<"Failed to load:"<<dir<<fname;
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
    QCOMPARE(project->childNode(0)->startTime().date(), QDate(2021, 4, 8));
    QCOMPARE(project->childNode(1)->startTime().date(), QDate(2021, 4, 9));
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
    // Booking 1: R1 booked 2021-04-08, 2021-04-09
    auto project = context.projects.first();
    // Debug::print(project, "--", true);
    QCOMPARE(project->childNode(0)->startTime().date(), QDate(2021, 4, 10));
    QCOMPARE(project->childNode(1)->startTime().date(), QDate(2021, 4, 11));
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
    // for (const Schedule::Log &l : qAsConst(context.log)) qDebug()<<l;
    auto project = projects.value(0)->document()->project();
    QCOMPARE(project->childNode(0)->startTime().date(), QDate(2021, 4, 8));
    QCOMPARE(project->childNode(1)->startTime().date(), QDate(2021, 4, 9));
    project = projects.value(1)->document()->project();
    QCOMPARE(project->childNode(0)->startTime().date(), QDate(2021, 4, 10));
    QCOMPARE(project->childNode(1)->startTime().date(), QDate(2021, 4, 11));
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
    // for (const Schedule::Log &l : qAsConst(context.log)) qDebug()<<l;
    // Booking 1: R1 booked 2021-04-08, 2021-04-09
    // Booking 2: R1 booked 2021-04-12, 2021-04-13
    auto project = projects.value(0)->document()->project();
    QCOMPARE(project->childNode(0)->startTime().date(), QDate(2021, 4, 10));
    QCOMPARE(project->childNode(1)->startTime().date(), QDate(2021, 4, 11));
    project = projects.value(1)->document()->project();
    QCOMPARE(project->childNode(0)->startTime().date(), QDate(2021, 4, 14));
    QCOMPARE(project->childNode(1)->startTime().date(), QDate(2021, 4, 15));
}

void TJSchedulerTester::testRecalculate()
{
    const auto projectFiles = QStringList() << "Test Recalculate 1.plan";
    QString dir = QFINDTESTDATA("data/multi/schedule/");
    QList<Part*> projects = loadDocuments(dir, projectFiles);
    QCOMPARE(projects.count(), 1);

    SchedulingContext context;
    populateSchedulingContext(context, "Test Recalculate Project", projects);
    QVERIFY(projects.value(0)->document()->project()->childNode(0)->schedule()->parent());
    context.calculateFrom = QDateTime(QDate(2021, 4, 26));
    m_scheduler->schedule(context);
    //for (const Schedule::Log &l : qAsConst(context.log)) qDebug()<<l;
    // T1 Recalculate 1: Two first days has been completed, 3 last days moved
    auto project = projects.value(0)->document()->project();
    QCOMPARE(project->childNode(0)->startTime().date(), QDate(2021, 4, 19)); // as before
    QCOMPARE(project->childNode(0)->endTime().date(), QDate(2021, 4, 28));
    QCOMPARE(project->childNode(1)->startTime().date(), QDate(2021, 4, 29));
}

void TJSchedulerTester::testRecalculateMultiple()
{
    const auto projectFiles = QStringList() << "Test 1.plan" << "Test Recalculate 1.plan";
    QString dir = QFINDTESTDATA("data/multi/schedule/");
    QList<Part*> projects = loadDocuments(dir, projectFiles);
    QCOMPARE(projects.count(), 2);

    SchedulingContext context;
    populateSchedulingContext(context, "Test Recalculate Multiple Projects", projects);
    QVERIFY(projects.value(0)->document()->project()->childNode(0)->schedule()->parent());
    context.calculateFrom = QDateTime(QDate(2021, 4, 26));
    m_scheduler->schedule(context);
    for (const Schedule::Log &l : qAsConst(context.log)) qDebug()<<l;
    auto project = projects.value(0)->document()->project();
    qDebug()<<"Check project"<<project;
    QCOMPARE(project->childNode(0)->startTime().date(), QDate(2021, 4, 26));
    QCOMPARE(project->childNode(0)->endTime().date(), QDate(2021, 4, 26));
    QCOMPARE(project->childNode(1)->startTime().date(), QDate(2021, 4, 30));

    project = projects.value(1)->document()->project();
    qDebug()<<"Check project"<<project;
    // T1 Recalculate 1: Two first days has been completed, 3 last days moved
    QCOMPARE(project->childNode(0)->startTime().date(), QDate(2021, 4, 19)); // as before
    QCOMPARE(project->childNode(0)->endTime().date(), QDate(2021, 4, 29));
    QCOMPARE(project->childNode(1)->startTime().date(), QDate(2021, 5, 1));

}

QTEST_MAIN(KPlato::TJSchedulerTester)
