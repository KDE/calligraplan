/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2022 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef REPORTGENERATORFACTORY_H
#define REPORTGENERATORFACTORY_H

#include "planmodels_export.h"

class QString;

namespace KPlato
{

class ReportGenerator;


class PLANMODELS_EXPORT ReportGeneratorFactory
{
public:
    explicit ReportGeneratorFactory();
    virtual ~ReportGeneratorFactory();

    virtual ReportGenerator *createReportGenerator(const QString &type);

protected:

};

} //namespace KPlato

#endif // REPORTGENERATORFACTORY_H
