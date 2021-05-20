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
#include "ReScheduleTester.h"

#include "kptproject.h"
#include "Resource.h"
#include "kptdatetime.h"
#include "kptschedule.h"

#include "debug.cpp"

namespace QTest
{
    template<>
            char *toString(const KPlato::DateTimeInterval &dt)
    {
        if (dt.first.isValid() && dt.second.isValid())
            return toString(dt.first.toString() + " - " + dt.second.toString());

        return toString("invalid interval");
    }
}

namespace KPlato
{

void ReScheduleTester::init()
{
    m_project = new Project();
    m_project->setId(m_project->uniqueNodeId());
    m_project->registerNodeId(m_project);
    m_project->setConstraintStartTime(QDateTime::fromString("2012-02-01T00:00", Qt::ISODate).toTimeZone(QTimeZone::systemTimeZone()));
    m_project->setConstraintEndTime(QDateTime::fromString("2012-03-01T00:00", Qt::ISODate).toTimeZone(QTimeZone::systemTimeZone()));
    // standard worktime defines 8 hour day as default
    QVERIFY(m_project->standardWorktime());
    QCOMPARE(m_project->standardWorktime()->day(), 8.0);
    auto calendar = new Calendar();
    calendar->setName("C1");
    calendar->setDefault(true);
    QTime t1(9, 0, 0);
    QTime t2 (17, 0, 0);
    int length = t1.msecsTo(t2);
    for (int i=1; i <= 7; ++i) {
        CalendarDay *d = calendar->weekday(i);
        d->setState(CalendarDay::Working);
        d->addInterval(t1, length);
    }
    m_project->addCalendar(calendar);

    resource1 = new Resource();
    resource1->setName("R1");
    resource1->setCalendar(calendar);
    m_project->addResource(resource1);

    resource2 = new Resource();
    resource2->setName("R2");
    resource2->setCalendar(calendar);
    m_project->addResource(resource2);

    Task *t = m_project->createTask();
    t->setName("T1");
    t->setPriority(10);
    m_project->addTask(t, m_project);
    t->completion().setEntrymode(Completion::EnterEffortPerResource);
    t->estimate()->setUnit(Duration::Unit_d);
    t->estimate()->setExpectedEstimate(6.0);
    t->estimate()->setType(Estimate::Type_Effort);
    ResourceRequest *request = new ResourceRequest(resource1, 100);
    t->requests().addResourceRequest(request);

    t = m_project->createTask();
    t->setName("T2");
    t->setPriority(8);
    m_project->addTask(t, m_project);
    t->completion().setEntrymode(Completion::EnterEffortPerResource);
    t->estimate()->setUnit(Duration::Unit_d);
    t->estimate()->setExpectedEstimate(6.0);
    t->estimate()->setType(Estimate::Type_Effort);
    request = new ResourceRequest(resource1, 100);
    t->requests().addResourceRequest(request);

    t = m_project->createTask();
    t->setName("T3");
    t->setPriority(6);
    m_project->addTask(t, m_project);
    t->completion().setEntrymode(Completion::EnterEffortPerResource);
    t->estimate()->setUnit(Duration::Unit_d);
    t->estimate()->setExpectedEstimate(6.0);
    t->estimate()->setType(Estimate::Type_Effort);
    request = new ResourceRequest(resource1, 100);
    t->requests().addResourceRequest(request);

    ScheduleManager *sm = m_project->createScheduleManager("Test Plan");
    m_project->addScheduleManager(sm);
    sm->createSchedules();
    m_project->calculate(*sm);

//     Debug::print(m_project, "", true);
}

void ReScheduleTester::cleanup()
{
    delete m_project;
}

void ReScheduleTester::completionPerTask()
{
    QDate date(2012, 02, 01);

    auto t = static_cast<Task*>(m_project->childNode(0));
    t->completion().setEntrymode(Completion::EnterEffortPerTask);
    QVERIFY(t->startTime().date() == date);

    t->completion().setStarted(true);
    t->completion().setStartTime(t->startTime());
    t->completion().setActualEffort(date, Duration(0, 8, 0, 0));
    t->completion().setRemainingEffort(date, Duration(0, 40, 0, 0));
    t->completion().setPercentFinished(date, 100/6);

    QHash<Resource*, Appointment> apps = t->completion().createAppointments();
    QCOMPARE(apps.count(), 1);
    qInfo()<<apps;
    QHash<Resource*, Appointment>::const_iterator it = apps.constBegin();
    QVERIFY(it.key() == resource1);
    QVERIFY(!it.value().isEmpty());
    QCOMPARE(it.value().count(), 1);
    QVERIFY(it.value().startTime() == t->completion().startTime());
    QCOMPARE(it.value().intervalAt(0).load(), 53); // 8 hours work in 15 hours
}

void ReScheduleTester::completionPerResource()
{
    QDate date(2012, 02, 01);

    auto t = static_cast<Task*>(m_project->childNode(0));
    QVERIFY(t->startTime().date() == date);

    t->completion().setStarted(true);
    t->completion().setStartTime(t->startTime());
    t->completion().setActualEffort(resource1, date, Completion::UsedEffort::ActualEffort(Duration(0, 8, 0, 0)));
    t->completion().setRemainingEffort(date, Duration(0, 40, 0, 0));
    t->completion().setPercentFinished(date, 100/6);

    QHash<Resource*, Appointment> apps = t->completion().createAppointments();
    QCOMPARE(apps.count(), 1);
    QHash<Resource*, Appointment>::const_iterator it = apps.constBegin();
    QVERIFY(it.key() == resource1);
    QVERIFY(!it.value().isEmpty());
    QCOMPARE(it.value().count(), 1);
    QVERIFY(it.value().startTime() == t->completion().startTime());
    QCOMPARE(it.value().intervalAt(0).load(), 53); // 8 hours work in 15 hours

    date = date.addDays(1);
    t->completion().setActualEffort(resource2, date, Completion::UsedEffort::ActualEffort(Duration(0, 8, 0, 0)));
    t->completion().setRemainingEffort(date, Duration(0, 32, 0, 0));
    t->completion().setPercentFinished(date, 100/3);

    QHash<Resource*, Appointment> apps2 = t->completion().createAppointments();
    QCOMPARE(apps2.count(), 2);
    QHash<Resource*, Appointment>::const_iterator it2 = apps2.constBegin();
    for (; it2 != apps2.constEnd(); ++it2) {
        if (it2.key() == resource1) {
            QVERIFY(!it2.value().isEmpty());
            QCOMPARE(it2.value().count(), 1);
            QString str = QString("Resource: %3: Appointment: %1, Expected: %2").arg(it2.value().startTime().toString(Qt::ISODate)).arg(t->completion().startTime().toString(Qt::ISODate).arg(it2.key()->name()));
            QVERIFY2(it2.value().startTime() == t->completion().startTime(), str.toLatin1());
            QCOMPARE(it2.value().intervalAt(0).load(), 53); // 8 hours work in 15 hours
        } else if (it2.key() == resource2) {
            QVERIFY(!it2.value().isEmpty());
            QCOMPARE(it2.value().count(), 1);
            QString str = QString("Resource: %3: Appointment: %1, Expected: %2").arg(it2.value().startTime().toString(Qt::ISODate)).arg(DateTime(date.startOfDay()).toString(Qt::ISODate).arg(it2.key()->name()));
            QVERIFY(it2.value().startTime() == DateTime(date.startOfDay()));
            QCOMPARE(it2.value().intervalAt(0).load(), 33); // 8 hours work in 24 hours
        } else {
            QVERIFY2(false, "Invalid resource");
        }
    }
}

void ReScheduleTester::reschedulePerTask()
{
    auto parentManager = m_project->createScheduleManager("Test Plan");
    m_project->addScheduleManager(parentManager);
    parentManager->createSchedules();
    m_project->calculate(*parentManager);
    auto parentId = parentManager->expected()->id();

    QDate date(2012, 2, 1);
    QDateTime start(date, QTime(9, 0, 0));

    QCOMPARE(m_project->numChildren(), 3);
    auto t1 = static_cast<Task*>(m_project->childNode(0));
    auto t2 = static_cast<Task*>(m_project->childNode(1));
    auto t3 = static_cast<Task*>(m_project->childNode(2));

    auto str = QString("Value: %1, Expected: %2").arg(t1->startTime(parentId).toString(Qt::ISODate)).arg(start.toString(Qt::ISODate));
    QVERIFY2(t1->startTime(parentId) == start, str.toLatin1());
    str = QString("Value: %1, Expected: %2").arg(t2->startTime(parentId).toString(Qt::ISODate)).arg(start.addDays(6).toString(Qt::ISODate));
    QVERIFY2(t2->startTime(parentId) == start.addDays(6), str.toLatin1());
    str = QString("Value: %1, Expected: %2").arg(t3->startTime(parentId).toString(Qt::ISODate)).arg(start.addDays(12).toString(Qt::ISODate));
    QVERIFY2(t3->startTime(parentId) == start.addDays(12), str.toLatin1());

    t1->completion().setStarted(true);
    t1->completion().setStartTime(t1->startTime(parentId));
    t1->completion().setFinished(true);
    t1->completion().setFinishTime(t1->endTime(parentId));

    t2->completion().setEntrymode(Completion::EnterEffortPerTask);
    t2->completion().setStarted(true);
    t2->completion().setStartTime(t2->startTime());
    t2->completion().setActualEffort(t2->startTime().date(), Duration(0, 8, 0, 0));
    t2->completion().setRemainingEffort(t2->startTime().date(), Duration(0, 40, 0, 0));
    t2->completion().setPercentFinished(t2->startTime().date(), 100/6);

    auto childManager = m_project->createScheduleManager(parentManager);
    childManager->setRecalculate(true);
    auto recalculateFrom = m_project->constraintStartTime().addDays(14);  // 2012-02-15T00:00:00
    childManager->setRecalculateFrom(recalculateFrom);
    childManager->setParentManager(parentManager);
    QCOMPARE(childManager->parentManager(), parentManager);
    childManager->createSchedules();
    m_project->calculate(*childManager);
    auto childId = childManager->expected()->id();

    // t1: not moved
    str = QString("Value: %1, Expected: %2").arg(t1->startTime(childId).toString(Qt::ISODate)).arg(t1->startTime(parentId).toString(Qt::ISODate));
    QVERIFY2(t1->startTime(childId) == t1->startTime(parentId), str.toLatin1());
    str = QString("Value: %1, Expected: %2").arg(t1->endTime(childId).toString(Qt::ISODate)).arg(t1->endTime(parentId).toString(Qt::ISODate));
    QVERIFY2(t1->endTime(childId).date() == t1->endTime(parentId).date(), str.toLatin1());
    // t2: same start, end at 2012-02-12T17:00:00
    str = QString("Value: %1, Expected: %2").arg(t2->startTime(childId).toString(Qt::ISODate)).arg(t2->startTime(parentId).toString(Qt::ISODate));
    QVERIFY2(t2->startTime(childId).date() == t2->startTime(parentId).date(), str.toLatin1());
    auto t2End = recalculateFrom.addDays(4);
    t2End.setTime(QTime(17, 0, 0));
    str = QString("Value: %1, Expected: %2").arg(t2->endTime(childId).toString(Qt::ISODate)).arg(t2End.toString(Qt::ISODate));
    QVERIFY2(t2->endTime(childId) == t2End, str.toLatin1());

    // check appoinments
    QCOMPARE(t2->currentSchedule()->appointments().count(), 1);
    QCOMPARE(t2->currentSchedule()->appointments().at(0)->count(), 6);

    auto interval = t2->currentSchedule()->appointments().at(0)->intervalAt(0);
    DateTime dt = QDateTime::fromString("2012-02-07T09:00:00", Qt::ISODate);
    str = QString("Value: %1, Expected: %2").arg(interval.startTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.startTime() == dt, str.toLatin1());
    dt = QDateTime::fromString("2012-02-08T00:00:00", Qt::ISODate); // rest of the day
    str = QString("Value: %1, Expected: %2").arg(interval.endTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.endTime() == dt, str.toLatin1());
    QCOMPARE(interval.load(), 53);

    interval = t2->currentSchedule()->appointments().at(0)->intervalAt(1);
    dt = recalculateFrom;
    dt.setTime(QTime(9, 0, 0));
    str = QString("Value: %1, Expected: %2").arg(interval.startTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.startTime() == dt, str.toLatin1());
    dt.setTime(QTime(17, 0, 0));
    str = QString("Value: %1, Expected: %2").arg(interval.endTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.endTime() == dt, str.toLatin1());
    QCOMPARE(interval.load(), 100);

    interval = t2->currentSchedule()->appointments().at(0)->intervalAt(2);
    dt = dt.addDays(1);
    dt.setTime(QTime(9, 0, 0));
    str = QString("Value: %1, Expected: %2").arg(interval.startTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.startTime() == dt, str.toLatin1());
    dt.setTime(QTime(17, 0, 0));
    str = QString("Value: %1, Expected: %2").arg(interval.endTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.endTime() == dt, str.toLatin1());
    QCOMPARE(interval.load(), 100);

    interval = t2->currentSchedule()->appointments().at(0)->intervalAt(3);
    dt = dt.addDays(1);
    dt.setTime(QTime(9, 0, 0));
    str = QString("Value: %1, Expected: %2").arg(interval.startTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.startTime() == dt, str.toLatin1());
    dt.setTime(QTime(17, 0, 0));
    str = QString("Value: %1, Expected: %2").arg(interval.endTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.endTime() == dt, str.toLatin1());
    QCOMPARE(interval.load(), 100);

    interval = t2->currentSchedule()->appointments().at(0)->intervalAt(4);
    dt = dt.addDays(1);
    dt.setTime(QTime(9, 0, 0));
    str = QString("Value: %1, Expected: %2").arg(interval.startTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.startTime() == dt, str.toLatin1());
    dt.setTime(QTime(17, 0, 0));
    str = QString("Value: %1, Expected: %2").arg(interval.endTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.endTime() == dt, str.toLatin1());
    QCOMPARE(interval.load(), 100);

    interval = t2->currentSchedule()->appointments().at(0)->intervalAt(5);
    dt = dt.addDays(1);
    dt.setTime(QTime(9, 0, 0));
    str = QString("Value: %1, Expected: %2").arg(interval.startTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.startTime() == dt, str.toLatin1());
    dt.setTime(QTime(17, 0, 0));
    str = QString("Value: %1, Expected: %2").arg(interval.endTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.endTime() == dt, str.toLatin1());
    QCOMPARE(interval.load(), 100);
}

void ReScheduleTester::reschedulePerResource()
{
    auto parentManager = m_project->createScheduleManager("Test Plan");
    m_project->addScheduleManager(parentManager);
    parentManager->createSchedules();
    m_project->calculate(*parentManager);
    auto parentId = parentManager->expected()->id();

    QDate date(2012, 2, 1);
    QDateTime start(date, QTime(9, 0, 0));

    QCOMPARE(m_project->numChildren(), 3);
    auto t1 = static_cast<Task*>(m_project->childNode(0));
    auto t2 = static_cast<Task*>(m_project->childNode(1));
    auto t3 = static_cast<Task*>(m_project->childNode(2));

    auto str = QString("Value: %1, Expected: %2").arg(t1->startTime(parentId).toString(Qt::ISODate)).arg(start.toString(Qt::ISODate));
    QVERIFY2(t1->startTime(parentId) == start, str.toLatin1());
    str = QString("Value: %1, Expected: %2").arg(t2->startTime(parentId).toString(Qt::ISODate)).arg(start.addDays(6).toString(Qt::ISODate));
    QVERIFY2(t2->startTime(parentId) == start.addDays(6), str.toLatin1());
    str = QString("Value: %1, Expected: %2").arg(t3->startTime(parentId).toString(Qt::ISODate)).arg(start.addDays(12).toString(Qt::ISODate));
    QVERIFY2(t3->startTime(parentId) == start.addDays(12), str.toLatin1());

    t1->completion().setStarted(true);
    t1->completion().setStartTime(t1->startTime(parentId));
    t1->completion().setFinished(true);
    t1->completion().setFinishTime(t1->endTime(parentId));

    t2->completion().setStarted(true);
    t2->completion().setStartTime(t2->startTime());
    t2->completion().setActualEffort(resource1, t2->startTime().date(), Completion::UsedEffort::ActualEffort(Duration(0, 8, 0, 0)));
    t2->completion().setRemainingEffort(t2->startTime().date(), Duration(0, 40, 0, 0));
    t2->completion().setPercentFinished(t2->startTime().date(), 100/6);

    auto childManager = m_project->createScheduleManager(parentManager);
    childManager->setRecalculate(true);
    auto recalculateFrom = m_project->constraintStartTime().addDays(14);  // 2012-02-15T00:00:00
    childManager->setRecalculateFrom(recalculateFrom);
    childManager->setParentManager(parentManager);
    QCOMPARE(childManager->parentManager(), parentManager);
    childManager->createSchedules();
    m_project->calculate(*childManager);
    auto childId = childManager->expected()->id();

    // t1: not moved
    str = QString("Value: %1, Expected: %2").arg(t1->startTime(childId).toString(Qt::ISODate)).arg(t1->startTime(parentId).toString(Qt::ISODate));
    QVERIFY2(t1->startTime(childId) == t1->startTime(parentId), str.toLatin1());
    str = QString("Value: %1, Expected: %2").arg(t1->endTime(childId).toString(Qt::ISODate)).arg(t1->endTime(parentId).toString(Qt::ISODate));
    QVERIFY2(t1->endTime(childId).date() == t1->endTime(parentId).date(), str.toLatin1());
    // t2: same start, end at recalculateFrom + 4 days, 8 hours
    str = QString("Value: %1, Expected: %2").arg(t2->startTime(childId).toString(Qt::ISODate)).arg(t2->startTime(parentId).toString(Qt::ISODate));
    QVERIFY2(t2->startTime(childId).date() == t2->startTime(parentId).date(), str.toLatin1());
    auto t2End = recalculateFrom.addDays(4);
    t2End.setTime(QTime(17, 0, 0));
    str = QString("Value: %1, Expected: %2").arg(t2->endTime(childId).toString(Qt::ISODate)).arg(t2End.toString(Qt::ISODate));
    QVERIFY2(t2->endTime(childId) == t2End, str.toLatin1());

    // check appoinments
    QCOMPARE(t2->currentSchedule()->appointments().count(), 1);
    QCOMPARE(t2->currentSchedule()->appointments().at(0)->count(), 6);

    auto interval = t2->currentSchedule()->appointments().at(0)->intervalAt(0);
    DateTime dt = QDateTime::fromString("2012-02-07T09:00:00", Qt::ISODate);
    str = QString("Value: %1, Expected: %2").arg(interval.startTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.startTime() == dt, str.toLatin1());
    dt = QDateTime::fromString("2012-02-08T00:00:00", Qt::ISODate); // rest of the day
    str = QString("Value: %1, Expected: %2").arg(interval.endTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.endTime() == dt, str.toLatin1());
    QCOMPARE(interval.load(), 53);

    interval = t2->currentSchedule()->appointments().at(0)->intervalAt(1);
    dt = recalculateFrom;
    dt.setTime(QTime(9, 0, 0));
    str = QString("Value: %1, Expected: %2").arg(interval.startTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.startTime() == dt, str.toLatin1());
    dt.setTime(QTime(17, 0, 0));
    str = QString("Value: %1, Expected: %2").arg(interval.endTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.endTime() == dt, str.toLatin1());
    QCOMPARE(interval.load(), 100);

    interval = t2->currentSchedule()->appointments().at(0)->intervalAt(2);
    dt = dt.addDays(1);
    dt.setTime(QTime(9, 0, 0));
    str = QString("Value: %1, Expected: %2").arg(interval.startTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.startTime() == dt, str.toLatin1());
    dt.setTime(QTime(17, 0, 0));
    str = QString("Value: %1, Expected: %2").arg(interval.endTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.endTime() == dt, str.toLatin1());
    QCOMPARE(interval.load(), 100);

    interval = t2->currentSchedule()->appointments().at(0)->intervalAt(3);
    dt = dt.addDays(1);
    dt.setTime(QTime(9, 0, 0));
    str = QString("Value: %1, Expected: %2").arg(interval.startTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.startTime() == dt, str.toLatin1());
    dt.setTime(QTime(17, 0, 0));
    str = QString("Value: %1, Expected: %2").arg(interval.endTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.endTime() == dt, str.toLatin1());
    QCOMPARE(interval.load(), 100);

    interval = t2->currentSchedule()->appointments().at(0)->intervalAt(4);
    dt = dt.addDays(1);
    dt.setTime(QTime(9, 0, 0));
    str = QString("Value: %1, Expected: %2").arg(interval.startTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.startTime() == dt, str.toLatin1());
    dt.setTime(QTime(17, 0, 0));
    str = QString("Value: %1, Expected: %2").arg(interval.endTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.endTime() == dt, str.toLatin1());
    QCOMPARE(interval.load(), 100);

    interval = t2->currentSchedule()->appointments().at(0)->intervalAt(5);
    dt = dt.addDays(1);
    dt.setTime(QTime(9, 0, 0));
    str = QString("Value: %1, Expected: %2").arg(interval.startTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.startTime() == dt, str.toLatin1());
    dt.setTime(QTime(17, 0, 0));
    str = QString("Value: %1, Expected: %2").arg(interval.endTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.endTime() == dt, str.toLatin1());
    QCOMPARE(interval.load(), 100);


    t2->completion().setActualEffort(resource2, t2->startTime().date().addDays(1), Completion::UsedEffort::ActualEffort(Duration(0, 8, 0, 0)));
    t2->completion().setRemainingEffort(t2->startTime().date(), Duration(0, 32, 0, 0));
    t2->completion().setPercentFinished(t2->startTime().date(), 100/3);

    childManager = m_project->createScheduleManager(parentManager);
    childManager->setRecalculate(true);
    recalculateFrom = m_project->constraintStartTime().addDays(14);  // 2012-02-15T00:00:00
    childManager->setRecalculateFrom(recalculateFrom);
    childManager->setParentManager(parentManager);
    QCOMPARE(childManager->parentManager(), parentManager);
    childManager->createSchedules();
    m_project->calculate(*childManager);
    childId = childManager->expected()->id();

    // t1: not moved
    str = QString("Value: %1, Expected: %2").arg(t1->startTime(childId).toString(Qt::ISODate)).arg(t1->startTime(parentId).toString(Qt::ISODate));
    QVERIFY2(t1->startTime(childId) == t1->startTime(parentId), str.toLatin1());
    str = QString("Value: %1, Expected: %2").arg(t1->endTime(childId).toString(Qt::ISODate)).arg(t1->endTime(parentId).toString(Qt::ISODate));
    QVERIFY2(t1->endTime(childId).date() == t1->endTime(parentId).date(), str.toLatin1());
    // t2: same start, end at recalculateFrom + 3 days, 8 hours
    str = QString("Value: %1, Expected: %2").arg(t2->startTime(childId).toString(Qt::ISODate)).arg(t2->startTime(parentId).toString(Qt::ISODate));
    QVERIFY2(t2->startTime(childId).date() == t2->startTime(parentId).date(), str.toLatin1());
    t2End = recalculateFrom.addDays(3);
    t2End.setTime(QTime(17, 0, 0));
    str = QString("Value: %1, Expected: %2").arg(t2->endTime(childId).toString(Qt::ISODate)).arg(t2End.toString(Qt::ISODate));
    QVERIFY2(t2->endTime(childId) == t2End, str.toLatin1());

    QCOMPARE(t2->currentSchedule()->appointments().count(), 2);
    int posR1 = 0;
    int posR2 = 1;
    if (t2->currentSchedule()->appointments().at(0)->resource()->resource() == resource2) {
        posR1 = 1;
        posR2 = 0;
    }
    QCOMPARE(t2->currentSchedule()->appointments().at(posR1)->count(), 5);
    QCOMPARE(t2->currentSchedule()->appointments().at(posR2)->count(), 1);

    interval = t2->currentSchedule()->appointments().at(posR1)->intervalAt(0);
    dt = QDateTime::fromString("2012-02-07T09:00:00", Qt::ISODate);
    str = QString("Value: %1, Expected: %2").arg(interval.startTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.startTime() == dt, str.toLatin1());
    dt = QDateTime::fromString("2012-02-08T00:00:00", Qt::ISODate); // rest of the day
    str = QString("Value: %1, Expected: %2").arg(interval.endTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.endTime() == dt, str.toLatin1());
    QCOMPARE(interval.load(), 53);

    interval = t2->currentSchedule()->appointments().at(posR2)->intervalAt(0);
    dt = QDateTime::fromString("2012-02-08T00:00:00", Qt::ISODate);
    str = QString("Value: %1, Expected: %2").arg(interval.startTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.startTime() == dt, str.toLatin1());
    dt = dt.addDays(1); // rest of the day
    str = QString("Value: %1, Expected: %2").arg(interval.endTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.endTime() == dt, str.toLatin1());
    QCOMPARE(interval.load(), 33);

    interval = t2->currentSchedule()->appointments().at(posR1)->intervalAt(1);
    dt = recalculateFrom;
    dt.setTime(QTime(9, 0, 0));
    str = QString("Value: %1, Expected: %2").arg(interval.startTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.startTime() == dt, str.toLatin1());
    dt.setTime(QTime(17, 0, 0));
    str = QString("Value: %1, Expected: %2").arg(interval.endTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.endTime() == dt, str.toLatin1());
    QCOMPARE(interval.load(), 100);

    interval = t2->currentSchedule()->appointments().at(posR1)->intervalAt(2);
    dt = dt.addDays(1);
    dt.setTime(QTime(9, 0, 0));
    str = QString("Value: %1, Expected: %2").arg(interval.startTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.startTime() == dt, str.toLatin1());
    dt.setTime(QTime(17, 0, 0));
    str = QString("Value: %1, Expected: %2").arg(interval.endTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.endTime() == dt, str.toLatin1());
    QCOMPARE(interval.load(), 100);

    interval = t2->currentSchedule()->appointments().at(posR1)->intervalAt(3);
    dt = dt.addDays(1);
    dt.setTime(QTime(9, 0, 0));
    str = QString("Value: %1, Expected: %2").arg(interval.startTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.startTime() == dt, str.toLatin1());
    dt.setTime(QTime(17, 0, 0));
    str = QString("Value: %1, Expected: %2").arg(interval.endTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.endTime() == dt, str.toLatin1());
    QCOMPARE(interval.load(), 100);

    interval = t2->currentSchedule()->appointments().at(posR1)->intervalAt(4);
    dt = dt.addDays(1);
    dt.setTime(QTime(9, 0, 0));
    str = QString("Value: %1, Expected: %2").arg(interval.startTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.startTime() == dt, str.toLatin1());
    dt.setTime(QTime(17, 0, 0));
    str = QString("Value: %1, Expected: %2").arg(interval.endTime().toString(Qt::ISODate)).arg(dt.toString(Qt::ISODate));
    QVERIFY2(interval.endTime() == dt, str.toLatin1());
    QCOMPARE(interval.load(), 100);
}

} //namespace KPlato

QTEST_GUILESS_MAIN(KPlato::ReScheduleTester)
