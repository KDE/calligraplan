/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2012 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PLANWORK_DEBUGAREA
#define PLANWORK_DEBUGAREA

#include "planwork_export.h"

#include <QDebug>
#include <QLoggingCategory>

extern const PLANWORK_EXPORT QLoggingCategory &PLANWORK_LOG();

#define debugPlanWork qCDebug(PLANWORK_LOG)<<Q_FUNC_INFO
#define warnPlanWork qCWarning(PLANWORK_LOG)
#define errorPlanWork qCCritical(PLANWORK_LOG)

#endif
