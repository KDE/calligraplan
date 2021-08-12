/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2009 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPTCONFIGSKELETON_H
#define KPTCONFIGSKELETON_H

#include "plan_export.h"

#include <kconfigskeleton.h>


class PLAN_EXPORT KPlatoConfigSkeleton : public KConfigSkeleton
{
    Q_OBJECT
public:    
    KPlatoConfigSkeleton();

    ~KPlatoConfigSkeleton() override;

};

#endif // KPTCONFIGSKELETON_H
