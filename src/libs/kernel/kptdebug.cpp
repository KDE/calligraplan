/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2012 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptdebug.h"

const QLoggingCategory &PLANSHARED_LOG()
{
    static const QLoggingCategory category("calligra.plan.shared");
    return category;
}

const QLoggingCategory &PLANDEPEDITOR_LOG()
{
    static const QLoggingCategory category("calligra.plan.dependencyeditor");
    return category;
}

const QLoggingCategory &PLANXML_LOG()
{
    static const QLoggingCategory category("calligra.plan.xml");
    return category;
}

const QLoggingCategory &PLAN_LOG()
{
    static const QLoggingCategory category("calligra.plan");
    return category;
}

const QLoggingCategory &PLANWP_LOG()
{
    static const QLoggingCategory category("calligra.plan.wp");
    return category;
}

const QLoggingCategory &PLANCHART_LOG()
{
    static const QLoggingCategory category("calligra.plan.chart");
    return category;
}

const QLoggingCategory &PLANINSPROJECT_LOG()
{
    static const QLoggingCategory category("calligra.plan.insertProject");
    return category;
}
