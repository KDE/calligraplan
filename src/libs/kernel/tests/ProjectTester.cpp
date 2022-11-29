/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2007 Dag Andersen <dag.andersen@kdemail.net>
   SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
   
   SPDX-License-Identifier: LGPL-2.0-or-later
*/
// clazy:excludeall=qstring-arg
#include "ProjectTester.h"

#include "kptcommand.h"
#include "kptcalendar.h"
#include "kptdatetime.h"
#include "kptresource.h"
#include "kptnode.h"
#include "kpttask.h"
#include "kptschedule.h"

#include <QTest>

#include "debug.cpp"


namespace KPlato
{

void ProjectTester::initTestCase()
{
    QTimeZone tz("Europe/Copenhagen");

    m_project = new Project();
    m_project->setId(m_project->uniqueNodeId());
    m_project->registerNodeId(m_project);
    m_project->setTimeZone(tz);
    const DateTime dt(QDate(2012, 02, 01), QTime(00, 00), tz);
    m_project->setConstraintStartTime(dt);
    m_project->setConstraintEndTime(dt.addYears(1));
    // standard worktime defines 8 hour day as default
    QVERIFY(m_project->standardWorktime());
    QCOMPARE(m_project->standardWorktime()->day(), 8.0);
    m_calendar = new Calendar();
    m_calendar->setName(QStringLiteral("C1"));
    m_calendar->setTimeZone(tz);
    m_calendar->setDefault(true);
    QTime t1(9, 0, 0);
    QTime t2 (17, 0, 0);
    int length = t1.msecsTo(t2);
    for (int i=1; i <= 7; ++i) {
        CalendarDay *d = m_calendar->weekday(i);
        d->setState(CalendarDay::Working);
        d->addInterval(t1, length);
    }
    m_project->addCalendar(m_calendar);

    m_task = nullptr;
}

void ProjectTester::cleanupTestCase()
{
    delete m_project;
}

void ProjectTester::testAddTask()
{
    m_task = m_project->createTask();
    QVERIFY(m_project->addTask(m_task, m_project));
    QVERIFY(m_task->parentNode() == m_project);
    QCOMPARE(m_project->findNode(m_task->id()), m_task);

    m_project->takeTask(m_task);
    delete m_task; m_task = nullptr;
}

void ProjectTester::testTakeTask()
{
    m_task = m_project->createTask();
    m_project->addTask(m_task, m_project);
    QCOMPARE(m_project->findNode(m_task->id()), m_task);

    m_project->takeTask(m_task);
    QVERIFY(m_project->findNode(m_task->id()) == nullptr);

    delete (m_task); m_task = nullptr;
}

void ProjectTester::testTaskAddCmd()
{
    m_task = m_project->createTask();
    SubtaskAddCmd *cmd = new SubtaskAddCmd(m_project, m_task, m_project);
    cmd->execute();
    QVERIFY(m_task->parentNode() == m_project);
    QCOMPARE(m_project->findNode(m_task->id()), m_task);
    cmd->unexecute();
    QVERIFY(m_project->findNode(m_task->id()) == nullptr);
    delete cmd;
    m_task = nullptr;
}

void ProjectTester::testTaskDeleteCmd()
{
    m_task = m_project->createTask();
    QVERIFY(m_project->addTask(m_task, m_project));
    QVERIFY(m_task->parentNode() == m_project);

    NodeDeleteCmd *cmd = new NodeDeleteCmd(m_task);
    cmd->execute();
    QVERIFY(m_project->findNode(m_task->id()) == nullptr);

    cmd->unexecute();
    QCOMPARE(m_project->findNode(m_task->id()), m_task);

    cmd->execute();
    delete cmd;
    m_task = nullptr;
}

void ProjectTester::schedule()
{
    QDate today = QDate::fromString(QStringLiteral("2012-02-01"), Qt::ISODate);
    QDate tomorrow = today.addDays(1);
    QDate yesterday = today.addDays(-1);
    QDate nextweek = today.addDays(7);
    QTime t1(9, 0, 0);
    QTime t2 (17, 0, 0);
    int length = t1.msecsTo(t2);

    Task *t = m_project->createTask();
    t->setName(QStringLiteral("T1"));
    m_project->addTask(t, m_project);
    t->estimate()->setUnit(Duration::Unit_d);
    t->estimate()->setExpectedEstimate(1.0);
    t->estimate()->setType(Estimate::Type_Duration);

    QString s = QStringLiteral("Calculate forward, Task: Duration -----------------------------------");
    qDebug()<<'\n'<<"Testing:"<<s;
    qDebug()<<t->name()<<t->id()<<m_project->findNode(t->id());
    qDebug()<<m_project->nodeDict();
    Debug::print(m_project, s, true);

    ScheduleManager *sm = m_project->createScheduleManager(QStringLiteral("Test Plan"));
    m_project->addScheduleManager(sm);
    sm->createSchedules();
    m_project->calculate(*sm);

    QCOMPARE(t->startTime(), m_project->startTime());
    QCOMPARE(t->endTime(), DateTime(t->startTime().addDays(1)));

    s = QStringLiteral("Calculate forward, Task: Duration w calendar -------------------------------");
    qDebug()<<'\n'<<"Testing:"<<s;

    t->estimate()->setCalendar(m_calendar);
    sm->createSchedules();
    m_project->calculate(*sm);

    QCOMPARE(t->startTime(), m_calendar->firstAvailableAfter(m_project->startTime(), m_project->endTime()));
    QCOMPARE(t->endTime(), DateTime(t->startTime().addMSecs(length)));

    ResourceGroup *g = new ResourceGroup();
    g->setName(QStringLiteral("G1"));
    m_project->addResourceGroup(g);
    Resource *r = new Resource();
    r->setName(QStringLiteral("R1"));
    r->setAvailableFrom(QDateTime(yesterday, QTime(), m_project->timeZone()));
    r->setCalendar(m_calendar);
    m_project->addResource(r);
    r->addParentGroup(g);

    ResourceRequest *rr = new ResourceRequest(r, 100);
    t->requests().addResourceRequest(rr);
    t->estimate()->setType(Estimate::Type_Effort);

    s = QStringLiteral("Calculate forward, Task: ASAP -----------------------------------");
    qDebug()<<'\n'<<"Testing:"<<s;
    m_project->setConstraintStartTime(DateTime(today, QTime(), m_project->timeZone()));
    sm = m_project->createScheduleManager(QStringLiteral("Test Plan"));
    m_project->addScheduleManager(sm);
    sm->createSchedules();
    m_project->calculate(*sm);

     Debug::print(m_project, t, s);
//      Debug::printSchedulingLog(*sm, s);

    QCOMPARE(t->earlyStart(), t->requests().workTimeAfter(m_project->startTime()));
    QVERIFY(t->lateStart() >=  t->earlyStart());
    QVERIFY(t->earlyFinish() <= t->endTime());
    QVERIFY(t->lateFinish() >= t->endTime());

    QCOMPARE(t->startTime(), DateTime(today, t1, m_project->timeZone()));
    QCOMPARE(t->endTime(), t->startTime() + Duration(0, 8, 0));
    QCOMPARE(t->plannedEffort().toHours(), 8.0);
    QVERIFY(t->schedulingError() == false);

    s = QStringLiteral("Calculate forward, Task: ASAP, Resource 50% available -----------------");
    qDebug()<<'\n'<<"Testing:"<<s;
    r->setUnits(50);
    sm = m_project->createScheduleManager(QStringLiteral("Test Plan"));
    m_project->addScheduleManager(sm);
    sm->createSchedules();
    m_project->calculate(*sm);

    Debug::print(m_project, t, s);
//     Debug::printSchedulingLog(*sm, s);

    QCOMPARE(t->earlyStart(), t->requests().workTimeAfter(m_project->startTime()));
    QVERIFY(t->lateStart() >=  t->earlyStart());
    QVERIFY(t->earlyFinish() <= t->endTime());
    QVERIFY(t->lateFinish() >= t->endTime());

    QCOMPARE(t->startTime(), DateTime(today, t1, m_project->timeZone()));
    QCOMPARE(t->endTime(), t->startTime() + Duration(1, 8, 0));
    QCOMPARE(t->plannedEffort().toHours(), 8.0);
    QVERIFY(t->schedulingError() == false);

    s = QStringLiteral("Calculate forward, Task: ASAP, Resource 50% available, Request 50% load ---------");
    qDebug()<<'\n'<<"Testing:"<<s;
    r->setUnits(50);
    rr->setUnits(50);
    sm = m_project->createScheduleManager(QStringLiteral("Test Plan"));
    m_project->addScheduleManager(sm);
    sm->createSchedules();
    m_project->calculate(*sm);

    Debug::print(m_project, s, true);
//     Debug::printSchedulingLog(*sm, s);
    QVERIFY(sm->isScheduled());

    QCOMPARE(t->earlyStart(), t->requests().workTimeAfter(m_project->startTime()));
    QVERIFY(t->lateStart() >=  t->earlyStart());
    QVERIFY(t->earlyFinish() <= t->endTime());
    QVERIFY(t->lateFinish() >= t->endTime());

    QCOMPARE(t->startTime(), DateTime(today, t1, m_project->timeZone()));
    QCOMPARE(t->endTime(), t->startTime() + Duration(3, 8, 0));
    QCOMPARE(t->plannedEffort().toHours(), 8.0);
    QVERIFY(t->schedulingError() == false);

    s = QStringLiteral("Calculate forward, Task: ASAP, Resource 200% available, Request 50% load ---------");
    qDebug()<<'\n'<<"Testing:"<<s;
    r->setUnits(200);
    rr->setUnits(50);
    sm = m_project->createScheduleManager(QStringLiteral("Test Plan"));
    m_project->addScheduleManager(sm);
    sm->createSchedules();
    m_project->calculate(*sm);

    Debug::print(m_project, s, true);
//     Debug::printSchedulingLog(*sm, s);

    QCOMPARE(t->earlyStart(), t->requests().workTimeAfter(m_project->startTime()));
    QVERIFY(t->lateStart() >=  t->earlyStart());
    QVERIFY(t->earlyFinish() <= t->endTime());
    QVERIFY(t->lateFinish() >= t->endTime());

    QCOMPARE(t->startTime(), DateTime(today, t1, m_project->timeZone()));
    QCOMPARE(t->endTime(), t->startTime() + Duration(0, 8, 0));
    QCOMPARE(t->plannedEffort().toHours(), 8.0);
    QVERIFY(t->schedulingError() == false);

    s = QStringLiteral("Calculate forward, Task: ASAP, Resource 200% available, Request 100% load ---------");
    qDebug()<<'\n'<<"Testing:"<<s;
    r->setUnits(200);
    rr->setUnits(100);
    sm = m_project->createScheduleManager(QStringLiteral("Test Plan"));
    m_project->addScheduleManager(sm);
    sm->createSchedules();
    m_project->calculate(*sm);

    Debug::print(m_project, s, true);
//     Debug::printSchedulingLog(*sm, s);

    QCOMPARE(t->earlyStart(), t->requests().workTimeAfter(m_project->startTime()));
    QVERIFY(t->lateStart() >=  t->earlyStart());
    QVERIFY(t->earlyFinish() <= t->endTime());
    QVERIFY(t->lateFinish() >= t->endTime());

    QCOMPARE(t->startTime(), DateTime(today, t1, m_project->timeZone()));
    QCOMPARE(t->endTime(), t->startTime() + Duration(0, 4, 0));
    QCOMPARE(t->plannedEffort().toHours(), 8.0);
    QVERIFY(t->schedulingError() == false);

    s = QStringLiteral("Calculate forward, 2 tasks: Resource 200% available, Request 50% load each ---------");
    qDebug()<<'\n'<<"Testing:"<<s;
    r->setUnits(200);
    rr->setUnits(50);

    Task *task2 = m_project->createTask(*t);
    task2->setName(QStringLiteral("T2"));
    m_project->addTask(task2, t);

    ResourceRequest *rr2 = new ResourceRequest(r, 50);
    task2->requests().addResourceRequest(rr2);

    sm = m_project->createScheduleManager(QStringLiteral("Test Plan"));
    m_project->addScheduleManager(sm);
    sm->createSchedules();
    m_project->calculate(*sm);

    Debug::print(m_project, s, true);
//     Debug::printSchedulingLog(*sm, s);

    QCOMPARE(t->startTime(), DateTime(today, t1, m_project->timeZone()));
    QCOMPARE(t->endTime(), t->startTime() + Duration(0, 8, 0));
    QCOMPARE(t->plannedEffort().toHours(), 8.0);

    QCOMPARE(task2->startTime(), DateTime(today, t1, m_project->timeZone()));
    QCOMPARE(task2->endTime(), task2->startTime() + Duration(0, 8, 0));
    QCOMPARE(task2->plannedEffort().toHours(), 8.0);
    QVERIFY(task2->schedulingError() == false);

    m_project->takeTask(task2);
    delete task2;

    s = QStringLiteral("Calculate forward, Task: ASAP, Resource available tomorrow --------");
    qDebug()<<'\n'<<"Testing:"<<s;
    r->setAvailableFrom(QDateTime(tomorrow, QTime(), m_project->timeZone()));
    qDebug()<<"Tomorrow:"<<QDateTime(tomorrow, QTime(), m_project->timeZone())<<r->availableFrom();
    r->setUnits(100);
    rr->setUnits(100);
    sm = m_project->createScheduleManager(QStringLiteral("Test Plan"));
    m_project->addScheduleManager(sm);
    sm->createSchedules();
    m_project->calculate(*sm);

//     Debug::print(m_project, t, s);
//     Debug::printSchedulingLog(*sm, s);

    QCOMPARE(t->earlyStart(), t->requests().workTimeAfter(m_project->startTime()));
    QVERIFY(t->lateStart() >=  t->earlyStart());
    QVERIFY(t->earlyFinish() <= t->endTime());
    QVERIFY(t->lateFinish() >= t->endTime());

    QCOMPARE(t->startTime(), DateTime(r->availableFrom().date(), t1, m_project->timeZone()));
    QCOMPARE(t->endTime(), t->startTime() + Duration(0, 8, 0));
    QVERIFY(t->schedulingError() == false);

    s = QStringLiteral("Calculate forward, Task: ALAP -----------------------------------");
    qDebug()<<'\n'<<"Testing:"<<s;
    m_project->setConstraintStartTime(DateTime(today, QTime(0,0,0), m_project->timeZone()));
    t->setConstraint(Node::ALAP);
    r->setAvailableFrom(QDateTime(yesterday, QTime(), m_project->timeZone()));
    sm = m_project->createScheduleManager(QStringLiteral("Test Plan"));
    m_project->addScheduleManager(sm);
    sm->createSchedules();
    m_project->calculate(*sm);

    Debug::print(m_project, s, true);
//     Debug::printSchedulingLog(*sm, s);

    QCOMPARE(t->earlyStart(), t->requests().workTimeAfter(m_project->startTime()));
    QVERIFY(t->lateStart() >=  t->earlyStart());
    QVERIFY(t->earlyFinish() <= t->endTime());
    QVERIFY(t->lateFinish() >= t->endTime());

    QCOMPARE(t->startTime(), DateTime(today, t1, m_project->timeZone()));
    QCOMPARE(t->endTime(), t->startTime() + Duration(0, 8, 0));
    QVERIFY(t->schedulingError() == false);

    s = QStringLiteral("Calculate forward, Task: MustStartOn -----------------------------------");
    qDebug()<<'\n'<<"Testing:"<<s;
    r->setAvailableFrom(QDateTime(yesterday, QTime(), m_project->timeZone()));
    t->setConstraint(Node::MustStartOn);
    t->setConstraintStartTime(DateTime(nextweek, t1, m_project->timeZone()));
    sm = m_project->createScheduleManager(QStringLiteral("Test Plan"));
    m_project->addScheduleManager(sm);
    sm->createSchedules();
    m_project->calculate(*sm);

    QCOMPARE(t->startTime(), DateTime(t->constraintStartTime().date(), t1, m_project->timeZone()));
    QCOMPARE(t->endTime(), t->startTime() + Duration(0, 8, 0));
    QVERIFY(t->schedulingError() == false);

    // Calculate backwards
    s = QStringLiteral("Calculate backward, Task: MustStartOn -----------------------------------");
    qDebug()<<'\n'<<"Testing:"<<s;
    m_project->setConstraintEndTime(DateTime(nextweek.addDays(1), QTime(), m_project->timeZone()));
    sm = m_project->createScheduleManager(QStringLiteral("Test Plan"));
    sm->setSchedulingDirection(true);
    m_project->addScheduleManager(sm);
    sm->createSchedules();
    m_project->calculate(*sm);

    Debug::print(m_project, t, s);
//     Debug::printSchedulingLog(*sm, s);
    
    QCOMPARE(t->startTime(), DateTime(t->constraintStartTime().date(), t1, m_project->timeZone()));
    QCOMPARE(t->endTime(), t->startTime() + Duration(0, 8, 0));
    QVERIFY(t->schedulingError() == false);

    // Calculate bacwords
    s = QStringLiteral("Calculate backwards, Task: MustFinishOn -----------------------------------");
    qDebug()<<'\n'<<"Testing:"<<s;
    m_project->setConstraintStartTime(DateTime(today, QTime(0,0,0), m_project->timeZone()));
    m_project->setConstraintEndTime(DateTime(nextweek.addDays(1), QTime(), m_project->timeZone()));
    t->setConstraint(Node::MustFinishOn);
    t->setConstraintEndTime(DateTime(nextweek.addDays(-2), t2, m_project->timeZone()));
    sm = m_project->createScheduleManager(QStringLiteral("Test Plan"));
    sm->setSchedulingDirection(true);
    m_project->addScheduleManager(sm);
    sm->createSchedules();
    m_project->calculate(*sm);

    QCOMPARE(t->endTime(), t->constraintEndTime());
    QCOMPARE(t->startTime(), t->endTime() - Duration(0, 8, 0));
    QVERIFY(t->schedulingError() == false);

    // Calculate forward
    s = QStringLiteral("Calculate forwards, Task: MustFinishOn -----------------------------------");
    qDebug()<<'\n'<<"Testing:"<<s;
    m_project->setConstraintStartTime(DateTime(today, QTime(), m_project->timeZone()));
    t->setConstraint(Node::MustFinishOn);
    t->setConstraintEndTime(DateTime(tomorrow, t2, m_project->timeZone()));
    sm = m_project->createScheduleManager(QStringLiteral("Test Plan"));
    sm->setSchedulingDirection(false);
    m_project->addScheduleManager(sm);
    sm->createSchedules();
    m_project->calculate(*sm);

    Debug::print(m_project, t, s);
//     Debug::printSchedulingLog(*sm, s);
    QCOMPARE(t->endTime(), t->constraintEndTime());
    QCOMPARE(t->startTime(), t->endTime() - Duration(0, 8, 0));
    QVERIFY(t->schedulingError() == false);

    // Calculate forward
    s = QStringLiteral("Calculate forwards, Task: StartNotEarlier -----------------------------------");
    qDebug()<<'\n'<<"Testing:"<<s;
    m_project->setConstraintStartTime(DateTime(today, QTime(), m_project->timeZone()));
    t->setConstraint(Node::StartNotEarlier);
    t->setConstraintStartTime(DateTime(today, t2, m_project->timeZone()));
    sm = m_project->createScheduleManager(QStringLiteral("Test Plan"));
    sm->setSchedulingDirection(false);
    m_project->addScheduleManager(sm);
    sm->createSchedules();
    m_project->calculate(*sm);

    Debug::print(m_project, s, true);
//     Debug::printSchedulingLog(*sm, s);

    QCOMPARE(t->startTime(), DateTime(tomorrow, t1, m_project->timeZone()));
    QCOMPARE(t->endTime(), t->startTime() + Duration(0, 8, 0));
    QVERIFY(t->schedulingError() == false);

    // Calculate backward
    s = QStringLiteral("Calculate backwards, Task: StartNotEarlier -----------------------------------");
    qDebug()<<'\n'<<"Testing:"<<s;
    m_project->setConstraintEndTime(DateTime(nextweek, QTime(), m_project->timeZone()));
    t->setConstraint(Node::StartNotEarlier);
    t->setConstraintStartTime(DateTime(today, t2, m_project->timeZone()));
    sm = m_project->createScheduleManager(QStringLiteral("Test Plan"));
    sm->setSchedulingDirection(true);
    m_project->addScheduleManager(sm);
    sm->createSchedules();
    m_project->calculate(*sm);

    Debug::print(m_project, t, s);
    Debug::print(m_project->resourceList().constFirst(), s);
//     Debug::printSchedulingLog(*sm, s);

    QVERIFY(t->lateStart() >=  t->constraintStartTime());
    QCOMPARE(t->earlyFinish(), t->endTime());
    QVERIFY(t->lateFinish() <= m_project->constraintEndTime());

    QVERIFY(t->endTime() <= t->lateFinish());
    QCOMPARE(t->startTime(), t->endTime() - Duration(0, 8, 0));
    QVERIFY(t->schedulingError() == false);

    // Calculate forward
    s = QStringLiteral("Calculate forwards, Task: FinishNotLater -----------------------------------");
    qDebug()<<'\n'<<"Testing:"<<s;
    m_project->setConstraintStartTime(DateTime(today, QTime(), m_project->timeZone()));
    t->setConstraint(Node::FinishNotLater);
    t->setConstraintEndTime(DateTime(tomorrow.addDays(1), t2, m_project->timeZone()));

    sm = m_project->createScheduleManager(QStringLiteral("Test Plan"));
    sm->setSchedulingDirection(false);
    m_project->addScheduleManager(sm);
    sm->createSchedules();
    m_project->calculate(*sm);

    QCOMPARE(t->earlyStart(), m_project->constraintStartTime());
    QVERIFY(t->startTime() >= t->earlyStart());
    QVERIFY(t->startTime() <= t->lateStart());
    QVERIFY(t->startTime() >= m_project->startTime());
    QVERIFY(t->endTime() >= t->earlyFinish());
    QVERIFY(t->endTime() <= t->lateFinish());
    QVERIFY(t->endTime() <= m_project->endTime());
    QVERIFY(t->earlyFinish() <= t->constraintEndTime());
    QVERIFY(t->lateFinish() <= m_project->constraintEndTime());
    QVERIFY(t->schedulingError() == false);

    // Calculate backward
    s = QStringLiteral("Calculate backwards, Task: FinishNotLater -----------------------------------");
    qDebug()<<'\n'<<"Testing:"<<s;
    m_project->setConstraintStartTime(DateTime(nextweek, QTime(), m_project->timeZone()));
    t->setConstraint(Node::FinishNotLater);
    t->setConstraintEndTime(DateTime(tomorrow, t2, m_project->timeZone()));

    sm = m_project->createScheduleManager(QStringLiteral("Test Plan"));
    sm->setSchedulingDirection(true);
    m_project->addScheduleManager(sm);
    sm->createSchedules();
    m_project->calculate(*sm);

    //Debug::print(m_project, t, s);

    QCOMPARE(t->earlyStart(), m_project->startTime());
    QCOMPARE(t->lateStart(),  t->startTime());
    QCOMPARE(t->earlyFinish(), t->constraintEndTime());
    QVERIFY(t->lateFinish() <= m_project->constraintEndTime());

    QCOMPARE(t->startTime(), m_project->startTime());
    QCOMPARE(t->endTime(), t->startTime() + Duration(0, 8, 0));
    QVERIFY(t->schedulingError() == false);

    // Calculate forward
    s = QStringLiteral("Calculate forward, Task: FixedInterval -----------------------------------");
    qDebug()<<'\n'<<"Testing:"<<s;
    m_project->setConstraintStartTime(DateTime(today, QTime(), m_project->timeZone()));
    t->setConstraint(Node::FixedInterval);
    t->setConstraintStartTime(DateTime(tomorrow, t1, m_project->timeZone()));
    t->setConstraintEndTime(DateTime(tomorrow, t2, m_project->timeZone()));

    sm = m_project->createScheduleManager(QStringLiteral("Test Plan"));
    sm->setSchedulingDirection(false);
    m_project->addScheduleManager(sm);
    sm->createSchedules();
    m_project->calculate(*sm);

    //Debug::print(m_project, t, s);

    QCOMPARE(t->earlyStart(), m_project->constraintStartTime());
    QCOMPARE(t->lateStart(), t->constraintStartTime());
    QCOMPARE(t->earlyFinish(), t->constraintEndTime());
    QCOMPARE(t->lateFinish(), t->constraintEndTime());

    QCOMPARE(t->startTime(), t->constraintStartTime());
    QCOMPARE(t->endTime(), t->constraintEndTime());
    QVERIFY(t->schedulingError() == false);

    // Calculate forward
    s = QStringLiteral("Calculate forwards, Task: FixedInterval -----------------------------------");
    qDebug()<<'\n'<<"Testing:"<<s;
    m_project->setConstraintStartTime(DateTime(today, QTime(), m_project->timeZone()));
    t->setConstraint(Node::FixedInterval);
    t->setConstraintStartTime(DateTime(tomorrow, QTime(), m_project->timeZone())); // outside working hours
    t->setConstraintEndTime(DateTime(tomorrow, t2, m_project->timeZone()));

    sm = m_project->createScheduleManager(QStringLiteral("Test Plan"));
    sm->setSchedulingDirection(false);
    m_project->addScheduleManager(sm);
    sm->createSchedules();
    m_project->calculate(*sm);

    QCOMPARE(t->earlyStart(), m_project->constraintStartTime());
    QCOMPARE(t->lateStart(), t->constraintStartTime());
    QCOMPARE(t->earlyFinish(), t->constraintEndTime());
    QCOMPARE(t->lateFinish(), t->constraintEndTime());

    QCOMPARE(t->startTime(), t->constraintStartTime());
    QCOMPARE(t->endTime(), t->constraintEndTime());
    QVERIFY(t->schedulingError() == false);

    // Calculate forward
    s = QStringLiteral("Calculate forwards, Task: Milestone, ASAP-------------------------");
    qDebug()<<'\n'<<"Testing:"<<s;
    m_project->setConstraintStartTime(DateTime(today, QTime(), m_project->timeZone()));
    t->setConstraint(Node::ASAP);
    t->estimate()->clear();

    sm = m_project->createScheduleManager(QStringLiteral("Test Plan"));
    sm->setSchedulingDirection(false);
    m_project->addScheduleManager(sm);
    sm->createSchedules();
    m_project->calculate(*sm);

    //Debug::print(m_project, t, s);

    QCOMPARE(t->earlyStart(), m_project->constraintStartTime());
    QCOMPARE(t->lateStart(), t->earlyStart());
    QCOMPARE(t->earlyFinish(), t->earlyStart());
    QCOMPARE(t->lateFinish(), t->earlyFinish());

    QCOMPARE(t->startTime(), t->earlyStart());
    QCOMPARE(t->endTime(), t->startTime());
    QVERIFY(t->schedulingError() == false);

    // Calculate backward
    s = QStringLiteral("Calculate backwards, Task: Milestone, ASAP-------------------------");
    qDebug()<<'\n'<<"Testing:"<<s;
    m_project->setConstraintEndTime(DateTime(today, QTime(), m_project->timeZone()));
    t->setConstraint(Node::ASAP);
    t->estimate()->clear();

    sm = m_project->createScheduleManager(QStringLiteral("Test Plan"));
    sm->setSchedulingDirection(true);
    m_project->addScheduleManager(sm);
    sm->createSchedules();
    m_project->calculate(*sm);

    QCOMPARE(t->earlyStart(), m_project->constraintStartTime());
    QCOMPARE(t->lateStart(), t->earlyStart());
    QCOMPARE(t->earlyFinish(), t->earlyStart());
    QCOMPARE(t->lateFinish(), t->earlyFinish());

    QCOMPARE(t->startTime(), t->earlyStart());
    QCOMPARE(t->endTime(), t->startTime());
    QVERIFY(t->schedulingError() == false);

    // Calculate forward
    s = QStringLiteral("Calculate forwards, Task: Milestone, ALAP-------------------------");
    qDebug()<<'\n'<<"Testing:"<<s;
    m_project->setConstraintStartTime(DateTime(today, QTime(), m_project->timeZone()));
    t->setConstraint(Node::ALAP);
    t->estimate()->clear();

    sm = m_project->createScheduleManager(QStringLiteral("Test Plan"));
    sm->setSchedulingDirection(false);
    m_project->addScheduleManager(sm);
    sm->createSchedules();
    m_project->calculate(*sm);

    QCOMPARE(t->earlyStart(), m_project->constraintStartTime());
    QCOMPARE(t->lateStart(), t->earlyStart());
    QCOMPARE(t->earlyFinish(), t->earlyStart());
    QCOMPARE(t->lateFinish(), t->earlyFinish());

    QCOMPARE(t->startTime(), t->earlyStart());
    QCOMPARE(t->endTime(), t->startTime());
    QVERIFY(t->schedulingError() == false);

    // Calculate backward
    s = QStringLiteral("Calculate backwards, Task: Milestone, ALAP-------------------------");
    qDebug()<<'\n'<<"Testing:"<<s;
    m_project->setConstraintEndTime(DateTime(today, QTime(), m_project->timeZone()));
    t->setConstraint(Node::ALAP);
    t->estimate()->clear();

    sm = m_project->createScheduleManager(QStringLiteral("Test Plan"));
    sm->setSchedulingDirection(true);
    m_project->addScheduleManager(sm);
    sm->createSchedules();
    m_project->calculate(*sm);

    QCOMPARE(t->earlyStart(), m_project->constraintStartTime());
    QCOMPARE(t->lateStart(), t->earlyStart());
    QCOMPARE(t->earlyFinish(), t->earlyStart());
    QCOMPARE(t->lateFinish(), t->earlyFinish());

    QCOMPARE(t->startTime(), t->earlyStart());
    QCOMPARE(t->endTime(), t->startTime());
    QVERIFY(t->schedulingError() == false);

    // Calculate forward
    s = QStringLiteral("Calculate forwards, Task: Milestone, MustStartOn ------------------");
    qDebug()<<'\n'<<"Testing:"<<s;
    m_project->setConstraintStartTime(DateTime(today, QTime(), m_project->timeZone()));
    t->setConstraint(Node::MustStartOn);
    t->setConstraintStartTime(DateTime(tomorrow, t1, m_project->timeZone()));

    sm = m_project->createScheduleManager(QStringLiteral("Test Plan"));
    sm->setSchedulingDirection(false);
    m_project->addScheduleManager(sm);
    sm->createSchedules();
    m_project->calculate(*sm);

    QCOMPARE(t->earlyStart(), m_project->constraintStartTime());
    QCOMPARE(t->lateStart(), t->constraintStartTime());
    QCOMPARE(t->earlyFinish(), t->lateStart());
    QCOMPARE(t->lateFinish(), t->earlyFinish());

    QCOMPARE(t->startTime(), t->constraintStartTime());
    QCOMPARE(t->endTime(), t->startTime());
    QVERIFY(t->schedulingError() == false);

    // Calculate backward
    s = QStringLiteral("Calculate backwards, Task: Milestone, MustStartOn ------------------");
    qDebug()<<'\n'<<"Testing:"<<s;
    m_project->setConstraintEndTime(DateTime(tomorrow, QTime(), m_project->timeZone()));
    t->setConstraint(Node::MustStartOn);
    t->setConstraintStartTime(DateTime(today, t1, m_project->timeZone()));

    sm = m_project->createScheduleManager(QStringLiteral("Test Plan"));
    sm->setSchedulingDirection(true);
    m_project->addScheduleManager(sm);
    sm->createSchedules();
    m_project->calculate(*sm);

    QCOMPARE(t->earlyStart(), t->constraintStartTime());
    QCOMPARE(t->lateStart(), t->earlyStart());
    QCOMPARE(t->earlyFinish(), t->earlyStart());
    QCOMPARE(t->lateFinish(), m_project->constraintEndTime());

    QCOMPARE(t->startTime(), t->constraintStartTime());
    QCOMPARE(t->endTime(), t->startTime());
    QVERIFY(t->schedulingError() == false);

    // Calculate forward
    s = QStringLiteral("Calculate forwards, Task: Milestone, MustFinishOn ------------------");
    qDebug()<<'\n'<<"Testing:"<<s;
    m_project->setConstraintStartTime(DateTime(today, QTime(), m_project->timeZone()));
    t->setConstraint(Node::MustFinishOn);
    t->setConstraintEndTime(DateTime(tomorrow, t1, m_project->timeZone()));

    sm = m_project->createScheduleManager(QStringLiteral("Test Plan"));
    sm->setSchedulingDirection(false);
    m_project->addScheduleManager(sm);
    sm->createSchedules();
    m_project->calculate(*sm);

    QCOMPARE(t->earlyStart(), m_project->constraintStartTime());
    QCOMPARE(t->lateStart(), t->constraintEndTime());
    QCOMPARE(t->earlyFinish(), t->lateStart());
    QCOMPARE(t->lateFinish(), t->earlyFinish());

    QCOMPARE(t->startTime(), t->constraintEndTime());
    QCOMPARE(t->endTime(), t->startTime());
    QVERIFY(t->schedulingError() == false);

    QCOMPARE(m_project->endTime(), t->endTime());

    // Calculate backward
    s = QStringLiteral("Calculate backwards, Task: Milestone, MustFinishOn ------------------");
    qDebug()<<'\n'<<"Testing:"<<s;
    m_project->setConstraintEndTime(DateTime(tomorrow, QTime(), m_project->timeZone()));
    t->setConstraint(Node::MustFinishOn);
    t->setConstraintEndTime(DateTime(today, t1, m_project->timeZone()));

    sm = m_project->createScheduleManager(QStringLiteral("Test Plan"));
    sm->setSchedulingDirection(true);
    m_project->addScheduleManager(sm);
    sm->createSchedules();
    m_project->calculate(*sm);

    QCOMPARE(t->earlyStart(), t->constraintEndTime());
    QCOMPARE(t->lateStart(), t->constraintEndTime());
    QCOMPARE(t->earlyFinish(), t->lateStart());
    QCOMPARE(t->lateFinish(), m_project->constraintEndTime());

    QCOMPARE(t->startTime(), t->constraintEndTime());
    QCOMPARE(t->endTime(), t->startTime());
    QVERIFY(t->schedulingError() == false);

    QCOMPARE(m_project->startTime(), t->startTime());

    // Calculate forward
    s = QStringLiteral("Calculate forwards, Task: Milestone, StartNotEarlier ---------------");
    qDebug()<<'\n'<<"Testing:"<<s;
    m_project->setConstraintStartTime(DateTime(today, QTime(), m_project->timeZone()));
    t->setConstraint(Node::StartNotEarlier);
    t->setConstraintEndTime(DateTime(tomorrow, t1, m_project->timeZone()));

    sm = m_project->createScheduleManager(QStringLiteral("Test Plan"));
    sm->setSchedulingDirection(false);
    m_project->addScheduleManager(sm);
    sm->createSchedules();
    m_project->calculate(*sm);

    QCOMPARE(t->earlyStart(), m_project->constraintStartTime());
    QCOMPARE(t->lateStart(), t->constraintStartTime());
    QCOMPARE(t->earlyFinish(), t->lateStart());
    QCOMPARE(t->lateFinish(), t->earlyFinish());

    QCOMPARE(t->startTime(), t->constraintStartTime());
    QCOMPARE(t->endTime(), t->startTime());
    QVERIFY(t->schedulingError() == false);

    QCOMPARE(m_project->endTime(), t->endTime());

    // Calculate backward
    s = QStringLiteral("Calculate backwards, Task: Milestone, StartNotEarlier ---------------");
    qDebug()<<'\n'<<"Testing:"<<s;
    m_project->setConstraintEndTime(DateTime(tomorrow, QTime(), m_project->timeZone()));
    t->setConstraint(Node::StartNotEarlier);
    t->setConstraintStartTime(DateTime(today, t1, m_project->timeZone()));

    sm = m_project->createScheduleManager(QStringLiteral("Test Plan"));
    sm->setSchedulingDirection(true);
    m_project->addScheduleManager(sm);
    sm->createSchedules();
    m_project->calculate(*sm);

    Debug::print(m_project, s, true);

    QVERIFY(t->earlyStart() >= t->constraintStartTime());
    QVERIFY(t->lateStart() >= t->earlyStart());
    QVERIFY(t->earlyFinish() <= t->lateFinish());
    QVERIFY(t->lateFinish() >= t->constraintStartTime());

    QVERIFY(t->startTime() >= t->constraintStartTime());
    QCOMPARE(t->endTime(), t->startTime());
    QVERIFY(t->schedulingError() == false);

    QCOMPARE(m_project->startTime(), t->startTime());

    // Calculate forward
    s = QStringLiteral("Calculate forwards, Task: Milestone, FinishNotLater ---------------");
    qDebug()<<'\n'<<"Testing:"<<s;
    m_project->setConstraintStartTime(DateTime(today, QTime(), m_project->timeZone()));
    t->setConstraint(Node::FinishNotLater);
    t->setConstraintEndTime(DateTime(tomorrow, t1, m_project->timeZone()));

    sm = m_project->createScheduleManager(QStringLiteral("Test Plan"));
    sm->setSchedulingDirection(false);
    m_project->addScheduleManager(sm);
    sm->createSchedules();
    m_project->calculate(*sm);

    QVERIFY(t->earlyStart() <= t->constraintEndTime());
    QVERIFY(t->lateStart() <= t->constraintEndTime());
    QVERIFY(t->earlyFinish() >= t->earlyStart());
    QVERIFY(t->lateFinish() >= t->earlyFinish());

    QVERIFY(t->startTime() <= t->constraintEndTime());
    QCOMPARE(t->endTime(), t->startTime());
    QVERIFY(t->schedulingError() == false);

    QCOMPARE(m_project->endTime(), t->endTime());

    // Calculate backward
    s = QStringLiteral("Calculate backwards, Task: Milestone, FinishNotLater ---------------");
    qDebug()<<'\n'<<"Testing:"<<s;
    m_project->setConstraintEndTime(DateTime(tomorrow, QTime(), m_project->timeZone()));
    t->setConstraint(Node::FinishNotLater);
    t->setConstraintEndTime(DateTime(today, t1, m_project->timeZone()));

    sm = m_project->createScheduleManager(QStringLiteral("Test Plan"));
    sm->setSchedulingDirection(true);
    m_project->addScheduleManager(sm);
    sm->createSchedules();
    m_project->calculate(*sm);

    QCOMPARE(t->earlyStart(), t->constraintStartTime());
    QCOMPARE(t->lateStart(), t->earlyStart());
    QCOMPARE(t->earlyFinish(), t->lateStart());
    QCOMPARE(t->lateFinish(), m_project->constraintEndTime());

    QCOMPARE(t->startTime(), t->constraintEndTime());
    QCOMPARE(t->endTime(), t->startTime());
    QVERIFY(t->schedulingError() == false);

    QCOMPARE(m_project->startTime(), t->startTime());

    // Calculate forward
    s = QStringLiteral("Calculate forward, 2 Tasks, no overbooking ----------");
    qDebug()<<'\n'<<"Testing:"<<s;
    m_project->setConstraintStartTime(DateTime(today, QTime(), m_project->timeZone()));
    m_project->setConstraintEndTime(DateTime(today, QTime(), m_project->timeZone()).addDays(4));
    t->setConstraint(Node::ASAP);
    t->estimate()->setUnit(Duration::Unit_d);
    t->estimate()->setExpectedEstimate(2.0);

    Task *tsk2 = m_project->createTask(*t);
    tsk2->setName(QStringLiteral("T2"));
    m_project->addTask(tsk2, m_project);

    rr = new ResourceRequest(r, 100);
    tsk2->requests().addResourceRequest(rr);

    sm = m_project->createScheduleManager(QStringLiteral("Test Plan"));
    sm->setAllowOverbooking(false);
    sm->setSchedulingDirection(false);
    m_project->addScheduleManager(sm);
    sm->createSchedules();
    m_project->calculate(*sm);

    Debug::print(m_project, t, s);
    Debug::print(m_project, tsk2, s);
//     Debug::printSchedulingLog(*sm, s);

    QCOMPARE(t->earlyStart(), t->requests().workTimeAfter(m_project->constraintStartTime()));
    QCOMPARE(t->lateStart(), tsk2->startTime());
    QCOMPARE(t->earlyFinish(), DateTime(tomorrow, t2, m_project->timeZone()));
    QCOMPARE(t->lateFinish(), t->lateFinish());

    QCOMPARE(t->startTime(), DateTime(today, t1, m_project->timeZone()));
    QCOMPARE(t->endTime(), t->earlyFinish());
    QVERIFY(t->schedulingError() == false);

    QCOMPARE(tsk2->earlyStart(), t->earlyStart());
    QCOMPARE(tsk2->lateStart(), t->earlyFinish() + Duration(0, 16, 0));
    QCOMPARE(tsk2->earlyFinish(), DateTime(tomorrow, t2, m_project->timeZone()));
    QCOMPARE(tsk2->lateFinish(), t->lateFinish());

    QCOMPARE(tsk2->startTime(), DateTime(tomorrow.addDays(1), t1, m_project->timeZone()));
    QCOMPARE(tsk2->endTime(), tsk2->lateFinish());
    QVERIFY(tsk2->schedulingError() == false);

    QCOMPARE(m_project->endTime(), tsk2->endTime());

    // Calculate forward
    s = QStringLiteral("Calculate forward, 2 Tasks, relation ---------------");
    qDebug()<<'\n'<<"Testing:"<<s;
    m_project->setConstraintStartTime(DateTime(today, QTime(), m_project->timeZone()));
    t->setConstraint(Node::ASAP);
    t->estimate()->setUnit(Duration::Unit_d);
    t->estimate()->setExpectedEstimate(2.0);

    Relation *rel = new Relation(t, tsk2);
    bool relationAdded = m_project->addRelation(rel);
    QVERIFY(relationAdded);

    sm = m_project->createScheduleManager(QStringLiteral("Test Plan"));
    sm->setAllowOverbooking(true);
    sm->setSchedulingDirection(false);
    m_project->addScheduleManager(sm);
    sm->createSchedules();
    m_project->calculate(*sm);

//    Debug::print(m_project, t, s);
//    Debug::print(m_project, tsk2, s);
//    Debug::printSchedulingLog(*sm, s);

    QCOMPARE(t->earlyStart(), t->requests().workTimeAfter(m_project->constraintStartTime()));
    QCOMPARE(t->lateStart(), DateTime(today, t1, m_project->timeZone()));
    QCOMPARE(t->earlyFinish(), DateTime(tomorrow, t2, m_project->timeZone()));
    QCOMPARE(t->lateFinish(), t->lateFinish());

    QCOMPARE(t->startTime(), DateTime(today, t1, m_project->timeZone()));
    QCOMPARE(t->endTime(), t->earlyFinish());
    QVERIFY(t->schedulingError() == false);

    QCOMPARE(tsk2->earlyStart(), tsk2->requests().workTimeAfter(t->earlyFinish()));
    QCOMPARE(tsk2->lateStart(), DateTime(tomorrow.addDays(1), t1, m_project->timeZone()));
    QCOMPARE(tsk2->earlyFinish(), DateTime(tomorrow.addDays(2), t2, m_project->timeZone()));
    QCOMPARE(tsk2->lateFinish(), tsk2->earlyFinish());

    QCOMPARE(tsk2->startTime(), DateTime(tomorrow.addDays(1), t1, m_project->timeZone()));
    QCOMPARE(tsk2->endTime(), tsk2->earlyFinish());
    QVERIFY(tsk2->schedulingError() == false);

    QCOMPARE(m_project->endTime(), tsk2->endTime());

}

void ProjectTester::scheduleFullday()
{
    QString s = QStringLiteral("Full day, 1 resource works 24 hours a day -------------");
    qDebug()<<'\n'<<"Testing:"<<s;

    const DateTime dt(QDate(2011, 9, 1), QTime(0, 0, 0), m_project->timeZone());
    m_project->setConstraintStartTime(dt);
    m_project->setConstraintEndTime(dt.addDays(7));
    qDebug()<<m_project->constraintStartTime()<<m_project->constraintEndTime();
    Calendar *c = new Calendar(QStringLiteral("Test"));
    QTime t1(0,0,0);
    int length = 24*60*60*1000;

    for (int i = 1; i <= 7; ++i) {
        CalendarDay *wd1 = c->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(t1, length));
    }
    m_project->addCalendar(c);

    ResourceGroup *g = new ResourceGroup();
    g->setName(QStringLiteral("G1"));
    m_project->addResourceGroup(g);
    Resource *r = new Resource();
    r->setName(QStringLiteral("R1"));
    r->setCalendar(c);
    r->setAvailableFrom(m_project->constraintStartTime());
    r->setAvailableUntil(r->availableFrom().addDays(21));
    m_project->addResource(r);
    r->addParentGroup(g);

    Task *t = m_project->createTask();
    t->setName(QStringLiteral("T1"));
    t->setId(m_project->uniqueNodeId());
    m_project->addTask(t, m_project);
    t->estimate()->setUnit(Duration::Unit_d);
    t->estimate()->setExpectedEstimate(3 * 14.0);
    t->estimate()->setType(Estimate::Type_Effort);

    t->requests().addResourceRequest(new ResourceRequest(r, 100));

    ScheduleManager *sm = m_project->createScheduleManager(QStringLiteral("Test Plan"));
    m_project->addScheduleManager(sm);
    sm->createSchedules();
    m_project->calculate(*sm);

//      Debug::print(c, s);
//      Debug::print(m_project, t, s);
//      Debug::print(r, s, true);
//      Debug::printSchedulingLog(*sm, s);

    QCOMPARE(t->startTime(), m_project->startTime());
    QCOMPARE(t->endTime(), DateTime(t->startTime().addDays(14)));

    s = QStringLiteral("Full day, 8 hour shifts, 3 resources ---------------");
    qDebug()<<'\n'<<"Testing:"<<s;
    int hour = 60*60*1000;
    Calendar *c1 = new Calendar(QStringLiteral("Test 1"));
    for (int i = 1; i <= 7; ++i) {
        CalendarDay *wd1 = c1->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(QTime(6, 0, 0), 8*hour));
    }
    m_project->addCalendar(c1);
    Calendar *c2 = new Calendar(QStringLiteral("Test 2"));
    for (int i = 1; i <= 7; ++i) {
        CalendarDay *wd1 = c2->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(QTime(14, 0, 0), 8*hour));
    }
    m_project->addCalendar(c2);
    Calendar *c3 = new Calendar(QStringLiteral("Test 3"));
    for (int i = 1; i <= 7; ++i) {
        CalendarDay *wd1 = c3->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(QTime(0, 0, 0), 6*hour));
        wd1->addInterval(TimeInterval(QTime(22, 0, 0), 2*hour));
    }
    m_project->addCalendar(c3);

    r->setCalendar(c1);

    r = new Resource();
    r->setName(QStringLiteral("R2"));
    r->setCalendar(c2);
    r->setAvailableFrom(m_project->constraintStartTime());
    r->setAvailableUntil(r->availableFrom().addDays(21));
    m_project->addResource(r);
    r->addParentGroup(g);
    t->requests().addResourceRequest(new ResourceRequest(r, 100));

    r = new Resource();
    r->setName(QStringLiteral("R3"));
    r->setCalendar(c3);
    r->setAvailableFrom(m_project->constraintStartTime());
    r->setAvailableUntil(r->availableFrom().addDays(21));
    m_project->addResource(r);
    r->addParentGroup(g);
    t->requests().addResourceRequest(new ResourceRequest(r, 100));


    sm->createSchedules();
    m_project->calculate(*sm);

//    Debug::print(m_project, t, s);
//    Debug::printSchedulingLog(*sm, s);

    QCOMPARE(t->startTime(), m_project->startTime());
    QCOMPARE(t->endTime(), DateTime(t->startTime().addDays(14)));
}

void ProjectTester::scheduleFulldayDstSpring()
{
    QString s = QStringLiteral("Daylight saving time - Spring, 1 resource works 24 hours a day -------------");
    qDebug()<<'\n'<<"Testing:"<<s;
    Project project;
    project.setName(QStringLiteral("DST"));
    project.setId(project.uniqueNodeId());
    project.registerNodeId(&project);
    project.setConstraintStartTime(DateTime(QDate::fromString(QStringLiteral("2011-03-25"), Qt::ISODate), QTime(), project.timeZone()));
    project.setConstraintEndTime(DateTime(QDate::fromString(QStringLiteral("2011-03-29"), Qt::ISODate), QTime(), project.timeZone()));
    qDebug()<<project.constraintStartTime()<<project.constraintEndTime();
    Calendar *c = new Calendar(QStringLiteral("Test"));
    QTime t1(0,0,0);
    int length = 24*60*60*1000;

    for (int i = 1; i <= 7; ++i) {
        CalendarDay *wd1 = c->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(t1, length));
    }
    project.addCalendar(c);

    ResourceGroup *g = new ResourceGroup();
    g->setName(QStringLiteral("G1"));
    project.addResourceGroup(g);
    Resource *r = new Resource();
    r->setName(QStringLiteral("R1"));
    r->setCalendar(c);
    r->setAvailableFrom(project.constraintStartTime().addDays(-1));
    r->setAvailableUntil(r->availableFrom().addDays(21));
    project.addResource(r);
    r->addParentGroup(g);

    Task *t = project.createTask();
    t->setName(QStringLiteral("T1"));
    t->setId(project.uniqueNodeId());
    project.addTask(t, &project);
    t->estimate()->setUnit(Duration::Unit_d);
    t->estimate()->setExpectedEstimate(3 * 4.0);
    t->estimate()->setType(Estimate::Type_Effort);

    t->requests().addResourceRequest(new ResourceRequest(r, 100));

    ScheduleManager *sm = project.createScheduleManager(QStringLiteral("Test Plan"));
    project.addScheduleManager(sm);
    sm->createSchedules();
    project.calculate(*sm);

//      Debug::print(c, s);
     Debug::print(&project, t, s);
//      Debug::print(r, s, true);
//      Debug::printSchedulingLog(*sm, s);

    QCOMPARE(t->startTime(), project.constraintStartTime());
    QCOMPARE(t->endTime(), t->startTime() + Duration(4, 0, 0));

    s = QStringLiteral("Daylight saving time - Spring, Backward: 1 resource works 24 hours a day -------------");
    qDebug()<<'\n'<<"Testing:"<<s;

    // make room for the task
    project.setConstraintStartTime(DateTime(QDate::fromString(QStringLiteral("2011-03-24"), Qt::ISODate), QTime(), project.timeZone()));

    sm = project.createScheduleManager(QStringLiteral("Test Backward"));
    project.addScheduleManager(sm);
    sm->setSchedulingDirection(true);
    sm->createSchedules();
    project.calculate(*sm);

//      Debug::print(c, s);
     Debug::print(&project, t, s);
//      Debug::print(r, s, true);
//      Debug::printSchedulingLog(*sm, s);

    QCOMPARE(t->endTime(), project.constraintEndTime());
    QCOMPARE(t->startTime(), t->endTime() - Duration(4, 0, 0));

    s = QStringLiteral("Daylight saving time - Spring, 8 hour shifts, 3 resources ---------------");
    qDebug()<<'\n'<<"Testing:"<<s;
    
    int hour = 60*60*1000;
    Calendar *c1 = new Calendar(QStringLiteral("Test 1"));
    for (int i = 1; i <= 7; ++i) {
        CalendarDay *wd1 = c1->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(QTime(6, 0, 0), 8*hour));
    }
    project.addCalendar(c1);
    Calendar *c2 = new Calendar(QStringLiteral("Test 2"));
    for (int i = 1; i <= 7; ++i) {
        CalendarDay *wd1 = c2->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(QTime(14, 0, 0), 8*hour));
    }
    project.addCalendar(c2);
    Calendar *c3 = new Calendar(QStringLiteral("Test 3"));
    for (int i = 1; i <= 7; ++i) {
        CalendarDay *wd1 = c3->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(QTime(0, 0, 0), 6*hour));
        wd1->addInterval(TimeInterval(QTime(22, 0, 0), 2*hour));
    }
    project.addCalendar(c3);

    r->setCalendar(c1);

    r = new Resource();
    r->setName(QStringLiteral("R2"));
    r->setCalendar(c2);
    r->setAvailableFrom(project.constraintStartTime().addDays(-1));
    r->setAvailableUntil(r->availableFrom().addDays(21));
    project.addResource(r);
    r->addParentGroup(g);
    t->requests().addResourceRequest(new ResourceRequest(r, 100));

    r = new Resource();
    r->setName(QStringLiteral("R3"));
    r->setCalendar(c3);
    r->setAvailableFrom(project.constraintStartTime().addDays(-1));
    r->setAvailableUntil(r->availableFrom().addDays(21));
    project.addResource(r);
    r->addParentGroup(g);
    t->requests().addResourceRequest(new ResourceRequest(r, 100));

    project.setConstraintStartTime(DateTime(QDate::fromString(QStringLiteral("2011-03-25"), Qt::ISODate), QTime(), project.timeZone()));

    sm = project.createScheduleManager(QStringLiteral("Test Foreword"));
    project.addScheduleManager(sm);
    sm->createSchedules();
    project.calculate(*sm);

//    Debug::print(&project, t, s);
//    Debug::printSchedulingLog(*sm, s);

    QCOMPARE(t->startTime().toUTC(), project.constraintStartTime().toUTC());
    QCOMPARE(t->endTime(), t->startTime() + Duration(4, 0, 0));

    s = QStringLiteral("Daylight saving time - Spring, Backward: 8 hour shifts, 3 resources ---------------");
    qDebug()<<'\n'<<"Testing:"<<s;

    project.setConstraintStartTime(DateTime(QDate::fromString(QStringLiteral("2011-03-24"), Qt::ISODate), QTime(), project.timeZone()));

    sm = project.createScheduleManager(QStringLiteral("Test Backward"));
    project.addScheduleManager(sm);
    sm->setSchedulingDirection(true);
    sm->createSchedules();
    project.calculate(*sm);

   Debug::print(&project, t, s);
//    Debug::printSchedulingLog(*sm, s);

    QCOMPARE(t->endTime(), project.constraintEndTime());
    QCOMPARE(t->startTime(), t->endTime() - Duration(4, 0, 0));

}

void ProjectTester::scheduleFulldayDstFall()
{
    QString s = QStringLiteral("Daylight saving time - Fall, 1 resource works 24 hours a day -------------");
    qDebug()<<'\n'<<"Testing:"<<s;
    Project project;
    project.setName(QStringLiteral("DST"));
    project.setId(project.uniqueNodeId());
    project.registerNodeId(&project);
    project.setConstraintStartTime(DateTime(QDate::fromString(QStringLiteral("2011-10-28"), Qt::ISODate), QTime(), project.timeZone()));
    project.setConstraintEndTime(DateTime(QDate::fromString(QStringLiteral("2011-11-01"), Qt::ISODate), QTime(), project.timeZone()));
    qDebug()<<project.constraintStartTime()<<project.constraintEndTime();
    Calendar *c = new Calendar(QStringLiteral("Test"));
    QTime t1(0,0,0);
    int length = 24*60*60*1000;

    for (int i = 1; i <= 7; ++i) {
        CalendarDay *wd1 = c->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(t1, length));
    }
    project.addCalendar(c);

    ResourceGroup *g = new ResourceGroup();
    g->setName(QStringLiteral("G1"));
    project.addResourceGroup(g);
    Resource *r = new Resource();
    r->setName(QStringLiteral("R1"));
    r->setCalendar(c);
    r->setAvailableFrom(project.constraintStartTime().addDays(-1));
    r->setAvailableUntil(r->availableFrom().addDays(21));
    project.addResource(r);
    r->addParentGroup(g);

    Task *t = project.createTask();
    t->setName(QStringLiteral("T1"));
    t->setId(project.uniqueNodeId());
    project.addTask(t, &project);
    t->estimate()->setUnit(Duration::Unit_d);
    t->estimate()->setExpectedEstimate(3 * 4.0);
    t->estimate()->setType(Estimate::Type_Effort);

    t->requests().addResourceRequest(new ResourceRequest(r, 100));

    ScheduleManager *sm = project.createScheduleManager(QStringLiteral("Test Plan"));
    project.addScheduleManager(sm);
    sm->createSchedules();
    project.calculate(*sm);

//      Debug::print(c, s);
     Debug::print(&project, t, s);
//      Debug::print(r, s, true);
//      Debug::printSchedulingLog(*sm, s);

    QCOMPARE(t->startTime(), project.constraintStartTime());
    QCOMPARE(t->endTime(), t->startTime() + Duration(4, 0, 0));

    s = QStringLiteral("Daylight saving time - Fall, Backward: 1 resource works 24 hours a day -------------");
    qDebug()<<'\n'<<"Testing:"<<s;

    sm = project.createScheduleManager(QStringLiteral("Test Backward"));
    project.addScheduleManager(sm);
    sm->setSchedulingDirection(true);
    sm->createSchedules();
    project.calculate(*sm);

//      Debug::print(c, s);
     Debug::print(&project, t, s);
//      Debug::print(r, s, true);
//      Debug::printSchedulingLog(*sm, s);

    QCOMPARE(t->endTime(), project.constraintEndTime());
    QCOMPARE(t->startTime(), t->endTime() - Duration(4, 0, 0));

    s = QStringLiteral("Daylight saving time - Fall, 8 hour shifts, 3 resources ---------------");
    qDebug()<<'\n'<<"Testing:"<<s;
    int hour = 60*60*1000;
    Calendar *c1 = new Calendar(QStringLiteral("Test 1"));
    for (int i = 1; i <= 7; ++i) {
        CalendarDay *wd1 = c1->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(QTime(6, 0, 0), 8*hour));
    }
    project.addCalendar(c1);
    Calendar *c2 = new Calendar(QStringLiteral("Test 2"));
    for (int i = 1; i <= 7; ++i) {
        CalendarDay *wd1 = c2->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(QTime(14, 0, 0), 8*hour));
    }
    project.addCalendar(c2);
    Calendar *c3 = new Calendar(QStringLiteral("Test 3"));
    for (int i = 1; i <= 7; ++i) {
        CalendarDay *wd1 = c3->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(QTime(0, 0, 0), 6*hour));
        wd1->addInterval(TimeInterval(QTime(22, 0, 0), 2*hour));
    }
    project.addCalendar(c3);

    r->setCalendar(c1);

    r = new Resource();
    r->setName(QStringLiteral("R2"));
    r->setCalendar(c2);
    r->setAvailableFrom(project.constraintStartTime().addDays(-1));
    r->setAvailableUntil(r->availableFrom().addDays(21));
    project.addResource(r);
    r->addParentGroup(g);
    t->requests().addResourceRequest(new ResourceRequest(r, 100));

    r = new Resource();
    r->setName(QStringLiteral("R3"));
    r->setCalendar(c3);
    r->setAvailableFrom(project.constraintStartTime().addDays(-1));
    r->setAvailableUntil(r->availableFrom().addDays(21));
    project.addResource(r);
    r->addParentGroup(g);
    t->requests().addResourceRequest(new ResourceRequest(r, 100));


    sm = project.createScheduleManager(QStringLiteral("Test Foreword"));
    project.addScheduleManager(sm);
    sm->createSchedules();
    project.calculate(*sm);

//    Debug::print(&project, t, s);
//    Debug::printSchedulingLog(*sm, s);

    QCOMPARE(t->startTime(), project.constraintStartTime());
    QCOMPARE(t->endTime(), t->startTime() + Duration(4, 0, 0));

    s = QStringLiteral("Daylight saving time - Fall, Backward: 8 hour shifts, 3 resources ---------------");
    qDebug()<<'\n'<<"Testing:"<<s;

    sm = project.createScheduleManager(QStringLiteral("Test Foreword"));
    project.addScheduleManager(sm);
    sm->setSchedulingDirection(true);
    sm->createSchedules();
    project.calculate(*sm);

    QCOMPARE(t->endTime(), project.constraintEndTime());
    QCOMPARE(t->startTime(), t->endTime() - Duration(4, 0, 0));
}

void ProjectTester::scheduleWithExternalAppointments()
{
    qputenv("TZ", QByteArray("America/Los_Angeles"));
    Project project;
    project.setName(QStringLiteral("P1"));
    project.setId(project.uniqueNodeId());
    project.registerNodeId(&project);
    DateTime targetstart = DateTime(QDate::fromString(QStringLiteral("2012-02-01"), Qt::ISODate), QTime(0,0,0), project.timeZone());
    DateTime targetend = DateTime(targetstart.addDays(3));
    project.setConstraintStartTime(targetstart);
    project.setConstraintEndTime(targetend);

    Calendar c(QStringLiteral("Test"));
    QTime t1(0,0,0);
    int length = 24*60*60*1000;

    for (int i = 1; i <= 7; ++i) {
        CalendarDay *wd1 = c.weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(t1, length));
    }
    ResourceGroup *g = new ResourceGroup();
    g->setName(QStringLiteral("G1"));
    project.addResourceGroup(g);
    Resource *r = new Resource();
    r->setName(QStringLiteral("R1"));
    r->setCalendar(&c);
    project.addResource(r);
    r->addParentGroup(g);

    r->addExternalAppointment(QStringLiteral("Ext-1"), QStringLiteral("External project 1"), targetstart, targetstart.addDays(1), 100);
    r->addExternalAppointment(QStringLiteral("Ext-1"), QStringLiteral("External project 1"), targetend.addDays(-1), targetend, 100);

    Task *t = project.createTask();
    t->setName(QStringLiteral("T1"));
    project.addTask(t, &project);
    t->estimate()->setUnit(Duration::Unit_h);
    t->estimate()->setExpectedEstimate(8.0);
    t->estimate()->setType(Estimate::Type_Effort);

    t->requests().addResourceRequest(new ResourceRequest(r, 100));

    ScheduleManager *sm = project.createScheduleManager(QStringLiteral("Test Plan"));
    project.addScheduleManager(sm);
    sm->createSchedules();
    project.calculate(*sm);

    QString s = QStringLiteral("Schedule with external appointments ----------");
    qDebug()<<'\n'<<"Testing:"<<s;

    Debug::print(r, s);
    Debug::print(&project, s, true);
//     Debug::printSchedulingLog(*sm, s);

    QCOMPARE(t->startTime(), targetstart + Duration(1, 0, 0));
    QCOMPARE(t->endTime(), t->startTime() + Duration(0, 8, 0));

    sm->setAllowOverbooking(true);
    sm->createSchedules();
    project.calculate(*sm);

//     Debug::printSchedulingLog(*sm);

    QCOMPARE(t->startTime(), targetstart);
    QCOMPARE(t->endTime(), t->startTime() + Duration(0, 8, 0));

    sm->setAllowOverbooking(false);
    sm->setSchedulingDirection(true); // backwards
    sm->createSchedules();
    project.calculate(*sm);

    Debug::print(&project, s, true);
    Debug::print(r, QString(), true);
//     Debug::printSchedulingLog(*sm, s);

    QCOMPARE(t->startTime(), targetend - Duration(1, 8, 0));
    QCOMPARE(t->endTime(), t->startTime() + Duration(0, 8, 0));

    sm->setAllowOverbooking(true);
    sm->createSchedules();
    project.calculate(*sm);

//     Debug::printSchedulingLog(*sm);

    QCOMPARE(t->startTime(), targetend - Duration(0, 8, 0));
    QCOMPARE(t->endTime(), targetend);

    sm->setAllowOverbooking(false);
    r->clearExternalAppointments();
    sm->createSchedules();
    project.calculate(*sm);

//     Debug::printSchedulingLog(*sm);

    QCOMPARE(t->endTime(), targetend);
    QCOMPARE(t->startTime(),  t->endTime() - Duration(0, 8, 0));
    qunsetenv("TZ");
}

void ProjectTester::reschedule()
{
    Project project;
    project.setName(QStringLiteral("P1"));
    project.setId(project.uniqueNodeId());
    project.registerNodeId(&project);
    DateTime targetstart = DateTime(QDate::fromString(QStringLiteral("2012-02-01"), Qt::ISODate), QTime(0,0,0), project.timeZone());
    DateTime targetend = DateTime(targetstart.addDays(7));
    project.setConstraintStartTime(targetstart);
    project.setConstraintEndTime(targetend);

    Calendar *c = new Calendar(QStringLiteral("Test"));
    QTime t1(8,0,0);
    int length = 8*60*60*1000; // 8 hours

    for (int i = 1; i <= 7; ++i) {
        CalendarDay *wd1 = c->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(t1, length));
    }
    project.addCalendar(c);
    ResourceGroup *g = new ResourceGroup();
    g->setName(QStringLiteral("G1"));
    project.addResourceGroup(g);
    Resource *r = new Resource();
    r->setName(QStringLiteral("R1"));
    r->setCalendar(c);
    project.addResource(r);
    r->addParentGroup(g);

    QString s = QStringLiteral("Re-schedule; schedule tasks T1, T2, T3 ---------------");
    qDebug()<<'\n'<<"Testing:"<<s;

    Task *task1 = project.createTask();
    task1->setName(QStringLiteral("T1"));
    project.addTask(task1, &project);
    task1->estimate()->setUnit(Duration::Unit_h);
    task1->estimate()->setExpectedEstimate(8.0);
    task1->estimate()->setType(Estimate::Type_Effort);

    task1->requests().addResourceRequest(new ResourceRequest(r, 100));

    Task *task2 = project.createTask();
    task2->setName(QStringLiteral("T2"));
    project.addTask(task2, &project);
    task2->estimate()->setUnit(Duration::Unit_h);
    task2->estimate()->setExpectedEstimate(8.0);
    task2->estimate()->setType(Estimate::Type_Effort);

    task2->requests().addResourceRequest(new ResourceRequest(r, 100));

    Task *task3 = project.createTask();
    task3->setName(QStringLiteral("T3"));
    project.addTask(task3, &project);
    task3->estimate()->setUnit(Duration::Unit_h);
    task3->estimate()->setExpectedEstimate(8.0);
    task3->estimate()->setType(Estimate::Type_Effort);

    task3->requests().addResourceRequest(new ResourceRequest(r, 100));

    Relation *rel = new Relation(task1, task2);
    project.addRelation(rel);
    rel = new Relation(task1, task3);
    project.addRelation(rel);

    ScheduleManager *sm = project.createScheduleManager(QStringLiteral("Plan"));
    project.addScheduleManager(sm);
    sm->createSchedules();
    project.calculate(*sm);

//     Debug::print(&project, task1, s, true);
//     Debug::print(&project, task2, s, true);
//     Debug::print(&project, task3, s, true);
//     Debug::print(r, s);
//     Debug::printSchedulingLog(*sm);

    QVERIFY(task1->startTime() >= c->firstAvailableAfter(targetstart, targetend));
    QVERIFY(task1->startTime() <= c->firstAvailableBefore(targetend, targetstart));
    QCOMPARE(task1->endTime(), task1->startTime() + Duration(0, 8, 0));

    QVERIFY(task2->startTime() >= c->firstAvailableAfter(targetstart, targetend));
    QVERIFY(task2->startTime() <= c->firstAvailableBefore(targetend, targetstart));
    QCOMPARE(task2->endTime(), task2->startTime() + Duration(0, 8, 0));

    QVERIFY(task3->startTime() >= c->firstAvailableAfter(targetstart, targetend));
    QVERIFY(task3->startTime() <= c->firstAvailableBefore(targetend, targetstart));
    QCOMPARE(task3->endTime(), task3->startTime() + Duration(0, 8, 0));


    DateTime restart = task1->endTime();
    s = QStringLiteral("Re-schedule; re-schedule from %1 - tasks T1 (finished), T2, T3 ------").arg(restart.toString());
    qDebug()<<'\n'<<"Testing:"<<s;

    task1->completion().setStarted(true);
    task1->completion().setPercentFinished(task1->endTime().date(), 100);
    task1->completion().setFinished(true);

    ScheduleManager *child = project.createScheduleManager(QStringLiteral("Plan.1"));
    project.addScheduleManager(child, sm);
    child->setRecalculate(true);
    child->setRecalculateFrom(restart);
    child->createSchedules();
    project.calculate(*child);

//     Debug::print(&project, task1, s, true);
//     Debug::print(&project, task2, s, true);
//     Debug::print(&project, task3, s, true);
//     Debug::printSchedulingLog(*child, s);

    QCOMPARE(task1->startTime(), c->firstAvailableAfter(targetstart, targetend));
    QCOMPARE(task1->endTime(), task1->startTime() + Duration(0, 8, 0));

    // either task2 or task3 may be scheduled first
    if (task2->startTime() < task3->startTime()) {
        QCOMPARE(task2->startTime(), c->firstAvailableAfter(qMax(task1->endTime(), restart), targetend));
        QCOMPARE(task2->endTime(), task2->startTime() + Duration(0, 8, 0));

        QCOMPARE(task3->startTime(), c->firstAvailableAfter(task2->endTime(), targetend));
        QCOMPARE(task3->endTime(), task3->startTime() + Duration(0, 8, 0));
    } else {
        QCOMPARE(task3->startTime(), c->firstAvailableAfter(qMax(task1->endTime(), restart), targetend));
        QCOMPARE(task3->endTime(), task3->startTime() + Duration(0, 8, 0));

        QCOMPARE(task2->startTime(), c->firstAvailableAfter(task3->endTime(), targetend));
        QCOMPARE(task2->endTime(), task2->startTime() + Duration(0, 8, 0));
    }
}

void ProjectTester::materialResource()
{
    Project project;
    project.setName(QStringLiteral("P1"));
    project.setId(project.uniqueNodeId());
    project.registerNodeId(&project);
    DateTime targetstart = DateTime(QDate::fromString(QStringLiteral("2012-02-01"), Qt::ISODate), QTime(0,0,0), project.timeZone());
    DateTime targetend = DateTime(targetstart.addDays(7));
    project.setConstraintStartTime(targetstart);
    project.setConstraintEndTime(targetend);

    Calendar *c = new Calendar(QStringLiteral("Test"));
    QTime t1(8,0,0);
    int length = 8*60*60*1000; // 8 hours

    for (int i = 1; i <= 7; ++i) {
        CalendarDay *wd1 = c->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(t1, length));
    }
    project.addCalendar(c);

    Task *task1 = project.createTask();
    task1->setName(QStringLiteral("T1"));
    project.addTask(task1, &project);
    task1->estimate()->setUnit(Duration::Unit_h);
    task1->estimate()->setExpectedEstimate(8.0);
    task1->estimate()->setType(Estimate::Type_Effort);

    QString s = QStringLiteral("Calculate forward, Task: ASAP, Working + material resource --------");
    qDebug()<<'\n'<<"Testing:"<<s;
    qDebug()<<s;
    ResourceGroup *g = new ResourceGroup();
    project.addResourceGroup(g);
    Resource *r = new Resource();
    r->setName(QStringLiteral("Work"));
    r->setAvailableFrom(targetstart);
    r->setCalendar(c);
    project.addResource(r);
    r->addParentGroup(g);

    ResourceGroup *mg = new ResourceGroup();
    mg->setType(QStringLiteral("Material"));
    project.addResourceGroup(mg);
    Resource *mr = new Resource();
    mr->setType(Resource::Type_Material);
    mr->setName(QStringLiteral("Material"));
    mr->setAvailableFrom(targetstart);
    mr->setCalendar(c);
    project.addResource(mr);
    mr->addParentGroup(mg);

    ResourceRequest *rr = new ResourceRequest(r, 100);
    task1->requests().addResourceRequest(rr);

    ResourceRequest *mrr = new ResourceRequest(mr, 100);
    task1->requests().addResourceRequest(mrr);

    ScheduleManager *sm = project.createScheduleManager(QStringLiteral("Test Plan"));
    project.addScheduleManager(sm);
    sm->createSchedules();
    project.calculate(*sm);

//     Debug::print(r, s);
//     Debug::print(mr, s);
//     Debug::print(&project, s, true);
//     Debug::printSchedulingLog(*sm, s);

    QCOMPARE(task1->earlyStart(), task1->requests().workTimeAfter(targetstart));
    QVERIFY(task1->lateStart() >=  task1->earlyStart());
    QVERIFY(task1->earlyFinish() <= task1->endTime());
    QVERIFY(task1->lateFinish() >= task1->endTime());

    QCOMPARE(task1->startTime(), DateTime(r->availableFrom().date(), t1, project.timeZone()));
    QCOMPARE(task1->endTime(), task1->startTime() + Duration(0, 8, 0));
    QVERIFY(task1->schedulingError() == false);
}

void ProjectTester::requiredResource()
{
    Project project;
    project.setName(QStringLiteral("P1"));
    project.setId(project.uniqueNodeId());
    project.registerNodeId(&project);
    DateTime targetstart = DateTime(QDate::fromString(QStringLiteral("2012-02-01"), Qt::ISODate), QTime(0,0,0), project.timeZone());
    DateTime targetend = DateTime(targetstart.addDays(7));
    project.setConstraintStartTime(targetstart);
    project.setConstraintEndTime(targetend);

    Calendar *c = new Calendar(QStringLiteral("Test"));
    QTime t1(8,0,0);
    int length = 8*60*60*1000; // 8 hours

    for (int i = 1; i <= 7; ++i) {
        CalendarDay *wd1 = c->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(t1, length));
    }
    project.addCalendar(c);

    Task *task1 = project.createTask();
    task1->setName(QStringLiteral("T1"));
    project.addTask(task1, &project);
    task1->estimate()->setUnit(Duration::Unit_h);
    task1->estimate()->setExpectedEstimate(8.0);
    task1->estimate()->setType(Estimate::Type_Effort);

    QString s = QStringLiteral("Required resource: Working + required material resource --------");
    qDebug()<<'\n'<<"Testing:"<<s;
    ResourceGroup *g = new ResourceGroup();
    project.addResourceGroup(g);
    Resource *r = new Resource();
    r->setName(QStringLiteral("Work"));
    r->setAvailableFrom(targetstart);
    r->setCalendar(c);
    project.addResource(r);
    r->addParentGroup(g);

    ResourceGroup *mg = new ResourceGroup();
    mg->setType(QStringLiteral("Material"));
    mg->setName(QStringLiteral("MG"));
    project.addResourceGroup(mg);
    Resource *mr = new Resource();
    mr->setType(Resource::Type_Material);
    mr->setName(QStringLiteral("Material"));
    mr->setAvailableFrom(targetstart);
    mr->setCalendar(c);
    project.addResource(mr);
    mr->addParentGroup(mg);

    ResourceRequest *rr = new ResourceRequest(r, 100);
    task1->requests().addResourceRequest(rr);

    QList<Resource*> lst; lst << mr;
    rr->setRequiredResources(lst);

    ScheduleManager *sm = project.createScheduleManager(QStringLiteral("Test Plan"));
    project.addScheduleManager(sm);
    sm->createSchedules();
    project.calculate(*sm);

//     Debug::print(r, s);
//     Debug::print(mr, s);
//     Debug::print(&project, task1, s);
//     Debug::printSchedulingLog(*sm, s);

    QCOMPARE(task1->earlyStart(), task1->requests().workTimeAfter(targetstart));
    QVERIFY(task1->lateStart() >=  task1->earlyStart());
    QVERIFY(task1->earlyFinish() <= task1->endTime());
    QVERIFY(task1->lateFinish() >= task1->endTime());

    QCOMPARE(task1->startTime(), DateTime(r->availableFrom().date(), t1, project.timeZone()));
    QCOMPARE(task1->endTime(), task1->startTime() + Duration(0, 8, 0));
    QVERIFY(task1->schedulingError() == false);

    QList<Appointment*> apps = r->appointments(sm->scheduleId());
    QVERIFY(apps.count() == 1);
    QCOMPARE(task1->startTime(), apps.first()->startTime());
    QCOMPARE(task1->endTime(), apps.last()->endTime());

    apps = mr->appointments(sm->scheduleId());
    QVERIFY(apps.count() == 1);
    QCOMPARE(task1->startTime(), apps.first()->startTime());
    QCOMPARE(task1->endTime(), apps.last()->endTime());

    s = QStringLiteral("Required resource limits availability --------");
    qDebug()<<'\n'<<"Testing:"<<s;
    DateTime tomorrow = targetstart.addDays(1);
    mr->setAvailableFrom(tomorrow);
    sm->createSchedules();
    project.calculate(*sm);

//     Debug::print(r, s);
//     Debug::print(mr, s);
//     Debug::print(&project, task1, s);
//     Debug::printSchedulingLog(*sm, s);

    QCOMPARE(task1->earlyStart(), task1->requests().workTimeAfter(targetstart));
    QVERIFY(task1->lateStart() >=  task1->earlyStart());
    QVERIFY(task1->earlyFinish() <= task1->endTime());
    QVERIFY(task1->lateFinish() >= task1->endTime());

    QCOMPARE(task1->startTime(), DateTime(mr->availableFrom().date(), t1, project.timeZone()));
    QCOMPARE(task1->endTime(), task1->startTime() + Duration(0, 8, 0));
    QVERIFY(task1->schedulingError() == false);

    apps = r->appointments(sm->scheduleId());
    QVERIFY(apps.count() == 1);
    QCOMPARE(task1->startTime(), apps.first()->startTime());
    QCOMPARE(task1->endTime(), apps.last()->endTime());

    apps = mr->appointments(sm->scheduleId());
    QVERIFY(apps.count() == 1);
    QCOMPARE(task1->startTime(), apps.first()->startTime());
    QCOMPARE(task1->endTime(), apps.last()->endTime());
}

void ProjectTester::resourceWithLimitedAvailability()
{
    Project project;
    project.setName(QStringLiteral("P1"));
    project.setId(project.uniqueNodeId());
    project.registerNodeId(&project);
    DateTime targetstart = DateTime(QDate(2010, 5, 1), QTime(0,0,0), project.timeZone());
    DateTime targetend = DateTime(targetstart.addDays(7));
    project.setConstraintStartTime(targetstart);
    project.setConstraintEndTime(targetend);

    DateTime expectedEndTime(QDate(2010, 5, 3), QTime(16, 0, 0), project.timeZone());

    Calendar *c = new Calendar(QStringLiteral("Test"));
    QTime t1(8,0,0);
    int length = 8*60*60*1000; // 8 hours

    for (int i = 1; i <= 7; ++i) {
        CalendarDay *wd1 = c->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(t1, length));
    }
    project.addCalendar(c);

    Task *task1 = project.createTask();
    task1->setName(QStringLiteral("T1"));
    project.addTask(task1, &project);
    task1->estimate()->setUnit(Duration::Unit_d);
    task1->estimate()->setExpectedEstimate(4.0);
    task1->estimate()->setType(Estimate::Type_Effort);

    QString s = QStringLiteral("Two resources: One with available until < resulting task length --------");
    qDebug()<<'\n'<<"Testing:"<<s;
    ResourceGroup *g = new ResourceGroup();
    project.addResourceGroup(g);
    Resource *r1 = new Resource();
    r1->setName(QStringLiteral("R1"));
    r1->setAvailableFrom(targetstart);
    r1->setCalendar(c);
    project.addResource(r1);
    r1->addParentGroup(g);

    Resource *r2 = new Resource();
    r2->setName(QStringLiteral("R2"));
    r2->setAvailableFrom(targetstart);
    r2->setAvailableUntil(targetstart.addDays(1));
    r2->setCalendar(c);
    project.addResource(r2);
    r2->addParentGroup(g);

    ResourceRequest *rr1 = new ResourceRequest(r1, 100);
    task1->requests().addResourceRequest(rr1);
    ResourceRequest *rr2 = new ResourceRequest(r2, 100);
    task1->requests().addResourceRequest(rr2);

    ScheduleManager *sm = project.createScheduleManager(QStringLiteral("Test Plan"));
    project.addScheduleManager(sm);
    sm->createSchedules();
    project.calculate(*sm);

//     Debug::print(r1, s);
//     Debug::print(r2, s);
//     Debug::print(&project, task1, s);
//     Debug::printSchedulingLog(*sm, s);

    QCOMPARE(task1->endTime(), expectedEndTime);
}

void ProjectTester::unavailableResource()
{
    Project project;
    project.setName(QStringLiteral("P1"));
    project.setId(project.uniqueNodeId());
    project.registerNodeId(&project);
    DateTime targetstart = DateTime(QDate(2010, 5, 1), QTime(0,0,0), project.timeZone());
    DateTime targetend = DateTime(targetstart.addDays(7));
    project.setConstraintStartTime(targetstart);
    project.setConstraintEndTime(targetend);

    Calendar *c = new Calendar(QStringLiteral("Test"));
    QTime t1(8,0,0);
    int length = 8*60*60*1000; // 8 hours

    for (int i = 1; i <= 7; ++i) {
        CalendarDay *wd1 = c->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(t1, length));
    }
    project.addCalendar(c);

    Task *task1 = project.createTask();
    task1->setName(QStringLiteral("T1"));
    project.addTask(task1, &project);
    task1->estimate()->setUnit(Duration::Unit_d);
    task1->estimate()->setExpectedEstimate(2.0);
    task1->estimate()->setType(Estimate::Type_Effort);

    QString s = QStringLiteral("One available resource --------");
    qDebug()<<'\n'<<"Testing:"<<s;
    ResourceGroup *g = new ResourceGroup();
    project.addResourceGroup(g);
    Resource *r1 = new Resource();
    r1->setName(QStringLiteral("R1"));
    r1->setCalendar(c);
    project.addResource(r1);
    r1->addParentGroup(g);

    Resource *r2 = new Resource();
    r2->setName(QStringLiteral("Unavailable"));
    r2->setCalendar(c);
    project.addResource(r2);
    r2->addParentGroup(g);

    task1->requests().addResourceRequest(new ResourceRequest(r1, 100));

    ScheduleManager *sm = project.createScheduleManager(QStringLiteral("Plan R1"));
    project.addScheduleManager(sm);
    sm->createSchedules();
    project.calculate(*sm);

//     Debug::print(r1, s);
//     Debug::print(r2, s);
     Debug::print(&project, task1, s);
//     Debug::printSchedulingLog(*sm, s);

    DateTime expectedEndTime = targetstart + Duration(1, 16, 0);

    QCOMPARE(task1->endTime(), expectedEndTime);

    s = QStringLiteral("One available resource + one unavailable resource --------");
    qDebug()<<'\n'<<"Testing:"<<s;

    r2->setAvailableFrom(targetend);
    r2->setAvailableUntil(targetend.addDays(1));
    task1->requests().addResourceRequest(new ResourceRequest(r2, 100));

    sm = project.createScheduleManager(QStringLiteral("Team + Resource"));
    project.addScheduleManager(sm);
    sm->createSchedules();
    project.calculate(*sm);

//     Debug::print(r1, s);
//     Debug::print(r2, s);
     Debug::print(&project, task1, s);
//     Debug::printSchedulingLog(*sm, s);
    QCOMPARE(task1->endTime(), expectedEndTime);

}


void ProjectTester::team()
{
    Project project;
    project.setName(QStringLiteral("P1"));
    project.setId(project.uniqueNodeId());
    project.registerNodeId(&project);
    DateTime targetstart = DateTime(QDate(2010, 5, 1), QTime(0,0,0), project.timeZone());
    DateTime targetend = DateTime(targetstart.addDays(7));
    project.setConstraintStartTime(targetstart);
    project.setConstraintEndTime(targetend);

    Calendar *c = new Calendar(QStringLiteral("Test"));
    QTime t1(8,0,0);
    int length = 8*60*60*1000; // 8 hours

    for (int i = 1; i <= 7; ++i) {
        CalendarDay *wd1 = c->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(t1, length));
    }
    project.addCalendar(c);

    Task *task1 = project.createTask();
    task1->setName(QStringLiteral("T1"));
    project.addTask(task1, &project);
    task1->estimate()->setUnit(Duration::Unit_d);
    task1->estimate()->setExpectedEstimate(2.0);
    task1->estimate()->setType(Estimate::Type_Effort);

    QString s = QStringLiteral("One team with one resource --------");
    qDebug()<<'\n'<<"Testing:"<<s;
    ResourceGroup *g = new ResourceGroup();
    project.addResourceGroup(g);
    Resource *r1 = new Resource();
    r1->setName(QStringLiteral("R1"));
    r1->setCalendar(c);
    project.addResource(r1);
    r1->addParentGroup(g);

    Resource *r2 = new Resource();
    r2->setName(QStringLiteral("Team member"));
    r2->setCalendar(c);
    project.addResource(r2);
    r2->addParentGroup(g);

    Resource *team = new Resource();
    team->setType(Resource::Type_Team);
    team->setName(QStringLiteral("Team"));
    project.addResource(team);
    team->addParentGroup(g);
    team->addTeamMemberId(r2->id());

    ResourceRequest *tr = new ResourceRequest(team, 100);
    task1->requests().addResourceRequest(tr);

    ScheduleManager *sm = project.createScheduleManager(QStringLiteral("Team"));
    project.addScheduleManager(sm);
    sm->createSchedules();
    project.calculate(*sm);
    
//     Debug::print(r1, s);
//     Debug::print(r2, s);
    Debug::print(team, s, false);
    Debug::print(&project, task1, s);
//     Debug::printSchedulingLog(*sm, s);

    DateTime expectedEndTime = targetstart + Duration(1, 16, 0);
    QCOMPARE(task1->endTime(), expectedEndTime);

    s = QStringLiteral("One team with one resource + one resource --------");
    qDebug()<<'\n'<<"Testing:"<<s;

    ResourceRequest *rr1 = new ResourceRequest(r1, 100);
    task1->requests().addResourceRequest(rr1);

    sm = project.createScheduleManager(QStringLiteral("Team + Resource"));
    project.addScheduleManager(sm);
    sm->createSchedules();
    project.calculate(*sm);

//     Debug::print(r1, s);
//     Debug::print(r2, s);
     Debug::print(team, s, false);
     Debug::print(&project, task1, s);
//     Debug::printSchedulingLog(*sm, s);
    expectedEndTime = targetstart + Duration(0, 16, 0);
    QCOMPARE(task1->endTime(), expectedEndTime);

    s = QStringLiteral("One team with one resource + one resource, resource available too late --------");
    qDebug()<<'\n'<<"Testing:"<<s;
    
    r1->setAvailableFrom(targetend);
    r1->setAvailableUntil(targetend.addDays(7));

    sm = project.createScheduleManager(QStringLiteral("Team + Resource not available"));
    project.addScheduleManager(sm);
    sm->createSchedules();
    project.calculate(*sm);

    Debug::print(r1, s);
//    Debug::print(r2, s);
    Debug::print(team, s, false);
    Debug::print(&project, s, true);
//     Debug::printSchedulingLog(*sm, s);
    expectedEndTime = targetstart + Duration(1, 16, 0);
    QCOMPARE(task1->endTime(), expectedEndTime);

    s = QStringLiteral("One team with two resources --------");
    qDebug()<<'\n'<<"Testing:"<<s;
    
    r1->removeRequests();
    team->addTeamMemberId(r1->id());
    r1->setAvailableFrom(targetstart);
    r1->setAvailableUntil(targetend);

    sm = project.createScheduleManager(QStringLiteral("Team with 2 resources"));
    project.addScheduleManager(sm);
    sm->createSchedules();
    project.calculate(*sm);

//    Debug::print(r1, s);
//    Debug::print(r2, s);
    Debug::print(team, s, false);
    Debug::print(&project, task1, s);
//     Debug::printSchedulingLog(*sm, s);
    expectedEndTime = targetstart + Duration(0, 16, 0);
    QCOMPARE(task1->endTime(), expectedEndTime);

    s = QStringLiteral("One team with two resources, one resource unavailable --------");
    qDebug()<<'\n'<<"Testing:"<<s;
    
    r1->setAvailableFrom(targetend);
    r1->setAvailableUntil(targetend.addDays(2));

    sm = project.createScheduleManager(QStringLiteral("Team, one unavailable resource"));
    project.addScheduleManager(sm);
    sm->createSchedules();
    project.calculate(*sm);

    Debug::print(r1, s);
//    Debug::print(r2, s);
    Debug::print(team, s, false);
    Debug::print(&project, task1, s);
//     Debug::printSchedulingLog(*sm, s);
    expectedEndTime = targetstart + Duration(1, 16, 0);
    QCOMPARE(task1->endTime(), expectedEndTime);

    task1->requests().removeResourceRequest(tr);
    project.takeResource(team);
    team->removeTeamMemberId(r2->id());
}

void ProjectTester::inWBSOrder()
{
    Project p;
    p.setName(QStringLiteral("WBS Order"));
    p.setId(p.uniqueNodeId());
    p.registerNodeId(&p);
    DateTime st = QDateTime::fromString(QStringLiteral("2012-02-01"), Qt::ISODate);
    st.setTimeZone(p.timeZone());
    st = DateTime(st.addDays(1));
    st.setTime(QTime (0, 0, 0));
    p.setConstraintStartTime(st);
    p.setConstraintEndTime(st.addDays(5));

    Calendar *c = new Calendar(QStringLiteral("Test"));
    QTime t1(8,0,0);
    int length = 8*60*60*1000; // 8 hours

    for (int i = 1; i <= 7; ++i) {
        CalendarDay *wd1 = c->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(t1, length));
    }
    p.addCalendar(c);
    p.setDefaultCalendar(c);
    
    ResourceGroup *g = new ResourceGroup();
    p.addResourceGroup(g);
    Resource *r1 = new Resource();
    r1->setName(QStringLiteral("R1"));
    p.addResource(r1);
    r1->addParentGroup(g);

    Task *t = p.createTask();
    t->setName(QStringLiteral("T1"));
    p.addSubTask(t, &p);
    t->estimate()->setUnit(Duration::Unit_d);
    t->estimate()->setExpectedEstimate(1.0);
    t->estimate()->setType(Estimate::Type_Effort);

    ResourceRequest *tr = new ResourceRequest(r1, 100);
    t->requests().addResourceRequest(tr);

    t = p.createTask();
    t->setName(QStringLiteral("T2"));
    p.addSubTask(t, &p);
    t->estimate()->setUnit(Duration::Unit_d);
    t->estimate()->setExpectedEstimate(1.0);
    t->estimate()->setType(Estimate::Type_Effort);

    tr = new ResourceRequest(r1, 100);
    t->requests().addResourceRequest(tr);

    t = p.createTask();
    t->setName(QStringLiteral("T3"));
    p.addSubTask(t, &p);
    t->estimate()->setUnit(Duration::Unit_d);
    t->estimate()->setExpectedEstimate(1.0);
    t->estimate()->setType(Estimate::Type_Effort);

    tr = new ResourceRequest(r1, 100);
    t->requests().addResourceRequest(tr);

    t = p.createTask();
    t->setName(QStringLiteral("T4"));
    p.addSubTask(t, &p);
    t->estimate()->setUnit(Duration::Unit_d);
    t->estimate()->setExpectedEstimate(1.0);
    t->estimate()->setType(Estimate::Type_Effort);

    tr = new ResourceRequest(r1, 100);
    t->requests().addResourceRequest(tr);
    
    ScheduleManager *sm = p.createScheduleManager(QStringLiteral("WBS Order, forward"));
    p.addScheduleManager(sm);
    sm->createSchedules();
    p.calculate(*sm);
    
//     QString s = QStringLiteral("Schedule 4 tasks forward in wbs order -------");
    // NOTE: It's not *mandatory* to schedule in wbs order but users expect it, so we'll try
    //       This test can be removed if for some important reason this isn't possible.

//     Debug::print (c, s);
//     Debug::print(r1, s);
//     Debug::print(&p, s, true);
//     Debug::printSchedulingLog(*sm, s);
    
    QCOMPARE(p.allTasks().count(), 4);
    QCOMPARE(p.allTasks().at(0)->startTime(), st + Duration(0, 8, 0));
    QCOMPARE(p.allTasks().at(1)->startTime(), st + Duration(1, 8, 0));
    QCOMPARE(p.allTasks().at(2)->startTime(), st + Duration(2, 8, 0));
    QCOMPARE(p.allTasks().at(3)->startTime(), st + Duration(3, 8, 0));
}

void ProjectTester::resourceConflictALAP()
{
    Project p;
    p.setName(QStringLiteral("resourceConflictALAP"));
    p.setId(p.uniqueNodeId());
    p.registerNodeId(&p);
    DateTime st = QDateTime::fromString(QStringLiteral("2012-02-01"), Qt::ISODate);
    st.setTimeZone(p.timeZone());
    st = DateTime(st.addDays(1));
    st.setTime(QTime (0, 0, 0));
    p.setConstraintStartTime(st);
    p.setConstraintEndTime(st.addDays(5));

    Calendar *c = new Calendar(QStringLiteral("Test"));
    QTime t1(8,0,0);
    int length = 8*60*60*1000; // 8 hours

    for (int i = 1; i <= 7; ++i) {
        CalendarDay *wd1 = c->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(t1, length));
    }
    p.addCalendar(c);
    p.setDefaultCalendar(c);
    
    ResourceGroup *g = new ResourceGroup();
    p.addResourceGroup(g);
    Resource *r1 = new Resource();
    r1->setName(QStringLiteral("R1"));
    p.addResource(r1);
    r1->addParentGroup(g);

    Task *t = p.createTask();
    t->setName(QStringLiteral("T1"));
    t->setConstraint(Node::ALAP);
    p.addSubTask(t, &p);
    t->estimate()->setUnit(Duration::Unit_d);
    t->estimate()->setExpectedEstimate(1.0);
    t->estimate()->setType(Estimate::Type_Effort);

    ResourceRequest *tr = new ResourceRequest(r1, 100);
    t->requests().addResourceRequest(tr);

    t = p.createTask();
    t->setName(QStringLiteral("T2"));
    p.addSubTask(t, &p);
    t->estimate()->setUnit(Duration::Unit_d);
    t->estimate()->setExpectedEstimate(1.0);
    t->estimate()->setType(Estimate::Type_Effort);

    tr = new ResourceRequest(r1, 100);
    t->requests().addResourceRequest(tr);

    t = p.createTask();
    t->setName(QStringLiteral("T3"));
    p.addSubTask(t, &p);
    t->estimate()->setUnit(Duration::Unit_d);
    t->estimate()->setExpectedEstimate(1.0);
    t->estimate()->setType(Estimate::Type_Effort);

    tr = new ResourceRequest(r1, 100);
    t->requests().addResourceRequest(tr);

    t = p.createTask();
    t->setName(QStringLiteral("T4"));
    p.addSubTask(t, &p);
    t->estimate()->setUnit(Duration::Unit_d);
    t->estimate()->setExpectedEstimate(1.0);
    t->estimate()->setType(Estimate::Type_Effort);

    tr = new ResourceRequest(r1, 100);
    t->requests().addResourceRequest(tr);
    
    ScheduleManager *sm = p.createScheduleManager(QStringLiteral("T1 ALAP"));
    p.addScheduleManager(sm);
    sm->createSchedules();
    p.calculate(*sm);
    
    QString s = QStringLiteral("Schedule T1 ALAP -------");

//     Debug::print (c, s);
//     Debug::print(r1, s);
//     Debug::print(&p, s, true);
//     Debug::printSchedulingLog(*sm, s);
    
    QCOMPARE(p.allTasks().count(), 4);
    QCOMPARE(p.allTasks().at(0)->startTime(), st + Duration(3, 8, 0));
    QCOMPARE(p.allTasks().at(0)->endTime(), st + Duration(3, 8, 0) + Duration(0, 8, 0));
    QCOMPARE(p.allTasks().at(1)->startTime(), st + Duration(0, 8, 0));
    QCOMPARE(p.allTasks().at(1)->endTime(), st + Duration(0, 8, 0) + Duration(0, 8, 0));
    QCOMPARE(p.allTasks().at(2)->startTime(), st + Duration(1, 8, 0));
    QCOMPARE(p.allTasks().at(2)->endTime(), st + Duration(1, 8, 0) + Duration(0, 8, 0));
    QCOMPARE(p.allTasks().at(3)->startTime(), st + Duration(2, 8, 0));
    QCOMPARE(p.allTasks().at(3)->endTime(), st + Duration(2, 8, 0) + Duration(0, 8, 0));

    s = QStringLiteral("Schedule T1, T2 ALAP -------");
    p.allTasks().at(1)->setConstraint(Node::ALAP);
    
    sm = p.createScheduleManager(QStringLiteral("T1, T2 ALAP"));
    p.addScheduleManager(sm);
    sm->createSchedules();
    p.calculate(*sm);
    

//     Debug::print (c, s);
//     Debug::print(r1, s);
//     Debug::print(&p, s, true);
//     Debug::printSchedulingLog(*sm, s);
    
    QCOMPARE(p.allTasks().count(), 4);
    QCOMPARE(p.allTasks().at(0)->startTime(), st + Duration(3, 8, 0));
    QCOMPARE(p.allTasks().at(0)->endTime(), st + Duration(3, 8, 0) + Duration(0, 8, 0));
    QCOMPARE(p.allTasks().at(1)->startTime(), st + Duration(2, 8, 0));
    QCOMPARE(p.allTasks().at(1)->endTime(), st + Duration(2, 8, 0) + Duration(0, 8, 0));
    QCOMPARE(p.allTasks().at(2)->startTime(), st + Duration(0, 8, 0));
    QCOMPARE(p.allTasks().at(2)->endTime(), st + Duration(0, 8, 0) + Duration(0, 8, 0));
    QCOMPARE(p.allTasks().at(3)->startTime(), st + Duration(1, 8, 0));
    QCOMPARE(p.allTasks().at(3)->endTime(), st + Duration(1, 8, 0) + Duration(0, 8, 0));

    s = QStringLiteral("Schedule T1, T2, T3 ALAP -------");
    p.allTasks().at(2)->setConstraint(Node::ALAP);
    
    sm = p.createScheduleManager(QStringLiteral("T1, T2, T3 ALAP"));
    p.addScheduleManager(sm);
    sm->createSchedules();
    p.calculate(*sm);
    

//     Debug::print (c, s);
//     Debug::print(r1, s);
//     Debug::print(&p, s, true);
//     Debug::printSchedulingLog(*sm, s);
    
    QCOMPARE(p.allTasks().count(), 4);
    QCOMPARE(p.allTasks().at(0)->startTime(), st + Duration(3, 8, 0));
    QCOMPARE(p.allTasks().at(0)->endTime(), st + Duration(3, 8, 0) + Duration(0, 8, 0));
    QCOMPARE(p.allTasks().at(1)->startTime(), st + Duration(2, 8, 0));
    QCOMPARE(p.allTasks().at(1)->endTime(), st + Duration(2, 8, 0) + Duration(0, 8, 0));
    QCOMPARE(p.allTasks().at(2)->startTime(), st + Duration(1, 8, 0));
    QCOMPARE(p.allTasks().at(2)->endTime(), st + Duration(1, 8, 0) + Duration(0, 8, 0));
    QCOMPARE(p.allTasks().at(3)->startTime(), st + Duration(0, 8, 0));
    QCOMPARE(p.allTasks().at(3)->endTime(), st + Duration(0, 8, 0) + Duration(0, 8, 0));

    s = QStringLiteral("Schedule T1, T2, T3, T4 ALAP -------");
    p.allTasks().at(3)->setConstraint(Node::ALAP);
    
    sm = p.createScheduleManager(QStringLiteral("T1, T2, T3, T4 ALAP"));
    p.addScheduleManager(sm);
    sm->createSchedules();
    p.calculate(*sm);
    

//     Debug::print (c, s);
//     Debug::print(r1, s);
//     Debug::print(&p, s, true);
//     Debug::printSchedulingLog(*sm, s);
    
    QCOMPARE(p.allTasks().count(), 4);
    QCOMPARE(p.allTasks().at(0)->startTime(), st + Duration(3, 8, 0));
    QCOMPARE(p.allTasks().at(0)->endTime(), st + Duration(3, 8, 0) + Duration(0, 8, 0));
    QCOMPARE(p.allTasks().at(1)->startTime(), st + Duration(2, 8, 0));
    QCOMPARE(p.allTasks().at(1)->endTime(), st + Duration(2, 8, 0) + Duration(0, 8, 0));
    QCOMPARE(p.allTasks().at(2)->startTime(), st + Duration(1, 8, 0));
    QCOMPARE(p.allTasks().at(2)->endTime(), st + Duration(1, 8, 0) + Duration(0, 8, 0));
    QCOMPARE(p.allTasks().at(3)->startTime(), st + Duration(0, 8, 0));
    QCOMPARE(p.allTasks().at(3)->endTime(), st + Duration(0, 8, 0) + Duration(0, 8, 0));
}

void ProjectTester::resourceConflictMustStartOn()
{
    Project p;
    p.setName(QStringLiteral("resourceConflictMustStartOn"));
    p.setId(p.uniqueNodeId());
    p.registerNodeId(&p);
    DateTime st = QDateTime::fromString(QStringLiteral("2012-02-01T00:00:00"), Qt::ISODate);
    st.setTimeZone(p.timeZone());
    st = DateTime(st.addDays(1));
    st.setTime(QTime (0, 0, 0));
    p.setConstraintStartTime(st);
    p.setConstraintEndTime(st.addDays(5));

    Calendar *c = new Calendar(QStringLiteral("Test"));
    QTime t1(8,0,0);
    int length = 8*60*60*1000; // 8 hours

    for (int i = 1; i <= 7; ++i) {
        CalendarDay *wd1 = c->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(t1, length));
    }
    p.addCalendar(c);
    p.setDefaultCalendar(c);
    
    ResourceGroup *g = new ResourceGroup();
    p.addResourceGroup(g);
    Resource *r1 = new Resource();
    r1->setName(QStringLiteral("R1"));
    p.addResource(r1);
    r1->addParentGroup(g);

    Task *t = p.createTask();
    t->setName(QStringLiteral("T1"));
    t->setConstraint(Node::MustStartOn);
    t->setConstraintStartTime(st + Duration(1, 8, 0));
    p.addSubTask(t, &p);
    t->estimate()->setUnit(Duration::Unit_d);
    t->estimate()->setExpectedEstimate(1.0);
    t->estimate()->setType(Estimate::Type_Effort);

    ResourceRequest *tr = new ResourceRequest(r1, 100);
    t->requests().addResourceRequest(tr);

    t = p.createTask();
    t->setName(QStringLiteral("T2"));
    p.addSubTask(t, &p);
    t->estimate()->setUnit(Duration::Unit_d);
    t->estimate()->setExpectedEstimate(1.0);
    t->estimate()->setType(Estimate::Type_Effort);

    tr = new ResourceRequest(r1, 100);
    t->requests().addResourceRequest(tr);

    t = p.createTask();
    t->setName(QStringLiteral("T3"));
    p.addSubTask(t, &p);
    t->estimate()->setUnit(Duration::Unit_d);
    t->estimate()->setExpectedEstimate(1.0);
    t->estimate()->setType(Estimate::Type_Effort);

    tr = new ResourceRequest(r1, 100);
    t->requests().addResourceRequest(tr);

    t = p.createTask();
    t->setName(QStringLiteral("T4"));
    p.addSubTask(t, &p);
    t->estimate()->setUnit(Duration::Unit_d);
    t->estimate()->setExpectedEstimate(1.0);
    t->estimate()->setType(Estimate::Type_Effort);

    tr = new ResourceRequest(r1, 100);
    t->requests().addResourceRequest(tr);
    
    ScheduleManager *sm = p.createScheduleManager(QStringLiteral("T1 MustStartOn"));
    p.addScheduleManager(sm);
    sm->createSchedules();
    p.calculate(*sm);
    
    QString s = QStringLiteral("Schedule T1 MustStartOn -------");

//     Debug::print (c, s);
//     Debug::print(r1, s);
     Debug::print(&p, s, true);
//     Debug::printSchedulingLog(*sm, s);
    
    QCOMPARE(p.allTasks().count(), 4);
    QCOMPARE(p.allTasks().at(0)->startTime(), st + Duration(1, 8, 0));
    QCOMPARE(p.allTasks().at(0)->endTime(), st + Duration(1, 8, 0) + Duration(0, 8, 0));
    QCOMPARE(p.allTasks().at(1)->startTime(), st + Duration(0, 8, 0));
    QCOMPARE(p.allTasks().at(1)->endTime(), st + Duration(0, 8, 0) + Duration(0, 8, 0));
    QCOMPARE(p.allTasks().at(2)->startTime(), st + Duration(2, 8, 0));
    QCOMPARE(p.allTasks().at(2)->endTime(), st + Duration(2, 8, 0) + Duration(0, 8, 0));
    QCOMPARE(p.allTasks().at(3)->startTime(), st + Duration(3, 8, 0));
    QCOMPARE(p.allTasks().at(3)->endTime(), st + Duration(3, 8, 0) + Duration(0, 8, 0));

    s = QStringLiteral("Schedule T1, T2 MustStartOn -------");
    p.allTasks().at(1)->setConstraint(Node::MustStartOn);
    p.allTasks().at(1)->setConstraintStartTime(st + Duration(2, 8, 0));
    
    sm = p.createScheduleManager(QStringLiteral("T1, T2 MustStartOn"));
    p.addScheduleManager(sm);
    sm->createSchedules();
    p.calculate(*sm);
    

//     Debug::print (c, s);
//     Debug::print(r1, s);
     Debug::print(&p, s, true);
//     Debug::printSchedulingLog(*sm, s);
    
    QCOMPARE(p.allTasks().count(), 4);
    QCOMPARE(p.allTasks().at(0)->startTime(), st + Duration(1, 8, 0));
    QCOMPARE(p.allTasks().at(0)->endTime(), st + Duration(1, 8, 0) + Duration(0, 8, 0));
    QCOMPARE(p.allTasks().at(1)->startTime(), st + Duration(2, 8, 0));
    QCOMPARE(p.allTasks().at(1)->endTime(), st + Duration(2, 8, 0) + Duration(0, 8, 0));
    QCOMPARE(p.allTasks().at(2)->startTime(), st + Duration(0, 8, 0));
    QCOMPARE(p.allTasks().at(2)->endTime(), st + Duration(0, 8, 0) + Duration(0, 8, 0));
    QCOMPARE(p.allTasks().at(3)->startTime(), st + Duration(3, 8, 0));
    QCOMPARE(p.allTasks().at(3)->endTime(), st + Duration(3, 8, 0) + Duration(0, 8, 0));

    s = QStringLiteral("Schedule T1, T2, T3 MustStartOn -------");
    p.allTasks().at(2)->setConstraint(Node::MustStartOn);
    p.allTasks().at(2)->setConstraintStartTime(st + Duration(3, 8, 0));
    
    sm = p.createScheduleManager(QStringLiteral("T1, T2, T3 MustStartOn"));
    p.addScheduleManager(sm);
    sm->createSchedules();
    p.calculate(*sm);
    

//     Debug::print (c, s);
//     Debug::print(r1, s);
//     Debug::print(&p, s, true);
//     Debug::printSchedulingLog(*sm, s);
    
    QCOMPARE(p.allTasks().count(), 4);
    QCOMPARE(p.allTasks().at(0)->startTime(), st + Duration(1, 8, 0));
    QCOMPARE(p.allTasks().at(0)->endTime(), st + Duration(1, 8, 0) + Duration(0, 8, 0));
    QCOMPARE(p.allTasks().at(1)->startTime(), st + Duration(2, 8, 0));
    QCOMPARE(p.allTasks().at(1)->endTime(), st + Duration(2, 8, 0) + Duration(0, 8, 0));
    QCOMPARE(p.allTasks().at(2)->startTime(), st + Duration(3, 8, 0));
    QCOMPARE(p.allTasks().at(2)->endTime(), st + Duration(3, 8, 0) + Duration(0, 8, 0));
    QCOMPARE(p.allTasks().at(3)->startTime(), st + Duration(0, 8, 0));
    QCOMPARE(p.allTasks().at(3)->endTime(), st + Duration(0, 8, 0) + Duration(0, 8, 0));


    s = QStringLiteral("Schedule backwards, T1 MustStartOn, T2 ASAP -------");

    p.takeTask(p.allTasks().at(3), false);
    p.takeTask(p.allTasks().at(2), false);

    Task *task1 = p.allTasks().at(0);
    Task *task2 = p.allTasks().at(1);
    DateTime et = p.constraintEndTime();
    task1->setConstraint(Node::MustStartOn);
    task1->setConstraintStartTime(et - Duration(1, 16, 0));
    task2->setConstraint(Node::ASAP);

    sm = p.createScheduleManager(QStringLiteral("T1 MustStartOn, T2 ASAP"));
    p.addScheduleManager(sm);
    sm->createSchedules();
    sm->setSchedulingDirection(true);
    p.calculate(*sm);

    Debug::print(&p, s, true);
//     Debug::printSchedulingLog(*sm, s);

    QCOMPARE(task1->startTime(), task1->mustStartOn());
    QCOMPARE(task2->startTime(), et - Duration(0, 16, 0));

    s = QStringLiteral("Schedule backwards, T1 MustStartOn, T2 StartNotEarlier -------");

    task2->setConstraint(Node::StartNotEarlier);
    task2->setConstraintStartTime(task1->mustStartOn().addDays(-1));

    sm = p.createScheduleManager(QStringLiteral("T1 MustStartOn, T2 StartNotEarlier"));
    p.addScheduleManager(sm);
    sm->createSchedules();
    sm->setSchedulingDirection(true);
    p.calculate(*sm);

    Debug::print(&p, s, true);
//     Debug::printSchedulingLog(*sm, s);

    QCOMPARE(task1->startTime(), task1->mustStartOn());
    QCOMPARE(task2->startTime(), et - Duration(0, 16, 0));

    task2->setConstraintStartTime(task1->mustStartOn());
    sm->createSchedules();
    p.calculate(*sm);

    Debug::print(&p, s, true);
//     Debug::printSchedulingLog(*sm, s);

    QCOMPARE(task1->startTime(), task1->mustStartOn());
    QCOMPARE(task2->startTime(), et - Duration(0, 16, 0));

    task2->setConstraintStartTime(task1->mustStartOn().addDays(1));
    sm->createSchedules();
    p.calculate(*sm);

    Debug::print(&p, s, true);
//     Debug::printSchedulingLog(*sm, s);

    QCOMPARE(task1->startTime(), task1->mustStartOn());
    QCOMPARE(task2->startTime(), et - Duration(0, 16, 0));

}

void ProjectTester::resourceConflictMustFinishOn()
{
    Project p;
    p.setName(QStringLiteral("P1"));
    p.setId(p.uniqueNodeId());
    p.registerNodeId(&p);
    DateTime st = QDateTime::fromString(QStringLiteral("2012-02-01"), Qt::ISODate);
    st.setTimeZone(p.timeZone());
    st = DateTime(st.addDays(1));
    st.setTime(QTime (0, 0, 0));
    p.setConstraintStartTime(st);
    p.setConstraintEndTime(st.addDays(5));

    Calendar *c = new Calendar(QStringLiteral("Test"));
    QTime t1(8,0,0);
    int length = 8*60*60*1000; // 8 hours

    for (int i = 1; i <= 7; ++i) {
        CalendarDay *wd1 = c->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(t1, length));
    }
    p.addCalendar(c);
    p.setDefaultCalendar(c);
    
    ResourceGroup *g = new ResourceGroup();
    p.addResourceGroup(g);
    Resource *r1 = new Resource();
    r1->setName(QStringLiteral("R1"));
    p.addResource(r1);
    r1->addParentGroup(g);

    Task *task1 = p.createTask();
    task1->setName(QStringLiteral("T1"));
    task1->setConstraint(Node::MustFinishOn);
    task1->setConstraintEndTime(st + Duration(1, 16, 0));
    p.addSubTask(task1, &p);
    task1->estimate()->setUnit(Duration::Unit_d);
    task1->estimate()->setExpectedEstimate(1.0);
    task1->estimate()->setType(Estimate::Type_Effort);

    ResourceRequest *tr = new ResourceRequest(r1, 100);
    task1->requests().addResourceRequest(tr);

    Task *task2 = p.createTask();
    task2->setName(QStringLiteral("T2"));
    p.addSubTask(task2, &p);
    task2->estimate()->setUnit(Duration::Unit_d);
    task2->estimate()->setExpectedEstimate(1.0);
    task2->estimate()->setType(Estimate::Type_Effort);

    tr = new ResourceRequest(r1, 100);
    task2->requests().addResourceRequest(tr);

    QString s = QStringLiteral("Schedule T1 MustFinishOn, T2 ASAP -------");
    qDebug()<<s;

    ScheduleManager *sm = p.createScheduleManager(QStringLiteral("T1 MustFinishOn"));
    p.addScheduleManager(sm);
    sm->createSchedules();
    p.calculate(*sm);

    Debug::print(&p, s, true);

    QCOMPARE(task1->endTime(), task1->mustFinishOn());
    QCOMPARE(task2->startTime(), p.constraintStartTime() + Duration(0, 8, 0));

    s = QStringLiteral("Schedule T1 MustFinishOn, T2 ALAP -------");
    qDebug()<<s;

    task2->setConstraint(Node::ALAP);

    sm->createSchedules();
    p.calculate(*sm);

    Debug::print(&p, s, true);

    QCOMPARE(task1->endTime(), task1->mustFinishOn());
    QCOMPARE(task2->startTime(), p.constraintStartTime() + Duration(0, 8, 0));

    s = QStringLiteral("Schedule T1 MustFinishOn, T2 StartNotEarlier -------");
    qDebug()<<s;

    task2->setConstraint(Node::StartNotEarlier);
    task2->setConstraintStartTime(p.constraintStartTime());

    sm->createSchedules();
    p.calculate(*sm);

    Debug::print(&p, s, true);

    QCOMPARE(task1->endTime(), task1->mustFinishOn());
    QCOMPARE(task2->startTime(), p.constraintStartTime() + Duration(0, 8, 0));

    s = QStringLiteral("Schedule T1 MustFinishOn, T2 StartNotEarlier -------");
    qDebug()<<s;

    task2->setConstraintStartTime(task1->mustFinishOn() - Duration(0, 8, 0));

    sm->createSchedules();
    p.calculate(*sm);

    Debug::print(&p, s, true);

    QCOMPARE(task1->endTime(), task1->mustFinishOn());
    QCOMPARE(task2->startTime(), task2->constraintStartTime() + Duration(1, 0, 0));

    s = QStringLiteral("Schedule T1 MustFinishOn, T2 StartNotEarlier -------");
    qDebug()<<s;

    task2->setConstraintStartTime(task1->mustFinishOn());

    sm->createSchedules();
    p.calculate(*sm);

    Debug::print(&p, s, true);

    QCOMPARE(task1->endTime(), task1->mustFinishOn());
    QCOMPARE(task2->startTime(), task2->constraintStartTime() + Duration(0, 16, 0));

    s = QStringLiteral("Schedule backwards, T1 MustFinishOn, T2 ASAP -------");
    qDebug()<<s;

    task2->setConstraint(Node::ASAP);

    sm->createSchedules();
    sm->setSchedulingDirection(true);
    p.calculate(*sm);

    Debug::print(&p, s, true);

    QCOMPARE(task1->endTime(), task1->mustFinishOn());
    QCOMPARE(task2->startTime(), task1->mustFinishOn() + Duration(0, 16, 0));

    s = QStringLiteral("Schedule backwards, T1 MustFinishOn, T2 ALAP -------");
    qDebug()<<s;

    DateTime et = p.constraintEndTime();

    task2->setConstraint(Node::ALAP);

    sm->createSchedules();
    p.calculate(*sm);

    Debug::print(&p, s, true);
//    Debug::printSchedulingLog(*sm, s);

    QCOMPARE(task1->endTime(), task1->mustFinishOn());
    QCOMPARE(task2->endTime(), et - Duration(0, 8, 0));

    s = QStringLiteral("Schedule backwards, T1 MustFinishOn, T2 StartNotEarlier -------");
    qDebug()<<s;

    task2->setConstraint(Node::StartNotEarlier);
    task2->setConstraintStartTime(task1->mustFinishOn());

    sm->createSchedules();
    p.calculate(*sm);

    Debug::print(&p, s, true);
//    Debug::printSchedulingLog(*sm, s);

    QCOMPARE(task1->endTime(), task1->mustFinishOn());
    QCOMPARE(task2->endTime(), et - Duration(0, 8, 0));

    task2->setConstraintStartTime(p.constraintStartTime());

    sm->createSchedules();
    p.calculate(*sm);

    Debug::print(&p, s, true);
//    Debug::printSchedulingLog(*sm, s);

    QCOMPARE(task1->endTime(), task1->mustFinishOn());
    QCOMPARE(task2->endTime(), et - Duration(0, 8, 0));

    s = QStringLiteral("Schedule backwards, T1 MustFinishOn, T2 FinishNotLater -------");
    qDebug()<<s;

    task2->setConstraint(Node::FinishNotLater);
    task2->setConstraintEndTime(task1->mustFinishOn());

    sm->createSchedules();
    p.calculate(*sm);

    Debug::print(&p, s, true);
//    Debug::printSchedulingLog(*sm, s);

    QCOMPARE(task1->endTime(), task1->mustFinishOn());
    QCOMPARE(task2->endTime(), task1->startTime() - Duration(0, 16, 0));

    task2->setConstraintEndTime(task1->mustFinishOn().addDays(2));

    sm->createSchedules();
    p.calculate(*sm);

    Debug::print(&p, s, true);
//    Debug::printSchedulingLog(*sm, s);

    QCOMPARE(task1->endTime(), task1->mustFinishOn());
    QCOMPARE(task2->endTime(), task2->finishNotLater());
}

void ProjectTester::fixedInterval()
{
    Project p;
    p.setName(QStringLiteral("P1"));
    p.setId(p.uniqueNodeId());
    p.registerNodeId(&p);
    DateTime st = QDateTime::fromString(QStringLiteral("2010-10-20T08:00"), Qt::ISODate);
    st.setTimeZone(p.timeZone());
    p.setConstraintStartTime(st);
    p.setConstraintEndTime(st.addDays(5));

    Calendar *c = new Calendar(QStringLiteral("Test"));
    QTime t1(8,0,0);
    int length = 8*60*60*1000; // 8 hours

    for (int i = 1; i <= 7; ++i) {
        CalendarDay *wd1 = c->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(t1, length));
    }
    p.addCalendar(c);
    p.setDefaultCalendar(c);
    
    ResourceGroup *g = new ResourceGroup();
    p.addResourceGroup(g);
    Resource *r1 = new Resource();
    r1->setName(QStringLiteral("R1"));
    p.addResource(r1);
    r1->addParentGroup(g);

    Task *task1 = p.createTask();
    task1->setName(QStringLiteral("T1"));
    task1->setConstraint(Node::FixedInterval);
    task1->setConstraintStartTime(st);
    task1->setConstraintEndTime(st.addDays(1));
    p.addTask(task1, &p);

    QString s = QStringLiteral("Schedule T1 Fixed interval -------");
    qDebug()<<s;

    ScheduleManager *sm = p.createScheduleManager(QStringLiteral("T1 Fixed interval"));
    p.addScheduleManager(sm);
    sm->createSchedules();
    p.calculate(*sm);

    Debug::print(&p, s, true);

    QCOMPARE(task1->startTime(), task1->constraintStartTime());
    QCOMPARE(task1->endTime(), task1->constraintEndTime());

    s = QStringLiteral("Schedule backward: T1 Fixed interval -------");
    qDebug()<<s;

    sm->setSchedulingDirection(true);
    sm->createSchedules();
    p.calculate(*sm);

    Debug::print(&p, s, true);
//     Debug::printSchedulingLog(*sm, s);

    QCOMPARE(task1->startTime(), task1->constraintStartTime());
    QCOMPARE(task1->endTime(), task1->constraintEndTime());
}

void ProjectTester::estimateDuration()
{
    Project p;
    p.setName(QStringLiteral("P1"));
    p.setId(p.uniqueNodeId());
    p.registerNodeId(&p);
    DateTime st = QDateTime::fromString(QStringLiteral("2010-10-20 08:00"), Qt::TextDate);
    st.setTimeZone(p.timeZone());
    p.setConstraintStartTime(st);
    p.setConstraintEndTime(st.addDays(5));

    Calendar *c = new Calendar(QStringLiteral("Test"));
    QTime t1(8,0,0);
    int length = 8*60*60*1000; // 8 hours

    for (int i = 1; i <= 7; ++i) {
        CalendarDay *wd1 = c->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(t1, length));
    }
    p.addCalendar(c);
    p.setDefaultCalendar(c);
    
    ResourceGroup *g = new ResourceGroup();
    p.addResourceGroup(g);
    Resource *r1 = new Resource();
    r1->setName(QStringLiteral("R1"));
    p.addResource(r1);
    r1->addParentGroup(g);

    Task *task1 = p.createTask();
    task1->setName(QStringLiteral("T1"));
    task1->setConstraint(Node::ASAP);
    p.addTask(task1, &p);

    task1->estimate()->setType(Estimate::Type_Duration);
    task1->estimate()->setUnit(Duration::Unit_h);
    task1->estimate()->setExpectedEstimate(10);

    QString s = QStringLiteral("Schedule T1 Estimate type Duration -------");
    qDebug()<<s;

    ScheduleManager *sm = p.createScheduleManager(QStringLiteral("T1 Duration"));
    p.addScheduleManager(sm);
    sm->createSchedules();
    p.calculate(*sm);

    Debug::print(&p, s, true);

    QCOMPARE(task1->startTime(), p.constraintStartTime());
    QCOMPARE(task1->endTime(), task1->startTime() + Duration(0, 10, 0));
    
    s = QStringLiteral("Schedule backward: T1 Estimate type Duration -------");
    qDebug()<<s;

    sm->setSchedulingDirection(true);
    sm->createSchedules();
    p.calculate(*sm);

    Debug::print(&p, s, true);

    QCOMPARE(task1->endTime(), p.constraintEndTime());
    QCOMPARE(task1->startTime(), task1->endTime() - Duration(0, 10, 0));
}

void ProjectTester::startStart()
{
    Project p;
    p.setName(QStringLiteral("P1"));
    p.setId(p.uniqueNodeId());
    p.registerNodeId(&p);
    DateTime st = QDateTime::fromString(QStringLiteral("2010-10-20T00:00:00"), Qt::ISODate);
    st.setTimeZone(p.timeZone());
    p.setConstraintStartTime(st);
    p.setConstraintEndTime(st.addDays(5));

    Calendar *c = new Calendar(QStringLiteral("Test"));
    QTime t1(8,0,0);
    int length = 8*60*60*1000; // 8 hours

    for (int i = 1; i <= 7; ++i) {
        CalendarDay *wd1 = c->weekday(i);
        wd1->setState(CalendarDay::Working);
        wd1->addInterval(TimeInterval(t1, length));
    }
    p.addCalendar(c);
    p.setDefaultCalendar(c);

    ResourceGroup *g = new ResourceGroup();
    p.addResourceGroup(g);
    Resource *r1 = new Resource();
    r1->setName(QStringLiteral("R1"));
    p.addResource(r1);
    r1->addParentGroup(g);

    Resource *r2 = new Resource();
    r2->setName(QStringLiteral("R2"));
    p.addResource(r2);
    r2->addParentGroup(g);

    Task *task1 = p.createTask();
    task1->setName(QStringLiteral("T1"));
    task1->setConstraint(Node::ASAP);
    p.addTask(task1, &p);

    task1->estimate()->setType(Estimate::Type_Duration);
    task1->estimate()->setUnit(Duration::Unit_h);
    task1->estimate()->setExpectedEstimate(2);

    Task *task2 = p.createTask();
    task2->setName(QStringLiteral("T2"));
    task2->setConstraint(Node::ASAP);
    p.addTask(task2, &p);

    task2->estimate()->setType(Estimate::Type_Duration);
    task2->estimate()->setUnit(Duration::Unit_h);
    task2->estimate()->setExpectedEstimate(2);

    task1->addDependChildNode(task2, Relation::StartStart);

    QString s = QStringLiteral("Schedule T1 Lag = 0 -------");
    qDebug()<<s;

    ScheduleManager *sm = p.createScheduleManager(QStringLiteral("Lag = 0"));
    p.addScheduleManager(sm);
    sm->createSchedules();
    p.calculate(*sm);

    Debug::print(&p, s, true);

    QCOMPARE(task1->startTime(), p.constraintStartTime());
    QCOMPARE(task1->lateStart(), task2->lateStart());
    QCOMPARE(task1->startTime(), task2->startTime());

    s = QStringLiteral("Schedule backward T1 Lag = 0 -------");
    qDebug()<<s;

    sm = p.createScheduleManager(QStringLiteral("Backward, Lag = 0"));
    p.addScheduleManager(sm);
    sm->createSchedules();
    sm->setSchedulingDirection(true);
    p.calculate(*sm);

//     Debug::printSchedulingLog(*sm, s);
    Debug::print(&p, s, true);

    qDebug()<<"predeccessors:"<<task2->dependParentNodes();
    QCOMPARE(task2->endTime(), p.constraintEndTime());
    QCOMPARE(task1->lateStart(), task2->lateStart());
    QCOMPARE(task1->startTime(), task2->startTime());

    s = QStringLiteral("Schedule T1 calendar -------");
    qDebug()<<s;

    task1->estimate()->setCalendar(c);

    sm = p.createScheduleManager(QStringLiteral("Lag = 0"));
    p.addScheduleManager(sm);
    sm->createSchedules();
    p.calculate(*sm);

    Debug::print(&p, s, true);

    QCOMPARE(task1->startTime(), p.constraintStartTime() + Duration(0, 8, 0));
    QCOMPARE(task1->lateStart(), task2->lateStart());
    QCOMPARE(task1->startTime(), task2->startTime());

    s = QStringLiteral("Schedule backward T1 calendar -------");
    qDebug()<<s;

    task1->estimate()->setCalendar(nullptr);
    task2->estimate()->setCalendar(c);

    sm = p.createScheduleManager(QStringLiteral("Backward, calendar, Lag = 0"));
    p.addScheduleManager(sm);
    sm->createSchedules();
    sm->setSchedulingDirection(true);
    p.calculate(*sm);

    Debug::print(&p, s, true);

    QCOMPARE(task2->endTime(), p.constraintEndTime() - Duration(0, 8, 0));
    QCOMPARE(task1->lateStart(), task2->lateStart());
    QCOMPARE(task1->startTime(), task2->startTime());

    s = QStringLiteral("Schedule Lag = 1 hour -------");
    qDebug()<<s;

    task1->estimate()->setCalendar(c);
    task2->estimate()->setCalendar(nullptr);

    task1->dependChildNodes().at(0)->setLag(Duration(0, 1, 0));

    sm = p.createScheduleManager(QStringLiteral("Lag = 1 hour"));
    p.addScheduleManager(sm);
    sm->createSchedules();
    p.calculate(*sm);

    Debug::print(&p, s, true);

    QCOMPARE(task1->startTime(), p.constraintStartTime() + Duration(0, 8, 0));
    QCOMPARE(task2->lateStart(), task1->lateStart() + task1->dependChildNodes().at(0)->lag());
    QCOMPARE(task2->startTime(), task1->startTime() + task1->dependChildNodes().at(0)->lag());

    s = QStringLiteral("Schedule backward Lag = 1 hour -------");
    qDebug()<<s;

    task1->estimate()->setCalendar(nullptr);
    task2->estimate()->setCalendar(c);

    sm = p.createScheduleManager(QStringLiteral("Backward, Lag = 1 hour"));
    p.addScheduleManager(sm);
    sm->createSchedules();
    sm->setSchedulingDirection(true);
    p.calculate(*sm);

    Debug::print(&p, s, true);

    QCOMPARE(task2->endTime(), p.constraintEndTime() - Duration(0, 8, 0));
    QCOMPARE(task2->lateStart(), task1->lateStart() + task1->dependChildNodes().at(0)->lag());
    QCOMPARE(task2->startTime(), task1->startTime() + task1->dependChildNodes().at(0)->lag());

    s = QStringLiteral("Schedule resources Lag = 1 hour -------");
    qDebug()<<s;

    task1->estimate()->setCalendar(nullptr);
    task2->estimate()->setCalendar(nullptr);

    ResourceRequest *rr1 = new ResourceRequest(r1, 100);
    task1->requests().addResourceRequest(rr1);
    task1->estimate()->setType(Estimate::Type_Effort);

    ResourceRequest *rr2 = new ResourceRequest(r2, 100);
    task2->requests().addResourceRequest(rr2);
    task2->estimate()->setType(Estimate::Type_Effort);

    sm = p.createScheduleManager(QStringLiteral("Resources, Lag = 1 hour"));
    p.addScheduleManager(sm);
    sm->createSchedules();
    sm->setSchedulingDirection(false);
    p.calculate(*sm);

    Debug::print(&p, s, true);

    QCOMPARE(task1->startTime(), p.constraintStartTime() + Duration(0, 8, 0));
    QCOMPARE(task2->lateStart(), task1->lateStart() + task1->dependChildNodes().at(0)->lag());
    QCOMPARE(task2->startTime(), task1->startTime() + task1->dependChildNodes().at(0)->lag());

    s = QStringLiteral("Schedule backward resources Lag = 1 hour -------");
    qDebug()<<s;

    sm = p.createScheduleManager(QStringLiteral("Resources backward, Lag = 1 hour"));
    p.addScheduleManager(sm);
    sm->createSchedules();
    sm->setSchedulingDirection(true);
    p.calculate(*sm);

    Debug::print(&p, s, true);

    QCOMPARE(task2->endTime(), p.constraintEndTime() - Duration(0, 8, 0));
    QCOMPARE(task2->lateStart(), task1->lateStart() + task1->dependChildNodes().at(0)->lag());
    QCOMPARE(task2->startTime(), task1->startTime() + task1->dependChildNodes().at(0)->lag());

    s = QStringLiteral("Schedule resources w limited availability, Lag = 1 hour -------");
    qDebug()<<s;

    r1->setAvailableFrom(p.constraintStartTime() + Duration(0, 9, 0));
    r1->setAvailableUntil(p.constraintEndTime() - Duration(0, 12, 0));

    sm = p.createScheduleManager(QStringLiteral("Resources, Lag = 1 hour"));
    p.addScheduleManager(sm);
    sm->createSchedules();
    sm->setSchedulingDirection(false);
    p.calculate(*sm);

    Debug::print(&p, s, true);

    QCOMPARE(task1->startTime(), p.constraintStartTime() + Duration(0, 9, 0));
    QCOMPARE(task2->lateStart(), task1->lateStart() + task1->dependChildNodes().at(0)->lag());
    QCOMPARE(task2->startTime(), task1->startTime() + task1->dependChildNodes().at(0)->lag());

    s = QStringLiteral("Schedule backward resources w limited availability Lag = 1 hour -------");
    qDebug()<<s;

    sm = p.createScheduleManager(QStringLiteral("Resources backward, Lag = 1 hour"));
    p.addScheduleManager(sm);
    sm->createSchedules();
    sm->setSchedulingDirection(true);
    p.calculate(*sm);

    Debug::print(&p, s, true);

    QVERIFY(task2->lateStart() >= task1->lateStart() + task1->dependChildNodes().at(0)->lag());
    QVERIFY(task2->startTime() >= task1->startTime() + task1->dependChildNodes().at(0)->lag());
}

void ProjectTester::scheduleTimeZone()
{
    qDebug()<<"System timezone: "<<QTimeZone::systemTimeZone();

    Project project;
    project.setName(QStringLiteral("P1"));
    project.setId(project.uniqueNodeId());
    project.registerNodeId(&project);
    project.setTimeZone(QTimeZone("Europe/Copenhagen"));
    project.setConstraintStartTime(DateTime(QDate::fromString(QStringLiteral("2016-07-04"), Qt::ISODate), QTime(), project.timeZone()));
    project.setConstraintEndTime(DateTime(QDate::fromString(QStringLiteral("2016-07-10"), Qt::ISODate), QTime(), project.timeZone()));
    // standard worktime defines 8 hour day as default
    QVERIFY(project.standardWorktime());
    QCOMPARE(project.standardWorktime()->day(), 8.0);
    
    Calendar *calendar = new Calendar();
    calendar->setName(QStringLiteral("LocalTime 2"));
    calendar->setTimeZone(project.timeZone());
    calendar->setDefault(true);

    QTime time1(9, 0, 0);
    QTime time2 (17, 0, 0);
    int length = time1.msecsTo(time2);
    for (int i=1; i <= 7; ++i) {
        CalendarDay *d = calendar->weekday(i);
        d->setState(CalendarDay::Working);
        d->addInterval(time1, length);
    }
    project.addCalendar(calendar);

    Calendar *cal2 = new Calendar();
    cal2->setName(QStringLiteral("Helsinki"));
    cal2->setTimeZone(QTimeZone("Europe/Helsinki"));
    QVERIFY(cal2->timeZone().isValid());
   
    for (int i=1; i <= 7; ++i) {
        CalendarDay *d = cal2->weekday(i);
        d->setState(CalendarDay::Working);
        d->addInterval(time1, length);
    }
    project.addCalendar(cal2);
    
    QDate today = project.constraintStartTime().date();
    
    Task *t = project.createTask();
    t->setName(QStringLiteral("T1"));
    project.addTask(t, &project);
    t->estimate()->setUnit(Duration::Unit_d);
    t->estimate()->setExpectedEstimate(1.0);
    t->estimate()->setType(Estimate::Type_Effort);
    
    Task *t2 = project.createTask();
    t2->setName(QStringLiteral("T2"));
    project.addTask(t2, &project);
    t2->estimate()->setUnit(Duration::Unit_d);
    t2->estimate()->setExpectedEstimate(1.0);
    t2->estimate()->setType(Estimate::Type_Effort);
    
    ResourceGroup *g = new ResourceGroup();
    g->setName(QStringLiteral("G1"));
    project.addResourceGroup(g);
    Resource *r = new Resource();
    r->setName(QStringLiteral("R1"));
    r->setCalendar(calendar);
    project.addResource(r);
    r->addParentGroup(g);

    Resource *r2 = new Resource();
    r2->setName(QStringLiteral("R2"));
    r2->setCalendar(cal2);
    project.addResource(r2);
    r2->addParentGroup(g);

    ResourceRequest *rr = new ResourceRequest(r, 100);
    t->requests().addResourceRequest(rr);

    ResourceRequest *rr2 = new ResourceRequest(r2, 100);
    t2->requests().addResourceRequest(rr2);
    
    QString s = QStringLiteral("Calculate forward, Task: ASAP -----------------------------------");
    qDebug()<<'\n'<<"Testing:"<<s;
    ScheduleManager *sm = project.createScheduleManager(QStringLiteral("Test Plan"));
    project.addScheduleManager(sm);
    sm->createSchedules();
    project.calculate(*sm);
    
    Debug::print(&project, t, s);
//     Debug::printSchedulingLog(*sm, s);
    
    QCOMPARE(t->startTime(), DateTime(today, time1, project.timeZone()));
    QCOMPARE(t->endTime(), t->startTime() + Duration(0, 8, 0));
    QCOMPARE(t->plannedEffort().toHours(), 8.0);
    QVERIFY(t->schedulingError() == false);

    QCOMPARE(t2->startTime(), DateTime(today, time1.addSecs(-3600), project.timeZone()));
    QCOMPARE(t2->endTime(), t2->startTime() + Duration(0, 8, 0));
    QCOMPARE(t2->plannedEffort().toHours(), 8.0);
    QVERIFY(t2->schedulingError() == false);
}

void ProjectTester::resourceTimezoneSpansMidnight()
{
    qDebug()<<"Local timezone: "<<QTimeZone::systemTimeZone();

    Calendar cal(QStringLiteral("LocalTime"));
    QCOMPARE(cal.timeZone(),  QTimeZone::systemTimeZone());

    Project project;
    project.setName(QStringLiteral("P1"));
    project.setId(project.uniqueNodeId());
    project.setTimeZone(QTimeZone("Europe/Copenhagen"));
    project.registerNodeId(&project);
    project.setConstraintStartTime(DateTime(QDate::fromString(QStringLiteral("2022-02-01"), Qt::ISODate), QTime(8, 0), project.timeZone()));
    project.setConstraintEndTime(DateTime(QDate::fromString(QStringLiteral("2022-05-01"), Qt::ISODate), QTime(), project.timeZone()));
    // standard worktime defines 8 hour day as default
    QVERIFY(project.standardWorktime());
    QCOMPARE(project.standardWorktime()->day(), 8.0);

    Calendar *calendar = new Calendar();
    calendar->setName(QStringLiteral("LocalTime 2"));
    calendar->setDefault(true);
    QCOMPARE(calendar->timeZone(),  QTimeZone::systemTimeZone());

    QTime time1(8, 0, 0);
    QTime time2 (16, 0, 0);
    int length = time1.msecsTo(time2);
    for (int i=1; i <= 7; ++i) {
        CalendarDay *d = calendar->weekday(i);
        d->setState(CalendarDay::Working);
        d->addInterval(time1, length);
    }
    project.addCalendar(calendar);

    Calendar *cal2 = new Calendar();
    cal2->setName(QStringLiteral("Sydney"));
    cal2->setTimeZone(QTimeZone("Australia/Sydney"));
    QVERIFY(cal2->timeZone().isValid());

    for (int i=1; i <= 7; ++i) {
        CalendarDay *d = cal2->weekday(i);
        d->setState(CalendarDay::Working);
        d->addInterval(time1, length);
    }
    project.addCalendar(cal2);

    Task *t = project.createTask();
    t->setName(QStringLiteral("T1"));
    project.addTask(t, &project);
    t->estimate()->setUnit(Duration::Unit_h);
    t->estimate()->setExpectedEstimate(10.0);
    t->estimate()->setType(Estimate::Type_Effort);

    ResourceGroup *g = new ResourceGroup();
    Resource *r2 = new Resource();
    r2->setName(QStringLiteral("R2"));
    r2->setCalendar(cal2);
    project.addResource(r2);
    r2->addParentGroup(g);
    QVERIFY(!r2->availableFrom().isValid());

    ResourceRequest *rr2 = new ResourceRequest(r2, 20);
    t->requests().addResourceRequest(rr2);

    QString s = QStringLiteral("Ref Bug 449786 -----------------------------------");
    qDebug()<<'\n'<<"Testing:"<<s;

    ScheduleManager *sm = project.createScheduleManager(QStringLiteral("Test Plan"));
    project.addScheduleManager(sm);
    sm->createSchedules();
    project.calculate(*sm);

//     Debug::print(&project, t, s);
//     Debug::printSchedulingLog(*sm, s);
    QVERIFY(t->schedulingError() == false);

    auto a2 = r2->appointmentIntervals();
    QCOMPARE(a2.startTime().timeZone(), project.timeZone());
    QCOMPARE(a2.startTime(), DateTime(QDate(2022, 2, 1), QTime(22, 0), project.timeZone()));
    QCOMPARE(t->startTime(), a2.startTime());
    QCOMPARE(t->plannedEffort().toHours(), t->estimate()->expectedEstimate());
    QCOMPARE(t->endTime(), t->startTime() + Duration(6, 2, 0));

    s = QStringLiteral("Resource timezone later than project timezone -----");
    qDebug()<<'\n'<<"Testing:"<<s;

    cal2->setTimeZone(QTimeZone("America/Ensenada"));

    ScheduleManager *sm2 = project.createScheduleManager(QStringLiteral("Test Plan"));
    project.addScheduleManager(sm2);
    sm2->createSchedules();
    project.calculate(*sm2);

//     Debug::print(&project, t, s);
//     Debug::printSchedulingLog(*sm, s);
    QVERIFY(t->schedulingError() == false);

    a2 = r2->appointmentIntervals();
    QCOMPARE(a2.startTime().timeZone(), project.timeZone());
    QCOMPARE(a2.startTime().toString(Qt::ISODate), QStringLiteral("2022-02-01T17:00:00+01:00"));
    QCOMPARE(t->startTime(), a2.startTime());
    QCOMPARE(t->plannedEffort().toHours(), t->estimate()->expectedEstimate());
    QCOMPARE(t->endTime(), t->startTime() + Duration(6, 2, 0));

    s = QStringLiteral("Project timezone different from system timezone -----");
    qDebug()<<"Testing:"<<s;
    project.setTimeZone(QTimeZone("Europe/London"));
    qDebug()<<"Project timezone:"<<project.constraintStartTime().timeZone()<<"System:"<<QTimeZone::systemTimeZone();

    cal2->setTimeZone(QTimeZone("Australia/Sydney"));
    auto dt = cal2->firstAvailableAfter(project.constraintStartTime(), project.constraintEndTime());

    ScheduleManager *sm3 = project.createScheduleManager(QStringLiteral("Test Plan"));
    project.addScheduleManager(sm3);
    sm3->createSchedules();
    project.calculate(*sm3);

//     Debug::print(&project, t, s);
//     Debug::printSchedulingLog(*sm, s);
    QVERIFY(t->schedulingError() == false);

    a2 = r2->appointmentIntervals();
    QCOMPARE(a2.startTime(), DateTime::fromString(QStringLiteral("2022-02-01T21:00:00"), project.timeZone()));
    QCOMPARE(t->startTime(), a2.startTime());
    QCOMPARE(t->plannedEffort().toHours(), t->estimate()->expectedEstimate());
    QCOMPARE(t->endTime(), t->startTime() + Duration(6, 2, 0));
}

} //namespace KPlato

QTEST_GUILESS_MAIN(KPlato::ProjectTester)
