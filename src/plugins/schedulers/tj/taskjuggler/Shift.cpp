/*
 * Shift.cpp - TaskJuggler
 *
 * SPDX-FileCopyrightText: 2001, 2002, 2003, 2004 Chris Schlaeger <cs@kde.org>
 * SPDX-FileCopyrightText: 2011 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */

// clazy:excludeall=qstring-arg
#include "Shift.h"
#include "Project.h"
#include "debug.h"

namespace TJ
{

Shift::Shift(Project* prj, const QString& i, const QString& n, Shift* p,
             const QString& df, uint dl) :
    CoreAttributes(prj, i, n, p, df, dl),
    workingHours()
{
    prj->addShift(this);

    for (int i = 0; i < 7; i++)
    {
        workingHours[i] = new QList<Interval*>();
//         workingHours[i]->setAutoDelete(true);
    }
}

Shift::~Shift()
{
    for (int i = 0; i < 7; i++)
        delete workingHours[i];
    project->deleteShift(this);
}

void
Shift::inheritValues()
{
    Shift* p = (Shift*) parent;

    if (p)
    {
        // Inherit start values from parent resource.
        for (int i = 0; i < 7; i++)
        {
            while (!workingHours[i]->isEmpty()) delete workingHours[i]->takeFirst();
            delete workingHours[i];
            workingHours[i] = new QList<Interval*>();
//             workingHours[i]->setAutoDelete(true);
            for (QListIterator<Interval*> ivi(*(p->workingHours[i])); ivi.hasNext();)
                workingHours[i]->append(new Interval(*ivi.next()));
        }
    }
    else
    {
        // Inherit start values from project defaults.
        for (int i = 0; i < 7; i++)
        {
            while (!workingHours[i]->isEmpty()) delete workingHours[i]->takeFirst();
            delete workingHours[i];
            workingHours[i] = new QList<Interval*>();
//             workingHours[i]->setAutoDelete(true);
            for (QListIterator<Interval*>
                 ivi(project->getWorkingHoursIterator(i)); ivi.hasNext();)
                workingHours[i]->append(new Interval(*ivi.next()));
        }
    }
}

void
Shift::setWorkingHours(int day, const QList<Interval*>& l)
{
    while (!workingHours[day]->isEmpty()) delete workingHours[day]->takeFirst();
    delete workingHours[day];

    // Create a deep copy of the interval list.
    workingHours[day] = new QList<Interval*>;
//     workingHours[day]->setAutoDelete(true);
    for (QListIterator<Interval*> pli(l); pli.hasNext();)
        workingHours[day]->append(new Interval(*(pli.next())));
}

ShiftListIterator
Shift::getSubListIterator() const
{
    return ShiftListIterator(*sub);
}

bool
Shift::isOnShift(const Interval& iv) const
{
    // Hacky way to enable handling working hours specified in timezone different from local time
    if (!workingIntervals.isEmpty()) {
        if (iv.getStart() >= workingIntervals.last().getEnd()) {
            return false;
        }
        for (const Interval &i : std::as_const(workingIntervals)) {
            if (iv.getEnd() <= i.getStart()) {
                return false;
            }
            if (iv.overlaps(i)) {
                return true;
            }
        }
        return false;
    }
    int dow = dayOfWeek(iv.getStart(), false);
    int ivStart = secondsOfDay(iv.getStart());
    int ivEnd = secondsOfDay(iv.getEnd());
    Interval dayIv(ivStart, ivEnd);
    for (QListIterator<Interval*> ili(*(workingHours[dow])); ili.hasNext();)
        if (ili.next()->contains(dayIv))
            return true;

    return false;
}

bool
Shift::isVacationDay(time_t day) const
{
    return workingHours[dayOfWeek(day, false)]->isEmpty();
}

void
Shift::addWorkingInterval(const Interval &interval)
{
    workingIntervals.append(interval);
}

QList<Interval>
Shift::getWorkingIntervals() const
{
    return workingIntervals;
}

} // namespace TJ

QDebug operator<<(QDebug dbg, const TJ::Shift &s)
{
    return dbg << &s;
}
QDebug operator<<(QDebug dbg, const TJ::Shift *s)
{
    dbg << "Shift[";
    if (s) dbg << s->getWorkingIntervals();
    else dbg << (void*)s;
    return dbg << ']';
}
