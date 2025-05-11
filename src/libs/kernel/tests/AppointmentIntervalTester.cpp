/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2008 Dag Andersen <dag.andersen@kdemail.net>
   SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
   
   SPDX-License-Identifier: LGPL-2.0-or-later
*/
// clazy:excludeall=qstring-arg
#include "AppointmentIntervalTester.h"
#include <kptappointment.h>
#include <kptdatetime.h>
#include <kptduration.h>

#include <QTest>

#include <QMultiMap>

#include "DateTimeTester.h"
#include "debug.cpp"

namespace KPlato
{

void AppointmentIntervalTester::interval()
{
    DateTime dt1 = DateTime(QDate(2011, 01, 02), QTime(7, 0, 0));
    DateTime dt2 = DateTime(QDate(2011, 01, 02), QTime(8, 0, 0));
    DateTime dt3 = DateTime(QDate(2011, 01, 02), QTime(9, 0, 0));
    DateTime dt4 = DateTime(QDate(2011, 01, 02), QTime(10, 0, 0));

    AppointmentInterval i1(dt1, dt2, 1);
    AppointmentInterval i2(dt1, dt2, 1);

    QVERIFY(! (i1 < i2));
    QVERIFY(! (i2 < i1));

    QVERIFY(i1.intersects(i2));
    QVERIFY(i2.intersects(i1));

    AppointmentInterval i3(dt2, dt3, 1);

    QVERIFY(i1 < i3);
    QVERIFY(! (i3 < i1));

    QVERIFY(! i1.intersects(i3));
    QVERIFY(! i3.intersects(i1));

    AppointmentInterval i4(dt2, dt4, 1);

    QVERIFY(i1 < i4);
    QVERIFY(i2 < i4);
    QVERIFY(i3 < i4);

    QVERIFY(! i1.intersects(i4));
    QVERIFY(! i4.intersects(i1));
    QVERIFY(i3.intersects(i4));
    QVERIFY(i4.intersects(i3));

    AppointmentInterval i5(dt3, dt4, 1);
    QVERIFY(! i1.intersects(i4));
    QVERIFY(! i4.intersects(i1));

}

void AppointmentIntervalTester::addInterval()
{
    AppointmentIntervalList lst;
    DateTime dt1 = DateTime(QDate(2011, 01, 02), QTime(7, 0, 0));
    DateTime dt2 = dt1 + Duration(0, 1, 0);
    double load = 1;
    
    qDebug()<<"Add an interval"<<dt1<<dt2;
    qDebug()<<'\n'<<lst;
    lst.add(dt1, dt2, load);
    qDebug()<<'\n'<<lst;

    QCOMPARE(dt1, lst.map().values().first().startTime());
    QCOMPARE(dt2, lst.map().values().first().endTime());
    QCOMPARE(load, lst.map().values().first().load());
    
    qDebug()<<"add load";
    qDebug()<<'\n'<<lst;
    lst.add(dt1, dt2, load);
    qDebug()<<'\n'<<lst;

    QCOMPARE(dt1, lst.map().values().first().startTime());
    QCOMPARE(dt2, lst.map().values().first().endTime());
    QCOMPARE(load*2, lst.map().values().first().load());
    
    DateTime dt3 = dt2 + Duration(0, 4, 0);
    DateTime dt4 = dt3 + Duration(0, 1, 0);
    qDebug()<<"Add an interval after:"<<dt3<<dt4;
    
    qDebug()<<'\n'<<lst;
    lst.add(dt3, dt4, load);
    qDebug()<<'\n'<<lst;

    QCOMPARE(dt1, lst.map().values().first().startTime());
    QCOMPARE(dt2, lst.map().values().first().endTime());
    QCOMPARE(load*2, lst.map().values().first().load());

    QCOMPARE(dt3, lst.map().values().last().startTime());
    QCOMPARE(dt4, lst.map().values().last().endTime());
    QCOMPARE(load, lst.map().values().last().load());

    DateTime dt5 = dt2 + Duration(0, 2, 0);
    DateTime dt6 = dt5 + Duration(0, 1, 0);
    qDebug()<<"Add an interval in between:"<<dt5<<dt6;

    qDebug()<<'\n'<<lst;
    lst.add(dt5, dt6, load);
    qDebug()<<'\n'<<lst;
{
    const auto map = lst.map();
    QMultiMap<QDate, AppointmentInterval>::const_iterator i(map.constBegin());

    QCOMPARE(dt1, i.value().startTime());
    QCOMPARE(dt2, i.value().endTime());
    QCOMPARE(load*2, i.value().load());
    ++i;
    QCOMPARE(dt5, i.value().startTime());
    QCOMPARE(dt6, i.value().endTime());
    QCOMPARE(load, i.value().load());
    ++i;
    QCOMPARE(dt3, i.value().startTime());
    QCOMPARE(dt4, i.value().endTime());
    QCOMPARE(load, i.value().load());
}    
    DateTime dt7 = dt1 - Duration(0, 1, 0);
    DateTime dt8 = dt7 + Duration(0, 2, 0);
    qDebug()<<"Add an overlapping interval at start:"<<dt7<<dt8;
    
    qDebug()<<'\n'<<lst;
    lst.add(dt7, dt8, load);
    qDebug()<<'\n'<<lst;
{
    const auto map = lst.map();
    QMultiMap<QDate, AppointmentInterval>::const_iterator i(map.constBegin());

    QCOMPARE(dt7, i.value().startTime());
    QCOMPARE(dt1, i.value().endTime());
    QCOMPARE(load, i.value().load());
    ++i;
    QCOMPARE(dt1, i.value().startTime());
    QCOMPARE(dt8, i.value().endTime());
    QCOMPARE(load*3, i.value().load());
    ++i;
    QCOMPARE(dt5, i.value().startTime());
    QCOMPARE(dt6, i.value().endTime());
    QCOMPARE(load, i.value().load());
    ++i;
    QCOMPARE(dt3, i.value().startTime());
    QCOMPARE(dt4, i.value().endTime());
    QCOMPARE(load, i.value().load());
}
    DateTime dt9 = dt7 +  Duration(0, 0, 30);
    DateTime dt10 = dt9 + Duration(0, 0, 30);
    qDebug()<<"Add an overlapping interval at start > start, end == end"<<dt9<<dt10;
    
    qDebug()<<'\n'<<lst;
    lst.add(dt9, dt10, load);
    qDebug()<<'\n'<<lst;
{
    const auto map = lst.map();
    QMultiMap<QDate, AppointmentInterval>::const_iterator i(map.constBegin());

    QCOMPARE(dt7, i.value().startTime());
    QCOMPARE(dt9, i.value().endTime());
    QCOMPARE(load, i.value().load());
    ++i;
    QCOMPARE(dt9, i.value().startTime());
    QCOMPARE(dt10, i.value().endTime());
    QCOMPARE(load*2, i.value().load());
    ++i;
    QCOMPARE(dt1, i.value().startTime());
    QCOMPARE(dt8, i.value().endTime());
    QCOMPARE(load*3, i.value().load());
    ++i;
    QCOMPARE(dt5, i.value().startTime());
    QCOMPARE(dt6, i.value().endTime());
    QCOMPARE(load, i.value().load());
    ++i;
    QCOMPARE(dt3, i.value().startTime());
    QCOMPARE(dt4, i.value().endTime());
    QCOMPARE(load, i.value().load());
}
    DateTime dt11 = dt3 +  Duration(0, 0, 10);
    DateTime dt12 = dt11 + Duration(0, 0, 30);
    qDebug()<<"Add an overlapping interval at start > start, end < end:"<<dt11<<dt12;
    
    qDebug()<<'\n'<<lst;
    lst.add(dt11, dt12, load);
    qDebug()<<'\n'<<lst;
{
    QCOMPARE(lst.map().count(), 7);
    const auto map = lst.map();
    QMultiMap<QDate, AppointmentInterval>::const_iterator i(map.constBegin());

    QCOMPARE(dt7, i.value().startTime());
    QCOMPARE(dt9, i.value().endTime());
    QCOMPARE(load, i.value().load());
    ++i;
    QCOMPARE(dt9, i.value().startTime());
    QCOMPARE(dt10, i.value().endTime());
    QCOMPARE(load*2, i.value().load());
    ++i;
    QCOMPARE(dt1, i.value().startTime());
    QCOMPARE(dt8, i.value().endTime());
    QCOMPARE(load*3, i.value().load());
    ++i;
    QCOMPARE(dt5, i.value().startTime());
    QCOMPARE(dt6, i.value().endTime());
    QCOMPARE(load, i.value().load());
    ++i;
    QCOMPARE(dt3, i.value().startTime());
    QCOMPARE(dt11, i.value().endTime());
    QCOMPARE(load, i.value().load());
    ++i;
    QCOMPARE(dt11, i.value().startTime());
    QCOMPARE(dt12, i.value().endTime());
    QCOMPARE(load*2, i.value().load());
    ++i;
    QCOMPARE(dt12, i.value().startTime());
    QCOMPARE(dt4, i.value().endTime());
    QCOMPARE(load, i.value().load());
}
    qDebug()<<"Add an interval overlapping 2 intervals at start == start.1, end == end.2"<<dt1<<dt4;
    lst.clear();
    
    qDebug()<<'\n'<<lst;
    lst.add(dt1, dt2, load);
    qDebug()<<'\n'<<lst;
    lst.add(dt3, dt4, load);
    qDebug()<<'\n'<<lst;
    lst.add(dt1, dt4, load);
    qDebug()<<'\n'<<lst;
{
    QCOMPARE(lst.map().count(), 3);
    const auto map = lst.map();
    QMultiMap<QDate, AppointmentInterval>::const_iterator i(map.constBegin());

    QCOMPARE(dt1, i.value().startTime());
    QCOMPARE(dt2, i.value().endTime());
    QCOMPARE(load*2, i.value().load());
    ++i;
    QCOMPARE(dt2, i.value().startTime());
    QCOMPARE(dt3, i.value().endTime());
    QCOMPARE(load, i.value().load());
    ++i;
    QCOMPARE(dt3, i.value().startTime());
    QCOMPARE(dt4, i.value().endTime());
    QCOMPARE(load*2, i.value().load());
}
    lst.clear();
    dt5 = dt1 - Duration(0, 1, 0);
    qDebug()<<"Add an interval overlapping 2 intervals at start < start.1, end == end.2"<<dt5<<dt4;

    qDebug()<<'\n'<<lst;
    lst.add(dt1, dt2, load);
    qDebug()<<'\n'<<lst;
    lst.add(dt3, dt4, load);
    qDebug()<<'\n'<<lst;
    lst.add(dt5, dt4, load);
    qDebug()<<'\n'<<lst;
{
    QCOMPARE(lst.map().count(), 4);
    const auto map = lst.map();
    QMultiMap<QDate, AppointmentInterval>::const_iterator i(map.constBegin());

    QCOMPARE(dt5, i.value().startTime());
    QCOMPARE(dt1, i.value().endTime());
    QCOMPARE(load, i.value().load());
    ++i;
    QCOMPARE(dt1, i.value().startTime());
    QCOMPARE(dt2, i.value().endTime());
    QCOMPARE(load*2, i.value().load());
    ++i;
    QCOMPARE(dt2, i.value().startTime());
    QCOMPARE(dt3, i.value().endTime());
    QCOMPARE(load, i.value().load());
    ++i;
    QCOMPARE(dt3, i.value().startTime());
    QCOMPARE(dt4, i.value().endTime());
    QCOMPARE(load*2, i.value().load());
}
    // Add an interval overlapping 2 intervals at start < start.1, end > end.2
    lst.clear();
    dt5 = dt1 - Duration(0, 1, 0);
    dt6 = dt4 + Duration(0, 1, 0);
    lst.add(dt1, dt2, load);
    lst.add(dt3, dt4, load);
    lst.add(dt5, dt6, load);
{
    const auto map = lst.map();
    QMultiMap<QDate, AppointmentInterval>::const_iterator i(map.constBegin());

    QCOMPARE(dt5, i.value().startTime());
    QCOMPARE(dt1, i.value().endTime());
    QCOMPARE(load, i.value().load());
    ++i;
    QCOMPARE(dt1, i.value().startTime());
    QCOMPARE(dt2, i.value().endTime());
    QCOMPARE(load*2, i.value().load());
    ++i;
    QCOMPARE(dt2, i.value().startTime());
    QCOMPARE(dt3, i.value().endTime());
    QCOMPARE(load, i.value().load());
    ++i;
    QCOMPARE(dt3, i.value().startTime());
    QCOMPARE(dt4, i.value().endTime());
    QCOMPARE(load*2, i.value().load());
    ++i;
    QCOMPARE(dt4, i.value().startTime());
    QCOMPARE(dt6, i.value().endTime());
    QCOMPARE(load, i.value().load());
}
    // Add an interval overlapping 2 intervals at start < start.1, end < end.2
    lst.clear();
    dt5 = dt1 - Duration(0, 1, 0);
    dt6 = dt4 - Duration(0, 0, 30);
    lst.add(dt1, dt2, load);
    lst.add(dt3, dt4, load);
    lst.add(dt5, dt6, load);
{
    const auto map = lst.map();
    QMultiMap<QDate, AppointmentInterval>::const_iterator i(map.constBegin());

    QCOMPARE(dt5, i.value().startTime());
    QCOMPARE(dt1, i.value().endTime());
    QCOMPARE(load, i.value().load());
    ++i;
    QCOMPARE(dt1, i.value().startTime());
    QCOMPARE(dt2, i.value().endTime());
    QCOMPARE(load*2, i.value().load());
    ++i;
    QCOMPARE(dt2, i.value().startTime());
    QCOMPARE(dt3, i.value().endTime());
    QCOMPARE(load, i.value().load());
    ++i;
    QCOMPARE(dt3, i.value().startTime());
    QCOMPARE(dt6, i.value().endTime());
    QCOMPARE(load*2, i.value().load());
    ++i;
    QCOMPARE(dt6, i.value().startTime());
    QCOMPARE(dt4, i.value().endTime());
    QCOMPARE(load, i.value().load());
}
    // Add an interval overlapping 2 intervals at start > start.1, end < end.2
    lst.clear();
    dt5 = dt1 + Duration(0, 0, 30);
    dt6 = dt4 - Duration(0, 0, 30);
    lst.add(dt1, dt2, load);
    lst.add(dt3, dt4, load);
    lst.add(dt5, dt6, load);
{
    const auto map = lst.map();
    QMultiMap<QDate, AppointmentInterval>::const_iterator i(map.constBegin());

    QCOMPARE(dt1, i.value().startTime());
    QCOMPARE(dt5, i.value().endTime());
    QCOMPARE(load, i.value().load());
    ++i;
    QCOMPARE(dt5, i.value().startTime());
    QCOMPARE(dt2, i.value().endTime());
    QCOMPARE(load*2, i.value().load());
    ++i;
    QCOMPARE(dt2, i.value().startTime());
    QCOMPARE(dt3, i.value().endTime());
    QCOMPARE(load, i.value().load());
    ++i;
    QCOMPARE(dt3, i.value().startTime());
    QCOMPARE(dt6, i.value().endTime());
    QCOMPARE(load*2, i.value().load());
    ++i;
    QCOMPARE(dt6, i.value().startTime());
    QCOMPARE(dt4, i.value().endTime());
    QCOMPARE(load, i.value().load());
}
    // Add an interval overlapping 2 intervals at start > start.1, end == end.2
    lst.clear();
    dt5 = dt1 + Duration(0, 0, 30);
    dt6 = dt4;
    lst.add(dt1, dt2, load);
    lst.add(dt3, dt4, load);
    lst.add(dt5, dt6, load);
{
    const auto map = lst.map();
    QMultiMap<QDate, AppointmentInterval>::const_iterator i(map.constBegin());

    QCOMPARE(dt1, i.value().startTime());
    QCOMPARE(dt5, i.value().endTime());
    QCOMPARE(load, i.value().load());
    ++i;
    QCOMPARE(dt5, i.value().startTime());
    QCOMPARE(dt2, i.value().endTime());
    QCOMPARE(load*2, i.value().load());
    ++i;
    QCOMPARE(dt2, i.value().startTime());
    QCOMPARE(dt3, i.value().endTime());
    QCOMPARE(load, i.value().load());
    ++i;
    QCOMPARE(dt3, i.value().startTime());
    QCOMPARE(dt6, i.value().endTime());
    QCOMPARE(load*2, i.value().load());
}
    // Add an interval overlapping 2 intervals at start > start.1, end > end.2
    lst.clear();
    dt5 = dt1 + Duration(0, 0, 30);
    dt6 = dt4 + Duration(0, 0, 30);
    lst.add(dt1, dt2, load);
    lst.add(dt3, dt4, load);
    lst.add(dt5, dt6, load);
{
    const auto map = lst.map();
    QMultiMap<QDate, AppointmentInterval>::const_iterator i(map.constBegin());

    QCOMPARE(dt1, i.value().startTime());
    QCOMPARE(dt5, i.value().endTime());
    QCOMPARE(load, i.value().load());
    ++i;
    QCOMPARE(dt5, i.value().startTime());
    QCOMPARE(dt2, i.value().endTime());
    QCOMPARE(load*2, i.value().load());
    ++i;
    QCOMPARE(dt2, i.value().startTime());
    QCOMPARE(dt3, i.value().endTime());
    QCOMPARE(load, i.value().load());
    ++i;
    QCOMPARE(dt3, i.value().startTime());
    QCOMPARE(dt4, i.value().endTime());
    QCOMPARE(load*2, i.value().load());
    ++i;
    QCOMPARE(dt4, i.value().startTime());
    QCOMPARE(dt6, i.value().endTime());
    QCOMPARE(load, i.value().load());
}

}

void AppointmentIntervalTester::addTangentIntervals()
{
    // Add an interval overlapping 2 intervals at start > start.1, end > end.2
    AppointmentIntervalList lst;

    DateTime dt1(QDate(2010, 1, 1), QTime(4, 0, 0));
    DateTime dt2(QDate(2010, 1, 1), QTime(8, 0, 0));
    DateTime dt3(QDate(2010, 1, 1), QTime(8, 0, 0));
    DateTime dt4(QDate(2010, 1, 1), QTime(12, 0, 0));
    DateTime dt5(QDate(2010, 1, 1), QTime(12, 0, 0));
    DateTime dt6(QDate(2010, 1, 1), QTime(16, 0, 0));
    double load = 1.;

    qDebug()<<'\n'<<lst;
    lst.add(dt1, dt2, load);
    qDebug()<<'\n'<<lst;
    QCOMPARE(lst.map().count(), 1);
    QCOMPARE(lst.map().values().at(0).startTime(), dt1);
    QCOMPARE(lst.map().values().at(0).endTime(), dt2);

    lst.add(dt5, dt6, load);
    qDebug()<<lst;
    QCOMPARE(lst.map().count(), 2);
    QCOMPARE(lst.map().values().at(0).startTime(), dt1);
    QCOMPARE(lst.map().values().at(0).endTime(), dt2);
    QCOMPARE(lst.map().values().at(1).startTime(), dt5);
    QCOMPARE(lst.map().values().at(1).endTime(), dt6);

    lst.add(dt3, dt4, load);
    qDebug()<<lst;
    QCOMPARE(lst.map().count(), 1);
    QCOMPARE(lst.map().values().at(0).startTime(), dt1);
    QCOMPARE(lst.map().values().at(0).endTime(), dt6);

    lst.add(dt3, dt4, load*2);
    QCOMPARE(lst.map().count(), 3);
    QCOMPARE(lst.map().values().at(0).startTime(), dt1);
    QCOMPARE(lst.map().values().at(0).endTime(), dt2);
    QCOMPARE(lst.map().values().at(1).startTime(), dt3);
    QCOMPARE(lst.map().values().at(1).endTime(), dt4);
    QCOMPARE(lst.map().values().at(2).startTime(), dt5);
    QCOMPARE(lst.map().values().at(2).endTime(), dt6);


}

void AppointmentIntervalTester::addAppointment()
{
    Appointment app1, app2;

    DateTime dt1 = DateTime(QDate(2011, 01, 02), QTime(7, 0, 0));
    DateTime dt2 = dt1 + Duration(0, 1, 0);
    double load = 1;
    
    app2.addInterval(dt1, dt2, load);
    app1 += app2;
    QCOMPARE(dt1, app1.intervals().map().values().first().startTime());
    QCOMPARE(dt2, app1.intervals().map().values().first().endTime());
    QCOMPARE(load, app1.intervals().map().values().first().load());

    app1 += app2;
    qDebug()<<load<<app1.intervals().map().values().first().load();
    QCOMPARE(dt1, app1.intervals().map().values().first().startTime());
    QCOMPARE(dt2, app1.intervals().map().values().first().endTime());
    QCOMPARE(load*2, app1.intervals().map().values().first().load());
}

void AppointmentIntervalTester::subtractList()
{
    QString s;

    AppointmentIntervalList lst1;
    AppointmentIntervalList lst2;
    DateTime dt1 = DateTime(QDate(2011, 01, 02), QTime(7, 0, 0));
    DateTime dt2 = dt1 + Duration(0, 3, 0);
    double load = 100;
    
    lst1.add(dt1, dt2, load);
    QCOMPARE(dt1, lst1.map().values().first().startTime());
    QCOMPARE(dt2, lst1.map().values().first().endTime());
    QCOMPARE(load, lst1.map().values().first().load());
    
    lst2 += lst1;
    QCOMPARE(dt1, lst2.map().values().first().startTime());
    QCOMPARE(dt2, lst2.map().values().first().endTime());
    QCOMPARE(load, lst2.map().values().first().load());
    QCOMPARE(lst2.map().count(), 1);
    
    lst2 -= lst1;
    QVERIFY(lst2.isEmpty());
    
    lst2.add(dt1, dt2, load * 2.);
    lst2 -= lst1;
    QCOMPARE(dt1, lst2.map().values().first().startTime());
    QCOMPARE(dt2, lst2.map().values().first().endTime());
    QCOMPARE(load, lst2.map().values().first().load());
    QCOMPARE(lst2.map().count(), 1);
    
    lst1.clear();
    DateTime dt3 = dt2 + Duration(0, 6, 0);
    DateTime dt4 = dt3 + Duration(0, 1, 0);
    lst1.add(dt3, dt4, load);
    qDebug()<<"Subtract non-overlapping intervals:";

    qDebug()<<'\n'<<lst2<<'\n'<<"minus"<<'\n'<<lst1;
    lst2 -= lst1;
    qDebug()<<'\n'<<"result:"<<'\n'<<lst2;
    QCOMPARE(dt1, lst2.map().values().first().startTime());
    QCOMPARE(dt2, lst2.map().values().first().endTime());
    QCOMPARE(load, lst2.map().values().first().load());
    QCOMPARE(lst2.map().count(), 1);
    
    DateTime dt5 = dt1 - Duration(0, 6, 0);
    DateTime dt6 = dt5 + Duration(0, 1, 0);
    lst1.add(dt5, dt6, load);

    qDebug()<<"-------- lst2 -= lst1";
    qDebug()<<'\n'<<lst2<<'\n'<<lst1;
    lst2 -= lst1;
    qDebug()<<'\n'<<lst2;
    QCOMPARE(dt1, lst2.map().values().first().startTime());
    QCOMPARE(dt2, lst2.map().values().first().endTime());
    QCOMPARE(load, lst2.map().values().first().load());
    QCOMPARE(lst2.map().count(), 1);

    s = QStringLiteral("Subtract tangent intervals");
    qDebug()<<s;
    lst1.clear();
    lst1.add(dt1.addDays(-1), dt1, load); // before
    Debug::print(lst2, QStringLiteral("List2: ") + s);
    Debug::print(lst1, QStringLiteral("List1: ") + s);

    lst2 -= lst1;
    Debug::print(lst2, QStringLiteral("Result: ") + s);
    
    QCOMPARE(dt1, lst2.map().values().first().startTime());
    QCOMPARE(dt2, lst2.map().values().first().endTime());
    QCOMPARE(load, lst2.map().values().first().load());
    QCOMPARE(lst2.map().count(), 1);

    lst1.clear();
    lst1.add(dt2, dt2.addDays(1), load); // after

    lst2 -= lst1;
    QCOMPARE(dt1, lst2.map().values().first().startTime());
    QCOMPARE(dt2, lst2.map().values().first().endTime());
    QCOMPARE(load, lst2.map().values().first().load());
    QVERIFY(lst2.map().count() == 1);
    
    // Subtract overlapping intervals
    lst1.clear();
    dt3 = dt1 + Duration(0, 1, 0);
    // starts at start, end in the middle
    lst1.add(dt1, dt3, load / 2.);

    s = QStringLiteral("Subtract half the load of the first hour of the interval");
    qDebug()<<s;
    Debug::print(lst2, QStringLiteral("List2: ") + s);
    Debug::print(lst1, QStringLiteral("List1: ") + s);

    lst2 -= lst1;
    Debug::print(lst2, s);

    QCOMPARE(lst2.map().count(), 2);
    QCOMPARE(dt1, lst2.map().values().first().startTime());
    QCOMPARE(dt3, lst2.map().values().first().endTime());
    QCOMPARE(load / 2., lst2.map().values().first().load());

    QCOMPARE(dt3, lst2.map().values().at(1).startTime());
    QCOMPARE(dt2, lst2.map().values().at(1).endTime());
    QCOMPARE(load, lst2.map().values().at(1).load());

    s = QStringLiteral("Subtract all load from first interval");
    qDebug()<<s;
    lst2 -= lst1; // remove first interval
    QCOMPARE(lst2.map().count(), 1);
    QCOMPARE(dt3, lst2.map().values().at(0).startTime());
    QCOMPARE(dt2, lst2.map().values().at(0).endTime());
    QCOMPARE(load, lst2.map().values().at(0).load());

    s = QStringLiteral("Subtract half the load from last hour of the interval");
    qDebug()<<s;
    lst1.clear();
    dt4 = dt2 - Duration(0, 1, 0);
    lst1.add(dt4, dt2, 50.);
    
    Debug::print(lst1, QStringLiteral("List1: ") + s);
    Debug::print(lst2, QStringLiteral("List2: ") + s);
    lst2 -= lst1;
    
    QCOMPARE(lst2.map().count(), 2);
    QCOMPARE(dt3, lst2.map().values().at(0).startTime());
    QCOMPARE(dt4, lst2.map().values().at(0).endTime());
    QCOMPARE(load, lst2.map().values().at(0).load());

    QCOMPARE(dt4, lst2.map().values().at(1).startTime());
    QCOMPARE(dt2, lst2.map().values().at(1).endTime());
    QCOMPARE(50., lst2.map().values().at(1).load());

    s = QStringLiteral("Subtract all load from last interval");
    qDebug()<<s;
    Debug::print(lst1, QStringLiteral("List1: ") + s);
    Debug::print(lst2, QStringLiteral("List2: ") + s);

    AppointmentInterval i = lst2.map().values().at(0);
    lst2 -= lst1;
    Debug::print(lst2, QStringLiteral("Result: ") + s);

    QCOMPARE(lst2.map().count(), 1);
    QCOMPARE(i.startTime(), lst2.map().values().at(0).startTime());
    QCOMPARE(i.endTime(), lst2.map().values().at(0).endTime());
    QCOMPARE(i.load(), lst2.map().values().at(0).load());

    // Subtract overlapping intervals (start < start, end > end)
    lst1.clear();
    lst2.clear();
    lst2.add(dt1, dt2, 100.);

    dt3 = dt1 + Duration(0, 1, 0);
    // starts before start, end in the middle
    lst1.add(dt1.addSecs(-10), dt3, load / 2.);

    s = QStringLiteral("Subtract half the load of the first hour of the interval");
    qDebug()<<s;
    Debug::print(lst2, QStringLiteral("List2: ") + s);
    Debug::print(lst1, QStringLiteral("List1: ") + s);

    lst2 -= lst1;
    Debug::print(lst2, s);

    QCOMPARE(lst2.map().count(), 2);
    QCOMPARE(dt1, lst2.map().values().first().startTime());
    QCOMPARE(dt3, lst2.map().values().first().endTime());
    QCOMPARE(load / 2., lst2.map().values().first().load());

    QCOMPARE(dt3, lst2.map().values().at(1).startTime());
    QCOMPARE(dt2, lst2.map().values().at(1).endTime());
    QCOMPARE(load, lst2.map().values().at(1).load());

    s = QStringLiteral("Subtract all load from first interval");
    qDebug()<<s;
    lst2 -= lst1; // remove first interval
    QCOMPARE(lst2.map().count(), 1);
    QCOMPARE(dt3, lst2.map().values().at(0).startTime());
    QCOMPARE(dt2, lst2.map().values().at(0).endTime());
    QCOMPARE(load, lst2.map().values().at(0).load());

    s = QStringLiteral("Subtract half the load from last hour of the interval");
    qDebug()<<s;
    lst1.clear();
    dt4 = dt2 - Duration(0, 1, 0);
    lst1.add(dt4, dt2.addSecs(10), 50.);
    
    Debug::print(lst2, QStringLiteral("List2: ") + s);
    Debug::print(lst1, QStringLiteral("List1: ") + s);
    lst2 -= lst1;
    
    Debug::print(lst2, QStringLiteral("Result: ") + s);

    QCOMPARE(lst2.map().count(), 2);
    QCOMPARE(dt3, lst2.map().values().at(0).startTime());
    QCOMPARE(dt4, lst2.map().values().at(0).endTime());
    QCOMPARE(load, lst2.map().values().at(0).load());

    QCOMPARE(dt4, lst2.map().values().at(1).startTime());
    QCOMPARE(dt2, lst2.map().values().at(1).endTime());
    QCOMPARE(50., lst2.map().values().at(1).load());

    s = QStringLiteral("Subtract all load from last interval");
    qDebug()<<s;
    Debug::print(lst1, QStringLiteral("List1: ") + s);
    Debug::print(lst2, QStringLiteral("List2: ") + s);

    i = lst2.map().values().at(0);
    qDebug()<<"i:"<<i;
    lst2 -= lst1;
    Debug::print(lst2, QStringLiteral("Result: ") + s);

    QCOMPARE(lst2.map().count(), 1);
    QCOMPARE(i.startTime(), lst2.map().values().at(0).startTime());
    QCOMPARE(i.endTime(), lst2.map().values().at(0).endTime());
    QCOMPARE(i.load(), lst2.map().values().at(0).load());
}

void AppointmentIntervalTester::subtractListMidnight()
{
    QString s;

    AppointmentIntervalList lst1;
    AppointmentIntervalList lst2;
    DateTime dt1 = DateTime(QDate(2011, 01, 02), QTime(22, 0, 0));
    DateTime dt2 = dt1 + Duration(0, 3, 0);
    double load = 100;
    
    lst1.add(dt1, dt2, load);
    QCOMPARE(lst1.map().count(), 2);
    QCOMPARE(dt1, lst1.map().values().first().startTime());
    QCOMPARE(dt2, lst1.map().values().last().endTime());
    QCOMPARE(load, lst1.map().values().first().load());
    QCOMPARE(load, lst1.map().values().last().load());
    
    lst2 += lst1;
    QCOMPARE(lst2.map().count(), 2);
    QCOMPARE(dt1, lst2.map().values().first().startTime());
    QCOMPARE(dt2, lst2.map().values().last().endTime());
    QCOMPARE(load, lst2.map().values().first().load());
    QCOMPARE(load, lst2.map().values().last().load());
    
    lst2 -= lst1;
    QVERIFY(lst2.isEmpty());
    
    lst2.add(dt1, dt2, load * 2.);
    lst2 -= lst1;
    QCOMPARE(lst2.map().count(), 2);
    QCOMPARE(dt1, lst2.map().values().first().startTime());
    QCOMPARE(dt2, lst2.map().values().last().endTime());
    QCOMPARE(load, lst2.map().values().first().load());
    QCOMPARE(load, lst2.map().values().last().load());
    
    lst1.clear();
    DateTime dt3 = dt2 + Duration(0, 6, 0);
    DateTime dt4 = dt3 + Duration(0, 1, 0);
    lst1.add(dt3, dt4, load);
    qDebug()<<"Subtract non-overlapping intervals:";

    qDebug()<<'\n'<<lst2<<'\n'<<"minus"<<'\n'<<lst1;
    lst2 -= lst1;
    qDebug()<<'\n'<<"result:"<<'\n'<<lst2;
    QCOMPARE(lst2.map().count(), 2);
    QCOMPARE(dt1, lst2.map().values().first().startTime());
    QCOMPARE(dt2, lst2.map().values().last().endTime());
    QCOMPARE(load, lst2.map().values().first().load());
    QCOMPARE(load, lst2.map().values().last().load());
    
    DateTime dt5 = dt1 - Duration(0, 6, 0);
    DateTime dt6 = dt5 + Duration(0, 1, 0);
    lst1.add(dt5, dt6, load);

    qDebug()<<"-------- lst2 -= lst1";
    qDebug()<<'\n'<<lst2<<'\n'<<lst1;
    lst2 -= lst1;
    qDebug()<<'\n'<<lst2;
    QCOMPARE(lst2.map().count(), 2);
    QCOMPARE(dt1, lst2.map().values().first().startTime());
    QCOMPARE(dt2, lst2.map().values().last().endTime());
    QCOMPARE(load, lst2.map().values().first().load());
    QCOMPARE(load, lst2.map().values().last().load());

    s = QStringLiteral("Subtract tangent intervals");
    qDebug()<<s;
    lst1.clear();
    lst1.add(dt1.addDays(-1), dt1, load); // before
    Debug::print(lst2, QStringLiteral("List2: ") + s);
    Debug::print(lst1, QStringLiteral("List1: ") + s);

    lst2 -= lst1;
    Debug::print(lst2, QStringLiteral("Result: ") + s);
    
    QCOMPARE(lst2.map().count(), 2);
    QCOMPARE(dt1, lst2.map().values().first().startTime());
    QCOMPARE(dt2, lst2.map().values().last().endTime());
    QCOMPARE(load, lst2.map().values().first().load());
    QCOMPARE(load, lst2.map().values().last().load());

    lst1.clear();
    lst1.add(dt2, dt2.addDays(1), load); // after

    lst2 -= lst1;
    QCOMPARE(lst2.map().count(), 2);
    QCOMPARE(dt1, lst2.map().values().first().startTime());
    QCOMPARE(dt2, lst2.map().values().last().endTime());
    QCOMPARE(load, lst2.map().values().first().load());
    QCOMPARE(load, lst2.map().values().last().load());
    
    // Subtract overlapping intervals
    lst1.clear();
    dt3 = dt1 + Duration(0, 1, 0);
    // starts at start, end in the middle (at 23:00)
    lst1.add(dt1, dt3, load / 2.);

    s = QStringLiteral("Subtract half the load of the first hour of the interval");
    qDebug()<<s;
    Debug::print(lst2, QStringLiteral("List2: ") + s);
    Debug::print(lst1, QStringLiteral("List1: ") + s);

    lst2 -= lst1;
    Debug::print(lst2, s);

    QCOMPARE(lst2.map().count(), 3);
    QCOMPARE(dt1, lst2.map().values().first().startTime());
    QCOMPARE(dt3, lst2.map().values().first().endTime());
    QCOMPARE(load / 2., lst2.map().values().first().load());

    QCOMPARE(dt3, lst2.map().values().at(1).startTime());
    QCOMPARE(dt2, lst2.map().values().at(2).endTime());
    QCOMPARE(load, lst2.map().values().at(1).load());
    QCOMPARE(load, lst2.map().values().at(2).load());

    s = QStringLiteral("Subtract all load from first interval");
    qDebug()<<s;
    lst2 -= lst1; // remove first interval
    QCOMPARE(lst2.map().count(), 2);
    QCOMPARE(dt3, lst2.map().values().at(0).startTime());
    QCOMPARE(dt2, lst2.map().values().at(1).endTime());
    QCOMPARE(load, lst2.map().values().at(0).load());
    QCOMPARE(load, lst2.map().values().at(1).load());

    s = QStringLiteral("Subtract half the load from last 30 min of the last interval");
    qDebug()<<s;
    lst1.clear();
    dt4 = dt2 - Duration(0, 0, 30);
    lst1.add(dt4, dt2, 50.);
    
    Debug::print(lst1, QStringLiteral("List1: ") + s);
    Debug::print(lst2, QStringLiteral("List2: ") + s);
    lst2 -= lst1;
    
    QCOMPARE(lst2.map().count(), 3);
    QCOMPARE(dt3, lst2.map().values().at(0).startTime());
    QCOMPARE(dt4, lst2.map().values().at(1).endTime());
    QCOMPARE(load, lst2.map().values().at(0).load());
    QCOMPARE(load, lst2.map().values().at(1).load());

    QCOMPARE(dt4, lst2.map().values().at(2).startTime());
    QCOMPARE(dt2, lst2.map().values().at(2).endTime());
    QCOMPARE(50., lst2.map().values().at(2).load());

    s = QStringLiteral("Subtract all load from last interval");
    qDebug()<<s;
    Debug::print(lst1, QStringLiteral("List1: ") + s);
    Debug::print(lst2, QStringLiteral("List2: ") + s);

    lst2 -= lst1;
    Debug::print(lst2, QStringLiteral("Result: ") + s);

    QCOMPARE(lst2.map().count(), 2);
    QCOMPARE(dt3, lst2.map().values().at(0).startTime());
    QCOMPARE(dt4, lst2.map().values().at(1).endTime());
    QCOMPARE(load, lst2.map().values().at(0).load());
    QCOMPARE(load, lst2.map().values().at(1).load());

}

void AppointmentIntervalTester::timeZones()
{
    QTimeZone tzCopenhagen("Europe/Copenhagen");
    AppointmentIntervalList lst1;
    DateTime dt1 = DateTime(QDate(2011, 02, 01), QTime(0, 0, 0), tzCopenhagen);
    DateTime dt2 = dt1 + Duration(0, 3, 0);
    double load = 100;
    lst1.add(dt1, dt2, load);
    QCOMPARE(lst1.map().count(), 1);
    QCOMPARE(dt1, lst1.map().values().first().startTime());
    QCOMPARE(dt2, lst1.map().values().last().endTime());

    QTimeZone tzLondon("Europe/London");
    lst1.toTimeZone(tzLondon);
    Debug::print(lst1);
    DateTime dt3 = dt1.toTimeZone(tzLondon);
    DateTime dt4 = dt3.addSecs(3600);
    DateTime dt5 = dt4.addSecs(2*3600);
    QCOMPARE(lst1.map().count(), 2);
    QCOMPARE(lst1.map().values().at(0).startTime(), dt3);
    QCOMPARE(lst1.map().values().at(0).endTime(), dt4);
    QCOMPARE(lst1.map().values().at(1).startTime(), dt4);
    QCOMPARE(lst1.map().values().at(1).endTime(), dt5);

    lst1.toTimeZone(tzCopenhagen);
    Debug::print(lst1);
    QCOMPARE(lst1.map().count(), 1);
    QCOMPARE(lst1.map().values().first().startTime(), dt1);
    QCOMPARE(lst1.map().values().last().endTime(), dt2);

    DateTime dt10 = DateTime(QDate(2011, 01, 31), QTime(23, 0, 0), tzLondon);
    DateTime dt11 = dt10 + Duration(0, 1, 0);
    lst1.add(dt10, dt11, 100);
    Debug::print(lst1);
    QCOMPARE(lst1.map().count(), 2);
    QCOMPARE(lst1.map().values().first().startTime(), dt10);
    QCOMPARE(lst1.map().values().first().load(), 200);
    QCOMPARE(lst1.map().values().first().endTime(), dt11);
    QCOMPARE(lst1.map().values().last().endTime(), dt2);

    DateTime dt20 = DateTime(QDate(2011, 01, 31), QTime(22, 0, 0), tzLondon);
    DateTime dt21 = dt20 + Duration(0, 1, 0);
    lst1.add(dt20, dt21, 100);
    Debug::print(lst1);
    auto date = dt20.toTimeZone(tzCopenhagen).date();
    QCOMPARE(lst1.timeZone(), tzCopenhagen);
    QCOMPARE(lst1.map().values(date).count(), 1);
    QCOMPARE(lst1.map().values(date).first().startTime(), dt20.toTimeZone(tzCopenhagen));
    QCOMPARE(lst1.map().values(date).first().load(), 100);
    QCOMPARE(lst1.map().values(date).first().endTime(), DateTime(dt21.toTimeZone(tzCopenhagen)));

    date = dt1.date();
    QCOMPARE(lst1.map().values(date).count(), 2);
    QCOMPARE(lst1.map().values(date).first().startTime(), dt1);
    QCOMPARE(lst1.map().values(date).first().load(), 200);
    QCOMPARE(lst1.map().values(date).first().endTime(), dt11);
    QCOMPARE(lst1.map().values(date).last().startTime(), dt2 - Duration(0, 2, 0));
    QCOMPARE(lst1.map().values(date).last().load(), 100);
    QCOMPARE(lst1.map().values(date).last().endTime(), dt2);

    QTimeZone tz2("Atlantic/South_Georgia");
    DateTime dt30 = DateTime(QDate(2011, 02, 1), QTime(0, 0, 0), tz2);
    DateTime dt31 = dt30 + Duration(0, 1, 0);
    lst1.add(dt30, dt31, 100); // should be merged with last interval
    Debug::print(lst1);
    QCOMPARE(lst1.map().values(date).count(), 2);
    QCOMPARE(lst1.map().values(date).first().startTime(), dt1);
    QCOMPARE(lst1.map().values(date).first().load(), 200);
    QCOMPARE(lst1.map().values(date).first().endTime(), dt11);
    QCOMPARE(lst1.map().values(date).last().startTime(), dt2 - Duration(0, 2, 0));
    QCOMPARE(lst1.map().values(date).last().load(), 100);
    QCOMPARE(lst1.map().values(date).last().endTime(), DateTime(dt31.toTimeZone(tzCopenhagen)));

}

} //namespace KPlato

QTEST_GUILESS_MAIN(KPlato::AppointmentIntervalTester)
