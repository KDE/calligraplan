/* This file is part of the KDE project
 * Copyright (C) 1998, 1999, 2000 Torben Weis <weis@kde.org>
 * Copyright (C) 2002 - 2010 Dag Andersen <dag.andersen@kdemail.net>
 * Copyright (C) 2019 Dag Andersen <dag.andersen@kdemail.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef PLANPORTFOLIO_DEBUG_H
#define PLANPORTFOLIO_DEBUG_H

#include <QLoggingCategory>
#include <QDebug>

extern const QLoggingCategory &PLANPORTFOLIO_LOG();

#define debugPlanGroup qCDebug(PLANPORTFOLIO_LOG)<<Q_FUNC_INFO
#define warnPlanGroup qCWarning(PLANPORTFOLIO_LOG)<<Q_FUNC_INFO
#define errorPlanGroup qCCritical(PLANPORTFOLIO_LOG)<<Q_FUNC_INFO

#endif
