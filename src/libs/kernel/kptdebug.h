/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2012 Dag Andersen <dag.andersen@kdemail.net>
  SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
  
  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTDEBUG_H
#define KPTDEBUG_H

#include "plankernel_export.h"

#include <QDebug>
#include <QLoggingCategory>
#include <QString>

extern const PLANKERNEL_EXPORT QLoggingCategory &PLAN_LOG();

#define debugPlan qCDebug(PLAN_LOG)<<Q_FUNC_INFO
#define warnPlan qCWarning(PLAN_LOG)<<"WARN:"<<Q_FUNC_INFO
#define errorPlan qCCritical(PLAN_LOG)<<"ERROR:"<<Q_FUNC_INFO

extern const PLANKERNEL_EXPORT QLoggingCategory &PLANDEPEDITOR_LOG();

#define debugPlanDepEditor qCDebug(PLANDEPEDITOR_LOG)<<Q_FUNC_INFO
#define warnPlanDepEditor qCWarning(PLANDEPEDITOR_LOG)<<"WARN:"<<Q_FUNC_INFO
#define errorPlanDepEditor qCCritical(PLANDEPEDITOR_LOG)<<"ERROR:"<<Q_FUNC_INFO

extern const PLANKERNEL_EXPORT QLoggingCategory &PLANXML_LOG();

#define debugPlanXml qCDebug(PLANXML_LOG)<<Q_FUNC_INFO
#define warnPlanXml qCWarning(PLANXML_LOG)<<"WARN:"<<Q_FUNC_INFO
#define errorPlanXml qCCritical(PLANXML_LOG)<<"ERROR:"<<Q_FUNC_INFO

extern const PLANKERNEL_EXPORT QLoggingCategory &PLANSHARED_LOG();

#define debugPlanShared qCDebug(PLANSHARED_LOG)<<Q_FUNC_INFO
#define warnPlanShared qCWarning(PLANSHARED_LOG)<<"WARN:"<<Q_FUNC_INFO
#define errorPlanShared qCCritical(PLANSHARED_LOG)<<"ERROR:"<<Q_FUNC_INFO

extern const PLANKERNEL_EXPORT QLoggingCategory &PLANWP_LOG();

#define debugPlanWp qCDebug(PLANWP_LOG)<<Q_FUNC_INFO
#define warnPlanWp qCWarning(PLANWP_LOG)<<"WARN:"<<Q_FUNC_INFO
#define errorPlanWp qCCritical(PLANWP_LOG)<<"ERROR:"<<Q_FUNC_INFO

extern const PLANKERNEL_EXPORT QLoggingCategory &PLANCHART_LOG();

#define debugPlanChart qCDebug(PLANCHART_LOG)<<Q_FUNC_INFO
#define warnPlanChart qCWarning(PLANCHART_LOG)<<"WARN:"<<Q_FUNC_INFO
#define errorPlanChart qCCritical(PLANCHART_LOG)<<"ERROR:"<<Q_FUNC_INFO

extern const PLANKERNEL_EXPORT QLoggingCategory &PLANINSPROJECT_LOG();

#define debugPlanInsertProject qCDebug(PLANINSPROJECT_LOG)
#define warnPlanInsertProject qCWarning(PLANINSPROJECT_LOG)
#define errorPlanInsertProject qCCritical(PLANINSPROJECT_LOG)

#endif
