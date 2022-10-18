/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2022 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ReportGeneratorDebug.h"

const QLoggingCategory &PLANRG_LOG()
{
    static const QLoggingCategory category("calligra.plan.report");
    return category;
}

const QLoggingCategory &PLANRG_TMP_LOG()
{
    static const QLoggingCategory category("calligra.plan.report.template");
    return category;
}

const QLoggingCategory &PLANRG_TABLE_LOG()
{
    static const QLoggingCategory category("calligra.plan.report.table");
    return category;
}

const QLoggingCategory &PLANRG_CHART_LOG()
{
    static const QLoggingCategory category("calligra.plan.report.chart");
    return category;
}

const QLoggingCategory &PLANRG_VARIABLE_LOG()
{
    static const QLoggingCategory category("calligra.plan.report.variable");
    return category;
}

const QLoggingCategory &PLANRG_TR_LOG()
{
    static const QLoggingCategory category("calligra.plan.report.tr");
    return category;
}
