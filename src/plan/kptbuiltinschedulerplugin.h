/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2009 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
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
    explicit KPlatoScheduler(ulong granularityIndex = 0, QObject *parent = nullptr);
    explicit KPlatoScheduler(Project *project, ScheduleManager *sm, ulong granularityIndex = 0, QObject *parent = nullptr);
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
