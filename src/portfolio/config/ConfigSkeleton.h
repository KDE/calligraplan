/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CONFIGSKELETON_H
#define CONFIGSKELETON_H

#include "planportfolio_export.h"

#include <KConfigSkeleton>


class PLANPORTFOLIO_EXPORT ConfigSkeleton : public KConfigSkeleton
{
    Q_OBJECT
public:    
    ConfigSkeleton();

    ~ConfigSkeleton() override;

};

#endif
