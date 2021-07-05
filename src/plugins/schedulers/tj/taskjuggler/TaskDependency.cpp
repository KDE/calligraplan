/*
 * TaskDependency.cpp - TaskJuggler
 *
 * SPDX-FileCopyrightText: 2001, 2002, 2003, 2004 Chris Schlaeger <cs@kde.org>
 * SPDX-FileCopyrightText: 2011 Dag Andersen <dag.andersen@kdemail.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */

// clazy:excludeall=qstring-arg
#include "TaskDependency.h"

#include <assert.h>

#include "Project.h"
#include "Task.h"
#include "Scenario.h"

namespace TJ
{

TaskDependency::TaskDependency(QString tri, int maxScenarios) :
    taskRefId(tri),
    taskRef(nullptr),
    gapDuration(new long[maxScenarios]),
    gapLength(new long[maxScenarios])
{
    for (int sc = 0; sc < maxScenarios; ++sc)
        gapDuration[sc] = gapLength[sc] = (sc == 0 ? 0 : -1);
}

TaskDependency::~TaskDependency()
{
    delete [] gapDuration;
    delete [] gapLength;
}

long
TaskDependency::getGapDuration(int sc) const
{
    for (; ;)
    {
        if (gapDuration[sc] >= 0)
            return gapDuration[sc];
        Project* p = taskRef->getProject();
        Scenario* parent = p->getScenario(sc)->getParent();
        assert(parent);
        sc = p->getScenarioIndex(parent->getId()) - 1;
    }
}

long
TaskDependency::getGapLength(int sc) const
{
    for (; ;)
    {
        if (gapLength[sc] >= 0)
            return gapLength[sc];
        Project* p = taskRef->getProject();
        Scenario* parent = p->getScenario(sc)->getParent();
        assert(parent);
        sc = p->getScenarioIndex(parent->getId()) - 1;
    }
}

} // namespace TJ

QDebug operator<<(QDebug dbg, const TJ::TaskDependency *dep)
{
    return dep == nullptr ? (dbg<<0x000000) : operator<<(dbg, *dep);
}
QDebug operator<<(QDebug dbg, const TJ::TaskDependency &dep)
{
    dbg<<"TaskDependency[";
    if (dep.getTaskRef()) {
        dbg.nospace()<<"ref="<<dep.getTaskRef()->getId();
    } else {
        dbg.nospace()<<"id="<<dep.getTaskRefId();
    }
    dbg << ']';
    return dbg;
}
