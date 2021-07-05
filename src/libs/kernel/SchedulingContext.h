/* This file is part of the KDE project
 SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>

 SPDX-License-Identifier: LGPL-2.0-or-later
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
