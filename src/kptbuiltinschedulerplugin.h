/* This file is part of the KDE project
  Copyright (C) 2009 Dag Andersen <dag.andersen@kdemail.net>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#ifndef KPTBUILTINSCHEDULERPLUGIN_H
#define KPTBUILTINSCHEDULERPLUGIN_H

#include "plan_export.h"
#include "kptschedulerplugin.h"

#include "kptschedule.h"

class KoDocument;

namespace KPlato
{

class KPlatoScheduler;
class Project;
class ScheduleManager;
class SchedulingContext;

class PLAN_EXPORT BuiltinSchedulerPlugin : public SchedulerPlugin
{
    Q_OBJECT
public:
    explicit BuiltinSchedulerPlugin(QObject *parent);
    ~BuiltinSchedulerPlugin() override;

    QString description() const override;
    /// Calculate the project
    void calculate(Project &project, ScheduleManager *sm, bool nothread = false) override;

    void schedule(SchedulingContext &context) override;

Q_SIGNALS:
    void sigCalculationStarted(KPlato::Project*, KPlato::ScheduleManager*);
    void sigCalculationFinished(KPlato::Project*, KPlato::ScheduleManager*);
    void maxProgress(int, KPlato::ScheduleManager*);
    void sigProgress(int, KPlato::ScheduleManager*);

protected Q_SLOTS:
    void slotStarted(KPlato::SchedulerThread *job);
    void slotFinished(KPlato::SchedulerThread *job);
};


class KPlatoScheduler : public SchedulerThread
{
    Q_OBJECT

public:
    KPlatoScheduler(QObject *parent = nullptr);
    KPlatoScheduler(Project *project, ScheduleManager *sm, QObject *parent = nullptr);
    ~KPlatoScheduler() override;

    void schedule(SchedulingContext &context) override;

public Q_SLOTS:
    /// Stop scheduling.
    void stopScheduling() override;
    /// Halt scheduling
    void haltScheduling() override { m_haltScheduling = true; stopScheduling(); }

protected:
    void run() override;
    void calculateProject(SchedulingContext &context, KoDocument *project, QList<const KoDocument*> includes);

};

} //namespace KPlato

#endif
