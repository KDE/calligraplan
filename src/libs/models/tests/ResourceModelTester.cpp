/* This file is part of the KDE project
 * Copyright (C) 2019 Dag Andersen <danders@get2net.dk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

// clazy:excludeall=qstring-arg
#include "ResourceModelTester.h"

#include "AddResourceCmd.h"
#include "AddParentGroupCmd.h"
#include "kptcommand.h"
#include "kptcalendar.h"
#include "kptdatetime.h"
#include "kptresource.h"

#include <QModelIndex>

#include <QTest>

#include "tests/debug.cpp"

using namespace KPlato;

void ResourceModelTester::init()
{
    m_model = new ResourceItemModel();
    m_project = new Project();
    m_project->setName("P1");
    m_project->setId(m_project->uniqueNodeId());
    m_project->registerNodeId(m_project);
    DateTime targetstart = DateTime(QDate::currentDate(), QTime(0,0,0));
    DateTime targetend = DateTime(targetstart.addDays(3));
    m_project->setConstraintStartTime(targetstart);
    m_project->setConstraintEndTime(targetend);

    // standard worktime defines 8 hour day as default
    QVERIFY(m_project->standardWorktime());
    QCOMPARE(m_project->standardWorktime()->day(), 8.0);
    m_calendar = new Calendar("Test");
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
    m_model->setProject(m_project);
}

void ResourceModelTester::cleanup()
{
    m_project->deref();
    delete m_model;
}

void ResourceModelTester::resources()
{
    QCOMPARE(m_project->resourceCount(), 0);
    Resource *r1 = new Resource();
    r1->setName("r1");
    r1->setType(Resource::Type_Work);
    AddResourceCmd c1(m_project, r1);
    c1.redo();
    QCOMPARE(m_project->resourceCount(), 1);
    c1.undo();
    QCOMPARE(m_project->resourceCount(), 0);
    c1.redo();
    QCOMPARE(m_project->resourceCount(), 1);
    QCOMPARE(m_model->rowCount(), 1);
    QModelIndex idx1 = m_model->index(r1);
    QVERIFY(idx1.isValid());
    QCOMPARE(idx1, m_model->index(0, 0));
    QCOMPARE(m_model->resource(idx1), r1);

    Resource *r2 = new Resource();
    r2->setName("r2");
    r2->setType(Resource::Type_Work);
    AddResourceCmd c2(m_project, r2);
    c2.redo();
    QCOMPARE(m_project->resourceCount(), 2);
    c2.undo();
    QCOMPARE(m_project->resourceCount(), 1);
    c2.redo();
    QCOMPARE(m_project->resourceCount(), 2);
    QCOMPARE(m_model->rowCount(), 2);
    QModelIndex idx2 = m_model->index(r2);
    QVERIFY(idx2.isValid());
    QCOMPARE(idx2, m_model->index(1, 0));
    QCOMPARE(m_model->resource(idx2), r2);

    Resource *r3 = new Resource();
    r3->setName("r3");
    r3->setType(Resource::Type_Team);
    AddResourceCmd c3(m_project, r3);
    c3.redo();
    QCOMPARE(m_project->resourceCount(), 3);
    c3.undo();
    QCOMPARE(m_project->resourceCount(), 2);
    c3.redo();
    QCOMPARE(m_project->resourceCount(), 3);
    QCOMPARE(m_model->rowCount(), 3);
    QModelIndex idx3 = m_model->index(r3);
    QVERIFY(idx3.isValid());
    QCOMPARE(idx3, m_model->index(2, 0));
    QCOMPARE(m_model->resource(idx3), r3);

    Resource *r4 = new Resource();
    r4->setName("r4");
    r4->setType(Resource::Type_Material);
    AddResourceCmd c4(m_project, r4);
    c4.redo();
    QCOMPARE(m_project->resourceCount(), 4);
    c4.undo();
    QCOMPARE(m_project->resourceCount(), 3);
    c4.redo();
    QCOMPARE(m_project->resourceCount(), 4);
    QCOMPARE(m_model->rowCount(), 4);
    QModelIndex idx4 = m_model->index(r4);
    QVERIFY(idx4.isValid());
    QCOMPARE(idx4, m_model->index(3, 0));
    QCOMPARE(m_model->resource(idx4), r4);

    ResourceGroup *g1 = new ResourceGroup();
    g1->setName("g1");
    m_project->addResourceGroup(g1);
    g1->addResource(r1);
    QVERIFY(r1->parentGroups().contains(g1));
    QCOMPARE(m_model->rowCount(idx1), 0);

    r3->addTeamMemberId(r1->id());
    //Debug::print(r3, "-------");
    QVERIFY(r3->teamMembers().contains(r1));
    QCOMPARE(m_model->rowCount(idx3), 0);
}

void ResourceModelTester::groups()
{
    m_model->setGroupsEnabled(true);

    Resource *r1 = new Resource();
    r1->setName("r1");
    r1->setType(Resource::Type_Work);
    AddResourceCmd c1(m_project, r1);
    c1.redo();
    QModelIndex idx1 = m_model->index(r1);
    QVERIFY(idx1.isValid());

    Resource *r2 = new Resource();
    r2->setName("r2");
    r2->setType(Resource::Type_Work);
    AddResourceCmd c2(m_project, r2);
    c2.redo();
    QModelIndex idx2 = m_model->index(r2);
    QVERIFY(idx2.isValid());

    Resource *r3 = new Resource();
    r3->setName("r3");
    r3->setType(Resource::Type_Team);
    AddResourceCmd c3(m_project, r3);
    c3.redo();
    QModelIndex idx3 = m_model->index(r3);
    QVERIFY(idx3.isValid());

    Resource *r4 = new Resource();
    r4->setName("r4");
    r4->setType(Resource::Type_Material);
    AddResourceCmd c4(m_project, r4);
    c4.redo();
    QModelIndex idx4 = m_model->index(r4);
    QVERIFY(idx4.isValid());

    ResourceGroup *g1 = new ResourceGroup();
    g1->setName("g1");
    m_project->addResourceGroup(g1);
    g1->addResource(r1);
    QVERIFY(r1->parentGroups().contains(g1));
    QCOMPARE(m_model->rowCount(idx1), 1);
    QModelIndex idxg1 = m_model->index(0, 0, idx1);
    QVERIFY(idxg1.isValid());
    QCOMPARE(m_model->group(idxg1), g1);

    g1->addResource(r4);
    QVERIFY(r4->parentGroups().contains(g1));
    QCOMPARE(m_model->rowCount(idx4), 1);
    idxg1 = m_model->index(0, 0, idx4);
    QVERIFY(idxg1.isValid());
    QCOMPARE(m_model->group(idxg1), g1);

    ResourceGroup *g2 = new ResourceGroup();
    g2->setName("g2");
    m_project->addResourceGroup(g2);
    g2->addResource(r2);
    QVERIFY(r2->parentGroups().contains(g2));
    QCOMPARE(m_model->rowCount(idx2), 1);
    QModelIndex idxg2 = m_model->index(0, 0, idx2);
    QVERIFY(idxg2.isValid());
    QCOMPARE(m_model->group(idxg2), g2);

    g2->addResource(r3);
    QVERIFY(r3->parentGroups().contains(g2));
    QCOMPARE(m_model->rowCount(idx3), 1);
    idxg2 = m_model->index(0, 0, idx3);
    QVERIFY(idxg2.isValid());
    QCOMPARE(m_model->group(idxg2), g2);

    g2->addResource(r1);
    QVERIFY(r1->parentGroups().contains(g2));
    QCOMPARE(m_model->rowCount(idx1), 2);
    idxg2 = m_model->index(1, 0, idx1);
    QVERIFY(idxg2.isValid());
    QCOMPARE(m_model->group(idxg2), g2);

    r3->addTeamMemberId(r1->id());
    //Debug::print(r3, "-------");
    QVERIFY(r3->teamMembers().contains(r1));
    QCOMPARE(m_model->rowCount(idx3), r3->groupCount());

    idxg2 = m_model->index(1, 0, idx1);
    QVERIFY(idxg2.isValid());
    QCOMPARE(m_model->group(idxg2), g2);
}

void ResourceModelTester::teams()
{
    m_model->setTeamsEnabled(true);

    Resource *r1 = new Resource();
    r1->setName("r1");
    r1->setType(Resource::Type_Work);
    AddResourceCmd c1(m_project, r1);
    c1.redo();
    QModelIndex idx1 = m_model->index(r1);
    QVERIFY(idx1.isValid());

    Resource *r2 = new Resource();
    r2->setName("r2");
    r2->setType(Resource::Type_Work);
    AddResourceCmd c2(m_project, r2);
    c2.redo();
    QModelIndex idx2 = m_model->index(r2);
    QVERIFY(idx2.isValid());

    Resource *r3 = new Resource();
    r3->setName("r3");
    r3->setType(Resource::Type_Team);
    AddResourceCmd c3(m_project, r3);
    c3.redo();
    QModelIndex idx3 = m_model->index(r3);
    QVERIFY(idx3.isValid());

    Resource *r4 = new Resource();
    r4->setName("r4");
    r4->setType(Resource::Type_Material);
    AddResourceCmd c4(m_project, r4);
    c4.redo();
    QModelIndex idx4 = m_model->index(r4);
    QVERIFY(idx4.isValid());

    r3->addTeamMemberId(r1->id());
    //Debug::print(r3, "-------");
    QVERIFY(r3->teamMembers().contains(r1));
    QCOMPARE(m_model->rowCount(idx3), 1);
    QModelIndex idxr3 = m_model->index(0, 0, idx3);
    QVERIFY(idxr3.isValid());
    QCOMPARE(m_model->resource(idxr3), r1);

    ResourceGroup *g1 = new ResourceGroup();
    g1->setName("g1");
    m_project->addResourceGroup(g1);
    g1->addResource(r1);
    QVERIFY(r1->parentGroups().contains(g1));
    QCOMPARE(m_model->rowCount(idx1), r1->teamCount());

    QCOMPARE(m_model->rowCount(idx3), 1);
    idxr3 = m_model->index(0, 0, idx3);
    QVERIFY(idxr3.isValid());
    QCOMPARE(m_model->resource(idxr3), r1);
}

void ResourceModelTester::groupsAndTeams()
{
    m_model->setGroupsEnabled(true);
    m_model->setTeamsEnabled(true);

    Resource *r1 = new Resource();
    r1->setName("r1");
    r1->setType(Resource::Type_Work);
    AddResourceCmd c1(m_project, r1);
    c1.redo();
    QModelIndex idx1 = m_model->index(r1);
    QVERIFY(idx1.isValid());

    Resource *r2 = new Resource();
    r2->setName("r2");
    r2->setType(Resource::Type_Work);
    AddResourceCmd c2(m_project, r2);
    c2.redo();
    QModelIndex idx2 = m_model->index(r2);
    QVERIFY(idx2.isValid());

    Resource *r3 = new Resource();
    r3->setName("r3");
    r3->setType(Resource::Type_Team);
    AddResourceCmd c3(m_project, r3);
    c3.redo();
    QModelIndex idx3 = m_model->index(r3);
    QVERIFY(idx3.isValid());

    Resource *r4 = new Resource();
    r4->setName("r4");
    r4->setType(Resource::Type_Material);
    AddResourceCmd c4(m_project, r4);
    c4.redo();
    QModelIndex idx4 = m_model->index(r4);
    QVERIFY(idx4.isValid());

    ResourceGroup *g1 = new ResourceGroup();
    g1->setName("g1");
    m_project->addResourceGroup(g1);
    g1->addResource(r1);
    QVERIFY(r1->parentGroups().contains(g1));
    QCOMPARE(m_model->rowCount(idx1), 1);
    QModelIndex idxg1 = m_model->index(0, 0, idx1);
    QVERIFY(idxg1.isValid());
    QCOMPARE(m_model->group(idxg1), g1);

    g1->addResource(r4);
    QVERIFY(r4->parentGroups().contains(g1));
    QCOMPARE(m_model->rowCount(idx4), 1);
    idxg1 = m_model->index(0, 0, idx4);
    QVERIFY(idxg1.isValid());
    QCOMPARE(m_model->group(idxg1), g1);

    ResourceGroup *g2 = new ResourceGroup();
    g2->setName("g2");
    m_project->addResourceGroup(g2);
    g2->addResource(r2);
    QVERIFY(r2->parentGroups().contains(g2));
    QCOMPARE(m_model->rowCount(idx2), 1);
    QModelIndex idxg2 = m_model->index(0, 0, idx2);
    QVERIFY(idxg2.isValid());
    QCOMPARE(m_model->group(idxg2), g2);

    g2->addResource(r3);
    QVERIFY(r3->parentGroups().contains(g2));
    QCOMPARE(m_model->rowCount(idx3), 1);
    idxg2 = m_model->index(0, 0, idx3);
    QVERIFY(idxg2.isValid());
    QCOMPARE(m_model->group(idxg2), g2);

    g2->addResource(r1);
    QVERIFY(r1->parentGroups().contains(g2));
    QCOMPARE(m_model->rowCount(idx1), 2);
    idxg2 = m_model->index(1, 0, idx1);
    QVERIFY(idxg2.isValid());
    QCOMPARE(m_model->group(idxg2), g2);

    r3->addTeamMemberId(r1->id());
    //Debug::print(r3, "-------");
    QVERIFY(r3->teamMembers().contains(r1));
    QCOMPARE(m_model->rowCount(idx3), 1 + r3->groupCount());
    QModelIndex idxr1 = m_model->index(r3->groupCount(), 0, idx3);
    QVERIFY(idxr1.isValid());
    QCOMPARE(m_model->resource(idxr1), r1);

    r3->addTeamMemberId(r2->id());
    //Debug::print(r3, "-------");
    QVERIFY(r3->teamMembers().contains(r2));
    QCOMPARE(m_model->rowCount(idx3), 2 + r3->groupCount());
    QModelIndex idxr2 = m_model->index(1 + r3->groupCount(), 0, idx3);
    QVERIFY(idxr2.isValid());
    QCOMPARE(m_model->resource(idxr2), r2);
}

QTEST_GUILESS_MAIN(KPlato::ResourceModelTester)

