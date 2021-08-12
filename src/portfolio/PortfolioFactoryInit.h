/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PORTFOLIO_FACTORYINIT_H
#define PORTFOLIO_FACTORYINIT_H

#include "PortfolioFactory.h"

class PortfolioFactoryInit : public PortfolioFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID KPluginFactory_iid FILE "calligraplanportfoliopart.json")
    Q_INTERFACES(KPluginFactory)

public:
    explicit PortfolioFactoryInit() : PortfolioFactory() {}
    ~PortfolioFactoryInit() override {}

};

#endif
