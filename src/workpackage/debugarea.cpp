/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2012 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "debugarea.h"

const QLoggingCategory &PLANWORK_LOG()
{
    static const QLoggingCategory category("calligra.planwork");
    return category;
}
