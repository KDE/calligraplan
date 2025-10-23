/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2006-2007 Dag Andersen <dag.andersen@kdemail.net>
   SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
   
   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "DateTimeTester.h"
#include <kptdatetime.h>
#include <kptduration.h>

#include "debug.cpp"

namespace KPlato
{

void DateTimeTester::subtractDay()
{
    DateTime dt1(QDate(2006, 1, 1), QTime(8, 0, 0, 0));
    DateTime dt2(QDate(2006, 1, 2), QTime(8, 0, 0, 0));
    Duration d(1, 0, 0, 0, 0);

    QVERIFY((dt2-dt1).toString() == d.toString());
    QVERIFY((dt1-dt2).toString() == d.toString()); // result always positive
    QVERIFY((dt2-d).toString() == dt1.toString());
}
void DateTimeTester::subtractHour()
{
    DateTime dt1(QDate(2006, 1, 1), QTime(0, 0, 0, 0));
    DateTime dt2(QDate(2006, 1, 1), QTime(1, 0, 0, 0));
    Duration d(0, 1, 0, 0, 0);

    QVERIFY((dt2-dt1).toString() == d.toString());
    QVERIFY((dt1-dt2).toString() == d.toString()); // result always positive
    QVERIFY((dt2-d).toString() == dt1.toString());
}
void DateTimeTester::subtractMinute()
{
    DateTime dt1(QDate(2006, 1, 1), QTime(0, 0, 0, 0));
    DateTime dt2(QDate(2006, 1, 1), QTime(0, 1, 0, 0));
    Duration d(0, 0, 1, 0, 0);

    QVERIFY((dt2-dt1).toString() == d.toString());
    QVERIFY((dt1-dt2).toString() == d.toString()); // result always positive
    QVERIFY((dt2-d).toString() == dt1.toString());
}
void DateTimeTester::subtractSecond()
{
    DateTime dt1(QDate(2006, 1, 1), QTime(0, 0, 0, 0));
    DateTime dt2(QDate(2006, 1, 1), QTime(0, 0, 1, 0));
    Duration d(0, 0, 0, 1, 0);

    QVERIFY((dt2-dt1).toString() == d.toString());
    QVERIFY((dt1-dt2).toString() == d.toString()); // result always positive
    QVERIFY((dt2-d).toString() == dt1.toString());
    
}

void DateTimeTester::subtractMillisecond()
{
    DateTime dt1(QDate(2006, 1, 1), QTime(0, 0, 0, 0));
    DateTime dt2(QDate(2006, 1, 1), QTime(0, 0, 0, 1));
    Duration d(0, 0, 0, 0, 1);

    QVERIFY((dt2-dt1).toString() == d.toString());
    QVERIFY((dt1-dt2).toString() == d.toString()); // result always positive
    QVERIFY((dt2-d) == dt1);
    
    dt1 = DateTime(QDate(2006, 1, 1), QTime(0, 0, 0, 1));
    dt2 = DateTime(QDate(2006, 1, 1), QTime(0, 0, 0, 0));
    d = Duration(0, 0, 0, 0, 1);

    QVERIFY((dt2-dt1).toString() == d.toString());
    QVERIFY((dt1-dt2).toString() == d.toString()); // result always positive
    QVERIFY((dt1-d) == dt2);
    
    dt1 = DateTime(QDate(2006, 1, 1), QTime(0, 0, 1, 1));
    dt2 = DateTime(QDate(2006, 1, 1), QTime(0, 1, 0, 0));
    d = Duration(0, 0, 0, 58, 999);

    QVERIFY((dt2-dt1).toString() == d.toString());
    QVERIFY((dt1-dt2).toString() == d.toString()); // result always positive
    QVERIFY((dt2-d) == dt1);   
}

void DateTimeTester::addDay()
{
    DateTime dt1(QDate(2006, 1, 1), QTime(8, 0, 0, 0));
    DateTime dt2(QDate(2006, 1, 2), QTime(8, 0, 0, 0));
    Duration d(1, 0, 0, 0, 0);
    
    QVERIFY((dt1+d) == dt2);
}
void DateTimeTester::addHour()
{
    DateTime dt1(QDate(2006, 1, 1), QTime(0, 0, 0, 0));
    DateTime dt2(QDate(2006, 1, 1), QTime(1, 0, 0, 0));
    Duration d(0, 1, 0, 0, 0);

    QVERIFY((dt1+d) == dt2);
}
void DateTimeTester::addMinute()
{
    DateTime dt1(QDate(2006, 1, 1), QTime(0, 0, 0, 0));
    DateTime dt2(QDate(2006, 1, 1), QTime(0, 1, 0, 0));
    Duration d(0, 0, 1, 0, 0);

    QVERIFY((dt1+d) == dt2);
}
void DateTimeTester::addSecond()
{
    DateTime dt1(QDate(2006, 1, 1), QTime(0, 0, 0, 0));
    DateTime dt2(QDate(2006, 1, 1), QTime(0, 0, 1, 0));
    Duration d(0, 0, 0, 1, 0);

    QVERIFY((dt1+d) == dt2);
    
}
void DateTimeTester::addMillisecond()
{
    DateTime dt1(QDate(2006, 1, 1), QTime(0, 0, 0, 0));
    DateTime dt2(QDate(2006, 1, 1), QTime(0, 0, 0, 1));
    Duration d(0, 0, 0, 0, 1);

    QVERIFY((dt1+d) == dt2);
    
}

void DateTimeTester::timeZones()
{
    DateTime dt0(QDate(2006, 1, 1), QTime(8, 0, 0, 0));
    QCOMPARE(dt0.timeZone(), QTimeZone::systemTimeZone());

    QTimeZone testZone("Europe/London");
    DateTime dt1(QDate(2006, 1, 1), QTime(8, 0, 0, 0), testZone);
    
    DateTime dt2 = dt1;
    qDebug()<<dt1<<dt2;
    QCOMPARE(dt1.timeZone(), testZone);
    QCOMPARE(dt1.timeZone(), dt2.timeZone());

    dt2 += Duration(1, 0, 0, 0, 0);
    qDebug()<<dt2;
    QCOMPARE(dt1.timeZone(), dt2.timeZone());
    
    dt2 -= Duration(1, 0, 0, 0, 0);
    qDebug()<<dt1<<dt2;
    QCOMPARE(dt1.timeZone(), dt2.timeZone());
    QCOMPARE(dt2, dt1);
    
    dt2 = dt1 + Duration(1, 0, 0, 0, 0);
    qDebug()<<dt1<<dt2;
    QCOMPARE(dt1.timeZone(), dt2.timeZone());

    dt2 = dt2 - Duration(1, 0, 0, 0, 0);
    qDebug()<<dt1<<dt2;
    QCOMPARE(dt1.timeZone(), dt2.timeZone());
    QCOMPARE(dt2, dt1);
    
    DateTime dt3 = QDateTime(QDate(2006, 1, 1), QTime(8, 0, 0, 0), testZone);
    qDebug()<<dt3;
    QCOMPARE(dt3.timeZone(), testZone);

    DateTime dt4(QDateTime(QDate(2006, 1, 1), QTime(8, 0, 0, 0), Qt::UTC));
    dt4 += Duration(1, 0, 0, 0, 0);
    qDebug()<<dt4<<dt4.timeZone();
    QCOMPARE(dt4.timeSpec(), Qt::UTC);

    QTimeZone tz("Europe/Copenhagen");
    DateTime dst1(QDate(2006, 3, 26), QTime(2, 30, 0, 0), tz); // invalid due to dst
    DateTime dst2(QDate(2006, 3, 26), QTime(3, 30, 0, 0), tz);
    qDebug()<<"dst:"<<dst1<<dst2<<(dst2-dst1).toString();
    QCOMPARE(dst1, dst2);

    DateTime dst3(QDate(2006, 3, 26), QTime(1, 30, 0, 0), tz);
    DateTime dst4 = dst3 + Duration(0, 1, 0);
    QCOMPARE(dst4, dst2);
    dst4 -= Duration(0, 1, 0);
    QCOMPARE(dst4, dst3);

    DateTime dst5(QDate(2022, 10, 30), QTime(1, 0, 0, 0), tz);
    DateTime dst6(QDate(2022, 10, 30), QTime(4, 0, 0, 0), tz);
    auto dst7 = dst5 + Duration(0, 4, 0);
    qDebug()<<"dst:"<<dst5<<dst6<<dst7;
    QCOMPARE(dst7, dst6);
    dst7 = dst6 - Duration(0, 1, 0);
    DateTime dst8(QDate(2022, 10, 30), QTime(3, 0, 0, 0), tz);
    QCOMPARE(dst7, dst8);

    dst7 = dst7 - Duration(0, 1, 0);
    dst8 = DateTime(QDate(2022, 10, 30), QTime(2, 0, 0, 0), tz);
    qDebug()<<"dst:"<<dst7<<dst8;
    QCOMPARE(dst7, dst8);

    dst7 = dst7 - Duration(0, 1, 0); // since Qt6.7 change see QDateTime::TransitionResolution, for a negative duration use RelativeToAfter instead of RelativeToBefore
    dst8 = DateTime(QDate(2022, 10, 30), QTime(1, 0, 0, 0), tz);
    qDebug()<<"dst:"<<dst7<<dst8;
    QCOMPARE(dst7, dst8);
}

} //namespace KPlato

QTEST_GUILESS_MAIN(KPlato::DateTimeTester)
