/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "PlanGroupDebug.h"


const QLoggingCategory &PLANPORTFOLIO_LOG()
{
    static const QLoggingCategory category("calligra.plan.group");
    return category;
}
