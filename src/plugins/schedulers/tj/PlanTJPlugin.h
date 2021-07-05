/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2009 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PLANTJPLUGIN_H
#define PLANTJPLUGIN_H

#include "plantj_export.h"

#include "kptschedulerplugin.h"

#include <QVariantList>


namespace KPlato
{
    class Project;
    class ScheduleManager;
    class Schedule;
}

using namespace KPlato;

class PLANTJ_EXPORT PlanTJPlugin : public SchedulerPlugin
{
    Q_OBJECT

public:
    PlanTJPlugin(QObject * parent,  const QVariantList &);
    ~PlanTJPlugin() override;

    QString description() const override;
    int capabilities() const override;

    /// Calculate the project
    void calculate(Project &project, ScheduleManager *sm, bool nothread = false) override;

    /// Return the scheduling granularity in milliseconds
    ulong currentGranularity() const;

    void schedule(SchedulingContext &context) override;

Q_SIGNALS:
    void sigCalculationStarted(KPlato::Project*, KPlato::ScheduleManager*);
    void sigCalculationFinished(KPlato::Project*, KPlato::ScheduleManager*);

public Q_SLOTS:
    void stopAllCalculations();
    void stopCalculation(KPlato::SchedulerThread *sch) override;

protected Q_SLOTS:
    void slotStarted(KPlato::SchedulerThread *job);
    void slotFinished(KPlato::SchedulerThread *job);
};


#endif // PLANTJPLUGIN_H
