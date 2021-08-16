/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2009 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/


// clazy:excludeall=qstring-arg
#include "ConfigSkeleton.h"

#include "PortfolioFactory.h"

#include <KoComponentData.h>

ConfigSkeleton::ConfigSkeleton()
    : KConfigSkeleton(PortfolioFactory::global().config())
{}

ConfigSkeleton::~ConfigSkeleton()
{}
