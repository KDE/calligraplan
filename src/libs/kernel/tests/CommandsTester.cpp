/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2011 Pierre Stirnweiss <pstirnweiss@googlemail.com>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "CommandsTester.h"

#include <kptcommand.h>

#include <kptcalendar.h>
#include <kptproject.h>
#include <kptresource.h>

#include <QTest>

namespace KPlato {

void CommandsTester::initTestCase()
{
    m_project = new Project();
    m_project->setId(m_project->uniqueNodeId());
    m_project->registerNodeId(m_project); //->rootTask());
    // standard worktime defines 8 hour day as default
    QVERIFY(m_project->standardWorktime());
    QCOMPARE(m_project->standardWorktime()->day(), 8.0);
/*    m_calendar = new Calendar();
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
*/
}

void CommandsTester::cleanupTestCase()
{
    delete m_project;
}

void CommandsTester::testNamedCommand()
{
    //todo: test the schedule handling
}

void CommandsTester::testCalendarAddCmd()
{
    Calendar *calendar1 = new Calendar();
    Calendar *calendar2 = new Calendar();

    CalendarAddCmd *cmd1 = new CalendarAddCmd(m_project, calendar1, 0, nullptr);
    cmd1->execute();
    QVERIFY(m_project->calendarAt(0) == calendar1); //NOLINT(clang-analyzer-cplusplus.NewDeleteLeaks)
    CalendarAddCmd *cmd2 = new CalendarAddCmd(m_project, calendar2, 0, calendar1);
    cmd2->execute();
    QVERIFY(calendar1->childAt(0) == calendar2);
    cmd2->unexecute();
    QVERIFY(!calendar1->childCount());
    cmd1->unexecute();
    QVERIFY(!m_project->calendarCount());

    delete cmd1;
    delete cmd2; //These also delete the calendars
}

void CommandsTester::testCalendarRemoveCmd()
{
    Calendar *calendar1 = new Calendar();
    Calendar *calendar2 = new Calendar();
    Calendar *calendar3 = new Calendar();
    Calendar *calendar4 = new Calendar();

    ResourceGroup *group1 = new ResourceGroup();
    Resource *resource1 = new Resource();

    m_project->addCalendar(calendar1);
    m_project->addCalendar(calendar2, calendar1);
    m_project->addCalendar(calendar3);
    m_project->addCalendar(calendar4);
    m_project->setDefaultCalendar(calendar3);
    m_project->addResourceGroup(group1);
    m_project->addResource(resource1);
    group1->addResource(-1, resource1, nullptr);
    resource1->setCalendar(calendar4);
    QVERIFY(m_project->calendarAt(0) == calendar1);
    QVERIFY(calendar1->childAt(0) == calendar2);
    QVERIFY(m_project->calendarAt(1) == calendar3);
    QVERIFY(m_project->calendarAt(2) == calendar4);
    QVERIFY(m_project->resourceGroupAt(0) == group1);
    QVERIFY(group1->resourceAt(0) == resource1);
    QVERIFY(m_project->defaultCalendar() == calendar3);
    QVERIFY(resource1->calendar(true) == calendar4);

    CalendarRemoveCmd *cmd1 = new CalendarRemoveCmd(m_project, calendar1); //calendar with one child calendar
    CalendarRemoveCmd *cmd2 = new CalendarRemoveCmd(m_project, calendar2); //simple calendar
    CalendarRemoveCmd *cmd3 = new CalendarRemoveCmd(m_project, calendar3); //default project calendar
    CalendarRemoveCmd *cmd4 = new CalendarRemoveCmd(m_project, calendar4); //calendar used by a resource

    cmd3->execute();
    QVERIFY(!m_project->calendars().contains(calendar3)); //NOLINT(clang-analyzer-cplusplus.NewDeleteLeaks)
    QVERIFY(m_project->defaultCalendar() != calendar3);

    cmd4->execute();
    QVERIFY(!m_project->calendars().contains(calendar4));
    QVERIFY(resource1->calendar(true) != calendar4);

    cmd2->execute();
    QVERIFY(!calendar1->calendars().contains(calendar2));

    cmd2->unexecute();
    QVERIFY(calendar1->calendars().contains(calendar2));

    cmd1->execute();
    QVERIFY(!m_project->calendars().contains(calendar1));

    cmd1->unexecute();
    QVERIFY(m_project->calendars().contains(calendar1));
    QVERIFY(calendar1->calendars().contains(calendar2));

    cmd3->unexecute();
    QVERIFY(m_project->calendars().contains(calendar3));
    QVERIFY(m_project->defaultCalendar() == calendar3);

    cmd4->unexecute();
    QVERIFY(m_project->calendars().contains(calendar4));
    QVERIFY(resource1->calendar(true) == calendar4);

    m_project->takeCalendar(calendar4);
    m_project->takeCalendar(calendar3);
    m_project->takeCalendar(calendar2);
    m_project->takeCalendar(calendar1);
    m_project->takeResource(resource1);
    m_project->takeResourceGroup(group1);
    delete cmd1;
    delete cmd2;
    delete cmd3;
    delete cmd4;
    delete calendar4;
    delete calendar3;
    delete calendar2;
    delete calendar1;
    delete resource1;
    delete group1;
}

void CommandsTester::testCalendarMoveCmd()
{
    Calendar *calendar1 = new Calendar();
    Calendar *calendar2 = new Calendar();
    m_project->addCalendar(calendar1);
    m_project->addCalendar(calendar2);
    QVERIFY(m_project->calendarCount() == 2);

    CalendarMoveCmd *cmd1 = new CalendarMoveCmd(m_project, calendar1, 0, calendar2);
    cmd1->execute();
    QVERIFY(calendar2->childAt(0) == calendar1);
    cmd1->unexecute();
    QVERIFY(!calendar2->childCount());

    delete cmd1;
    m_project->takeCalendar(calendar1);
    m_project->takeCalendar(calendar2);
    delete calendar1;
    delete calendar2;
}

void CommandsTester::testCalendarModifyNameCmd()
{
    Calendar *calendar1 = new Calendar(QStringLiteral("test1"));
    m_project->addCalendar(calendar1);
    QVERIFY(m_project->calendars().contains(calendar1));
    QVERIFY(calendar1->name() == QStringLiteral("test1"));

    CalendarModifyNameCmd *cmd1 = new CalendarModifyNameCmd(calendar1, QStringLiteral("test2"));
    cmd1->execute();
    QVERIFY(calendar1->name() == QStringLiteral("test2"));
    cmd1->unexecute();
    QVERIFY(calendar1->name() == QStringLiteral("test1"));

    m_project->takeCalendar(calendar1);

    delete cmd1;
    delete calendar1;
}

void CommandsTester::testCalendarModifyParentCmd()
{
    Calendar *calendar1 = new Calendar();
    calendar1->setTimeZone(QTimeZone("Europe/Berlin"));
    Calendar *calendar2 = new Calendar();
    calendar2->setTimeZone(QTimeZone("Africa/Cairo"));
    m_project->addCalendar(calendar1);
    m_project->addCalendar(calendar2);
    QVERIFY(m_project->calendarCount() == 2);
    QVERIFY(calendar1->timeZone().id() == "Europe/Berlin");
    QVERIFY(calendar2->timeZone().id() == "Africa/Cairo");

    CalendarModifyParentCmd *cmd1 = new CalendarModifyParentCmd(m_project, calendar1, calendar2);
    cmd1->execute();
    QVERIFY(calendar2->childAt(0) == calendar1);
    QVERIFY(calendar1->timeZone().id() == "Africa/Cairo");
    cmd1->unexecute();
    QVERIFY(!calendar2->childCount());
    QVERIFY(calendar1->timeZone().id() == "Europe/Berlin");

    delete cmd1;
    m_project->takeCalendar(calendar1);
    m_project->takeCalendar(calendar2);
    delete calendar1;
    delete calendar2;
}

void CommandsTester::testCalendarModifyTimeZoneCmd()
{
    Calendar *calendar1 = new Calendar();
    calendar1->setTimeZone(QTimeZone("Europe/Berlin"));
    m_project->addCalendar(calendar1);
    QVERIFY(m_project->calendarCount() == 1);
    QVERIFY(calendar1->timeZone().id() == "Europe/Berlin");

    CalendarModifyTimeZoneCmd *cmd1 = new CalendarModifyTimeZoneCmd(calendar1, QTimeZone("Africa/Cairo"));
    cmd1->execute();
    QVERIFY(calendar1->timeZone().id() == "Africa/Cairo");
    cmd1->unexecute();
    QVERIFY(calendar1->timeZone().id() == "Europe/Berlin");

    delete cmd1;
    m_project->takeCalendar(calendar1);
    delete calendar1;
}

void CommandsTester::testCalendarAddDayCmd()
{
    Calendar *calendar1 = new Calendar();
    CalendarDay *day1 = new CalendarDay();
    m_project->addCalendar(calendar1);
    QVERIFY(m_project->calendarCount() == 1); //NOLINT(clang-analyzer-cplusplus.NewDeleteLeaks)
    QVERIFY(calendar1->days().isEmpty());

    CalendarAddDayCmd *cmd1 = new CalendarAddDayCmd(calendar1, day1);
    cmd1->execute();
    QVERIFY(calendar1->days().contains(day1));
    cmd1->unexecute();
    QVERIFY(calendar1->days().isEmpty());

    delete cmd1; //also deletes day1
    m_project->takeCalendar(calendar1);
    delete calendar1;
}

void CommandsTester::testCalendarRemoveDayCmd()
{
    Calendar *calendar1 = new Calendar();
    CalendarDay *day1 = new CalendarDay(QDate(1974,12,19));
    calendar1->addDay(day1);
    m_project->addCalendar(calendar1);
    QVERIFY(m_project->calendarCount() == 1);
    QVERIFY(calendar1->days().contains(day1));

    CalendarRemoveDayCmd *cmd1 = new CalendarRemoveDayCmd(calendar1, day1);
    cmd1->execute();
    QVERIFY(calendar1->days().isEmpty());
    cmd1->unexecute();
    QVERIFY(calendar1->days().contains(day1));

    CalendarRemoveDayCmd *cmd2 = new CalendarRemoveDayCmd(calendar1, QDate(1974, 12, 19));
    cmd2->execute();
    QVERIFY(calendar1->days().isEmpty());
    cmd2->unexecute();
    QVERIFY(calendar1->days().contains(day1));

    calendar1->takeDay(day1);
    delete cmd1;
    delete cmd2;
    delete day1;
    m_project->takeCalendar(calendar1);
    delete calendar1;
}

void CommandsTester::testCalendarModifyDayCmd()
{
    Calendar *calendar1 = new Calendar();
    CalendarDay *day1 = new CalendarDay(QDate(1974,12,19));
    CalendarDay *day2 = new CalendarDay(QDate(2011,03,11));
    calendar1->addDay(day1);
    m_project->addCalendar(calendar1);
    QVERIFY(m_project->calendarCount() == 1); //NOLINT(clang-analyzer-cplusplus.NewDeleteLeaks)
    QVERIFY(calendar1->days().contains(day1));

    CalendarModifyDayCmd *cmd1 = new CalendarModifyDayCmd(calendar1, day2);
    cmd1->execute();
    QVERIFY(calendar1->days().contains(day2));
    cmd1->unexecute();
    QVERIFY(calendar1->days().contains(day1));
    QVERIFY(!calendar1->days().contains(day2));

    calendar1->takeDay(day1);
    delete cmd1; //also deletes day2
    delete day1;
    m_project->takeCalendar(calendar1);
    delete calendar1;
}

void CommandsTester::testCalendarModifyStateCmd()
{
    Calendar *calendar1 = new Calendar();
    CalendarDay *day1 = new CalendarDay(QDate(1974,12,19));
    day1->setState(CalendarDay::Working);
    calendar1->addDay(day1);
    CalendarDay *day2 = new CalendarDay(QDate(2011,03,26));
    TimeInterval *interval1 = new TimeInterval(QTime(8,0), 8);
    day2->addInterval(interval1);
    day2->setState(CalendarDay::Working);
    calendar1->addDay(day2);
    m_project->addCalendar(calendar1);
    QVERIFY(m_project->calendarCount() == 1);
    QVERIFY(calendar1->days().contains(day1));
    QVERIFY(day1->state() == CalendarDay::Working);
    QVERIFY(calendar1->days().contains(day2));
    QVERIFY(day2->timeIntervals().contains(interval1));

    CalendarModifyStateCmd *cmd1 = new CalendarModifyStateCmd(calendar1, day1, CalendarDay::NonWorking);
    cmd1->execute();
    QVERIFY(day1->state() == CalendarDay::NonWorking);
    cmd1->unexecute();
    QVERIFY(day1->state() == CalendarDay::Working);

    CalendarModifyStateCmd *cmd2 = new CalendarModifyStateCmd(calendar1, day2, CalendarDay::NonWorking);
    cmd2->execute();
    QVERIFY(day2->state() == CalendarDay::NonWorking);
    QVERIFY(!day2->timeIntervals().count());
    cmd2->unexecute();
    QVERIFY(day2->timeIntervals().contains(interval1));
    QVERIFY(day2->state() == CalendarDay::Working);

    calendar1->takeDay(day1);
    calendar1->takeDay(day2);
    day2->clearIntervals();
    m_project->takeCalendar(calendar1);
    delete cmd1;
    delete cmd2;
    delete interval1;
    delete day1;
    delete day2;
    delete calendar1;
}

void CommandsTester::testCalendarModifyTimeIntervalCmd()
{
    Calendar *calendar1 = new Calendar();
    CalendarDay *day1 = new CalendarDay(QDate(1974,12,19));
    TimeInterval *interval1 = new TimeInterval(QTime(8,0), 3600000);
    TimeInterval *interval2 = new TimeInterval(QTime(12,0),7200000);
    day1->addInterval(interval1);
    calendar1->addDay(day1);
    m_project->addCalendar(calendar1);
    QVERIFY(m_project->calendarCount() == 1);
    QVERIFY(calendar1->days().contains(day1));
    QVERIFY(day1->timeIntervals().contains(interval1));
    QVERIFY(interval1->startTime() == QTime(8,0));
    QVERIFY(interval1->hours() == 1);

    CalendarModifyTimeIntervalCmd *cmd1 = new CalendarModifyTimeIntervalCmd(calendar1, *interval2, interval1);
    cmd1->execute();
    QVERIFY(interval1->startTime() == QTime(12,0));
    QVERIFY(interval1->hours() == 2);
    cmd1->unexecute();
    QVERIFY(interval1->startTime() == QTime(8,0));
    QVERIFY(interval1->hours() == 1);

    day1->clearIntervals();
    calendar1->takeDay(day1);
    m_project->takeCalendar(calendar1);
    delete cmd1;
    delete interval1;
    delete interval2;
    delete day1;
    delete calendar1;
}

void CommandsTester::testCalendarAddTimeIntervalCmd()
{
    Calendar *calendar1 = new Calendar();
    CalendarDay *day1 = new CalendarDay(QDate(1974,12,19));
    TimeInterval *interval1 = new TimeInterval(QTime(8,0), 3600000);
    calendar1->addDay(day1);
    m_project->addCalendar(calendar1);
    QVERIFY(m_project->calendarCount() == 1);
    QVERIFY(calendar1->days().contains(day1));

    CalendarAddTimeIntervalCmd *cmd1 = new CalendarAddTimeIntervalCmd(calendar1, day1, interval1);
    cmd1->execute();
    QVERIFY(day1->timeIntervals().contains(interval1));
    cmd1->unexecute();
    QVERIFY(!day1->timeIntervals().contains(interval1));

    calendar1->takeDay(day1);
    m_project->takeCalendar(calendar1);
    delete cmd1; //also delete interval
    delete day1;
    delete calendar1;
}

void CommandsTester::testCalendarRemoveTimeIntervalCmd()
{
    Calendar *calendar1 = new Calendar();
    CalendarDay *day1 = new CalendarDay(QDate(1974,12,19));
    TimeInterval *interval1 = new TimeInterval(QTime(8,0), 3600000);
    calendar1->addDay(day1);
    calendar1->addWorkInterval(day1, interval1);
    m_project->addCalendar(calendar1);
    QVERIFY(m_project->calendarCount() == 1);
    QVERIFY(calendar1->days().contains(day1));
    QVERIFY(day1->timeIntervals().contains(interval1));

    CalendarRemoveTimeIntervalCmd *cmd1 = new CalendarRemoveTimeIntervalCmd(calendar1, day1, interval1);
    cmd1->execute();
    QVERIFY(!day1->timeIntervals().contains(interval1));
    cmd1->unexecute();
    QVERIFY(day1->timeIntervals().contains(interval1));

    day1->clearIntervals();
    calendar1->takeDay(day1);
    m_project->takeCalendar(calendar1);
    delete cmd1;
    delete interval1;
    delete day1;
    delete calendar1;
}

void CommandsTester::testCalendarModifyWeekdayCmd()
{
    Calendar *calendar1 = new Calendar();
    CalendarDay *day1 = new CalendarDay(QDate(1974,12,19));
    TimeInterval *interval1 = new TimeInterval(QTime(8,0), 3600000);
    day1->setState(CalendarDay::Working);
    day1->addInterval(interval1);
    CalendarDay *day2 = new CalendarDay(QDate(2011,3,26));
    TimeInterval *interval2 = new TimeInterval(QTime(12,0), 7200000);
    day2->setState(CalendarDay::NonWorking);
    day2->addInterval(interval2);
    m_project->addCalendar(calendar1);
    calendar1->setWeekday(1, *day1);
    QVERIFY(m_project->calendarCount() == 1);
    QVERIFY(calendar1->weekday(1)->state() == day1->state());
    QVERIFY(calendar1->weekday(1)->timeIntervals().first()->startTime() == day1->timeIntervals().constFirst()->startTime());
    QVERIFY(calendar1->weekday(1)->timeIntervals().first()->hours() == day1->timeIntervals().constFirst()->hours());

    CalendarModifyWeekdayCmd *cmd1 = new CalendarModifyWeekdayCmd(calendar1, 1, day2);
    cmd1->execute();
    QVERIFY(calendar1->weekday(1)->state() == day2->state());
    QVERIFY(calendar1->weekday(1)->timeIntervals().first()->startTime() == day2->timeIntervals().constFirst()->startTime());
    QVERIFY(calendar1->weekday(1)->timeIntervals().first()->hours() == day2->timeIntervals().constFirst()->hours());
    cmd1->unexecute();
    QVERIFY(calendar1->weekday(1)->state() == day1->state());
    QVERIFY(calendar1->weekday(1)->timeIntervals().first()->startTime() == day1->timeIntervals().constFirst()->startTime());
    QVERIFY(calendar1->weekday(1)->timeIntervals().first()->hours() == day1->timeIntervals().constFirst()->hours());

    m_project->takeCalendar(calendar1);
    day1->clearIntervals();
    day2->clearIntervals();
    delete cmd1; //also deletes day2
    delete day1;
    delete interval1;
    delete interval2;
    delete calendar1;
}

void CommandsTester::testCalendarModifyDateCmd()
{
    Calendar *calendar1 = new Calendar();
    CalendarDay *day1 = new CalendarDay(QDate(1974,12,19));
    calendar1->addDay(day1);
    m_project->addCalendar(calendar1);
    QVERIFY(m_project->calendarCount() == 1);
    QVERIFY(calendar1->days().contains(day1));
    QVERIFY(day1->date() == QDate(1974,12,19));

    CalendarModifyDateCmd *cmd1 = new CalendarModifyDateCmd(calendar1, day1, QDate(2011,3,26));
    cmd1->execute();
    QVERIFY(day1->date() == QDate(2011,3,26));
    cmd1->unexecute();
    QVERIFY(day1->date() == QDate(1974,12,19));

    calendar1->takeDay(day1);
    m_project->takeCalendar(calendar1);
    delete cmd1;
    delete day1;
    delete calendar1;
}

void CommandsTester::testProjectModifyDefaultCalendarCmd()
{
    Calendar *calendar1 = new Calendar();
    Calendar *calendar2 = new Calendar();
    m_project->addCalendar(calendar1);
    m_project->addCalendar(calendar2);
    m_project->setDefaultCalendar(calendar1);
    QVERIFY(m_project->calendars().contains(calendar1));
    QVERIFY(m_project->calendars().contains(calendar2));
    QVERIFY(m_project->defaultCalendar() == calendar1);

    ProjectModifyDefaultCalendarCmd *cmd1 = new ProjectModifyDefaultCalendarCmd(m_project, calendar2);
    cmd1->execute();
    QVERIFY(m_project->defaultCalendar() == calendar2);
    cmd1->unexecute();
    QVERIFY(m_project->defaultCalendar() == calendar1);

    m_project->takeCalendar(calendar1);
    m_project->takeCalendar(calendar2);
    delete calendar1;
    delete calendar2;
    delete cmd1;
}
} // namespace KPlato

QTEST_GUILESS_MAIN(KPlato::CommandsTester)
