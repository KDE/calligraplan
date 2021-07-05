/* This file is part of the KDE project
   SPDX-FileCopyrightText: 1998, 1999, 2000 Torben Weis <weis@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KOFACTORY_H
#define KOFACTORY_H

#include "komain_export.h"

#include <kpluginfactory.h>

class KAboutData;
class KoComponentData;

class KOMAIN_EXPORT KoFactory : public KPluginFactory
{
    Q_OBJECT
public:
    explicit KoFactory();
    ~KoFactory() override;

    QObject* create(const char* iface, QWidget* parentWidget, QObject *parent, const QVariantList& args, const QString& keyword) override;

    static const KoComponentData &global();

    static KAboutData* aboutData();

private:
    static KoComponentData* s_global;
    static KAboutData* s_aboutData;
};

#endif
