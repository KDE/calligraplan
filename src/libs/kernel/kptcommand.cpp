/* This file is part of the KDE project
 * Copyright (C) 2004 - 2007 Dag Andersen <dag.andersen@kdemail.net>
 * Copyright (C) 2011 Dag Andersen <dag.andersen@kdemail.net>
 * Copyright (C) 2016 Dag Andersen <dag.andersen@kdemail.net>
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
#include "kptcommand.h"
#include "AddResourceCmd.h"
#include "AddParentGroupCmd.h"
#include "RemoveParentGroupCmd.h"
#include "RemoveResourceCmd.h"
#include "kptaccount.h"
#include "kptappointment.h"
#include "kptproject.h"
#include "kpttask.h"
#include "kptcalendar.h"
#include "kptrelation.h"
#include "kptresource.h"
#include "kptdocuments.h"
#include "kptlocale.h"
#include "kptdebug.h"

#include <QApplication>


const QLoggingCategory &PLANCMDINSPROJECT_LOG()
{
    static const QLoggingCategory category("calligra.plan.command.insertProject");
    return category;
}

#define debugPlanInsertProject qCDebug(PLANCMDINSPROJECT_LOG)
#define warnPlanInsertProject qCWarning(PLANCMDINSPROJECT_LOG)
#define errorPlanInsertProject qCCritical(PLANCMDINSPROJECT_LOG)


namespace KPlato
{

CalendarAddCmd::CalendarAddCmd(Project *project, Calendar *cal, int pos, Calendar *parent, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_project(project),
        m_cal(cal),
        m_pos(pos),
        m_parent(parent),
        m_mine(true)
{
    //debugPlan<<cal->name();
    Q_ASSERT(project != nullptr);
}
CalendarAddCmd::~CalendarAddCmd()
{
    if (m_mine)
        delete m_cal;
}
void CalendarAddCmd::execute()
{
    if (m_project) {
        m_project->addCalendar(m_cal, m_parent, m_pos);
        m_mine = false;
    }

    //debugPlan<<m_cal->name()<<" added to:"<<m_project->name();
}

void CalendarAddCmd::unexecute()
{
    if (m_project) {
        m_project->takeCalendar(m_cal);
        m_mine = true;
    }

    //debugPlan<<m_cal->name();
}

CalendarRemoveCmd::CalendarRemoveCmd(Project *project, Calendar *cal, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_project(project),
        m_parent(cal->parentCal()),
        m_cal(cal),
        m_index(-1),
        m_mine(false),
        m_cmd(new MacroCommand(KUndo2MagicString()))
{
    Q_ASSERT(project != nullptr);

    m_index = m_parent ? m_parent->indexOf(cal) : project->indexOf(cal);

    const auto resources = project->resourceList();
    for (Resource *r : resources) {
        if (r->calendar(true) == cal) {
            m_cmd->addCommand(new ModifyResourceCalendarCmd(r, nullptr));
        }
    }
    if (project->defaultCalendar() == cal) {
        m_cmd->addCommand(new ProjectModifyDefaultCalendarCmd(project, nullptr));
    }
    const auto calendars = cal->calendars();
    for (Calendar *c : calendars) {
        m_cmd->addCommand(new CalendarRemoveCmd(project, c));
    }
}
CalendarRemoveCmd::~CalendarRemoveCmd()
{
    delete m_cmd;
    if (m_mine)
        delete m_cal;
}
void CalendarRemoveCmd::execute()
{
    m_cmd->execute();
    m_project->takeCalendar(m_cal);
    m_mine = true;

}
void CalendarRemoveCmd::unexecute()
{
    m_project->addCalendar(m_cal, m_parent, m_index);
    m_cmd->unexecute();
    m_mine = false;

}

CalendarMoveCmd::CalendarMoveCmd(Project *project, Calendar *cal, int position, Calendar *parent, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_project(project),
        m_cal(cal),
        m_newpos(position),
        m_newparent(parent),
        m_oldparent(cal->parentCal())
{
    //debugPlan<<cal->name();
    Q_ASSERT(project != nullptr);

    m_oldpos = m_oldparent ? m_oldparent->indexOf(cal) : project->indexOf(cal);
}
void CalendarMoveCmd::execute()
{
    m_project->takeCalendar(m_cal);
    m_project->addCalendar(m_cal, m_newparent, m_newpos);
}

void CalendarMoveCmd::unexecute()
{
    m_project->takeCalendar(m_cal);
    m_project->addCalendar(m_cal, m_oldparent, m_oldpos);
}

CalendarModifyNameCmd::CalendarModifyNameCmd(Calendar *cal, const QString& newvalue, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_cal(cal)
{

    m_oldvalue = cal->name();
    m_newvalue = newvalue;
    //debugPlan<<cal->name();
}
void CalendarModifyNameCmd::execute()
{
    m_cal->setName(m_newvalue);

    //debugPlan<<m_cal->name();
}
void CalendarModifyNameCmd::unexecute()
{
    m_cal->setName(m_oldvalue);

    //debugPlan<<m_cal->name();
}

CalendarModifyParentCmd::CalendarModifyParentCmd(Project *project, Calendar *cal, Calendar *newvalue, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_project(project),
        m_cal(cal),
        m_cmd(new MacroCommand(KUndo2MagicString())),
        m_oldindex(-1),
        m_newindex(-1)
{
    m_oldvalue = cal->parentCal();
    m_newvalue = newvalue;
    m_oldindex = m_oldvalue ? m_oldvalue->indexOf(cal) : m_project->indexOf(cal);

    if (newvalue) {
        m_cmd->addCommand(new CalendarModifyTimeZoneCmd(cal, newvalue->timeZone()));
    }
    //debugPlan<<cal->name();
}
CalendarModifyParentCmd::~CalendarModifyParentCmd()
{
    delete m_cmd;
}
void CalendarModifyParentCmd::execute()
{
    m_project->takeCalendar(m_cal);
    m_project->addCalendar(m_cal, m_newvalue, m_newindex);
    m_cmd->execute();
}
void CalendarModifyParentCmd::unexecute()
{
    m_cmd->unexecute();
    m_project->takeCalendar(m_cal);
    m_project->addCalendar(m_cal, m_oldvalue, m_oldindex);
}

CalendarModifyTimeZoneCmd::CalendarModifyTimeZoneCmd(Calendar *cal, const QTimeZone &value, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_cal(cal),
        m_newvalue(value),
        m_cmd(new MacroCommand(KUndo2MagicString()))
{
    m_oldvalue = cal->timeZone();
    const auto calendars = cal->calendars();
    for (Calendar *c : calendars) {
        m_cmd->addCommand(new CalendarModifyTimeZoneCmd(c, value));
    }
    //debugPlan<<cal->name();
}
CalendarModifyTimeZoneCmd::~CalendarModifyTimeZoneCmd()
{
    delete m_cmd;
}
void CalendarModifyTimeZoneCmd::execute()
{
    m_cmd->execute();
    m_cal->setTimeZone(m_newvalue);
}
void CalendarModifyTimeZoneCmd::unexecute()
{
    m_cal->setTimeZone(m_oldvalue);
    m_cmd->unexecute();
}

#ifdef HAVE_KHOLIDAYS
CalendarModifyHolidayRegionCmd::CalendarModifyHolidayRegionCmd(Calendar *cal, const QString &value, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_cal(cal),
    m_newvalue(value)
{
    m_oldvalue = cal->holidayRegionCode();
}
CalendarModifyHolidayRegionCmd::~CalendarModifyHolidayRegionCmd()
{
}
void CalendarModifyHolidayRegionCmd::execute()
{
    m_cal->setHolidayRegion(m_newvalue);
}
void CalendarModifyHolidayRegionCmd::unexecute()
{
    m_cal->setHolidayRegion(m_oldvalue);
}
#endif

CalendarAddDayCmd::CalendarAddDayCmd(Calendar *cal, CalendarDay *newvalue, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_cal(cal),
        m_mine(true)
{

    m_newvalue = newvalue;
    //debugPlan<<cal->name();
}
CalendarAddDayCmd::~CalendarAddDayCmd()
{
    //debugPlan;
    if (m_mine)
        delete m_newvalue;
}
void CalendarAddDayCmd::execute()
{
    //debugPlan<<m_cal->name();
    m_cal->addDay(m_newvalue);
    m_mine = false;
}
void CalendarAddDayCmd::unexecute()
{
    //debugPlan<<m_cal->name();
    m_cal->takeDay(m_newvalue);
    m_mine = true;
}

CalendarRemoveDayCmd::CalendarRemoveDayCmd(Calendar *cal,CalendarDay *day, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_cal(cal),
        m_value(day),
        m_mine(false)
{
    //debugPlan<<cal->name();
    // TODO check if any resources uses this calendar
    init();
}
CalendarRemoveDayCmd::CalendarRemoveDayCmd(Calendar *cal, const QDate &day, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_cal(cal),
        m_mine(false)
{

    m_value = cal->findDay(day);
    //debugPlan<<cal->name();
    // TODO check if any resources uses this calendar
    init();
}
void CalendarRemoveDayCmd::init()
{
}
void CalendarRemoveDayCmd::execute()
{
    //debugPlan<<m_cal->name();
    m_cal->takeDay(m_value);
    m_mine = true;
}
void CalendarRemoveDayCmd::unexecute()
{
    //debugPlan<<m_cal->name();
    m_cal->addDay(m_value);
    m_mine = false;
}

CalendarModifyDayCmd::CalendarModifyDayCmd(Calendar *cal, CalendarDay *value, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_cal(cal),
        m_mine(true)
{

    m_newvalue = value;
    m_oldvalue = cal->findDay(value->date());
    //debugPlan<<cal->name()<<" old:("<<m_oldvalue<<") new:("<<m_newvalue<<")";
}
CalendarModifyDayCmd::~CalendarModifyDayCmd()
{
    //debugPlan;
    if (m_mine) {
        delete m_newvalue;
    } else {
        delete m_oldvalue;
    }
}
void CalendarModifyDayCmd::execute()
{
    //debugPlan;
    if (m_oldvalue) {
        m_cal->takeDay(m_oldvalue);
    }
    m_cal->addDay(m_newvalue);
    m_mine = false;
}
void CalendarModifyDayCmd::unexecute()
{
    //debugPlan;
    m_cal->takeDay(m_newvalue);
    if (m_oldvalue) {
        m_cal->addDay(m_oldvalue);
    }
    m_mine = true;
}

CalendarModifyStateCmd::CalendarModifyStateCmd(Calendar *calendar, CalendarDay *day, CalendarDay::State value, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_calendar(calendar),
        m_day(day),
        m_cmd(new MacroCommand(KUndo2MagicString()))
{

    m_newvalue = value;
    m_oldvalue = (CalendarDay::State)day->state();
    if (value != CalendarDay::Working) {
        const auto intervals = day->timeIntervals();
        for (TimeInterval *ti : intervals) {
            m_cmd->addCommand(new CalendarRemoveTimeIntervalCmd(calendar, day, ti));
        }
    }
}
CalendarModifyStateCmd::~CalendarModifyStateCmd()
{
    delete m_cmd;
}
void CalendarModifyStateCmd::execute()
{
    //debugPlan;
    m_cmd->execute();
    m_calendar->setState(m_day, m_newvalue);

}
void CalendarModifyStateCmd::unexecute()
{
    //debugPlan;
    m_calendar->setState(m_day, m_oldvalue);
    m_cmd->unexecute();

}

CalendarModifyTimeIntervalCmd::CalendarModifyTimeIntervalCmd(Calendar *calendar, TimeInterval &newvalue, TimeInterval *value, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_calendar(calendar)
{

    m_value = value; // keep pointer
    m_oldvalue = *value; // save value
    m_newvalue = newvalue;
}
void CalendarModifyTimeIntervalCmd::execute()
{
    //debugPlan;
    m_calendar->setWorkInterval(m_value, m_newvalue);

}
void CalendarModifyTimeIntervalCmd::unexecute()
{
    //debugPlan;
    m_calendar->setWorkInterval(m_value, m_oldvalue);

}

CalendarAddTimeIntervalCmd::CalendarAddTimeIntervalCmd(Calendar *calendar, CalendarDay *day, TimeInterval *value, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_calendar(calendar),
    m_day(day),
    m_value(value),
    m_mine(true)
{
}
CalendarAddTimeIntervalCmd::~CalendarAddTimeIntervalCmd()
{
    if (m_mine)
        delete m_value;
}
void CalendarAddTimeIntervalCmd::execute()
{
    //debugPlan;
    m_calendar->addWorkInterval(m_day, m_value);
    m_mine = false;

}
void CalendarAddTimeIntervalCmd::unexecute()
{
    //debugPlan;
    m_calendar->takeWorkInterval(m_day, m_value);
    m_mine = true;

}

CalendarRemoveTimeIntervalCmd::CalendarRemoveTimeIntervalCmd(Calendar *calendar, CalendarDay *day, TimeInterval *value, const KUndo2MagicString& name)
    : CalendarAddTimeIntervalCmd(calendar, day, value, name)
{
    m_mine = false ;
}
void CalendarRemoveTimeIntervalCmd::execute()
{
    CalendarAddTimeIntervalCmd::unexecute();
}
void CalendarRemoveTimeIntervalCmd::unexecute()
{
    CalendarAddTimeIntervalCmd::execute();
}

CalendarModifyWeekdayCmd::CalendarModifyWeekdayCmd(Calendar *cal, int weekday, CalendarDay *value, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_weekday(weekday),
        m_cal(cal),
        m_value(value),
        m_orig(*(cal->weekday(weekday)))
{

    //debugPlan << cal->name() <<" (" << value <<")";
}
CalendarModifyWeekdayCmd::~CalendarModifyWeekdayCmd()
{
    //debugPlan << m_weekday <<":" << m_value;
    delete m_value;

}
void CalendarModifyWeekdayCmd::execute()
{
    m_cal->setWeekday(m_weekday, *m_value);
}
void CalendarModifyWeekdayCmd::unexecute()
{
    m_cal->setWeekday(m_weekday, m_orig);
}

CalendarModifyDateCmd::CalendarModifyDateCmd(Calendar *cal, CalendarDay *day, const QDate &value, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_cal(cal),
    m_day(day),
    m_newvalue(value),
    m_oldvalue(day->date())
{
    //debugPlan << cal->name() <<" (" << value <<")";
}
void CalendarModifyDateCmd::execute()
{
    m_cal->setDate(m_day, m_newvalue);
}
void CalendarModifyDateCmd::unexecute()
{
    m_cal->setDate(m_day, m_oldvalue);
}

ProjectModifyDefaultCalendarCmd::ProjectModifyDefaultCalendarCmd(Project *project, Calendar *cal, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_project(project),
    m_newvalue(cal),
    m_oldvalue(project->defaultCalendar())
{
    //debugPlan << cal->name() <<" (" << value <<")";
}
void ProjectModifyDefaultCalendarCmd::execute()
{
    m_project->setDefaultCalendar(m_newvalue);

}
void ProjectModifyDefaultCalendarCmd::unexecute()
{
    m_project->setDefaultCalendar(m_oldvalue);

}

NodeDeleteCmd::NodeDeleteCmd(Node *node, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_node(node),
        m_index(-1),
        m_relCmd(nullptr)
{

    m_parent = node->parentNode();
    m_mine = false;

    m_project = static_cast<Project*>(node->projectNode());
    if (m_project) {
        const auto schedules = m_project->schedules();
        for (Schedule * s : schedules) {
            if (s && s->isScheduled()) {
                // Only invalidate schedules this node is part of
                Schedule *ns = node->findSchedule(s->id());
                if (ns && ! ns->isDeleted()) {
                    addSchScheduled(s);
                }
            }
        }
    }
    m_cmd = new MacroCommand(KUndo2MagicString());
    QList<Node*> lst = node->childNodeIterator();
    for (int i = lst.count(); i > 0; --i) {
        m_cmd->addCommand(new NodeDeleteCmd(lst[ i - 1 ]));
    }
    if (node->runningAccount()) {
        m_cmd->addCommand(new NodeModifyRunningAccountCmd(*node, node->runningAccount(), nullptr));
    }
    if (node->startupAccount()) {
        m_cmd->addCommand(new NodeModifyRunningAccountCmd(*node, node->startupAccount(), nullptr));
    }
    if (node->shutdownAccount()) {
        m_cmd->addCommand(new NodeModifyRunningAccountCmd(*node, node->shutdownAccount(), nullptr));
    }

}
NodeDeleteCmd::~NodeDeleteCmd()
{
    delete m_relCmd; // before node
    if (m_mine) {
        delete m_node;
    }
    delete m_cmd;
    while (!m_appointments.isEmpty())
        delete m_appointments.takeFirst();
}
void NodeDeleteCmd::execute()
{
    if (m_parent && m_project) {
        m_index = m_parent->findChildNode(m_node);
        //debugPlan<<m_node->name()<<""<<m_index;
        if (!m_relCmd) {
            m_relCmd = new MacroCommand();
            // Only add delete relation commands if we (still) have relations
            // The other node might have deleted them...
            const auto relations = m_node->dependChildNodes();
            for (Relation * r : relations) {
                m_relCmd->addCommand(new DeleteRelationCmd(*m_project, r));
            }
            const auto relations2 = m_node->dependParentNodes();
            for (Relation * r : relations2) {
                m_relCmd->addCommand(new DeleteRelationCmd(*m_project, r));
            }
        }
        m_relCmd->execute();
        if (m_cmd) {
            m_cmd->execute();
        }
        m_project->takeTask(m_node);
        m_mine = true;
        setSchScheduled(false);
    }
}
void NodeDeleteCmd::unexecute()
{
    if (m_parent && m_project) {
        //debugPlan<<m_node->name()<<""<<m_index;
        m_project->addSubTask(m_node, m_index, m_parent);
        if (m_cmd) {
            m_cmd->unexecute();
        }
        m_relCmd->unexecute();
        m_mine = false;
        setSchScheduled();
    }
}

TaskAddCmd::TaskAddCmd(Project *project, Node *node, Node *after, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_project(project),
        m_node(node),
        m_after(after),
        m_added(false)
{

    // set some reasonable defaults for normally calculated values
    if (after && after->parentNode() && after->parentNode() != project) {
        node->setStartTime(after->parentNode() ->startTime());
        node->setEndTime(node->startTime() + node->duration());
    } else {
        if (project->constraint() == Node::MustFinishOn) {
            node->setEndTime(project->endTime());
            node->setStartTime(node->endTime() - node->duration());
        } else {
            node->setStartTime(project->startTime());
            node->setEndTime(node->startTime() + node->duration());
        }
    }
    node->setEarlyStart(node->startTime());
    node->setLateFinish(node->endTime());
    node->setWorkStartTime(node->startTime());
    node->setWorkEndTime(node->endTime());
}
TaskAddCmd::~TaskAddCmd()
{
    if (!m_added)
        delete m_node;
}
void TaskAddCmd::execute()
{
    //debugPlan<<m_node->name();
    m_project->addTask(m_node, m_after);
    m_added = true;


}
void TaskAddCmd::unexecute()
{
    m_project->takeTask(m_node);
    m_added = false;


}

SubtaskAddCmd::SubtaskAddCmd(Project *project, Node *node, Node *parent, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_project(project),
        m_node(node),
        m_parent(parent),
        m_added(false),
        m_cmd(nullptr)
{

    // set some reasonable defaults for normally calculated values
    node->setStartTime(parent->startTime());
    node->setEndTime(node->startTime() + node->duration());
    node->setEarlyStart(node->startTime());
    node->setLateFinish(node->endTime());
    node->setWorkStartTime(node->startTime());
    node->setWorkEndTime(node->endTime());

    // Summarytasks can't have resources, so remove resource requests from the new parent
    const auto requests = parent->requests().resourceRequests();
    for (ResourceRequest *r : requests) {
        if (m_cmd == nullptr) m_cmd = new MacroCommand(KUndo2MagicString());
        m_cmd->addCommand(new RemoveResourceRequestCmd(r));
    }
    // Also remove accounts
    if (parent->runningAccount()) {
        if (m_cmd == nullptr) m_cmd = new MacroCommand(KUndo2MagicString());
        m_cmd->addCommand(new NodeModifyRunningAccountCmd(*parent, parent->runningAccount(), nullptr));
    }
    if (parent->startupAccount()) {
        if (m_cmd == nullptr) m_cmd = new MacroCommand(KUndo2MagicString());
        m_cmd->addCommand(new NodeModifyStartupAccountCmd(*parent, parent->startupAccount(), nullptr));
    }
    if (parent->shutdownAccount()) {
        if (m_cmd == nullptr) m_cmd = new MacroCommand(KUndo2MagicString());
        m_cmd->addCommand(new NodeModifyShutdownAccountCmd(*parent, parent->shutdownAccount(), nullptr));
    }
}
SubtaskAddCmd::~SubtaskAddCmd()
{
    delete m_cmd;
    if (!m_added)
        delete m_node;
}
void SubtaskAddCmd::execute()
{
    m_project->addSubTask(m_node, m_parent);
    if (m_cmd) {
        m_cmd->execute();
    }
    m_added = true;


}
void SubtaskAddCmd::unexecute()
{
    m_project->takeTask(m_node);
    if (m_cmd) {
        m_cmd->unexecute();
    }
    m_added = false;


}

NodeModifyNameCmd::NodeModifyNameCmd(Node &node, const QString& nodename, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_node(node),
        newName(nodename),
        oldName(node.name())
{
}
void NodeModifyNameCmd::execute()
{
    m_node.setName(newName);


}
void NodeModifyNameCmd::unexecute()
{
    m_node.setName(oldName);


}

NodeModifyPriorityCmd::NodeModifyPriorityCmd(Node &node, int oldValue, int newValue, const KUndo2MagicString& name)
    : NamedCommand(name)
    , m_node(node)
    , m_oldValue(oldValue)
    , m_newValue(newValue)
{
}
void NodeModifyPriorityCmd::execute()
{
    m_node.setPriority(m_newValue);
}
void NodeModifyPriorityCmd::unexecute()
{
    m_node.setPriority(m_oldValue);
}

NodeModifyLeaderCmd::NodeModifyLeaderCmd(Node &node, const QString& leader, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_node(node),
        newLeader(leader),
        oldLeader(node.leader())
{
}
void NodeModifyLeaderCmd::execute()
{
    m_node.setLeader(newLeader);


}
void NodeModifyLeaderCmd::unexecute()
{
    m_node.setLeader(oldLeader);


}

NodeModifyDescriptionCmd::NodeModifyDescriptionCmd(Node &node, const QString& description, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_node(node),
        newDescription(description),
        oldDescription(node.description())
{
}
void NodeModifyDescriptionCmd::execute()
{
    m_node.setDescription(newDescription);


}
void NodeModifyDescriptionCmd::unexecute()
{
    m_node.setDescription(oldDescription);


}

NodeModifyConstraintCmd::NodeModifyConstraintCmd(Node &node, Node::ConstraintType c, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_node(node),
        newConstraint(c),
        oldConstraint(static_cast<Node::ConstraintType>(node.constraint()))
{
}
void NodeModifyConstraintCmd::execute()
{
    m_node.setConstraint(newConstraint);
}
void NodeModifyConstraintCmd::unexecute()
{
    m_node.setConstraint(oldConstraint);
}

NodeModifyConstraintStartTimeCmd::NodeModifyConstraintStartTimeCmd(Node &node, const QDateTime& dt, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_node(node),
        newTime(dt),
        oldTime(node.constraintStartTime())
{
    if (node.projectNode()) {
        m_timeZone = static_cast<Project*>(node.projectNode())->timeZone();
    }
}
void NodeModifyConstraintStartTimeCmd::execute()
{
    m_node.setConstraintStartTime(DateTime(newTime, m_timeZone));

}
void NodeModifyConstraintStartTimeCmd::unexecute()
{
    m_node.setConstraintStartTime(oldTime);

}

NodeModifyConstraintEndTimeCmd::NodeModifyConstraintEndTimeCmd(Node &node, const QDateTime& dt, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_node(node),
        newTime(dt),
        oldTime(node.constraintEndTime())
{
    if (node.projectNode()) {
        m_timeZone = static_cast<Project*>(node.projectNode())->timeZone();
    }
}
void NodeModifyConstraintEndTimeCmd::execute()
{
    m_node.setConstraintEndTime(DateTime(newTime, m_timeZone));
}
void NodeModifyConstraintEndTimeCmd::unexecute()
{
    m_node.setConstraintEndTime(oldTime);
}

NodeModifyStartTimeCmd::NodeModifyStartTimeCmd(Node &node, const QDateTime& dt, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_node(node),
        newTime(dt),
        oldTime(node.startTime())
{
    m_timeZone = static_cast<Project*>(node.projectNode())->timeZone();
}
void NodeModifyStartTimeCmd::execute()
{
    m_node.setStartTime(DateTime(newTime, m_timeZone));


}
void NodeModifyStartTimeCmd::unexecute()
{
    m_node.setStartTime(oldTime);


}

NodeModifyEndTimeCmd::NodeModifyEndTimeCmd(Node &node, const QDateTime& dt, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_node(node),
        newTime(dt),
        oldTime(node.endTime())
{
    m_timeZone = static_cast<Project*>(node.projectNode())->timeZone();
}
void NodeModifyEndTimeCmd::execute()
{
    m_node.setEndTime(DateTime(newTime, m_timeZone));


}
void NodeModifyEndTimeCmd::unexecute()
{
    m_node.setEndTime(oldTime);


}

NodeModifyIdCmd::NodeModifyIdCmd(Node &node, const QString& id, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_node(node),
        newId(id),
        oldId(node.id())
{
}
void NodeModifyIdCmd::execute()
{
    m_node.setId(newId);


}
void NodeModifyIdCmd::unexecute()
{
    m_node.setId(oldId);


}

NodeIndentCmd::NodeIndentCmd(Node &node, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_node(node),
        m_newparent(nullptr),
        m_newindex(-1),
        m_cmd(nullptr)
{
}
NodeIndentCmd::~NodeIndentCmd()
{
    delete m_cmd;
}
void NodeIndentCmd::execute()
{
    m_oldparent = m_node.parentNode();
    m_oldindex = m_oldparent->findChildNode(&m_node);
    Project *p = dynamic_cast<Project *>(m_node.projectNode());
    if (p && p->indentTask(&m_node, m_newindex)) {
        m_newparent = m_node.parentNode();
        m_newindex = m_newparent->findChildNode(&m_node);
        // Summarytasks can't have resources, so remove resource requests from the new parent
        if (m_cmd == nullptr) {
            const auto requests = m_newparent->requests().resourceRequests();
            for (ResourceRequest *r : requests) {
                if (m_cmd == nullptr) m_cmd = new MacroCommand(KUndo2MagicString());
                m_cmd->addCommand(new RemoveResourceRequestCmd(r));
            }
            // Also remove accounts
            if (m_newparent->runningAccount()) {
                if (m_cmd == nullptr) m_cmd = new MacroCommand(KUndo2MagicString());
                m_cmd->addCommand(new NodeModifyRunningAccountCmd(*m_newparent, m_newparent->runningAccount(), nullptr));
            }
            if (m_newparent->startupAccount()) {
                if (m_cmd == nullptr) m_cmd = new MacroCommand(KUndo2MagicString());
                m_cmd->addCommand(new NodeModifyStartupAccountCmd(*m_newparent, m_newparent->startupAccount(), nullptr));
            }
            if (m_newparent->shutdownAccount()) {
                if (m_cmd == nullptr) m_cmd = new MacroCommand(KUndo2MagicString());
                m_cmd->addCommand(new NodeModifyShutdownAccountCmd(*m_newparent, m_newparent->shutdownAccount(), nullptr));
            }
       }
        if (m_cmd) {
            m_cmd->execute();
        }
    }
}
void NodeIndentCmd::unexecute()
{
    Project * p = dynamic_cast<Project *>(m_node.projectNode());
    if (m_newindex != -1 && p && p->unindentTask(&m_node)) {
        m_newindex = -1;
        if (m_cmd) {
            m_cmd->unexecute();
        }
    }


}

NodeUnindentCmd::NodeUnindentCmd(Node &node, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_node(node),
        m_newparent(nullptr),
        m_newindex(-1)
{}
void NodeUnindentCmd::execute()
{
    m_oldparent = m_node.parentNode();
    m_oldindex = m_oldparent->findChildNode(&m_node);
    Project *p = dynamic_cast<Project *>(m_node.projectNode());
    if (p && p->unindentTask(&m_node)) {
        m_newparent = m_node.parentNode();
        m_newindex = m_newparent->findChildNode(&m_node);
    }


}
void NodeUnindentCmd::unexecute()
{
    Project * p = dynamic_cast<Project *>(m_node.projectNode());
    if (m_newindex != -1 && p && p->indentTask(&m_node, m_oldindex)) {
        m_newindex = -1;
    }


}

NodeMoveUpCmd::NodeMoveUpCmd(Node &node, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_node(node),
        m_moved(false)
{

    m_project = static_cast<Project *>(m_node.projectNode());
}
void NodeMoveUpCmd::execute()
{
    if (m_project) {
        m_moved = m_project->moveTaskUp(&m_node);
    }


}
void NodeMoveUpCmd::unexecute()
{
    if (m_project && m_moved) {
        m_project->moveTaskDown(&m_node);
    }
    m_moved = false;

}

NodeMoveDownCmd::NodeMoveDownCmd(Node &node, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_node(node),
        m_moved(false)
{

    m_project = static_cast<Project *>(m_node.projectNode());
}
void NodeMoveDownCmd::execute()
{
    if (m_project) {
        m_moved = m_project->moveTaskDown(&m_node);
    }

}
void NodeMoveDownCmd::unexecute()
{
    if (m_project && m_moved) {
        m_project->moveTaskUp(&m_node);
    }
    m_moved = false;

}

NodeMoveCmd::NodeMoveCmd(Project *project, Node *node, Node *newParent, int newPos, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_project(project),
    m_node(node),
    m_newparent(newParent),
    m_newpos(newPos),
    m_moved(false)
{
    m_oldparent = node->parentNode();
    Q_ASSERT(m_oldparent);
}
void NodeMoveCmd::execute()
{
    if (m_project) {
        m_oldpos = m_oldparent->indexOf(m_node);
        m_moved = m_project->moveTask(m_node, m_newparent, m_newpos);
        if (m_moved) {
            if (m_cmd.isEmpty()) {
                // Summarytasks can't have resources, so remove resource requests from the new parent
                const auto requests = m_newparent->requests().resourceRequests();
                for (ResourceRequest *r : requests) {
                    m_cmd.addCommand(new RemoveResourceRequestCmd(r));
                }
                // TODO appointments ??
            }
            m_cmd.execute();
        }
    }
}
void NodeMoveCmd::unexecute()
{
    if (m_project && m_moved) {
        m_moved = m_project->moveTask(m_node, m_oldparent, m_oldpos);
        m_cmd.unexecute();
    }
    m_moved = false;
}

AddRelationCmd::AddRelationCmd(Project &project, Relation *rel, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_rel(rel),
        m_project(project)
{
    m_taken = true;
}
AddRelationCmd::~AddRelationCmd()
{
    if (m_taken)
        delete m_rel;
}
void AddRelationCmd::execute()
{
    //debugPlan<<m_rel->parent()<<" to"<<m_rel->child();
    m_taken = false;
    m_project.addRelation(m_rel, false);
}
void AddRelationCmd::unexecute()
{
    m_taken = true;
    m_project.takeRelation(m_rel);
}

DeleteRelationCmd::DeleteRelationCmd(Project &project, Relation *rel, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_rel(rel),
        m_project(project)
{
    m_taken = false;
}
DeleteRelationCmd::~DeleteRelationCmd()
{
    if (m_taken) {
        // do not access nodes, the may already be deleted
        m_rel->setParent(nullptr);
        m_rel->setChild(nullptr);
        delete m_rel;
    }
}
void DeleteRelationCmd::execute()
{
    //debugPlan<<m_rel->parent()<<" to"<<m_rel->child();
    m_taken = true;
    m_project.takeRelation(m_rel);
}
void DeleteRelationCmd::unexecute()
{
    m_taken = false;
    m_project.addRelation(m_rel, false);
}

ModifyRelationTypeCmd::ModifyRelationTypeCmd(Relation *rel, Relation::Type type, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_rel(rel),
        m_newtype(type)
{

    m_oldtype = rel->type();
    m_project = dynamic_cast<Project*>(rel->parent() ->projectNode());
}
void ModifyRelationTypeCmd::execute()
{
    if (m_project) {
        m_project->setRelationType(m_rel, m_newtype);
    }
}
void ModifyRelationTypeCmd::unexecute()
{
    if (m_project) {
        m_project->setRelationType(m_rel, m_oldtype);
    }
}

ModifyRelationLagCmd::ModifyRelationLagCmd(Relation *rel, Duration lag, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_rel(rel),
        m_newlag(lag)
{

    m_oldlag = rel->lag();
    m_project = dynamic_cast<Project*>(rel->parent() ->projectNode());
}
void ModifyRelationLagCmd::execute()
{
    if (m_project) {
        m_project->setRelationLag(m_rel, m_newlag);
    }
}
void ModifyRelationLagCmd::unexecute()
{
    if (m_project) {
        m_project->setRelationLag(m_rel, m_oldlag);
    }
}

AddResourceRequestCmd::AddResourceRequestCmd(ResourceRequestCollection *requests, ResourceRequest *request, const KUndo2MagicString& name)
    : NamedCommand(name)
    , m_collection(requests)
    , m_request(request)
{
    m_mine = true;
}
AddResourceRequestCmd::~AddResourceRequestCmd()
{
    if (m_mine)
        delete m_request;
}
void AddResourceRequestCmd::execute()
{
    //debugPlan<<"group="<<m_group<<" req="<<m_request;
    m_collection->addResourceRequest(m_request);
    m_mine = false;
}
void AddResourceRequestCmd::unexecute()
{
    //debugPlan<<"group="<<m_group<<" req="<<m_request;
    m_collection->removeResourceRequest(m_request);
    m_mine = true;
}

RemoveResourceRequestCmd::RemoveResourceRequestCmd(ResourceRequest *request, const KUndo2MagicString& name)
    : NamedCommand(name)
    , m_collection(request->collection())
    , m_request(request)
{
    Q_ASSERT(m_collection);
    m_mine = false;
    //debugPlan<<"group req="<<group<<" req="<<request<<" to gr="<<m_group->group();
}
RemoveResourceRequestCmd::~RemoveResourceRequestCmd()
{
    if (m_mine)
        delete m_request;
}
void RemoveResourceRequestCmd::execute()
{
    m_collection->removeResourceRequest(m_request);
    m_mine = true;
}
void RemoveResourceRequestCmd::unexecute()
{
    m_collection->addResourceRequest(m_request);
    m_mine = false;
}

ModifyResourceRequestUnitsCmd::ModifyResourceRequestUnitsCmd(ResourceRequest *request, int oldvalue, int newvalue, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_request(request),
    m_oldvalue(oldvalue),
    m_newvalue(newvalue)
{
}
void ModifyResourceRequestUnitsCmd::execute()
{
    m_request->setUnits(m_newvalue);
}
void ModifyResourceRequestUnitsCmd::unexecute()
{
    m_request->setUnits(m_oldvalue);
}

ModifyResourceRequestRequiredCmd::ModifyResourceRequestRequiredCmd(ResourceRequest *request, const QList<Resource*> &value, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_request(request),
    m_newvalue(value)
{
    m_oldvalue = request->requiredResources();
}
void ModifyResourceRequestRequiredCmd::execute()
{
    m_request->setRequiredResources(m_newvalue);
}
void ModifyResourceRequestRequiredCmd::unexecute()
{
    m_request->setRequiredResources(m_oldvalue);
}

ModifyEstimateCmd::ModifyEstimateCmd(Node &node, double oldvalue, double newvalue, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_estimate(node.estimate()),
    m_oldvalue(oldvalue),
    m_newvalue(newvalue),
    m_optimistic(node.estimate()->optimisticRatio()),
    m_pessimistic(node.estimate()->pessimisticRatio()),
    m_cmd(nullptr)
{
    if (newvalue == 0.0) {
        // Milestones can't have resources, so remove resource requests
        const auto resourceRequests = node.requests().resourceRequests();
        for (ResourceRequest *r : resourceRequests) {
            if (m_cmd == nullptr) m_cmd = new MacroCommand(KUndo2MagicString());
            m_cmd->addCommand(new RemoveResourceRequestCmd(r));
        }
    }
}
ModifyEstimateCmd::~ModifyEstimateCmd()
{
    delete m_cmd;
}
void ModifyEstimateCmd::execute()
{
    m_estimate->setExpectedEstimate(m_newvalue);
    if (m_cmd) {
        m_cmd->execute();
    }
    m_estimate->setPessimisticRatio(m_pessimistic);
    m_estimate->setOptimisticRatio(m_optimistic);
}
void ModifyEstimateCmd::unexecute()
{
    m_estimate->setExpectedEstimate(m_oldvalue);
    if (m_cmd) {
        m_cmd->unexecute();
    }
    m_estimate->setPessimisticRatio(m_pessimistic);
    m_estimate->setOptimisticRatio(m_optimistic);
}

EstimateModifyOptimisticRatioCmd::EstimateModifyOptimisticRatioCmd(Node &node, int oldvalue, int newvalue, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_estimate(node.estimate()),
        m_oldvalue(oldvalue),
        m_newvalue(newvalue)
{
}
void EstimateModifyOptimisticRatioCmd::execute()
{
    m_estimate->setOptimisticRatio(m_newvalue);
}
void EstimateModifyOptimisticRatioCmd::unexecute()
{
    m_estimate->setOptimisticRatio(m_oldvalue);
}

EstimateModifyPessimisticRatioCmd::EstimateModifyPessimisticRatioCmd(Node &node, int oldvalue, int newvalue, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_estimate(node.estimate()),
        m_oldvalue(oldvalue),
        m_newvalue(newvalue)
{
}
void EstimateModifyPessimisticRatioCmd::execute()
{
    m_estimate->setPessimisticRatio(m_newvalue);
}
void EstimateModifyPessimisticRatioCmd::unexecute()
{
    m_estimate->setPessimisticRatio(m_oldvalue);
}

ModifyEstimateTypeCmd::ModifyEstimateTypeCmd(Node &node, int oldvalue, int newvalue, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_estimate(node.estimate()),
        m_oldvalue(oldvalue),
        m_newvalue(newvalue)
{
}
void ModifyEstimateTypeCmd::execute()
{
    m_estimate->setType(static_cast<Estimate::Type>(m_newvalue));
}
void ModifyEstimateTypeCmd::unexecute()
{
    m_estimate->setType(static_cast<Estimate::Type>(m_oldvalue));
}

ModifyEstimateCalendarCmd::ModifyEstimateCalendarCmd(Node &node, Calendar *oldvalue, Calendar *newvalue, const KUndo2MagicString& name)
    : NamedCommand(name),
        m_estimate(node.estimate()),
        m_oldvalue(oldvalue),
        m_newvalue(newvalue)
{
}
void ModifyEstimateCalendarCmd::execute()
{
    m_estimate->setCalendar(m_newvalue);
}
void ModifyEstimateCalendarCmd::unexecute()
{
    m_estimate->setCalendar(m_oldvalue);
}

ModifyEstimateUnitCmd::ModifyEstimateUnitCmd(Node &node, Duration::Unit oldvalue, Duration::Unit newvalue, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_estimate(node.estimate()),
        m_oldvalue(oldvalue),
        m_newvalue(newvalue)
{
}
void ModifyEstimateUnitCmd::execute()
{
    m_estimate->setUnit(m_newvalue);

}
void ModifyEstimateUnitCmd::unexecute()
{
    m_estimate->setUnit(m_oldvalue);
}

EstimateModifyRiskCmd::EstimateModifyRiskCmd(Node &node, int oldvalue, int newvalue, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_estimate(node.estimate()),
        m_oldvalue(oldvalue),
        m_newvalue(newvalue)
{
}
void EstimateModifyRiskCmd::execute()
{
    m_estimate->setRisktype(static_cast<Estimate::Risktype>(m_newvalue));
}
void EstimateModifyRiskCmd::unexecute()
{
    m_estimate->setRisktype(static_cast<Estimate::Risktype>(m_oldvalue));
}

MoveResourceCmd::MoveResourceCmd(ResourceGroup *group, Resource *resource, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_project(*(group->project())),
    m_resource(resource),
    m_oldvalue(resource->parentGroups().value(0)),
    m_newvalue(group)
{
    for (ResourceRequest *r : resource->requests()) {
        cmd.addCommand(new RemoveResourceRequestCmd(r));
    }
}
void MoveResourceCmd::execute()
{
    cmd.execute();
    m_project.moveResource(m_newvalue, m_resource);
}
void MoveResourceCmd::unexecute()
{
    m_project.moveResource(m_oldvalue, m_resource);
    cmd.unexecute();
}

ModifyResourceNameCmd::ModifyResourceNameCmd(Resource *resource, const QString& value, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_resource(resource),
        m_newvalue(value)
{
    m_oldvalue = resource->name();
}
void ModifyResourceNameCmd::execute()
{
    m_resource->setName(m_newvalue);


}
void ModifyResourceNameCmd::unexecute()
{
    m_resource->setName(m_oldvalue);
}
ModifyResourceInitialsCmd::ModifyResourceInitialsCmd(Resource *resource, const QString& value, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_resource(resource),
        m_newvalue(value)
{
    m_oldvalue = resource->initials();
}
void ModifyResourceInitialsCmd::execute()
{
    m_resource->setInitials(m_newvalue);


}
void ModifyResourceInitialsCmd::unexecute()
{
    m_resource->setInitials(m_oldvalue);


}
ModifyResourceEmailCmd::ModifyResourceEmailCmd(Resource *resource, const QString& value, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_resource(resource),
        m_newvalue(value)
{
    m_oldvalue = resource->email();
}
void ModifyResourceEmailCmd::execute()
{
    m_resource->setEmail(m_newvalue);
}
void ModifyResourceEmailCmd::unexecute()
{
    m_resource->setEmail(m_oldvalue);
}
ModifyResourceAutoAllocateCmd::ModifyResourceAutoAllocateCmd(Resource *resource,bool value, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_resource(resource),
    m_newvalue(value)
{
    m_oldvalue = resource->autoAllocate();
}
void ModifyResourceAutoAllocateCmd::execute()
{
    m_resource->setAutoAllocate(m_newvalue);
}
void ModifyResourceAutoAllocateCmd::unexecute()
{
    m_resource->setAutoAllocate(m_oldvalue);
}
ModifyResourceTypeCmd::ModifyResourceTypeCmd(Resource *resource, int value, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_resource(resource),
    m_newvalue(value)
{
    m_oldvalue = resource->type();
    if (m_oldvalue == Resource::Type_Team) {
        const auto teamMemberIds = resource->teamMemberIds();
        for (const QString &id : teamMemberIds) {
            m_cmd.addCommand(new RemoveResourceTeamCmd(resource, id));
        }
    }
}
void ModifyResourceTypeCmd::execute()
{
    m_cmd.redo();
    m_resource->setType((Resource::Type) m_newvalue);
}
void ModifyResourceTypeCmd::unexecute()
{
    m_resource->setType((Resource::Type) m_oldvalue);
    m_cmd.undo();
}
ModifyResourceUnitsCmd::ModifyResourceUnitsCmd(Resource *resource, int value, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_resource(resource),
        m_newvalue(value)
{
    m_oldvalue = resource->units();
}
void ModifyResourceUnitsCmd::execute()
{
    m_resource->setUnits(m_newvalue);
}
void ModifyResourceUnitsCmd::unexecute()
{
    m_resource->setUnits(m_oldvalue);
}

ModifyResourceAvailableFromCmd::ModifyResourceAvailableFromCmd(Resource *resource, const QDateTime& value, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_resource(resource),
        m_newvalue(value)
{
    m_oldvalue = resource->availableFrom();
    m_timeZone = resource->timeZone();
}
void ModifyResourceAvailableFromCmd::execute()
{
    m_resource->setAvailableFrom(DateTime(m_newvalue, m_timeZone));
}
void ModifyResourceAvailableFromCmd::unexecute()
{
    m_resource->setAvailableFrom(m_oldvalue);
}

ModifyResourceAvailableUntilCmd::ModifyResourceAvailableUntilCmd(Resource *resource, const QDateTime& value, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_resource(resource),
        m_newvalue(value)
{
    m_oldvalue = resource->availableUntil();
    m_timeZone = resource->timeZone();
}
void ModifyResourceAvailableUntilCmd::execute()
{
    m_resource->setAvailableUntil(DateTime(m_newvalue, m_timeZone));
}
void ModifyResourceAvailableUntilCmd::unexecute()
{
    m_resource->setAvailableUntil(m_oldvalue);
}

ModifyResourceNormalRateCmd::ModifyResourceNormalRateCmd(Resource *resource, double value, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_resource(resource),
        m_newvalue(value)
{
    m_oldvalue = resource->normalRate();
}
void ModifyResourceNormalRateCmd::execute()
{
    m_resource->setNormalRate(m_newvalue);


}
void ModifyResourceNormalRateCmd::unexecute()
{
    m_resource->setNormalRate(m_oldvalue);


}
ModifyResourceOvertimeRateCmd::ModifyResourceOvertimeRateCmd(Resource *resource, double value, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_resource(resource),
        m_newvalue(value)
{
    m_oldvalue = resource->overtimeRate();
}
void ModifyResourceOvertimeRateCmd::execute()
{
    m_resource->setOvertimeRate(m_newvalue);


}
void ModifyResourceOvertimeRateCmd::unexecute()
{
    m_resource->setOvertimeRate(m_oldvalue);


}

ModifyResourceCalendarCmd::ModifyResourceCalendarCmd(Resource *resource, Calendar *value, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_resource(resource),
        m_newvalue(value)
{
    m_oldvalue = resource->calendar(true);
}
void ModifyResourceCalendarCmd::execute()
{
    m_resource->setCalendar(m_newvalue);
}
void ModifyResourceCalendarCmd::unexecute()
{
    m_resource->setCalendar(m_oldvalue);
}

ModifyRequiredResourcesCmd::ModifyRequiredResourcesCmd(Resource *resource, const QStringList &value, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_resource(resource),
        m_newvalue(value)
{
    m_oldvalue = resource->requiredIds();
}
void ModifyRequiredResourcesCmd::execute()
{
    m_resource->setRequiredIds(m_newvalue);
}
void ModifyRequiredResourcesCmd::unexecute()
{
    m_resource->setRequiredIds(m_oldvalue);
}

AddResourceTeamCmd::AddResourceTeamCmd(Resource *team, const QString &member, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_team(team),
    m_member(member)
{
}
void AddResourceTeamCmd::execute()
{
    m_team->addTeamMemberId(m_member);
}
void AddResourceTeamCmd::unexecute()
{
    m_team->removeTeamMemberId(m_member);
}

RemoveResourceTeamCmd::RemoveResourceTeamCmd(Resource *team, const QString &member, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_team(team),
    m_member(member)
{
}
void RemoveResourceTeamCmd::execute()
{
    m_team->removeTeamMemberId(m_member);
}
void RemoveResourceTeamCmd::unexecute()
{
    m_team->addTeamMemberId(m_member);
}

RemoveResourceGroupCmd::RemoveResourceGroupCmd(Project *project, ResourceGroup *parent, ResourceGroup *group, const KUndo2MagicString& name)
    : NamedCommand(name)
    , m_group(group)
    , m_project(project)
    , m_parent(parent)
{
    m_index = m_parent ? m_parent->indexOf(group) : project->indexOf(group);
    m_mine = false;
    const auto resources = group->resources();
    for (Resource *r : resources) {
        cmd.addCommand(new RemoveParentGroupCmd(r, group));
    }
}
RemoveResourceGroupCmd::RemoveResourceGroupCmd(Project *project, ResourceGroup *group, const KUndo2MagicString& name)
    : NamedCommand(name)
    , m_group(group)
    , m_project(project)
    , m_parent(group->parentGroup())
{
    m_index = m_parent ? m_parent->indexOf(group) : project->indexOf(group);
    m_mine = false;
    const auto resources = group->resources();
    for (Resource *r : resources) {
        cmd.addCommand(new RemoveParentGroupCmd(r, group));
    }
}
RemoveResourceGroupCmd::~RemoveResourceGroupCmd()
{
    if (m_mine)
        delete m_group;
}
void RemoveResourceGroupCmd::execute()
{
    cmd.execute();
    if (m_project) {
        m_project->takeResourceGroup(m_group);
    }
    m_mine = true;
}
void RemoveResourceGroupCmd::unexecute()
{
    if (m_project) {
        m_project->addResourceGroup(m_group, m_parent, m_index);
    }
    cmd.unexecute();
    m_mine = false;
}

AddResourceGroupCmd::AddResourceGroupCmd(Project *project, ResourceGroup *parent, ResourceGroup *group, const KUndo2MagicString& name)
    : RemoveResourceGroupCmd(project, parent, group, name)
{
    m_mine = true;
}

AddResourceGroupCmd::AddResourceGroupCmd(Project *project, ResourceGroup *group, const KUndo2MagicString& name)
    : RemoveResourceGroupCmd(project, group, name)
{
    m_mine = true;
}
void AddResourceGroupCmd::execute()
{
    RemoveResourceGroupCmd::unexecute();
}
void AddResourceGroupCmd::unexecute()
{
    RemoveResourceGroupCmd::execute();
}

ModifyResourceGroupNameCmd::ModifyResourceGroupNameCmd(ResourceGroup *group, const QString& value, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_group(group),
        m_newvalue(value)
{
    m_oldvalue = group->name();
}
void ModifyResourceGroupNameCmd::execute()
{
    m_group->setName(m_newvalue);


}
void ModifyResourceGroupNameCmd::unexecute()
{
    m_group->setName(m_oldvalue);


}

ModifyResourceGroupTypeCmd::ModifyResourceGroupTypeCmd(ResourceGroup *group, const QString &value, const KUndo2MagicString& name)
    : NamedCommand(name),
        m_group(group),
        m_newvalue(value)
{
    m_oldvalue = group->type();
}
void ModifyResourceGroupTypeCmd::execute()
{
    m_group->setType(m_newvalue);


}
void ModifyResourceGroupTypeCmd::unexecute()
{
    m_group->setType(m_oldvalue);


}


ModifyCompletionEntrymodeCmd::ModifyCompletionEntrymodeCmd(Completion &completion, Completion::Entrymode value, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_completion(completion),
        oldvalue(m_completion.entrymode()),
        newvalue(value)
{
}
void ModifyCompletionEntrymodeCmd::execute()
{
    m_completion.setEntrymode(newvalue);
}
void ModifyCompletionEntrymodeCmd::unexecute()
{
    m_completion.setEntrymode(oldvalue);
}

ModifyCompletionPercentFinishedCmd::ModifyCompletionPercentFinishedCmd(Completion &completion, const QDate &date, int value, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_completion(completion),
    m_date(date),
    m_newvalue(value),
    m_oldvalue(completion.percentFinished(date))
{
    if (! completion.entries().contains(date)) {
        Completion::Entry *e = new Completion::Entry();
        Completion::Entry *latest = completion.entry(completion.entryDate());
        if (latest) {
            *e = *latest;
        }
        cmd.addCommand(new AddCompletionEntryCmd(completion, date, e));
    }

}
void ModifyCompletionPercentFinishedCmd::execute()
{
    cmd.execute();
    m_completion.setPercentFinished(m_date, m_newvalue);
}
void ModifyCompletionPercentFinishedCmd::unexecute()
{
    m_completion.setPercentFinished(m_date, m_oldvalue);
    cmd.unexecute();
}

ModifyCompletionRemainingEffortCmd::ModifyCompletionRemainingEffortCmd(Completion &completion, const QDate &date, const Duration &value, const KUndo2MagicString &name)
    : NamedCommand(name),
    m_completion(completion),
    m_date(date),
    m_newvalue(value),
    m_oldvalue(completion.remainingEffort(date))
{
    if (! completion.entries().contains(date)) {
        Completion::Entry *e = new Completion::Entry();
        Completion::Entry *latest = completion.entry(completion.entryDate());
        if (latest) {
            *e = *latest;
        }
        cmd.addCommand(new AddCompletionEntryCmd(completion, date, e));
    }

}
void ModifyCompletionRemainingEffortCmd::execute()
{
    cmd.execute();
    m_completion.setRemainingEffort(m_date, m_newvalue);
}
void ModifyCompletionRemainingEffortCmd::unexecute()
{
    m_completion.setRemainingEffort(m_date, m_oldvalue);
    cmd.unexecute();
}

ModifyCompletionActualEffortCmd::ModifyCompletionActualEffortCmd(Completion &completion, const QDate &date, const Duration &value, const KUndo2MagicString &name)
    : NamedCommand(name),
    m_completion(completion),
    m_date(date),
    m_newvalue(value),
    m_oldvalue(completion.actualEffort(date))
{
    if (! completion.entries().contains(date)) {
        Completion::Entry *e = new Completion::Entry();
        Completion::Entry *latest = completion.entry(completion.entryDate());
        if (latest) {
            *e = *latest;
        }
        cmd.addCommand(new AddCompletionEntryCmd(completion, date, e));
    }

}
void ModifyCompletionActualEffortCmd::execute()
{
    cmd.execute();
    m_completion.setActualEffort(m_date, m_newvalue);
}
void ModifyCompletionActualEffortCmd::unexecute()
{
    m_completion.setActualEffort(m_date, m_oldvalue);
    cmd.unexecute();
}

ModifyCompletionStartedCmd::ModifyCompletionStartedCmd(Completion &completion, bool value, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_completion(completion),
        oldvalue(m_completion.isStarted()),
        newvalue(value)
{
}
void ModifyCompletionStartedCmd::execute()
{
    m_completion.setStarted(newvalue);


}
void ModifyCompletionStartedCmd::unexecute()
{
    m_completion.setStarted(oldvalue);


}

ModifyCompletionFinishedCmd::ModifyCompletionFinishedCmd(Completion &completion, bool value, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_completion(completion),
        oldvalue(m_completion.isFinished()),
        newvalue(value)
{
}
void ModifyCompletionFinishedCmd::execute()
{
    m_completion.setFinished(newvalue);


}
void ModifyCompletionFinishedCmd::unexecute()
{
    m_completion.setFinished(oldvalue);


}

ModifyCompletionStartTimeCmd::ModifyCompletionStartTimeCmd(Completion &completion, const QDateTime &value, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_completion(completion),
        oldvalue(m_completion.startTime()),
        newvalue(value)
{
    m_timeZone = static_cast<Project*>(completion.node()->projectNode())->timeZone();
}
void ModifyCompletionStartTimeCmd::execute()
{
    m_completion.setStartTime(DateTime(newvalue, m_timeZone));


}
void ModifyCompletionStartTimeCmd::unexecute()
{
    m_completion.setStartTime(oldvalue);


}

ModifyCompletionFinishTimeCmd::ModifyCompletionFinishTimeCmd(Completion &completion, const QDateTime &value, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_completion(completion),
        oldvalue(m_completion.finishTime()),
        newvalue(value)
{
    m_timeZone = static_cast<Project*>(completion.node()->projectNode())->timeZone();
}
void ModifyCompletionFinishTimeCmd::execute()
{
    m_completion.setFinishTime(DateTime(newvalue, m_timeZone));


}
void ModifyCompletionFinishTimeCmd::unexecute()
{
    m_completion.setFinishTime(oldvalue);


}

AddCompletionEntryCmd::AddCompletionEntryCmd(Completion &completion, const QDate &date, Completion::Entry *value, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_completion(completion),
        m_date(date),
        newvalue(value),
        m_newmine(true)
{
}
AddCompletionEntryCmd::~AddCompletionEntryCmd()
{
    if (m_newmine)
        delete newvalue;
}
void AddCompletionEntryCmd::execute()
{
    Q_ASSERT(! m_completion.entries().contains(m_date));
    m_completion.addEntry(m_date, newvalue);
    m_newmine = false;

}
void AddCompletionEntryCmd::unexecute()
{
    m_completion.takeEntry(m_date);
    m_newmine = true;

}

RemoveCompletionEntryCmd::RemoveCompletionEntryCmd(Completion &completion, const QDate &date, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_completion(completion),
        m_date(date),
        m_mine(false)
{
    value = m_completion.entry(date);
}
RemoveCompletionEntryCmd::~RemoveCompletionEntryCmd()
{
    debugPlan<<m_mine<<value;
    if (m_mine)
        delete value;
}
void RemoveCompletionEntryCmd::execute()
{
    if (! m_completion.entries().contains(m_date)) {
        warnPlan<<"Completion entries does not contain date:"<<m_date;
    }
    if (value) {
        m_completion.takeEntry(m_date);
        m_mine = true;
    }

}
void RemoveCompletionEntryCmd::unexecute()
{
    if (value) {
        m_completion.addEntry(m_date, value);
    }
    m_mine = false;

}


ModifyCompletionEntryCmd::ModifyCompletionEntryCmd(Completion &completion, const QDate &date, Completion::Entry *value, const KUndo2MagicString& name)
        : NamedCommand(name)
{
    cmd = new MacroCommand(KUndo2MagicString());
    cmd->addCommand(new RemoveCompletionEntryCmd(completion, date));
    cmd->addCommand(new AddCompletionEntryCmd(completion, date, value));
}
ModifyCompletionEntryCmd::~ModifyCompletionEntryCmd()
{
    delete cmd;
}
void ModifyCompletionEntryCmd::execute()
{
    cmd->execute();
}
void ModifyCompletionEntryCmd::unexecute()
{
    cmd->unexecute();
}

AddCompletionUsedEffortCmd::AddCompletionUsedEffortCmd(Completion &completion, const Resource *resource, Completion::UsedEffort *value, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_completion(completion),
        m_resource(resource),
        newvalue(value),
        m_newmine(true),
        m_oldmine(false)
{
    oldvalue = m_completion.usedEffort(resource);
}
AddCompletionUsedEffortCmd::~AddCompletionUsedEffortCmd()
{
    if (m_oldmine)
        delete oldvalue;
    if (m_newmine)
        delete newvalue;
}
void AddCompletionUsedEffortCmd::execute()
{
    if (oldvalue) {
        m_completion.takeUsedEffort(m_resource);
        m_oldmine = true;
    }
    m_completion.addUsedEffort(m_resource, newvalue);
    m_newmine = false;

}
void AddCompletionUsedEffortCmd::unexecute()
{
    m_completion.takeUsedEffort(m_resource);
    if (oldvalue) {
        m_completion.addUsedEffort(m_resource, oldvalue);
    }
    m_newmine = true;
    m_oldmine = false;

}

AddCompletionActualEffortCmd::AddCompletionActualEffortCmd(Task *task, Resource *resource, const QDate &date, const Completion::UsedEffort::ActualEffort &value, const KUndo2MagicString& name)
    : NamedCommand(name)
    , m_task(task)
    , m_resource(resource)
    , m_date(date)
    , newvalue(value)
{
    oldvalue = task->completion().getActualEffort(resource, date);
}
AddCompletionActualEffortCmd::~AddCompletionActualEffortCmd()
{
}
void AddCompletionActualEffortCmd::execute()
{
    m_task->completion().setActualEffort(m_resource, m_date, newvalue);
}
void AddCompletionActualEffortCmd::unexecute()
{
    m_task->completion().setActualEffort(m_resource, m_date, oldvalue);
}

AddAccountCmd::AddAccountCmd(Project &project, Account *account, const QString& parent, int index, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_project(project),
        m_account(account),
        m_parent(nullptr),
        m_index(index),
        m_parentName(parent)
{
    m_mine = true;
}

AddAccountCmd::AddAccountCmd(Project &project, Account *account, Account *parent, int index, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_project(project),
        m_account(account),
        m_parent(parent),
        m_index(index)
{
    m_mine = true;
}

AddAccountCmd::~AddAccountCmd()
{
    if (m_mine)
        delete m_account;
}

void AddAccountCmd::execute()
{
    if (m_parent == nullptr && !m_parentName.isEmpty()) {
        m_parent = m_project.accounts().findAccount(m_parentName);
    }
    m_project.accounts().insert(m_account, m_parent, m_index);


    m_mine = false;
}
void AddAccountCmd::unexecute()
{
    m_project.accounts().take(m_account);


    m_mine = true;
}

RemoveAccountCmd::RemoveAccountCmd(Project &project, Account *account, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_project(project),
    m_account(account),
    m_parent(account->parent())
{
    if (m_parent) {
        m_index = m_parent->accountList().indexOf(account);
    } else {
        m_index = project.accounts().accountList().indexOf(account);
    }
    m_mine = false;
    m_isDefault = account == project.accounts().defaultAccount();

    for (Account::CostPlace *cp : m_account->costPlaces()) {
        if (cp->node()) {
            if (cp->running()) {
                m_cmd.addCommand(new NodeModifyRunningAccountCmd(*cp->node(), cp->node()->runningAccount(), nullptr));
            }
            if (cp->startup()) {
                m_cmd.addCommand(new NodeModifyStartupAccountCmd(*cp->node(), cp->node()->startupAccount(), nullptr));
            }
            if (cp->shutdown()) {
                m_cmd.addCommand(new NodeModifyShutdownAccountCmd(*cp->node(), cp->node()->shutdownAccount(), nullptr));
            }
        } else if (cp->resource()) {
            m_cmd.addCommand(new ResourceModifyAccountCmd(*cp->resource(), cp->resource()->account(), nullptr));
        }
    }
    for (int i = account->accountList().count()-1; i >= 0; --i) {
        m_cmd.addCommand(new RemoveAccountCmd(project, account->accountList().at(i)));
    }
}

RemoveAccountCmd::~RemoveAccountCmd()
{
    if (m_mine)
        delete m_account;
}

void RemoveAccountCmd::execute()
{
    if (m_isDefault) {
        m_project.accounts().setDefaultAccount(nullptr);
    }
    m_cmd.execute(); // remove costplaces and children

    m_project.accounts().take(m_account);

    m_mine = true;
}
void RemoveAccountCmd::unexecute()
{
    m_project.accounts().insert(m_account, m_parent, m_index);

    m_cmd.unexecute(); // add costplaces && children

    if (m_isDefault) {
        m_project.accounts().setDefaultAccount(m_account);
    }

    m_mine = false;
}

RenameAccountCmd::RenameAccountCmd(Account *account, const QString& value, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_account(account)
{
    m_oldvalue = account->name();
    m_newvalue = value;
}

void RenameAccountCmd::execute()
{
    m_account->setName(m_newvalue);

}
void RenameAccountCmd::unexecute()
{
    m_account->setName(m_oldvalue);

}

ModifyAccountDescriptionCmd::ModifyAccountDescriptionCmd(Account *account, const QString& value, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_account(account)
{
    m_oldvalue = account->description();
    m_newvalue = value;
}

void ModifyAccountDescriptionCmd::execute()
{
    m_account->setDescription(m_newvalue);

}
void ModifyAccountDescriptionCmd::unexecute()
{
    m_account->setDescription(m_oldvalue);

}


NodeModifyStartupCostCmd::NodeModifyStartupCostCmd(Node &node, double value, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_node(node)
{
    m_oldvalue = node.startupCost();
    m_newvalue = value;
}

void NodeModifyStartupCostCmd::execute()
{
    m_node.setStartupCost(m_newvalue);

}
void NodeModifyStartupCostCmd::unexecute()
{
    m_node.setStartupCost(m_oldvalue);

}

NodeModifyShutdownCostCmd::NodeModifyShutdownCostCmd(Node &node, double value, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_node(node)
{
    m_oldvalue = node.shutdownCost();
    m_newvalue = value;
}

void NodeModifyShutdownCostCmd::execute()
{
    m_node.setShutdownCost(m_newvalue);

}
void NodeModifyShutdownCostCmd::unexecute()
{
    m_node.setShutdownCost(m_oldvalue);

}

NodeModifyRunningAccountCmd::NodeModifyRunningAccountCmd(Node &node, Account *oldvalue, Account *newvalue, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_node(node)
{
    m_oldvalue = oldvalue;
    m_newvalue = newvalue;
    //debugPlan;
}
void NodeModifyRunningAccountCmd::execute()
{
    //debugPlan;
    if (m_oldvalue) {
        m_oldvalue->removeRunning(m_node);
    }
    if (m_newvalue) {
        m_newvalue->addRunning(m_node);
    }

}
void NodeModifyRunningAccountCmd::unexecute()
{
    //debugPlan;
    if (m_newvalue) {
        m_newvalue->removeRunning(m_node);
    }
    if (m_oldvalue) {
        m_oldvalue->addRunning(m_node);
    }

}

NodeModifyStartupAccountCmd::NodeModifyStartupAccountCmd(Node &node, Account *oldvalue, Account *newvalue, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_node(node)
{
    m_oldvalue = oldvalue;
    m_newvalue = newvalue;
    //debugPlan;
}

void NodeModifyStartupAccountCmd::execute()
{
    //debugPlan;
    if (m_oldvalue) {
        m_oldvalue->removeStartup(m_node);
    }
    if (m_newvalue) {
        m_newvalue->addStartup(m_node);
    }

}
void NodeModifyStartupAccountCmd::unexecute()
{
    //debugPlan;
    if (m_newvalue) {
        m_newvalue->removeStartup(m_node);
    }
    if (m_oldvalue) {
        m_oldvalue->addStartup(m_node);
    }

}

NodeModifyShutdownAccountCmd::NodeModifyShutdownAccountCmd(Node &node, Account *oldvalue, Account *newvalue, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_node(node)
{
    m_oldvalue = oldvalue;
    m_newvalue = newvalue;
    //debugPlan;
}

void NodeModifyShutdownAccountCmd::execute()
{
    //debugPlan;
    if (m_oldvalue) {
        m_oldvalue->removeShutdown(m_node);
    }
    if (m_newvalue) {
        m_newvalue->addShutdown(m_node);
    }

}
void NodeModifyShutdownAccountCmd::unexecute()
{
    //debugPlan;
    if (m_newvalue) {
        m_newvalue->removeShutdown(m_node);
    }
    if (m_oldvalue) {
        m_oldvalue->addShutdown(m_node);
    }

}

ModifyDefaultAccountCmd::ModifyDefaultAccountCmd(Accounts &acc, Account *oldvalue, Account *newvalue, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_accounts(acc)
{
    m_oldvalue = oldvalue;
    m_newvalue = newvalue;
    //debugPlan;
}

void ModifyDefaultAccountCmd::execute()
{
    //debugPlan;
    m_accounts.setDefaultAccount(m_newvalue);

}
void ModifyDefaultAccountCmd::unexecute()
{
    //debugPlan;
    m_accounts.setDefaultAccount(m_oldvalue);

}

ResourceModifyAccountCmd::ResourceModifyAccountCmd(Resource &resource,  Account *oldvalue, Account *newvalue, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_resource(resource)
{
    m_oldvalue = oldvalue;
    m_newvalue = newvalue;
}
void ResourceModifyAccountCmd::execute()
{
    //debugPlan;
    if (m_oldvalue) {
        m_oldvalue->removeRunning(m_resource);
    }
    if (m_newvalue) {
        m_newvalue->addRunning(m_resource);
    }
}
void ResourceModifyAccountCmd::unexecute()
{
    //debugPlan;
    if (m_newvalue) {
        m_newvalue->removeRunning(m_resource);
    }
    if (m_oldvalue) {
        m_oldvalue->addRunning(m_resource);
    }
}

ProjectModifyConstraintCmd::ProjectModifyConstraintCmd(Project &node, Node::ConstraintType c, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_node(node),
        newConstraint(c),
        oldConstraint(static_cast<Node::ConstraintType>(node.constraint()))
{
}
void ProjectModifyConstraintCmd::execute()
{
    m_node.setConstraint(newConstraint);
}
void ProjectModifyConstraintCmd::unexecute()
{
    m_node.setConstraint(oldConstraint);
}

ProjectModifyStartTimeCmd::ProjectModifyStartTimeCmd(Project &node, const QDateTime& dt, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_node(node),
        newTime(dt),
        oldTime(node.startTime())
{
    m_timeZone = node.timeZone();
}

void ProjectModifyStartTimeCmd::execute()
{
    m_node.setConstraintStartTime(DateTime(newTime, m_timeZone));
}
void ProjectModifyStartTimeCmd::unexecute()
{
    m_node.setConstraintStartTime(oldTime);
}

ProjectModifyEndTimeCmd::ProjectModifyEndTimeCmd(Project &node, const QDateTime& dt, const KUndo2MagicString& name)
        : NamedCommand(name),
        m_node(node),
        newTime(dt),
        oldTime(node.endTime())
{
    m_timeZone = node.timeZone();
}
void ProjectModifyEndTimeCmd::execute()
{
    m_node.setEndTime(DateTime(newTime, m_timeZone));
    m_node.setConstraintEndTime(DateTime(newTime, m_timeZone));
}
void ProjectModifyEndTimeCmd::unexecute()
{
    m_node.setConstraintEndTime(oldTime);
}

ProjectModifyWorkPackageInfoCmd::ProjectModifyWorkPackageInfoCmd(Project &project, const Project::WorkPackageInfo &wpi, const KUndo2MagicString& name)
    : NamedCommand(name)
    , m_node(project)
    , m_newWpi(wpi)
    , m_oldWpi(project.workPackageInfo())
{
}
void ProjectModifyWorkPackageInfoCmd::execute()
{
    m_node.setWorkPackageInfo(m_newWpi);
}
void ProjectModifyWorkPackageInfoCmd::unexecute()
{
    m_node.setWorkPackageInfo(m_oldWpi);
}

//----------------------------
SwapScheduleManagerCmd::SwapScheduleManagerCmd(Project &project, ScheduleManager *from, ScheduleManager *to, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_node(project),
    m_from(from),
    m_to(to)
{
}

SwapScheduleManagerCmd::~SwapScheduleManagerCmd()
{
}

void SwapScheduleManagerCmd::execute()
{
    m_node.swapScheduleManagers(m_from, m_to);
}

void SwapScheduleManagerCmd::unexecute()
{
    m_node.swapScheduleManagers(m_to, m_from);
}

AddScheduleManagerCmd::AddScheduleManagerCmd(Project &node, ScheduleManager *sm, int index, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_node(node),
    m_parent(sm->parentManager()),
    m_sm(sm),
    m_index(index),
    m_exp(sm->expected()),
    m_mine(true)
{
}

AddScheduleManagerCmd::AddScheduleManagerCmd(ScheduleManager *parent, ScheduleManager *sm, int index, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_node(parent->project()),
    m_parent(parent),
    m_sm(sm),
    m_index(index),
    m_exp(sm->expected()),
    m_mine(true)
{
    if (parent->schedulingMode() == ScheduleManager::AutoMode) {
        m_cmd.addCommand(new ModifyScheduleManagerSchedulingModeCmd(*parent, ScheduleManager::ManualMode));
    }
}

AddScheduleManagerCmd::~AddScheduleManagerCmd()
{
    if (m_mine) {
        m_sm->setParentManager(nullptr);
        delete m_sm;
    }
}

void AddScheduleManagerCmd::execute()
{
    m_cmd.redo();
    m_node.addScheduleManager(m_sm, m_parent, m_index);
    m_sm->setExpected(m_exp);
    m_mine = false;
}

void AddScheduleManagerCmd::unexecute()
{
    m_node.takeScheduleManager(m_sm);
    m_sm->setExpected(nullptr);
    m_mine = true;
    m_cmd.undo();
}

DeleteScheduleManagerCmd::DeleteScheduleManagerCmd(Project &node, ScheduleManager *sm, const KUndo2MagicString& name)
    : AddScheduleManagerCmd(node, sm, -1, name)
{
    m_mine = false;
    m_index = m_parent ? m_parent->indexOf(sm) : node.indexOf(sm);
    const auto managers = sm->children();
    for (ScheduleManager *s : managers) {
        cmd.addCommand(new DeleteScheduleManagerCmd(node, s));
    }
}

void DeleteScheduleManagerCmd::execute()
{
    cmd.execute();
    AddScheduleManagerCmd::unexecute();
}

void DeleteScheduleManagerCmd::unexecute()
{
    AddScheduleManagerCmd::execute();
    cmd.unexecute();
}

MoveScheduleManagerCmd::MoveScheduleManagerCmd(ScheduleManager *sm, ScheduleManager *newparent, int newindex, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_sm(sm),
    m_oldparent(sm->parentManager()),
    m_newparent(newparent),
    m_newindex(newindex)
{
    m_oldindex = sm->parentManager() ? sm->parentManager()->indexOf(sm) : sm->project().indexOf(sm);
}

void MoveScheduleManagerCmd::execute()
{
    m_sm->project().moveScheduleManager(m_sm, m_newparent, m_newindex);
}

void MoveScheduleManagerCmd::unexecute()
{
    m_sm->project().moveScheduleManager(m_sm, m_oldparent, m_oldindex);
}

ModifyScheduleManagerNameCmd::ModifyScheduleManagerNameCmd(ScheduleManager &sm, const QString& value, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_sm(sm),
    oldvalue(sm.name()),
    newvalue(value)
{
}

void ModifyScheduleManagerNameCmd::execute()
{
    m_sm.setName(newvalue);
}

void ModifyScheduleManagerNameCmd::unexecute()
{
    m_sm.setName(oldvalue);
}

ModifyScheduleManagerSchedulingModeCmd::ModifyScheduleManagerSchedulingModeCmd(ScheduleManager &sm, int value, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_sm(sm),
    oldvalue(sm.schedulingMode()),
    newvalue(value)
{
    if (value == ScheduleManager::AutoMode) {
        // Allow only one
        const auto allScheduleManagers = sm.project().allScheduleManagers();
        for (ScheduleManager *m : allScheduleManagers) {
            if (m->schedulingMode() == ScheduleManager::AutoMode) {
                m_cmd.addCommand(new ModifyScheduleManagerSchedulingModeCmd(*m, ScheduleManager::ManualMode));
                break;
            }
        }
    }
}

void ModifyScheduleManagerSchedulingModeCmd::execute()
{
    m_cmd.redo();
    m_sm.setSchedulingMode(newvalue);
}

void ModifyScheduleManagerSchedulingModeCmd::unexecute()
{
    m_sm.setSchedulingMode(oldvalue);
    m_cmd.undo();
}

ModifyScheduleManagerAllowOverbookingCmd::ModifyScheduleManagerAllowOverbookingCmd(ScheduleManager &sm, bool value, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_sm(sm),
    oldvalue(sm.allowOverbooking()),
    newvalue(value)
{
}

void ModifyScheduleManagerAllowOverbookingCmd::execute()
{
    m_sm.setAllowOverbooking(newvalue);
}

void ModifyScheduleManagerAllowOverbookingCmd::unexecute()
{
    m_sm.setAllowOverbooking(oldvalue);
}

ModifyScheduleManagerDistributionCmd::ModifyScheduleManagerDistributionCmd(ScheduleManager &sm, bool value, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_sm(sm),
    oldvalue(sm.usePert()),
    newvalue(value)
{
}

void ModifyScheduleManagerDistributionCmd::execute()
{
    m_sm.setUsePert(newvalue);
}

void ModifyScheduleManagerDistributionCmd::unexecute()
{
    m_sm.setUsePert(oldvalue);
}

ModifyScheduleManagerSchedulingDirectionCmd::ModifyScheduleManagerSchedulingDirectionCmd(ScheduleManager &sm, bool value, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_sm(sm),
    oldvalue(sm.schedulingDirection()),
    newvalue(value)
{
}

void ModifyScheduleManagerSchedulingDirectionCmd::execute()
{
    m_sm.setSchedulingDirection(newvalue);
}

void ModifyScheduleManagerSchedulingDirectionCmd::unexecute()
{
    m_sm.setSchedulingDirection(oldvalue);
}

ModifyScheduleManagerSchedulerCmd::ModifyScheduleManagerSchedulerCmd(ScheduleManager &sm, int value, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_sm(sm),
    oldvalue(sm.schedulerPluginIndex()),
    newvalue(value)
{
}

void ModifyScheduleManagerSchedulerCmd::execute()
{
    m_sm.setSchedulerPlugin(newvalue);
}

void ModifyScheduleManagerSchedulerCmd::unexecute()
{
    m_sm.setSchedulerPlugin(oldvalue);
}

ModifyScheduleManagerSchedulingGranularityCmd::ModifyScheduleManagerSchedulingGranularityCmd(ScheduleManager &sm, int value, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_sm(sm),
    oldvalue(sm.granularity()),
    newvalue(value)
{
}

void ModifyScheduleManagerSchedulingGranularityCmd::execute()
{
    m_sm.setGranularity(newvalue);
}

void ModifyScheduleManagerSchedulingGranularityCmd::unexecute()
{
    m_sm.setGranularity(oldvalue);
}

CalculateScheduleCmd::CalculateScheduleCmd(Project &node, ScheduleManager *sm, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_node(node),
    m_sm(sm),
    m_first(true),
    m_newexpected(nullptr)
{
    if (sm->recalculate() && sm->isScheduled()) {
        m_sm = new ScheduleManager(node);
        m_sm->setRecalculate(true);
        m_sm->setGranularity(sm->granularity());
        m_sm->setUsePert(sm->usePert());
        m_sm->setSchedulerPluginId(sm->schedulerPluginId());
        m_sm->setAllowOverbooking(sm->allowOverbooking());
        m_sm->setName(sm->name());

        preCmd.addCommand(new AddScheduleManagerCmd(sm, m_sm));

        postCmd.addCommand(new SwapScheduleManagerCmd(node, sm, m_sm));
        postCmd.addCommand(new MoveScheduleManagerCmd(m_sm, sm->parentManager(), sm->parentManager() ? sm->parentManager()->indexOf(sm)+1 : node.indexOf(sm)+1));
        postCmd.addCommand(new DeleteScheduleManagerCmd(node, sm));
    }
    m_oldexpected = m_sm->expected();
}

CalculateScheduleCmd::~CalculateScheduleCmd()
{
    if (m_sm->scheduling()) {
        m_sm->haltCalculation();
    }
}

void CalculateScheduleCmd::execute()
{
    Q_ASSERT(m_sm);
    preCmd.redo();
    if (m_first) {
        m_sm->calculateSchedule();
        if (m_sm->calculationResult() != ScheduleManager::CalculationCanceled) {
            m_first = false;
        }
        m_newexpected = m_sm->expected();
    } else {
        m_sm->setExpected(m_newexpected);
    }
    postCmd.redo();
}

void CalculateScheduleCmd::unexecute()
{
    if (m_sm->scheduling()) {
        // terminate scheduling
        QApplication::setOverrideCursor(Qt::WaitCursor);
        m_sm->haltCalculation();
        m_first = true;
        QApplication::restoreOverrideCursor();

    }
    postCmd.undo();
    m_sm->setExpected(m_oldexpected);
    preCmd.undo();
}

//------------------------
BaselineScheduleCmd::BaselineScheduleCmd(ScheduleManager &sm, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_sm(sm)
{
}

void BaselineScheduleCmd::execute()
{
    m_sm.setBaselined(true);
}

void BaselineScheduleCmd::unexecute()
{
    m_sm.setBaselined(false);
}

ResetBaselineScheduleCmd::ResetBaselineScheduleCmd(ScheduleManager &sm, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_sm(sm)
{
}

void ResetBaselineScheduleCmd::execute()
{
    m_sm.setBaselined(false);
}

void ResetBaselineScheduleCmd::unexecute()
{
    m_sm.setBaselined(true);
}

//------------------------
ModifyStandardWorktimeYearCmd::ModifyStandardWorktimeYearCmd(StandardWorktime *wt, double oldvalue, double newvalue, const KUndo2MagicString& name)
        : NamedCommand(name),
        swt(wt),
        m_oldvalue(oldvalue),
        m_newvalue(newvalue)
{
}
void ModifyStandardWorktimeYearCmd::execute()
{
    swt->setYear(m_newvalue);

}
void ModifyStandardWorktimeYearCmd::unexecute()
{
    swt->setYear(m_oldvalue);

}

ModifyStandardWorktimeMonthCmd::ModifyStandardWorktimeMonthCmd(StandardWorktime *wt, double oldvalue, double newvalue, const KUndo2MagicString& name)
        : NamedCommand(name),
        swt(wt),
        m_oldvalue(oldvalue),
        m_newvalue(newvalue)
{
}
void ModifyStandardWorktimeMonthCmd::execute()
{
    swt->setMonth(m_newvalue);

}
void ModifyStandardWorktimeMonthCmd::unexecute()
{
    swt->setMonth(m_oldvalue);

}

ModifyStandardWorktimeWeekCmd::ModifyStandardWorktimeWeekCmd(StandardWorktime *wt, double oldvalue, double newvalue, const KUndo2MagicString& name)
        : NamedCommand(name),
        swt(wt),
        m_oldvalue(oldvalue),
        m_newvalue(newvalue)
{
}
void ModifyStandardWorktimeWeekCmd::execute()
{
    swt->setWeek(m_newvalue);

}
void ModifyStandardWorktimeWeekCmd::unexecute()
{
    swt->setWeek(m_oldvalue);

}

ModifyStandardWorktimeDayCmd::ModifyStandardWorktimeDayCmd(StandardWorktime *wt, double oldvalue, double newvalue, const KUndo2MagicString& name)
        : NamedCommand(name),
        swt(wt),
        m_oldvalue(oldvalue),
        m_newvalue(newvalue)
{
}

void ModifyStandardWorktimeDayCmd::execute()
{
    swt->setDay(m_newvalue);

}
void ModifyStandardWorktimeDayCmd::unexecute()
{
    swt->setDay(m_oldvalue);

}

//----------------
DocumentAddCmd::DocumentAddCmd(Documents &docs, Document *value, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_docs(docs),
    m_mine(true)
{
    Q_ASSERT(value);
    m_value = value;
}
DocumentAddCmd::~DocumentAddCmd()
{
    //debugPlan;
    if (m_mine)
        delete m_value;
}
void DocumentAddCmd::execute()
{
    m_docs.addDocument(m_value);
    m_mine = false;
}
void DocumentAddCmd::unexecute()
{
    m_docs.takeDocument(m_value);
    m_mine = true;
}

//----------------
DocumentRemoveCmd::DocumentRemoveCmd(Documents &docs, Document *value, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_docs(docs),
    m_mine(false)
{
    Q_ASSERT(value);
    m_value = value;
}
DocumentRemoveCmd::~DocumentRemoveCmd()
{
    //debugPlan;
    if (m_mine)
        delete m_value;
}
void DocumentRemoveCmd::execute()
{
    m_docs.takeDocument(m_value);
    m_mine = true;
}
void DocumentRemoveCmd::unexecute()
{
    m_docs.addDocument(m_value);
    m_mine = false;
}

//----------------
DocumentModifyUrlCmd::DocumentModifyUrlCmd(Document *doc, const QUrl &value, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_doc(doc)
{
    Q_ASSERT(doc);
    m_value = value;
    m_oldvalue = doc->url();
}
void DocumentModifyUrlCmd::execute()
{
    m_doc->setUrl(m_value);
}
void DocumentModifyUrlCmd::unexecute()
{
    m_doc->setUrl(m_oldvalue);
}

//----------------
DocumentModifyNameCmd::DocumentModifyNameCmd(Document *doc, const QString &value, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_doc(doc)
{
    Q_ASSERT(doc);
    m_value = value;
    m_oldvalue = doc->name();
}
void DocumentModifyNameCmd::execute()
{
    m_doc->setName(m_value);
}
void DocumentModifyNameCmd::unexecute()
{
    m_doc->setName(m_oldvalue);
}

//----------------
DocumentModifyTypeCmd::DocumentModifyTypeCmd(Document *doc, Document::Type value, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_doc(doc)
{
    Q_ASSERT(doc);
    m_value = value;
    m_oldvalue = doc->type();
}
void DocumentModifyTypeCmd::execute()
{
    m_doc->setType(m_value);
}
void DocumentModifyTypeCmd::unexecute()
{
    m_doc->setType(m_oldvalue);
}

//----------------
DocumentModifyStatusCmd::DocumentModifyStatusCmd(Document *doc, const QString &value, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_doc(doc)
{
    Q_ASSERT(doc);
    m_value = value;
    m_oldvalue = doc->type();
}
void DocumentModifyStatusCmd::execute()
{
    m_doc->setStatus(m_value);
}
void DocumentModifyStatusCmd::unexecute()
{
    m_doc->setStatus(m_oldvalue);
}

//----------------
DocumentModifySendAsCmd::DocumentModifySendAsCmd(Document *doc, const Document::SendAs value, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_doc(doc)
{
    Q_ASSERT(doc);
    m_value = value;
    m_oldvalue = doc->sendAs();
}
void DocumentModifySendAsCmd::execute()
{
    m_doc->setSendAs(m_value);
}
void DocumentModifySendAsCmd::unexecute()
{
    m_doc->setSendAs(m_oldvalue);
}

//----------------
WBSDefinitionModifyCmd::WBSDefinitionModifyCmd(Project &project, const WBSDefinition value, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_project(project)
{
    m_newvalue = value;
    m_oldvalue = m_project.wbsDefinition();
}
void WBSDefinitionModifyCmd::execute()
{
    m_project.setWbsDefinition(m_newvalue);
}
void WBSDefinitionModifyCmd::unexecute()
{
    m_project.setWbsDefinition(m_oldvalue);
}

//----------------
InsertProjectCmd::InsertProjectCmd(Project &fromProject, Node *parent, Node *after, const KUndo2MagicString& name)
    : MacroCommand(name),
    m_project(static_cast<Project*>(parent->projectNode())),
    m_parent(parent)
{
    Q_ASSERT(&fromProject != m_project);

    if (m_project->defaultCalendar()) {
        fromProject.setDefaultCalendar(nullptr); // or else m_project default calendar may be overwitten
    }
    QString defaultAccount;
    if (! m_project->accounts().defaultAccount() && fromProject.accounts().defaultAccount()) {
        defaultAccount = fromProject.accounts().defaultAccount()->name();
    }

    QHash<Node*, QString> startupaccountmap;
    QHash<Node*, QString> shutdownaccountmap;
    QHash<Node*, QString> runningaccountmap;
    QHash<Node*, QString> nodecalendarmap;

    // remove unhandled info in tasks and get accounts and calendars
    const auto nodes = fromProject.allNodes();
    for (Node *n : nodes ) {
        if (n->type() == Node::Type_Task) {
            Task *t = static_cast<Task*>(n);
            t->workPackage().clear();
            while (! t->workPackageLog().isEmpty()) {
                WorkPackage *wp = t->workPackageLog().at(0);
                t->removeWorkPackage(wp);
                delete wp;
            }
        }
        if (n->startupAccount()) {
            startupaccountmap.insert(n, n->startupAccount()->name());
            n->setStartupAccount(nullptr);
        }
        if (n->shutdownAccount()) {
            shutdownaccountmap.insert(n, n->shutdownAccount()->name());
            n->setShutdownAccount(nullptr);
        }
        if (n->runningAccount()) {
            runningaccountmap.insert(n, n->runningAccount()->name());
            n->setRunningAccount(nullptr);
        }
        if (n->estimate()->calendar()) {
            nodecalendarmap.insert(n, n->estimate()->calendar()->id());
            n->estimate()->setCalendar(nullptr);
        }
    }
    // get resources pointing to calendars and accounts
    QHash<Resource*, QString> resaccountmap;
    QHash<Resource*, QString> rescalendarmap;
    const auto resources = fromProject.resourceList();
    for (Resource *r : resources) {
        if (r->account()) {
            resaccountmap.insert(r, r->account()->name());
            r->setAccount(nullptr);
        }
        if (r->calendar()) {
            rescalendarmap.insert(r, r->calendar()->id());
            r->setCalendar(nullptr);
        }
    }
    // create add account commands and keep track of used and unused accounts
    QList<Account*> unusedAccounts;
    QMap<QString, Account*> accountsmap;
    const auto accounts = m_project->accounts().allAccounts();
    for (Account *a : accounts) {
        accountsmap.insert(a->name(), a);
    }
    const auto accounts2 = fromProject.accounts().accountList();
    for (Account *a : accounts2) {
        addAccounts(a, nullptr, unusedAccounts, accountsmap);
    }
    // create add calendar commands and keep track of used and unused calendars
    QList<Calendar*> unusedCalendars;
    QMap<QString, Calendar*> calendarsmap;
    const auto calendars = m_project->allCalendars();
    for (Calendar *c : calendars) {
        calendarsmap.insert(c->id(), c);
    }
    const auto calendars2 = fromProject.calendars();
    for (Calendar *c : calendars2) {
        addCalendars(c, nullptr, unusedCalendars, calendarsmap);
    }
    // get all requests from fromProject before resources are merged
    QMultiHash<Node*, QPair<ResourceRequest*, Resource*> > rreqs;
    const auto nodes2 = fromProject.allNodes();
    for (Node *n : nodes2 ) {
        if (n->type() != (int)Node::Type_Task || n->requests().isEmpty()) {
            continue;
        }
        while (ResourceRequest *rr = n->requests().resourceRequests(false).value(0)) {
            debugPlanInsertProject<<"Get resource request:"<<rr;
            rreqs.insert(n, QPair<ResourceRequest*, Resource*>(rr, rr->resource()));
            // all resource requests shall be reinserted
            rr->unregisterRequest();
            n->requests().removeResourceRequest(rr);
        }
    }
    debugPlanInsertProject<<"fromProject resource requests:"<<rreqs;
    QList<ResourceGroup*> allGroups;
    QList<ResourceGroup*> newGroups;
    QHash<ResourceGroup*, ResourceGroup*> existingGroups; // QHash<fromProject group, toProject group>
    const auto groups = fromProject.resourceGroups();
    for (ResourceGroup *g : groups) {
        ResourceGroup *gr = m_project->findResourceGroup(g->id());
        if (gr == nullptr) {
            gr = g;
            newGroups << gr;
        } else {
            existingGroups[ gr ] = g;
        }
        allGroups << gr;
    }
    debugPlanInsertProject<<"fromProject resources:"<<fromProject.resourceList();
    QList<Resource*> allResources;
    QList<Resource*> newResources; // resource in fromProject that does not exist in toProject
    QHash<Resource*, Resource*> existingResources; // hash[toProject resource, fromProject resource]
    const auto resources2 = fromProject.resourceList();
    for (Resource *r : resources2) {
        while (Schedule *s = r->schedules().values().value(0)) {
            r->deleteSchedule(s); // schedules not handled
        }
        Resource *res = m_project->resource(r->id());
        if (res == nullptr) {
            newResources << r;
            allResources << r;
        } else {
            existingResources[ res ] = r;
            allResources << res;
        }
    }
    // Add new resources
    for (Resource *r : qAsConst(newResources)) {
        debugPlanInsertProject<<"AddResourceCmd:"<<r->name()<<r->parentGroups();
        addCommand(new AddResourceCmd(m_project, r, kundo2_noi18n("Resource")));
    }
    // Add new groups
    for (ResourceGroup *g : qAsConst(newGroups)) {
        debugPlanInsertProject<<"AddResourceGroupCmd:"<<g->name()<<g->resources();
        addCommand(new AddResourceGroupCmd(m_project, g, kundo2_noi18n("ResourceGroup")));
        const auto resources = g->resources();
        for (Resource *r : resources) {
            addCommand(new AddParentGroupCmd(r, g));
        }
    }
    // Update resource account
    {QHash<Resource*, QString>::const_iterator it = resaccountmap.constBegin();
    QHash<Resource*, QString>::const_iterator end = resaccountmap.constEnd();
    for (; it != end; ++it) {
        Resource *r = it.key();
        if (newResources.contains(r)) {
            Q_ASSERT(allResources.contains(r));
            addCommand(new ResourceModifyAccountCmd(*r, nullptr, accountsmap.value(it.value())));
        }
    }}
    // Update resource calendar
    {QHash<Resource*, QString>::const_iterator it = rescalendarmap.constBegin();
    QHash<Resource*, QString>::const_iterator end = rescalendarmap.constEnd();
    for (; it != end; ++it) {
        Resource *r = it.key();
        if (newResources.contains(r)) {
            Q_ASSERT(allResources.contains(r));
            addCommand(new ModifyResourceCalendarCmd(r, calendarsmap.value(it.value())));
        }
    }}
    debugPlanInsertProject<<"Requests: clean up requests to resources already in m_project";
    debugPlanInsertProject<<"All groups:"<<allGroups;
    debugPlanInsertProject<<"New groups:"<<newGroups;
    debugPlanInsertProject<<"Existing groups:"<<existingGroups;
    debugPlanInsertProject<<"All resources:"<<allResources;
    debugPlanInsertProject<<"New resources:"<<newResources;
    debugPlanInsertProject<<"Existing resources:"<<existingResources;
    debugPlanInsertProject<<"Resource requests:"<<rreqs;

    int ri = 0;
    QHash<Node*, QPair<ResourceRequest*, Resource*> >::const_iterator i;
    for (i = rreqs.constBegin(); i != rreqs.constEnd(); ++i) {
        Node *n = i.key();
        ResourceRequest *rr = i.value().first;
        rr->setId(0);
        rr->setCollection(nullptr);
        Resource *newRes = i.value().second;
        debugPlanInsertProject<<"Resource exists:"<<newRes<<newRes->id()<<(void*)newRes<<':'<<m_project->resource(newRes->id());
        if (Resource *nr = existingResources.key(newRes)) {
            newRes = nr;
            debugPlanInsertProject<<"Resource existed:"<<newRes<<(void*)newRes;
        }
        debugPlanInsertProject<<"Add resource request:"<<"task:"<<n->wbsCode()<<n->name()<<"resource:"<<newRes<<"requests:"<<rr;
        if (!rr->requiredResources().isEmpty()) {
            // the resource request may have required resources that needs mapping
            QList<Resource*> required;
            const auto resources = rr->requiredResources();
            for (Resource *r : resources) {
                if (newResources.contains(r)) {
                    required << r;
                    debugPlanInsertProject<<"Request: required (new)"<<r->name();
                    continue;
                }
                Resource *r2 = existingResources.key(r);
                Q_ASSERT(allResources.contains(r2));
                if (r2) {
                    debugPlanInsertProject<<"Request: required (existing)"<<r2->name();
                    required << r2;
                }
            }
            rr->setRequiredResources(required);
        }
        if (!rr->alternativeRequests().isEmpty()) {
            // TODO alternatives
        }
        Q_ASSERT(allResources.contains(newRes));
        // all resource requests shall be reinserted
        rr->setResource(newRes);
        addCommand(new AddResourceRequestCmd(&n->requests(), rr, kundo2_noi18n("Resource %1", ++ri)));
    }
    // Add nodes (ids are unique, no need to check)
    Node *node_after = after;
    for (int i = 0; i < fromProject.numChildren(); ++i) {
        Node *n = fromProject.childNode(i);
        Q_ASSERT(n);
        while (Schedule *s = n->schedules().values().value(0)) {
            n->takeSchedule(s); // schedules not handled
            delete s;
        }
        n->setParentNode(nullptr);
        if (node_after) {
            addCommand(new TaskAddCmd(m_project, n, node_after, kundo2_noi18n("Task")));
            node_after = n;
        } else {
            addCommand(new SubtaskAddCmd(m_project, n, parent, kundo2_noi18n("Subtask")));
        }
        addChildNodes(n);
    }
    // Dependencies:
    const auto nodes3 = fromProject.allNodes();
    for (Node *n : nodes3) {
        while (n->numDependChildNodes() > 0) {
            Relation *r = n->dependChildNodes().at(0);
            n->takeDependChildNode(r);
            r->child()->takeDependParentNode(r);
            addCommand(new AddRelationCmd(*m_project, r));
        }
    }
    // node calendar
    {QHash<Node*, QString>::const_iterator it = nodecalendarmap.constBegin();
    QHash<Node*, QString>::const_iterator end = nodecalendarmap.constEnd();
    for (; it != end; ++it) {
        addCommand(new ModifyEstimateCalendarCmd(*(it.key()), nullptr, calendarsmap.value(it.value())));
    }}
    // node startup account
    {QHash<Node*, QString>::const_iterator it = startupaccountmap.constBegin();
    QHash<Node*, QString>::const_iterator end = startupaccountmap.constEnd();
    for (; it != end; ++it) {
        addCommand(new NodeModifyStartupAccountCmd(*(it.key()), nullptr, accountsmap.value(it.value())));
    }}
    // node shutdown account
    {QHash<Node*, QString>::const_iterator it = shutdownaccountmap.constBegin();
    QHash<Node*, QString>::const_iterator end = shutdownaccountmap.constEnd();
    for (; it != end; ++it) {
        addCommand(new NodeModifyShutdownAccountCmd(*(it.key()), nullptr, accountsmap.value(it.value())));
    }}
    // node running account
    {QHash<Node*, QString>::const_iterator it = runningaccountmap.constBegin();
    QHash<Node*, QString>::const_iterator end = runningaccountmap.constEnd();
    for (; it != end; ++it) {
        addCommand(new NodeModifyRunningAccountCmd(*(it.key()), nullptr, accountsmap.value(it.value())));
    }}

    if (! defaultAccount.isEmpty()) {
        Account *a = accountsmap.value(defaultAccount);
        if (a && a->list()) {
            addCommand(new ModifyDefaultAccountCmd(m_project->accounts(), nullptr, a));
        }
    }
    debugPlanInsertProject<<"Cleanup unused stuff from inserted project:"<<&fromProject;
    // Cleanup
    // Remove nodes from fromProject so they are not deleted
    while (Node *ch = fromProject.childNode(0)) {
        fromProject.takeChildNode(ch);
    }
    const auto nodes4 = fromProject.allNodes();
    for (Node *n : nodes4) {
        fromProject.removeId(n->id());
    }

    // Remove calendars from fromProject
    while (fromProject.calendarCount() > 0) {
        fromProject.takeCalendar(fromProject.calendarAt(0));
    }
    qDeleteAll(unusedCalendars);

    // Remove accounts from fromProject
    while (fromProject.accounts().accountCount() > 0) {
        fromProject.accounts().take(fromProject.accounts().accountAt(0));
    }
    qDeleteAll(unusedAccounts);

    for (Resource *r = fromProject.resourceList().value(0); r; r = fromProject.resourceList().value(0)) {
        debugPlanInsertProject<<"remove all resources from fromProject:"<<r;
        if (!fromProject.takeResource(r)) {
            errorPlanInsertProject<<"Internal error, failed to remove resource";
            Q_ASSERT(false);
            break;
        }
    }
    while (fromProject.numResourceGroups() > 0) {
        ResourceGroup *g = fromProject.resourceGroupAt(0);
        debugPlanInsertProject<<"Take used group:"<<g;
        fromProject.takeResourceGroup(g);
    }
    debugPlanInsertProject<<"Delete unused resources:"<<existingResources;
    qDeleteAll(existingResources); // deletes unused resources
    debugPlanInsertProject<<"Delete unused groups:"<<existingGroups;
    qDeleteAll(existingGroups); // deletes unused resource groups
}

void InsertProjectCmd::addCalendars(Calendar *calendar, Calendar *parent, QList<Calendar*> &unused, QMap<QString, Calendar*> &calendarsmap) {
    Calendar *par = nullptr;
    if (parent) {
        par = calendarsmap.value(parent->id());
    }
    if (par == nullptr) {
        par = parent;
    }
    Calendar *cal = calendarsmap.value(calendar->id());
    if (cal == nullptr) {
        calendarsmap.insert(calendar->id(), calendar);
        addCommand(new CalendarAddCmd(m_project, calendar, -1, par));
    } else {
        unused << calendar;
    }
    const auto calendars = calendar->calendars();
    for (Calendar *c : calendars) {
        addCalendars(c, calendar, unused, calendarsmap);
    }
}

void InsertProjectCmd::addAccounts(Account *account, Account *parent, QList<Account*>  &unused, QMap<QString, Account*>  &accountsmap) {
    Account *par = nullptr;
    if (parent) {
        par = accountsmap.value(parent->name());
    }
    if (par == nullptr) {
        par = parent;
    }
    Account *acc = accountsmap.value(account->name());
    if (acc == nullptr) {
        accountsmap.insert(account->name(), account);
        addCommand(new AddAccountCmd(*m_project, account, par, -1, kundo2_noi18n("Add account %1", account->name())));
    } else {
        unused << account;
    }
    while (! account->accountList().isEmpty()) {
        Account *a = account->accountList().first();
        account->list()->take(a);
        addAccounts(a, account, unused, accountsmap);
    }
}

void InsertProjectCmd::addChildNodes(Node *node) {
    // schedules not handled
    while (Schedule *s = node->schedules().values().value(0)) {
        node->takeSchedule(s); // schedules not handled
        delete s;
    }
    const auto nodes = node->childNodeIterator();
    for (Node *n : nodes ) {
        n->setParentNode(nullptr);
        addCommand(new SubtaskAddCmd(m_project, n, node, kundo2_noi18n("Subtask")));
        addChildNodes(n);
    }
    // Remove child nodes so they are not added twice
    while (Node *ch = node->childNode(0)) {
        node->takeChildNode(ch);
    }
}

void InsertProjectCmd::execute()
{
    debugPlanInsertProject<<"before execute:"<<m_project->resourceGroups()<<m_project->resourceList();
    QApplication::setOverrideCursor(Qt::WaitCursor);
    MacroCommand::execute();
    QApplication::restoreOverrideCursor();
    debugPlanInsertProject<<"after execute:"<<m_project->resourceGroups()<<m_project->resourceList();
}
void InsertProjectCmd::unexecute()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    MacroCommand::unexecute();
    QApplication::restoreOverrideCursor();
}

WorkPackageAddCmd::WorkPackageAddCmd(Project *project, Node *node, WorkPackage *value, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_project(project),
    m_node(node),
    m_wp(value),
    m_mine(true)
{
}
WorkPackageAddCmd::~WorkPackageAddCmd()
{
    if (m_mine) {
        delete m_wp;
    }
}
void WorkPackageAddCmd::execute()
{
    // FIXME use project
    //m_project->addWorkPackage(m_node, m_wp);
    static_cast<Task*>(m_node)->addWorkPackage(m_wp);
}
void WorkPackageAddCmd::unexecute()
{
    // FIXME use project
    //m_project->removeWorkPackage(m_node, m_wp);
    static_cast<Task*>(m_node)->removeWorkPackage(m_wp);
}

ModifyProjectLocaleCmd::ModifyProjectLocaleCmd(Project &project, const KUndo2MagicString& name)
    : MacroCommand(name),
    m_project(project)
{
};
void ModifyProjectLocaleCmd::execute()
{
    MacroCommand::execute();
    m_project.emitLocaleChanged();
}
void ModifyProjectLocaleCmd::unexecute()
{
    MacroCommand::unexecute();
    m_project.emitLocaleChanged();
}

ModifyCurrencySymolCmd::ModifyCurrencySymolCmd(Locale *locale, const QString &value, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_locale(locale),
    m_newvalue(value),
    m_oldvalue(locale->currencySymbol())
{
};
void ModifyCurrencySymolCmd::execute()
{
    m_locale->setCurrencySymbol(m_newvalue);
}
void ModifyCurrencySymolCmd::unexecute()
{
    m_locale->setCurrencySymbol(m_oldvalue);
}

ModifyCurrencyFractionalDigitsCmd::ModifyCurrencyFractionalDigitsCmd(Locale *locale, int value, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_locale(locale),
    m_newvalue(value),
    m_oldvalue(locale->monetaryDecimalPlaces())
{
};
void ModifyCurrencyFractionalDigitsCmd::execute()
{
    m_locale->setMonetaryDecimalPlaces(m_newvalue);
}
void ModifyCurrencyFractionalDigitsCmd::unexecute()
{
    m_locale->setMonetaryDecimalPlaces(m_oldvalue);
}

AddExternalAppointmentCmd::AddExternalAppointmentCmd(Resource *resource, const QString &pid, const QString &pname, const QDateTime &start, const QDateTime &end, double load, const KUndo2MagicString& name)
    : NamedCommand(name),
    m_resource(resource),
    m_pid(pid),
    m_pname(pname),
    m_start(start),
    m_end(end),
    m_load(load)
{
}

void AddExternalAppointmentCmd::execute()
{
    m_resource->addExternalAppointment(m_pid, m_pname, m_start, m_end, m_load);
}

void AddExternalAppointmentCmd::unexecute()
{
    m_resource->subtractExternalAppointment(m_pid, m_start, m_end, m_load);
    // FIXME do this smarter
    if (! m_resource->externalAppointments(m_pid).isEmpty()) {
        m_resource->takeExternalAppointment(m_pid);
    }
}

ClearExternalAppointmentCmd::ClearExternalAppointmentCmd(Resource *resource, const QString &pid, const KUndo2MagicString &name)
    : NamedCommand(name),
    m_resource(resource),
    m_pid(pid),
    m_appointments(nullptr)
{
}

ClearExternalAppointmentCmd::~ClearExternalAppointmentCmd()
{
    delete m_appointments;
}

void ClearExternalAppointmentCmd::execute()
{
//     debugPlan<<text()<<":"<<m_resource->name()<<m_pid;
    m_appointments = m_resource->takeExternalAppointment(m_pid);
}

void ClearExternalAppointmentCmd::unexecute()
{
//     debugPlan<<text()<<":"<<m_resource->name()<<m_pid;
    if (m_appointments) {
        m_resource->addExternalAppointment(m_pid, m_appointments);
    }
    m_appointments = nullptr;
}

ClearAllExternalAppointmentsCmd::ClearAllExternalAppointmentsCmd(Project *project, const KUndo2MagicString &name)
    : NamedCommand(name),
    m_project(project)
{
    const auto resources = project->resourceList();
    for (Resource *r : resources) {
        const QMap<QString, QString> map = r->externalProjects();
        QMap<QString, QString>::const_iterator it;
        for (it = map.constBegin(); it != map.constEnd(); ++it) {
            m_cmd.addCommand(new ClearExternalAppointmentCmd(r, it.key()));
        }
    }
}

void ClearAllExternalAppointmentsCmd::execute()
{
    m_cmd.redo();
}

void ClearAllExternalAppointmentsCmd::unexecute()
{
    m_cmd.undo();
}

SharedResourcesFileCmd::SharedResourcesFileCmd(Project *project, const QString &newValue, const KUndo2MagicString& name)
    : NamedCommand(name)
    , m_project(project)
    , m_oldValue(project->sharedResourcesFile())
    , m_newValue(newValue)
{
}

void SharedResourcesFileCmd::execute()
{
    m_project->setSharedResourcesFile(m_newValue);
}

void SharedResourcesFileCmd::unexecute()
{
    m_project->setSharedResourcesFile(m_oldValue);
}

UseSharedResourcesCmd::UseSharedResourcesCmd(Project *project, bool newValue, const KUndo2MagicString& name)
    : NamedCommand(name)
    , m_project(project)
    , m_oldValue(project->useSharedResources())
    , m_newValue(newValue)
{
}

void UseSharedResourcesCmd::execute()
{
    m_project->setUseSharedResources(m_newValue);
}

void UseSharedResourcesCmd::unexecute()
{
    m_project->setUseSharedResources(m_oldValue);
}

SharedProjectsUrlCmd::SharedProjectsUrlCmd(Project *project, const QUrl &newValue, const KUndo2MagicString& name)
    : NamedCommand(name)
    , m_project(project)
    , m_oldValue(project->sharedProjectsUrl())
    , m_newValue(newValue)
{
}

void SharedProjectsUrlCmd::execute()
{
    m_project->setSharedProjectsUrl(m_newValue);
}

void SharedProjectsUrlCmd::unexecute()
{
    m_project->setSharedProjectsUrl(m_oldValue);
}

LoadProjectsAtStartupCmd::LoadProjectsAtStartupCmd(Project *project, bool newValue, const KUndo2MagicString& name)
    : NamedCommand(name)
    , m_project(project)
    , m_oldValue(project->loadProjectsAtStartup())
    , m_newValue(newValue)
{
}

void LoadProjectsAtStartupCmd::execute()
{
    m_project->setLoadProjectsAtStartup(m_newValue);
}

void LoadProjectsAtStartupCmd::unexecute()
{
    m_project->setLoadProjectsAtStartup(m_oldValue);
}

}  //KPlato namespace
