/* This file is part of the KDE project
   SPDX-FileCopyrightText: 20012 Dag Andersen <dag.andersen@kdemail.net>
   SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
   
   SPDX-License-Identifier: LGPL-2.0-or-later
*/
// clazy:excludeall=qstring-arg
#include "WorkInfoCacheTester.h"
#include "DateTimeTester.h"

#include "kptresource.h"
#include "kptcalendar.h"
#include "kptdatetime.h"
#include "kptglobal.h"

#include <QTest>
#include <QDir>

#include <cstdlib>

#include "debug.cpp"

namespace KPlato
{

QTimeZone createTimeZoneWithOffsetFromSystem(int hours, const QString & name, int *shiftDays)
{
    QTimeZone systemTimeZone = QTimeZone::systemTimeZone();
    int systemOffsetSeconds = systemTimeZone.standardTimeOffset(QDateTime(QDate(1980, 1, 1), QTime(), Qt::UTC));
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

void WorkInfoCacheTester::basics()
{
    Calendar cal(QStringLiteral("Test"));
    QDate wdate(2012,1,2);
    DateTime before = DateTime(wdate.addDays(-1), QTime());
    DateTime after = DateTime(wdate.addDays(1), QTime());
    QTime t1(8,0,0);
    QTime t2(10,0,0);
    DateTime wdt1(wdate, t1);
    DateTime wdt2(wdate, t2);
    int length = t1.msecsTo(t2);
    CalendarDay *day = new CalendarDay(wdate, CalendarDay::Working);
    day->addInterval(TimeInterval(t1, length));
    cal.addDay(day);
    QVERIFY(cal.findDay(wdate) == day);

    Resource r;
    r.setCalendar(&cal);
    const Resource::WorkInfoCache &wic = r.workInfoCache();
    QVERIFY(! wic.isValid());

    r.calendarIntervals(before, after);
    qDebug()<<wic.intervals.map();
    QCOMPARE(wic.intervals.map().count(), 1);

    wdt1 = wdt1.addDays(1);
    wdt2 = wdt2.addDays(1);
    day = new CalendarDay(wdt1.date(), CalendarDay::Working);
    day->addInterval(TimeInterval(t1, length));
    cal.addDay(day);

    r.calendarIntervals(before, after);
    QCOMPARE(wic.intervals.map().count(), 1);

    after = after.addDays(1);
    r.calendarIntervals(wdt1, after);
    
    qDebug()<<wic.intervals.map();
    QCOMPARE(wic.intervals.map().count(), 2);
}

void WorkInfoCacheTester::addAfter()
{
    Calendar cal(QStringLiteral("Test"));
    QDate wdate(2012,1,2);
    DateTime before = DateTime(wdate.addDays(-1), QTime());
    DateTime after = DateTime(wdate.addDays(1), QTime());
    QTime t1(8,0,0);
    QTime t2(10,0,0);
    QTime t3(12,0,0);
    QTime t4(14,0,0);
    DateTime wdt1(wdate, t1);
    DateTime wdt2(wdate, t2);
    int length = t1.msecsTo(t2);
    CalendarDay *day = new CalendarDay(wdate, CalendarDay::Working);
    day->addInterval(TimeInterval(t1, length));
    length = t3.msecsTo(t4);
    day->addInterval(TimeInterval(t3, length));
    cal.addDay(day);
    QVERIFY(cal.findDay(wdate) == day);

    Resource r;
    r.setCalendar(&cal);
    const Resource::WorkInfoCache &wic = r.workInfoCache();
    QVERIFY(! wic.isValid());

    r.calendarIntervals(before, after);
    qDebug()<<wic.intervals.map();
    QCOMPARE(wic.intervals.map().count(), 2);

    wdt1 = wdt1.addDays(1);
    wdt2 = wdt2.addDays(1);
    day = new CalendarDay(wdt1.date(), CalendarDay::Working);
    day->addInterval(TimeInterval(t1, length));
    cal.addDay(day);

    // wdate: 8-10, 12-14
    // wdate+1: 8-10
    r.calendarIntervals(DateTime(wdate, t1), DateTime(wdate, t2));
    QCOMPARE(wic.intervals.map().count(), 1);

    r.calendarIntervals(DateTime(wdate, t3), DateTime(wdate, t4));
    QCOMPARE(wic.intervals.map().count(), 2);

    r.calendarIntervals(DateTime(wdate.addDays(1), t1), DateTime(wdate.addDays(1), t2));
    qDebug()<<wic.intervals.map();
    QCOMPARE(wic.intervals.map().count(), 3);
}

void WorkInfoCacheTester::addBefore()
{
    Calendar cal(QStringLiteral("Test"));
    QDate wdate(2012,1,2);
    DateTime before = DateTime(wdate.addDays(-1), QTime());
    DateTime after = DateTime(wdate.addDays(1), QTime());
    QTime t1(8,0,0);
    QTime t2(10,0,0);
    QTime t3(12,0,0);
    QTime t4(14,0,0);
    DateTime wdt1(wdate, t1);
    DateTime wdt2(wdate, t2);
    int length = t1.msecsTo(t2);
    CalendarDay *day = new CalendarDay(wdate, CalendarDay::Working);
    day->addInterval(TimeInterval(t1, length));
    length = t3.msecsTo(t4);
    day->addInterval(TimeInterval(t3, length));
    cal.addDay(day);
    QVERIFY(cal.findDay(wdate) == day);

    Resource r;
    r.setCalendar(&cal);
    const Resource::WorkInfoCache &wic = r.workInfoCache();
    QVERIFY(! wic.isValid());

    r.calendarIntervals(before, after);
    qDebug()<<wic.intervals.map();
    QCOMPARE(wic.intervals.map().count(), 2);

    wdt1 = wdt1.addDays(1);
    wdt2 = wdt2.addDays(1);
    day = new CalendarDay(wdt1.date(), CalendarDay::Working);
    day->addInterval(TimeInterval(t1, length));
    cal.addDay(day);

    // wdate: 8-10, 12-14
    // wdate+1: 8-10
    r.calendarIntervals(DateTime(wdate.addDays(1), t1), DateTime(wdate.addDays(1), t2));
    qDebug()<<wic.intervals.map();
    QCOMPARE(wic.intervals.map().count(), 1);

    r.calendarIntervals(DateTime(wdate, t3), DateTime(wdate, t4));
    QCOMPARE(wic.intervals.map().count(), 2);

    r.calendarIntervals(DateTime(wdate, t1), DateTime(wdate, t2));
    QCOMPARE(wic.intervals.map().count(), 3);
}

void WorkInfoCacheTester::addMiddle()
{
    Calendar cal(QStringLiteral("Test"));
    QDate wdate(2012,1,2);
    DateTime before = DateTime(wdate.addDays(-1), QTime());
    DateTime after = DateTime(wdate.addDays(1), QTime());
    QTime t1(8,0,0);
    QTime t2(10,0,0);
    QTime t3(12,0,0);
    QTime t4(14,0,0);
    DateTime wdt1(wdate, t1);
    DateTime wdt2(wdate, t2);
    int length = t1.msecsTo(t2);
    CalendarDay *day = new CalendarDay(wdate, CalendarDay::Working);
    day->addInterval(TimeInterval(t1, length));
    length = t3.msecsTo(t4);
    day->addInterval(TimeInterval(t3, length));
    cal.addDay(day);
    QVERIFY(cal.findDay(wdate) == day);

    Resource r;
    r.setCalendar(&cal);
    const Resource::WorkInfoCache &wic = r.workInfoCache();
    QVERIFY(! wic.isValid());

    r.calendarIntervals(before, after);
    qDebug()<<wic.intervals.map();
    QCOMPARE(wic.intervals.map().count(), 2);

    wdt1 = wdt1.addDays(1);
    wdt2 = wdt2.addDays(1);
    day = new CalendarDay(wdt1.date(), CalendarDay::Working);
    day->addInterval(TimeInterval(t1, length));
    cal.addDay(day);

    // wdate: 8-10, 12-14
    // wdate+1: 8-10
    r.calendarIntervals(DateTime(wdate.addDays(1), t1), DateTime(wdate.addDays(1), t2));
    qDebug()<<wic.intervals.map();
    QCOMPARE(wic.intervals.map().count(), 1);

    // the middle interval will be filled in automatically
    r.calendarIntervals(DateTime(wdate, t1), DateTime(wdate, t2));
    QCOMPARE(wic.intervals.map().count(), 3);
}

void WorkInfoCacheTester::fullDay()
{
    Calendar cal(QStringLiteral("Test"));
    QDate wdate(2012,1,2);

    QTime t1(0,0,0);
    DateTime wdt1(wdate, t1);
    DateTime wdt2(wdate.addDays(1), t1);
    long length = (wdt2 - wdt1).milliseconds();
    CalendarDay *day = new CalendarDay(wdate, CalendarDay::Working);
    day->addInterval(TimeInterval(t1, length));
    cal.addDay(day);
    QVERIFY(cal.findDay(wdate) == day);

    Resource r;
    r.setCalendar(&cal);
    const Resource::WorkInfoCache &wic = r.workInfoCache();
    QVERIFY(! wic.isValid());

    r.calendarIntervals(wdt1, wdt2);
    qDebug()<<wic.intervals.map();
    QCOMPARE(wic.intervals.map().count(), 1);

    day = new CalendarDay(wdt2.date(), CalendarDay::Working);
    day->addInterval(TimeInterval(t1, length));
    cal.addDay(day);

    r.calendarIntervals(wdt1, DateTime(wdt2.addDays(2)));
    qDebug()<<wic.intervals.map();
    QCOMPARE(wic.intervals.map().count(), 2);
}

void WorkInfoCacheTester::timeZone()
{
    //qputenv("TZ", QByteArray("America/Los_Angeles"));
    qDebug()<<"Local timezone: "<<QTimeZone::systemTimeZone();
    
    Calendar cal(QStringLiteral("Test"));
    // Create a dummy timezone 9 hours from system timezone
    int laShiftDays;
    QTimeZone dummytz = createTimeZoneWithOffsetFromSystem(-9, QStringLiteral("DummyTimeZone"), &laShiftDays);
    QVERIFY(dummytz.isValid());
    cal.setTimeZone(dummytz);

    QDate wdate(2012,1,2);
    DateTime before = DateTime(wdate.addDays(-1), QTime(), dummytz);
    DateTime after = DateTime(wdate.addDays(2), QTime(), dummytz);
//    qDebug() << "before, after" << before << after;
    QTime t1(14,0,0); // 23 dummytz
    QTime t2(16,0,0); // 01 dummytz next day
    DateTime wdt1(wdate, t1);
    DateTime wdt2(wdate, t2);
    qint64 length = t1.msecsTo(t2);
    qDebug() << "length:" << Duration(length).toString() << "wdt1, wdt2" << wdt1 << wdt2 << wdt1.toTimeZone(dummytz) << wdt2.toTimeZone(dummytz);
    CalendarDay *day = new CalendarDay(wdate, CalendarDay::Working);
    day->addInterval(TimeInterval(t1, length));
    cal.addDay(day);
    QVERIFY(cal.findDay(wdate) == day);

    Debug::print(&cal, QStringLiteral("DummyTimeZone"));
    Resource r;
    r.setCalendar(&cal);
    const Resource::WorkInfoCache &wic = r.workInfoCache();
    QVERIFY(! wic.isValid());

    r.calendarIntervals(before, after);
    QVERIFY(wic.isValid());

    QCOMPARE(wic.intervals.map().count(), 1);
    QCOMPARE(wic.intervals.map().values(wdate).count(), 1);
    QMultiMap<QDate, AppointmentInterval>::const_iterator it = wic.intervals.map().constBegin();
    QCOMPARE(it.value().startTime(), DateTime(wdate, QTime(14, 0, 0), dummytz));
    QCOMPARE(it.value().endTime(), DateTime(wdate, QTime(16, 0, 0), dummytz));

    // convert to localtime, moves the interval to around midnite (splits it into two)
    auto localIntervals = wic.intervals;
    qDebug()<<"before"<<localIntervals;
    localIntervals.toTimeZone(QTimeZone::systemTimeZone());
    qDebug()<<"after"<<localIntervals;
    wdate = wdate.addDays(laShiftDays);
    QCOMPARE(localIntervals.map().count(), 2);
    QCOMPARE(localIntervals.map().values(wdate).count(), 1);
    QCOMPARE(localIntervals.map().value(wdate).startTime(), DateTime(wdate, QTime(23, 0, 0), QTimeZone::systemTimeZone()));
    QCOMPARE(localIntervals.map().value(wdate).endTime(), DateTime(wdate.addDays(1), QTime(0, 0, 0), QTimeZone::systemTimeZone()));

    wdate = wdate.addDays(1);
    QCOMPARE(localIntervals.map().values(wdate).count(), 1);
    QCOMPARE(localIntervals.map().value(wdate).startTime(), DateTime(wdate, QTime(0, 0, 0)));
    QCOMPARE(localIntervals.map().value(wdate).endTime(), DateTime(wdate, QTime(1, 0, 0)));
    //qunsetenv("TZ");
}

void WorkInfoCacheTester::doubleTimeZones()
{
    QTimeZone tz("Europe/Copenhagen");
    qDebug()<<"Local timezone: "<<QTimeZone::systemTimeZone()<<"tz:"<<tz;
    
    Calendar cal(QStringLiteral("LocalTime/Copenhagen"));
    cal.setTimeZone(tz);
    QCOMPARE(cal.timeZone(),  tz);

    QTimeZone htz("Europe/Helsinki");
    Calendar cal2(QStringLiteral("Helsinki"));
    cal2.setTimeZone(htz);
    QVERIFY(cal2.timeZone().isValid());
    
    QDate wdate(2012,1,2);
    DateTime before = DateTime(wdate, QTime(), tz);
    DateTime after = DateTime(wdate.addDays(1), QTime(), tz);
    //    qDebug() << "before, after" << before << after;
    QTime t1(14,0,0);
    QTime t2(16,0,0);
    DateTime wdt1(wdate, t1);
    DateTime wdt2(wdate, t2);
    int length = t1.msecsTo(t2);

    CalendarDay *day = new CalendarDay(wdate, CalendarDay::Working);
    day->addInterval(TimeInterval(t1, length));
    cal.addDay(day);
    QVERIFY(cal.findDay(wdate) == day);
    
    Debug::print(&cal, QString());
    Resource r1;
    r1.setCalendar(&cal);
    const Resource::WorkInfoCache &wic = r1.workInfoCache();
    QVERIFY(! wic.isValid());
    
    r1.calendarIntervals(before, after);
    Debug::print(wic.intervals);
    QCOMPARE(wic.intervals.map().count(), 1);
    
    QCOMPARE(wic.intervals.map().value(wdate).startTime(), DateTime(wdate, QTime(14, 0, 0), tz));
    QCOMPARE(wic.intervals.map().value(wdate).endTime(), DateTime(wdate, QTime(16, 0, 0), tz));

    day = new CalendarDay(wdate, CalendarDay::Working);
    day->addInterval(TimeInterval(t1, length));
    cal2.addDay(day);
    QVERIFY(cal2.findDay(wdate) == day);
    
    Debug::print(&cal2, QString());
    Resource r2;
    r2.setCalendar(&cal2);
    const Resource::WorkInfoCache &wic2 = r2.workInfoCache();
    QVERIFY(! wic2.isValid());
    
    r2.calendarIntervals(before, after);
    Debug::print(wic2.intervals);
    QCOMPARE(wic2.intervals.map().count(), 1);
    
    QCOMPARE(wic2.intervals.map().value(wdate).startTime(), DateTime(wdate, QTime(13, 0, 0), tz));
    QCOMPARE(wic2.intervals.map().value(wdate).endTime(), DateTime(wdate, QTime(15, 0, 0), tz));
}

} //namespace KPlato

QTEST_GUILESS_MAIN(KPlato::WorkInfoCacheTester)
