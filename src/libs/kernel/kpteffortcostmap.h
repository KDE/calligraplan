/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTEFFORTCOST_H
#define KPTEFFORTCOST_H

#include <QDate>
#include <QMap>

#include "kptduration.h"
#include "kptdebug.h"

#include <QDebug>
#include <QMetaType>

namespace KPlato
{

class PLANKERNEL_EXPORT EffortCost
{
public:
    EffortCost()
        : m_effort(Duration::zeroDuration),
        m_cost(0),
        m_bcwpEffort(0.0),
        m_bcwpCost(0.0)
    {}
    EffortCost(KPlato::Duration effort, const double cost)
        : m_effort(effort),
        m_cost(cost),
        m_bcwpEffort(0.0),
        m_bcwpCost(0.0)
    {
        //debugPlan;
    }
    ~EffortCost() {
        //debugPlan;
    }
    double hours() const { return m_effort.toDouble(Duration::Unit_h); }
    Duration effort() const { return m_effort; }
    void setEffort(KPlato::Duration effort) { m_effort = effort; }
    double cost() const { return m_cost; }
    void setCost(double cost) { m_cost = cost; }
    void setBcwpEffort(double value) { m_bcwpEffort = value; }
    double bcwpEffort() const { return m_bcwpEffort; }
    void setBcwpCost(double value) { m_bcwpCost = value; }
    double bcwpCost() const { return m_bcwpCost; }
    void add(const Duration &effort, double cost, double bcwpEffort = 0.0, double bcwpCost = 0.0);
    EffortCost &operator+=(const EffortCost &ec) {
        add(ec.m_effort, ec.m_cost, ec.m_bcwpEffort, ec.m_bcwpCost);
        return *this;
    }
    
#ifndef QT_NO_DEBUG_STREAM
    QDebug debug(QDebug dbg) const;
#endif

private:
    Duration m_effort;
    double m_cost;
    double m_bcwpEffort;
    double m_bcwpCost;
};

typedef QMap<QDate, EffortCost> EffortCostDayMap;
class PLANKERNEL_EXPORT EffortCostMap
{
public:
    EffortCostMap()
        : m_days() {
        //debugPlan; 
    }
    EffortCostMap(const EffortCostMap &map);
    
    ~EffortCostMap() {
        //debugPlan;
        m_days.clear();
    }
    
    void clear() { m_days.clear(); }
    
    EffortCost effortCost(QDate date) const {
        EffortCost ec;
        if (!date.isValid()) {
            //errorPlan<<"Date not valid";
            return ec;
        }
        EffortCostDayMap::const_iterator it = m_days.find(date);
        if (it != m_days.end())
            ec = it.value();
        return ec;
    }
    void insert(const QDate &date, const EffortCost &ec);

    void insert(QDate date, KPlato::Duration effort, const double cost) {
        if (!date.isValid()) {
            //errorPlan<<"Date not valid";
            return;
        }
        m_days.insert(date, EffortCost(effort, cost));
    }
    /** 
     * If data for this date already exists add the new values to the old,
     * else the new values are inserted.
     */
    EffortCost &add(QDate date, KPlato::Duration effort, const double cost) {
        return add(date, EffortCost(effort, cost));
    }
    /** 
     * If data for this date already exists add the new values to the old,
     * else the new value is inserted.
     */
    EffortCost &add(QDate date, const EffortCost &ec) {
        if (!date.isValid()) {
            //errorPlan<<"Date not valid";
            return zero();
        }
        //debugPlan<<date.toString();
        return m_days[date] += ec;
    }
    
    bool isEmpty() const {
        return m_days.isEmpty();
    }
    
    const EffortCostDayMap &days() const { return m_days; }
    
    EffortCostMap &operator=(const EffortCostMap &ec);
    EffortCostMap &operator+=(const EffortCostMap &ec);
    EffortCost &effortCostOnDate(QDate date) {
        return m_days[date];
    }
    /// Return total cost for the next num days starting at date
    double cost(QDate date, int num=7) {
        double r=0.0;
        for (int i=0; i < num; ++i) {
            r += costOnDate(date.addDays(i));
        }
        return r;
    }
    double costOnDate(QDate date) const {
        if (!date.isValid()) {
            //errorPlan<<"Date not valid";
            return 0.0;
        }
        if (m_days.contains(date)) {
            return m_days[date].cost();
        }
        return 0.0;
    }
    Duration effortOnDate(QDate date) const {
        if (!date.isValid()) {
            errorPlan<<"Date not valid";
            return Duration::zeroDuration;
        }
        if (m_days.contains(date)) {
            return m_days[date].effort();
        }
        return Duration::zeroDuration;
    }
    double hoursOnDate(QDate date) const {
        if (!date.isValid()) {
            errorPlan<<"Date not valid";
            return 0.0;
        }
        if (m_days.contains(date)) {
            return m_days[date].hours();
        }
        return 0.0;
    }
    void addBcwpCost(const QDate &date, double cost);

    double bcwpCostOnDate(QDate date) const {
        if (!date.isValid()) {
            //errorPlan<<"Date not valid";
            return 0.0;
        }
        if (m_days.contains(date)) {
            return m_days[date].bcwpCost();
        }
        return 0.0;
    }
    double bcwpEffortOnDate(QDate date) const {
        if (!date.isValid()) {
            //errorPlan<<"Date not valid";
            return 0.0;
        }
        if (m_days.contains(date)) {
            return m_days[date].bcwpEffort();
        }
        return 0.0;
    }
    double totalCost() const {
        double cost = 0.0;
        EffortCostDayMap::const_iterator it;
        for(it = m_days.constBegin(); it != m_days.constEnd(); ++it) {
            cost += it.value().cost();
        }
        return cost;
    }
    Duration totalEffort() const {
        Duration eff;
        EffortCostDayMap::const_iterator it;
        for(it = m_days.constBegin(); it != m_days.constEnd(); ++it) {
            eff += it.value().effort();
        }
        return eff;
    }
    
    double costTo(QDate date) const;
    Duration effortTo(QDate date) const;
    double hoursTo(QDate date) const;

    /// Return the BCWP cost to @p date. (BSWP is cumulative)
    double bcwpCost(const QDate &date) const;
    /// Return the BCWP effort to @p date. (BSWP is cumulative)
    double bcwpEffort(const QDate &date) const;
    /// Return the BCWP total cost. Since BCWP is cumulative this is the last entry.
    double bcwpTotalCost() const {
        double cost = 0.0;
        if (! m_days.isEmpty()) {
            cost = m_days.last().bcwpCost();
        }
        return cost;
    }
    /// Return the BCWP total cost. Since BCWP is cumulative this is the last entry.
    double bcwpTotalEffort() const {
        double eff = 0.0;
        if (! m_days.isEmpty()) {
            eff = m_days.last().bcwpEffort();
        }
        return eff;
    }
    
    QDate startDate() const { return m_days.isEmpty() ? QDate() : m_days.firstKey(); }
    QDate endDate() const { return m_days.isEmpty() ? QDate() : m_days.lastKey(); }
    
#ifndef QT_NO_DEBUG_STREAM
    QDebug debug(QDebug dbg) const;
#endif

private:
    EffortCost &zero() { return m_zero; }
    
private:
    EffortCost m_zero;
    EffortCostDayMap m_days;
};


} //namespace KPlato

Q_DECLARE_METATYPE(KPlato::EffortCost)
Q_DECLARE_METATYPE(KPlato::EffortCostMap)

#ifndef QT_NO_DEBUG_STREAM
PLANKERNEL_EXPORT QDebug operator<<(QDebug dbg, const KPlato::EffortCost &ec);
PLANKERNEL_EXPORT QDebug operator<<(QDebug dbg, const KPlato::EffortCost *ec);
PLANKERNEL_EXPORT QDebug operator<<(QDebug dbg, const KPlato::EffortCostMap &i);
#endif

#endif
