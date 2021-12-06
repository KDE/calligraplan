/* This file is part of the KDE project
 SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 
 SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "SchedulingContext.h"

#include "kptproject.h"

using namespace KPlato;

SchedulingContext::SchedulingContext(QObject *parent)
    : QObject(parent)
    , project(nullptr)
    , scheduleInParallel(false)
{
}

SchedulingContext::~SchedulingContext()
{
    clear();
}

void SchedulingContext::clear()
{
    log.clear();
    projects.clear();
    resourceBookings.clear();
    delete project;
    project = nullptr;
    scheduleInParallel = false;
}


void SchedulingContext::addResourceBookings(const KoDocument *project)
{
    if (!resourceBookings.contains(project)) {
        resourceBookings.append(project);
        debugPlan<<project<<"Add resource bookings";
    } else {
        warnPlan<<"WARN"<<Q_FUNC_INFO<<project<<"Resource bookings already added";
    }
}

void SchedulingContext::addProject(KoDocument *project, int priority)
{
    int prio = projects.key(project, -1);
    if (prio != -1) {
        projects.remove(prio, project);
    }
    projects.insert(priority, project);
}
