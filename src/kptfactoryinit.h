/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2015 Friedrich W. H. Kossebau <kossebau@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KPTFACTORYINIT_H
#define KPTFACTORYINIT_H

#include "kptfactory.h"

class KPlatoFactoryInit : public KPlato::Factory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID KPluginFactory_iid FILE "planpart.json")
    Q_INTERFACES(KPluginFactory)

public:
    explicit KPlatoFactoryInit() : KPlato::Factory() {}
    ~KPlatoFactoryInit() override {}

};

#endif
