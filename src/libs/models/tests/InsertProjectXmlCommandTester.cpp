/* This file is part of the KDE project
 SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>

 SPDX-License-Identifier: LGPL-2.0-or-later
*/
// clazy:excludeall=qstring-arg
#include "InsertProjectXmlCommandTester.h"

#include "kptcommand.h"
#include "kptcalendar.h"
#include "kptdatetime.h"
#include "kptresource.h"
#include "kptnode.h"
#include "kpttask.h"
#include "kptschedule.h"
#include "kptappointment.h"
#include "XmlSaveContext.h"
#include "InsertProjectXmlCommand.h"

#include <QModelIndex>

#include <QTest>

#include <tests/debug.cpp>

using namespace KPlato;


Project *createProject()
{
    Project *project = new Project();
    project->setName("P1");
    project->setId(project->uniqueNodeId());
    project->registerNodeId(project);
    DateTime targetstart = DateTime(QDate::currentDate(), QTime(0,0,0));
    DateTime targetend = DateTime(targetstart.addDays(3));
    project->setConstraintStartTime(targetstart);
    project->setConstraintEndTime(targetend);

    // standard worktime defines 8 hour day as default
    Calendar *calendar = new Calendar("Test");
    calendar->setDefault(true);
    QTime t1(9, 0, 0);
    QTime t2 (17, 0, 0);
    int length = t1.msecsTo(t2);
    for (int i=1; i <= 7; ++i) {
        CalendarDay *d = calendar->weekday(i);
        d->setState(CalendarDay::Working);
        d->addInterval(t1, length);
    }
    project->addCalendar(calendar);
    return project;
}

void addResources(Project *project)
{
    ResourceGroup *g = new ResourceGroup();
    g->setName("G1");
    project->addResourceGroup(g);
    Resource *resource = new Resource();
    resource->setName("R1");
    resource->setCalendar(project->calendars().value(0));
    project->addResource(resource);
    g->addResource(resource);

    resource = new Resource();
    resource->setName("A1");
    resource->setCalendar(project->calendars().value(0));
    project->addResource(resource);

    resource = new Resource();
    resource->setName("M1");
    resource->setType(Resource::Type_Material);
    resource->setCalendar(project->calendars().value(0));
    project->addResource(resource);
}

void addTask(Project *project, const QString &name, Task *parent = nullptr)
{
    Task *task = project->createTask();
    task->setName(name);
    project->addSubTask(task, parent ? (Node*)parent : (Node*)project);
    task->estimate()->setUnit(Duration::Unit_h);
    task->estimate()->setExpectedEstimate(8.0);
    task->estimate()->setType(Estimate::Type_Effort);
}

void addRequests(Node *task, Resource *r, Resource *alternative, Resource *required)
{
    ResourceRequest *rr = new ResourceRequest(r, 100);
    task->requests().addResourceRequest(rr);

    if (alternative) {
        rr->addAlternativeRequest(new ResourceRequest(alternative));
    }
    if (required) {
        rr->addRequiredResource(required);
    }
}

void addDependency(Node *t1, Node *t2)
{
    t1->addDependChildNode(t2, Relation::FinishStart);
}

void InsertProjectXmlCommandTester::init()
{
    m_project = createProject();
}

void InsertProjectXmlCommandTester::cleanup()
{
    Debug::print(m_project, "--------------------", true);
    delete m_project;
}

void InsertProjectXmlCommandTester::copyBasics()
{
    addTask(m_project, "T1");

    QList<Node*> old = m_project->childNodeIterator();

    XmlSaveContext context(m_project);
    context.options = XmlSaveContext::SaveSelectedNodes;
    context.nodes << m_project->childNode(0);
    context.save();
    qInfo()<<context.document.toString();
    InsertProjectXmlCommand cmd(m_project, context.document.toByteArray(), m_project, m_project->childNode(0));
    cmd.redo();

    QCOMPARE(m_project->allTasks().count(), 2);
    QVERIFY(!old.contains(m_project->childNode(0)));
    QCOMPARE(m_project->childNode(1), old.at(0));
}

void InsertProjectXmlCommandTester::copyRequests()
{
    addResources(m_project);

    addTask(m_project, "T1");
    addTask(m_project, "T2");

    Node *org1 = m_project->childNode(0);
    Node *org2 = m_project->childNode(1);
    addRequests(org1, m_project->resourceGroupAt(0)->resourceAt(0), m_project->resourceList().at(1), m_project->resourceList().at(2));
    addRequests(org2, m_project->resourceGroupAt(0)->resourceAt(0), nullptr, nullptr);
    Debug::print(m_project, "Before copy: --------------------", true);

    XmlSaveContext context(m_project);
    context.options = XmlSaveContext::SaveSelectedNodes|XmlSaveContext::SaveRequests;
    context.nodes << m_project->childNode(0) << m_project->childNode(1);
    context.save();

    InsertProjectXmlCommand cmd(m_project, context.document.toByteArray(), m_project, nullptr /*last*/);
    cmd.redo();
    Debug::print(m_project, "After copy: --------------------", true);
    QCOMPARE(m_project->allTasks().count(), 4);

    Node *copy1 = m_project->childNode(2);
    QVERIFY(org1->id() != copy1->id());
    QCOMPARE(org1->name(), copy1->name());

    ResourceRequest *rr = org1->requests().resourceRequests().at(0);
    ResourceRequest *cprr = copy1->requests().resourceRequests().at(0);
    QCOMPARE(rr->resource(), cprr->resource());
    QCOMPARE(rr->units(), cprr->units());

    QCOMPARE(rr->alternativeRequests().count(), 1);
    QCOMPARE(cprr->alternativeRequests().count(), 1);
    QCOMPARE(rr->alternativeRequests().at(0)->resource(), cprr->alternativeRequests().at(0)->resource());
    QCOMPARE(rr->requiredResources().count(), 1);
    QCOMPARE(cprr->requiredResources().count(), 1);
    QCOMPARE(rr->requiredResources().at(0), cprr->requiredResources().at(0));
}

void InsertProjectXmlCommandTester::copyDependency()
{
    addResources(m_project);

    addTask(m_project, "T1");
    addTask(m_project, "T2");

    addDependency(m_project->childNode(0), m_project->childNode(1));

    Debug::print(m_project, "--------------------", true);

    XmlSaveContext context(m_project);
    context.options = XmlSaveContext::SaveSelectedNodes | XmlSaveContext::SaveRelations;
    context.nodes << m_project->childNode(0) << m_project->childNode(1);
    context.save();
    InsertProjectXmlCommand cmd(m_project, context.document.toByteArray(), m_project, nullptr /*last*/);
    cmd.redo();
    Debug::print(m_project, "--------------------", true);
    QCOMPARE(m_project->allTasks().count(), 4);

    Node *copy1 = m_project->childNode(2);
    Node *copy2 = m_project->childNode(3);
    QVERIFY(m_project->childNode(0)->id() != copy1->id());
    QCOMPARE(m_project->childNode(0)->name(), copy1->name());

    QCOMPARE(m_project->childNode(0)->numDependChildNodes(), copy1->numDependChildNodes());
    QCOMPARE(m_project->childNode(1)->numDependParentNodes(), copy2->numDependParentNodes());
}

void InsertProjectXmlCommandTester::copyToPosition()
{
    addTask(m_project, "T1");
    addTask(m_project, "T2");

    QList<Node*> old = m_project->childNodeIterator();
    XmlSaveContext context(m_project);
    context.options = XmlSaveContext::SaveSelectedNodes;
    context.nodes << m_project->childNode(0);
    context.save();
    {
    InsertProjectXmlCommand cmd(m_project, context.document.toByteArray(), m_project, m_project->childNode(1));
    cmd.redo();
    }
    QCOMPARE(m_project->allTasks().count(), 3);
    QCOMPARE(m_project->childNode(0), old.at(0));
    QCOMPARE(m_project->childNode(2), old.at(1));
    QVERIFY(!old.contains(m_project->childNode(1)));

    old = m_project->childNodeIterator();
    context.options = XmlSaveContext::SaveSelectedNodes;
    context.save();
    {
    InsertProjectXmlCommand cmd(m_project, context.document.toByteArray(), m_project->childNode(1), nullptr);
    cmd.redo();
    }

    QCOMPARE(m_project->numChildren(), 3);
    QCOMPARE(m_project->allNodes().count(), 4);
    QVERIFY(m_project->childNode(1)->type() == Node::Type_Summarytask);
    QCOMPARE(m_project->childNode(1)->numChildren(), 1);
}

QTEST_GUILESS_MAIN(KPlato::InsertProjectXmlCommandTester)
