/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "PortfolioFactory.h"
#include "MainDocument.h"
#include "Part.h"
#include "AboutData.h"

#include <KoResourcePaths.h>
#include <KoComponentData.h>

KoComponentData* PortfolioFactory::s_global = nullptr;
KAboutData* PortfolioFactory::s_aboutData = nullptr;

PortfolioFactory::PortfolioFactory()
    : KPluginFactory()
{
    global();
}

PortfolioFactory::~PortfolioFactory()
{
    delete s_aboutData;
    s_aboutData = nullptr;
    delete s_global;
    s_global = nullptr;
}

QObject* PortfolioFactory::create(const char* /*iface*/, QWidget* /*parentWidget*/, QObject *parent,
                             const QVariantList& args)
{
    Q_UNUSED(args);

    Part *part = new Part(parent);
    MainDocument *doc = new MainDocument(part);
    part->setDocument(doc);

    return part;
}

KAboutData* PortfolioFactory::aboutData()
{
    if (!s_aboutData)
        s_aboutData = newAboutData();
    return s_aboutData;
}

const KoComponentData &PortfolioFactory::global()
{
    if (!s_global)
    {
        s_global = new KoComponentData(*aboutData());

        // Add any application-specific resource directories here

        // Tell the iconloader about share/apps/calligra/icons
//        KIconLoader::global()->addAppDir("calligra");

//        KoDockRegistry *dockRegistry = KoDockRegistry::instance();
//        dockRegistry->remove("StencilBox"); //don't want this in plan
    }
    return *s_global;
}
