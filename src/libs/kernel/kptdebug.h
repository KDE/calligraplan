/* This file is part of the KDE project
  Copyright (C) 2012 Dag Andersen <dag.andersen@kdemail.net>
  Copyright (C) 2016 Dag Andersen <dag.andersen@kdemail.net>
  
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version..

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
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

#endif
