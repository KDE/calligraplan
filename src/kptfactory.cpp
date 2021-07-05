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

#include <kiconloader.h>

#include <QTimer>

namespace KPlato
{

Factory::Factory()
    : KoFactory()
{
}

Factory::~Factory()
{
}

QObject* Factory::create(const char* /*iface*/, QWidget* /*parentWidget*/, QObject *parent,
                             const QVariantList& args, const QString& keyword)
{
    Q_UNUSED(args);
    Q_UNUSED(keyword);

    Part *part = new Part(parent);
    MainDocument *doc = new MainDocument(part);
    part->setDocument(doc);

    // start checking for workpackages
    QTimer *timer = new QTimer(doc);
    connect(timer, &QTimer::timeout, doc, &MainDocument::autoCheckForWorkPackages);
    timer->start(5000);

    return part;
}

} // KPlato namespace
