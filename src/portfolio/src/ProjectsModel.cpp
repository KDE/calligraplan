/* This file is part of the KDE project
* Copyright (C) 2021 Dag Andersen <dag.andersen@kdemail.net>
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Library General Public
* License as published by the Free Software Foundation; either
* version 2 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Library General Public License for more details.
*
* You should have received a copy of the GNU Library General Public License
* along with this library; see the file COPYING.LIB.  If not, write to
* the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
* Boston, MA 02110-1301, USA.
*/

#include "ProjectsModel.h"
#include <MainDocument.h>

#include <kptproject.h>

#include <KoIcon.h>

ProjectsFilterModel::ProjectsFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    m_baseModel = new ProjectsModel(this);
    setSourceModel(m_baseModel);
}

ProjectsFilterModel::~ProjectsFilterModel()
{
}

void ProjectsFilterModel::setAcceptedRows(const QList<int> rows)
{
    m_acceptedRows = rows;
}

void ProjectsFilterModel::setAcceptedColumns(const QList<int> columns)
{
    m_acceptedColumns = columns;
}

MainDocument *ProjectsFilterModel::portfolio() const
{
    return m_baseModel->portfolio();
}

void ProjectsFilterModel::setPortfolio(MainDocument *portfolio)
{
    m_baseModel->setPortfolio(portfolio);
    Q_EMIT portfolioChanged();
}

KoDocument *ProjectsFilterModel::documentFromIndex(const QModelIndex &idx) const
{
    return m_baseModel->documentFromIndex(mapToSource(idx));
}

bool ProjectsFilterModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    Q_UNUSED(source_parent)
    return m_acceptedRows.isEmpty() || m_acceptedRows.contains(source_row);
}

bool ProjectsFilterModel::filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const
{
    Q_UNUSED(source_parent)
    return m_acceptedColumns.isEmpty() || m_acceptedColumns.contains(source_column);
}

//-----------------------------
ProjectsModel::ProjectsModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_portfolio(nullptr)
{
}

ProjectsModel::~ProjectsModel()
{
}

bool ProjectsModel::hasChildren(const QModelIndex &parent) const
{
    return !parent.isValid() && rowCount(parent) > 0;
}

int ProjectsModel::rowCount(const QModelIndex &parent) const
{
    int rows = m_portfolio && !parent.isValid() ? m_portfolio->documents().count() : 0;
    return rows;
}

int ProjectsModel::columnCount(const QModelIndex &parent) const
{
    return m_nodeModel.propertyCount() + 1;
}

Qt::ItemFlags ProjectsModel::flags(const QModelIndex &idx) const
{
    Qt::ItemFlags f = QAbstractItemModel::flags(idx);
    KoDocument *doc = documentFromRow(idx.row());
    if (doc) {
        switch (idx.column()) {
            case KPlato::NodeModel::NodeConstraintStart: {
                f |= Qt::ItemIsEditable;
                break;
            }
            case KPlato::NodeModel::NodeConstraintEnd: {
                f |= Qt::ItemIsEditable;
                break;
            }
            default: {
                int extraColumn = idx.column() - m_nodeModel.propertyCount();
                switch (extraColumn) {
                    case ScheduleManagerColumn: {
                        f |= Qt::ItemIsEditable;
                        break;
                    }
                    default: break;
                }
                break;
            }
        }
    }
    return f;
}

QVariant ProjectsModel::headerData(int section, Qt::Orientation o, int role) const
{
    int extraSection = section - m_nodeModel.propertyCount();
    switch (extraSection) {
        case ScheduleManagerColumn: {
            switch (role) {
                case Qt::DisplayRole:
                    return i18n("Schedule");
                case Qt::ToolTipRole:
                    return i18nc("@info:tooltip", "Select schedule");
                default:
                    return QVariant();
            }
        }
        default: break;
    }
    QVariant v = m_nodeModel.headerData(section, role);
    return v;
}

QVariant ProjectsModel::data(const QModelIndex &idx, int role) const
{
    KoDocument *doc = documentFromRow(idx.row());
    if (!doc) {
        return QVariant();
    }
    switch (role) {
        case DOCUMENT_ROLE: {
            QVariant v = QVariant::fromValue<KoDocument*>(doc);
            qInfo()<<Q_FUNC_INFO<<v;
            return v;
        }
        case PROJECT_ROLE:
            return QVariant::fromValue<void*>(doc->project());
        case ISPORTFOLIO_ROLE:
            return doc->property(ISPORTFOLIO);
        default: break;
    }
    KPlato::ScheduleManager *sm = m_portfolio->scheduleManager(doc);
    switch (idx.column()) {
        case KPlato::NodeModel::NodeName: {
            switch (role) {
                case Qt::ToolTipRole: {
                    auto id = sm ? sm->scheduleId() : NOTSCHEDULED;
                    KPlato::Project *project = doc->project();
                    QString s = xi18nc("@info:tooltip",
                                       "<title>Project: %1</title>"
                                       "<para>Schedule: %6</para>"
                                       "<para><list>"
                                       "<item>Planned start:\t%2</item>"
                                       "<item>Planned finish:\t%3</item>"
                                       "<item>Target start:\t%4</item>"
                                       "<item>Target finish:\t%5</item>"
                                       "</list></para>",
                                       project->name(),
                                       project->startTime(id).toString(),
                                       project->endTime(id).toString(),
                                       project->constraintStartTime().toString(),
                                       project->constraintEndTime().toString(),
                                       (sm ? sm->name() : i18n("No schedule"))
                    );
                    return s;
                }
                default:
                    break;
            }
        }
    }
    int extraColumn = idx.column() - m_nodeModel.propertyCount();
    switch (extraColumn) {
        case ScheduleManagerColumn: {
            const QString name = doc->property(SCHEDULEMANAGERNAME).toString();
            const KPlato::Project *project = doc->project();
            const KPlato::ScheduleManager *sm = project->findScheduleManagerByName(name);
            switch (role) {
                case Qt::DisplayRole:
                    return name;
                case Qt::ToolTipRole:
                    if (sm) {
                        if (sm->isBaselined()) {
                            return i18nc("@info:tooltip", "Schedule is baselined");
                        }
                        if (sm->isScheduled()) {
                            return i18nc("@info:tooltip", "Scheduled but not baselined");
                        }
                    }
                    return i18nc("@info:tooltip", "Not scheduled");
                case Qt::DecorationRole:
                    if (sm) {
                        if (sm->isBaselined()) {
                            return koIcon("view-time-schedule-baselined");
                        }
                        if (sm->isScheduled()) {
                            return koIcon("view-time-schedule-calculus");
                        }
                        return koIcon("list-remove");
                    }
                    return QVariant();
                case KPlato::Role::EnumList: {
                    QStringList lst;
                    for (const KPlato::ScheduleManager *sm : project->allScheduleManagers()) {
                        lst << sm->name();
                    }
                    return lst;
                }
                case KPlato::Role::EnumListValue: {
                    QStringList lst;
                    for (const KPlato::ScheduleManager *sm : project->allScheduleManagers()) {
                        lst << sm->name();
                    }
                    return lst.indexOf(doc->property(SCHEDULEMANAGERNAME).toString());
                }
                default:
                    return QVariant();
            }
        }
        default: break;
    }
    KPlato::Project *project = doc->project();
    const_cast<ProjectsModel*>(this)->m_nodeModel.setProject(project);
    const_cast<ProjectsModel*>(this)->m_nodeModel.setManager(sm);
    return m_nodeModel.data(project, idx.column(), role);
}

bool ProjectsModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    KoDocument *doc = documentFromRow(idx.row());
    if (!doc) {
        return false;
    }
    if (role == Qt::EditRole) {
        switch (idx.column()) {
            case KPlato::NodeModel::NodeConstraintStart: {
                break;
            }
            case KPlato::NodeModel::NodeConstraintEnd: {
                QDateTime dt = value.toDateTime();
                if (dt.isValid()) {
                    doc->project()->setConstraintEndTime(dt);
                    Q_EMIT dataChanged(idx, idx);
                    doc->setModified(true);
                    return true;
                }
                break;
            }
            default: {
                int extraColumn = idx.column() - m_nodeModel.propertyCount();
                switch (extraColumn) {
                    case ScheduleManagerColumn: {
                        KPlato::Project *project = doc->project();
                        KPlato::ScheduleManager *sm = project->allScheduleManagers().value(value.toInt());
                        doc->setProperty(SCHEDULEMANAGERNAME, sm ? QVariant(sm->name()) : QVariant());
                        Q_EMIT dataChanged(idx, idx);
                        m_portfolio->setModified(true);
                        return true;
                    }
                    default:
                        break;
                }
                break;
            }
        }
    }
    return false;
}

QModelIndex ProjectsModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return QModelIndex();
    }
    return createIndex(row, column);
}

QModelIndex ProjectsModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return QModelIndex();
}

bool ProjectsModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (parent.isValid()) {
        return false;
    }
    QList<KoDocument*> docs;
    for (int i = row; i < row + count; ++i) {
        KoDocument *doc = documentFromRow(i);
        if (doc) {
            docs << doc;
        }
    }
    if (docs.isEmpty()) {
        return false;
    }
    beginResetModel();
    for (KoDocument *doc : docs) {
        doc->setParent(nullptr);
    }
    endResetModel();
    return true;
}

MainDocument *ProjectsModel::portfolio() const
{
    return m_portfolio;
}

void ProjectsModel::setPortfolio(MainDocument *portfolio)
{
    beginResetModel();
    if (m_portfolio) {
        disconnect(m_portfolio, &MainDocument::documentChanged, this, &ProjectsModel::documentChanged);
        disconnect(m_portfolio, &MainDocument::projectChanged, this, &ProjectsModel::projectChanged);
    }
    m_portfolio = portfolio;
    if (m_portfolio) {
        connect(m_portfolio, &MainDocument::documentChanged, this, &ProjectsModel::documentChanged);
        connect(m_portfolio, &MainDocument::projectChanged, this, &ProjectsModel::projectChanged);
    }
    endResetModel();
    Q_EMIT portfolioChanged();
}

void ProjectsModel::reset()
{
    qInfo()<<Q_FUNC_INFO;
    beginResetModel();
    endResetModel();
    Q_EMIT portfolioChanged();
}

void ProjectsModel::documentChanged(KoDocument *doc, int row)
{
    Q_UNUSED(doc);
    const QModelIndex idx = this->index(row, 0);
    Q_EMIT dataChanged(idx, idx.siblingAtColumn(columnCount()-1));
}

void ProjectsModel::projectChanged(KoDocument *doc)
{
    qInfo()<<Q_FUNC_INFO<<doc;
    documentChanged(doc, m_portfolio->documents().indexOf(doc));
}

KoDocument *ProjectsModel::documentFromIndex(const QModelIndex &idx) const
{
    return m_portfolio ? m_portfolio->documents().value(idx.row()) : nullptr;
}

KoDocument *ProjectsModel::documentFromRow(int row) const
{
    return m_portfolio ? m_portfolio->documents().value(row) : nullptr;
}

KPlato::Project *ProjectsModel::projectFromIndex(const QModelIndex &idx) const
{
    KPlato::Project *project = nullptr;
    KoDocument *doc = documentFromRow(idx.row());
    if (doc) {
        project = doc->project();
    }
    return project;
}
