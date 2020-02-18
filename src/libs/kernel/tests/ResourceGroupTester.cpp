/* This file is part of the KDE project
 * Copyright (C) 2020 Dag Andersen <danders@get2net.dk>
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

#include "ResourceGroupTester.h"

#include <ResourceGroup.h>
#include <Resource.h>


#include <QTest>

#include "debug.cpp"

using namespace KPlato;

ResourceGroup *ResourceGroupTester::createResourceGroup(Project &project, const QString name, ResourceGroup *parent)
{
    ResourceGroup *g = new ResourceGroup();
    QString id = project.uniqueResourceGroupId();
    g->setName(name);
    g->setId(id);
    project.addResourceGroup(g, parent);
    return g;
}

void ResourceGroupTester::topLevelGroups()
{
    Project project;
    QString name = "Group 1";
    ResourceGroup *g = createResourceGroup(project, name);
    QString id = g->id();
    QCOMPARE(project.resourceGroupAt(0), g);
    QCOMPARE(project.findResourceGroup(id), g);

    project.takeResourceGroup(g);
    QCOMPARE(project.resourceGroupCount(), 0);

    project.addResourceGroup(g);
    QCOMPARE(project.resourceGroupAt(0), g);
    delete g;
    QCOMPARE(project.numResourceGroups(), 0);
    QCOMPARE(project.findResourceGroup(id), nullptr);
}

void ResourceGroupTester::childGroups()
{
    Project project;
    QString name = "Group 1";
    ResourceGroup *g1 = createResourceGroup(project, name);
    QCOMPARE(project.resourceGroupAt(0), g1);
    QCOMPARE(project.findResourceGroup(g1->id()), g1);

    name = "Child 1";
    ResourceGroup *c1 = createResourceGroup(project, name, g1);
    QString id = c1->id();
    QCOMPARE(project.findResourceGroup(id), c1);
    QCOMPARE(g1->numChildGroups(), 1);
    QCOMPARE(g1->childGroupAt(0), c1);

    Debug::print(&project, "----", true);
    project.takeResourceGroup(c1);
    QCOMPARE(g1->numChildGroups(), 0);
    QCOMPARE(project.findResourceGroup(id), nullptr);

    project.addResourceGroup(c1, g1);
    QCOMPARE(g1->numChildGroups(), 1);
    QCOMPARE(project.findResourceGroup(id), c1);
    delete c1;
    QCOMPARE(g1->numChildGroups(), 0);
    QCOMPARE(project.findResourceGroup(id), nullptr);
}

void ResourceGroupTester::resources()
{
    
}

QTEST_GUILESS_MAIN(KPlato::ResourceGroupTester)
