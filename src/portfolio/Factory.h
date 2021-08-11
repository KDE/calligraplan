/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PLANPORTFOLIO_FACTORY_H
#define PLANPORTFOLIO_FACTORY_H

#include "planportfolio_export.h"

#include <KoFactory.h>

class PLANPORTFOLIO_EXPORT Factory : public KoFactory
{
    Q_OBJECT
public:
    explicit Factory();
    ~Factory() override;

    QObject* create(const char* iface, QWidget* parentWidget, QObject *parent, const QVariantList& args, const QString& keyword) override;
};

#endif
