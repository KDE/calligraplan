/* This file is part of the KDE project
   SPDX-FileCopyrightText: 1998, 1999, 2000 Torben Weis <weis@kde.org>
   SPDX-FileCopyrightText: 2009 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "factory.h"
#include "part.h"
#include "aboutdata.h"

#include <KoResourcePaths.h>
#include <KoComponentData.h>

#include <KIconLoader>


namespace KPlatoWork
{

KoComponentData* Factory::s_global = nullptr;
KAboutData* Factory::s_aboutData = nullptr;

Factory::Factory()
    : KPluginFactory()
{
    global();
}

Factory::~Factory()
{
    delete s_aboutData;
    s_aboutData = nullptr;
    delete s_global;
    s_global = nullptr;
}

QObject* Factory::create(const char* iface, QWidget* parentWidget, QObject *parent,
                             const QVariantList& args)
{
    Q_UNUSED(args);
    Q_UNUSED(iface);
    Part *part = new Part(parentWidget, parent);
    return part;
}

KAboutData* Factory::aboutData()
{
    if (!s_aboutData)
        s_aboutData = newAboutData();
    return s_aboutData;
}

const KoComponentData &Factory::global()
{
    if (!s_global)
    {
        s_global = new KoComponentData(*aboutData());

        // Add any application-specific resource directories here
        KoResourcePaths::addResourceType("planwork_template", "data", QStringLiteral("calligraplanwork/templates/"));
        KoResourcePaths::addResourceType("projects", "data", QStringLiteral("calligraplanwork/projects/"));

        // Tell the iconloader about share/apps/calligra/icons
        KIconLoader::global()->addAppDir(QStringLiteral("calligra"));

    }
    return *s_global;
}

} // KPlatoWork namespace
