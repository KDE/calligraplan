/* This file is part of the KDE project
   SPDX-FileCopyrightText: 1998, 1999, 2000 Torben Weis <weis@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "kptfactory.h"
#include "kptmaindocument.h"
#include "kptpart.h"
#include "kptaboutdata.h"

#include <KoResourcePaths.h>
#include <KoDockRegistry.h>
#include <KoComponentData.h>

#include <KIconLoader>

#include <QTimer>

namespace KPlato
{

KoComponentData* Factory::s_global = nullptr;
KAboutData* Factory::s_aboutData = nullptr;

Factory::Factory()
    : KPluginFactory()
{
}

Factory::~Factory()
{
}

QObject* Factory::create(const char* /*iface*/, QWidget* /*parentWidget*/, QObject *parent,
                             const QVariantList& args)
{
    Q_UNUSED(args);

    Part *part = new Part(parent);
    MainDocument *doc = new MainDocument(part);
    part->setDocument(doc);

    // start checking for workpackages
    QTimer *timer = new QTimer(doc);
    connect(timer, &QTimer::timeout, doc, &MainDocument::autoCheckForWorkPackages);
    timer->start(5000);

    return part;
}

KAboutData* Factory::aboutData()
{
    if (!s_aboutData) {
        s_aboutData = KPlato::newAboutData();
    }
    return s_aboutData;
}

const KoComponentData &Factory::global()
{
    if (!s_global) {
        debugPlan;
        s_global = new KoComponentData(*aboutData());

        // Add any application-specific resource directories here
        KoResourcePaths::addResourceType("calligraplan_taskmodules", "data", QStringLiteral("calligraplan/taskmodules/"));
    }
    return *s_global;
}

} // KPlato namespace
