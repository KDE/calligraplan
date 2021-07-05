/*
 *  SPDX-FileCopyrightText: 2015 Friedrich W. H. Kossebau <kossebau@kde.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "MainDebug.h"

const QLoggingCategory &MAIN_LOG() \
{
    static const QLoggingCategory category("calligra.plan.lib.main");
    return category;
}

const QLoggingCategory &FILTER_LOG() \
{
    static const QLoggingCategory category("calligra.plan.lib.filter");
    return category;
}



