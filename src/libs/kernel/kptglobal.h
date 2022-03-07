/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2006, 2007 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTGLOBAL_H
#define KPTGLOBAL_H

#include "plankernel_export.h"

#include <Qt> // for things in Qt namespace
#include <QLatin1String>

// The Plan file syntax is used in parts of the KPlatoWork file, so:
// * If you change PLAN_FILE_SYNTAX_VERSION, change PLANWORK_FILE_SYNTAX_VERSION too!
// * You don't need to change PLAN_FILE_SYNTAX_VERSION when you change KPLATOWORK_FILE_SYNTAX_VERSION
static const QLatin1String PLAN_FILE_SYNTAX_VERSION("0.7.0");
static const QLatin1String PLANWORK_FILE_SYNTAX_VERSION("0.7.0");

#define CURRENTSCHEDULE     -1
#define NOTSCHEDULED        -2
#define BASELINESCHEDULE    -3
#define ANYSCHEDULED        -4

namespace KPlato
{

/// EffortCostCalculationType controls how effort and cost is calculated
enum EffortCostCalculationType {
    ECCT_All, /// Include both work and material in both effort and cost calculations
    ECCT_EffortWork, /// Include only Work in effort calculations, both work and material in cost calculations
    ECCT_Work /// Include only Work in both effort and cost calculations
};

enum ObjectType {
    OT_None = 0,
    OT_Project,
    OT_Task,
    OT_Summarytask,
    OT_ResourceGroup,
    OT_Resource,
    OT_Appointment,
    OT_External,
    OT_Interval,
    OT_ScheduleManager,
    OT_Schedule,
    OT_Calendar,
    OT_CalendarWeek,
    OT_CalendarDay
};

namespace Role
{
    enum Roles {
        EnumList = Qt::UserRole + 1,
        EnumListValue,
        List,
        ListValues,
        DurationUnit,
        DurationScales,
        Maximum,
        Minimum,
        EditorType,
        ReadWrite,
        ObjectType,
        InternalAppointments,
        ExternalAppointments,
        ColumnTag,
        Planned,
        Actual,
        Foreground,
        Object
    };
} //namespace Role


struct PLANKERNEL_EXPORT SchedulingState
{
    static QString deleted(bool trans = true);
    static QString notScheduled(bool trans = true);
    static QString scheduled(bool trans = true);
    static QString resourceOverbooked(bool trans = true);
    static QString resourceNotAvailable(bool trans = true);
    static QString resourceNotAllocated(bool trans = true);
    static QString constraintsNotMet(bool trans = true);
    static QString effortNotMet(bool trans = true);
    static QString schedulingError(bool trans = true);

}; //namespace WhatsThis


} //namespace KPlato

#endif
