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
    AddResourceGroupCmd c1(m_project, g1);
    c1.redo();
    QCOMPARE(m_project->numResourceGroups(), 1);
    c1.undo();
    QCOMPARE(m_project->numResourceGroups(), 0);
    c1.redo();
    QCOMPARE(m_project->numResourceGroups(), 1);

    QCOMPARE(m_model.rowCount(), 1);
    
    ResourceGroup *g2 = new ResourceGroup();
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
    AddResourceCmd c11(m_project, r1);
    c11.redo();
    QCOMPARE(m_project->resourceCount(), 1);
    AddParentGroupCmd c12(r1, g1);
    c12.redo();
    QModelIndex idx = m_model.index(g1);
    QVERIFY(idx.isValid());
    QCOMPARE(m_model.rowCount(idx), 0);
    c11.undo();
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
    AddResourceGroupCmd cg(m_project, g1);
    cg.redo();
    QCOMPARE(m_model.rowCount(), 1);

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

    m_model.setResourcesEnabled(false);
    QCOMPARE(m_model.rowCount(m_model.index(g1)), 0);
    m_model.setResourcesEnabled(true);
    QCOMPARE(m_model.rowCount(m_model.index(g1)), 1);
}


QTEST_GUILESS_MAIN(KPlato::ResourceGroupModelTester)
