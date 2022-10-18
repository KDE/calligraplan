/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2022 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "planmodels_export.h"

#include <QLoggingCategory>

extern const PLANMODELS_EXPORT QLoggingCategory &PLANRG_LOG();
#define dbgRG qCDebug(PLANRG_LOG)<<Q_FUNC_INFO

extern const PLANMODELS_EXPORT QLoggingCategory &PLANRG_TMP_LOG();
#define dbgRGTmp qCDebug(PLANRG_TMP_LOG)<<Q_FUNC_INFO

extern const PLANMODELS_EXPORT QLoggingCategory &PLANRG_TABLE_LOG();
#define dbgRGTable qCDebug(PLANRG_TABLE_LOG)<<Q_FUNC_INFO

extern const PLANMODELS_EXPORT QLoggingCategory &PLANRG_CHART_LOG();
#define dbgRGChart qCDebug(PLANRG_CHART_LOG)<<Q_FUNC_INFO

extern const PLANMODELS_EXPORT QLoggingCategory &PLANRG_VARIABLE_LOG();
#define dbgRGVariable qCDebug(PLANRG_VARIABLE_LOG)<<Q_FUNC_INFO

extern const PLANMODELS_EXPORT QLoggingCategory &PLANRG_TR_LOG();
#define dbgRGTr qCDebug(PLANRG_TR_LOG)<<Q_FUNC_INFO
