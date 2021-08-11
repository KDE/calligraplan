/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

// clazy:excludeall=qstring-arg
#include "Factory.h"
#include "MainDocument.h"
#include "Part.h"

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
    part->setDocument(part->createDocument(part));

    return part;
}
