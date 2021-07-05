/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2006-2007 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/
// clazy:excludeall=qstring-arg
#include "DurationTester.h"
#include <kptduration.h>

#include <QTest>

namespace KPlato
{

void DurationTester::add() {
    Duration d1(0, 2, 0);
    Duration d2(1, 0, 0);
    
    QVERIFY((d1+d1) == Duration(0, 4, 0));
}
void DurationTester::subtract() {
    Duration d1(0, 2, 0);
    Duration d2(1, 0, 0);
    
    QVERIFY((d1-d1) == Duration(0, 0, 0));
    QVERIFY((d2-d1) == Duration(0, 22, 0));
    QVERIFY((d1-d2) == Duration::zeroDuration); // underflow, return 0
}
void DurationTester::divide() {
    Duration d1(0, 2, 0);
    
    QVERIFY((d1/2) == Duration(0, 1, 0));
}
void DurationTester::equal() {
    Duration d1(0, 2, 0);
    
    QVERIFY(d1==d1);
}
void DurationTester::lessThanOrEqual() {
    Duration d1(0, 2, 0);
    Duration d2(1, 0, 0);
    
    QVERIFY(d1<=d1);
    QVERIFY(d1<=d2);
}
void DurationTester::greaterThanOrEqual() {
    Duration d1(0, 2, 0);
    Duration d2(1, 0, 0);
    
    QVERIFY(d1>=d1);
    QVERIFY(d2>=d1);
}
void DurationTester::notEqual() {
    Duration d1(0, 2, 0);
    Duration d2(1, 0, 0);
    
    QVERIFY(!(d1!=d1));
    QVERIFY(d1!=d2);
    QVERIFY(d2!=d1);
}
void DurationTester::greaterThan() {
    Duration d1(0, 2, 0);
    Duration d2(1, 0, 0);
    
    QVERIFY(d2>d1);
    QVERIFY(d1 > 1*60*60*1000);
}
void DurationTester::lessThan() {
    Duration d1(0, 2, 0);
    Duration d2(1, 0, 0);
    
    QVERIFY(d1<d2);
    QVERIFY(d1 < 3*60*60*1000);
}

} //namespace KPlato

QTEST_GUILESS_MAIN(KPlato::DurationTester)
