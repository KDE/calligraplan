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
    close();
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

bool ReportGenerator::open()
{
    m_lastError.clear();
    if (m_reportType == "odt") {
        m_reportGenerator = new ReportGeneratorOdt();
        m_reportGenerator->setTemplateFile(m_templateFile);
        m_reportGenerator->setReportFile(m_reportFile);
        m_reportGenerator->setProject(m_project);
        m_reportGenerator->setScheduleManager(m_manager);
        return m_reportGenerator->open();
    }
    m_lastError = i18n("Unknown report type: %1", m_reportType);
    return false;
}

void ReportGenerator::close()
{
    delete m_reportGenerator;
    m_reportGenerator = nullptr;
}

bool ReportGenerator::createReport()
{
    if (!m_reportGenerator) {
        m_lastError = i18n("The report generator has not been opened successfully.");
        return false;
    }
    return m_reportGenerator->createReport();
}

QString ReportGenerator::lastError() const
{
    if (m_reportGenerator) {
        return m_reportGenerator->lastError();
    }
    return m_lastError;
}

} //namespace KPlato
