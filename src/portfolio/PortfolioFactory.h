/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2021 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PORTFOLIO_FACTORY_H
#define PORTFOLIO_FACTORY_H

#include "planportfolio_export.h"

#include <KPluginFactory>

class KAboutData;
class KoComponentData;

class PLANPORTFOLIO_EXPORT PortfolioFactory : public KPluginFactory
{
    Q_OBJECT
public:
    explicit PortfolioFactory();
    ~PortfolioFactory() override;

    QObject* create(const char* iface, QWidget* parentWidget, QObject *parent, const QVariantList& args) override;

    static const KoComponentData &global();

    static KAboutData* aboutData();

private:
    static KoComponentData* s_global;
    static KAboutData* s_aboutData;

};

#endif
