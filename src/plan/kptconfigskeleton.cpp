/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2009 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/


// clazy:excludeall=qstring-arg
#include "kptconfigskeleton.h"

#include "kptfactory.h"

#include <KoComponentData.h>

KPlatoConfigSkeleton::KPlatoConfigSkeleton()
    : KConfigSkeleton(KPlato::Factory::global().config())
{}

KPlatoConfigSkeleton::~KPlatoConfigSkeleton()
{}
