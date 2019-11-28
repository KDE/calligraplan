/* This file is part of the KDE project
 Copyright (C) 2019 Dag Andersen <danders@get2net.dk>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
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

namespace QTest
{
    template<>
            char *toString(const KPlato::DateTime &dt)
    {
        return toString(dt.toString());
    }
}


using namespace KPlato;


void InsertProjectXmlCommandTester::printDebug(Project *project) const
{
    Project *p = project;
    qInfo()<<"Debug info -------------------------------------";
    qInfo()<<"Project:"<<p;
    qInfo()<<"Num tasks:"<<p->allTasks().count();
    for (int i = 0; i < p->numChildren(); ++i) {
        qInfo()<<i<<':'<<p->childNode(i);
        const ResourceRequestCollection &coll = p->childNode(i)->requests();
        qInfo()<<"\tRequests:"<<coll.requests().count();
        for (int g = 0; g < coll.requests().count(); ++g) {
            ResourceGroupRequest *gr = p->childNode(i)->requests().requests().at(g);
            qInfo()<<"\tGroup:"<<gr->group();
            for (int r = 0; r < gr->resourceRequests().count(); ++r) {
                ResourceRequest *rr = gr->resourceRequests().at(r);
                qInfo()<<"\t\tResource:"<<rr->resource();
            }
        }
    }
}

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
}

void addTask(Project *project, const QString &name, Task *parent = 0)
{
    Task *task = project->createTask();
    task->setName(name);
    project->addSubTask(task, parent ? (Node*)parent : (Node*)project);
    task->estimate()->setUnit(Duration::Unit_h);
    task->estimate()->setExpectedEstimate(8.0);
    task->estimate()->setType(Estimate::Type_Effort);
}

void addRequests(Node *task, ResourceGroup *g, Resource *r)
{
    ResourceGroupRequest *gr = new ResourceGroupRequest(g);
    gr->addResourceRequest(new ResourceRequest(r, 100));
    task->requests().addRequest(gr);
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
    printDebug(m_project);
    delete m_project;
}

void InsertProjectXmlCommandTester::copyBasics()
{
    addTask(m_project, "T1");

    QList<Node*> old = m_project->childNodeIterator();

    XmlSaveContext context(m_project);
    context.options = XmlSaveContext::SaveNodes;
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

    addRequests(m_project->childNode(0), m_project->resourceGroupAt(0), m_project->resourceGroupAt(0)->resourceAt(0));
    addRequests(m_project->childNode(1), m_project->resourceGroupAt(0), m_project->resourceGroupAt(0)->resourceAt(0));
    printDebug(m_project);

    XmlSaveContext context(m_project);
    context.options = XmlSaveContext::SaveNodes;
    context.nodes << m_project->childNode(0) << m_project->childNode(1);
    context.save();
    qInfo()<<context.document.toString();
    InsertProjectXmlCommand cmd(m_project, context.document.toByteArray(), m_project, nullptr /*last*/);
    cmd.redo();
    printDebug(m_project);
    QCOMPARE(m_project->allTasks().count(), 4);

    Node *copy1 = m_project->childNode(2);
    QVERIFY(m_project->childNode(0)->id() != copy1->id());
    QCOMPARE(m_project->childNode(0)->name(), copy1->name());

    QCOMPARE(m_project->childNode(0)->requests().requests().count(), copy1->requests().requests().count());
    ResourceGroupRequest *gr = m_project->childNode(0)->requests().requests().at(0);
    ResourceGroupRequest *cpgr = copy1->requests().requests().at(0);
    QCOMPARE(gr->group(), cpgr->group());
    QCOMPARE(gr->units(), cpgr->units());
    QCOMPARE(gr->count(), cpgr->count());
    ResourceRequest *rr = gr->resourceRequests().at(0);
    ResourceRequest *cprr = cpgr->resourceRequests().at(0);
    QCOMPARE(rr->resource(), cprr->resource());
    QCOMPARE(rr->units(), cprr->units());

}

void InsertProjectXmlCommandTester::copyDependency()
{
    addResources(m_project);

    addTask(m_project, "T1");
    addTask(m_project, "T2");

    addDependency(m_project->childNode(0), m_project->childNode(1));

    printDebug(m_project);

    XmlSaveContext context(m_project);
    context.options = XmlSaveContext::SaveNodes;
    context.nodes << m_project->childNode(0) << m_project->childNode(1);
    context.save();
    InsertProjectXmlCommand cmd(m_project, context.document.toByteArray(), m_project, nullptr /*last*/);
    cmd.redo();
    printDebug(m_project);
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
    context.options = XmlSaveContext::SaveNodes;
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

    qInfo()<<m_project->childNodeIterator();
    old = m_project->childNodeIterator();
    context.options = XmlSaveContext::SaveNodes;
    context.save();
    {
    InsertProjectXmlCommand cmd(m_project, context.document.toByteArray(), m_project->childNode(1), nullptr);
    cmd.redo();
    }
    qInfo()<<m_project->childNodeIterator();
    QCOMPARE(m_project->numChildren(), 3);
    QCOMPARE(m_project->allNodes().count(), 4);
    QVERIFY(m_project->childNode(1)->type() == Node::Type_Summarytask);
    QCOMPARE(m_project->childNode(1)->numChildren(), 1);
}

QTEST_GUILESS_MAIN(KPlato::InsertProjectXmlCommandTester)
