/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2007, 2012 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptglobal.h"

#include <KLocalizedString>

namespace KPlato
{

// namespace SchedulingState
// {
    QString SchedulingState::deleted(bool trans)
        { return trans ? i18n("Deleted") : QString("Deleted"); }
    QString SchedulingState::notScheduled(bool trans)
        { return trans ? i18n("Not scheduled") : QString("Not scheduled"); }
    QString SchedulingState::scheduled(bool trans)
        { return trans ? i18n("Scheduled") : QString("Scheduled"); }
    QString SchedulingState::resourceOverbooked(bool trans)
        { return trans ? i18n("Resource overbooked") : QString("Resource overbooked"); }
    QString SchedulingState::resourceNotAvailable(bool trans)
        { return trans ? i18n("Resource not available") : QString("Resource not available"); }
    QString SchedulingState::resourceNotAllocated(bool trans)
        { return trans ? i18n("No resource allocated") : QString("No resource allocated"); }
    QString SchedulingState::constraintsNotMet(bool trans)
        { return trans ? i18n("Cannot fulfill constraints") : QString("Cannot fulfill constraints"); }
    QString SchedulingState::effortNotMet(bool trans)
        { return trans ? i18n("Effort not met") : QString("Effort not met"); }
    QString SchedulingState::schedulingError(bool trans)
        { return trans ? i18n("Scheduling error") : QString("Scheduling error"); }

//} namespace SchedulingState

} //namespace KPlato

