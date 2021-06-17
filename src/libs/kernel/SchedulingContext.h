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

#ifndef SCHEDULINGCONTEXT_H
#define SCHEDULINGCONTEXT_H

#include "plankernel_export.h"

#include "kptschedule.h"

#include <QObject>
#include <QVector>
#include <QList>
#include <QMultiMap>

class KoDocument;

namespace KPlato 
{
class Project;

class PLANKERNEL_EXPORT SchedulingContext : public QObject
{
public:
    explicit SchedulingContext(QObject *parent = nullptr);
    ~SchedulingContext();

    void clear();

    void addProject(KoDocument *project, int priority = -1);
    void addResourceBookings(const KoDocument *project);

    Project *project;
    QMultiMap<int, KoDocument*> projects;
    QList<const KoDocument*> resourceBookings;
    QDateTime calculateFrom;
    bool scheduleInParallel;

    QVector<KPlato::Schedule::Log> log;
};

} //namespace KPlato

#endif
