/* This file is part of the Calligra project
 * SPDX-FileCopyrightText: 2008 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "Account.h"

#include "Project.h"

#include <kptaccount.h>

Scripting::Account::Account(Scripting::Project *project, KPlato::Account *account, QObject *parent)
    : QObject(parent), m_project(project), m_account(account)
{
}

QObject *Scripting::Account::project()
{
    return m_project;
}

QString Scripting::Account::name() const
{
    return m_account->name();
}

int Scripting::Account::childCount() const
{
    return m_account->childCount();
}

QObject *Scripting::Account::childAt(int index)
{
    return m_project->account(m_account->childAt(index));
}

QVariant Scripting::Account::plannedEffortCostPrDay(const QVariant &start, const QVariant &end, const QVariant &schedule)
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
    KPlato::Accounts *a = m_account->list();
    if (a == 0) {
        return QVariant();
    }
    KPlato::EffortCostMap ec = a->plannedCost(*m_account, s, e, schedule.toLongLong());
    KPlato::EffortCostDayMap::ConstIterator it = ec.days().constBegin();
    for (; it != ec.days().constEnd(); ++it) {
        map.insert(it.key().toString(Qt::ISODate), QVariantList() << it.value().effort().toDouble(KPlato::Duration::Unit_h) << it.value().cost());
    }
    return map;
}

QVariant Scripting::Account::actualEffortCostPrDay(const QVariant &start, const QVariant &end, const QVariant &schedule)
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
    KPlato::Accounts *a = m_account->list();
    if (a == 0) {
        return QVariant();
    }
    KPlato::EffortCostMap ec = a->actualCost(*m_account, s, e, schedule.toLongLong());
    KPlato::EffortCostDayMap::ConstIterator it = ec.days().constBegin();
    for (; it != ec.days().constEnd(); ++it) {
        map.insert(it.key().toString(Qt::ISODate), QVariantList() << it.value().effort().toDouble(KPlato::Duration::Unit_h) << it.value().cost());
    }
    return map;
}

QVariant Scripting::Account::plannedEffortCostPrDay(const QVariant &schedule)
{
    //kDebug(planDbg())<<start<<end<<schedule;
    QVariantMap map;
    KPlato::EffortCostMap ec = m_account->plannedCost(schedule.toLongLong());
    KPlato::EffortCostDayMap::ConstIterator it = ec.days().constBegin();
    for (; it != ec.days().constEnd(); ++it) {
        map.insert(it.key().toString(Qt::ISODate), QVariantList() << it.value().effort().toDouble(KPlato::Duration::Unit_h) << it.value().cost());
    }
    return map;
}

QVariant Scripting::Account::actualEffortCostPrDay(const QVariant &schedule)
{
    //kDebug(planDbg())<<start<<end<<schedule;
    QVariantMap map;
    KPlato::EffortCostMap ec = m_account->actualCost(schedule.toLongLong());
    KPlato::EffortCostDayMap::ConstIterator it = ec.days().constBegin();
    for (; it != ec.days().constEnd(); ++it) {
        map.insert(it.key().toString(Qt::ISODate), QVariantList() << it.value().effort().toDouble(KPlato::Duration::Unit_h) << it.value().cost());
    }
    return map;
}
