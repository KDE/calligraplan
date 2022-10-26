/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2004-2007 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2011 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KPTCOMMAND_H
#define KPTCOMMAND_H

#include "plankernel_export.h"

#include "NamedCommand.h"
#include "MacroCommand.h"

#include <kundo2command.h>

#include <QPointer>
#include <QHash>

#include "kptappointment.h"
#include "kptnode.h"
#include "kptproject.h"
#include "kptduration.h"
#include "kpttask.h"
#include "kptwbsdefinition.h"

class QString;
/**
 * @file
 * This file includes undo/redo commands for kernel data structures
 */

/// The main namespace
namespace KPlato
{

class Locale;
class Account;
class Accounts;
class Calendar;
class CalendarDay;
class Relation;
class ResourceGroupRequest;
class ResourceRequest;
class ResourceGroup;
class Resource;
class Schedule;
class StandardWorktime;


class PLANKERNEL_EXPORT CalendarAddCmd : public NamedCommand
{
public:
    CalendarAddCmd(Project *project, Calendar *cal, int pos, Calendar *parent, const KUndo2MagicString& name = KUndo2MagicString());
    ~CalendarAddCmd() override;
    void execute() override;
    void unexecute() override;

private:
    Project *m_project;
    Calendar *m_cal;
    int m_pos;
    Calendar *m_parent;
    bool m_mine;
};

class PLANKERNEL_EXPORT CalendarRemoveCmd : public NamedCommand
{
public:
    CalendarRemoveCmd(Project *project, Calendar *cal, const KUndo2MagicString& name = KUndo2MagicString());
    ~CalendarRemoveCmd() override;
    void execute() override;
    void unexecute() override;

private:
    Project *m_project;
    Calendar *m_parent;
    Calendar *m_cal;
    int m_index;
    bool m_mine;
    MacroCommand *m_cmd;
};

class PLANKERNEL_EXPORT CalendarMoveCmd : public NamedCommand
{
public:
    CalendarMoveCmd(Project *project, Calendar *cal, int position, Calendar *parent, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Project *m_project;
    Calendar *m_cal;
    int m_newpos;
    int m_oldpos;
    Calendar *m_newparent;
    Calendar *m_oldparent;
};

class PLANKERNEL_EXPORT CalendarModifyNameCmd : public NamedCommand
{
public:
    CalendarModifyNameCmd(Calendar *cal, const QString& newvalue, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Calendar *m_cal;
    QString m_newvalue;
    QString m_oldvalue;
};

class PLANKERNEL_EXPORT CalendarModifyParentCmd : public NamedCommand
{
public:
    CalendarModifyParentCmd(Project *project, Calendar *cal, Calendar *newvalue, const KUndo2MagicString& name = KUndo2MagicString());
    ~CalendarModifyParentCmd() override;
    void execute() override;
    void unexecute() override;

private:
    Project *m_project;
    Calendar *m_cal;
    Calendar *m_newvalue;
    Calendar *m_oldvalue;
    MacroCommand *m_cmd;

    int m_oldindex;
    int m_newindex;
};

class PLANKERNEL_EXPORT CalendarModifyTimeZoneCmd : public NamedCommand
{
public:
    CalendarModifyTimeZoneCmd(Calendar *cal, const QTimeZone &value, const KUndo2MagicString& name = KUndo2MagicString());
    ~CalendarModifyTimeZoneCmd() override;
    void execute() override;
    void unexecute() override;

private:
    Calendar *m_cal;
    QTimeZone m_newvalue;
    QTimeZone m_oldvalue;
    MacroCommand *m_cmd;
};

#ifdef HAVE_KHOLIDAYS
class PLANKERNEL_EXPORT CalendarModifyHolidayRegionCmd : public NamedCommand
{
public:
    CalendarModifyHolidayRegionCmd(Calendar *cal, const QString &value, const KUndo2MagicString& name = KUndo2MagicString());
    ~CalendarModifyHolidayRegionCmd() override;
    void execute() override;
    void unexecute() override;

private:
    Calendar *m_cal;
    QString m_newvalue;
    QString m_oldvalue;
};
#endif

class PLANKERNEL_EXPORT CalendarAddDayCmd : public NamedCommand
{
public:
    CalendarAddDayCmd(Calendar *cal, CalendarDay *newvalue, const KUndo2MagicString& name = KUndo2MagicString());
    ~CalendarAddDayCmd() override;
    void execute() override;
    void unexecute() override;

protected:
    Calendar *m_cal;
    CalendarDay *m_newvalue;
    bool m_mine;
};

class PLANKERNEL_EXPORT CalendarRemoveDayCmd : public NamedCommand
{
public:
    CalendarRemoveDayCmd(Calendar *cal, CalendarDay *day, const KUndo2MagicString& name = KUndo2MagicString());
    CalendarRemoveDayCmd(Calendar *cal, const QDate &day, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

protected:
    Calendar *m_cal;
    CalendarDay *m_value;
    bool m_mine;

private:
    void init();
};

class PLANKERNEL_EXPORT CalendarModifyDayCmd : public NamedCommand
{
public:
    CalendarModifyDayCmd(Calendar *cal, CalendarDay *value, const KUndo2MagicString& name = KUndo2MagicString());
    ~CalendarModifyDayCmd() override;
    void execute() override;
    void unexecute() override;

private:
    Calendar *m_cal;
    CalendarDay *m_newvalue;
    CalendarDay *m_oldvalue;
    bool m_mine;
};

class PLANKERNEL_EXPORT CalendarModifyStateCmd : public NamedCommand
{
public:
    CalendarModifyStateCmd(Calendar *calendar, CalendarDay *day, CalendarDay::State value, const KUndo2MagicString& name = KUndo2MagicString());
    ~CalendarModifyStateCmd() override;
    void execute() override;
    void unexecute() override;

private:
    Calendar *m_calendar;
    CalendarDay *m_day;
    CalendarDay::State m_newvalue;
    CalendarDay::State m_oldvalue;
    MacroCommand *m_cmd;
};

class PLANKERNEL_EXPORT CalendarModifyTimeIntervalCmd : public NamedCommand
{
public:
    CalendarModifyTimeIntervalCmd(Calendar *calendar, TimeInterval &newvalue, TimeInterval *value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Calendar *m_calendar;
    TimeInterval *m_value;
    TimeInterval m_newvalue;
    TimeInterval m_oldvalue;
};

class PLANKERNEL_EXPORT CalendarAddTimeIntervalCmd : public NamedCommand
{
public:
    CalendarAddTimeIntervalCmd(Calendar *calendar, CalendarDay *day, TimeInterval *value, const KUndo2MagicString& name = KUndo2MagicString());
    ~CalendarAddTimeIntervalCmd() override;
    void execute() override;
    void unexecute() override;

protected:
    Calendar *m_calendar;
    CalendarDay *m_day;
    TimeInterval *m_value;
    bool m_mine;
};

class PLANKERNEL_EXPORT CalendarRemoveTimeIntervalCmd : public CalendarAddTimeIntervalCmd
{
public:
    CalendarRemoveTimeIntervalCmd(Calendar *calendar, CalendarDay *day, TimeInterval *value, const KUndo2MagicString& name = KUndo2MagicString());

    void execute() override;
    void unexecute() override;
};

class PLANKERNEL_EXPORT CalendarModifyWeekdayCmd : public NamedCommand
{
public:
    CalendarModifyWeekdayCmd(Calendar *cal, int weekday, CalendarDay *value, const KUndo2MagicString& name = KUndo2MagicString());
    ~CalendarModifyWeekdayCmd() override;
    void execute() override;
    void unexecute() override;

private:
    int m_weekday;
    Calendar *m_cal;
    CalendarDay *m_value;
    CalendarDay m_orig;
};

class PLANKERNEL_EXPORT CalendarModifyDateCmd : public NamedCommand
{
public:
    CalendarModifyDateCmd(Calendar *cal, CalendarDay *day, const QDate &value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Calendar *m_cal;
    CalendarDay *m_day;
    QDate m_newvalue, m_oldvalue;
};

class PLANKERNEL_EXPORT ProjectModifyDefaultCalendarCmd : public NamedCommand
{
public:
    ProjectModifyDefaultCalendarCmd(Project *project, Calendar *cal, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Project *m_project;
    Calendar *m_newvalue, *m_oldvalue;
};


class PLANKERNEL_EXPORT NodeDeleteCmd : public NamedCommand
{
public:
    explicit NodeDeleteCmd(Node *node, const KUndo2MagicString& name = KUndo2MagicString());
    ~NodeDeleteCmd() override;
    void execute() override;
    void unexecute() override;

private:
    Node *m_node;
    Node *m_parent;
    Project *m_project;
    int m_index;
    bool m_mine;
    QList<Appointment*> m_appointments;
    MacroCommand *m_cmd;
    MacroCommand *m_relCmd;
};

class PLANKERNEL_EXPORT TaskAddCmd : public NamedCommand
{
public:
    TaskAddCmd(Project *project, Node *node, Node *after, const KUndo2MagicString& name = KUndo2MagicString());
    ~TaskAddCmd() override;
    void execute() override;
    void unexecute() override;

private:
    Project *m_project;
    Node *m_node;
    Node *m_after;
    bool m_added;
};

class PLANKERNEL_EXPORT SubtaskAddCmd : public NamedCommand
{
public:
    SubtaskAddCmd(Project *project, Node *node, Node *parent, const KUndo2MagicString& name = KUndo2MagicString());
    ~SubtaskAddCmd() override;
    void execute() override;
    void unexecute() override;

private:
    Project *m_project;
    Node *m_node;
    Node *m_parent;
    bool m_added;
    MacroCommand *m_cmd;
    bool m_first;
};


class PLANKERNEL_EXPORT NodeModifyNameCmd : public NamedCommand
{
public:
    NodeModifyNameCmd(Node &node, const QString& nodename, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Node &m_node;
    QString newName;
    QString oldName;
};

class PLANKERNEL_EXPORT NodeModifyPriorityCmd : public NamedCommand
{
public:
    NodeModifyPriorityCmd(Node &node, int oldValue, int newValue, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Node &m_node;
    int m_oldValue;
    int m_newValue;
};

class PLANKERNEL_EXPORT NodeModifyLeaderCmd : public NamedCommand
{
public:
    NodeModifyLeaderCmd(Node &node, const QString& leader, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Node &m_node;
    QString newLeader;
    QString oldLeader;
};

class PLANKERNEL_EXPORT NodeModifyDescriptionCmd : public NamedCommand
{
public:
    NodeModifyDescriptionCmd(Node &node, const QString& description, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Node &m_node;
    QString newDescription;
    QString oldDescription;
};

class PLANKERNEL_EXPORT NodeModifyConstraintCmd : public NamedCommand
{
public:
    NodeModifyConstraintCmd(Node &node, Node::ConstraintType c, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Node &m_node;
    Node::ConstraintType newConstraint;
    Node::ConstraintType oldConstraint;

};

class PLANKERNEL_EXPORT NodeModifyConstraintStartTimeCmd : public NamedCommand
{
public:
    NodeModifyConstraintStartTimeCmd(Node &node, const QDateTime& dt, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Node &m_node;
    QDateTime newTime;
    DateTime oldTime;
    QTimeZone m_timeZone;
};
class PLANKERNEL_EXPORT NodeModifyConstraintEndTimeCmd : public NamedCommand
{
public:
    NodeModifyConstraintEndTimeCmd(Node &node, const QDateTime& dt, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Node &m_node;
    QDateTime newTime;
    DateTime oldTime;
    QTimeZone m_timeZone;
};
class PLANKERNEL_EXPORT NodeModifyStartTimeCmd : public NamedCommand
{
public:
    NodeModifyStartTimeCmd(Node &node, const QDateTime& dt, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Node &m_node;
    QDateTime newTime;
    DateTime oldTime;
    QTimeZone m_timeZone;
};
class PLANKERNEL_EXPORT NodeModifyEndTimeCmd : public NamedCommand
{
public:
    NodeModifyEndTimeCmd(Node &node, const QDateTime& dt, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Node &m_node;
    QDateTime newTime;
    DateTime oldTime;
    QTimeZone m_timeZone;
};
class PLANKERNEL_EXPORT NodeModifyIdCmd : public NamedCommand
{
public:
    NodeModifyIdCmd(Node &node, const QString& id, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Node &m_node;
    QString newId;
    QString oldId;
};

class PLANKERNEL_EXPORT NodeIndentCmd : public NamedCommand
{
public:
    explicit NodeIndentCmd(Node &node, const KUndo2MagicString& name = KUndo2MagicString());
    ~NodeIndentCmd() override;
    void execute() override;
    void unexecute() override;

private:
    Node &m_node;
    Node *m_oldparent, *m_newparent;
    int m_oldindex, m_newindex;
    MacroCommand *m_cmd;
};

class PLANKERNEL_EXPORT NodeUnindentCmd : public NamedCommand
{
public:
    explicit NodeUnindentCmd(Node &node, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Node &m_node;
    Node *m_oldparent, *m_newparent;
    int m_oldindex, m_newindex;
};

class PLANKERNEL_EXPORT NodeMoveUpCmd : public NamedCommand
{
public:
    explicit NodeMoveUpCmd(Node &node, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Node &m_node;
    Project *m_project;
    bool m_moved;
};

class PLANKERNEL_EXPORT NodeMoveDownCmd : public NamedCommand
{
public:
    explicit NodeMoveDownCmd(Node &node, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Node &m_node;
    Project *m_project;
    bool m_moved;
};

class PLANKERNEL_EXPORT NodeMoveCmd : public NamedCommand
{
public:
    NodeMoveCmd(Project *project, Node *node, Node *newParent, int newPos, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Project *m_project;
    Node *m_node;
    Node *m_newparent;
    Node *m_oldparent;
    int m_newpos;
    int m_oldpos;
    bool m_moved;
    MacroCommand m_cmd;
};

class PLANKERNEL_EXPORT AddRelationCmd : public NamedCommand
{
public:
    AddRelationCmd(Project &project, Relation *rel, const KUndo2MagicString& name = KUndo2MagicString());
    ~AddRelationCmd() override;
    void execute() override;
    void unexecute() override;

private:
    Relation *m_rel;
    Project &m_project;
    bool m_taken;

};

class PLANKERNEL_EXPORT DeleteRelationCmd : public NamedCommand
{
public:
    DeleteRelationCmd(Project &project, Relation *rel, const KUndo2MagicString& name = KUndo2MagicString());
    ~DeleteRelationCmd() override;
    void execute() override;
    void unexecute() override;

private:
    Relation *m_rel;
    Project &m_project;
    bool m_taken;

};

class PLANKERNEL_EXPORT ModifyRelationTypeCmd : public NamedCommand
{
public:
    ModifyRelationTypeCmd(Relation *rel, Relation::Type type, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Project *m_project;
    Relation *m_rel;
    Relation::Type m_newtype;
    Relation::Type m_oldtype;

};

class PLANKERNEL_EXPORT ModifyRelationLagCmd : public NamedCommand
{
public:
    ModifyRelationLagCmd(Relation *rel, Duration lag, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Project *m_project;
    Relation *m_rel;
    Duration m_newlag;
    Duration m_oldlag;

};

class PLANKERNEL_EXPORT AddResourceRequestCmd : public NamedCommand
{
public:
    AddResourceRequestCmd(ResourceRequestCollection *requests, ResourceRequest *request, const KUndo2MagicString& name = KUndo2MagicString());
    ~AddResourceRequestCmd() override;
    void execute() override;
    void unexecute() override;

private:
    ResourceRequestCollection *m_collection;
    ResourceRequest *m_request;
    bool m_mine;

};

class PLANKERNEL_EXPORT RemoveResourceRequestCmd : public NamedCommand
{
public:
    RemoveResourceRequestCmd(ResourceRequest *request, const KUndo2MagicString& name = KUndo2MagicString());
    ~RemoveResourceRequestCmd() override;
    void execute() override;
    void unexecute() override;

private:
    ResourceRequestCollection *m_collection;
    ResourceRequest *m_request;
    bool m_mine;

};

class PLANKERNEL_EXPORT ModifyResourceRequestUnitsCmd : public NamedCommand
{
public:
    ModifyResourceRequestUnitsCmd(ResourceRequest *request, int oldvalue, int newvalue, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    ResourceRequest *m_request;
    int m_oldvalue, m_newvalue;

};

class PLANKERNEL_EXPORT ModifyResourceRequestRequiredCmd : public NamedCommand
{
public:
    ModifyResourceRequestRequiredCmd(ResourceRequest *request, const QList<Resource*> &value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    ResourceRequest *m_request;
    QList<Resource*> m_oldvalue, m_newvalue;

};

class PLANKERNEL_EXPORT ModifyEstimateCmd : public NamedCommand
{
public:
    ModifyEstimateCmd(Node &node, double oldvalue, double newvalue, const KUndo2MagicString& name = KUndo2MagicString());
    ~ModifyEstimateCmd() override;
    void execute() override;
    void unexecute() override;

private:
    Estimate *m_estimate;
    double m_oldvalue, m_newvalue;
    int m_optimistic, m_pessimistic;
    MacroCommand *m_cmd;

};

class PLANKERNEL_EXPORT EstimateModifyOptimisticRatioCmd : public NamedCommand
{
public:
    EstimateModifyOptimisticRatioCmd(Node &node, int oldvalue, int newvalue, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Estimate *m_estimate;
    int m_oldvalue, m_newvalue;

};

class PLANKERNEL_EXPORT EstimateModifyPessimisticRatioCmd : public NamedCommand
{
public:
    EstimateModifyPessimisticRatioCmd(Node &node, int oldvalue, int newvalue, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Estimate *m_estimate;
    int m_oldvalue, m_newvalue;

};

class PLANKERNEL_EXPORT ModifyEstimateTypeCmd : public NamedCommand
{
public:
    ModifyEstimateTypeCmd(Node &node, int oldvalue, int newvalue, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Estimate *m_estimate;
    int m_oldvalue, m_newvalue;

};

class PLANKERNEL_EXPORT ModifyEstimateUnitCmd : public NamedCommand
{
public:
    ModifyEstimateUnitCmd(Node &node, Duration::Unit oldvalue, Duration::Unit newvalue, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Estimate *m_estimate;
    Duration::Unit m_oldvalue, m_newvalue;
};

class PLANKERNEL_EXPORT EstimateModifyRiskCmd : public NamedCommand
{
public:
    EstimateModifyRiskCmd(Node &node, int oldvalue, int newvalue, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Estimate *m_estimate;
    int m_oldvalue, m_newvalue;

};

class PLANKERNEL_EXPORT ModifyEstimateCalendarCmd : public NamedCommand
{
public:
    ModifyEstimateCalendarCmd(Node &node, Calendar *oldvalue, Calendar *newvalue, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Estimate *m_estimate;
    Calendar *m_oldvalue, *m_newvalue;

};

class PLANKERNEL_EXPORT MoveResourceCmd : public NamedCommand
{
public:
    MoveResourceCmd(ResourceGroup *group, Resource *resource, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Project &m_project;
    Resource *m_resource;
    ResourceGroup *m_oldvalue, *m_newvalue;
    MacroCommand cmd;
};

class PLANKERNEL_EXPORT ModifyResourceNameCmd : public NamedCommand
{
public:
    ModifyResourceNameCmd(Resource *resource, const QString& value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:

    Resource *m_resource;
    QString m_newvalue;
    QString m_oldvalue;
};
class PLANKERNEL_EXPORT ModifyResourceInitialsCmd : public NamedCommand
{
public:
    ModifyResourceInitialsCmd(Resource *resource, const QString& value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Resource *m_resource;
    QString m_newvalue;
    QString m_oldvalue;
};
class PLANKERNEL_EXPORT ModifyResourceEmailCmd : public NamedCommand
{
public:
    ModifyResourceEmailCmd(Resource *resource, const QString& value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Resource *m_resource;
    QString m_newvalue;
    QString m_oldvalue;
};
class PLANKERNEL_EXPORT ModifyResourceAutoAllocateCmd : public NamedCommand
{
public:
    ModifyResourceAutoAllocateCmd(Resource *resource, bool value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Resource *m_resource;
    bool m_newvalue;
    bool m_oldvalue;
};
class PLANKERNEL_EXPORT ModifyResourceTypeCmd : public NamedCommand
{
public:
    ModifyResourceTypeCmd(Resource *resource, int value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Resource *m_resource;
    int m_newvalue;
    int m_oldvalue;
    MacroCommand m_cmd;
};

class PLANKERNEL_EXPORT ModifyResourceUnitsCmd : public NamedCommand
{
public:
    ModifyResourceUnitsCmd(Resource *resource, int value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Resource *m_resource;
    int m_newvalue;
    int m_oldvalue;
};

class PLANKERNEL_EXPORT ModifyResourceAvailableFromCmd : public NamedCommand
{
public:
    ModifyResourceAvailableFromCmd(Resource *resource, const QDateTime& value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Resource *m_resource;
    QDateTime m_newvalue;
    DateTime m_oldvalue;
    QTimeZone m_timeZone;
};
class PLANKERNEL_EXPORT ModifyResourceAvailableUntilCmd : public NamedCommand
{
public:
    ModifyResourceAvailableUntilCmd(Resource *resource, const QDateTime& value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Resource *m_resource;
    QDateTime m_newvalue;
    DateTime m_oldvalue;
    QTimeZone m_timeZone;
};

class PLANKERNEL_EXPORT ModifyResourceNormalRateCmd : public NamedCommand
{
public:
    ModifyResourceNormalRateCmd(Resource *resource, double value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Resource *m_resource;
    double m_newvalue;
    double m_oldvalue;
};
class PLANKERNEL_EXPORT ModifyResourceOvertimeRateCmd : public NamedCommand
{
public:
    ModifyResourceOvertimeRateCmd(Resource *resource, double value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Resource *m_resource;
    double m_newvalue;
    double m_oldvalue;
};
class PLANKERNEL_EXPORT ModifyResourceCalendarCmd : public NamedCommand
{
public:
    ModifyResourceCalendarCmd(Resource *resource, Calendar *value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Resource *m_resource;
    Calendar *m_newvalue;
    Calendar *m_oldvalue;
};
class PLANKERNEL_EXPORT ModifyRequiredResourcesCmd : public NamedCommand
{
public:
    ModifyRequiredResourcesCmd(Resource *resource, const QStringList &value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Resource *m_resource;
    QStringList m_newvalue;
    QStringList m_oldvalue;
};
class PLANKERNEL_EXPORT ModifyResourceAccountCmd : public NamedCommand
{
public:
    ModifyResourceAccountCmd(Resource *resource, Account *account, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Resource *m_resource;
    Account *m_newvalue;
    Account *m_oldvalue;
};
class PLANKERNEL_EXPORT AddResourceTeamCmd : public NamedCommand
{
public:
    AddResourceTeamCmd(Resource *team, const QString &member, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Resource *m_team;
    QString m_member;
};
class PLANKERNEL_EXPORT RemoveResourceTeamCmd : public NamedCommand
{
public:
    RemoveResourceTeamCmd(Resource *team, const QString &member, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Resource *m_team;
    QString m_member;
};

class PLANKERNEL_EXPORT RemoveResourceGroupCmd : public NamedCommand
{
public:
    RemoveResourceGroupCmd(Project *project, ResourceGroup *parent, ResourceGroup *group, const KUndo2MagicString& name = KUndo2MagicString());
    RemoveResourceGroupCmd(Project *project, ResourceGroup *group, const KUndo2MagicString& name = KUndo2MagicString());
    ~RemoveResourceGroupCmd() override;
    void execute() override;
    void unexecute() override;

protected:
    ResourceGroup *m_group;
    Project *m_project;
    ResourceGroup *m_parent;
    int m_index;
    bool m_mine;
    MacroCommand cmd;
};

class PLANKERNEL_EXPORT AddResourceGroupCmd : public RemoveResourceGroupCmd
{
public:
    AddResourceGroupCmd(Project *project, ResourceGroup *parent, ResourceGroup *group, const KUndo2MagicString& name = KUndo2MagicString());
    AddResourceGroupCmd(Project *project, ResourceGroup *group, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;
};

class PLANKERNEL_EXPORT ModifyResourceGroupNameCmd : public NamedCommand
{
public:
    ModifyResourceGroupNameCmd(ResourceGroup *group, const QString& value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    ResourceGroup *m_group;
    QString m_newvalue;
    QString m_oldvalue;
};

class PLANKERNEL_EXPORT ModifyResourceGroupTypeCmd : public NamedCommand
{
    public:
        ModifyResourceGroupTypeCmd(ResourceGroup *group, const QString &value, const KUndo2MagicString& name = KUndo2MagicString());
        void execute() override;
        void unexecute() override;

    private:
        ResourceGroup *m_group;
        QString m_newvalue;
        QString m_oldvalue;
};

class PLANKERNEL_EXPORT ModifyCompletionEntrymodeCmd : public NamedCommand
{
public:
    ModifyCompletionEntrymodeCmd(Completion &completion, Completion::Entrymode value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Completion &m_completion;
    Completion::Entrymode oldvalue;
    Completion::Entrymode newvalue;
};

class PLANKERNEL_EXPORT ModifyCompletionStartedCmd : public NamedCommand
{
public:
    ModifyCompletionStartedCmd(Completion &completion, bool value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Completion &m_completion;
    bool oldvalue;
    bool newvalue;
};

class PLANKERNEL_EXPORT ModifyCompletionFinishedCmd : public NamedCommand
{
public:
    ModifyCompletionFinishedCmd(Completion &completion, bool value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Completion &m_completion;
    bool oldvalue;
    bool newvalue;
};

class PLANKERNEL_EXPORT ModifyCompletionStartTimeCmd : public NamedCommand
{
public:
    ModifyCompletionStartTimeCmd(Completion &completion, const QDateTime &value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Completion &m_completion;
    DateTime oldvalue;
    QDateTime newvalue;
    QTimeZone m_timeZone;
};

class PLANKERNEL_EXPORT ModifyCompletionFinishTimeCmd : public NamedCommand
{
public:
    ModifyCompletionFinishTimeCmd(Completion &completion, const QDateTime &value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Completion &m_completion;
    DateTime oldvalue;
    QDateTime newvalue;
    QTimeZone m_timeZone;
};

class PLANKERNEL_EXPORT AddCompletionEntryCmd : public NamedCommand
{
public:
    AddCompletionEntryCmd(Completion &completion, const QDate &date, Completion::Entry *value, const KUndo2MagicString& name = KUndo2MagicString());
    ~AddCompletionEntryCmd() override;
    void execute() override;
    void unexecute() override;

private:
    Completion &m_completion;
    QDate m_date;
    Completion::Entry *newvalue;
    bool m_newmine;
};

class PLANKERNEL_EXPORT RemoveCompletionEntryCmd : public NamedCommand
{
public:
    RemoveCompletionEntryCmd(Completion &completion, const QDate& date, const KUndo2MagicString& name = KUndo2MagicString());
    ~RemoveCompletionEntryCmd() override;
    void execute() override;
    void unexecute() override;

private:
    Completion &m_completion;
    QDate m_date;
    Completion::Entry *value;
    bool m_mine;
};

class PLANKERNEL_EXPORT ModifyCompletionEntryCmd : public NamedCommand
{
public:
    ModifyCompletionEntryCmd(Completion &completion, const QDate &date, Completion::Entry *value, const KUndo2MagicString& name = KUndo2MagicString());
    ~ModifyCompletionEntryCmd() override;
    void execute() override;
    void unexecute() override;

private:
    MacroCommand *cmd;
};

class PLANKERNEL_EXPORT ModifyCompletionPercentFinishedCmd : public NamedCommand
{
public:
    ModifyCompletionPercentFinishedCmd(Completion &completion, const QDate &date, int value, const KUndo2MagicString& name = KUndo2MagicString());

    void execute() override;
    void unexecute() override;

private:
    Completion &m_completion;
    QDate m_date;
    int m_newvalue, m_oldvalue;
    MacroCommand cmd;
};

class PLANKERNEL_EXPORT ModifyCompletionRemainingEffortCmd : public NamedCommand
{
public:
    ModifyCompletionRemainingEffortCmd(Completion &completion, const QDate &date, const Duration &value, const KUndo2MagicString &name = KUndo2MagicString());

    void execute() override;
    void unexecute() override;

private:
    Completion &m_completion;
    QDate m_date;
    Duration m_newvalue, m_oldvalue;
    MacroCommand cmd;
};

class PLANKERNEL_EXPORT ModifyCompletionActualEffortCmd : public NamedCommand
{
public:
    ModifyCompletionActualEffortCmd(Completion &completion, const QDate &date, const Duration &value, const KUndo2MagicString &name = KUndo2MagicString());

    void execute() override;
    void unexecute() override;

private:
    Completion &m_completion;
    QDate m_date;
    Duration m_newvalue, m_oldvalue;
    MacroCommand cmd;
};

/**
 * Add used effort for @p resource.
 * Note that the used effort definition in @p value must contain entries for *all* dates.
 * If used effort is already defined it will be replaced.
 */
class PLANKERNEL_EXPORT AddCompletionUsedEffortCmd : public NamedCommand
{
public:
    AddCompletionUsedEffortCmd(Completion &completion, const Resource *resource, Completion::UsedEffort *value, const KUndo2MagicString& name = KUndo2MagicString());
    ~AddCompletionUsedEffortCmd() override;
    void execute() override;
    void unexecute() override;

private:
    Completion &m_completion;
    const Resource *m_resource;
    Completion::UsedEffort *oldvalue;
    Completion::UsedEffort *newvalue;
    bool m_newmine, m_oldmine;
};

class PLANKERNEL_EXPORT AddCompletionActualEffortCmd : public NamedCommand
{
public:
    AddCompletionActualEffortCmd(Task *task, Resource *resource, const QDate &date, const Completion::UsedEffort::ActualEffort &value, const KUndo2MagicString& name = KUndo2MagicString());
    ~AddCompletionActualEffortCmd() override;
    void execute() override;
    void unexecute() override;

private:
    Task *m_task;
    Resource *m_resource;
    QDate m_date;
    Completion::UsedEffort::ActualEffort oldvalue;
    Completion::UsedEffort::ActualEffort newvalue;
};


class PLANKERNEL_EXPORT AddAccountCmd : public NamedCommand
{
public:
    AddAccountCmd(Project &project, Account *account, Account *parent = nullptr, int index = -1, const KUndo2MagicString& name = KUndo2MagicString());
    AddAccountCmd(Project &project, Account *account, const QString& parent, int index = -1, const KUndo2MagicString& name = KUndo2MagicString());
    ~AddAccountCmd() override;
    void execute() override;
    void unexecute() override;

protected:
    bool m_mine;

private:
    Project &m_project;
    Account *m_account;
    Account *m_parent;
    int m_index;
    QString m_parentName;
};

class PLANKERNEL_EXPORT RemoveAccountCmd : public NamedCommand
{
public:
    RemoveAccountCmd(Project &project, Account *account, const KUndo2MagicString& name = KUndo2MagicString());
    ~RemoveAccountCmd() override;
    void execute() override;
    void unexecute() override;

private:
    Project &m_project;
    Account *m_account;
    Account *m_parent;
    int m_index;
    bool m_isDefault;
    bool m_mine;
    MacroCommand m_cmd;
};

class PLANKERNEL_EXPORT RenameAccountCmd : public NamedCommand
{
public:
    RenameAccountCmd(Account *account, const QString& value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Account *m_account;
    QString m_oldvalue;
    QString m_newvalue;
};

class PLANKERNEL_EXPORT ModifyAccountDescriptionCmd : public NamedCommand
{
public:
    ModifyAccountDescriptionCmd(Account *account, const QString& value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Account *m_account;
    QString m_oldvalue;
    QString m_newvalue;
};

class PLANKERNEL_EXPORT NodeModifyStartupCostCmd : public NamedCommand
{
public:
    NodeModifyStartupCostCmd(Node &node, double value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Node &m_node;
    double m_oldvalue;
    double m_newvalue;
};

class PLANKERNEL_EXPORT NodeModifyShutdownCostCmd : public NamedCommand
{
public:
    NodeModifyShutdownCostCmd(Node &node, double value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Node &m_node;
    double m_oldvalue;
    double m_newvalue;
};

class PLANKERNEL_EXPORT NodeModifyRunningAccountCmd : public NamedCommand
{
public:
    NodeModifyRunningAccountCmd(Node &node, Account *oldvalue, Account *newvalue, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Node &m_node;
    Account *m_oldvalue;
    Account *m_newvalue;
};

class PLANKERNEL_EXPORT NodeModifyStartupAccountCmd : public NamedCommand
{
public:
    NodeModifyStartupAccountCmd(Node &node, Account *oldvalue, Account *newvalue, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Node &m_node;
    Account *m_oldvalue;
    Account *m_newvalue;
};

class PLANKERNEL_EXPORT NodeModifyShutdownAccountCmd : public NamedCommand
{
public:
    NodeModifyShutdownAccountCmd(Node &node, Account *oldvalue, Account *newvalue, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Node &m_node;
    Account *m_oldvalue;
    Account *m_newvalue;
};

class PLANKERNEL_EXPORT ModifyDefaultAccountCmd : public NamedCommand
{
public:
    ModifyDefaultAccountCmd(Accounts &acc, Account *oldvalue, Account *newvalue, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Accounts &m_accounts;
    Account *m_oldvalue;
    Account *m_newvalue;
};

class PLANKERNEL_EXPORT ResourceModifyAccountCmd : public NamedCommand
{
public:
    ResourceModifyAccountCmd(Resource &resource, Account *oldvalue, Account *newvalue, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Resource &m_resource;
    Account *m_oldvalue;
    Account *m_newvalue;
};

class PLANKERNEL_EXPORT ProjectModifyConstraintCmd : public NamedCommand
{
public:
    ProjectModifyConstraintCmd(Project &node, Node::ConstraintType c, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Project &m_node;
    Node::ConstraintType newConstraint;
    Node::ConstraintType oldConstraint;

};

class PLANKERNEL_EXPORT ProjectModifyStartTimeCmd : public NamedCommand
{
public:
    ProjectModifyStartTimeCmd(Project &node, const QDateTime& dt, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Project &m_node;
    QDateTime newTime;
    DateTime oldTime;
    QTimeZone m_timeZone;
};

class PLANKERNEL_EXPORT ProjectModifyEndTimeCmd : public NamedCommand
{
public:
    ProjectModifyEndTimeCmd(Project &project, const QDateTime& dt, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Project &m_node;
    QDateTime newTime;
    DateTime oldTime;
    QTimeZone m_timeZone;
};

class PLANKERNEL_EXPORT ProjectModifyWorkPackageInfoCmd : public NamedCommand
{
public:
    ProjectModifyWorkPackageInfoCmd(Project &project, const Project::WorkPackageInfo &wpi, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Project &m_node;
    Project::WorkPackageInfo m_newWpi;
    Project::WorkPackageInfo m_oldWpi;
};

class PLANKERNEL_EXPORT SwapScheduleManagerCmd : public NamedCommand
{
public:
    SwapScheduleManagerCmd(Project &project, ScheduleManager *from, ScheduleManager *to, const KUndo2MagicString& name = KUndo2MagicString());
    ~SwapScheduleManagerCmd() override;
    void execute() override;
    void unexecute() override;

protected:
    Project &m_node;
    ScheduleManager *m_from;
    ScheduleManager *m_to;
};

class PLANKERNEL_EXPORT AddScheduleManagerCmd : public NamedCommand
{
public:
    AddScheduleManagerCmd(Project &project, ScheduleManager *sm, int index = -1, const KUndo2MagicString& name = KUndo2MagicString());
    AddScheduleManagerCmd(ScheduleManager *parent, ScheduleManager *sm, int index = -1, const KUndo2MagicString& name = KUndo2MagicString());
    ~AddScheduleManagerCmd() override;
    void execute() override;
    void unexecute() override;

protected:
    Project &m_node;
    ScheduleManager *m_parent;
    ScheduleManager *m_sm;
    int m_index;
    MainSchedule *m_exp;
    bool m_mine;
    MacroCommand m_cmd;
};

class PLANKERNEL_EXPORT DeleteScheduleManagerCmd : public AddScheduleManagerCmd
{
public:
    DeleteScheduleManagerCmd(Project &project, ScheduleManager *sm, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    MacroCommand cmd;
};

class PLANKERNEL_EXPORT MoveScheduleManagerCmd : public NamedCommand
{
public:
    MoveScheduleManagerCmd(ScheduleManager *sm, ScheduleManager *newparent, int newindex, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    ScheduleManager *m_sm;
    ScheduleManager *m_oldparent;
    int m_oldindex;
    ScheduleManager *m_newparent;
    int m_newindex;
    MacroCommand m_cmd;
};

class PLANKERNEL_EXPORT ModifyScheduleManagerNameCmd : public NamedCommand
{
public:
    ModifyScheduleManagerNameCmd(ScheduleManager &sm, const QString& value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    ScheduleManager &m_sm;
    QString oldvalue, newvalue;
};

class PLANKERNEL_EXPORT ModifyScheduleManagerSchedulingModeCmd : public NamedCommand
{
public:
    ModifyScheduleManagerSchedulingModeCmd(ScheduleManager &sm, int value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    ScheduleManager &m_sm;
    int oldvalue, newvalue;
    MacroCommand m_cmd;
};

class PLANKERNEL_EXPORT ModifyScheduleManagerAllowOverbookingCmd : public NamedCommand
{
public:
    ModifyScheduleManagerAllowOverbookingCmd(ScheduleManager &sm, bool value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    ScheduleManager &m_sm;
    bool oldvalue, newvalue;
};

class PLANKERNEL_EXPORT ModifyScheduleManagerDistributionCmd : public NamedCommand
{
public:
    ModifyScheduleManagerDistributionCmd(ScheduleManager &sm, bool value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    ScheduleManager &m_sm;
    bool oldvalue, newvalue;
};

class PLANKERNEL_EXPORT CalculateScheduleCmd : public NamedCommand
{
public:
    CalculateScheduleCmd(Project &project, ScheduleManager *sm, const KUndo2MagicString& name = KUndo2MagicString());
    ~CalculateScheduleCmd() override;
    void execute() override;
    void unexecute() override;

private:
    Project &m_node;
    QPointer<ScheduleManager> m_sm;
    bool m_first;
    MainSchedule *m_oldexpected;
    MainSchedule *m_newexpected;
    MacroCommand preCmd;
    MacroCommand postCmd;
};

class PLANKERNEL_EXPORT BaselineScheduleCmd : public NamedCommand
{
public:
    explicit BaselineScheduleCmd(ScheduleManager &sm, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    ScheduleManager &m_sm;
};

class PLANKERNEL_EXPORT ResetBaselineScheduleCmd : public NamedCommand
{
public:
    explicit ResetBaselineScheduleCmd(ScheduleManager &sm, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    ScheduleManager &m_sm;
};

class PLANKERNEL_EXPORT ModifyScheduleManagerSchedulingDirectionCmd : public NamedCommand
{
public:
    ModifyScheduleManagerSchedulingDirectionCmd(ScheduleManager &sm, bool value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    ScheduleManager &m_sm;
    bool oldvalue, newvalue;
};

class PLANKERNEL_EXPORT ModifyScheduleManagerSchedulerCmd : public NamedCommand
{
public:
    ModifyScheduleManagerSchedulerCmd(ScheduleManager &sm, int value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    ScheduleManager &m_sm;
    int oldvalue, newvalue;
};

class PLANKERNEL_EXPORT ModifyScheduleManagerSchedulingGranularityIndexCmd : public NamedCommand
{
public:
    ModifyScheduleManagerSchedulingGranularityIndexCmd(ScheduleManager &sm, int value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    ScheduleManager &m_sm;
    int oldvalue, newvalue;
};

class PLANKERNEL_EXPORT ModifyStandardWorktimeYearCmd : public NamedCommand
{
public:
    ModifyStandardWorktimeYearCmd(StandardWorktime *wt, double oldvalue, double newvalue, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;
private:
    StandardWorktime *swt;
    double m_oldvalue;
    double m_newvalue;
};

class PLANKERNEL_EXPORT ModifyStandardWorktimeMonthCmd : public NamedCommand
{
public:
    ModifyStandardWorktimeMonthCmd(StandardWorktime *wt, double oldvalue, double newvalue, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;
private:
    StandardWorktime *swt;
    double m_oldvalue;
    double m_newvalue;
};

class PLANKERNEL_EXPORT ModifyStandardWorktimeWeekCmd : public NamedCommand
{
public:
    ModifyStandardWorktimeWeekCmd(StandardWorktime *wt, double oldvalue, double newvalue, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;
private:
    StandardWorktime *swt;
    double m_oldvalue;
    double m_newvalue;
};

class PLANKERNEL_EXPORT ModifyStandardWorktimeDayCmd : public NamedCommand
{
public:
    ModifyStandardWorktimeDayCmd(StandardWorktime *wt, double oldvalue, double newvalue, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;
private:
    StandardWorktime *swt;
    double m_oldvalue;
    double m_newvalue;
};

class PLANKERNEL_EXPORT DocumentAddCmd : public NamedCommand
{
public:
    DocumentAddCmd(Documents& docs, Document *value, const KUndo2MagicString& name = KUndo2MagicString());
    ~DocumentAddCmd() override;
    void execute() override;
    void unexecute() override;
private:
    Documents& m_docs;
    Document *m_value;
    bool m_mine;
};

class PLANKERNEL_EXPORT DocumentRemoveCmd : public NamedCommand
{
public:
    DocumentRemoveCmd(Documents& docs, Document *value, const KUndo2MagicString& name = KUndo2MagicString());
    ~DocumentRemoveCmd() override;
    void execute() override;
    void unexecute() override;
private:
    Documents& m_docs;
    Document *m_value;
    bool m_mine;
};

class PLANKERNEL_EXPORT DocumentModifyUrlCmd : public NamedCommand
{
public:
    DocumentModifyUrlCmd(Document *doc, const QUrl &url, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;
private:
    Document *m_doc;
    QUrl m_value;
    QUrl m_oldvalue;
};

class PLANKERNEL_EXPORT DocumentModifyNameCmd : public NamedCommand
{
public:
    DocumentModifyNameCmd(Document *doc, const QString &value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;
private:
    Document *m_doc;
    QString m_value;
    QString m_oldvalue;
};

class PLANKERNEL_EXPORT DocumentModifyTypeCmd : public NamedCommand
{
public:
    DocumentModifyTypeCmd(Document *doc, Document::Type value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;
private:
    Document *m_doc;
    Document::Type m_value;
    Document::Type m_oldvalue;
};

class PLANKERNEL_EXPORT DocumentModifyStatusCmd : public NamedCommand
{
public:
    DocumentModifyStatusCmd(Document *doc, const QString &value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;
private:
    Document *m_doc;
    QString  m_value;
    QString  m_oldvalue;
};

class PLANKERNEL_EXPORT DocumentModifySendAsCmd : public NamedCommand
{
    public:
        DocumentModifySendAsCmd(Document *doc, const Document::SendAs value, const KUndo2MagicString& name = KUndo2MagicString());
        void execute() override;
        void unexecute() override;
    private:
        Document *m_doc;
        Document::SendAs m_value;
        Document::SendAs m_oldvalue;
};

class PLANKERNEL_EXPORT WBSDefinitionModifyCmd : public NamedCommand
{
public:
    WBSDefinitionModifyCmd(Project &project, const WBSDefinition value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;
private:
    Project &m_project;
    WBSDefinition m_newvalue, m_oldvalue;
};

class PLANKERNEL_EXPORT WorkPackageAddCmd : public NamedCommand
{
public:
    WorkPackageAddCmd(Project *project, Node *node, WorkPackage *wp, const KUndo2MagicString& name = KUndo2MagicString());
    ~WorkPackageAddCmd() override;
    void execute() override;
    void unexecute() override;
private:
    Project *m_project;
    Node *m_node;
    WorkPackage *m_wp;
    bool m_mine;

};

class PLANKERNEL_EXPORT ModifyProjectLocaleCmd : public MacroCommand
{
public:
    ModifyProjectLocaleCmd(Project &project, const KUndo2MagicString &name);
    void execute() override;
    void unexecute() override;
private:
    Project &m_project;
};

class PLANKERNEL_EXPORT ModifyCurrencySymolCmd : public NamedCommand
{
public:
    ModifyCurrencySymolCmd(Locale *locale, const QString &value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Locale *m_locale;
    QString m_newvalue;
    QString m_oldvalue;
};

class  PLANKERNEL_EXPORT ModifyCurrencyFractionalDigitsCmd : public NamedCommand
{
public:
    ModifyCurrencyFractionalDigitsCmd(Locale *locale, int value, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Locale *m_locale;
    int m_newvalue;
    int m_oldvalue;
};

class  PLANKERNEL_EXPORT AddExternalAppointmentCmd : public NamedCommand
{
public:
    AddExternalAppointmentCmd(Resource *resource, const QString &pid, const QString &pname, const QDateTime &start, const QDateTime &end, double load, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Resource *m_resource;
    QString m_pid;
    QString m_pname;
    QDateTime m_start;
    QDateTime m_end;
    double m_load;
};

class  PLANKERNEL_EXPORT ClearExternalAppointmentCmd : public NamedCommand
{
public:
    ClearExternalAppointmentCmd(Resource *resource, const QString &pid, const KUndo2MagicString& name = KUndo2MagicString());
    ~ClearExternalAppointmentCmd() override;
    void execute() override;
    void unexecute() override;

private:
    Resource *m_resource;
    QString m_pid;
    Appointment *m_appointments;
};

class  PLANKERNEL_EXPORT ClearAllExternalAppointmentsCmd : public NamedCommand
{
public:
    explicit ClearAllExternalAppointmentsCmd(Project *project, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Project *m_project;
    MacroCommand m_cmd;
};

class  PLANKERNEL_EXPORT SharedResourcesFileCmd : public NamedCommand
{
public:
    explicit SharedResourcesFileCmd(Project *project, const QString &newValue, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Project *m_project;
    QString m_oldValue;
    QString m_newValue;
};

class  PLANKERNEL_EXPORT UseSharedResourcesCmd : public NamedCommand
{
public:
    explicit UseSharedResourcesCmd(Project *project, bool newValue, const KUndo2MagicString& name = KUndo2MagicString());
    void execute() override;
    void unexecute() override;

private:
    Project *m_project;
    bool m_oldValue;
    bool m_newValue;
};

}  //KPlato namespace

#endif //COMMAND_H
