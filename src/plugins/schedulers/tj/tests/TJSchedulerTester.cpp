/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "TJSchedulerTester.h"

#include "PlanTJScheduler.h"

#include "plan/kptmaindocument.h"
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

#include <ExtraProperties.h>

#include <QMultiMap>
#include <QTest>

#include "tests/DateTimeTester.h"

#include "tests/debug.cpp"

using namespace KPlato;

void TJSchedulerTester::init()
{
    ulong granularity =  60*60*1000;
    m_scheduler = new PlanTJScheduler(granularity);
}

void TJSchedulerTester::cleanup()
{
    delete m_scheduler;
}

void TJSchedulerTester::populateSchedulingContext(SchedulingContext &context, const QString &name, const QList<Part*> &projects, const QList<Part*> &bookings) const
{
    context.project = new Project();
    context.project->setName(name);
    context.project->setConstraintStartTime(DateTime());
    context.project->setConstraintEndTime(DateTime());
    int prio = projects.count();
    for (const auto part : std::as_const(projects)) {
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
        project->setProperty(SCHEDULEMANAGERNAME, sm->name());
        project->initiateCalculation(*(sm->expected()));
        project->setCurrentScheduleManager(sm);
        project->setCurrentSchedule(sm->expected()->id());
        if (parentManager) {
            project->setParentSchedule(parentManager->expected());
        }
        context.projects.insert(prio--, doc);

        if (!context.project->constraintStartTime().isValid()) {
            context.project->setConstraintStartTime(project->constraintStartTime());
            context.project->setConstraintEndTime(project->constraintEndTime());
        } else {
            context.project->setConstraintStartTime(std::min(project->constraintStartTime(), context.project->constraintStartTime()));
            context.project->setConstraintEndTime(std::max(project->constraintEndTime(), context.project->constraintEndTime()));
        }
    }
    for (const auto part : bookings) {
        auto doc = part->document();
        auto project = doc->project();
        auto sm = project->scheduleManagers().value(0);
        project->setCurrentScheduleManager(sm);
        project->setCurrentSchedule(sm->expected()->id());
        doc->setProperty(SCHEDULEMANAGERNAME, sm->name());
        context.resourceBookings.append(doc);
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
    doc->setUrl(QUrl::fromUserInput(localFilePath));
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

void TJSchedulerTester::deleteAll(const QList<Part*> parts)
{
    for (auto part : parts) {
        delete part->document();
        delete part;
    }
}

void TJSchedulerTester::testSingleProject()
{
    const auto projectFiles = QStringList() << "Test 1.plan";
    QString dir = QFINDTESTDATA("data/multi/schedule/");
    QList<Part*> projects = loadDocuments(dir, projectFiles);
    QCOMPARE(projects.count(), projectFiles.count());

    SchedulingContext context;
    context.scheduleInParallel = true;
    populateSchedulingContext(context, "Test Single Project", projects);

    m_scheduler->schedule(context);
    auto project = context.projects.first()->project();
    Debug::print(project, "--", true);
    QCOMPARE(project->childNode(0)->startTime().toTimeZone(project->timeZone()).date(), QDate(2021, 4, 8));
    QCOMPARE(project->childNode(1)->startTime().toTimeZone(project->timeZone()).date(), QDate(2021, 4, 9));

    deleteAll(projects);
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
    context.scheduleInParallel = true;
    populateSchedulingContext(context, "Test Single Project With Bookings", projects, bookings);
    m_scheduler->schedule(context);
    // Booking 1: R1 booked 2021-04-08, 2021-04-09
    auto project = context.projects.first()->project();
    // Debug::print(project, "--", true);
    for (auto &log : std::as_const(context.log)) {
        qInfo()<<log.formatMsg();
    }
    Debug::print(project, "", true);
    QCOMPARE(project->childNode(0)->startTime().toTimeZone(project->timeZone()).date(), QDate(2021, 4, 10));
    QCOMPARE(project->childNode(1)->startTime().toTimeZone(project->timeZone()).date(), QDate(2021, 4, 11));

    deleteAll(projects);
    deleteAll(bookings);
}

void TJSchedulerTester::testMultiple()
{
    const auto projectFiles = QStringList() << "Test 1.plan" << "Test 2.plan";
    QString dir = QFINDTESTDATA("data/multi/schedule/");
    QList<Part*> projects = loadDocuments(dir, projectFiles);
    QCOMPARE(projects.count(), 2);

    SchedulingContext context;
    context.scheduleInParallel = true;
    populateSchedulingContext(context, "Test Multiple Project", projects);
    m_scheduler->schedule(context);
    // for (const Schedule::Log &l : std::as_const(context.log)) qDebug()<<l;
    auto project = projects.value(0)->document()->project();
    QCOMPARE(project->childNode(0)->startTime().toTimeZone(project->timeZone()).date(), QDate(2021, 4, 8));
    QCOMPARE(project->childNode(1)->startTime().toTimeZone(project->timeZone()).date(), QDate(2021, 4, 9));
    project = projects.value(1)->document()->project();
    QCOMPARE(project->childNode(0)->startTime().toTimeZone(project->timeZone()).date(), QDate(2021, 4, 10));
    QCOMPARE(project->childNode(1)->startTime().toTimeZone(project->timeZone()).date(), QDate(2021, 4, 11));

    deleteAll(projects);
}

void TJSchedulerTester::testMultipleWithBookingsParalell()
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
    context.scheduleInParallel = true;
    populateSchedulingContext(context, "Test Multiple Project With Bookings", projects, bookings);
    m_scheduler->schedule(context);
    // for (const Schedule::Log &l : std::as_const(context.log)) qDebug()<<l;
    // Booking 1: R1 booked 2021-04-08, 2021-04-09
    // Booking 2: R1 booked 2021-04-12, 2021-04-13
    auto project = projects.value(0)->document()->project();
    QCOMPARE(project->childNode(0)->startTime().date(), QDate(2021, 4, 10));
    QCOMPARE(project->childNode(1)->startTime().date(), QDate(2021, 4, 11));
    project = projects.value(1)->document()->project();
    QCOMPARE(project->childNode(0)->startTime().date(), QDate(2021, 4, 14));
    QCOMPARE(project->childNode(1)->startTime().date(), QDate(2021, 4, 15));

    deleteAll(projects);
    deleteAll(bookings);
}

void TJSchedulerTester::testMultipleWithBookingsSequential()
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
    context.scheduleInParallel = false;
    populateSchedulingContext(context, "Test Multiple Project With Bookings", projects, bookings);
    m_scheduler->schedule(context);
    // for (const Schedule::Log &l : std::as_const(context.log)) qDebug()<<l;
    // Booking 1: R1 booked 2021-04-08, 2021-04-09
    // Booking 2: R1 booked 2021-04-12, 2021-04-13
    auto project = projects.value(0)->document()->project();
    QCOMPARE(project->childNode(0)->startTime().date(), QDate(2021, 4, 10));
    QCOMPARE(project->childNode(1)->startTime().date(), QDate(2021, 4, 11));
    project = projects.value(1)->document()->project();
    QCOMPARE(project->childNode(0)->startTime().date(), QDate(2021, 4, 14));
    QCOMPARE(project->childNode(1)->startTime().date(), QDate(2021, 4, 15));

    deleteAll(projects);
    deleteAll(bookings);
}

void TJSchedulerTester::testRecalculate()
{
    const auto projectFiles = QStringList() << "Test Recalculate 1.plan";
    QString dir = QFINDTESTDATA("data/multi/schedule/");
    QList<Part*> projects = loadDocuments(dir, projectFiles);
    QCOMPARE(projects.count(), 1);

    SchedulingContext context;
    context.scheduleInParallel = true;
    populateSchedulingContext(context, "Test Recalculate Project", projects);
    QVERIFY(projects.value(0)->document()->project()->childNode(0)->schedule()->parent());
    context.calculateFrom = QDate(2021, 4, 26).startOfDay();
    m_scheduler->schedule(context);
    //for (const Schedule::Log &l : std::as_const(context.log)) qDebug()<<l;
    // T1 Recalculate 1: Two first days has been completed, 3 last days moved
    auto project = projects.value(0)->document()->project();
    auto T1 = static_cast<Task*>(project->childNode(0));
    auto T2 = static_cast<Task*>(project->childNode(1));
    auto Length = static_cast<Task*>(project->childNode(2));
    QCOMPARE(T1->startTime().toTimeZone(project->timeZone()).date(), QDate(2021, 4, 19)); // as before
    QCOMPARE(T1->endTime().toTimeZone(project->timeZone()).date(), QDate(2021, 4, 28));
    QCOMPARE(T2->startTime().toTimeZone(project->timeZone()).date(), QDate(2021, 4, 29));

    QCOMPARE(Length->startTime().toTimeZone(project->timeZone()).date(), QDate(2021, 4, 19));
    QCOMPARE(Length->endTime().toTimeZone(project->timeZone()).date(), QDate(2021, 4, 23));

    deleteAll(projects);
}

void TJSchedulerTester::testRecalculateMultiple()
{
    const auto projectFiles = QStringList() << "Test 1.plan" << "Test Recalculate 1.plan";
    QString dir = QFINDTESTDATA("data/multi/schedule/");
    QList<Part*> projects = loadDocuments(dir, projectFiles);
    QCOMPARE(projects.count(), projectFiles.count());

    SchedulingContext context;
    context.scheduleInParallel = true;
    populateSchedulingContext(context, "Test Recalculate Multiple Projects", projects);
    QVERIFY(projects.value(0)->document()->project()->childNode(0)->schedule()->parent());

    auto project = projects.value(0)->document()->project();
    auto T1 = static_cast<Task*>(project->childNode(0));
    auto T2 = static_cast<Task*>(project->childNode(1));

    context.calculateFrom = QDateTime(QDate(2021, 4, 26), QTime());
    m_scheduler->schedule(context);
    //for (const Schedule::Log &l : std::as_const(context.log)) qDebug()<<l;
    //qDebug()<<"Check project"<<project;
    QCOMPARE(T1->startTime().toTimeZone(project->timeZone()).date(), QDate(2021, 4, 26));
    QCOMPARE(T1->endTime().toTimeZone(project->timeZone()).date(), QDate(2021, 4, 26));
    QCOMPARE(T2->startTime().toTimeZone(project->timeZone()).date(), QDate(2021, 4, 30));

    project = projects.value(1)->document()->project();
    T1 = static_cast<Task*>(project->childNode(0));
    T2 = static_cast<Task*>(project->childNode(1));
    auto Length = static_cast<Task*>(project->childNode(2));

    //qDebug()<<"Check project"<<project;
    // T1 Recalculate 1: Two first days has been completed, 3 last days moved
    QCOMPARE(T1->startTime().toTimeZone(project->timeZone()).date(), QDate(2021, 4, 19)); // as before
    QCOMPARE(T1->endTime().toTimeZone(project->timeZone()).date(), QDate(2021, 4, 29));
    QCOMPARE(T2->startTime().toTimeZone(project->timeZone()).date(), QDate(2021, 5, 1));

    qInfo()<<"Length:"<<Length->startTime().toTimeZone(project->timeZone());
    QCOMPARE(Length->startTime().toTimeZone(project->timeZone()).date(), QDate(2021, 4, 19));
    QCOMPARE(Length->endTime().toTimeZone(project->timeZone()).date(), QDate(2021, 4, 23));

    deleteAll(projects);
}

void TJSchedulerTester::testRecalculateMultipleSeq()
{
    const auto projectFiles = QStringList() << "Test 1.plan" << "Test Recalculate 1.plan";
    QString dir = QFINDTESTDATA("data/multi/schedule/");
    QList<Part*> projects = loadDocuments(dir, projectFiles);
    QCOMPARE(projects.count(), projectFiles.count());

    SchedulingContext context;
    context.scheduleInParallel = false;
    populateSchedulingContext(context, "Test Recalculate Multiple Projects", projects);
    QVERIFY(projects.value(0)->document()->project()->childNode(0)->schedule()->parent());

    auto project = projects.value(0)->document()->project();
    auto T1 = static_cast<Task*>(project->childNode(0));
    auto T2 = static_cast<Task*>(project->childNode(1));

    context.calculateFrom = QDateTime(QDate(2021, 4, 26), QTime());
    m_scheduler->schedule(context);
    //for (const Schedule::Log &l : std::as_const(context.log)) qDebug()<<l;
    //qDebug()<<"Check project"<<project;
    QCOMPARE(T1->startTime().toTimeZone(project->timeZone()).date(), QDate(2021, 4, 26));
    QCOMPARE(T1->endTime().toTimeZone(project->timeZone()).date(), QDate(2021, 4, 26));
    QCOMPARE(T2->startTime().toTimeZone(project->timeZone()).date(), QDate(2021, 4, 27));

    project = projects.value(1)->document()->project();
    T1 = static_cast<Task*>(project->childNode(0));
    T2 = static_cast<Task*>(project->childNode(1));
    auto Length = static_cast<Task*>(project->childNode(2));

    //qDebug()<<"Check project"<<project;
    // T1 Recalculate 1: Two first days has been completed, 3 last days moved
    QCOMPARE(T1->startTime().toTimeZone(project->timeZone()).date(), QDate(2021, 4, 19)); // as before
    QCOMPARE(T1->endTime().toTimeZone(project->timeZone()).date(), QDate(2021, 4, 30)); // continued 28/29/30
    QCOMPARE(T2->startTime().toTimeZone(project->timeZone()).date(), QDate(2021, 5, 1));

    qInfo()<<"Length:"<<Length->startTime().toTimeZone(project->timeZone());
    QCOMPARE(Length->startTime().toTimeZone(project->timeZone()).date(), QDate(2021, 4, 19));
    QCOMPARE(Length->endTime().toTimeZone(project->timeZone()).date(), QDate(2021, 4, 23));

    deleteAll(projects);
}

QTEST_MAIN(KPlato::TJSchedulerTester)
