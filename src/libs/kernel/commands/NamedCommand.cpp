/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "NamedCommand.h"
#include "kptschedule.h"
#include "kptappointment.h"
#include "kptdebug.h"

#include <QApplication>


using namespace KPlato;


void NamedCommand::setSchScheduled()
{
    QHash<Schedule*, bool>::ConstIterator it;
    for (it = m_schedules.constBegin(); it != m_schedules.constEnd(); ++it) {
        //debugPlan << it.key() ->name() <<":" << it.value();
        it.key() ->setScheduled(it.value());
    }
}
void NamedCommand::setSchScheduled(bool state)
{
    QHash<Schedule*, bool>::ConstIterator it;
    for (it = m_schedules.constBegin(); it != m_schedules.constEnd(); ++it) {
        //debugPlan << it.key() ->name() <<":" << state;
        it.key() ->setScheduled(state);
    }
}
void NamedCommand::addSchScheduled(Schedule *sch)
{
    //debugPlan << sch->id() <<":" << sch->isScheduled();
    m_schedules.insert(sch, sch->isScheduled());
    const auto appointments  = sch->appointments();
    for (Appointment * a : appointments) {
        if (a->node() == sch) {
            m_schedules.insert(a->resource(), a->resource() ->isScheduled());
        } else if (a->resource() == sch) {
            m_schedules.insert(a->node(), a->node() ->isScheduled());
        }
    }
}
