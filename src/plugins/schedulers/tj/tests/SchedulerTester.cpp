/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2007-2011 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/
// clazy:excludeall=qstring-arg
#include "SchedulerTester.h"

#include "PlanTJScheduler.h"

#include "kptcommand.h"
#include "kptcalendar.h"
#include "kptdatetime.h"
#include "kptresource.h"
#include "kptnode.h"
#include "kpttask.h"
#include "kptschedule.h"
#include "kptxmlloaderobject.h"

#include <QTest>

#include "tests/DateTimeTester.h"

#include "tests/debug.cpp"

namespace KPlato
{

QStringList SchedulerTester::data()
{
    return QStringList()
            << "test1.plan"
            << "startnotearlier.plan"
            ;
}

void SchedulerTester::loadDocument(const QString &dir, const QString &fname, KoXmlDocument &doc) const
{
    QFile file(dir + fname);
    QVERIFY2(file.open(QIODevice::ReadOnly), fname.toLatin1());

    QString error;
    bool setContent;
    int line, column;
    if (! (setContent = doc.setContent(&file, &error, &line, &column))) {
        file.close();
        QString s = QString("%1: %2 Line %3, column %4").arg(fname).arg(error).arg(line).arg(column);
        QVERIFY2(setContent, s.toLatin1());
    }
}

void SchedulerTester::testSingle()
{
    QString dir = QFINDTESTDATA("data/");
    const auto lst = data();
    for (const QString &fname : lst) {
        qDebug()<<"Testing file:"<<fname;
        KoXmlDocument doc;
        loadDocument(dir, fname, doc);

        testProject(fname, doc);
    }
}

void SchedulerTester::testProject(const QString &fname, const KoXmlDocument &doc)
{
    KoXmlElement pel = doc.documentElement().namedItem("project").toElement();
    if (pel.isNull()) {
        const QString s = QString("%1: Cannot find 'project' element").arg(fname);
        QVERIFY2(!pel.isNull(), s.toLatin1());
    }
    Project project;
    project.setTimeZone(QTimeZone("UTC"));

    XMLLoaderObject status;
    status.setProject(&project);
    status.setVersion(doc.documentElement().attribute("version", PLAN_FILE_SYNTAX_VERSION));
    bool projectLoad = status.loadProject(&project, doc);
    if (! projectLoad) {
        const QString s = QString("%1: Failed to load project").arg(fname);
        QVERIFY2(projectLoad, s.toLatin1());        
    }
    QString s = project.description();
    if (! s.isEmpty()) {
        qDebug();
        qDebug()<<project.description();
        qDebug();
    }
    //Debug::print(&project, "Before calculation ------------", true);
    ScheduleManager *manager = project.scheduleManagers().value(0);
    s = QString("%1: No schedule to compare with").arg(fname);
    QVERIFY2(manager, s.toLatin1());

    ScheduleManager *sm = project.createScheduleManager("Test Plan");
    project.addScheduleManager(sm);

    PlanTJScheduler tj(&project, sm, 5*60*1000);
    qDebug() << "+++++++++++++++++++++++++++calculate-start";
    tj.doRun();
    tj.updateProject(tj.project(), tj.manager(), &project, sm);
    qDebug() << "+++++++++++++++++++++++++++calculate-end";
    //Debug::print(&project, "After calculation ------------", true);
    if (sm->calculationResult() != ScheduleManager::CalculationDone) {
        Debug::printSchedulingLog(*sm, "----");
    }
    s = QString("%1: Scheduling failed").arg(fname);
    QVERIFY2(sm->calculationResult() == ScheduleManager::CalculationDone, s.toLatin1());

    long id1 = manager->scheduleId();
    long id2 = sm->scheduleId();
    qDebug()<<"Project start, finish:"<<project.startTime(id1)<<project.startTime(id2)<<project.timeZone();
    s = QString("%1: Compare project schedules:\n Expected: %2\n   Result: %3")
            .arg(fname)
            .arg(project.startTime(id1).toString(Qt::ISODate))
            .arg(project.startTime(id2).toString(Qt::ISODate));
    QVERIFY2(project.startTime(id1) == project.startTime(id2), s.toLatin1()); 
    const auto lst = project.allNodes();
    for (Node *n : lst) {
        compare(fname, n, id1, id2);
    }
}

void SchedulerTester::compare(const QString &fname, Node *n, long id1, long id2)
{
    QString s = QString("%1: '%2' Compare task schedules:\n Expected: %3\n   Result: %4").arg(fname).arg(n->name());
    QVERIFY2(n->startTime(id1) == n->startTime(id2), (s.arg(n->startTime(id1).toString(Qt::ISODate)).arg(n->startTime(id2).toString(Qt::ISODate))).toLatin1());
    QVERIFY2(n->endTime(id1) == n->endTime(id2), (s.arg(n->endTime(id1).toString(Qt::ISODate)).arg(n->endTime(id2).toString(Qt::ISODate))).toLatin1());
}

void SchedulerTester::loadDocuments(QString &dir, QList<QString> files, QList<KoXmlDocument> &docs) const
{
    for (const QString &fname : files) {
        QFile file(dir + fname);
        QVERIFY2(file.open(QIODevice::ReadOnly), fname.toLatin1());
        KoXmlDocument doc;
        loadDocument(dir, fname, doc);
        docs << doc;
    }
}

} //namespace KPlato

QTEST_GUILESS_MAIN(KPlato::SchedulerTester)
