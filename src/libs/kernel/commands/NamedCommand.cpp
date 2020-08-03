/* This file is part of the KDE project
 * Copyright (C) 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
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

// clazy:excludeall=qstring-arg
#include "NamedCommand.h"
#include "kptschedule.h"
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
    foreach (Appointment * a, sch->appointments()) {
        if (a->node() == sch) {
            m_schedules.insert(a->resource(), a->resource() ->isScheduled());
        } else if (a->resource() == sch) {
            m_schedules.insert(a->node(), a->node() ->isScheduled());
        }
    }
}
