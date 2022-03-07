/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2020 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
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
    QString name = QStringLiteral("Group 1");
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
    QString name = QStringLiteral("Group 1");
    ResourceGroup *g1 = createResourceGroup(project, name);
    QCOMPARE(project.resourceGroupAt(0), g1);
    QCOMPARE(project.findResourceGroup(g1->id()), g1);

    name = QStringLiteral("Child 1");
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
