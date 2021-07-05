/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2017 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef REPORTGENERATOR_H
#define REPORTGENERATOR_H

#include <QModelIndexList>

#include <KoXmlReaderForward.h>

#include <qdom.h>
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

    virtual bool open();
    virtual void close();

    virtual bool createReport();

    QString lastError() const;

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
