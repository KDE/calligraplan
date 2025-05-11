/* This file is part of the KDE project
   SPDX-FileCopyrightText: 1998, 1999, 2000 Torben Weis <weis@kde.org>
   SPDX-FileCopyrightText: 2009 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPLATOWORK_FACTORY_H
#define KPLATOWORK_FACTORY_H

#include "planwork_export.h"

#include <KPluginFactory>

class KAboutData;
class KoComponentData;

namespace KPlatoWork
{

class PLANWORK_EXPORT Factory : public KPluginFactory
{
    Q_OBJECT
public:
    explicit Factory();
    ~Factory() override;

    QObject* create(const char* iface, QWidget* parentWidget, QObject *parent, const QVariantList& args) override;

    static const KoComponentData &global();

    static KAboutData* aboutData();

private:
    static KoComponentData* s_global;
    static KAboutData* s_aboutData;
};

} // KPlatoWork namespace

#endif
