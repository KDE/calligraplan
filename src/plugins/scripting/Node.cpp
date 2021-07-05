/* This file is part of the Calligra project
 * SPDX-FileCopyrightText: 2008 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "Node.h"

#include "Project.h"

#include "kptnode.h"
#include "kpteffortcostmap.h"
#include "kptduration.h"

Scripting::Node::Node(Scripting::Project *project, KPlato::Node *node, QObject *parent)
    : QObject(parent), m_project(project), m_node(node)
{
}

QObject *Scripting::Node::project()
{
    return m_project;
}

QString Scripting::Node::name()
{
    return m_node->name();
}

QDate Scripting::Node::startDate()
{
    return m_node->startTime().date();
}

QDate Scripting::Node::endDate()
{
    return m_node->endTime().date();
}

QString Scripting::Node::id()
{
    return m_node->id();
}

QVariant Scripting::Node::type()
{
    return m_node->typeToString();
}

int Scripting::Node::childCount() const
{
    return m_node->numChildren();
}

QObject *Scripting::Node::childAt(int index)
{
    return m_project->node(m_node->childNode(index));
}

QObject *Scripting::Node::parentNode()
{
    return m_project->node(m_node->parentNode());
}

QVariant Scripting::Node::plannedEffortCostPrDay(const QVariant &start, const QVariant &end, const QVariant &schedule)
{
    //kDebug(planDbg())<<start<<end<<schedule;
    QVariantMap map;
    QDate s = start.toDate();
    QDate e = end.toDate();
    if (! s.isValid()) {
        s = QDate::currentDate();
    }
    if (! e.isValid()) {
        e = QDate::currentDate();
    }
    KPlato::EffortCostMap ec = m_node->plannedEffortCostPrDay(s, e, schedule.toLongLong());
    KPlato::EffortCostDayMap::ConstIterator it = ec.days().constBegin();
    for (; it != ec.days().constEnd(); ++it) {
        map.insert(it.key().toString(Qt::ISODate), QVariantList() << it.value().effort().toDouble(KPlato::Duration::Unit_h) << it.value().cost());
    }
    return map;
}

QVariant Scripting::Node::bcwsPrDay(const QVariant &schedule) const
{
    QVariantMap map;
    KPlato::EffortCostMap ec = m_node->bcwsPrDay(schedule.toLongLong());
    KPlato::EffortCostDayMap::ConstIterator it = ec.days().constBegin();
    for (; it != ec.days().constEnd(); ++it) {
        map.insert(it.key().toString(Qt::ISODate), QVariantList() << it.value().effort().toDouble(KPlato::Duration::Unit_h) << it.value().cost());
    }
    return map;
}

QVariant Scripting::Node::bcwpPrDay(const QVariant &schedule) const
{
    QVariantMap map;
    KPlato::EffortCostMap ec = m_node->bcwpPrDay(schedule.toLongLong());
    KPlato::EffortCostDayMap::ConstIterator it = ec.days().constBegin();
    for (; it != ec.days().constEnd(); ++it) {
        map.insert(it.key().toString(Qt::ISODate), QVariantList() << it.value().effort().toDouble(KPlato::Duration::Unit_h) << it.value().cost());
    }
    return map;
}

QVariant Scripting::Node::acwpPrDay(const QVariant &schedule) const
{
    QVariantMap map;
    KPlato::EffortCostMap ec = m_node->acwp(schedule.toLongLong());
    KPlato::EffortCostDayMap::ConstIterator it = ec.days().constBegin();
    for (; it != ec.days().constEnd(); ++it) {
        map.insert(it.key().toString(Qt::ISODate), QVariantList() << it.value().effort().toDouble(KPlato::Duration::Unit_h) << it.value().cost());
    }
    return map;
}
