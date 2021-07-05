/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PLANPORTFOLIO_FACTORYINIT_H
#define PLANPORTFOLIO_FACTORYINIT_H

#include "Factory.h"

class FactoryInit : public Factory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID KPluginFactory_iid FILE "calligraplanportfoliopart.json")
    Q_INTERFACES(KPluginFactory)

public:
    explicit FactoryInit() : Factory() {}
    ~FactoryInit() override {}

};

#endif
