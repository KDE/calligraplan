/* This file is part of the KDE project
   Copyright (C) 1998, 1999, 2000 Torben Weis <weis@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
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
