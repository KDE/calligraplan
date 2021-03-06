/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2009 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "InsertProjectTester.h"

#include "kptcommand.h"
#include <AddResourceCmd.h>
#include "kptmaindocument.h"
#include "kptpart.h"
#include "kptcalendar.h"
#include "kptresource.h"
#include "kpttask.h"
#include "kptaccount.h"

#include <KoXmlReader.h>

#include <QTest>

#include <tests/debug.cpp>

namespace KPlato
{

Account *InsertProjectTester::addAccount(MainDocument &part, Account *parent)
{
    Project &p = part.getProject();
    Account *a = new Account();
    QString s = parent == nullptr ? "Account" : parent->name();
    a->setName(p.accounts().uniqueId(s));
    KUndo2Command *c = new AddAccountCmd(p, a, parent);
    part.addCommand(c);
    return a;
}

void InsertProjectTester::testAccount()
{
    Part pp(nullptr);
    MainDocument part(&pp);
    pp.setDocument(&part);

    addAccount(part);
    Project &p = part.getProject();
    QCOMPARE(p.accounts().accountCount(), 1);

    Part pp2(nullptr);
    MainDocument part2(&pp2);
    pp2.setDocument(&part2);

    part2.insertProject(p, nullptr, nullptr);
    QCOMPARE(part2.getProject().accounts().accountCount(), 1);

    part2.insertProject(part.getProject(), nullptr, nullptr);
    QCOMPARE(part2.getProject().accounts().accountCount(), 1);

    Part ppB(nullptr);
    MainDocument partB(&ppB);
    ppB.setDocument(&partB);

    Account *parent = addAccount(partB);
    QCOMPARE(partB.getProject().accounts().accountCount(), 1);
    addAccount(partB, parent);
    QCOMPARE(partB.getProject().accounts().accountCount(), 1);
    QCOMPARE(parent->childCount(), 1);

    part2.insertProject(partB.getProject(), nullptr, nullptr);
    QCOMPARE(part2.getProject().accounts().accountCount(), 1);
    QCOMPARE(part2.getProject().accounts().accountAt(0)->childCount(), 1);
}

Calendar *InsertProjectTester::addCalendar(MainDocument &part)
{
    Project &p = part.getProject();
    Calendar *c = new Calendar();
    p.setCalendarId(c);
    part.addCommand(new CalendarAddCmd(&p, c, -1, nullptr));
    return c;
}

void InsertProjectTester::testCalendar()
{
    Part pp(nullptr);
    MainDocument part(&pp);
    pp.setDocument(&part);

    addCalendar(part);
    Project &p = part.getProject();
    QVERIFY(p.calendarCount() == 1);

    Part pp2(nullptr);
    MainDocument part2(&pp2);
    pp2.setDocument(&part2);

    part2.insertProject(part.getProject(), nullptr, nullptr);
    QVERIFY(part2.getProject().calendarCount() == 1);
    QVERIFY(part2.getProject().defaultCalendar() == nullptr);
}

void InsertProjectTester::testDefaultCalendar()
{
    Part pp(nullptr);
    MainDocument part(&pp);
    pp.setDocument(&part);

    Calendar *c = addCalendar(part);
    Project &p = part.getProject();
    p.setDefaultCalendar(c);
    QVERIFY(p.calendarCount() == 1);
    QCOMPARE(p.defaultCalendar(), c);

    Part pp2(nullptr);
    MainDocument part2(&pp2);
    pp2.setDocument(&part2);

    part2.insertProject(p, nullptr, nullptr);
    QVERIFY(part2.getProject().calendarCount() == 1);
    QCOMPARE(part2.getProject().defaultCalendar(), c);

    Part ppB(nullptr);
    MainDocument partB(&ppB);
    ppB.setDocument(&partB);

    Calendar *c2 = addCalendar(partB);
    partB.getProject().setDefaultCalendar(c2);
    part2.insertProject(partB.getProject(), nullptr, nullptr);
    QVERIFY(part2.getProject().calendarCount() == 2);
    QCOMPARE(part2.getProject().defaultCalendar(), c); // NB: still c, not c2
}

ResourceGroup *InsertProjectTester::addResourceGroup(MainDocument &part)
{
    Project &p = part.getProject();
    ResourceGroup *g = new ResourceGroup();
    KUndo2Command *c = new AddResourceGroupCmd(&p, g);
    part.addCommand(c);
    QString s = QString("G%1").arg(part.getProject().indexOf(g));
    g->setName(s);
    return g;
}

void InsertProjectTester::testResourceGroup()
{
    Part pp(nullptr);
    MainDocument part(&pp);
    pp.setDocument(&part);

    addResourceGroup(part);
    Project &p = part.getProject();
    QVERIFY(p.resourceGroupCount() == 1);

    Part pp2(nullptr);
    MainDocument part2(&pp2);
    pp2.setDocument(&part2);
    part2.insertProject(p, nullptr, nullptr);
    QVERIFY(part2.getProject().resourceGroupCount() == 1);
}

Resource *InsertProjectTester::addResource(MainDocument &part, ResourceGroup *g)
{
    Project &p = part.getProject();
    if (p.resourceGroupCount() == 0) {
        qInfo()<<"No resource groups in project";
        return nullptr;
    }
    if (g == nullptr) {
        g = p.resourceGroupAt(0);
    }
    Q_ASSERT(g);
    if (!p.resourceGroups().contains(g)) {
        qInfo()<<"Project does not contain resource group";
        return nullptr;
    }
    Resource *r = new Resource();
    KUndo2Command *c = new AddResourceCmd(g, r);
    part.addCommand(c);
    QString s = QString("%1.R%2").arg(g->name()).arg(g->indexOf(r));
    r->setName(s);
    return r;
}

void InsertProjectTester::testResource()
{
    Part pp(nullptr);
    MainDocument part(&pp);
    pp.setDocument(&part);

    addResourceGroup(part);
    addResource(part);
    Project &p = part.getProject();
    QVERIFY(p.resourceGroupAt(0)->numResources() == 1);

    Part pp2(nullptr);
    MainDocument part2(&pp2);
    pp2.setDocument(&part2);
    //Debug::print(&part.getProject(), "Project to insert from: ------------", true);
    //Debug::print(&part2.getProject(), "Project to insert into: -----------", true);
    part2.insertProject(p, nullptr, nullptr);
    //Debug::print(&part2.getProject(), "Result: ---------------------------", true);
    QVERIFY(part2.getProject().resourceGroupAt(0)->numResources() == 1);
}

void InsertProjectTester::testTeamResource()
{
    Part pp(nullptr);
    MainDocument part(&pp);
    pp.setDocument(&part);

    addResourceGroup(part);
    Resource *r = addResource(part);
    r->setType(Resource::Type_Team);
    ResourceGroup *tg = addResourceGroup(part);
    Resource *t1 = addResource(part, tg);
    Resource *t2 = addResource(part, tg);
    r->setRequiredIds(QStringList() << t1->id() << t2->id());
    Project &p = part.getProject();
    QVERIFY(p.resourceGroupAt(0)->numResources() == 1);
    QVERIFY(p.resourceGroupAt(1)->numResources() == 2);
    QList<Resource*> required = p.resourceGroupAt(0)->resources().at(0)->requiredResources();
    QCOMPARE(required.count(), 2);
    QCOMPARE(required.at(0), t1);
    QCOMPARE(required.at(1), t2);

    Part pp2(nullptr);
    MainDocument part2(&pp2);
    pp2.setDocument(&part2);

    part2.insertProject(p, nullptr, nullptr);
    Project &p2 = part2.getProject();
    QVERIFY(p2.resourceGroupAt(0)->numResources() == 1);
    QVERIFY(p2.resourceGroupAt(1)->numResources() == 2);
    required = p2.resourceGroupAt(0)->resources().at(0)->requiredResources();
    QCOMPARE(required.count(), 2);
    QCOMPARE(required.at(0), t1);
    QCOMPARE(required.at(1), t2);
}

void InsertProjectTester::testResourceAccount()
{
    Part pp(nullptr);
    MainDocument part(&pp);
    pp.setDocument(&part);

    addResourceGroup(part);
    Resource *r = addResource(part);
    Account *a = addAccount(part);
    part.addCommand(new ResourceModifyAccountCmd(*r, r->account(), a));

    Project &p = part.getProject();
    QVERIFY(p.resourceGroupAt(0)->numResources() == 1);

    Part pp2(nullptr);
    MainDocument part2(&pp2);
    pp2.setDocument(&part2);

    part2.insertProject(p, nullptr, nullptr);
    QVERIFY(part2.getProject().resourceGroupAt(0)->numResources() == 1);
    QVERIFY(part2.getProject().accounts().allAccounts().contains(a));
    QCOMPARE(part2.getProject().resourceGroupAt(0)->resourceAt(0)->account(), a);
}

void InsertProjectTester::testResourceCalendar()
{
    Part pp(nullptr);
    MainDocument part(&pp);
    pp.setDocument(&part);

    Calendar *c = addCalendar(part);
    Project &p = part.getProject();
    QVERIFY(p.calendarCount() == 1);

    addResourceGroup(part);
    Resource *r = addResource(part);
    part.addCommand(new ModifyResourceCalendarCmd(r, c));

    QVERIFY(p.resourceGroupAt(0)->numResources() == 1);

    Part pp2(nullptr);
    MainDocument part2(&pp2);
    pp2.setDocument(&part2);

    part2.insertProject(p, nullptr, nullptr);
    QVERIFY(part2.getProject().resourceGroupAt(0)->numResources() == 1);
    QCOMPARE(part2.getProject().allCalendars().count(), 1);
    QVERIFY(part2.getProject().allCalendars().contains(c));
    QCOMPARE(part2.getProject().resourceGroupAt(0)->resourceAt(0)->calendar(true), c);
}

Task *InsertProjectTester::addTask(MainDocument &part)
{
    Project &p = part.getProject();
    Task *t = new Task();
    t->setId(p.uniqueNodeId());
    KUndo2Command *c = new TaskAddCmd(&p, t, nullptr);
    part.addCommand(c);
    return t;
}

void InsertProjectTester::testTask()
{
    Part pp(nullptr);
    MainDocument part(&pp);
    pp.setDocument(&part);
    Project &p = part.getProject();
    p.setName("Insert from");

    addTask(part);
    QVERIFY(p.numChildren() == 1);

    Part pp2(nullptr);
    MainDocument part2(&pp2);
    pp2.setDocument(&part2);
    part2.getProject().setName("Insert into");
    part2.insertProject(p, nullptr, nullptr);
    QVERIFY(part2.getProject().numChildren() == 1);
}
void InsertProjectTester::addResourceRequest(MainDocument &part)
{
    Project &p = part.getProject();
    KUndo2Command *c = new AddResourceRequestCmd(&p.childNode(0)->requests(), new  ResourceRequest(p.resourceGroupAt(0)->resourceAt(0), 1));
    part.addCommand(c);
}

void InsertProjectTester::testResourceRequest()
{
    Part pp(nullptr);
    MainDocument part(&pp);
    pp.setDocument(&part);

    addCalendar(part);
    addResourceGroup(part);
    addResource(part);
    addTask(part);
    addResourceRequest(part);

    Project &p = part.getProject();
    Debug::print(&p, "To be inserted: --------", true);
    Part pp2(nullptr);
    MainDocument part2(&pp2);
    pp2.setDocument(&part2);
    part2.insertProject(p, nullptr, nullptr);
    Project &p2 = part2.getProject();
    QVERIFY(p2.childNode(0)->requests().find(p2.resourceGroupAt(0)->resourceAt(0)) != nullptr);
    Debug::print(&p2, "After insert: --------", true);
}

void InsertProjectTester::testTeamResourceRequest()
{
    Part pp(nullptr);
    MainDocument part(&pp);
    pp.setDocument(&part);

    addCalendar(part);
    addResourceGroup(part);
    Resource *r = addResource(part);
    r->setType(Resource::Type_Team);
    ResourceGroup *tg = addResourceGroup(part);
    Resource *t1 = addResource(part, tg);
    r->addTeamMemberId(t1->id());
    Resource *t2 = addResource(part, tg);
    r->addTeamMemberId(t2->id());
    addTask(part);
    addResourceRequest(part);

    qDebug()<<"Start test:";
    Part pp2(nullptr);
    MainDocument part2(&pp2);
    pp2.setDocument(&part2);
    part2.insertProject(part.getProject(), nullptr, nullptr);
    Project &p2 = part2.getProject();
    ResourceRequest *rr = p2.childNode(0)->requests().find(p2.resourceGroupAt(0)->resourceAt(0));
    QVERIFY(rr);
    QCOMPARE(rr->resource(), r);
    QCOMPARE(rr->resource()->teamMembers().count(), 2);
    QCOMPARE(rr->resource()->teamMembers().at(0), t1);
    QCOMPARE(rr->resource()->teamMembers().at(1), t2);
}

Relation *InsertProjectTester::addDependency(MainDocument &part, Task *t1, Task *t2)
{
    Project &p = part.getProject();
    Relation *r = new Relation(t1, t2);
    part.addCommand(new AddRelationCmd(p, r));
    return r;
}

void InsertProjectTester::testDependencies()
{
    Part pp(nullptr);
    MainDocument part(&pp);
    pp.setDocument(&part);

    Task *t1 = addTask(part);
    Task *t2 = addTask(part);
    QCOMPARE(t1->numDependChildNodes(), 0);
    QCOMPARE(t2->numDependParentNodes(), 0);

    Relation *r = addDependency(part, t1, t2);

    QCOMPARE(t1->numDependChildNodes(), 1);
    QCOMPARE(t2->numDependParentNodes(), 1);
    QCOMPARE(t1->getDependChildNode(0), r);
    QCOMPARE(t2->getDependParentNode(0), r);

    Part pp2(nullptr);
    MainDocument part2(&pp2);
    pp2.setDocument(&part2);

    QVERIFY(part2.insertProject(part.getProject(), nullptr, nullptr));
    Project &p2 = part2.getProject();

    QVERIFY(p2.numChildren() == 2);
    QCOMPARE(p2.childNode(0), t1);
    QCOMPARE(p2.childNode(1), t2);

    QCOMPARE(t1->numDependChildNodes(), 1);
    QCOMPARE(t2->numDependParentNodes(), 1);
    QCOMPARE(t1->getDependChildNode(0), r);
    QCOMPARE(t2->getDependParentNode(0), r);
}

void InsertProjectTester::testExistingResourceAccount()
{
    Part pp(nullptr);
    MainDocument part(&pp);
    pp.setDocument(&part);

    addResourceGroup(part);
    Resource *r = addResource(part);
    Account *a = addAccount(part);
    part.addCommand(new ResourceModifyAccountCmd(*r, r->account(), a));

    Project &p = part.getProject();
    QVERIFY(p.resourceGroupAt(0)->numResources() == 1);

    QDomDocument doc = part.saveXML();

    Part pp2(nullptr);
    MainDocument part2(&pp2);
    pp2.setDocument(&part2);

    part2.insertProject(p, nullptr, nullptr);
    QVERIFY(part2.getProject().resourceGroupAt(0)->numResources() == 1);
    QVERIFY(part2.getProject().accounts().allAccounts().contains(a));
    QCOMPARE(part2.getProject().resourceGroupAt(0)->resourceAt(0)->account(), a);

    part2.addCommand(new ResourceModifyAccountCmd(*(part2.getProject().resourceGroupAt(0)->resourceAt(0)), part2.getProject().resourceGroupAt(0)->resourceAt(0)->account(), nullptr));

    KoXmlDocument xdoc;
    xdoc.setContent(doc.toString());
    part.loadXML(xdoc, nullptr);

    part2.insertProject(part.getProject(), nullptr, nullptr);
    QVERIFY(part2.getProject().resourceGroupAt(0)->numResources() == 1);
    QVERIFY(part2.getProject().accounts().allAccounts().contains(a));
    QVERIFY(part2.getProject().resourceGroupAt(0)->resourceAt(0)->account() == nullptr);
}

void InsertProjectTester::testExistingResourceCalendar()
{
    Part pp(nullptr);
    MainDocument part(&pp);
    pp.setDocument(&part);

    Calendar *c = addCalendar(part);
    Project &p = part.getProject();
    QVERIFY(p.calendarCount() == 1);

    addResourceGroup(part);
    Resource *r = addResource(part);
    part.addCommand(new ModifyResourceCalendarCmd(r, c));

    QVERIFY(p.resourceGroupAt(0)->numResources() == 1);

    QDomDocument doc = part.saveXML();

    Part pp2(nullptr);
    MainDocument part2(&pp2);
    pp2.setDocument(&part2);

    part2.insertProject(p, nullptr, nullptr);
    QVERIFY(part2.getProject().resourceGroupAt(0)->numResources() == 1);
    QCOMPARE(part2.getProject().allCalendars().count(), 1);
    QVERIFY(part2.getProject().allCalendars().contains(c));
    QCOMPARE(part2.getProject().resourceGroupAt(0)->resourceAt(0)->calendar(true), c);

    part2.getProject().resourceGroupAt(0)->resourceAt(0)->setCalendar(nullptr);

    KoXmlDocument xdoc;
    xdoc.setContent(doc.toString());
    part.loadXML(xdoc, nullptr);

    part2.insertProject(part.getProject(), nullptr, nullptr);
    QVERIFY(part2.getProject().resourceGroupAt(0)->numResources() == 1);
    QCOMPARE(part2.getProject().allCalendars().count(), 1);
    QVERIFY(part2.getProject().allCalendars().contains(c));
    QVERIFY(part2.getProject().resourceGroupAt(0)->resourceAt(0)->calendar(true) == nullptr);
}

void InsertProjectTester::testExistingResourceRequest()
{
    Part pp(nullptr);
    MainDocument part(&pp);
    pp.setDocument(&part);

    addCalendar(part);
    addResourceGroup(part);
    addResource(part);
    addTask(part);
    addResourceRequest(part);

    QDomDocument doc = part.saveXML();

    Project &p = part.getProject();
    Part pp2(nullptr);
    MainDocument part2(&pp2);
    pp2.setDocument(&part2);
    //Debug::print(&part.getProject(), "Project to be inserted from: --------------------------", true);
    //Debug::print(&part2.getProject(), "Project to be inserted into: -------------------------", true);
    part2.insertProject(p, nullptr, nullptr);
    //Debug::print(&part2.getProject(), "Result1:", true);
    Project &p2 = part2.getProject();
    QVERIFY(p2.childNode(0)->requests().find(p2.resourceGroupAt(0)->resourceAt(0)) != nullptr);

    KoXmlDocument xdoc;
    xdoc.setContent(doc.toString());
    part.loadXML(xdoc, nullptr);
    //Debug::print(&part.getProject(), "Project to be inserted from:", true);
    //Debug::print(&part2.getProject(), "Project to be inserted into:", true);
    part2.insertProject(part.getProject(), nullptr, nullptr);
    //Debug::print(&part2.getProject(), "Result2:", true);
    QVERIFY(p2.childNode(0)->requests().find(p2.resourceGroupAt(0)->resourceAt(0)) != nullptr);
    QVERIFY(p2.childNode(1)->requests().find(p2.resourceGroupAt(0)->resourceAt(0)) != nullptr);
}

void InsertProjectTester::testExistingRequiredResourceRequest()
{
    Part pp(nullptr);
    MainDocument part(&pp);
    pp.setDocument(&part);

    addCalendar(part);
    addResourceGroup(part);
    Resource *r = addResource(part);
    ResourceGroup *g = addResourceGroup(part);
    g->setType("Material");
    QList<Resource*> m; m << addResource(part, g);
    m.first()->setType(Resource::Type_Material);
    r->setRequiredIds(QStringList() << m.first()->id());
    addTask(part);
    addResourceRequest(part);

    QDomDocument doc = part.saveXML();

    Project &p = part.getProject();
    Part pp2(nullptr);
    MainDocument part2(&pp2);
    pp2.setDocument(&part2);
    part2.insertProject(p, nullptr, nullptr);
    Project &p2 = part2.getProject();
    ResourceRequest *rr = p2.childNode(0)->requests().find(p2.resourceGroupAt(0)->resourceAt(0));
    QVERIFY(rr);
    QVERIFY(! rr->requiredResources().isEmpty());
    QCOMPARE(rr->requiredResources().at(0), m.first());

    KoXmlDocument xdoc;
    xdoc.setContent(doc.toString());
    part.loadXML(xdoc, nullptr);

    part2.insertProject(part.getProject(), nullptr, nullptr);
    rr = p2.childNode(0)->requests().find(p2.resourceGroupAt(0)->resourceAt(0));
    QVERIFY(rr);
    QVERIFY(! rr->requiredResources().isEmpty());
    QCOMPARE(rr->requiredResources().at(0), m.first());

    rr = p2.childNode(1)->requests().find(p2.resourceGroupAt(0)->resourceAt(0));
    QVERIFY(rr);
    QVERIFY(! rr->requiredResources().isEmpty());
    QCOMPARE(rr->requiredResources().at(0), m.first());
}

void InsertProjectTester::testExistingTeamResourceRequest()
{
    Part pp(nullptr);
    MainDocument part(&pp);
    pp.setDocument(&part);

    addCalendar(part);
    addResourceGroup(part);
    Resource *r = addResource(part);
    r->setName("R1");
    r->setType(Resource::Type_Team);
    ResourceGroup *tg = addResourceGroup(part);
    tg->setName("TG");
    Resource *t1 = addResource(part, tg);
    t1->setName("T1");
    r->addTeamMemberId(t1->id());
    Resource *t2 = addResource(part, tg);
    t2->setName("T2");
    r->addTeamMemberId(t2->id());
    addTask(part);
    addResourceRequest(part);

    QDomDocument doc = part.saveXML();

    Part pp2(nullptr);
    MainDocument part2(&pp2);
    pp2.setDocument(&part2);

    Project &p2 = part2.getProject();
    part2.insertProject(part.getProject(), nullptr, nullptr);
    ResourceRequest *rr = p2.childNode(0)->requests().find(p2.resourceGroupAt(0)->resourceAt(0));
    QVERIFY(rr);
    QCOMPARE(rr->resource()->teamMembers().count(), 2);
    QCOMPARE(rr->resource()->teamMembers().at(0), t1);
    QCOMPARE(rr->resource()->teamMembers().at(1), t2);

    KoXmlDocument xdoc;
    xdoc.setContent(doc.toString());
    part.loadXML(xdoc, nullptr);

    part2.insertProject(part.getProject(), nullptr, nullptr);
    QCOMPARE(p2.numChildren(), 2);

    QVERIFY(! p2.childNode(0)->requests().isEmpty());
    rr = p2.childNode(0)->requests().find(p2.resourceGroupAt(0)->resourceAt(0));
    QVERIFY(rr);
    QCOMPARE(rr->resource()->teamMembers().count(), 2);
    QCOMPARE(rr->resource()->teamMembers().at(0), t1);
    QCOMPARE(rr->resource()->teamMembers().at(1), t2);

    QVERIFY(! p2.childNode(1)->requests().isEmpty());
    rr = p2.childNode(1)->requests().find(p2.resourceGroupAt(0)->resourceAt(0));
    QVERIFY(rr);
    QCOMPARE(rr->resource()->teamMembers().count(), 2);
    QCOMPARE(rr->resource()->teamMembers().at(0), t1);
    QCOMPARE(rr->resource()->teamMembers().at(1), t2);
}

} //namespace KPlato

QTEST_MAIN(KPlato::InsertProjectTester)
