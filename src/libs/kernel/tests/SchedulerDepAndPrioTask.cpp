/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2026 Mickael Sergent <miko53@free.fr>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "SchedulerDepAndPrioTask.h"
#include <QTest>

#include "debug.cpp"

namespace KPlato
{

void SchedulerDepAndPrioTask::init()
{
    QTimeZone tz("Europe/Paris");

    p1 = new Project();
    p1->setId(p1->uniqueNodeId());
    p1->registerNodeId(p1);
    p1->setName(QStringLiteral("TestScheduleWithDependanceAndPriority"));
    p1->setTimeZone(tz);

    Calendar *c = new Calendar();
    c->setDefault(true);
    QTime a(8, 0, 0);
    QTime b (16, 0, 0);
    int length = a.msecsTo(b);
    for (int i=1; i <= 5; ++i) {
        CalendarDay *d = c->weekday(i);
        d->setState(CalendarDay::Working);
        d->addInterval(a, length);
    }
    for (int i=6; i <= 7; ++i) {
        CalendarDay *d = c->weekday(i);
        d->setState(CalendarDay::NonWorking);
    }

    p1->addCalendar(c);

    m = new Resource();
    m->setName(QStringLiteral("M"));
    p1->addResource(m);
    n = new Resource();
    n->setName(QStringLiteral("N"));
    p1->addResource(n);

    t1 = p1->createTask();
    t1->setName(QStringLiteral("T1"));
    p1->addTask(t1, p1);

    ta = p1->createTask();
    ta->setName(QStringLiteral("Ta"));
    ta->estimate()->setUnit(Duration::Unit_h);
    ta->estimate()->setExpectedEstimate(40.0);
    ta->estimate()->setType(Estimate::Type_Effort);
    ResourceRequest *rr = new ResourceRequest(m, 100);
    ta->requests().addResourceRequest(rr);

    p1->addSubTask(ta, t1);

    tb = p1->createTask();
    tb->setName(QStringLiteral("Tb"));
    tb->estimate()->setUnit(Duration::Unit_h);
    tb->estimate()->setExpectedEstimate(20.0);
    tb->estimate()->setType(Estimate::Type_Effort);
    rr = new ResourceRequest(m, 100);
    tb->requests().addResourceRequest(rr);

    p1->addSubTask(tb, t1);

    tc = p1->createTask();
    tc->setName(QStringLiteral("Tc"));
    tc->estimate()->setUnit(Duration::Unit_h);
    tc->estimate()->setExpectedEstimate(10.0);
    tc->estimate()->setType(Estimate::Type_Effort);
    rr = new ResourceRequest(m, 100);
    tc->requests().addResourceRequest(rr);
    p1->addSubTask(tc, t1);

    t2 = p1->createTask();
    t2->setName(QStringLiteral("T2"));
    p1->addTask(t2, p1);

    td = p1->createTask();
    td->setName(QStringLiteral("Td"));
    td->estimate()->setUnit(Duration::Unit_h);
    td->estimate()->setExpectedEstimate(30.0);
    td->estimate()->setType(Estimate::Type_Effort);
    rr = new ResourceRequest(n, 100);
    td->requests().addResourceRequest(rr);
    p1->addSubTask(td, t2);

    te = p1->createTask();
    te->setName(QStringLiteral("Te"));
    te->estimate()->setUnit(Duration::Unit_h);
    te->estimate()->setExpectedEstimate(30.0);
    te->estimate()->setType(Estimate::Type_Effort);
    rr = new ResourceRequest(n, 100);
    te->requests().addResourceRequest(rr);
    p1->addSubTask(te, t2);

    tf = p1->createTask();
    tf->setName(QStringLiteral("Tf"));
    tf->estimate()->setUnit(Duration::Unit_h);
    tf->estimate()->setExpectedEstimate(30.0);
    tf->estimate()->setType(Estimate::Type_Effort);
    rr = new ResourceRequest(n, 100);
    tf->requests().addResourceRequest(rr);
    p1->addSubTask(tf, t2);

    Relation * rel = new Relation(t1, t2);
    rel->setType(Relation::FinishStart);
    p1->addRelation(rel);
}

void SchedulerDepAndPrioTask::cleanup()
{
    qInfo("cleanup()");
    delete p1;
    p1 = nullptr;
}

void SchedulerDepAndPrioTask::test1ScheduleWithDependsInForward()
{
    QDate today = QDate::fromString(QStringLiteral("2026-02-15"), Qt::ISODate);
    p1->setConstraintStartTime(DateTime(today, QTime(0, 0, 0), p1->timeZone()));
    ScheduleManager *sm = p1->createScheduleManager(QStringLiteral("S1"));
    p1->addScheduleManager(sm);
    sm->createSchedules();
    p1->calculate(*sm);

    Debug::print(p1, "Project data", true);
    //Debug::printSchedulingLog(*sm, QStringLiteral("S1"));

    QCOMPARE(t1->startTime(), DateTime(QDate(2026, 2, 16), QTime(8, 0, 0)));
    QCOMPARE(t1->endTime(),   DateTime(QDate(2026, 2, 26), QTime(14, 0, 0)));

    QCOMPARE(ta->startTime(), DateTime(QDate(2026, 2, 16), QTime(8, 0, 0)));
    QCOMPARE(ta->endTime(),   DateTime(QDate(2026, 2, 20), QTime(16, 0, 0)));
    QCOMPARE(tb->startTime(), DateTime(QDate(2026, 2, 23), QTime(8, 0, 0)));
    QCOMPARE(tb->endTime(),   DateTime(QDate(2026, 2, 25), QTime(12, 0, 0)));
    QCOMPARE(tc->startTime(), DateTime(QDate(2026, 2, 25), QTime(12, 0, 0)));
    QCOMPARE(tc->endTime(),   DateTime(QDate(2026, 2, 26), QTime(14, 0, 0)));

    QCOMPARE(t2->startTime(), DateTime(QDate(2026, 2, 26), QTime(14, 0, 0)));
    QCOMPARE(t2->endTime(),   DateTime(QDate(2026, 3, 13), QTime(16, 0, 0)));

    QCOMPARE(td->startTime(), DateTime(QDate(2026, 2, 26), QTime(14, 0, 0)));
    QCOMPARE(td->endTime(),   DateTime(QDate(2026, 3, 4),  QTime(12, 0, 0)));
    QCOMPARE(te->startTime(), DateTime(QDate(2026, 3, 4),  QTime(12, 0, 0)));
    QCOMPARE(te->endTime(),   DateTime(QDate(2026, 3, 10), QTime(10, 0, 0)));
    QCOMPARE(tf->startTime(), DateTime(QDate(2026, 3, 10), QTime(10, 0, 0)));
    QCOMPARE(tf->endTime(),   DateTime(QDate(2026, 3, 13), QTime(16, 0, 0)));
}

void SchedulerDepAndPrioTask::test2ScheduleWithDependsAndPriorityInForward()
{
    QDate today = QDate::fromString(QStringLiteral("2026-02-15"), Qt::ISODate);
    p1->setConstraintStartTime(DateTime(today, QTime(0, 0, 0), p1->timeZone()));
    ScheduleManager *sm = p1->createScheduleManager(QStringLiteral("S1"));
    p1->addScheduleManager(sm);
    sm->createSchedules();

    //change priority of tb to change schedule order
    tb->setPriority(10);

    p1->calculate(*sm);

    Debug::print(p1, "Project data", true);
    //Debug::printSchedulingLog(*sm, QStringLiteral("S1"));

    QCOMPARE(t1->startTime(), DateTime(QDate(2026, 2, 16), QTime(8, 0, 0)));
    QCOMPARE(t1->endTime(),   DateTime(QDate(2026, 2, 26), QTime(14, 0, 0)));

    //now tb is the first task, then ta and tc
    QCOMPARE(tb->startTime(), DateTime(QDate(2026, 2, 16), QTime(8, 0, 0)));
    QCOMPARE(tb->endTime(),   DateTime(QDate(2026, 2, 18), QTime(12, 0, 0)));
    QCOMPARE(ta->startTime(), DateTime(QDate(2026, 2, 18), QTime(12, 0, 0)));
    QCOMPARE(ta->endTime(),   DateTime(QDate(2026, 2, 25), QTime(12, 0, 0)));
    QCOMPARE(tc->startTime(), DateTime(QDate(2026, 2, 25), QTime(12, 0, 0)));
    QCOMPARE(tc->endTime(),   DateTime(QDate(2026, 2, 26), QTime(14, 0, 0)));

    QCOMPARE(t2->startTime(), DateTime(QDate(2026, 2, 26), QTime(14, 0, 0)));
    QCOMPARE(t2->endTime(),   DateTime(QDate(2026, 3, 13), QTime(16, 0, 0)));

    QCOMPARE(td->startTime(), DateTime(QDate(2026, 2, 26), QTime(14, 0, 0)));
    QCOMPARE(td->endTime(),   DateTime(QDate(2026, 3, 4),  QTime(12, 0, 0)));
    QCOMPARE(te->startTime(), DateTime(QDate(2026, 3, 4),  QTime(12, 0, 0)));
    QCOMPARE(te->endTime(),   DateTime(QDate(2026, 3, 10), QTime(10, 0, 0)));
    QCOMPARE(tf->startTime(), DateTime(QDate(2026, 3, 10), QTime(10, 0, 0)));
    QCOMPARE(tf->endTime(),   DateTime(QDate(2026, 3, 13), QTime(16, 0, 0)));
}


void SchedulerDepAndPrioTask::test3ScheduleWithDependsAndPriorityInForward()
{
    QDate today = QDate::fromString(QStringLiteral("2026-02-15"), Qt::ISODate);
    p1->setConstraintStartTime(DateTime(today, QTime(0, 0, 0), p1->timeZone()));
    ScheduleManager *sm = p1->createScheduleManager(QStringLiteral("S1"));
    p1->addScheduleManager(sm);
    sm->createSchedules();

    //change priority of te to change schedule order
    te->setPriority(10);

    p1->calculate(*sm);

    Debug::print(p1, "Project data", true);
    //Debug::printSchedulingLog(*sm, QStringLiteral("S1"));

    QCOMPARE(t1->startTime(), DateTime(QDate(2026, 2, 16), QTime(8, 0, 0)));
    QCOMPARE(t1->endTime(),   DateTime(QDate(2026, 2, 26), QTime(14, 0, 0)));

    QCOMPARE(ta->startTime(), DateTime(QDate(2026, 2, 16), QTime(8, 0, 0)));
    QCOMPARE(ta->endTime(),   DateTime(QDate(2026, 2, 20), QTime(16, 0, 0)));
    QCOMPARE(tb->startTime(), DateTime(QDate(2026, 2, 23), QTime(8, 0, 0)));
    QCOMPARE(tb->endTime(),   DateTime(QDate(2026, 2, 25), QTime(12, 0, 0)));
    QCOMPARE(tc->startTime(), DateTime(QDate(2026, 2, 25), QTime(12, 0, 0)));
    QCOMPARE(tc->endTime(),   DateTime(QDate(2026, 2, 26), QTime(14, 0, 0)));

    QCOMPARE(t2->startTime(), DateTime(QDate(2026, 2, 26), QTime(14, 0, 0)));
    QCOMPARE(t2->endTime(),   DateTime(QDate(2026, 3, 13), QTime(16, 0, 0)));

    //now te is the first task launched
    QCOMPARE(te->startTime(), DateTime(QDate(2026, 2, 26), QTime(14, 0, 0)));
    QCOMPARE(te->endTime(),   DateTime(QDate(2026, 3, 4),  QTime(12, 0, 0)));
    QCOMPARE(td->startTime(), DateTime(QDate(2026, 3, 4),  QTime(12, 0, 0)));
    QCOMPARE(td->endTime(),   DateTime(QDate(2026, 3, 10), QTime(10, 0, 0)));
    QCOMPARE(tf->startTime(), DateTime(QDate(2026, 3, 10), QTime(10, 0, 0)));
    QCOMPARE(tf->endTime(),   DateTime(QDate(2026, 3, 13), QTime(16, 0, 0)));
}

void SchedulerDepAndPrioTask::test4ScheduleWithDependsInBackward()
{
    QDate today = QDate::fromString(QStringLiteral("2028-02-15"), Qt::ISODate);
    p1->setConstraintEndTime(DateTime(today, QTime(0, 0, 0), p1->timeZone()));
    ScheduleManager *sm = p1->createScheduleManager(QStringLiteral("S1"));
    sm->setSchedulingDirection(true);
    p1->addScheduleManager(sm);
    sm->createSchedules();

    p1->calculate(*sm);

    Debug::print(p1, "Project data", true);
    //Debug::printSchedulingLog(*sm, QStringLiteral("S1"));

    QCOMPARE(t1->startTime(), DateTime(QDate(2028, 1, 18), QTime(8, 0, 0)));
    QCOMPARE(t1->endTime(),   DateTime(QDate(2028, 2, 2),  QTime(10, 0, 0)));

    QCOMPARE(ta->startTime(), DateTime(QDate(2028, 1, 18), QTime(8, 0, 0)));
    QCOMPARE(ta->endTime(),   DateTime(QDate(2028, 1, 24), QTime(16, 0, 0)));
    QCOMPARE(tb->startTime(), DateTime(QDate(2028, 1, 25), QTime(8, 0, 0)));
    QCOMPARE(tb->endTime(),   DateTime(QDate(2028, 1, 27), QTime(12, 0, 0)));
    QCOMPARE(tc->startTime(), DateTime(QDate(2028, 1, 27), QTime(12, 0, 0)));
    QCOMPARE(tc->endTime(),   DateTime(QDate(2028, 1, 28), QTime(14, 0, 0)));

    QCOMPARE(t2->startTime(), DateTime(QDate(2028, 1, 28), QTime(14, 0, 0)));
    QCOMPARE(t2->endTime(),   DateTime(QDate(2028, 2, 14), QTime(16, 0, 0)));

    QCOMPARE(td->startTime(), DateTime(QDate(2028, 1, 28), QTime(14, 0, 0)));
    QCOMPARE(td->endTime(),   DateTime(QDate(2028, 2, 3),  QTime(12, 0, 0)));
    QCOMPARE(te->startTime(), DateTime(QDate(2028, 2, 3),  QTime(12, 0, 0)));
    QCOMPARE(te->endTime(),   DateTime(QDate(2028, 2, 9),  QTime(10, 0, 0)));
    QCOMPARE(tf->startTime(), DateTime(QDate(2028, 2, 9),  QTime(10, 0, 0)));
    QCOMPARE(tf->endTime(),   DateTime(QDate(2028, 2, 14), QTime(16, 0, 0)));
}

void SchedulerDepAndPrioTask::test5ScheduleWithDependsAndPriorityInBackward()
{
    QDate today = QDate::fromString(QStringLiteral("2028-02-15"), Qt::ISODate);
    p1->setConstraintEndTime(DateTime(today, QTime(0, 0, 0), p1->timeZone()));
    ScheduleManager *sm = p1->createScheduleManager(QStringLiteral("S1"));
    sm->setSchedulingDirection(true);
    p1->addScheduleManager(sm);
    sm->createSchedules();

    //change priority of tb to change schedule order
    tb->setPriority(10);

    p1->calculate(*sm);

    Debug::print(p1, "Project data", true);
    //Debug::printSchedulingLog(*sm, QStringLiteral("S1"));

    QCOMPARE(t1->startTime(), DateTime(QDate(2028, 1, 18), QTime(8, 0, 0)));
    QCOMPARE(t1->endTime(),   DateTime(QDate(2028, 2, 2),  QTime(10, 0, 0)));

    QCOMPARE(tb->startTime(), DateTime(QDate(2028, 1, 18), QTime(8, 0, 0)));
    QCOMPARE(tb->endTime(),   DateTime(QDate(2028, 1, 20), QTime(12, 0, 0)));
    QCOMPARE(ta->startTime(), DateTime(QDate(2028, 1, 20), QTime(12, 0, 0)));
    QCOMPARE(ta->endTime(),   DateTime(QDate(2028, 1, 27), QTime(12, 0, 0)));
    QCOMPARE(tc->startTime(), DateTime(QDate(2028, 1, 27), QTime(12, 0, 0)));
    QCOMPARE(tc->endTime(),   DateTime(QDate(2028, 1, 28), QTime(14, 0, 0)));

    QCOMPARE(t2->startTime(), DateTime(QDate(2028, 1, 28), QTime(14, 0, 0)));
    QCOMPARE(t2->endTime(),   DateTime(QDate(2028, 2, 14), QTime(16, 0, 0)));

    QCOMPARE(td->startTime(), DateTime(QDate(2028, 1, 28), QTime(14, 0, 0)));
    QCOMPARE(td->endTime(),   DateTime(QDate(2028, 2, 3),  QTime(12, 0, 0)));
    QCOMPARE(te->startTime(), DateTime(QDate(2028, 2, 3),  QTime(12, 0, 0)));
    QCOMPARE(te->endTime(),   DateTime(QDate(2028, 2, 9),  QTime(10, 0, 0)));
    QCOMPARE(tf->startTime(), DateTime(QDate(2028, 2, 9),  QTime(10, 0, 0)));
    QCOMPARE(tf->endTime(),   DateTime(QDate(2028, 2, 14), QTime(16, 0, 0)));
}

void SchedulerDepAndPrioTask::test6ScheduleWithDependsAndPriorityInBackward()
{
    QDate today = QDate::fromString(QStringLiteral("2028-02-15"), Qt::ISODate);
    p1->setConstraintEndTime(DateTime(today, QTime(0, 0, 0), p1->timeZone()));
    ScheduleManager *sm = p1->createScheduleManager(QStringLiteral("S1"));
    sm->setSchedulingDirection(true);
    p1->addScheduleManager(sm);
    sm->createSchedules();

    //change priority of te to change schedule order
    te->setPriority(10);

    p1->calculate(*sm);

    Debug::print(p1, "Project data", true);
    //Debug::printSchedulingLog(*sm, QStringLiteral("S1"));

    QCOMPARE(t1->startTime(), DateTime(QDate(2028, 1, 18), QTime(8, 0, 0)));
    QCOMPARE(t1->endTime(),   DateTime(QDate(2028, 2, 2),  QTime(10, 0, 0)));

    QCOMPARE(ta->startTime(), DateTime(QDate(2028, 1, 18), QTime(8, 0, 0)));
    QCOMPARE(ta->endTime(),   DateTime(QDate(2028, 1, 24), QTime(16, 0, 0)));
    QCOMPARE(tb->startTime(), DateTime(QDate(2028, 1, 25), QTime(8, 0, 0)));
    QCOMPARE(tb->endTime(),   DateTime(QDate(2028, 1, 27), QTime(12, 0, 0)));
    QCOMPARE(tc->startTime(), DateTime(QDate(2028, 1, 27), QTime(12, 0, 0)));
    QCOMPARE(tc->endTime(),   DateTime(QDate(2028, 1, 28), QTime(14, 0, 0)));

    QCOMPARE(t2->startTime(), DateTime(QDate(2028, 1, 28), QTime(14, 0, 0)));
    QCOMPARE(t2->endTime(),   DateTime(QDate(2028, 2, 14), QTime(16, 0, 0)));

    QCOMPARE(te->startTime(), DateTime(QDate(2028, 1, 28), QTime(14, 0, 0)));
    QCOMPARE(te->endTime(),   DateTime(QDate(2028, 2, 3),  QTime(12, 0, 0)));
    QCOMPARE(td->startTime(), DateTime(QDate(2028, 2, 3),  QTime(12, 0, 0)));
    QCOMPARE(td->endTime(),   DateTime(QDate(2028, 2, 9),  QTime(10, 0, 0)));
    QCOMPARE(tf->startTime(), DateTime(QDate(2028, 2, 9),  QTime(10, 0, 0)));
    QCOMPARE(tf->endTime(),   DateTime(QDate(2028, 2, 14), QTime(16, 0, 0)));
}

} //namespace KPlato

QTEST_GUILESS_MAIN(KPlato::SchedulerDepAndPrioTask)
