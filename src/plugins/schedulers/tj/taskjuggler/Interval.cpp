/*
 * Interval.h - TaskJuggler
 *
 * SPDX-FileCopyrightText: 2011 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2011 Dag Andersen <dag.andersen@kdemail.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * $Id$
 */

// clazy:excludeall=qstring-arg
#include "Interval.h"

QDebug operator<<(QDebug dbg, const TJ::Interval *i)
{
    return i == nullptr ? (dbg << (void*)i) : operator<<(dbg, *i);
}

QDebug operator<<(QDebug dbg, const TJ::Interval &i)
{
    dbg << "Interval[";
    if (i.isNull()) dbg << "Null";
    else dbg << TJ::time2ISO(i.getStart())<<"-"<<TJ::time2ISO(i.getEnd());
    dbg << "]";
    return dbg;
}
