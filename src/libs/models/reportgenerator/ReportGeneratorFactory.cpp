/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2022 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "planmodels_export.h"

#include "ReportGeneratorFactory.h"
#include "ReportGenerator.h"
#include "ReportGeneratorOdt.h"


namespace KPlato
{

ReportGeneratorFactory::ReportGeneratorFactory()
{
}


ReportGeneratorFactory::~ReportGeneratorFactory()
{
}

ReportGenerator *ReportGeneratorFactory::createReportGenerator(const QString &type)
{
    if (type.toLower() == QStringLiteral("odt")) {
        return new ReportGeneratorOdt();
    }
    return nullptr;
}

} //namespace KPlato
