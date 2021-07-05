/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2019 Dag Andersen <dag.andersen@kdemail.net>
 * 
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PLANNERIMPORT_H
#define PLANNERIMPORT_H


#include <KoFilter.h>


#include <QObject>
#include <QVariantList>

class QByteArray;
class QDomDocument;

class KoDocument;

class PlannerImport : public KoFilter
{
    Q_OBJECT
public:
    PlannerImport(QObject* parent, const QVariantList &);
    ~PlannerImport() override {}

    KoFilter::ConversionStatus convert(const QByteArray& from, const QByteArray& to) override;

    bool loadPlanner(const QDomDocument &in, KoDocument *doc) const;
};

#endif // PLANNERIMPORT_H
