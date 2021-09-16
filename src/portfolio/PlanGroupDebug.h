/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 1998, 1999, 2000 Torben Weis <weis@kde.org>
 * SPDX-FileCopyrightText: 2002-2010 Dag Andersen <dag.andersen@kdemail.net>
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PLANPORTFOLIO_DEBUG_H
#define PLANPORTFOLIO_DEBUG_H

#include <QLoggingCategory>
#include <QDebug>

extern const QLoggingCategory &PLANPORTFOLIO_LOG();

#define debugPortfolio qCDebug(PLANPORTFOLIO_LOG)<<Q_FUNC_INFO
#define warnPortfolio qCWarning(PLANPORTFOLIO_LOG)<<Q_FUNC_INFO
#define errorPortfolio qCCritical(PLANPORTFOLIO_LOG)<<Q_FUNC_INFO

#endif
