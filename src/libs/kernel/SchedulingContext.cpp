/* This file is part of the KDE project
 * Copyright (C) 2021 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation;
 * version 2 of the License.
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

#include "SchedulingContext.h"

#include "kptproject.h"

using namespace KPlato;

SchedulingContext::SchedulingContext(QObject *parent)
    : QObject(parent)
    , project(nullptr)
    , granularity(0)
    , m_scheduleManager(nullptr)
{
}

SchedulingContext::~SchedulingContext()
{
    clear();
}

void SchedulingContext::clear()
{
    m_scheduleManager = nullptr;
    log.clear();
    projects.clear();
    resourceBookings.clear();
    delete project;
    project = nullptr;
}

ScheduleManager *SchedulingContext::scheduleManager() const
{
    return m_scheduleManager;
}

void SchedulingContext::addResourceBookings(const Project *project)
{
    if (!project->currentScheduleManager()) {
        errorPlan<<"ERROR"<<Q_FUNC_INFO<<"No current schedule manager";
        return;
    }
    if (!resourceBookings.contains(project)) {
        resourceBookings.append(project);
        debugPlan<<project<<"Add resource bookings, using Schedule Manager"<<project->currentScheduleManager();
    } else {
        warnPlan<<"WARN"<<Q_FUNC_INFO<<project<<"Resource bookings already added";
    }
}

void SchedulingContext::addProject(Project *project, int priority)
{    
    if (!project->currentScheduleManager()) {
        errorPlan<<"ERROR"<<Q_FUNC_INFO<<"No current schedule manager";
        return;
    }
    if (projects.values().contains(project)) {
        int prio = projects.key(project);
        projects.remove(prio, project);
    }
    projects.insert(priority, project);
}
