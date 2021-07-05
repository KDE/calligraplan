/* This file is part of the KDE project
   SPDX-FileCopyrightText: 1998, 1999, 2000 Torben Weis <weis@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "KoFactory.h"

#include <KoResourcePaths.h>
#include <KoComponentData.h>

#include "kptaboutdata.h"
#include "MainDebug.h"

KoComponentData* KoFactory::s_global = nullptr;
KAboutData* KoFactory::s_aboutData = nullptr;

KoFactory::KoFactory()
    : KPluginFactory()
{
    global();
}

KoFactory::~KoFactory()
{
    delete s_aboutData;
    s_aboutData = nullptr;
    delete s_global;
    s_global = nullptr;
}

QObject* KoFactory::create(const char* /*iface*/, QWidget* /*parentWidget*/, QObject *parent,
                             const QVariantList& args, const QString& keyword)
{
    Q_UNUSED(args);
    Q_UNUSED(keyword);
    return nullptr;
}

KAboutData* KoFactory::aboutData()
{
    if (!s_aboutData)
        s_aboutData = KPlato::newAboutData();
    return s_aboutData;
}

const KoComponentData &KoFactory::global()
{
    if (!s_global)
    {
        debugMain;
        s_global = new KoComponentData(*aboutData());

        // Add any application-specific resource directories here
        KoResourcePaths::addResourceType("calligraplan_taskmodules", "data", "calligraplan/taskmodules/");

        // Tell the iconloader about share/apps/calligra/icons
//        KIconLoader::global()->addAppDir("calligra");

//        KoDockRegistry *dockRegistry = KoDockRegistry::instance();
//        dockRegistry->remove("StencilBox"); //don't want this in plan
    }
    return *s_global;
}
