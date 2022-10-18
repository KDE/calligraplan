/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2017 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef REPORTGENERATOR_H
#define REPORTGENERATOR_H

#include "planmodels_export.h"

#include <QModelIndexList>

#include <KoXmlReaderForward.h>

#include <QDomDocument>
#include <QSortFilterProxyModel>

class QIODevice;
class QString;

class KoXmlWriter;
class KoStore;
class KoOdfWriteStore;
class KoOdfReadStore;

namespace KPlato
{

class Project;
class ScheduleManager;
class ItemModelBase;


class PLANMODELS_EXPORT ReportGenerator
{
public:
    explicit ReportGenerator();
    virtual ~ReportGenerator();

    void setReportType(const QString &type);

    void setTemplateFile(const QString &file);
    void setReportFile(const QString &file);
    void setProject(Project *project);
    void setScheduleManager(ScheduleManager *manager);

    bool initiate();
    virtual bool createReport() = 0;

    QString lastError() const;

protected:
    /// Re-implement this to initate your report generator
    virtual bool initiateInternal();

protected:
    QString m_lastError;
    QString m_reportType;
    QString m_templateFile;
    QString m_reportFile;
    Project *m_project;
    ScheduleManager *m_manager;

    ReportGenerator *m_reportGenerator;
};

} //namespace KPlato

#endif // REPORTGENERATOR_H
