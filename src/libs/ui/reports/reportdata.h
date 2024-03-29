/*
* KPlato Report Plugin
* SPDX-FileCopyrightText: 2007-2009 Adam Pigg (adam@piggz.co.uk)
* SPDX-FileCopyrightText: 2010 Dag Andersen <dag.andersen@kdemail.net>
* SPDX-FileCopyrightText: 2016 Dag Andersen <dag.andersen@kdemail.net>
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef KPLATO_REPORTDATA_H
#define KPLATO_REPORTDATA_H

#include "planui_export.h"

#include <KReportData>

#include "kptitemmodelbase.h"
#include "kptnodeitemmodel.h"
#include "kptnodechartmodel.h"
#include "kptproject.h"

#include <QSortFilterProxyModel>
#include <QPointer>
#include <QString>
#include <QStringList>

namespace KPlato
{

class ScheduleManager;
class ReportData;
class ChartItemModel;

namespace Report
{
    PLANUI_EXPORT QList<ReportData*> createBaseReportDataModels(QObject *parent = 0);
    PLANUI_EXPORT ReportData *findReportData(const QList<ReportData*> &lst, const QString &type);
}

class PLANUI_EXPORT ReportData : public QObject, public KReportData
{
    Q_OBJECT
public:
    explicit ReportData(QObject *parent = 0);
    ReportData(const ReportData &other);
    virtual ~ReportData();

    /// Re-implement this to create a clone of your report data object
    /// Returns 0 by default
    virtual ReportData *clone() const { return 0; }

    /// Set the @p role that shall be used when fetching data for @p column
    /// Default is Qt::DisplayRole
    void setColumnRole(int column, int role);

    //!Open the dataset
    virtual bool open();

    //!Close the dataset
    virtual bool close();

    //!Move to the next record
    virtual bool moveNext();

    //!Move to the previous record
    virtual bool movePrevious();

    //!Move to the first record
    virtual bool moveFirst();

    //!Move to the last record
    virtual bool moveLast();

    //!Return the current position in the dataset
    virtual qint64 at() const;

    //!Return the total number of records
    virtual qint64 recordCount() const;

    //!Return the index number of the field given by name field
    virtual int fieldNumber(const QString &field) const;

    //!Return the list of field names
    virtual QStringList fieldNames() const;
    //!Return the list of field keys
    virtual QStringList fieldKeys() const;

    //!Return the value of the field at the given position for the current record
    virtual QVariant value(unsigned int) const;

    //!Return the value of the field fir the given name for the current record
    virtual QVariant value(const QString &field) const;

    //!Return the name of this source
    virtual QString sourceName() const;

    //!Sets the sorting for the data
    //!Should be called before open() so that the data source can be edited accordingly
    //!Default impl does nothing
    virtual void setSorting(const QList<SortedField>& sorting);

    //!Return a list of data sources possible for advanced controls
    virtual QStringList dataSources() const;
    //!Return a list of data source names possible for advanced controls
    virtual QStringList dataSourceNames() const;

    //!Allow a driver to create a new instance with a new data source
    //!source is a driver specific identifier
    //!Owner of the returned pointer is the caller
    virtual KReportData* data(const QString &source);

    void setModel(QAbstractItemModel *model);
    QAbstractItemModel *model() const;
    ItemModelBase *itemModel() const;

    Project *project() const { return m_project; }
    ScheduleManager *scheduleManager() const { return m_schedulemanager; }

    virtual bool loadXml(const KoXmlElement &/*element*/) { return true; }
    virtual void saveXml(QDomElement &/*element*/) const {}

    bool isMainDataSource() const { return m_maindatasource; }
    bool isSubDataSource() const { return m_subdatasource; }
    void setSubDataSources(QList<ReportData*> &lst) { m_subdatasources = lst; }

public Q_SLOTS:
    virtual void setProject(Project *project);
    virtual void setScheduleManager(ScheduleManager *sm);

Q_SIGNALS:
    void scheduleManagerChanged(ScheduleManager *sm);
    void createReportData(const QString &type, ReportData *rd);

protected:
    /// Re-implement this to create data models
    virtual void createModels() {}
    ReportData *getReportData(const QString &tag) const;

protected:
    QSortFilterProxyModel m_model;
    long m_row;
    Project *m_project;
    ScheduleManager *m_schedulemanager;

    QMap<QString, QVariant> m_expressions;

    QMap<int, int> m_columnroles;
    QString m_name;
    QList<SortedField> m_sortlist;
    QList<QAbstractItemModel*> m_sortmodels;
    bool m_maindatasource;
    bool m_subdatasource;
    QList<ReportData*> m_subdatasources;
    mutable QMap<QString, ReportData*> m_datasources;
};

class PLANUI_EXPORT TaskReportData : public ReportData
{
    Q_OBJECT
public:
    explicit TaskReportData(QObject *parent = 0);
    TaskReportData(const TaskReportData &other);

    bool loadXml(const KoXmlElement &element);
    void saveXml(QDomElement &element) const;

    ReportData *clone() const;

protected:
    void createModels();
};

class PLANUI_EXPORT TaskStatusReportData : public ReportData
{
    Q_OBJECT
public:
    explicit TaskStatusReportData(QObject *parent = 0);
    TaskStatusReportData(const TaskStatusReportData &other);

    bool loadXml(const KoXmlElement &element);
    void saveXml(QDomElement &element) const;

    ReportData *clone() const;

protected:
    void createModels();
};

class PLANUI_EXPORT ResourceReportData : public ReportData
{
    Q_OBJECT
public:
    explicit ResourceReportData(QObject *parent = 0);
    ResourceReportData(const ResourceReportData &other);

    bool loadXml(const KoXmlElement &element);
    void saveXml(QDomElement &element) const;

    ReportData *clone() const;

protected:
    void createModels();
};

class PLANUI_EXPORT ResourceAssignmentReportData : public ReportData
{
    Q_OBJECT
public:
    explicit ResourceAssignmentReportData(QObject *parent = 0);
    ResourceAssignmentReportData(const ResourceAssignmentReportData &other);

    bool loadXml(const KoXmlElement &element);
    void saveXml(QDomElement &element) const;

    ReportData *clone() const;

protected:
    void createModels();
};

class PLANUI_EXPORT ChartReportData : public ReportData
{
    Q_OBJECT
public:
    explicit ChartReportData(QObject *parent = 0);
    ChartReportData(const ChartReportData &other);

    /// Prepare the data for access
    virtual bool open();

    //!Move to the next record
    virtual bool moveNext();

    //!Move to the previous record
    virtual bool movePrevious();

    //!Move to the first record
    virtual bool moveFirst();

    //!Move to the last record
    virtual bool moveLast();

    //!Return the total number of records
    virtual qint64 recordCount() const;

    //!Return the value of the field at the given position for the current record
    virtual QVariant value(unsigned int) const;
    //!Return the value of the field named @p name
    QVariant value(const QString &name) const;

    //!Return the list of field names, used for legends in a chart
    virtual QStringList fieldNames() const;

    void addExpression(const QString &field, const QVariant &value, char relation = '=');

    bool cbs;

    bool loadXml(const KoXmlElement &element);
    void saveXml(QDomElement &element) const;

protected:
    int firstRow();
    int lastRow() const;

    int m_firstrow;
    int m_lastrow;
    QDate m_startdate;
    QStringList m_keywords;
    bool m_fakedata;
};

class PLANUI_EXPORT CostPerformanceReportData : public ChartReportData
{
    Q_OBJECT
public:
    explicit CostPerformanceReportData(QObject *parent = 0);
    CostPerformanceReportData(const CostPerformanceReportData &other);

    ReportData *clone() const;

    virtual bool open();

protected:
    void createModels();

private:
    ChartItemModel *m_chartmodel;
};

class PLANUI_EXPORT EffortPerformanceReportData : public ChartReportData
{
    Q_OBJECT
public:
    explicit EffortPerformanceReportData(QObject *parent = 0);
    EffortPerformanceReportData(const EffortPerformanceReportData &other);

    ReportData *clone() const;

    virtual bool open();

protected:
    void createModels();

private:
    ChartItemModel *m_chartmodel;
};

class PLANUI_EXPORT CostBreakdownReportData : public ChartReportData
{
    Q_OBJECT
public:
    explicit CostBreakdownReportData(QObject *parent = 0);
    CostBreakdownReportData(const CostBreakdownReportData &other);

    ReportData *clone() const;

    bool open();

protected:
    void createModels();
};

class PLANUI_EXPORT ProjectReportData : public ReportData
{
    Q_OBJECT
public:
    explicit ProjectReportData(QObject *parent = 0);
    ProjectReportData(const ProjectReportData &other);

    ReportData *clone() const;

    //!Move to the next record
    virtual bool moveNext();

    //!Move to the first record
    virtual bool moveFirst();

    //!Move to the last record
    virtual bool moveLast();

    //! return number of records
    qint64 recordCount() const;

    QStringList fieldNames() const;
    QStringList fieldKeys() const;

    using ReportData::value;
    //!Return the value of the field for the given column
    virtual QVariant value(int column) const;
    //!Return the value of the field for the given name for the current record
    virtual QVariant value(const QString &field) const;

public Q_SLOTS:
    virtual void setProject(Project *project);
    virtual void setScheduleManager(ScheduleManager *sm);

protected:
    void createModels();

private:
    NodeModel m_data;
    QMap<int, QString> m_keys;
    QMap<int, QString> m_names;

};

} //namespace KPlato

#endif
