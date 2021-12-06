/*
 * ShiftSelection.cpp - TaskJuggler
 *
 * SPDX-FileCopyrightText: 2001, 2002, 2003, 2004 Chris Schlaeger <cs@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */

// clazy:excludeall=qstring-arg
#include "ShiftSelection.h"


namespace TJ
{

ShiftSelection::ShiftSelection(const ShiftSelection& sl) :
    period(new Interval(*sl.period)),
    shift(sl.shift)
{
}

bool
ShiftSelection::isVacationDay(time_t day) const
{
    return period->contains(day) && shift->isVacationDay(day);
}

} // namespace TJ

QDebug operator<<(QDebug dbg, const TJ::ShiftSelection &s)
{
    return dbg << &s;
}
QDebug operator<<(QDebug dbg, const TJ::ShiftSelection *s)
{
    dbg << "ShiftSelection[";
    if (s) dbg << s->getPeriod() << s->getShift();
    else dbg << (void*)s;
    return dbg << ']';
}
