/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2022 Dag Andersen <dag.andersen@kdemail.net>
   
   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "RelationTester.h"

#include "kptcommand.h"
#include "kptcalendar.h"
#include "kptresource.h"
#include "kptnode.h"
#include "kpttask.h"
#include "kptschedule.h"

#include <QTest>

#include "debug.cpp"


namespace KPlato
{

void RelationTester::initTestCase()
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
}

void RelationTester::cleanupTestCase()
{
    delete m_project;
}


void RelationTester::proxyRelations()
{
    QDate today = QDate::fromString(QStringLiteral("2012-02-01"), Qt::ISODate);

    Resource *r = new Resource();
    r->setName(QStringLiteral("R1"));
    r->setCalendar(m_calendar);
    m_project->addResource(r);

    Task *t1 = m_project->createTask();
    t1->setName(QStringLiteral("t1"));
    m_project->addSubTask(t1, m_project);
    t1->estimate()->setUnit(Duration::Unit_h);
    t1->estimate()->setExpectedEstimate(8.0);
    t1->estimate()->setType(Estimate::Type_Effort);

    Task *t2 = m_project->createTask();
    t2->setName(QStringLiteral("t2"));
    m_project->addSubTask(t2, m_project);
    t2->estimate()->setUnit(Duration::Unit_h);
    t2->estimate()->setExpectedEstimate(8.0);
    t2->estimate()->setType(Estimate::Type_Effort);

    Task *t3 = m_project->createTask();
    t3->setName(QStringLiteral("t3"));
    m_project->addSubTask(t3, m_project);
    t3->estimate()->setUnit(Duration::Unit_h);
    t3->estimate()->setExpectedEstimate(8.0);
    t3->estimate()->setType(Estimate::Type_Effort);

    Task *t4 = m_project->createTask();
    t4->setName(QStringLiteral("t4"));
    m_project->addSubTask(t4, m_project);
    t4->estimate()->setUnit(Duration::Unit_h);
    t4->estimate()->setExpectedEstimate(8.0);
    t4->estimate()->setType(Estimate::Type_Effort);

    Task *s1 = m_project->createTask();
    s1->setName(QStringLiteral("s1"));
    m_project->addTask(s1, m_project);

    Task *s2 = m_project->createTask();
    s2->setName(QStringLiteral("s2"));
    m_project->addTask(s2, m_project);

    Task *s1_t1 = m_project->createTask();
    s1_t1->setName(QStringLiteral("s1_t1"));
    m_project->addSubTask(s1_t1, s1);
    s1_t1->estimate()->setUnit(Duration::Unit_h);
    s1_t1->estimate()->setExpectedEstimate(8.0);
    s1_t1->estimate()->setType(Estimate::Type_Effort);

    Task *s1_t2 = m_project->createTask();
    s1_t2->setName(QStringLiteral("s1_t2"));
    m_project->addSubTask(s1_t2, s1);
    s1_t2->estimate()->setUnit(Duration::Unit_h);
    s1_t2->estimate()->setExpectedEstimate(8.0);
    s1_t2->estimate()->setType(Estimate::Type_Effort);

    Task *s2_t1 = m_project->createTask();
    s2_t1->setName(QStringLiteral("s2_t1"));
    m_project->addSubTask(s2_t1, s2);
    s2_t1->estimate()->setUnit(Duration::Unit_h);
    s2_t1->estimate()->setExpectedEstimate(8.0);
    s2_t1->estimate()->setType(Estimate::Type_Effort);

    Task *s2_t2 = m_project->createTask();
    s2_t2->setName(QStringLiteral("s2_t2"));
    m_project->addSubTask(s2_t2, s2);
    s2_t2->estimate()->setUnit(Duration::Unit_h);
    s2_t2->estimate()->setExpectedEstimate(8.0);
    s2_t2->estimate()->setType(Estimate::Type_Effort);

    Task *s2_s1 = m_project->createTask();
    s2_s1->setName(QStringLiteral("s2_s1"));
    m_project->addSubTask(s2_s1, s2);

    Task *s2_s1_t1 = m_project->createTask();
    s2_s1_t1->setName(QStringLiteral("s2_s1_t1"));
    m_project->addSubTask(s2_s1_t1, s2_s1);
    s2_s1_t1->estimate()->setUnit(Duration::Unit_h);
    s2_s1_t1->estimate()->setExpectedEstimate(8.0);
    s2_s1_t1->estimate()->setType(Estimate::Type_Effort);

    ResourceRequest *rr = new ResourceRequest(r, 100);
    t1->requests().addResourceRequest(rr);
    t1->estimate()->setType(Estimate::Type_Effort);

    rr = new ResourceRequest(r, 100);
    t2->requests().addResourceRequest(rr);
    t2->estimate()->setType(Estimate::Type_Effort);

    rr = new ResourceRequest(r, 100);
    t3->requests().addResourceRequest(rr);
    t3->estimate()->setType(Estimate::Type_Effort);

    rr = new ResourceRequest(r, 100);
    t4->requests().addResourceRequest(rr);
    t4->estimate()->setType(Estimate::Type_Effort);

    rr = new ResourceRequest(r, 100);
    s1_t1->requests().addResourceRequest(rr);
    s1_t1->estimate()->setType(Estimate::Type_Effort);

    rr = new ResourceRequest(r, 100);
    s1_t2->requests().addResourceRequest(rr);
    s1_t2->estimate()->setType(Estimate::Type_Effort);

    rr = new ResourceRequest(r, 100);
    s2_t1->requests().addResourceRequest(rr);
    s2_t1->estimate()->setType(Estimate::Type_Effort);

    rr = new ResourceRequest(r, 100);
    s2_t2->requests().addResourceRequest(rr);
    s2_t2->estimate()->setType(Estimate::Type_Effort);

    auto s = QStringLiteral("Check predesseccor proxies -----------------------------------");
    qDebug()<<"Testing:"<<s;
    // t1 -> s1
    //    -> t2 -> s1
    s1->addDependParentNode(t1);
    s1->addDependParentNode(t2);
    t2->addDependParentNode(t1);

    //Debug::print(m_project, s, true);
    bool checkProxy = true;
    bool checkSummarytasks = true;

    m_project->setConstraintStartTime(DateTime(today, QTime(), m_project->timeZone()));
    auto sm = m_project->createScheduleManager(QStringLiteral("Test Plan"));
    m_project->addScheduleManager(sm);
    sm->createSchedules();

    QVERIFY(t1->isPredecessorof(t2, !checkProxy, !checkSummarytasks));
    QVERIFY(t1->isPredecessorof(s1, !checkProxy, checkSummarytasks));
    QVERIFY(!t1->isPredecessorof(s1_t1, !checkProxy, checkSummarytasks));
    QVERIFY(!t1->isPredecessorof(s1_t2, !checkProxy, checkSummarytasks));
    QVERIFY(!t2->isPredecessorof(s1_t1, !checkProxy, checkSummarytasks));
    QVERIFY(!t2->isPredecessorof(s1_t2, !checkProxy, !checkSummarytasks));

    QVERIFY(!s1_t1->isPredecessorof(s1_t2, !checkProxy, !checkSummarytasks));
    QVERIFY(!s1_t2->isSuccessorof(s1_t1, !checkProxy, !checkSummarytasks));

    m_project->initiateCalculationLists(*sm->expected()); // generates proxies
    Debug::printRelations(m_project);

    QVERIFY2(t1->isPredecessorof(t2, !checkProxy, !checkSummarytasks), "t1 should be direct predecessor of t2");
    QVERIFY2(t2->isSuccessorof(t1, !checkProxy, !checkSummarytasks), "t2 should be direct successor of t1");

    QVERIFY2(t2->isPredecessorof(s1, !checkProxy, checkSummarytasks), "t2 should be direct predecessor of s1");
    QVERIFY2(s1->isSuccessorof(t2, !checkProxy, checkSummarytasks), "s1 should be direct successor of t2");

    QVERIFY2(t1->isPredecessorof(s1, !checkProxy, checkSummarytasks), "t1 should be predecessor of s1, via t2");
    QVERIFY2(s1->isSuccessorof(t1, !checkProxy, checkSummarytasks), "s1 should be successor of t1, via t2");

    QVERIFY2(!t2->isPredecessorof(s1_t1, !checkProxy, checkSummarytasks), "t2 not should not be direct predecessor of s1_t1");
    QVERIFY2(t2->isPredecessorof(s1_t1, checkProxy, !checkSummarytasks), "t2 should be proxy predecessor of s1_t1");
    QVERIFY2(s1_t1->isSuccessorof(t2, checkProxy, !checkSummarytasks), "s1_t1 should be proxy successor of t2");

    QVERIFY2(!t2->isPredecessorof(s1_t2, !checkProxy, checkSummarytasks), "t2 not should not be direct predecessor of s1_t2");
    QVERIFY2(t2->isPredecessorof(s1_t2, checkProxy, !checkSummarytasks), "t2 should be proxy predecessor of s1_t2");
    QVERIFY2(s1_t2->isSuccessorof(t2, checkProxy, !checkSummarytasks), "s1_t2 should be proxy successor of t2");

    QVERIFY2(!t1->isPredecessorof(s1_t1, !checkProxy, checkSummarytasks), "t1 not should not be direct predecessor of s1_t1");
    QVERIFY2(t1->isPredecessorof(s1_t1, checkProxy, !checkSummarytasks), "t1 should be proxy predecessor of s1_t1");
    QVERIFY2(s1_t1->isSuccessorof(t1, checkProxy, !checkSummarytasks), "s1_t1 should be proxy successor of t1");

    QVERIFY2(!t1->isPredecessorof(s1_t2, !checkProxy, checkSummarytasks), "t1 not should not be direct predecessor of s1_t2");
    QVERIFY2(t1->isPredecessorof(s1_t2, checkProxy, !checkSummarytasks), "t1 should be proxy predecessor of s1_t2");
    QVERIFY2(s1_t2->isSuccessorof(t1, checkProxy, !checkSummarytasks), "s1_t2 should be proxy successor of t1");


    s1_t2->addDependParentNode(s1_t1);
    m_project->initiateCalculationLists(*sm->expected()); // generates proxies
    Debug::printRelations(m_project);

    QVERIFY2(t1->isPredecessorof(t2, !checkProxy, !checkSummarytasks), "t1 should be direct predecessor of t2");
    QVERIFY2(t2->isSuccessorof(t1, !checkProxy, !checkSummarytasks), "t2 should be direct successor of t1");

    QVERIFY2(t2->isPredecessorof(s1, !checkProxy, checkSummarytasks), "t2 should be direct predecessor of s1");
    QVERIFY2(s1->isSuccessorof(t2, !checkProxy, checkSummarytasks), "s1 should be direct successor of t2");

    QVERIFY2(t1->isPredecessorof(s1, !checkProxy, checkSummarytasks), "t1 should be predecessor of s1, via t2");
    QVERIFY2(s1->isSuccessorof(t1, !checkProxy, checkSummarytasks), "s1 should be successor of t1, via t2");

    QVERIFY2(!t2->isPredecessorof(s1_t1, !checkProxy, checkSummarytasks), "t2 not should not be direct predecessor of s1_t1");
    QVERIFY2(t2->isPredecessorof(s1_t1, checkProxy, !checkSummarytasks), "t2 should be proxy predecessor of s1_t1");
    QVERIFY2(s1_t1->isSuccessorof(t2, checkProxy, !checkSummarytasks), "s1_t1 should be proxy successor of t2");

    QVERIFY2(!t2->isPredecessorof(s1_t2, !checkProxy, checkSummarytasks), "t2 not should not be direct predecessor of s1_t2");
    QVERIFY2(t2->isPredecessorof(s1_t2, checkProxy, !checkSummarytasks), "t2 should be proxy predecessor of s1_t2");
    QVERIFY2(s1_t2->isSuccessorof(t2, checkProxy, !checkSummarytasks), "s1_t2 should be proxy successor of t2");

    QVERIFY2(!t1->isPredecessorof(s1_t1, !checkProxy, checkSummarytasks), "t1 not should not be direct predecessor of s1_t1");
    QVERIFY2(t1->isPredecessorof(s1_t1, checkProxy, !checkSummarytasks), "t1 should be proxy predecessor of s1_t1");
    QVERIFY2(s1_t1->isSuccessorof(t1, checkProxy, !checkSummarytasks), "s1_t1 should be proxy successor of t1");

    QVERIFY2(!t1->isPredecessorof(s1_t2, !checkProxy, checkSummarytasks), "t1 not should not be direct predecessor of s1_t2");
    QVERIFY2(t1->isPredecessorof(s1_t2, checkProxy, !checkSummarytasks), "t1 should be proxy predecessor of s1_t2 and s1_t1");
    QVERIFY2(s1_t2->isSuccessorof(t1, checkProxy, !checkSummarytasks), "s1_t2 should be proxy successor of t1");

    s2->addDependParentNode(s1);
    m_project->initiateCalculationLists(*sm->expected()); // generates proxies
    Debug::printRelations(m_project);

    QVERIFY2(s1_t2->isPredecessorof(s2_t1, checkProxy, !checkSummarytasks), "s1_t2 should be proxy predecessor of s2_t1");
    QVERIFY2(s2_t1->isSuccessorof(s1_t2, checkProxy, !checkSummarytasks), "s2_t1 should be proxy successor of s1_t2");

    QVERIFY2(t1->isPredecessorof(s2_t1, checkProxy, !checkSummarytasks), "t1 should be proxy predecessor of s2_t1");
    QVERIFY2(s2_t1->isSuccessorof(t1, checkProxy, !checkSummarytasks), "s2_t1 should be proxy successor of t1");

    QVERIFY2(t1->isPredecessorof(s2_t2, checkProxy, !checkSummarytasks), "t1 should be proxy predecessor of s2_t2");
    QVERIFY2(s2_t2->isSuccessorof(t1, checkProxy, !checkSummarytasks), "s2_t2 should be proxy successor of t1");

    m_project->initiateCalculationLists(*sm->expected()); // generates proxies
    Debug::printRelations(m_project);
}

} //namespace KPlato

QTEST_GUILESS_MAIN(KPlato::RelationTester)
