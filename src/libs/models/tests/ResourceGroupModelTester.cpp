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
#include "ResourceGroupModelTester.h"

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

void ResourceGroupModelTester::init()
{
    m_model.setResourcesEnabled(true);

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
    m_model.setProject(m_project);
}

void ResourceGroupModelTester::cleanup()
{
    m_project->deref();
}

void ResourceGroupModelTester::groups()
{
    m_model.setResourcesEnabled(false);

    QCOMPARE(m_project->numResourceGroups(), 0);
    ResourceGroup *g1 = new ResourceGroup();
    g1->setName("g1");
    AddResourceGroupCmd c1(m_project, g1);
    c1.redo();
    QCOMPARE(m_project->numResourceGroups(), 1);
    c1.undo();
    QCOMPARE(m_project->numResourceGroups(), 0);
    c1.redo();
    QCOMPARE(m_project->numResourceGroups(), 1);

    QCOMPARE(m_model.rowCount(), 1);

    ResourceGroup *g2 = new ResourceGroup();
    g2->setName("g2");
    AddResourceGroupCmd c2(m_project, g2);
    c2.redo();
    QCOMPARE(m_project->numResourceGroups(), 2);
    c2.undo();
    QCOMPARE(m_project->numResourceGroups(), 1);
    c2.redo();
    QCOMPARE(m_project->numResourceGroups(), 2);

    QCOMPARE(m_model.rowCount(), 2);

    // resources should not pop up in the model
    Resource *r1 = new Resource();
    r1->setName("r1");
    AddResourceCmd c11(m_project, r1);
    c11.redo();
    QCOMPARE(m_project->resourceCount(), 1);
    AddParentGroupCmd c12(r1, g1);
    c12.redo();
    //Debug::print(m_project, "--------", true);
    QModelIndex idx = m_model.index(g1);
    QVERIFY(idx.isValid());
    QCOMPARE(m_model.rowCount(idx), 0);
    c11.undo();
}

void ResourceGroupModelTester::childGroups()
{
    m_model.setResourcesEnabled(false);

    QCOMPARE(m_project->numResourceGroups(), 0);
    ResourceGroup *g1 = new ResourceGroup();
    AddResourceGroupCmd c1(m_project, g1);
    c1.redo();
    QCOMPARE(m_project->numResourceGroups(), 1);
    QCOMPARE(m_model.rowCount(), 1);
    QModelIndex idx1 = m_model.index(g1);
    QVERIFY(idx1.isValid());
    QCOMPARE(m_model.rowCount(idx1), 0);
    QVERIFY(!idx1.parent().isValid());

    ResourceGroup *g2 = new ResourceGroup();
    AddResourceGroupCmd c2(m_project, g2);
    c2.redo();
    QCOMPARE(m_project->numResourceGroups(), 2);
    QCOMPARE(m_model.rowCount(), 2);
    QModelIndex idx2 = m_model.index(g2);
    QVERIFY(idx2.isValid());
    QCOMPARE(m_model.rowCount(idx2), 0);
    QVERIFY(!idx2.parent().isValid());

    ResourceGroup *g11 = new ResourceGroup();
    AddResourceGroupCmd c11(m_project, g1, g11);
    c11.redo();
    QCOMPARE(g1->numChildGroups(), 1);
    QCOMPARE(m_model.rowCount(idx1), 1);
    QModelIndex idx11 = m_model.index(g11);
    QVERIFY(idx11.isValid());

    QCOMPARE(m_model.group(idx11), g11);
    QCOMPARE(idx11.parent(), idx1);
    QCOMPARE(m_model.group(idx11.parent()), g1);

    ResourceGroup *g21 = new ResourceGroup();
    AddResourceGroupCmd c21(m_project, g2, g21);
    c21.redo();
    QCOMPARE(g2->numChildGroups(), 1);
    QCOMPARE(m_model.rowCount(idx2), 1);
    QModelIndex idx21 = m_model.index(g21);
    QVERIFY(idx21.isValid());

    QCOMPARE(m_model.group(idx21), g21);
    QCOMPARE(idx21.parent(), idx2);
    QCOMPARE(m_model.group(idx21.parent()), g2);

    ResourceGroup *g12 = new ResourceGroup();
    AddResourceGroupCmd c12(m_project, g1, g12);
    c12.redo();
    QCOMPARE(g1->numChildGroups(), 2);
    QCOMPARE(m_model.rowCount(idx1), 2);
    QModelIndex idx12 = m_model.index(g12);
    QVERIFY(idx12.isValid());

    QCOMPARE(m_model.group(idx12), g12);
    QCOMPARE(idx12.parent(), idx1);
    QCOMPARE(m_model.group(idx12.parent()), g1);

    ResourceGroup *g111 = new ResourceGroup();
    AddResourceGroupCmd c111(m_project, g11, g111);
    c111.redo();
    QCOMPARE(g11->numChildGroups(), 1);
    QCOMPARE(m_model.rowCount(idx11), 1);
    QModelIndex idx111 = m_model.index(g111);
    QVERIFY(idx111.isValid());

    QCOMPARE(m_model.group(idx111), g111);
    QCOMPARE(idx111.parent(), idx11);
    QCOMPARE(m_model.group(idx111.parent()), g11);

    c111.undo();
    QCOMPARE(g11->numChildGroups(), 0);
    QCOMPARE(m_model.rowCount(idx11), 0);

    c12.undo();
    QCOMPARE(g1->numChildGroups(), 1);
    QCOMPARE(m_model.rowCount(idx1), 1);

    c21.undo();
    QCOMPARE(g2->numChildGroups(), 0);
    QCOMPARE(m_model.rowCount(idx2), 0);

    c11.undo();
    QCOMPARE(g1->numChildGroups(), 0);
    QCOMPARE(m_model.rowCount(idx1), 0);
}

void ResourceGroupModelTester::resources()
{
    m_model.setResourcesEnabled(true);

    QCOMPARE(m_project->resourceCount(), 0);
    Resource *r1 = new Resource();
    AddResourceCmd c1(m_project, r1);
    c1.redo();
    QCOMPARE(m_project->resourceCount(), 1);
    c1.undo();
    QCOMPARE(m_project->resourceCount(), 0);
    c1.redo();
    QCOMPARE(m_project->resourceCount(), 1);

    Resource *r2 = new Resource();
    AddResourceCmd c2(m_project, r2);
    c2.redo();
    QCOMPARE(m_project->resourceCount(), 2);
    c2.undo();
    QCOMPARE(m_project->resourceCount(), 1);
    c2.redo();
    QCOMPARE(m_project->resourceCount(), 2);

    QCOMPARE(m_model.rowCount(), 0);

    ResourceGroup *g1 = new ResourceGroup();
    AddResourceGroupCmd cg1(m_project, g1);
    cg1.redo();
    QCOMPARE(m_model.rowCount(), 1);
    QModelIndex idx1 = m_model.index(g1);

    AddParentGroupCmd c3(r1, g1);
    c3.redo();
    QCOMPARE(m_model.rowCount(), 1);
    QCOMPARE(m_model.rowCount(m_model.index(g1)), 1);
    c3.undo();
    QCOMPARE(m_model.rowCount(m_model.index(g1)), 0);
    c3.redo();

    AddParentGroupCmd c4(r2, g1);
    c4.redo();
    QCOMPARE(m_model.rowCount(), 1);
    QCOMPARE(m_model.rowCount(m_model.index(g1)), 2);
    c4.undo();
    QCOMPARE(m_model.rowCount(m_model.index(g1)), 1);
    c4.redo();
    QCOMPARE(m_model.rowCount(m_model.index(g1)), 2);

    ResourceGroup *g2 = new ResourceGroup();
    AddResourceGroupCmd cg2(m_project, g2);
    cg2.redo();
    QCOMPARE(m_model.rowCount(), 2);

    ResourceGroup *g11 = new ResourceGroup();
    AddResourceGroupCmd cg11(m_project, g1, g11);
    cg11.redo();
    QModelIndex idx11 = m_model.index(g11);
    QModelIndex idx12 = idx11.sibling(idx11.row()+1, idx11.column()); // r1
    QModelIndex idx13 = idx11.sibling(idx12.row()+1, idx11.column()); // r2
    QCOMPARE(m_model.rowCount(m_model.index(g1)), 3); // group + 2 resources
    QCOMPARE(m_model.resource(idx12), r1);
    QCOMPARE(m_model.resource(idx13), r2);
    QCOMPARE(idx11.parent(), idx1);
    // check if parent of a resource index works
    QCOMPARE(idx12.parent(), idx1);
    QCOMPARE(idx13.parent(), idx1);

    AddParentGroupCmd c5(r1, g2);
    c5.redo();
    QCOMPARE(m_model.rowCount(m_model.index(g2)), 1);
    QCOMPARE(m_model.resource(m_model.index(0, 0, m_model.index(g2))), r1);
    QCOMPARE(m_model.resource(idx11.sibling(idx11.row()+1, idx11.column())), r1); // found both places

    AddParentGroupCmd c6(r2, g2);
    c6.redo();
    QCOMPARE(m_model.rowCount(m_model.index(g2)), 2);
    QCOMPARE(m_model.resource(m_model.index(1, 0, m_model.index(g2))), r2);
    QCOMPARE(m_model.resource(idx11.sibling(idx11.row()+2, idx11.column())), r2); // found both places
}

QTEST_GUILESS_MAIN(KPlato::ResourceGroupModelTester)
