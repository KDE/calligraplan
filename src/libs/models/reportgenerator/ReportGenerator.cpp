/* This file is part of the KDE project
  SPDX-FileCopyrightText: 2017 Dag Andersen <dag.andersen@kdemail.net>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/

// clazy:excludeall=qstring-arg
#include "planmodels_export.h"
#include "ReportGenerator.h"
#include "ReportGeneratorDebug.h"

#include <kptproject.h>
#include <kptschedule.h>
#include <kptnodeitemmodel.h>
#include <kpttaskstatusmodel.h>
#include <kptitemmodelbase.h>
#include <kptnodechartmodel.h>
#include <kptschedulemodel.h>

#include <KLocalizedString>

#include <QFile>

namespace KPlato
{

QAbstractItemModel *translationModel()
{
    QList<QPair<QString, QString> > names;
    names << QPair<QString, QString>(QStringLiteral("Project"), i18n("Project"))
    << QPair<QString, QString>(QStringLiteral("Manager"), i18n("Manager"))
    << QPair<QString, QString>(QStringLiteral("Schedule"), i18n("Schedule"))
    << QPair<QString, QString>(QStringLiteral("BCWS"), xi18nc("@title:column Budgeted Cost of Work Scheduled", "BCWS"))
    << QPair<QString, QString>(QStringLiteral("BCWP"), xi18nc("@title:column Budgeted Cost of Work Performed", "BCWP"))
    << QPair<QString, QString>(QStringLiteral("ACWP"), xi18nc("@title:column Actual Cost of Work Performed", "ACWP"))
    << QPair<QString, QString>(QStringLiteral("SPI"), xi18nc("@title:column Schedule Performance Index", "SPI"))
    << QPair<QString, QString>(QStringLiteral("CPI"), xi18nc("@title:column Cost Performance Index", "CPI"));

    QStandardItemModel *model = new QStandardItemModel(0, names.count());
    for (int column = 0; column < names.count(); ++column) {
        model->setHeaderData(column, Qt::Horizontal, names.at(column).first, HeaderRole);
        model->setHeaderData(column, Qt::Horizontal, names.at(column).second);
    }
    return model;
}

QAbstractItemModel *projectModel()
{
    QList<QPair<QString, QString> > names;
    names << QPair<QString, QString>(QStringLiteral("Name"), i18n("Name"))
    << QPair<QString, QString>(QStringLiteral("Manager"), i18n("Manager"))
    << QPair<QString, QString>(QStringLiteral("BCWS Cost"), i18nc("Cost based Budgeted Cost of Work Scheduled", "BCWS Cost"))
    << QPair<QString, QString>(QStringLiteral("BCWP Cost"), i18nc("Cost based Budgeted Cost of Work Performed", "BCWP Cost"))
    << QPair<QString, QString>(QStringLiteral("ACWP Cost"), i18nc("Cost based Actual Cost of Work Performed", "ACWP Cost"))
    << QPair<QString, QString>(QStringLiteral("SPI Cost"), i18nc("Cost based Schedule Performance Index", "SPI Cost"))
    << QPair<QString, QString>(QStringLiteral("CPI Cost"), i18nc("Cost based Cost Performance Index", "CPI Cost"))
    << QPair<QString, QString>(QStringLiteral("BCWS Effort"), i18nc("Effort based Budgeted Cost of Work Scheduled", "BCWS Effort"))
    << QPair<QString, QString>(QStringLiteral("BCWP Effort"), i18nc("Effort based Budgeted Cost of Work Performed", "BCWP Effort"))
    << QPair<QString, QString>(QStringLiteral("ACWP Effort"), i18nc("Effort based Actual Cost of Work Performed", "ACWP Effort"))
    << QPair<QString, QString>(QStringLiteral("SPI Effort"), i18nc("Effort based Schedule Performance Index", "SPI Effort"))
    << QPair<QString, QString>(QStringLiteral("CPI Effort"), i18nc("Effort based Cost Performance Index", "CPI Effort"));

    QStandardItemModel *model = new QStandardItemModel(0, names.count());
    for (int column = 0; column < names.count(); ++column) {
        model->setHeaderData(column, Qt::Horizontal, names.at(column).first, HeaderRole);
        model->setHeaderData(column, Qt::Horizontal, names.at(column).second);
    }
    return model;
}

QAbstractItemModel *scheduleModel()
{
    ScheduleItemModel m;
    QStandardItemModel *model = new QStandardItemModel(0, m.columnCount());
    for (int c = 0; c < m.columnCount(); ++c) {
        model->setHeaderData(c, Qt::Horizontal, m.headerData(c, Qt::Horizontal));
        model->setHeaderData(c, Qt::Horizontal, m.headerData(c, Qt::Horizontal, Qt::EditRole), HeaderRole);
    }
    return model;
}

ReportGenerator::ReportGenerator()
    : m_project(nullptr)
    , m_manager(nullptr)
    , m_reportGenerator(nullptr)
{
    m_keys = QStringList() << QStringLiteral("table") << QStringLiteral("chart");
    m_variables = QStringList() << QStringLiteral("project") << QStringLiteral("schedule");

    m_basemodels
    << new NodeItemModel()
    << new TaskStatusItemModel()
    << new ChartItemModel()
    << new ScheduleItemModel()
    << new NodeItemModel();

    addDataModel(QStringLiteral("tasks"), m_basemodels.at(0), Qt::EditRole);
    addDataModel(QStringLiteral("taskstatus"), m_basemodels.at(1), Qt::EditRole);
    addDataModel(QStringLiteral("chart.project"), m_basemodels.at(2), Qt::EditRole);
    addDataModel(QStringLiteral("projects"), projectsModel(m_basemodels.at(4)), Qt::EditRole);
    addDataModel(QStringLiteral("schedules"), m_basemodels.at(3), Qt::EditRole);
    addDataModel(QStringLiteral("project"), projectModel(), HeaderRole);
    addDataModel(QStringLiteral("schedule"), scheduleModel(), HeaderRole);
    addDataModel(QStringLiteral("tr"), translationModel(), HeaderRole);

//     for (QAbstractItemModel *m : m_datamodels) {
//         const QString key = m_datamodels.key(m);
//         qInfo()<<key<<"columns:"<<m->columnCount();
//         for (int i = 0; i < m->columnCount(); ++i) {
//             qInfo()<<'\t'<<i<<m->headerData(i, Qt::Horizontal, m_headerrole.value(key)).toString();
//         }
//     }
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

ChartItemModel *ReportGenerator::findChartItemModel(QSortFilterProxyModel &model)
{
    for (QAbstractProxyModel *p = &model; p; p = qobject_cast<QAbstractProxyModel*>(p->sourceModel())) {
        ChartItemModel *c = qobject_cast<ChartItemModel*>(p->sourceModel());
        if (c) {
            return c;
        }
    }
    return nullptr;
}

bool ReportGenerator::startsWith(const QStringList &keys, const QString &key)
{
    const auto lowerKey = key.toLower();
    for (const QString &k : keys) {
        if (lowerKey.startsWith(k)) {
            return true;
        }
    }
    return false;
}

QStringList ReportGenerator::trimmed(const QStringList &lst)
{
    QStringList rlst;
    for (const QString &s : lst) {
        QString r = s.trimmed();
        if (!r.isEmpty()) {
            rlst << r;

        }
    }
    return rlst;
}

void initProjectModel(QAbstractItemModel *model, Project *project, ScheduleManager *sm)
{
    if (model->rowCount() == 0) {
        model->insertRow(0);
    }
    QModelIndex idx = model->index(0, 0);
    model->setData(idx, project->name());
    idx = model->index(0, 1);
    model->setData(idx, project->leader());

    PerformanceDataCurrentDateModel m(nullptr);
    m.setProject(project);
    m.setScheduleManager(sm);
    m.setNodes(QList<Node*>() << project);

    int col = 2; // column of BCWS Cost
    for (int r = 0; r < 2; ++r) {
        for (int c = 0; c < 5; ++c) {
            idx = model->index(0, col++);
            QModelIndex cidx = m.index(r, c);
            model->setData(idx, cidx.data());
        }
    }
}

void initScheduleModel(QAbstractItemModel *model, Project *project, ScheduleManager *sm)
{
    ScheduleItemModel m;
    m.setProject(project);
    QModelIndex idx = m.index(sm);
    if (idx.isValid()) {
        if (model->rowCount() == 0) {
            model->insertRow(0);
        }
        for (QModelIndex i = idx; i.isValid(); i = i.sibling(i.row(), i.column() + 1)) {
            QModelIndex midx = model->index(0, i.column());
            model->setData(midx, i.data());
            dbgRGVariable<<model->headerData(midx.column(), Qt::Horizontal, HeaderRole).toString()<<'='<<i.data(Qt::UserRole+1).toString();
        }
    } else dbgRGVariable<<"Could not find schedule"<<sm;
    dbgRGVariable<<model->rowCount()<<model->columnCount();
}

QAbstractItemModel *ReportGenerator::projectsModel(ItemModelBase *base) const
{
    static_cast<NodeItemModel*>(base)->setShowProject(true);

    auto model = new QSortFilterProxyModel();
    model->setSourceModel(base);
    model->setFilterRole(Qt::EditRole);
    model->setFilterFixedString(QString::number(Node::Type_Project));
    model->setFilterKeyColumn(NodeModel::NodeType);
    return model;
}

void ReportGenerator::addDataModel(const QString &name, QAbstractItemModel *model, int role)
{
    model->setObjectName(name);
    m_datamodels[name] = model;
    m_headerrole[name] = role;
}

bool ReportGenerator::initiate()
{
    m_lastError.clear();
    if (m_templateFile.isEmpty()) {
        m_lastError = i18n("Missing report template file");
        return false;
    }
    if (m_reportFile.isEmpty()) {
        m_lastError = i18n("Missing report result file");
        return false;
    }
    if (!QFile::exists(m_templateFile)) {
        m_lastError = i18n("Report template file does not exist");
        return false;
    }

    for (ItemModelBase *m : std::as_const(m_basemodels)) {
        m->setProject(m_project);
        m->setScheduleManager(m_manager);
        if (qobject_cast<ChartItemModel*>(m)) {
            static_cast<ChartItemModel*>(m)->setNodes(QList<Node*>() << m_project);
            dbgRGChart<<"chart:"<<m_project<<m_manager<<"set nodes"<<m_project;
        }
    }
    initProjectModel(m_datamodels[QStringLiteral("project")], m_project, m_manager);
    initScheduleModel(m_datamodels[QStringLiteral("schedule")], m_project, m_manager);
    static_cast<ScheduleItemModel*>(m_datamodels[QStringLiteral("schedules")])->setProject(m_project);
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
