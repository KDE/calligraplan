/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2006-2007 Dag Andersen <dag.andersen@kdemail.net>
   SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
   
   SPDX-License-Identifier: LGPL-2.0-or-later
*/
// clazy:excludeall=qstring-arg
#include "CalendarTester.h"
#include "DateTimeTester.h"
#include <kptcalendar.h>
#include <kptdatetime.h>
#include <kptduration.h>
#include <kptmap.h>
#include "kptappointment.h"

#include <QTimeZone>
#include <QDateTime>

#include "debug.cpp"


namespace KPlato
{

QTimeZone createTimeZoneWithOffsetFromSystem(int hours, const QString & name, int *shiftDays, const QTimeZone &tz)
{
    int systemOffsetSeconds = tz.standardTimeOffset(QDateTime(QDate(1980, 1, 1), QTime(), Qt::UTC));
    int offsetSeconds = systemOffsetSeconds + 3600 * hours;
    if (offsetSeconds >= (12*3600)) {
        qDebug() << "reducing offset by 24h";
        offsetSeconds -= (24*3600);
        *shiftDays = 1;
    } else if (offsetSeconds <= -(12*3600)) {
        qDebug() << "increasing offset by 24h";
        offsetSeconds += (24*3600);
        *shiftDays = -1;
    } else {
        *shiftDays = 0;
    }
    qDebug() << "creating timezone for offset" << hours << offsetSeconds << "systemoffset" << systemOffsetSeconds
             << "shiftDays" << *shiftDays;
    return QTimeZone(name.toLatin1(), offsetSeconds, name, name);
}

void CalendarTester::testSingleDay() {
    Calendar t(QStringLiteral("Test"));
    QDate wdate(2006,1,2);
    DateTime before = DateTime(wdate.addDays(-1), QTime());
    DateTime after = DateTime(wdate.addDays(1), QTime());
    QTime t1(8,0,0);
    QTime t2(10,0,0);
    DateTime wdt1(wdate, t1);
    DateTime wdt2(wdate, t2);
    int length = t1.msecsTo(t2);
    CalendarDay *day = new CalendarDay(wdate, CalendarDay::Working);
    day->addInterval(TimeInterval(t1, length));
    t.addDay(day);
    QVERIFY(t.findDay(wdate) == day);

    QVERIFY(t.hasInterval(after, DateTime(after.addDays(1))) == false);
    QVERIFY(t.hasInterval(before, DateTime(before.addDays(-1))) == false);

    QVERIFY(t.hasInterval(after, before) == false);
    QVERIFY(t.hasInterval(before, after));

    QVERIFY((t.firstAvailableAfter(after, DateTime(after.addDays(10)))).isValid() == false);
    QVERIFY((t.firstAvailableBefore(before, DateTime(before.addDays(-10)))).isValid() == false);

    QCOMPARE(t.firstAvailableAfter(before,after), wdt1);
    QCOMPARE(t.firstAvailableBefore(after, before), wdt2);

    Duration e(0, 2, 0);
    QCOMPARE((t.effort(before, after)), e);
}

void CalendarTester::testWeekdays() {
    Calendar t(QStringLiteral("Test"));
    QDate wdate(2006,1,4); // wednesday
    DateTime before = DateTime(wdate.addDays(-2), QTime());
    DateTime after = DateTime(wdate.addDays(2), QTime());
    QTime t1(8,0,0);
    QTime t2(10,0,0);
    int length = t1.msecsTo(t2);

    CalendarDay *wd1 = t.weekday(Qt::Wednesday);
    QVERIFY(wd1 != nullptr);

    wd1->setState(CalendarDay::Working);
    wd1->addInterval(TimeInterval(t1, length));

    QCOMPARE(t.firstAvailableAfter(before, after), DateTime(QDate(2006, 1, 4), QTime(8,0,0)));
    QCOMPARE((t.firstAvailableBefore(after, before)), DateTime(QDate(2006, 1, 4), QTime(10,0,0)));

    QCOMPARE(t.firstAvailableAfter(after, DateTime(QDate(QDate(2006,1,14)), QTime())), DateTime(QDate(2006, 1, 11), QTime(8,0,0)));
    QCOMPARE(t.firstAvailableBefore(before, DateTime(QDate(2005,12,25), QTime())), DateTime(QDate(2005, 12, 28), QTime(10,0,0)));
}

void CalendarTester::testCalendarWithParent() {
    Calendar p(QStringLiteral("Test 3 parent"));
    Calendar t(QStringLiteral("Test 3"));
    t.setParentCal(&p);
    QDate wdate(2006,1,2);
    DateTime before = DateTime(wdate.addDays(-1), QTime());
    DateTime after = DateTime(wdate.addDays(1), QTime());
    QTime t1(8,0,0);
    QTime t2(10,0,0);
    int length = t1.msecsTo(t2);
    DateTime wdt1(wdate, t1);
    DateTime wdt2(wdate, t2);

    CalendarDay *day = new CalendarDay(wdate, CalendarDay::Working);
    day->addInterval(TimeInterval(t1, length));
    p.addDay(day);
    QVERIFY(p.findDay(wdate) == day);

    //same tests as in testSingleDay()
    QVERIFY(t.hasInterval(after, DateTime(after.addDays(1))) == false);
    QVERIFY(t.hasInterval(before, DateTime(before.addDays(-1))) == false);

    QVERIFY(t.hasInterval(after, before) == false);
    QVERIFY(t.hasInterval(before, after));

    QVERIFY((t.firstAvailableAfter(after, DateTime(after.addDays(10)))).isValid() == false);
    QVERIFY((t.firstAvailableBefore(before, DateTime(before.addDays(-10)))).isValid() == false);

    QVERIFY(t.firstAvailableAfter(before, after).isValid());
    QVERIFY(t.firstAvailableBefore(after, before).isValid());

    QCOMPARE(t.firstAvailableAfter(before,after), wdt1);
    QCOMPARE(t.firstAvailableBefore(after, before), wdt2);

    Duration e(0, 2, 0);
    QCOMPARE((t.effort(before, after)), e);
}

void CalendarTester::testTimezone()
{
    Calendar t(QStringLiteral("Test"));
    t.setTimeZone(QTimeZone("Europe/Berlin"));
    QDate wdate(2006,1,2);
    DateTime before = DateTime(wdate.addDays(-1), QTime(), t.timeZone());
    DateTime after = DateTime(wdate.addDays(1), QTime(), t.timeZone());
    QTime t1(8,0,0);
    QTime t2(10,0,0);
    DateTime wdt1(wdate, t1, t.timeZone());
    DateTime wdt2(wdate, t2, t.timeZone());
    int length = t1.msecsTo(t2);
    CalendarDay *day = new CalendarDay(wdate, CalendarDay::Working);
    day->addInterval(TimeInterval(t1, length));
    t.addDay(day);
    Debug::print(&t, QStringLiteral("Time zone testing"));
    QVERIFY(t.findDay(wdate) == day);

    // local zone: Europe/Berlin (1 hours from London)
    int loShiftDays;
    QTimeZone lo = createTimeZoneWithOffsetFromSystem(-1, QStringLiteral("DummyLondon"), &loShiftDays, t.timeZone());
    QVERIFY(lo.isValid());
    QDateTime dt1 = QDateTime(wdate, t1, lo).addDays(loShiftDays).addSecs(-2 * 3600);
    QDateTime dt2 = QDateTime(wdate, t2, lo).addDays(loShiftDays).addSecs(0 * 3600);

    qDebug()<<QDateTime(wdt1)<<QDateTime(wdt2);
    qDebug()<<dt1<<dt2<<"("<<dt1.toLocalTime()<<dt2.toLocalTime()<<")";
    QCOMPARE(t.firstAvailableAfter(DateTime(dt1), after), wdt1);
    QCOMPARE(t.firstAvailableBefore(DateTime(dt2), before), wdt2);

    Duration e(0, 2, 0);
    QCOMPARE(t.effort(DateTime(dt1), DateTime(dt2)), e);

    // local zone: Europe/Berlin (9 hours from America/Los_Angeles)
    int laShiftDays;
    QTimeZone la = createTimeZoneWithOffsetFromSystem(-9, QStringLiteral("DummyLos_Angeles"), &laShiftDays, t.timeZone());
    QVERIFY(la.isValid());
    QDateTime dt3 = QDateTime(wdate, t1, la).addDays(laShiftDays).addSecs(-10 * 3600);
    QDateTime dt4 = QDateTime(wdate, t2, la).addDays(laShiftDays).addSecs(-8 * 3600);

    qDebug()<<QDateTime(wdt1)<<QDateTime(wdt2);
    qDebug()<<dt3<<dt4<<"("<<dt3.toLocalTime()<<dt4.toLocalTime()<<")";
    QCOMPARE(t.firstAvailableAfter(DateTime(dt3), after), wdt1);
    QCOMPARE(t.firstAvailableBefore(DateTime(dt4), before), wdt2);

    QCOMPARE(t.effort(DateTime(dt3), DateTime(dt4)), e);

    QString s = QStringLiteral("Test Cairo:");
    qDebug()<<s;
    // local zone: Europe/Berlin (1 hour from cairo)
    int caShiftDays;
    QTimeZone ca = createTimeZoneWithOffsetFromSystem(1, QStringLiteral("DummyCairo"), &caShiftDays, t.timeZone());
    QDateTime dt5 = QDateTime(wdate, t1, ca).addDays(caShiftDays).addSecs(0 * 3600);
    QDateTime dt6 = QDateTime(wdate, t2, ca).addDays(caShiftDays).addSecs(2 * 3600);

    qDebug()<<QDateTime(wdt1)<<QDateTime(wdt2);
    qDebug()<<dt5<<dt6<<"("<<dt5.toLocalTime()<<dt6.toLocalTime()<<")";
    QCOMPARE(t.firstAvailableAfter(DateTime(dt5), after), wdt1);
    QCOMPARE(t.firstAvailableBefore(DateTime(dt6), before), wdt2);

    QCOMPARE(t.effort(DateTime(dt5), DateTime(dt6)), e);

    qDebug()<<"Test Samoa:";
    // local zone: Europe/Berlin (10 hours from Samoa)
    int saShiftDays;
    QTimeZone sa = createTimeZoneWithOffsetFromSystem(10, QStringLiteral("DummySamoa"), &saShiftDays, t.timeZone());
    QDateTime dt7 = QDateTime(wdate, t1, ca).addDays(saShiftDays).addSecs(0 * 3600);
    QDateTime dt8 = QDateTime(wdate, t2, ca).addDays(saShiftDays).addSecs(2 * 3600);

    qDebug()<<QDateTime(wdt1)<<QDateTime(wdt2);
    qDebug()<<dt7<<dt8<<"("<<dt7.toLocalTime()<<dt8.toLocalTime()<<")";
    QCOMPARE(t.firstAvailableAfter(DateTime(dt7), after), wdt1);
    QCOMPARE(t.firstAvailableBefore(DateTime(dt8), before), wdt2);

    QCOMPARE(t.effort(DateTime(dt7), DateTime(dt8)), e);
}

void CalendarTester::workIntervals()
{
    Calendar t(QStringLiteral("Test"));
    QDate wdate(2006,1,2);
    DateTime before = DateTime(wdate.addDays(-1), QTime());
    DateTime after = DateTime(wdate.addDays(1), QTime());
    QTime t1(8,0,0);
    QTime t2(10,0,0);
    DateTime wdt1(wdate, t1);
    DateTime wdt2(wdate, t2);
    int length = t1.msecsTo(t2);
    CalendarDay *day = new CalendarDay(wdate, CalendarDay::Working);
    day->addInterval(TimeInterval(t1, length));
    t.addDay(day);
    QVERIFY(t.findDay(wdate) == day);

    AppointmentIntervalList lst = t.workIntervals(before, after, 100.);
    QCOMPARE(lst.map().count(), 1);
    QCOMPARE(wdate, lst.map().values().first().startTime().date());
    QCOMPARE(t1, lst.map().values().first().startTime().time());
    QCOMPARE(wdate, lst.map().values().first().endTime().date());
    QCOMPARE(t2, lst.map().values().first().endTime().time());
    QCOMPARE(100., lst.map().values().first().load());

    QTime t3(12, 0, 0);
    day->addInterval(TimeInterval(t3, length));

    lst = t.workIntervals(before, after, 100.);
    Debug::print(lst);
    QCOMPARE(lst.map().count(), 2);
    QCOMPARE(wdate, lst.map().values().first().startTime().date());
    QCOMPARE(t1, lst.map().values().first().startTime().time());
    QCOMPARE(wdate, lst.map().values().first().endTime().date());
    QCOMPARE(t2, lst.map().values().first().endTime().time());
    QCOMPARE(100., lst.map().values().first().load());

    QCOMPARE(wdate, lst.map().values().at(1).startTime().date());
    QCOMPARE(t3, lst.map().values().at(1).startTime().time());
    QCOMPARE(wdate, lst.map().values().at(1).endTime().date());
    QCOMPARE(t3.addMSecs(length), lst.map().values().at(1).endTime().time());
    QCOMPARE(100., lst.map().values().at(1).load());

    // add interval before the existing
    QTime t4(5, 30, 0);
    day->addInterval(TimeInterval(t4, length));

    lst = t.workIntervals(before, after, 100.);
    Debug::print(lst);
    QCOMPARE(lst.map().count(), 3);
    QCOMPARE(wdate, lst.map().values().first().startTime().date());
    QCOMPARE(t4, lst.map().values().first().startTime().time());
    QCOMPARE(wdate, lst.map().values().first().endTime().date());
    QCOMPARE(t4.addMSecs(length), lst.map().values().first().endTime().time());
    QCOMPARE(100., lst.map().values().first().load());

    QCOMPARE(wdate, lst.map().values().at(1).startTime().date());
    QCOMPARE(t1, lst.map().values().at(1).startTime().time());
    QCOMPARE(wdate, lst.map().values().at(1).endTime().date());
    QCOMPARE(t2, lst.map().values().at(1).endTime().time());
    QCOMPARE(100., lst.map().values().at(1).load());

    QCOMPARE(wdate, lst.map().values().at(2).startTime().date());
    QCOMPARE(t3, lst.map().values().at(2).startTime().time());
    QCOMPARE(wdate, lst.map().values().at(2).endTime().date());
    QCOMPARE(t3.addMSecs(length), lst.map().values().at(2).endTime().time());
    QCOMPARE(100., lst.map().values().at(2).load());
}

void CalendarTester::workIntervalsFullDays()
{
    Calendar t(QStringLiteral("Test"));
    QDate wdate(2006,1,2);
    DateTime before = DateTime(wdate.addDays(-1), QTime());
    DateTime after = DateTime(wdate.addDays(10), QTime());

    CalendarDay *day = new CalendarDay(wdate, CalendarDay::Working);
    day->addInterval(TimeInterval(QTime(0, 0, 0), 24*60*60*1000));
    t.addDay(day);

    QCOMPARE(day->numIntervals(), 1);
    QVERIFY(day->timeIntervals().constFirst()->endsMidnight());

    DateTime start = day->start();
    DateTime end = day->end();

    QCOMPARE(t.workIntervals(start, end, 100.).map().count(), 1);
    QCOMPARE(t.workIntervals(before, after, 100.).map().count(), 1);

    day = new CalendarDay(wdate.addDays(1), CalendarDay::Working);
    day->addInterval(TimeInterval(QTime(0, 0, 0), 24*60*60*1000));
    t.addDay(day);

    end = day->end();

    QCOMPARE(t.workIntervals(start, end, 100.).map().count(), 2);
    QCOMPARE(t.workIntervals(before, after, 100.).map().count(), 2);

    day = new CalendarDay(wdate.addDays(2), CalendarDay::Working);
    day->addInterval(TimeInterval(QTime(0, 0, 0), 24*60*60*1000));
    t.addDay(day);

    end = day->end();

    QCOMPARE(t.workIntervals(start, end, 100.).map().count(), 3);
    QCOMPARE(t.workIntervals(before, after, 100.).map().count(), 3);

}

void CalendarTester::dstSpring()
{
    QTimeZone tz("Europe/Copenhagen");

    Calendar t(QStringLiteral("DST"));
    t.setTimeZone(tz);

    QDate wdate(2016,3,27);

    DateTime before = DateTime(wdate.addDays(-1), QTime(), tz);
    DateTime after = DateTime(wdate.addDays(1), QTime(), tz);
    QTime t1(0,0,0);
    int length = 24*60*60*1000;
    
    CalendarDay *wd1 = t.weekday(Qt::Sunday);
    QVERIFY(wd1 != nullptr);
    
    wd1->setState(CalendarDay::Working);
    wd1->addInterval(TimeInterval(t1, length));
    AppointmentIntervalList lst = t.workIntervals(before, after, 100.);
    qDebug()<<lst;
    QCOMPARE(lst.map().count(), 1);
    QCOMPARE(lst.map().first().effort().toHours(), 23.); // clazy:exclude=detaching-temporary
    
    wd1->clearIntervals();
    qDebug()<<"clear list";
    wd1->addInterval(TimeInterval(QTime(0,0,0), Duration(2., Duration::Unit_h).milliseconds()));
    wd1->addInterval(TimeInterval(QTime(2,0,0), Duration(2., Duration::Unit_h).milliseconds()));
    
    lst = t.workIntervals(before, after, 100.);
    qDebug()<<"DST?"<<DateTime(wdate, QTime(2,0,0))<<lst;
    QCOMPARE(lst.map().count(), 1);
    AppointmentInterval ai = lst.map().values().value(0);
    QCOMPARE(ai.startTime(),  DateTime(wdate, QTime(), tz));
    QCOMPARE(ai.endTime(),  DateTime(wdate, QTime(4,0,0), tz));
    QCOMPARE(ai.effort().toHours(),  3.);
}

void CalendarTester::timeZones()
{
    QTimeZone tz("Europe/Copenhagen");

    QTimeZone tzSydney("Australia/Sydney");
    int length = 8*60*60*1000;
    Calendar sydney(QStringLiteral("Sydney"));
    sydney.setTimeZone(tzSydney);
    CalendarDay day(CalendarDay::Working);
    TimeInterval ti(QTime(8, 0, 0), length);
    day.addInterval(ti);
    for (int i = 0; i < 7; ++i) {
        sydney.setWeekday(i, day);
    }
    QDate wdate(2016,2,1);
    DateTime before = DateTime(wdate.addDays(-1), QTime(), tz);
    DateTime after = DateTime(wdate.addDays(1), QTime(), tz);
    qInfo()<<before<<after;
    auto lst = sydney.workIntervals(before, after, 100);
    Debug::print(lst);

    DateTime beforeSydney(before); beforeSydney = beforeSydney.toTimeZone(tzSydney);
    DateTime afterSydney(after); afterSydney = afterSydney.toTimeZone(tzSydney);
    lst = sydney.workIntervals(beforeSydney, afterSydney, 100);
    Debug::print(lst);

//     ai = lst.map().values().value(1);
//     QCOMPARE(ai.startTime(),  DateTime(wdate, QTime(2,0,0)));
//     QCOMPARE(ai.endTime(),  DateTime(wdate, QTime(4,0,0)));
//     QCOMPARE(ai.effort().toHours(),  1.); // Missing DST hour is skipped

    qunsetenv("TZ");
}

} //namespace KPlato

QTEST_GUILESS_MAIN(KPlato::CalendarTester)
