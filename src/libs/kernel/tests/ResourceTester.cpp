/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2006-2010 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/
// clazy:excludeall=qstring-arg
#include "ResourceTester.h"
#include "DateTimeTester.h"

#include <kptresource.h>
#include <kptcalendar.h>
#include <AddResourceCmd.h>
#include <AddParentGroupCmd.h>
#include <kptcommand.h>
#include <kptdatetime.h>
#include <kptduration.h>
#include <kptmap.h>


#include <QTest>

#include "kptglobal.h"
#include "kptxmlloaderobject.h"
#include "XmlSaveContext.h"

#include <KoXmlReader.h>

#include "debug.cpp"

#include <QDomDocument>
#include <QDomElement>

namespace KPlato
{

void ResourceTester::testAvailable()
{
    Resource r;
    QVERIFY(! r.availableFrom().isValid());
    QVERIFY(! r.availableUntil().isValid());
    
    QDateTime qt = QDateTime::currentDateTime();
    DateTime dt = DateTime(qt);
    qDebug()<<"dt"<<dt;
    
    r.setAvailableFrom(qt);
    Debug::print(&r, QStringLiteral("Test setAvailableFrom with QDateTime"));
    DateTime x = r.availableFrom();
    qDebug()<<"------"<<x;
    QCOMPARE(x, dt);
    qDebug()<<"----------------";
    r.setAvailableUntil(qt.addDays(1));
    Debug::print(&r, QStringLiteral("Test setAvailableUntil with QDateTime"));
    QCOMPARE(r.availableUntil(), DateTime(dt.addDays(1)));
    qDebug()<<"----------------";
}

void ResourceTester::testSingleDay() {
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
    
    Resource r;
    r.setAvailableFrom(before);
    r.setAvailableUntil(after);
    r.setCalendar(&t);
    
    Debug::print(&r, QStringLiteral("Test single resource, no group, no project"), true);

    QVERIFY(r.availableAfter(after, DateTime(after.addDays(1))).isValid() == false);
    QVERIFY(r.availableBefore(before, DateTime(before.addDays(-1))).isValid() == false);
    
    QVERIFY(r.availableAfter(before, after).isValid());
    
    QVERIFY((r.availableAfter(after, DateTime(after.addDays(10)))).isValid() == false);
    QVERIFY((r.availableBefore(before, DateTime(before.addDays(-10)))).isValid() == false);
    
    QCOMPARE(r.availableAfter(before,after), wdt1);
    QCOMPARE(r.availableBefore(after, before), wdt2);
    
    Duration e(0, 2, 0);
    QCOMPARE(r.effort(before, Duration(2, 0, 0)).toString(), e.toString());

}

void ResourceTester::team()
{
    {
        Project p1;

        AddResourceGroupCmd *c1 = new AddResourceGroupCmd(&p1, new ResourceGroup());
        c1->redo();
        QCOMPARE(p1.resourceGroups().count(), 1);
        ResourceGroup *g = p1.resourceGroups().at(0);
        delete c1;

        AddResourceCmd *c2 = new AddResourceCmd(&p1, new Resource());
        c2->redo();
        QCOMPARE(p1.resourceList().count(), 1);
        Resource *r1 = p1.resourceList().at(0);
        AddParentGroupCmd *c21 = new AddParentGroupCmd(r1, g);
        c21->redo();
        QCOMPARE(g->resources().count(), 1);
        QCOMPARE(g->resourceAt(0), r1);
        delete c2;
        delete c21;
        c2 = new AddResourceCmd(&p1, new Resource());
        c2->redo();
        QCOMPARE(p1.resourceList().count(), 2);
        Resource *r2 = p1.resourceList().at(1);
        c21 = new AddParentGroupCmd(r2, g);
        c21->redo();
        QCOMPARE(g->resources().count(), 2);
        QCOMPARE(g->resourceAt(1), r2);
        delete c2;
        delete c21;
        c2 = new AddResourceCmd(&p1, new Resource());
        c2->redo();
        QCOMPARE(p1.resourceList().count(), 3);
        Resource *r3 = p1.resourceList().at(2);
        c21 = new AddParentGroupCmd(r3, g);
        c21->redo();
        QCOMPARE(g->resources().count(), 3);
        QCOMPARE(g->resourceAt(2), r3);
        delete c2;
        delete c21;

        AddResourceTeamCmd *c3 = new AddResourceTeamCmd(r1, r2->id());
        c3->redo();
        delete c3;
        QCOMPARE(r1->teamMemberIds().count(), 1);
        QCOMPARE(r1->teamMembers().count(), 1);
        QCOMPARE(r1->teamMembers().at(0), r2);

        c3 = new AddResourceTeamCmd(r1, r3->id());
        c3->redo();
        delete c3;
        QCOMPARE(r1->teamMemberIds().count(), 2);
        QCOMPARE(r1->teamMembers().count(), 2);
        QCOMPARE(r1->teamMembers().at(1), r3);

        RemoveResourceTeamCmd *c4 = new RemoveResourceTeamCmd(r1, r2->id());
        c4->redo();
        delete c4;
        QCOMPARE(r1->teamMemberIds().count(), 1);
        QCOMPARE(r1->teamMembers().count(), 1);
        QCOMPARE(r1->teamMembers().at(0), r3);
    }

    {
        Project p1;
        p1.setId(QStringLiteral("p1"));

        AddResourceGroupCmd *c1 = new AddResourceGroupCmd(&p1, new ResourceGroup());
        c1->redo();
        ResourceGroup *g = p1.resourceGroups().at(0);
        QVERIFY(g);
        delete c1;

        Resource *r1 = new Resource();
        r1->setType(Resource::Type_Team);
        AddResourceCmd *c2 = new AddResourceCmd(&p1, r1);
        c2->redo();
        AddParentGroupCmd *c21 = new AddParentGroupCmd(r1, g);
        c21->redo();
        r1 = g->resourceAt(0);
        QVERIFY(r1);
        delete c2;
        delete c21;
        Resource *r2 = new Resource();
        c2 = new AddResourceCmd(&p1, r2);
        c2->redo();
        c21 = new AddParentGroupCmd(r2, g);
        c21->redo();
        QCOMPARE(g->resourceAt(1), r2);
        delete c2;
        delete c21;
        Resource *r3 = new Resource();
        c2 = new AddResourceCmd(&p1, r3);
        c2->redo();
        c21 = new AddParentGroupCmd(r3, g);
        c21->redo();
        QCOMPARE(g->resourceAt(2), r3);
        delete c2;
        delete c21;

        AddResourceTeamCmd *c3 = new AddResourceTeamCmd(r1, r2->id());
        c3->redo();
        delete c3;
        QCOMPARE(r1->teamMemberIds().count(), 1);
        QCOMPARE(r1->teamMembers().count(), 1);
        QCOMPARE(r1->teamMembers().at(0), r2);

        c3 = new AddResourceTeamCmd(r1, r3->id());
        c3->redo();
        delete c3;
        QCOMPARE(r1->teamMemberIds().count(), 2);
        QCOMPARE(r1->teamMembers().count(), 2);
        QCOMPARE(r1->teamMembers().at(1), r3);

        // copy
        Project p2;

        c1 = new AddResourceGroupCmd(&p2, new ResourceGroup(g));
        c1->redo();
        ResourceGroup *g2 = p2.resourceGroups().at(0);
        QVERIFY(g2);
        delete c1;

        c2 = new AddResourceCmd(&p2, new Resource(r1));
        c2->redo();
        QCOMPARE(p2.resourceList().count(), 1);
        Resource *r11 = p2.resourceList().at(0);
        c21 = new AddParentGroupCmd(r11, g2);
        c21->redo();
        QCOMPARE(g2->resources().count(), 1);
        QCOMPARE(g2->resourceAt(0), r11);
        delete c2;
        delete c21;
        c2 = new AddResourceCmd(&p2, new Resource(r2));
        c2->redo();
        QCOMPARE(p2.resourceList().count(), 2);
        Resource *r12 = p2.resourceList().at(1);
        c21 = new AddParentGroupCmd(r12, g2);
        c21->redo();
        QCOMPARE(g2->resources().count(), 2);
        QCOMPARE(g2->resourceAt(1), r12);
        delete c2;
        delete c21;
        c2 = new AddResourceCmd(&p2, new Resource(r3));
        c2->redo();
        QCOMPARE(p2.resourceList().count(), 3);
        Resource *r13 = p2.resourceList().at(2);
        c21 = new AddParentGroupCmd(r13, g2);
        c21->redo();
        QCOMPARE(g2->resources().count(), 3);
        QCOMPARE(g2->resourceAt(2), r13);
        delete c2;
        delete c21;
        
        QCOMPARE(r11->teamMemberIds().count(), 2);
        QCOMPARE(r11->teamMembers().count(), 2);
        QCOMPARE(r11->teamMembers().at(0), r12);
        QCOMPARE(r11->teamMembers().at(1), r13);

        // xml
        XmlSaveContext context(&p1);
        context.save();

        KoXmlDocument xdoc;
        xdoc.setContent(context.document.toByteArray());
        XMLLoaderObject sts;

        Project p3;
        sts.loadProject(&p3, xdoc);

        QCOMPARE(p3.numResourceGroups(), 1);
        ResourceGroup *g3 = p3.resourceGroupAt(0);
        QCOMPARE(g3->numResources(), 3);
        Resource *r21 = g3->resourceAt(0);
        QCOMPARE(r21->type(), Resource::Type_Team);
        QCOMPARE(r21->teamMemberIds().count(), 2);
        QCOMPARE(r21->teamMembers().count(), 2);
        QCOMPARE(r21->teamMembers().at(0), g3->resourceAt(1));
        QCOMPARE(r21->teamMembers().at(1), g3->resourceAt(2));
    }
    {
        // team members in different group
        Project p1;
        p1.setId(QStringLiteral("p1"));

        AddResourceGroupCmd *c1 = new AddResourceGroupCmd(&p1, new ResourceGroup());
        c1->redo();
        ResourceGroup *g = p1.resourceGroups().at(0);
        QVERIFY(g);
        delete c1;
        ResourceGroup *mg = new ResourceGroup();
        c1 = new AddResourceGroupCmd(&p1, mg);
        c1->redo();
        QCOMPARE(mg, p1.resourceGroups().at(1));
        delete c1;

        Resource *r1 = new Resource();
        r1->setType(Resource::Type_Team);
        AddResourceCmd *c2 = new AddResourceCmd(&p1, r1);
        c2->redo();
        AddParentGroupCmd *c21 = new AddParentGroupCmd(r1, g);
        c21->redo();
        QCOMPARE(r1, g->resourceAt(0));
        delete c2;
        delete c21;
        Resource *r2 = new Resource();
        c2 = new AddResourceCmd(&p1, r2);
        c2->redo();
        c21 = new AddParentGroupCmd(r2, mg);
        c21->redo();
        QCOMPARE(r2, mg->resourceAt(0));
        delete c2;
        delete c21;
        Resource *r3 = new Resource();
        c2 = new AddResourceCmd(&p1, r3);
        c2->redo();
        c21 = new AddParentGroupCmd(r3, mg);
        c21->redo();
        QCOMPARE(r3, mg->resourceAt(1));
        delete c2;
        delete c21;
        AddResourceTeamCmd *c3 = new AddResourceTeamCmd(r1, r2->id());
        c3->redo();
        delete c3;
        QCOMPARE(r1->teamMemberIds().count(), 1);
        QCOMPARE(r1->teamMembers().count(), 1);
        QCOMPARE(r1->teamMembers().at(0), r2);

        c3 = new AddResourceTeamCmd(r1, r3->id());
        c3->redo();
        delete c3;
        QCOMPARE(r1->teamMemberIds().count(), 2);
        QCOMPARE(r1->teamMembers().count(), 2);
        QCOMPARE(r1->teamMembers().at(0), r2);
        QCOMPARE(r1->teamMembers().at(1), r3);

        // copy
        Project p2;

        c1 = new AddResourceGroupCmd(&p2, new ResourceGroup(g));
        c1->redo();
        ResourceGroup *g2 = p2.resourceGroups().at(0);
        QVERIFY(g2);
        delete c1;

        c1 = new AddResourceGroupCmd(&p2, new ResourceGroup(mg));
        c1->redo();
        ResourceGroup *mg2 = p2.resourceGroups().at(1);
        QVERIFY(mg2);
        delete c1;

        c2 = new AddResourceCmd(&p2, new Resource(r1));
        c2->redo();
        Resource *r11 = p2.resourceList().at(0);
        c21 = new AddParentGroupCmd(r11, g2);
        c21->redo();
        QCOMPARE(r11, g2->resourceAt(0));
        delete c2;
        delete c21;
        c2 = new AddResourceCmd(&p2, new Resource(r2));
        c2->redo();
        Resource *r12 = p2.resourceList().at(1);
        c21 = new AddParentGroupCmd(r12, mg2);
        c21->redo();
        QCOMPARE(r12, mg2->resourceAt(0));
        delete c2;
        delete c21;
        c2 = new AddResourceCmd(&p2, new Resource(r3));
        c2->redo();
        Resource *r13 = p2.resourceList().at(2);
        c21 = new AddParentGroupCmd(r13, mg2);
        c21->redo();
        QCOMPARE(r13, mg2->resourceAt(1));
        delete c2;
        delete c21;
        
        QCOMPARE(r11->teamMemberIds().count(), 2);
        QCOMPARE(r11->teamMembers().count(), 2);
        QCOMPARE(r11->teamMembers().at(0), r12);
        QCOMPARE(r11->teamMembers().at(1), r13);

        // xml
        XmlSaveContext context(&p1);
        context.save();

        KoXmlDocument xdoc;
        xdoc.setContent(context.document.toByteArray());

        XMLLoaderObject sts;
        Project p3;
        sts.loadProject(&p3, xdoc);

        QCOMPARE(p3.numResourceGroups(), p1.numResourceGroups());
        ResourceGroup *g3 = p3.resourceGroupAt(0);
        QCOMPARE(g3->numResources(), 1);
        ResourceGroup *mg3 = p3.resourceGroupAt(1);
        QCOMPARE(mg3->numResources(), 2);
        Resource *r21 = g3->resourceAt(0);
        QCOMPARE(r21->type(), Resource::Type_Team);
        QCOMPARE(r21->teamMemberIds().count(), 2);
        QCOMPARE(r21->teamMembers().count(), 2);
        QCOMPARE(r21->teamMembers().at(0), mg3->resourceAt(0));
        QCOMPARE(r21->teamMembers().at(1), mg3->resourceAt(1));
    }

}

void ResourceTester::required()
{
    Project p;
    AddResourceGroupCmd *c1 = new AddResourceGroupCmd(&p, new ResourceGroup());
    c1->redo();

    ResourceGroup *g = p.resourceGroups().at(0);
    QVERIFY(g);
    delete c1;

    AddResourceCmd *c2 = new AddResourceCmd(&p, new Resource());
    c2->redo();
    QCOMPARE(p.resourceList().count(), 1);
    Resource *r1 = p.resourceList().at(0);
    AddParentGroupCmd *c21 = new AddParentGroupCmd(r1, g);
    c21->redo();
    QCOMPARE(g->resources().count(), 1);
    QCOMPARE(g->resourceAt(0), r1);
    delete c2;
    delete c21;
    c2 = new AddResourceCmd(&p, new Resource());
    c2->redo();
    QCOMPARE(p.resourceList().count(), 2);
    Resource *r2 = p.resourceList().at(1);
    c21 = new AddParentGroupCmd(r2, g);
    c21->redo();
    QCOMPARE(g->resources().count(), 2);
    QCOMPARE(g->resourceAt(1), r2);
    delete c2;
    delete c21;
    c2 = new AddResourceCmd(&p, new Resource());
    c2->redo();
    QCOMPARE(p.resourceList().count(), 3);
    Resource *r3 = p.resourceList().at(2);
    c21 = new AddParentGroupCmd(r3, g);
    c21->redo();
    QCOMPARE(g->resources().count(), 3);
    QCOMPARE(g->resourceAt(2), r3);
    delete c2;
    delete c21;

    QVERIFY(r1->requiredIds().isEmpty());
    QVERIFY(r1->requiredResources().isEmpty());

    r1->addRequiredId(QString()); // not allowed to add empty id
    QVERIFY(r1->requiredIds().isEmpty());

    r1->addRequiredId(r2->id());
    QCOMPARE(r1->requiredIds().count(), 1);
    QCOMPARE(r1->requiredResources().count(), 1);
    QCOMPARE(r1->requiredResources().at(0), r2);

    r1->addRequiredId(r3->id());
    QCOMPARE(r1->requiredIds().count(), 2);
    QCOMPARE(r1->requiredResources().count(), 2);
    QCOMPARE(r1->requiredResources().at(0), r2);
    QCOMPARE(r1->requiredResources().at(1), r3);

    r1->addRequiredId(r2->id()); // not allowed to add existing id
    QCOMPARE(r1->requiredIds().count(), 2);
    QCOMPARE(r1->requiredResources().count(), 2);
    QCOMPARE(r1->requiredResources().at(0), r2);
    QCOMPARE(r1->requiredResources().at(1), r3);

    QStringList lst;
    r1->setRequiredIds(lst);
    QCOMPARE(r1->requiredIds().count(), 0);
    QCOMPARE(r1->requiredIds().count(), 0);

    lst << r2->id() << r3->id();
    r1->setRequiredIds(lst);
    QCOMPARE(r1->requiredIds().count(), 2);
    QCOMPARE(r1->requiredResources().count(), 2);
    QCOMPARE(r1->requiredResources().at(0), r2);
    QCOMPARE(r1->requiredResources().at(1), r3);

    // copy to different project
    Project p2;
    c1 = new AddResourceGroupCmd(&p2, new ResourceGroup(g));
    c1->redo();
    delete c1;

    ResourceGroup *g1 = p2.resourceGroupAt(0);

    c2 = new AddResourceCmd(&p2, new Resource(r1));
    c2->redo();
    QCOMPARE(p2.resourceList().count(), 1);
    Resource *r4 = p2.resourceList().at(0);
    c21 = new AddParentGroupCmd(r4, g1);
    c21->redo();
    QCOMPARE(g1->resources().count(), 1);
    QCOMPARE(g1->resourceAt(0), r4);
    delete c2;
    delete c21;
    c2 = new AddResourceCmd(&p2, new Resource(r2));
    c2->redo();
    QCOMPARE(p2.resourceList().count(), 2);
    Resource *r5 = p2.resourceList().at(1);
    c21 = new AddParentGroupCmd(r5, g1);
    c21->redo();
    QCOMPARE(g1->resources().count(), 2);
    QCOMPARE(g1->resourceAt(1), r5);
    delete c2;
    delete c21;
    c2 = new AddResourceCmd(&p2, new Resource(r3));
    c2->redo();
    QCOMPARE(p2.resourceList().count(), 3);
    Resource *r6 = p2.resourceList().at(2);
    c21 = new AddParentGroupCmd(r6, g1);
    c21->redo();
    QCOMPARE(g1->resources().count(), 3);
    QCOMPARE(g1->resourceAt(2), r6);
    delete c2;
    delete c21;

    QCOMPARE(r4->requiredIds().count(), 2);
    QCOMPARE(r4->requiredResources().count(), 2);
    QCOMPARE(r4->requiredResources().at(0), r5);
    QCOMPARE(r4->requiredResources().at(1), r6);

    // using xml
    {
        p2.setId(QStringLiteral("p2"));
        XmlSaveContext context(&p2);
        context.save();

        KoXmlDocument xdoc;
        xdoc.setContent(context.document.toString());
        XMLLoaderObject sts;
        sts.setProject(&p);
        sts.setVersion(PLAN_FILE_SYNTAX_VERSION);

        Project p3;
        KoXmlElement xe = xdoc.documentElement().firstChildElement();
        sts.loadProject(&p3, xdoc);

        ResourceGroup *g2 = p3.resourceGroupAt(0);
        QVERIFY(g2);
        QCOMPARE(g2->numResources(), 3);

        Resource *r7 = g2->resourceAt(0);
        QVERIFY(r7);
        Resource *r8 = g2->resourceAt(1);
        QVERIFY(r8);
        Resource *r9 = g2->resourceAt(2);
        QVERIFY(r9);

        QCOMPARE(r7->requiredIds().count(), 2);
        QCOMPARE(r7->requiredResources().count(), 2);
        QCOMPARE(r7->requiredResources().at(0), r8);
        QCOMPARE(r7->requiredResources().at(1), r9);
    }
    {
        // required in different group
        Project p4;
        p4.setId(QStringLiteral("p4"));

        c1 = new AddResourceGroupCmd(&p4, new ResourceGroup());
        c1->redo();
        delete c1;

        ResourceGroup *m = new ResourceGroup();
        m->setType(QStringLiteral("Material"));
        c1 = new AddResourceGroupCmd(&p4, m);
        c1->redo();
        delete c1;

        ResourceGroup *g3 = p4.resourceGroupAt(0);
        QVERIFY(g3);

        c2 = new AddResourceCmd(&p4, new Resource());
        c2->redo();
        QCOMPARE(p4.resourceList().count(), 1);
        Resource *r10 = p4.resourceList().at(0);
        c21 = new AddParentGroupCmd(r10, g3);
        c21->redo();
        QCOMPARE(g3->resources().count(), 1);
        QCOMPARE(g3->resourceAt(0), r10);
        delete c2;
        delete c21;

        c2 = new AddResourceCmd(&p4, new Resource());
        c2->redo();
        QCOMPARE(p4.resourceList().count(), 2);
        Resource *r11 = p4.resourceList().at(1);
        r11->setType(Resource::Type_Material);
        c21 = new AddParentGroupCmd(r11, m);
        c21->redo();
        QCOMPARE(m->resources().count(), 1);
        QCOMPARE(m->resourceAt(0), r11);
        delete c2;
        delete c21;

        c2 = new AddResourceCmd(&p4, new Resource());
        c2->redo();
        QCOMPARE(p4.resourceList().count(), 3);
        Resource *r12 = p4.resourceList().at(2);
        r12->setType(Resource::Type_Material);
        c21 = new AddParentGroupCmd(r12, m);
        c21->redo();
        QCOMPARE(m->resources().count(), 2);
        QCOMPARE(m->resourceAt(1), r12);
        delete c2;
        delete c21;

        r10->addRequiredId(r11->id());
        r10->addRequiredId(r12->id());
        QCOMPARE(r10->requiredIds().count(), 2);
        QCOMPARE(r10->requiredResources().count(), 2);
        QCOMPARE(r10->requiredResources().at(0), r11);
        QCOMPARE(r10->requiredResources().at(1), r12);

        // using xml
        XmlSaveContext context(&p4);
        context.save();

        KoXmlDocument xdoc;
        xdoc.setContent(context.document.toByteArray());

        Project p5;
        XMLLoaderObject sts;
        sts.setProject(&p5);
        sts.loadProject(&p5, xdoc);

        QCOMPARE(p5.numResourceGroups(), p4.numResourceGroups());
        ResourceGroup *g4 = p5.resourceGroupAt(0);
        QVERIFY(g4);
        QCOMPARE(g4->numResources(), 1);

        ResourceGroup *g5 = p5.resourceGroupAt(1);
        QVERIFY(g5);
        QCOMPARE(g5->numResources(), 2);

        Resource *r13 = g4->resourceAt(0);
        QVERIFY(r13);
        QCOMPARE(r13->id(), r10->id());
        Resource *r14 = g5->resourceAt(0);
        QVERIFY(r14);
        QCOMPARE(r14->id(), r11->id());
        Resource *r15 = g5->resourceAt(1);
        QVERIFY(r15);
        QCOMPARE(r15->id(), r12->id());

        QCOMPARE(r13->requiredIds().count(), 2);
        QCOMPARE(r13->requiredResources().count(), 2);
        QCOMPARE(r13->requiredResources().at(0), r14);
        QCOMPARE(r13->requiredResources().at(1), r15);
    }
}

} //namespace KPlato

QTEST_GUILESS_MAIN(KPlato::ResourceTester)
