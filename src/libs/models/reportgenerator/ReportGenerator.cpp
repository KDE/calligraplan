/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2017 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "planmodels_export.h"
#include "ReportGenerator.h"

#include "ReportGeneratorOdt.h"


#include <KLocalizedString>

namespace KPlato
{

ReportGenerator::ReportGenerator()
    : m_project(nullptr)
    , m_manager(nullptr)
    , m_reportGenerator(nullptr)
{
}


ReportGenerator::~ReportGenerator()
{
    delete m_reportGenerator;
}

void ReportGenerator::setReportType(const QString &type)
{
    m_reportType = type;
}

void ReportGenerator::setTemplateFile(const QString &file)
{
    m_templateFile = file;
}

void ReportGenerator::setReportFile(const QString &file)
{
    m_reportFile = file;
}

void ReportGenerator::setProject(Project *project)
{
    m_project = project;
}

void ReportGenerator::setScheduleManager(ScheduleManager *manager)
{
    m_manager = manager;
}

bool ReportGenerator::initiate()
{
    m_lastError.clear();
    return initiateInternal();
}

bool ReportGenerator::initiateInternal()
{
    return true;
}

QString ReportGenerator::lastError() const
{
    if (m_reportGenerator) {
        return m_reportGenerator->lastError();
    }
    return m_lastError;
}

} //namespace KPlato
