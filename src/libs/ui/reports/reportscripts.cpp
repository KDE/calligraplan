/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2010, 2012 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "reportscripts.h"

#include "reportdata.h"
#include "kptdebug.h"


namespace KPlato
{


ProjectAccess::ProjectAccess(ReportData *rd)
    : m_reportdata(rd)
{
}

QVariant ProjectAccess::Name() const
{
    if (m_reportdata && m_reportdata->project()) {
        return m_reportdata->project()->name().toUtf8();
    }
    return QVariant();
}

QVariant ProjectAccess::Manager() const
{
    if (m_reportdata && m_reportdata->project()) {
        return m_reportdata->project()->leader().toUtf8();
    }
    return QVariant();
}

QVariant ProjectAccess::Plan() const
{
    if (m_reportdata && m_reportdata->scheduleManager()) {
        return m_reportdata->scheduleManager()->name().toUtf8();
    }
    return QVariant();
}

QVariant ProjectAccess::BCWS() const
{
    if (m_reportdata && m_reportdata->project()) {
        long id = m_reportdata->scheduleManager() ? m_reportdata->scheduleManager()->scheduleId() : BASELINESCHEDULE;
        double r = m_reportdata->project()->bcws(QDate::currentDate(), id);
        return QLocale().toString(r, 'f', 2).toUtf8();
    }
    return QVariant();
}

QVariant ProjectAccess::BCWP() const
{
    if (m_reportdata && m_reportdata->project()) {
        long id = m_reportdata->scheduleManager() ? m_reportdata->scheduleManager()->scheduleId() : BASELINESCHEDULE;
        double r = m_reportdata->project()->bcwp(QDate::currentDate(), id);
        return QLocale().toString(r, 'f', 2).toUtf8();
    }
    warnPlan<<"No report data or project"<<m_reportdata;
    return QVariant();
}

QVariant ProjectAccess::ACWP() const
{
    if (m_reportdata && m_reportdata->project()) {
        long id = m_reportdata->scheduleManager() ? m_reportdata->scheduleManager()->scheduleId() : BASELINESCHEDULE;
        double r = m_reportdata->project()->acwp(QDate::currentDate(), id).cost();
        return QLocale().toString(r, 'f', 2).toUtf8();
    }
    return QVariant();
}

QVariant ProjectAccess::CPI() const
{
    if (m_reportdata && m_reportdata->project()) {
        double r = 0.0;
        long id = m_reportdata->scheduleManager() ? m_reportdata->scheduleManager()->scheduleId() : BASELINESCHEDULE;
        double b = m_reportdata->project()->bcwp(QDate::currentDate(), id);
        double a = m_reportdata->project()->acwp(QDate::currentDate(), id).cost();
        if (a > 0) {
            r = b / a;
        }
        return QLocale().toString(r, 'f', 2).toUtf8();
    }
    return QVariant();
}

QVariant ProjectAccess::SPI() const
{
    debugPlan<<"ProjectAccess::SPI:";
    if (m_reportdata && m_reportdata->project()) {
        int id = m_reportdata->scheduleManager() ? m_reportdata->scheduleManager()->scheduleId() : BASELINESCHEDULE;
        double r = m_reportdata->project()->schedulePerformanceIndex(QDate::currentDate(), id);
        return QLocale().toString(r, 'f', 2).toUtf8();
    }
    return QVariant();
}


} // namespace KPlato
