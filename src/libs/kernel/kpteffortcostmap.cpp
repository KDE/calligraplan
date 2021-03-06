/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2010 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kpteffortcostmap.h"

namespace KPlato
{

void EffortCost::add(const Duration &effort, double cost, double bcwpEffort, double bcwpCost)
{
    m_effort += effort;
    m_cost += cost;
    m_bcwpEffort += bcwpEffort;
    m_bcwpCost += bcwpCost;
}

//-----------------------
EffortCostMap::EffortCostMap(const EffortCostMap &map)
{
    m_days = map.m_days;
}

void EffortCostMap::insert(const QDate &date, const EffortCost &ec)
{
    Q_ASSERT(date.isValid());
    m_days[ date ] = ec;
}

EffortCostMap &EffortCostMap::operator=(const EffortCostMap &ec)
{
    m_days = ec.m_days;
    return *this;
}

EffortCostMap &EffortCostMap::operator+=(const EffortCostMap &ec) {
    //debugPlan<<"me="<<m_days.count()<<" ec="<<ec.days().count();
    if (ec.isEmpty()) {
        return *this;
    }
    if (isEmpty()) {
        m_days = ec.days();
        return *this;
    }
    EffortCostMap other = ec;
    QDate oed = other.endDate();
    QDate ed = endDate();
    // get bcwp of the last entries
    EffortCost last_oec = other.m_days[oed];
    last_oec.setEffort(Duration::zeroDuration);
    last_oec.setCost(0.0);
    EffortCost last_ec = m_days[ed];
    last_ec.setEffort(Duration::zeroDuration);
    last_ec.setCost(0.0);
    if (oed > ed) {
        // expand my last entry to match other
        for (QDate d = ed.addDays(1); d <= oed; d = d.addDays(1)) {
            m_days[ d ] = last_ec ;
        }
    }
    EffortCostDayMap::const_iterator it;
    for(it = ec.days().constBegin(); it != ec.days().constEnd(); ++it) {
        add(it.key(), it.value());
    }
    if (oed < ed) {
        // add others last entry to my trailing entries
        for (QDate d = oed.addDays(1); d <= ed; d = d.addDays(1)) {
            m_days[ d ] += last_oec;
        }
    }
    return *this;
}

void EffortCostMap::addBcwpCost(const QDate &date, double cost)
{
    EffortCost ec = m_days[ date ];
    ec.setBcwpCost(ec.bcwpCost() + cost);
    m_days[ date ] = ec;
}

double EffortCostMap::costTo(QDate date) const {
    double cost = 0.0;
    EffortCostDayMap::const_iterator it;
    for(it = m_days.constBegin(); it != m_days.constEnd(); ++it) {
        if (it.key() > date) {
            break;
        }
        cost += it.value().cost();
    }
    return cost;
}

Duration EffortCostMap::effortTo(QDate date) const {
    Duration eff;
    EffortCostDayMap::const_iterator it;
    for(it = m_days.constBegin(); it != m_days.constEnd(); ++it) {
        if (it.key() > date) {
            break;
        }
        eff += it.value().effort();
    }
    return eff;
}

double EffortCostMap::hoursTo(QDate date) const {
    double eff = 0.0;
    EffortCostDayMap::const_iterator it;
    for(it = m_days.constBegin(); it != m_days.constEnd(); ++it) {
        if (it.key() > date) {
            break;
        }
        eff += it.value().hours();
    }
    return eff;
}

double EffortCostMap::bcwpCost(const QDate &date) const
{
    double v = 0.0;
    for (EffortCostDayMap::const_iterator it = m_days.constBegin(); it != m_days.constEnd(); ++it) {
        if (it.key() > date) {
            break;
        }
        v = it.value().bcwpCost();
    }
    return v;
}

double EffortCostMap::bcwpEffort(const QDate &date) const
{
    double v = 0.0;
    for (EffortCostDayMap::const_iterator it = m_days.constBegin(); it != m_days.constEnd(); ++it) {
        if (it.key() > date) {
            break;
        }
        v = it.value().bcwpEffort();
    }
    return v;
}

#ifndef QT_NO_DEBUG_STREAM
QDebug EffortCostMap::debug(QDebug dbg) const
{
    dbg.nospace()<<"EffortCostMap[";
    if (! m_days.isEmpty()) {
        dbg<<startDate().toString(Qt::ISODate)<<" "<<endDate().toString(Qt::ISODate)
            <<" total bcws="<<totalEffort().toDouble(Duration::Unit_h)<<", "<<totalCost()<<" bcwp="<<bcwpTotalEffort()<<" "<<bcwpTotalCost();
    }
    dbg.nospace()<<']';
    if (! m_days.isEmpty()) {
        QMap<QDate, KPlato::EffortCost>::ConstIterator it = days().constBegin();
        for (; it != days().constEnd(); ++it) {
            dbg<<'\n';
            dbg<<"     "<<it.key().toString(Qt::ISODate)<<" "<<it.value();
        }
        dbg<<'\n';
    }
    return dbg;
}
#endif

} //namespace KPlato

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const KPlato::EffortCost &ec)
{
    dbg.nospace()<<"EffortCost[ bcws effort="<<ec.hours()<<" cost="<<ec.cost()<<" : bcwp effort="<<ec.bcwpEffort()<<" cost="<<ec.bcwpCost()<<"]";
    return dbg;
}
QDebug operator<<(QDebug dbg, const KPlato::EffortCost *ec)
{
    return operator<<(dbg, *ec);
}

QDebug operator<<(QDebug dbg, const KPlato::EffortCostMap &i)
{
    return i.debug(dbg);
}
#endif
