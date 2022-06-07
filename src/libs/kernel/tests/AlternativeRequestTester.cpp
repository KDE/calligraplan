/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2022 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/
// clazy:excludeall=qstring-arg
#include "AlternativeRequestTester.h"

#include "kptdatetime.h"
#include "kptschedule.h"
#include "kptcommand.h"
#include "AddResourceCmd.h"

#include "debug.cpp"

#include <QTest>


namespace KPlato
{

void initWeek(Calendar *c)
{
    auto week = c->weekdays();
    QTime t(8, 0);
    int length = 8*60*60*1000;
    for (int i = 0; i < 7; ++i) {
        week->setState(i, CalendarDay::Working);
        week->setIntervals(i, QList<TimeInterval*>()<<new TimeInterval(t, length));
    }
}

void AlternativeRequestTester::initTestCase()
{
    project = new Project();
    project->setId(project->uniqueNodeId());
    project->registerNodeId(project);
    project->setTimeZone(QTimeZone::utc());
    project->setConstraintStartTime(DateTime::fromString(QStringLiteral("2022-11-01"), project->timeZone()));
    project->setConstraintEndTime(DateTime::fromString(QStringLiteral("2022-12-01"), project->timeZone()));

    MacroCommand cmd;
    auto c1 = new Calendar("C1");
    c1->setTimeZone(project->timeZone());
    cmd.addCommand(new CalendarAddCmd(project, c1, -1, nullptr));
    c1->setDefault(true);
    initWeek(c1);

    t1 = new Task();
    t1->setName(QStringLiteral("T1"));
    cmd.addCommand(new TaskAddCmd(project, t1, nullptr));
    t2 = new Task();
    t2->setName(QStringLiteral("T2"));
    cmd.addCommand(new TaskAddCmd(project, t2, nullptr));

    r1 = new Resource();
    r1->setName(QStringLiteral("R1"));
    cmd.addCommand(new AddResourceCmd(project, r1));
    r2 = new Resource();
    r2->setName(QStringLiteral("R2"));
    cmd.addCommand(new AddResourceCmd(project, r2));

    auto rr1 = new ResourceRequest(r1, 100);
    auto rr2 = new ResourceRequest(r2, 100);
    rr1->addAlternativeRequest(rr2);
    cmd.addCommand(new AddResourceRequestCmd(&t2->requests(), rr1));

    manager = new ScheduleManager(*project);
    cmd.addCommand(new AddScheduleManagerCmd(*project, manager));

    cmd.execute();
    manager->createSchedules();

    Debug::print(project, "Base project", true);
}

void AlternativeRequestTester::cleanupTestCase()
{
    delete project;
}

void AlternativeRequestTester::scheduleForwardUnavailable()
{
    // make resource R1 unavailable
    r1->setAvailableFrom(project->constraintStartTime().addDays(2));
    project->calculate(*manager);
    //Debug::print(project, "Scheduled forward, R1 Unavailable", true);
    //Debug::printSchedulingLog(*manager);
    QCOMPARE(t2->currentSchedule(), t2->schedule()); // sanety
    QVERIFY(t2->isScheduled());
    QVERIFY(!t2->schedule()->appointments().isEmpty());
    QCOMPARE(t2->schedule()->appointments().value(0)->resource()->resource(), r2);
    QCOMPARE(t2->startTime(), DateTime::fromString(QStringLiteral("2022-11-01T08:00:00"), project->timeZone()));
}

void AlternativeRequestTester::scheduleForwardBooked()
{
    // make resource R1 booked by t1
    auto rr1 = new ResourceRequest(r1, 100);
    MacroCommand cmd;
    cmd.addCommand(new AddResourceRequestCmd(&t1->requests(), rr1));
    cmd.execute();

    project->calculate(*manager);
    //Debug::print(project, "Scheduled forward, R1 Booked", true);
    //Debug::printSchedulingLog(*manager);
    QCOMPARE(t2->currentSchedule(), t2->schedule()); // sanety
    QVERIFY(t2->isScheduled());
    QVERIFY(!t2->schedule()->appointments().isEmpty());
    QCOMPARE(t2->schedule()->appointments().value(0)->resource()->resource(), r2);
    QCOMPARE(t2->startTime(), DateTime::fromString(QStringLiteral("2022-11-01T08:00:00"), project->timeZone()));
}

void AlternativeRequestTester::rescheduleStarted()
{
    // make resource R1 booked by t1
    auto rr1 = new ResourceRequest(r1, 100);
    AddResourceRequestCmd c(&t1->requests(), rr1);
    c.execute();

    project->calculate(*manager);
    QCOMPARE(t2->currentSchedule(), t2->schedule()); // sanety
    QVERIFY(t2->isScheduled());
    QVERIFY(!t2->schedule()->appointments().isEmpty());
    QCOMPARE(t2->schedule()->appointments().value(0)->resource()->resource(), r2);
    QCOMPARE(t2->startTime(), DateTime::fromString(QStringLiteral("2022-11-01T08:00:00"), project->timeZone()));

    // start t2
    MacroCommand cmd;
    cmd.addCommand(new ModifyCompletionStartedCmd(t2->completion(), true));
    cmd.addCommand(new ModifyCompletionStartTimeCmd(t2->completion(), DateTime::fromString(QStringLiteral("2022-11-01T08:00:00"), project->timeZone())));
    cmd.addCommand(new AddCompletionActualEffortCmd(t2, r2, QDate(2022, 11, 1), Completion::UsedEffort::ActualEffort(Duration(2.0))));
    cmd.addCommand(new AddCompletionEntryCmd(t2->completion(), QDate(2022, 11, 1), new Completion::Entry(25, Duration(6.0), Duration(2.0))));
    cmd.execute();

    auto m = new ScheduleManager(*project);
    m->setRecalculate(true);
    m->setRecalculateFrom(DateTime::fromString(QStringLiteral("2022-11-03T08:00:00"), project->timeZone()));
    AddScheduleManagerCmd cm(manager, m);
    cm.execute();
    m->createSchedules();

    project->calculate(*m);
    //Debug::print(project, "Re-scheduled started, R2 Booked", true);
    //Debug::printSchedulingLog(*m);
    QCOMPARE(t2->currentSchedule(), t2->schedule()); // sanety
    QVERIFY(t2->isScheduled());
    QVERIFY(!t2->schedule()->appointments().isEmpty());
    QCOMPARE(t2->schedule()->appointments().value(0)->resource()->resource(), r2);
    QCOMPARE(t2->startTime(), DateTime::fromString(QStringLiteral("2022-11-01T08:00:00"), project->timeZone()));
    QCOMPARE(t2->endTime(), DateTime::fromString(QStringLiteral("2022-11-03T14:00:00"), project->timeZone()));
}

} //namespace KPlato

QTEST_GUILESS_MAIN(KPlato::AlternativeRequestTester)
