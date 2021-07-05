/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2011 Dag Andersen <dag.andersen@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KPLATOIMPORT_H
#define KPLATOIMPORT_H


#include <KoFilter.h>


#include <QObject>
#include <QVariantList>

class QByteArray;

namespace KPlato
{
}

class KPlatoImport : public KoFilter
{
    Q_OBJECT
public:
    KPlatoImport(QObject* parent, const QVariantList &);
    ~KPlatoImport() override {}

    KoFilter::ConversionStatus convert(const QByteArray& from, const QByteArray& to) override;
};

#endif // KPLATOIMPORT_H
